// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include "lattice/lattice.hpp"
#include "lattice/flat_square_lattice.hpp"
#include "logging/trivial_log.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"
#include "statistics/autocorrelation.hpp"
#include "statistics/bootstrap.hpp"

#include <boost/container/small_vector.hpp>

#include <algorithm> 
#include <chrono>
#include <cmath>
#include <concepts>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#ifdef PARATORIC_HAS_MPI
#include <mpi.h>
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace paratoric {

namespace checkpoint_detail {

inline constexpr char SAMPLE_CHECKPOINT_MAGIC[8] = {'P', 'T', 'C', 'K', 'P', 'T', '1', '\0'};
// M11: version 4 adds a collective completed-sample marker and cumulative PT
// diagnostics. Version 2/3 files remain readable through the legacy branches.
inline constexpr std::uint32_t SAMPLE_CHECKPOINT_VERSION = 4;
inline constexpr std::uint32_t SAMPLE_CHECKPOINT_MIN_SUPPORTED_VERSION = 2;

template<typename T>
void write_binary(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
void read_binary(std::istream& in, T& value) {
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
}

inline void write_string(std::ostream& out, const std::string& value) {
    const auto size = static_cast<std::uint64_t>(value.size());
    write_binary(out, size);
    if (!value.empty()) {
        out.write(value.data(), static_cast<std::streamsize>(value.size()));
    }
}

inline std::string read_string(std::istream& in) {
    std::uint64_t size = 0;
    read_binary(in, size);
    std::string value(static_cast<std::size_t>(size), '\0');
    if (!value.empty()) {
        in.read(value.data(), static_cast<std::streamsize>(value.size()));
    }
    return value;
}

inline void write_string_vector(std::ostream& out, const std::vector<std::string>& values) {
    const auto size = static_cast<std::uint64_t>(values.size());
    write_binary(out, size);
    for (const auto& value : values) {
        write_string(out, value);
    }
}

inline std::vector<std::string> read_string_vector(std::istream& in) {
    std::uint64_t size = 0;
    read_binary(in, size);
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(size));
    for (std::uint64_t i = 0; i < size; ++i) {
        values.emplace_back(read_string(in));
    }
    return values;
}

inline void write_diagnostics(std::ostream& out, const AcceptanceDiagnostics& diagnostics) {
    write_binary(out, diagnostics.attempted);
    write_binary(out, diagnostics.accepted);
    write_binary(out, diagnostics.acceptance_ratio_sum);
    auto write_u64_vector = [&out](const std::vector<std::uint64_t>& values) {
        const auto size = static_cast<std::uint64_t>(values.size());
        write_binary(out, size);
        if (!values.empty()) {
            out.write(reinterpret_cast<const char*>(values.data()),
                      static_cast<std::streamsize>(values.size() * sizeof(std::uint64_t)));
        }
    };
    auto write_double_vector = [&out](const std::vector<double>& values) {
        const auto size = static_cast<std::uint64_t>(values.size());
        write_binary(out, size);
        if (!values.empty()) {
            out.write(reinterpret_cast<const char*>(values.data()),
                      static_cast<std::streamsize>(values.size() * sizeof(double)));
        }
    };
    write_u64_vector(diagnostics.attempted_by_update);
    write_u64_vector(diagnostics.accepted_by_update);
    write_double_vector(diagnostics.acceptance_ratio_sum_by_update);
    write_u64_vector(diagnostics.block_attempted);
    write_u64_vector(diagnostics.block_accepted);
    write_double_vector(diagnostics.block_acceptance_ratio_sum);
}

inline void read_diagnostics(std::istream& in, AcceptanceDiagnostics& diagnostics) {
    auto read_u64_vector = [&in]() {
        std::uint64_t size = 0;
        read_binary(in, size);
        std::vector<std::uint64_t> values(static_cast<std::size_t>(size));
        if (!values.empty()) {
            in.read(reinterpret_cast<char*>(values.data()),
                    static_cast<std::streamsize>(values.size() * sizeof(std::uint64_t)));
        }
        return values;
    };
    auto read_double_vector = [&in]() {
        std::uint64_t size = 0;
        read_binary(in, size);
        std::vector<double> values(static_cast<std::size_t>(size));
        if (!values.empty()) {
            in.read(reinterpret_cast<char*>(values.data()),
                    static_cast<std::streamsize>(values.size() * sizeof(double)));
        }
        return values;
    };
    read_binary(in, diagnostics.attempted);
    read_binary(in, diagnostics.accepted);
    read_binary(in, diagnostics.acceptance_ratio_sum);
    diagnostics.attempted_by_update = read_u64_vector();
    diagnostics.accepted_by_update = read_u64_vector();
    diagnostics.acceptance_ratio_sum_by_update = read_double_vector();
    diagnostics.block_attempted = read_u64_vector();
    diagnostics.block_accepted = read_u64_vector();
    diagnostics.block_acceptance_ratio_sum = read_double_vector();
}

inline void write_observable_series(
    std::ostream& out,
    const std::vector<std::vector<std::variant<std::complex<double>, double>>>& series
) {
    const auto outer = static_cast<std::uint64_t>(series.size());
    write_binary(out, outer);
    for (const auto& obs_series : series) {
        const auto inner = static_cast<std::uint64_t>(obs_series.size());
        write_binary(out, inner);
        for (const auto& value : obs_series) {
            if (const auto* real_value = std::get_if<double>(&value)) {
                const std::uint8_t tag = 0;
                write_binary(out, tag);
                write_binary(out, *real_value);
            } else {
                const auto complex = std::get<std::complex<double>>(value);
                const std::uint8_t tag = 1;
                const double complex_real = complex.real();
                const double complex_imag = complex.imag();
                write_binary(out, tag);
                write_binary(out, complex_real);
                write_binary(out, complex_imag);
            }
        }
    }
}

inline std::vector<std::vector<std::variant<std::complex<double>, double>>> read_observable_series(
    std::istream& in
) {
    std::uint64_t outer = 0;
    read_binary(in, outer);
    std::vector<std::vector<std::variant<std::complex<double>, double>>> series;
    series.resize(static_cast<std::size_t>(outer));
    for (auto& obs_series : series) {
        std::uint64_t inner = 0;
        read_binary(in, inner);
        obs_series.reserve(static_cast<std::size_t>(inner));
        for (std::uint64_t i = 0; i < inner; ++i) {
            std::uint8_t tag = 0;
            read_binary(in, tag);
            if (tag == 0) {
                double value = 0.;
                read_binary(in, value);
                obs_series.emplace_back(value);
            } else if (tag == 1) {
                double real = 0.;
                double imag = 0.;
                read_binary(in, real);
                read_binary(in, imag);
                obs_series.emplace_back(std::complex<double>{real, imag});
            } else {
                throw std::runtime_error("Unknown observable variant tag in checkpoint.");
            }
        }
    }
    return series;
}

inline void write_u64_vector(std::ostream& out, const std::vector<std::uint64_t>& values) {
    const auto size = static_cast<std::uint64_t>(values.size());
    write_binary(out, size);
    if (!values.empty()) {
        out.write(reinterpret_cast<const char*>(values.data()),
                  static_cast<std::streamsize>(values.size() * sizeof(std::uint64_t)));
    }
}

inline std::vector<std::uint64_t> read_u64_vector(std::istream& in) {
    std::uint64_t size = 0;
    read_binary(in, size);
    std::vector<std::uint64_t> values(static_cast<std::size_t>(size));
    if (!values.empty()) {
        in.read(reinterpret_cast<char*>(values.data()),
                static_cast<std::streamsize>(values.size() * sizeof(std::uint64_t)));
    }
    return values;
}

inline void write_double_vector(std::ostream& out, const std::vector<double>& values) {
    const auto size = static_cast<std::uint64_t>(values.size());
    write_binary(out, size);
    if (!values.empty()) {
        out.write(reinterpret_cast<const char*>(values.data()),
                  static_cast<std::streamsize>(values.size() * sizeof(double)));
    }
}

inline std::vector<double> read_double_vector(std::istream& in) {
    std::uint64_t size = 0;
    read_binary(in, size);
    std::vector<double> values(static_cast<std::size_t>(size));
    if (!values.empty()) {
        in.read(reinterpret_cast<char*>(values.data()),
                static_cast<std::streamsize>(values.size() * sizeof(double)));
    }
    return values;
}

inline void write_int_vector(std::ostream& out, const std::vector<int>& values) {
    const auto size = static_cast<std::uint64_t>(values.size());
    write_binary(out, size);
    if (!values.empty()) {
        out.write(reinterpret_cast<const char*>(values.data()),
                  static_cast<std::streamsize>(values.size() * sizeof(int)));
    }
}

inline std::vector<int> read_int_vector(std::istream& in) {
    std::uint64_t size = 0;
    read_binary(in, size);
    std::vector<int> values(static_cast<std::size_t>(size));
    if (!values.empty()) {
        in.read(reinterpret_cast<char*>(values.data()),
                static_cast<std::streamsize>(values.size() * sizeof(int)));
    }
    return values;
}

inline void write_pt_diagnostics(std::ostream& out, const ParallelTemperingDiagnostics& diagnostics) {
    write_binary(out, diagnostics.enabled);
    write_binary(out, diagnostics.mpi_compiled);
    write_binary(out, diagnostics.world_size);
    write_binary(out, diagnostics.rank);
    write_binary(out, diagnostics.initial_ladder_index);
    write_binary(out, diagnostics.final_ladder_index);
    write_binary(out, diagnostics.initial_s);
    write_binary(out, diagnostics.final_s);
    write_binary(out, diagnostics.initial_mu);
    write_binary(out, diagnostics.final_mu);
    write_binary(out, diagnostics.initial_J);
    write_binary(out, diagnostics.final_J);
    write_binary(out, diagnostics.initial_lmbda);
    write_binary(out, diagnostics.final_lmbda);
    write_binary(out, diagnostics.swap_attempted);
    write_binary(out, diagnostics.swap_accepted);
    write_binary(out, diagnostics.max_swap_log_weight_deviation);
    write_binary(out, diagnostics.max_swap_balance_err);
    write_binary(out, diagnostics.max_shared_function_deviation);
    write_u64_vector(out, diagnostics.swap_attempted_by_link);
    write_u64_vector(out, diagnostics.swap_accepted_by_link);
    write_u64_vector(out, diagnostics.R_count_by_link);
    write_double_vector(out, diagnostics.R_sum_by_link);
    write_double_vector(out, diagnostics.R_sumsq_by_link);
    write_int_vector(out, diagnostics.ladder_index_by_sample);
}

inline ParallelTemperingDiagnostics read_pt_diagnostics(std::istream& in) {
    ParallelTemperingDiagnostics diagnostics;
    read_binary(in, diagnostics.enabled);
    read_binary(in, diagnostics.mpi_compiled);
    read_binary(in, diagnostics.world_size);
    read_binary(in, diagnostics.rank);
    read_binary(in, diagnostics.initial_ladder_index);
    read_binary(in, diagnostics.final_ladder_index);
    read_binary(in, diagnostics.initial_s);
    read_binary(in, diagnostics.final_s);
    read_binary(in, diagnostics.initial_mu);
    read_binary(in, diagnostics.final_mu);
    read_binary(in, diagnostics.initial_J);
    read_binary(in, diagnostics.final_J);
    read_binary(in, diagnostics.initial_lmbda);
    read_binary(in, diagnostics.final_lmbda);
    read_binary(in, diagnostics.swap_attempted);
    read_binary(in, diagnostics.swap_accepted);
    read_binary(in, diagnostics.max_swap_log_weight_deviation);
    read_binary(in, diagnostics.max_swap_balance_err);
    read_binary(in, diagnostics.max_shared_function_deviation);
    diagnostics.swap_attempted_by_link = read_u64_vector(in);
    diagnostics.swap_accepted_by_link = read_u64_vector(in);
    diagnostics.R_count_by_link = read_u64_vector(in);
    diagnostics.R_sum_by_link = read_double_vector(in);
    diagnostics.R_sumsq_by_link = read_double_vector(in);
    diagnostics.ladder_index_by_sample = read_int_vector(in);
    return diagnostics;
}

} // namespace checkpoint_detail

#ifdef PARATORIC_HAS_MPI
namespace mpi_detail {

inline void fail_on_mpi_error(int rc, const char* where) {
    if (rc == MPI_SUCCESS) {
        return;
    }
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (initialized) {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    throw std::runtime_error(std::format("{} failed with MPI error code {}", where, rc));
}

struct MpiFinalizeGuard {
    bool active = false;
    ~MpiFinalizeGuard() {
        if (!active) {
            return;
        }
        int finalized = 0;
        MPI_Finalized(&finalized);
        if (!finalized) {
            MPI_Finalize();
        }
    }
};

} // namespace mpi_detail
#endif

template<char B>
concept ValidBasis = (B == 'x' || B == 'z');

inline std::int64_t square_lattice_sweep_size_moves(const LatSpec& lat_spec) {
    if (lat_spec.system_size <= 0) {
        throw std::invalid_argument("system_size must be positive for PT sweep scheduling.");
    }
    const auto L = static_cast<std::int64_t>(lat_spec.system_size);
    if (L > std::numeric_limits<std::int64_t>::max() / (2 * L)) {
        throw std::overflow_error("system_size is too large for PT sweep scheduling.");
    }
    return 2 * L * L;
}

inline std::int64_t parallel_tempering_swap_period_moves(
    const ParallelTemperingSpec& pt_spec,
    const LatSpec& lat_spec
) {
    if (pt_spec.swap_period > 0) {
        return static_cast<std::int64_t>(pt_spec.swap_period);
    }
    if (!(pt_spec.swap_period_sweeps > 0.0)) {
        throw std::invalid_argument("pt_swap_period_sweeps must be finite and positive.");
    }
    const double moves = pt_spec.swap_period_sweeps
        * static_cast<double>(square_lattice_sweep_size_moves(lat_spec));
    if (!(moves <= static_cast<double>(std::numeric_limits<std::int64_t>::max()))) {
        throw std::overflow_error("pt_swap_period_sweeps is too large.");
    }
    return std::max<std::int64_t>(1, std::llround(moves));
}

inline std::vector<int> parallel_tempering_rank_of_ladder_index(
    std::span<const int> ladder_index_by_rank
) {
    const auto world_size = static_cast<int>(ladder_index_by_rank.size());
    std::vector<int> rank_of_ladder_index(ladder_index_by_rank.size(), -1);
    for (int rank = 0; rank < world_size; ++rank) {
        const int ladder_index = ladder_index_by_rank[static_cast<size_t>(rank)];
        if (ladder_index < 0 || ladder_index >= world_size) {
            throw std::invalid_argument(std::format(
                "PT ladder index {} from rank {} is outside [0, {}).",
                ladder_index, rank, world_size));
        }
        if (rank_of_ladder_index[static_cast<size_t>(ladder_index)] != -1) {
            throw std::invalid_argument(std::format(
                "PT ladder index {} is reported by both rank {} and rank {}.",
                ladder_index,
                rank_of_ladder_index[static_cast<size_t>(ladder_index)],
                rank));
        }
        rank_of_ladder_index[static_cast<size_t>(ladder_index)] = rank;
    }
    return rank_of_ladder_index;
}

inline std::optional<int> parallel_tempering_partner_rank(
    int my_rank,
    std::span<const int> ladder_index_by_rank,
    int phase
) {
    const auto world_size = static_cast<int>(ladder_index_by_rank.size());
    if (my_rank < 0 || my_rank >= world_size) {
        throw std::invalid_argument("PT rank is outside the gathered rank range.");
    }
    if (phase != 0 && phase != 1) {
        throw std::invalid_argument("PT swap phase must be 0 or 1.");
    }

    const auto rank_of_ladder_index =
        parallel_tempering_rank_of_ladder_index(ladder_index_by_rank);
    const int my_ladder_index = ladder_index_by_rank[static_cast<size_t>(my_rank)];
    const int partner_ladder_index = phase == 0
        ? (my_ladder_index % 2 == 0 ? my_ladder_index + 1 : my_ladder_index - 1)
        : (my_ladder_index % 2 == 1 ? my_ladder_index + 1 : my_ladder_index - 1);
    if (partner_ladder_index < 0 || partner_ladder_index >= world_size) {
        return std::nullopt;
    }
    return rank_of_ladder_index[static_cast<size_t>(partner_ladder_index)];
}

template<char Basis, typename LatticeT = Lattice>
requires ValidBasis<Basis>
class ExtendedToricCodeQMC {
    public:
        using RNG = paratoric::rng::RNG;
        using SmallIndexVector = LatticeT::SmallIndexVector;
        using SmallEnergyVector = LatticeT::SmallEnergyVector;
        using SmallBoolVector = boost::container::small_vector<bool, 8>;

        ExtendedToricCodeQMC(std::shared_ptr<RNG> rng = nullptr) 
        : rng(rng ? std::move(rng) : std::make_shared<RNG>()) {};

        ~ExtendedToricCodeQMC() = default;

        ExtendedToricCodeQMC(ExtendedToricCodeQMC const&) = default;
        ExtendedToricCodeQMC& operator=(ExtendedToricCodeQMC const&) = default;

        /**
         * @brief This method will run a QMC thermalization of the extended toric code with the specified parameters and return observables and acceptance ratio diagnostics.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h the Hamiltonian parameter (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda the Hamiltonian parameter (gauge field term)
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.path_out output directory for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every 10000th snapshot will be saved)
         * 
         * @return Result             the result object.
         * @return Result.series      Time series (per snapshot) of all requested observables,
         *                            measured during thermalization. Each observable’s entry
         *                            contains one value per recorded snapshot, in time order.
         * @return Result.acc_ratio   Time series of Monte Carlo acceptance ratios.
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_thermalization(
            const Config& config
        );

        /**
         * @brief This method will run a QMC simulation of the extended toric code with the specified parameters and return observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_samples the number of snapshots
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_between_samples the number of steps between snapshots
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.custom_therm if custom thermalization is used (to probe hysteresis)
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h the Hamiltonian parameter (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda the Hamiltonian parameter (gauge field term)
         * @param config.param_spec.h_therm the thermalization value of h, used if custom_therm enabled
         * @param config.param_spec.lmbda_therm the thermalization value of lmbda, used if custom_therm enabled
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.path_out output directory for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every snapshot will be saved)
         * 
         * @return Result             the result object.
         * @return Result.series      Full counting statistics of all requested observables in the input order,
         *                            Each observable’s entry contains one value per recorded snapshot, 
         *                            in time order.
         * @return Result.mean        Bootstrap observable means
         * @return Result.mean_std    Bootstrap standard errors of the mean
         * @return Result.binder      Bootstrap binder ratios
         * @return Result.binder_std  Bootstrap standard errors of the binder ratios
         * @return Result.tau_int     Estimated integrated autocorrelation times
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_samples > 0
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_sample(
            const Config& config
        );

        /**
         * @brief Run only the core Metropolis update loop for backend timing.
         *
         * This is intended for comparing lattice backends without observable
         * bootstrap/HDF5 overhead. The total number of updates is
         * N_thermalization + N_samples * N_between_samples.
         */
        Result get_core_update_benchmark(
            const Config& config
        );

        double validate_parallel_tempering_swap_weight_debug(
            const Config& config,
            std::vector<double>* log_weight_trace = nullptr
        );

        /**
         * @brief Test scaffolding only, not itself the swap-weight feature. Not called
         * from any production path -- only from tests.
         *
         * Evolves two independent replicas under fully independent (h, mu, J, lmbda),
         * at a shared beta, using the private metropolis_step loop (mirrors
         * validate_parallel_tempering_swap_weight_debug, which is restricted to
         * mu=J=1). At each swap checkpoint, returns:
         *   .production_log_ratio: parallel_tempering_log_weight_ratio(...) itself.
         *   .m1_derivation_log_ratio: the general four-coupling swap weight derived in
         *     doc/m1_common_factor_swap_weight_derivation.md section 2, computed
         *     directly from existing accessors (get_non_diag_single_energy_z,
         *     get_non_diag_tuple_energy_z, cached_total_integrated_edge_energy,
         *     cached_total_integrated_plaquette_energy / their recompute_* twins).
         *
         * IMPORTANT: after M3, .production_log_ratio and .m1_derivation_log_ratio are
         * two evaluations of the SAME formula (the production code now implements the
         * M1 derivation). Agreement between them is a CONSISTENCY check -- it confirms
         * no term is missing from the production path -- not an independent physics
         * validation of the derivation itself. It cannot catch a wrong sign, a wrong
         * coefficient, or the wrong accessor in the right slot, because both sides
         * would be wrong the same way. See the finite-difference and raw-container
         * checks added in M3 for that (test_extended_toric_code_qmc.cpp,
         * boost_pt_diagonal_terms_match_finite_difference_l8 and
         * boost_pt_off_diagonal_terms_match_raw_kink_counts_l8), which are independent
         * of this derivation.
         */
        struct GeneralizedSwapWeightCheck {
            double production_log_ratio;
            double m1_derivation_log_ratio;
        };
        GeneralizedSwapWeightCheck validate_parallel_tempering_swap_weight_generalized_debug(
            const Config& config,
            double local_mu, double partner_mu,
            double local_J, double partner_J
        );

        /**
         * @brief M3 non-circular checks (test scaffolding, not reachable from any
         * production path). Evolves ONE lattice via metropolis_step at fixed
         * (h, mu, J, lmbda), then computes diagnostics that do NOT go through
         * parallel_tempering_log_weight_ratio or the M1 derivation:
         *  - finite_diff_dlmbda / finite_diff_dJ: derivative of the existing
         *    total_integrated_pot_energy(...) w.r.t. lmbda / J, by central finite
         *    difference. Independent of the swap-weight formula; only depends on
         *    the pre-existing Metropolis potential-energy function.
         *  - E_edge / E_plaquette: cached_total_integrated_plaquette_energy() and
         *    cached_total_integrated_edge_energy(), the same accessors the swap
         *    weight's diagonal terms use.
         *  - beta_times_single / beta_times_tuple: beta * get_non_diag_single_energy_z()
         *    and beta * get_non_diag_tuple_energy_z() (the cached kink-count accessors
         *    the swap weight's off-diagonal terms use).
         *  - kL_plus_kR_single: kL+kR from get_kL_kR_single(), a pre-existing,
         *    independently-implemented single-flip count (written for the
         *    Fredenhagen-Marcu estimator, unrelated to this work) -- an outside
         *    check on beta_times_single that does not go through
         *    recompute_non_diag_single_energy_z().
         *  - recompute_beta_times_tuple: beta * recompute_non_diag_tuple_energy_z().
         *    No independently-implemented public counter exists for star flips in
         *    this codebase (unlike single flips, which have get_kL_kR_single()), so
         *    this is the closest available check for the mu/tuple slot -- it is a
         *    cache-vs-recompute check, same family as beta_times_tuple, not a fully
         *    independent count. Documented as a limitation, not overstated.
         */
        struct SingleReplicaDiagnostics {
            double finite_diff_dlmbda;
            double finite_diff_dJ;
            double E_edge;
            double E_plaquette;
            double beta_times_single;
            double beta_times_tuple;
            double kL_plus_kR_single;
            double recompute_beta_times_tuple;
        };
        SingleReplicaDiagnostics validate_single_replica_diagnostics_debug(
            const Config& config, double h, double mu, double J, double lmbda, double eps
        );

        /**
         * @brief M7'' exact-identity check (Feynman, replacing the marginal-distribution
         * route). Test scaffolding only, not reachable from any production path.
         *
         * Runs TWO fully independent chains, one at rung i's couplings and one at
         * rung j's, each with its own RNG stream. Swap acceptance is never applied --
         * this never routes a configuration to the "wrong" rung, so every
         * configuration is a genuine equilibrium sample of its own rung's
         * distribution, whatever the swap weight computes. At each of N_samples
         * checkpoints (after independent thermalization on each side), calls the
         * PRODUCTION parallel_tempering_log_weight_ratio(...) -- the exact function
         * this test exists to validate -- on the pair (lattice_i, lattice_j), and
         * records log R = that log-ratio.
         *
         * Exact identity, for ANY correct swap-weight formula, at ANY size/couplings:
         *   <R> = <exp(log R)> = 1
         * (proof: R factors as [W_j(C_i)/W_i(C_i)] * [W_i(C_j)/W_j(C_j)], and C_i, C_j
         * are independent draws from their own normalized equilibrium distributions,
         * so each factor's expectation is a ratio of partition functions that cancels
         * exactly against the other.) A wrong sign, coefficient, or missing term
         * breaks this identity systematically, not something local relaxation can
         * repair (there is no relaxation to repair -- nothing is ever accepted).
         *
         * NOTE, corrected after an earlier wrong claim (Feynman review): R is
         * SYMMETRIC under exchanging i and j (R = W_j(C_i)W_i(C_j) / [W_i(C_i)W_j(C_j)]
         * is unchanged by swapping the labels), so "the same argument with the roles
         * reversed" reproduces <R>=1, NOT a second identity <1/R>=1. <1/R> expands to
         * <W_i(C_i)/W_j(C_i)> * <W_j(C_j)/W_i(C_j)>, a ratio-of-squares-type average
         * with no reason to equal 1. There is exactly one identity here. Do not report
         * <1/R> as a check, and do not attribute any deviation of <1/R> from 1 to
         * sampling/tail pathology -- it is simply not expected to be 1.
         */
        struct WeightRatioIdentityCheck {
            double R_mean;
            double R_sem;
            double acceptance_mean;
            double logR_mean;
            double logR_stddev;
            std::size_t n;
        };
        WeightRatioIdentityCheck validate_swap_weight_ratio_identity_debug(
            const Config& config_i, const Config& config_j
        );

        std::int64_t load_sample_checkpoint_lattice_debug(
            const std::filesystem::path& path,
            const Config& config,
            LatticeT& lat
        ) {
            const auto state = read_sample_checkpoint(path, config, lat);
            return state.production_metropolis_steps;
        }

        /**
         * @brief Read-only checkpoint extraction for standalone smoke tools.
         *
         * The checkpoint header is read once to reconstruct the exact Config
         * required by read_sample_checkpoint(). The actual checkpoint payload is
         * then read by that existing loader, including diagnostics, the
         * variable-length observable series, RNG state, and lattice state. This
         * helper never opens the checkpoint for writing.
         */
        struct CheckpointDumpData {
            std::uint32_t format_version = 0;
            std::vector<std::string> observables{};
            std::int64_t completed_samples = 0;
            bool has_pt_state = false;
            int pt_ladder_index = 0;
            double runtime_h = 0.;
            double runtime_mu = 0.;
            double runtime_J = 0.;
            double runtime_lmbda = 0.;
            std::vector<std::vector<std::variant<std::complex<double>, double>>> series{};
        };

        CheckpointDumpData read_checkpoint_for_dump(const std::filesystem::path& path) {
            if constexpr (!std::same_as<LatticeT, Lattice>) {
                throw std::invalid_argument("Checkpoint dumping is supported only for the Boost Lattice backend.");
            } else {
                std::ifstream header_in(path, std::ios::binary);
                if (!header_in) {
                    throw std::runtime_error(std::format("Could not open checkpoint file \"{}\".", path.string()));
                }

                char magic[sizeof(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC)]{};
                header_in.read(magic, sizeof(magic));
                if (!std::equal(std::begin(magic), std::end(magic),
                                std::begin(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC))) {
                    throw std::runtime_error("Checkpoint magic does not match ParaToric sample checkpoint format.");
                }
                std::uint32_t version = 0;
                checkpoint_detail::read_binary(header_in, version);
                if (version < checkpoint_detail::SAMPLE_CHECKPOINT_MIN_SUPPORTED_VERSION
                    || version > checkpoint_detail::SAMPLE_CHECKPOINT_VERSION) {
                    throw std::runtime_error("Unsupported checkpoint version.");
                }

                char basis = '\0';
                int system_size = 0;
                double beta = 0.;
                int default_spin = 0;
                double mu = 0.;
                double h = 0.;
                double J = 0.;
                double lmbda = 0.;
                int n_between_samples = 0;
                bool custom_therm = false;
                checkpoint_detail::read_binary(header_in, basis);
                const auto lattice_type = checkpoint_detail::read_string(header_in);
                checkpoint_detail::read_binary(header_in, system_size);
                checkpoint_detail::read_binary(header_in, beta);
                const auto boundaries = checkpoint_detail::read_string(header_in);
                checkpoint_detail::read_binary(header_in, default_spin);
                const auto lattice_backend = checkpoint_detail::read_string(header_in);
                checkpoint_detail::read_binary(header_in, mu);
                checkpoint_detail::read_binary(header_in, h);
                checkpoint_detail::read_binary(header_in, J);
                checkpoint_detail::read_binary(header_in, lmbda);
                checkpoint_detail::read_binary(header_in, n_between_samples);
                checkpoint_detail::read_binary(header_in, custom_therm);
                const auto observables = checkpoint_detail::read_string_vector(header_in);

                if (basis != Basis) {
                    throw std::runtime_error("Checkpoint basis does not match this dumper.");
                }

                Config config;
                config.lat_spec = LatSpec{
                    .basis = basis,
                    .lattice_type = lattice_type,
                    .system_size = system_size,
                    .beta = beta,
                    .boundaries = boundaries,
                    .default_spin = default_spin,
                    .lattice_backend = lattice_backend
                };
                config.param_spec = ParamSpec{.mu = mu, .h = h, .J = J, .lmbda = lmbda};
                // N_samples is absent from the checkpoint header. The existing
                // loader only uses it as an upper-bound consistency check.
                config.sim_spec.N_samples = std::numeric_limits<int>::max();
                config.sim_spec.N_between_samples = n_between_samples;
                config.sim_spec.custom_therm = custom_therm;
                config.sim_spec.observables = observables;

                LatticeT lat(config.lat_spec, rng);
                auto state = read_sample_checkpoint(path, config, lat);
                return CheckpointDumpData{
                    .format_version = version,
                    .observables = observables,
                    .completed_samples = state.completed_samples,
                    .has_pt_state = state.has_pt_state,
                    .pt_ladder_index = state.pt_ladder_index,
                    .runtime_h = state.runtime_h,
                    .runtime_mu = state.runtime_mu,
                    .runtime_J = state.runtime_J,
                    .runtime_lmbda = state.runtime_lmbda,
                    .series = std::move(state.observable_vector)
                };
            }
        }

        /**
         * @brief M9 test scaffolding: round-trips the PT ladder state fields
         * (pt_ladder_index, runtime_h/mu/J/lmbda) through write_sample_checkpoint /
         * read_sample_checkpoint directly, without needing an actual MPI-enabled PT
         * run (write_sample_checkpoint/read_sample_checkpoint are private, and PT
         * itself requires an MPI build, neither of which the non-MPI Boost test
         * binary can exercise). This tests the NEW serialization code (version-4
         * fields and the version>=3 legacy read guard) in isolation; it does not exercise real
         * swap dynamics -- that needs a genuine multi-rank run, done separately in
         * m4_smoke/m9_pt_checkpoint_restart.cpp.
         */
        struct PtCheckpointRoundTrip {
            bool has_pt_state;
            int pt_ladder_index;
            double runtime_h, runtime_mu, runtime_J, runtime_lmbda;
        };
        PtCheckpointRoundTrip checkpoint_pt_state_round_trip_debug(
            const Config& config,
            const std::filesystem::path& path,
            int pt_ladder_index, double runtime_h, double runtime_mu, double runtime_J, double runtime_lmbda
        ) {
            LatticeT lat(config.lat_spec, rng);
            write_sample_checkpoint(
                path, config, lat,
                SampleCheckpointState{
                    .completed_samples = 0,
                    .production_metropolis_steps = 0,
                    .integrated_pot_energy = 0.,
                    .acc_ratio = 1.,
                    .production_acceptance = AcceptanceDiagnostics{},
                    .observable_vector = std::vector<std::vector<std::variant<std::complex<double>, double>>>(
                        config.sim_spec.observables.size()),
                    .pt_ladder_index = pt_ladder_index,
                    .runtime_h = runtime_h,
                    .runtime_mu = runtime_mu,
                    .runtime_J = runtime_J,
                    .runtime_lmbda = runtime_lmbda
                }
            );
            LatticeT lat_reload(config.lat_spec, rng);
            const auto state = read_sample_checkpoint(path, config, lat_reload);
            return PtCheckpointRoundTrip{
                state.has_pt_state, state.pt_ladder_index,
                state.runtime_h, state.runtime_mu, state.runtime_J, state.runtime_lmbda
            };
        }

        /**
         * @brief This method will run a QMC hysteresis simulation of the extended toric code with the specified parameters and return observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_samples the number of snapshots
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_between_samples the number of steps between snapshots
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h_hys the Hamiltonian parameters (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda_hys the Hamiltonian parameters (gauge field term)
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.paths_out all output directories for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every snapshot will be saved)
         * 
         * @return Result                 the result object.
         * @return Result.series_hys      Full counting statistics of all requested observables.
         *                                Each element in the outer vector represents one parameter point (h_hys, lmbda_hys)
         *                                Each element in the middle vectors represent one observables in the order of input.
         *                                The inner vector are the full counting statistics in time order.
         * @return Result.mean_hys        Bootstrap observable means. 
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.mean_std_hys    Bootstrap standard errors of the mean.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.binder_hys      Bootstrap binder ratios.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.binder_std_hys  Bootstrap standard errors of the binder ratios.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.tau_int_hys     Estimated integrated autocorrelation times.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_samples > 0
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_hysteresis(
            const Config& config
        );
        
        /**
         * @brief Here we "assemble" the types of the observables. Most of the observables are probably reals.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param observables contains the names of the observables
         * 
         * @return vector of observable types in the order of occurance in observables.
         * 
         */
        std::vector<std::string> get_obs_type_vec(const std::vector<std::string>& observables);
    
    private:
        std::function<double(LatticeT&, double, double, double, double)> 
        percolation_probability_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.percolation_probability(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        plaquette_percolation_probability_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.plaquette_percolation_probability(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        cube_percolation_probability 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.cube_percolation_probability(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        percolation_strength_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.percolation_strength(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        plaquette_percolation_strength_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.plaquette_percolation_strength(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        string_number_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.get_string_count(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        largest_cluster_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.largest_cluster(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        largest_plaquette_cluster_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.largest_plaquette_cluster(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        anyon_count_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.get_anyon_count(); 
        };

        std::function<std::complex<double>(LatticeT&, double, double, double, double)> 
        fredenhagen_marcu_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            return lat.fredenhagen_marcu(); 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        staggered_imaginary_times_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_staggered_imaginary_times_plaquette(); 
            else return lat.get_staggered_imaginary_times_star();
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        energy_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return - lat.get_diag_single_energy() * h - lat.get_diag_tuple_energy_x() * mu 
                - lat.get_non_diag_single_energy_x() - lat.get_non_diag_tuple_energy_x();
            }
            else {
                return - lat.get_diag_single_energy() * lmbda - lat.get_diag_tuple_energy_z() * J 
                - lat.get_non_diag_single_energy_z() - lat.get_non_diag_tuple_energy_z();
            } 
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        energy_h_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_diag_single_energy() * h; 
            else return - lat.get_non_diag_single_energy_z();
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        energy_mu_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_diag_tuple_energy_x() * mu; 
            else return - lat.get_non_diag_tuple_energy_z();
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        energy_lmbda_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_non_diag_single_energy_x(); 
            else return - lat.get_diag_single_energy() * lmbda;
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        energy_J_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_non_diag_tuple_energy_x(); 
            else return - lat.get_diag_tuple_energy_z() * J;
        }; 

        std::function<double(LatticeT&, double, double, double, double)> 
        sigma_x_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return lat.get_diag_single_energy()/static_cast<double>(lat.get_edge_count()); 
            } else {
                return lat.get_non_diag_single_energy_z()/static_cast<double>(lat.get_edge_count() * h);
            }
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        sigma_x_tau_avg_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return lat.total_integrated_edge_energy()/static_cast<double>(lat.get_beta() * lat.get_edge_count()); 
            } else {
                return lat.get_non_diag_single_energy_z()/static_cast<double>(lat.get_edge_count() * h);
            }
        };

        std::function<std::complex<double>(LatticeT&, double, double, double, double)> 
        sigma_x_static_susceptibility_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_diag_M_M(); 
            else return lat.get_non_diag_M_M();
        };

        std::function<std::complex<double>(LatticeT&, double, double, double, double)> 
        sigma_x_dynamical_susceptibility_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_diag_dynamical_M_M();
            else return lat.get_kL_kR_single() / static_cast<double>(std::sqrt(2) * h);
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        sigma_z_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_non_diag_single_energy_x()/static_cast<double>(lat.get_edge_count() * lmbda); 
            else 
                return (lat.get_diag_single_energy()/static_cast<double>(lat.get_edge_count()));
        };

        std::function<std::complex<double>(LatticeT&, double, double, double, double)> 
        sigma_z_static_susceptibility_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_non_diag_M_M(); 
            else return lat.get_diag_M_M();
        };

        std::function<std::complex<double>(LatticeT&, double, double, double, double)> 
        sigma_z_dynamical_susceptibility_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_kL_kR_single() / static_cast<double>(std::sqrt(2) * lmbda);
            else 
                return lat.get_diag_dynamical_M_M();
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        star_x_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_diag_tuple_energy_x()/static_cast<double>(lat.get_vertex_count()); 
            else 
                return lat.get_non_diag_tuple_energy_z()/static_cast<double>(lat.get_vertex_count() * mu);
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        plaquette_z_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_non_diag_tuple_energy_x()/static_cast<double>(lat.get_plaquette_count() * J); 
            else 
                return lat.get_diag_tuple_energy_z()/static_cast<double>(lat.get_plaquette_count());
        };

        // Match the diagonal plaquette estimator to the full imaginary-time
        // extent.  The off-diagonal branch is already a kink count divided by
        // beta, so its established normalization is retained.
        std::function<double(LatticeT&, double, double, double, double)>
        plaquette_z_tau_avg_obs
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) {
            if constexpr (Basis == 'x')
                return lat.get_non_diag_tuple_energy_x()/static_cast<double>(lat.get_plaquette_count() * J);
            else
                return lat.total_integrated_plaquette_energy()
                    / static_cast<double>(lat.get_beta() * lat.get_plaquette_count());
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        delta_obs = [](LatticeT& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return - lat.get_diag_tuple_energy_x()/static_cast<double>(lat.get_vertex_count()) 
                + lat.get_non_diag_tuple_energy_x()/static_cast<double>(lat.get_plaquette_count() * J); 
            } else {
                return - lat.get_non_diag_tuple_energy_z()/static_cast<double>(lat.get_vertex_count() * mu) 
                + lat.get_diag_tuple_energy_z()/static_cast<double>(lat.get_plaquette_count());
            }
        };

        std::function<double(LatticeT&, double, double, double, double)>
        delta_tau_avg_obs = [](LatticeT& lat, double h, double lmbda, double mu, double J) {
            if constexpr (Basis == 'x') {
                return - lat.total_integrated_star_energy()
                    / static_cast<double>(lat.get_beta() * lat.get_vertex_count())
                    + lat.get_non_diag_tuple_energy_x()
                    / static_cast<double>(lat.get_plaquette_count() * J);
            } else {
                return - lat.get_non_diag_tuple_energy_z()
                    / static_cast<double>(lat.get_vertex_count() * mu)
                    + lat.total_integrated_plaquette_energy()
                    / static_cast<double>(lat.get_beta() * lat.get_plaquette_count());
            }
        };

        std::function<double(LatticeT&, double, double, double, double)> 
        anyon_density_obs 
        = [](LatticeT& lat, double h, double lmbda, double mu, double J) {
            if constexpr (Basis == 'x') 
                return lat.get_anyon_count()/static_cast<double>(lat.get_vertex_count()); 
            else 
                return lat.get_anyon_count()/static_cast<double>(lat.get_plaquette_count()); 
        };

        // This structure contains the name, the type (e.g. real, i.e. just a double) and the function of an observable.
        struct Obs {
            public:
                std::string obs_name;
                std::string obs_type;
                std::function<
                    std::variant< std::complex<double>, double>(LatticeT&, double, double, double, double)
                > obs_func;
        };

        // This vector contains the names of the observables (input as vector of strings: observables) and connects them to the appropriate function.  
        std::vector<Obs> obs_vec = {
            {"anyon_count", "real", anyon_count_obs},
            {"anyon_density", "real", anyon_density_obs},
            {"cube_percolation_probability", "real", cube_percolation_probability},
            {"delta", "real", delta_obs},
            {"delta_tau_avg", "real", delta_tau_avg_obs},
            {"energy", "real", energy_obs},
            {"energy_h", "real", energy_h_obs},
            {"energy_lmbda", "real", energy_lmbda_obs},
            {"energy_J", "real", energy_J_obs},
            {"energy_mu", "real", energy_mu_obs},
            {"fredenhagen_marcu", "fredenhagen_marcu", fredenhagen_marcu_obs},
            {"largest_cluster", "real", largest_cluster_obs},
            {"largest_plaquette_cluster", "real", largest_plaquette_cluster_obs},
            {"percolation_probability", "real", percolation_probability_obs},
            {"percolation_strength", "real", percolation_strength_obs},
            {"plaquette_percolation_probability", "real", plaquette_percolation_probability_obs},
            {"plaquette_percolation_strength", "real", plaquette_percolation_strength_obs},
            {"plaquette_z", "real", plaquette_z_obs},
            {"plaquette_z_tau_avg", "real", plaquette_z_tau_avg_obs},
            {"sigma_x", "real", sigma_x_obs},
            {"sigma_x_tau_avg", "real", sigma_x_tau_avg_obs},
            {"sigma_x_static_susceptibility", "susceptibility", sigma_x_static_susceptibility_obs},
            // TODO fix
            {"sigma_x_dynamical_susceptibility", "susceptibility", sigma_x_dynamical_susceptibility_obs},
            {"sigma_z", "real", sigma_z_obs},
            {"sigma_z_static_susceptibility", "susceptibility", sigma_z_static_susceptibility_obs},
            // TODO fix
            {"sigma_z_dynamical_susceptibility", "susceptibility", sigma_z_dynamical_susceptibility_obs},
            {"staggered_imaginary_times", "real", staggered_imaginary_times_obs},
            {"star_x", "real", star_x_obs},
            {"string_number", "real", string_number_obs}
            };
        
        /**
         * @brief Here we "assemble" the lambda functions for the observables. We use this vector to calculate the observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param observables contains the names of the observables
         * 
         * @return vector of lambda functions which can be applied to a lattice to extract the obervables.
         * 
         */
        std::vector<std::function<std::variant< std::complex<double>, double>(LatticeT&, double, double, double, double)>>
        get_obs_func_vec(
            const std::vector<std::string>& observables
        );

        std::shared_ptr<RNG> rng;
        std::uniform_real_distribution<double> uniform_dist{0., 1.};
        static constexpr double PRECISION = std::numeric_limits<double>::epsilon();
        static constexpr double AUTOCORRELATION_WARNING_SAMPLE_FRACTION = 0.1;
        static constexpr int UPDATE_TYPE_COUNT = 7;
        AcceptanceDiagnostics* active_acceptance_diagnostics_ = nullptr;
        int active_update_type_ = -1;
        std::uint64_t active_block_attempted_ = 0;
        std::uint64_t active_block_accepted_ = 0;
        double active_block_acceptance_ratio_sum_ = 0.;

        struct SampleCheckpointState {
            std::int64_t completed_samples = 0;
            std::int64_t production_metropolis_steps = 0;
            double integrated_pot_energy = 0.;
            double acc_ratio = 1.;
            AcceptanceDiagnostics production_acceptance{};
            std::vector<std::vector<std::variant<std::complex<double>, double>>> observable_vector{};
            // M9 (Feynman): persisted PT ladder position and post-swap runtime
            // couplings, checkpoint format version >= 3 only. has_pt_state is false
            // after reading a version-2 file (written before this change existed) --
            // the caller MUST then fall back to the pre-M9 behaviour (pt_ladder_index
            // = pt_rank, runtime couplings = launch values), not assume these fields
            // mean anything. On write, always populated (every write now produces a
            // version-4 file); has_pt_state is unused on write.
            bool has_pt_state = false;
            int pt_ladder_index = 0;
            double runtime_h = 0.;
            double runtime_mu = 0.;
            double runtime_J = 0.;
            double runtime_lmbda = 0.;
            // M11: completed sample count, duplicated explicitly in v4 so all
            // rank-local files can be checked as one global snapshot on resume.
            std::int64_t global_checkpoint_marker = 0;
            bool has_pt_diagnostics = false;
            ParallelTemperingDiagnostics pt_diagnostics{};
        };

        int random_index(int bound) {
            return static_cast<int>(
                paratoric::rng::uniform_index(*rng, static_cast<std::uint64_t>(bound))
            );
        }

        double uniform_real(double lower, double upper) {
            return lower + (upper - lower) * uniform_dist(*rng);
        }

        bool accept(double ratio) {
            return ratio >= 1.0 || uniform_dist(*rng) < ratio;
        }

        void start_acceptance_diagnostics(AcceptanceDiagnostics& diagnostics, bool reset = true);
        void stop_acceptance_diagnostics();
        void start_acceptance_block();
        void finish_acceptance_block();
        void record_update_attempt(int update_type, double acc_ratio);
        void record_update_acceptance();

        void write_sample_checkpoint(
            const std::filesystem::path& path,
            const Config& config,
            const LatticeT& lat,
            const SampleCheckpointState& state
        ) const;

        SampleCheckpointState read_sample_checkpoint(
            const std::filesystem::path& path,
            const Config& config,
            LatticeT& lat
        );

        static bool checkpoint_due(
            const Config& config,
            std::int64_t completed_samples,
            std::chrono::steady_clock::time_point now,
            std::chrono::steady_clock::time_point& last_checkpoint_time
        );

        static double calculate_autocorrelation_time_with_warning(
            const std::vector<double>& obs_real,
            const std::string& observable_name,
            bool has_hysteresis_context = false,
            size_t hysteresis_point = 0,
            double h = 0.,
            double lmbda = 0.
        );

        /**
         * @brief Return the total potential energy (integrated over imaginary time) of the lattice lat.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         * @return the total integrated potential energy
         * 
         */
        static double total_integrated_pot_energy(LatticeT& lat, double h, double mu, double J, double lmbda);

        static double parallel_tempering_log_weight_ratio(
            LatticeT& local_lat,
            LatticeT& partner_lat,
            double local_h,
            double partner_h,
            double local_mu,
            double partner_mu,
            double local_lmbda,
            double partner_lmbda,
            double local_J,
            double partner_J,
            bool use_cached
        );

        /**
         * @brief M8 (Feynman): the scalar core of parallel_tempering_log_weight_ratio,
         * pulled out so the inline swap blocks -- which never materialize a partner
         * Lattice object, only the scalars exchanged over MPI -- can call the SAME
         * formula the shared function calls, not a hand-typed duplicate of it.
         * parallel_tempering_log_weight_ratio itself is now a thin wrapper: it reads
         * these eight scalars off local_lat/partner_lat and delegates here. Take
         * `beta` explicitly since there is no Lattice object to call get_beta() on
         * in the inline-block context.
         */
        static double parallel_tempering_log_weight_ratio_from_scalars(
            double beta,
            double local_h, double partner_h,
            double local_mu, double partner_mu,
            double local_lmbda, double partner_lmbda,
            double local_J, double partner_J,
            double local_single, double partner_single,
            double local_tuple, double partner_tuple,
            double local_edge, double partner_edge,
            double local_plaquette, double partner_plaquette
        );

        /**
         * @brief Return the potential EDGE energy DIFFERENCE (integrated over imaginary time) when flipping the spin between imag_time_spin_flip and imag_time_next_spin_flip 
         * on the edge between vertices source_v and target_v on the lattice lat.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * @param source_v the source vertex of the edge of interest
         * @param target_v the target vertex of the edge of interest
         * @param imag_time_spin_flip the lower bound for imaginary time
         * @param imag_time_next_spin_flip the upper bound for imaginary time
         * @param total_cache              If true, use cached integrated energies
         *                                 and return @c -2*cache per affected tuple.
         *                                 This is a constant-time fast path intended
         *                                 for full-period flips (e.g., [0, β]).
         *                                 Do not set for partial intervals.
         * 
         * @return the integrated potential energy difference
         * 
         */
        static std::tuple<double, double> 
        integrated_pot_energy_diff_single_spin_flip_edge(
            LatticeT& lat, double h, double mu, double J, double lmbda, 
            const typename LatticeT::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache
        );

        /**
         * @brief Integrated tuple–energy difference for a single spin flip on an edge.
         *
         * Computes the tuple (star/plaquette) energy change, integrated over the
         * imaginary-time interval [@p imag_time_spin_flip, @p imag_time_next_spin_flip],
         * when the spin on @p edg is flipped across that interval. The affected tuples
         * are:
         *  - Basis 'x': the two STARS at the endpoints of @p edg.
         *  - Basis 'z': the PLAQUETTES adjacent to @p edg.
         *
         * Returns the per-tuple **bare** energy differences and the **coupled** scalar
         * sum used for acceptance. Couplings are applied only to the scalar:
         *  - Basis 'x': @c delta = -mu * sum(bare_diffs)
         *  - Basis 'z': @c delta = -J  * sum(bare_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                      Lattice object.
         * @param h                        Electric-field coupling (unused here).
         * @param mu                       Star coupling (used if Basis=='x').
         * @param J                        Plaquette coupling (used if Basis=='z').
         * @param lmbda                    Gauge-field coupling (unused here).
         * @param edg                      Edge whose spin is flipped.
         * @param imag_time_spin_flip      Lower bound of imaginary time (inclusive).
         * @param imag_time_next_spin_flip Upper bound of imaginary time (inclusive).
         * @param total_cache              If true, use cached integrated tuple energies
         *                                 and return @c -2*cache per affected tuple.
         *                                 This is a constant-time fast path intended
         *                                 for full-period flips (e.g., [0, β]).
         *                                 Do not set for partial intervals.
         *
         * @return std::tuple<
         *           double,                 // coupled scalar delta (see above)
         *           std::vector<int>,       // tuple indices: star centers (Basis 'x')
         *                                   // or plaquette indices (Basis 'z')
         *           std::vector<double>     // per-tuple **bare** energy differences on [t1,t2],
         *                                   // aligned with the index vector
         *         >
         *
         * @pre @p imag_time_next_spin_flip > @p imag_time_spin_flip. Interval must be non-zero.
         * @note Per-tuple values are bare; apply couplings only when forming totals or acceptance ratios.
         *       The order of indices matches the lattice adjacency.
         */
        static std::tuple<double, SmallIndexVector, SmallEnergyVector> 
        integrated_pot_energy_diff_single_spin_flip_tuple(
            LatticeT& lat, double h, double mu, double J, double lmbda, 
            const typename LatticeT::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache
        );

        /**
         * @brief Integrated EDGE–energy difference for flipping all spins of a tuple.
         *
         * Computes the (bare) edge–energy change for each edge in @p tuple_edges,
         * integrated over [@p imag_time_spin_flip, @p imag_time_next_spin_flip],
         * when the spins on those edges are flipped across that interval. Returns the
         * per-edge **bare** differences and the **coupled** scalar sum used for
         * acceptance. Couplings are applied only to the scalar:
         *  - Basis 'x': @c delta = -h * sum(bare_edge_diffs)
         *  - Basis 'z': @c delta = -lmbda * sum(bare_edge_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                       Lattice object.
         * @param h                         Electric-field coupling (used if Basis=='x').
         * @param mu                        Star coupling (unused here).
         * @param J                         Plaquette coupling (unused here).
         * @param lmbda                     Gauge-field coupling (used if Basis=='z').
         * @param tuple_index               Index of the tuple (for bookkeeping).
         * @param tuple_edges               Edges belonging to the tuple; order is preserved.
         * @param imag_time_spin_flip       Lower bound of imaginary time (inclusive).
         * @param imag_time_next_spin_flip  Upper bound of imaginary time (inclusive).
         * @param total_cache               If true, use cached integrated edge energies and
         *                                  return @c -2 * cache for each edge. This is a
         *                                  constant-time fast path intended for full-period
         *                                  flips (e.g., [0, β]). Do not set for partial intervals.
         *
         * @return std::tuple<
         *           double,                         // coupled scalar delta (see above)
         *           std::span<typename LatticeT::Edge>,     // echo of @p tuple_edges (same order)
         *           std::vector<double>             // per-edge **bare** energy differences on [t1,t2],
         *                                           // aligned with the returned edge vector
         *         >
         *
         * @pre @p imag_time_next_spin_flip > @p imag_time_spin_flip (non-zero interval).
         * @note Per-edge values are bare; apply couplings only when forming totals or acceptance ratios.
         * @note The second return component mirrors the input edges by value; consider
         *       using a view/reference in hot paths to avoid copies.
         */
        static std::tuple<double, std::span<const typename LatticeT::Edge>, SmallEnergyVector> 
        integrated_pot_energy_diff_tuple_flip_edge(
            LatticeT& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
            double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache,
            bool interval_has_no_inner_flips = false
        );

        /**
         * @brief Integrated EDGE–energy difference for a “combination” tuple update.
         *
         * Computes, for each edge in @p tuple_edges, the (bare) edge–energy change
         * integrated over [@p tau_left, @p tau_right] when a tuple flip occurs at
         * @p imag_time_tuple_flip and a single–spin flip on that same edge occurs at
         * @p imag_time_spin_flips[i]. The per–edge bare differences are summed into a
         * **coupled** scalar used for acceptance:
         *  - Basis 'x': @c delta = -h      * sum(bare_edge_diffs)
         *  - Basis 'z': @c delta = -lmbda  * sum(bare_edge_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                   Lattice object.
         * @param h                     Electric–field coupling (used if Basis=='x').
         * @param mu                    Star coupling (unused here).
         * @param J                     Plaquette coupling (unused here).
         * @param lmbda                 Gauge–field coupling (used if Basis=='z').
         * @param tuple_index           Index of the tuple (for bookkeeping).
         * @param tuple_edges           Edges belonging to the tuple; order is preserved.
         * @param imag_time_tuple_flip  Imaginary time of the tuple flip.
         * @param imag_time_spin_flips  Per–edge single–flip times aligned with @p tuple_edges;
         *                              @c imag_time_spin_flips[i] is the time on @c tuple_edges[i].
         * @param tau_left              Lower bound of the integration interval (inclusive).
         * @param tau_right             Upper bound of the integration interval (inclusive).
         * @param create_vector         For each edge, whether a single flip is created (true)
         *                              or removed (false). (Currently not used in the integral.)
         * @param tuple_destroy         Whether the tuple flip is destroyed (true) or created (false).
         *                              (Currently not used in the integral.)
         *
         * @return std::tuple<
         *           double,                         // coupled scalar delta (see above)
         *           std::span<typename LatticeT::Edge>,     // echo of @p tuple_edges (same order)
         *           std::vector<double>             // per–edge **bare** energy differences on [tau_left,tau_right],
         *                                           // aligned with the returned edge vector
         *         >
         *
         * @pre @p tau_right > @p tau_left.  Interval must be non-zero.
         * @pre @p imag_time_spin_flips.size() == @p tuple_edges.size().
         *
         * @note Per–edge values are **bare**; couplings are applied only to the scalar sum.
         * @note The local schedule per edge is {(@p imag_time_tuple_flip, tuple), (@p imag_time_spin_flips[i], single)}.
         *       Equal–time flips are combined by parity; order does not affect the integral.
         * @note The parameters @p create_vector and @p tuple_destroy are accepted for interface
         *       symmetry but are not currently used in the computation.
         */
        static std::tuple<double, std::span<const typename LatticeT::Edge>, SmallEnergyVector> 
        integrated_pot_energy_diff_combination_flip_edge(
            LatticeT& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
            double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
            double tau_left, double tau_right, 
            const SmallBoolVector& create_vector, bool tuple_destroy
        );

        /**
         * @brief Integrated TUPLE–energy difference for a “combination” update.
         *
         * Computes the (bare) tuple energy change integrated over
         * [@p tau_left, @p tau_right] when a tuple flip occurs at
         * @p imag_time_tuple_flip and, on the same tuple, single–spin flips occur on
         * its edges at the times in @p imag_time_spin_flips. The affected tuples are:
         *  - Basis 'x' → STARS at the endpoints of the tuple edges.
         *  - Basis 'z' → PLAQUETTES adjacent to the tuple edges.
         *
         * Returns the per-tuple **bare** energy differences (aligned with the returned
         * index vector) and the **coupled** scalar sum used for acceptance:
         *  - Basis 'x': @c delta = -mu * sum(bare_tuple_diffs)
         *  - Basis 'z': @c delta = -J  * sum(bare_tuple_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                   Lattice object.
         * @param h                     Electric-field coupling (unused here).
         * @param mu                    Star coupling (used if Basis=='x').
         * @param J                     Plaquette coupling (used if Basis=='z').
         * @param lmbda                 Gauge-field coupling (unused here).
         * @param tuple_index           Index of the tuple (bookkeeping).
         * @param tuple_edges           Edges that form the tuple; order is preserved.
         * @param imag_time_tuple_flip  Imaginary time of the tuple flip event.
         * @param imag_time_spin_flips  Per-edge single-flip times aligned with
         *                              @p tuple_edges; @c imag_time_spin_flips[i] is
         *                              the time on @c tuple_edges[i].
         * @param tau_left              Lower bound of the integration interval (inclusive).
         * @param tau_right             Upper bound of the integration interval (inclusive).
         * @param create_vector         For each edge, whether a single flip is created (true)
         *                              or removed (false). (Accepted for symmetry; not used here.)
         * @param tuple_destroy         Whether the tuple flip is destroyed (true) or created (false).
         *                              (Accepted for symmetry; not used here.)
         *
         * @return std::tuple<
         *           double,                 // coupled scalar delta (see above)
         *           std::vector<int>,       // tuple indices: star centers (Basis 'x')
         *                                   // or plaquette indices (Basis 'z')
         *           std::vector<double>     // per-tuple **bare** energy differences on [tau_left,tau_right],
         *                                   // aligned with the index vector
         *         >
         *
         * @pre @p tau_right > @p tau_left (non-zero interval).
         * @pre @p imag_time_spin_flips.size() == @p tuple_edges.size().
         *
         * @note “Bare” means couplings are NOT applied to the per-tuple values.
         *       Couplings are applied only to the returned scalar @c delta.
         * @note Equal-time flips are combined by parity; their order does not affect the integral.
         */
        static std::tuple<double, SmallIndexVector, SmallEnergyVector> 
        integrated_pot_energy_diff_combination_flip_tuple(
            LatticeT& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
            double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
            double tau_left, double tau_right, 
            const SmallBoolVector& create_vector, bool tuple_destroy
        );

        /**
         * @brief Perform tuple combination update, where both tuple and 
         * single spin flips are created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param tuple_edges the vector which stores the edges of the tuple
         * @param imag_time_tuple_flip the imaginary time of the tuple flip that is created or destroyed
         * @param imag_time_spin_flips vector which contains the imaginary times of the single spin flips 
         * that are created or destroyed.
         * @param create_vector vector which contains the information if single spin flip is added (1) 
         * or removed (0) on the tuple edges. Order of edges is identical to imag_time_spin_flips.
         * @param tuple_destroy if the tuple is destroyed (1) or created (0)
         * 
         */
        static void combination_flip(
            LatticeT& lat, double h, double mu, 
            int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
            double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
            const SmallBoolVector& create_vector, bool tuple_destroy
        );

        /**
         * @brief Metropolis-Hastings update where a pair of single spin flips is created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_double_single_spin_flip(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a single spin flip is moved in imaginary time.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_single_spin_flip_move(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where the spin is flipped globally on a random edge.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_global_single_spin_flip(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );


        /**
         * @brief Metropolis-Hastings update where the spin is flipped globally on a random tuple.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_global_tuple_flip(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a pair of tuple flips is created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_double_tuple_flip(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a tuple flip is moved in imaginary time.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_single_tuple_flip_move(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a tuple flip and single spin flips on the same tuple are created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_spin_tuple_combination(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings helper function that distributes updates randomly.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step(
            LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        template<typename T>
        requires std::integral<T> || std::floating_point<T>
        constexpr T modulo(T a, T b) {
            if constexpr (std::integral<T>) {
                T result = a % b;
                return result >= 0 ? result : result + b;
            } else {
                T result = std::fmod(a, b);
                return result >= 0 ? result : result + b;
            }
        }

        template<std::floating_point T>
        constexpr bool almost_equal(
            T a, T b, 
            T rel_tol = std::numeric_limits<T>::epsilon(), 
            T abs_tol = std::numeric_limits<T>::min()
        ) {
            T diff = std::abs(a - b);

            if (diff <= abs_tol) return true;

            T largest = std::max(std::abs(a), std::abs(b));
            return diff <= largest * rel_tol;
        }
};

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::vector<std::function<std::variant< std::complex<double>, double>(LatticeT&, double, double, double, double)>> 
ExtendedToricCodeQMC<Basis, LatticeT>::get_obs_func_vec(const std::vector<std::string>& observables) {
    std::vector<std::function<std::variant< std::complex<double>, double>(LatticeT&, double, double, double, double)>> result;
    for (const auto& obs_name : observables) {
        bool obs_found = false;
        for (const auto& obs : obs_vec) {
            if (obs.obs_name == obs_name) {
                result.emplace_back( obs.obs_func );
                obs_found = true;
                break;
            } 
        }
        if (!obs_found) {
            throw std::invalid_argument(std::string("The observable \"") + obs_name + std::string("\" does not exist!"));
        }
    }
    return result;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::vector<std::string> 
ExtendedToricCodeQMC<Basis, LatticeT>::get_obs_type_vec(const std::vector<std::string>& observables) {
    std::vector<std::string> result;
    for (const auto& obs_name : observables) {
        bool obs_found = false;
        for (const auto& obs : obs_vec) {
            if (obs.obs_name == obs_name) {
                result.emplace_back( obs.obs_type );
                obs_found = true;
                break;
            } 
        }
        if (!obs_found) {
            throw std::invalid_argument(std::string("The observable \"") + obs_name + std::string("\" does not exist!"));
        }
    }
    return result;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::start_acceptance_diagnostics(AcceptanceDiagnostics& diagnostics, bool reset) {
    if (reset) {
        diagnostics = AcceptanceDiagnostics{};
    }
    active_acceptance_diagnostics_ = &diagnostics;
    active_update_type_ = -1;
    active_block_attempted_ = 0;
    active_block_accepted_ = 0;
    active_block_acceptance_ratio_sum_ = 0.;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::stop_acceptance_diagnostics() {
    active_acceptance_diagnostics_ = nullptr;
    active_update_type_ = -1;
    active_block_attempted_ = 0;
    active_block_accepted_ = 0;
    active_block_acceptance_ratio_sum_ = 0.;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::start_acceptance_block() {
    active_block_attempted_ = 0;
    active_block_accepted_ = 0;
    active_block_acceptance_ratio_sum_ = 0.;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::finish_acceptance_block() {
    if (!active_acceptance_diagnostics_) return;
    active_acceptance_diagnostics_->block_attempted.emplace_back(active_block_attempted_);
    active_acceptance_diagnostics_->block_accepted.emplace_back(active_block_accepted_);
    active_acceptance_diagnostics_->block_acceptance_ratio_sum.emplace_back(active_block_acceptance_ratio_sum_);
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::record_update_attempt(int update_type, double acc_ratio) {
    if (!active_acceptance_diagnostics_) return;
    active_acceptance_diagnostics_->attempted += 1;
    active_acceptance_diagnostics_->acceptance_ratio_sum += acc_ratio;
    active_block_attempted_ += 1;
    active_block_acceptance_ratio_sum_ += acc_ratio;
    if (update_type >= 0 && update_type < UPDATE_TYPE_COUNT) {
        const auto index = static_cast<size_t>(update_type);
        active_acceptance_diagnostics_->attempted_by_update[index] += 1;
        active_acceptance_diagnostics_->acceptance_ratio_sum_by_update[index] += acc_ratio;
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::record_update_acceptance() {
    if (!active_acceptance_diagnostics_) return;
    active_acceptance_diagnostics_->accepted += 1;
    active_block_accepted_ += 1;
    if (active_update_type_ >= 0 && active_update_type_ < UPDATE_TYPE_COUNT) {
        active_acceptance_diagnostics_->accepted_by_update[static_cast<size_t>(active_update_type_)] += 1;
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::write_sample_checkpoint(
    const std::filesystem::path& path,
    const Config& config,
    const LatticeT& lat,
    const SampleCheckpointState& state
) const {
    if (path.empty()) {
        return;
    }
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Checkpointing is supported only for the Boost Lattice backend.");
    } else {
        const auto parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
        const auto tmp_path = path.string() + ".tmp";
        std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error(std::format("Could not open checkpoint file \"{}\".", tmp_path));
        }

        out.write(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC, sizeof(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC));
        checkpoint_detail::write_binary(out, checkpoint_detail::SAMPLE_CHECKPOINT_VERSION);
        checkpoint_detail::write_binary(out, config.lat_spec.basis);
        checkpoint_detail::write_string(out, config.lat_spec.lattice_type);
        checkpoint_detail::write_binary(out, config.lat_spec.system_size);
        checkpoint_detail::write_binary(out, config.lat_spec.beta);
        checkpoint_detail::write_string(out, config.lat_spec.boundaries);
        checkpoint_detail::write_binary(out, config.lat_spec.default_spin);
        checkpoint_detail::write_string(out, config.lat_spec.lattice_backend);
        checkpoint_detail::write_binary(out, config.param_spec.mu);
        checkpoint_detail::write_binary(out, config.param_spec.h);
        checkpoint_detail::write_binary(out, config.param_spec.J);
        checkpoint_detail::write_binary(out, config.param_spec.lmbda);
        checkpoint_detail::write_binary(out, config.sim_spec.N_between_samples);
        checkpoint_detail::write_binary(out, config.sim_spec.custom_therm);
        checkpoint_detail::write_string_vector(out, config.sim_spec.observables);
        checkpoint_detail::write_binary(out, state.completed_samples);
        checkpoint_detail::write_binary(out, state.production_metropolis_steps);
        checkpoint_detail::write_binary(out, state.integrated_pot_energy);
        checkpoint_detail::write_binary(out, state.acc_ratio);
        checkpoint_detail::write_diagnostics(out, state.production_acceptance);
        checkpoint_detail::write_observable_series(out, state.observable_vector);
        // M11: completed sample count is the global snapshot marker.
        checkpoint_detail::write_binary(out, state.global_checkpoint_marker);
        // M9: PT ladder position and post-swap runtime couplings (version 3+).
        checkpoint_detail::write_binary(out, state.pt_ladder_index);
        checkpoint_detail::write_binary(out, state.runtime_h);
        checkpoint_detail::write_binary(out, state.runtime_mu);
        checkpoint_detail::write_binary(out, state.runtime_J);
        checkpoint_detail::write_binary(out, state.runtime_lmbda);
        // M11: preserve cumulative PT diagnostics and the ladder trace prefix.
        checkpoint_detail::write_pt_diagnostics(out, state.pt_diagnostics);
        rng->write_state(out);
        out << '\n';
        lat.write_checkpoint_state(out);

        out.flush();
        if (!out) {
            throw std::runtime_error(std::format("Failed while writing checkpoint file \"{}\".", tmp_path));
        }
        out.close();
        if (out.fail()) {
            throw std::runtime_error(std::format("Failed while closing checkpoint file \"{}\".", tmp_path));
        }
        std::filesystem::rename(tmp_path, path);
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
typename ExtendedToricCodeQMC<Basis, LatticeT>::SampleCheckpointState
ExtendedToricCodeQMC<Basis, LatticeT>::read_sample_checkpoint(
    const std::filesystem::path& path,
    const Config& config,
    LatticeT& lat
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Checkpoint restart is supported only for the Boost Lattice backend.");
    } else {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error(std::format("Could not open checkpoint file \"{}\".", path.string()));
        }

        char magic[sizeof(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC)]{};
        in.read(magic, sizeof(magic));
        if (!std::equal(std::begin(magic), std::end(magic), std::begin(checkpoint_detail::SAMPLE_CHECKPOINT_MAGIC))) {
            throw std::runtime_error("Checkpoint magic does not match ParaToric sample checkpoint format.");
        }
        std::uint32_t version = 0;
        checkpoint_detail::read_binary(in, version);
        // M11: accept legacy version 2/3 files as well as current version 4.
        if (version < checkpoint_detail::SAMPLE_CHECKPOINT_MIN_SUPPORTED_VERSION
            || version > checkpoint_detail::SAMPLE_CHECKPOINT_VERSION) {
            throw std::runtime_error("Unsupported checkpoint version.");
        }

        char basis = '\0';
        int system_size = 0;
        double beta = 0.;
        int default_spin = 0;
        double mu = 0.;
        double h = 0.;
        double J = 0.;
        double lmbda = 0.;
        int n_between_samples = 0;
        bool custom_therm = false;
        checkpoint_detail::read_binary(in, basis);
        const auto lattice_type = checkpoint_detail::read_string(in);
        checkpoint_detail::read_binary(in, system_size);
        checkpoint_detail::read_binary(in, beta);
        const auto boundaries = checkpoint_detail::read_string(in);
        checkpoint_detail::read_binary(in, default_spin);
        const auto lattice_backend = checkpoint_detail::read_string(in);
        checkpoint_detail::read_binary(in, mu);
        checkpoint_detail::read_binary(in, h);
        checkpoint_detail::read_binary(in, J);
        checkpoint_detail::read_binary(in, lmbda);
        checkpoint_detail::read_binary(in, n_between_samples);
        checkpoint_detail::read_binary(in, custom_therm);
        const auto observables = checkpoint_detail::read_string_vector(in);

        if (basis != config.lat_spec.basis
            || lattice_type != config.lat_spec.lattice_type
            || system_size != config.lat_spec.system_size
            || beta != config.lat_spec.beta
            || boundaries != config.lat_spec.boundaries
            || default_spin != config.lat_spec.default_spin
            || lattice_backend != config.lat_spec.lattice_backend
            || mu != config.param_spec.mu
            || h != config.param_spec.h
            || J != config.param_spec.J
            || lmbda != config.param_spec.lmbda
            || n_between_samples != config.sim_spec.N_between_samples
            || custom_therm != config.sim_spec.custom_therm
            || observables != config.sim_spec.observables) {
            throw std::runtime_error("Checkpoint configuration does not match this run.");
        }

        SampleCheckpointState state;
        checkpoint_detail::read_binary(in, state.completed_samples);
        checkpoint_detail::read_binary(in, state.production_metropolis_steps);
        checkpoint_detail::read_binary(in, state.integrated_pot_energy);
        checkpoint_detail::read_binary(in, state.acc_ratio);
        checkpoint_detail::read_diagnostics(in, state.production_acceptance);
        state.observable_vector = checkpoint_detail::read_observable_series(in);
        if (version >= 4) {
            checkpoint_detail::read_binary(in, state.global_checkpoint_marker);
        } else {
            // Legacy files have no explicit marker; completed_samples is the
            // implicit marker used by the collective legacy-load check.
            state.global_checkpoint_marker = state.completed_samples;
        }
        // M9: PT ladder position and runtime couplings, version 3+ only. A version-2
        // file has none of this; state.has_pt_state stays false and the caller falls
        // back to pre-M9 behaviour (pt_ladder_index = pt_rank, runtime couplings =
        // launch values) rather than reading garbage or assuming zeros mean anything.
        if (version >= 3) {
            checkpoint_detail::read_binary(in, state.pt_ladder_index);
            checkpoint_detail::read_binary(in, state.runtime_h);
            checkpoint_detail::read_binary(in, state.runtime_mu);
            checkpoint_detail::read_binary(in, state.runtime_J);
            checkpoint_detail::read_binary(in, state.runtime_lmbda);
            state.has_pt_state = true;
        }
        if (version >= 4) {
            state.pt_diagnostics = checkpoint_detail::read_pt_diagnostics(in);
            state.has_pt_diagnostics = true;
        }
        rng->read_state(in);
        in.get();
        lat.read_checkpoint_state(in);
        if (!in) {
            throw std::runtime_error("Checkpoint file ended unexpectedly.");
        }
        if (state.completed_samples < 0 || state.completed_samples > config.sim_spec.N_samples) {
            throw std::runtime_error("Checkpoint completed sample count is outside requested N_samples.");
        }
        return state;
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
bool ExtendedToricCodeQMC<Basis, LatticeT>::checkpoint_due(
    const Config& config,
    std::int64_t completed_samples,
    std::chrono::steady_clock::time_point now,
    std::chrono::steady_clock::time_point& last_checkpoint_time
) {
    bool due = false;
    if (config.out_spec.checkpoint_every_samples > 0
        && completed_samples > 0
        && completed_samples % config.out_spec.checkpoint_every_samples == 0) {
        due = true;
    }
    if (config.out_spec.checkpoint_every_seconds > 0) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_checkpoint_time).count();
        if (elapsed >= config.out_spec.checkpoint_every_seconds) {
            due = true;
        }
    }
    if (due) {
        last_checkpoint_time = now;
    }
    return due;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis, LatticeT>::total_integrated_pot_energy(
    LatticeT& lat, double h, double mu, double J, double lmbda
) {
    double integrated_potential_energy = 0.;
    if constexpr (Basis == 'x') {
        integrated_potential_energy = - lat.total_integrated_edge_energy() * h - lat.total_integrated_star_energy() * mu;
    } else if constexpr (Basis == 'z') {
        integrated_potential_energy = - lat.total_integrated_edge_energy() * lmbda - lat.total_integrated_plaquette_energy() * J;
    }
    return integrated_potential_energy;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis, LatticeT>::parallel_tempering_log_weight_ratio(
    LatticeT& local_lat,
    LatticeT& partner_lat,
    double local_h,
    double partner_h,
    double local_mu,
    double partner_mu,
    double local_lmbda,
    double partner_lmbda,
    double local_J,
    double partner_J,
    bool use_cached
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Parallel tempering is implemented only for the Boost Lattice backend.");
    } else {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("Parallel tempering is implemented only for basis=z.");
        } else {
            const double local_single = use_cached
                ? local_lat.get_non_diag_single_energy_z()
                : local_lat.recompute_non_diag_single_energy_z();
            const double partner_single = use_cached
                ? partner_lat.get_non_diag_single_energy_z()
                : partner_lat.recompute_non_diag_single_energy_z();
            const double local_edge = use_cached
                ? local_lat.cached_total_integrated_edge_energy()
                : local_lat.recompute_total_integrated_edge_energy();
            const double partner_edge = use_cached
                ? partner_lat.cached_total_integrated_edge_energy()
                : partner_lat.recompute_total_integrated_edge_energy();
            const double local_tuple = use_cached
                ? local_lat.get_non_diag_tuple_energy_z()
                : local_lat.recompute_non_diag_tuple_energy_z();
            const double partner_tuple = use_cached
                ? partner_lat.get_non_diag_tuple_energy_z()
                : partner_lat.recompute_non_diag_tuple_energy_z();
            const double local_plaquette = use_cached
                ? local_lat.cached_total_integrated_plaquette_energy()
                : local_lat.recompute_total_integrated_plaquette_energy();
            const double partner_plaquette = use_cached
                ? partner_lat.cached_total_integrated_plaquette_energy()
                : partner_lat.recompute_total_integrated_plaquette_energy();

            return parallel_tempering_log_weight_ratio_from_scalars(
                local_lat.get_beta(),
                local_h, partner_h, local_mu, partner_mu, local_lmbda, partner_lmbda, local_J, partner_J,
                local_single, partner_single, local_tuple, partner_tuple,
                local_edge, partner_edge, local_plaquette, partner_plaquette
            );
        }
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis, LatticeT>::parallel_tempering_log_weight_ratio_from_scalars(
    double beta,
    double local_h, double partner_h,
    double local_mu, double partner_mu,
    double local_lmbda, double partner_lmbda,
    double local_J, double partner_J,
    double local_single, double partner_single,
    double local_tuple, double partner_tuple,
    double local_edge, double partner_edge,
    double local_plaquette, double partner_plaquette
) {
    double log_ratio = 0.;
    if (local_h != partner_h) {
        if (local_h <= 0. || partner_h <= 0.) {
            throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
        }
        log_ratio += beta
            * (local_single - partner_single)
            * std::log(partner_h / local_h);
    }
    if (local_lmbda != partner_lmbda) {
        log_ratio += (partner_lmbda - local_lmbda) * (local_edge - partner_edge);
    }
    // Common-factor (beta,s) tempering: mu and J terms, structurally identical
    // to the h and lmbda terms above (doc/m1_common_factor_swap_weight_derivation.md
    // section 2). Guarded the same way, so with mu/J untempered (local==partner,
    // the pre-existing default) this reproduces the two-term formula bit for bit.
    if (local_mu != partner_mu) {
        if (local_mu <= 0. || partner_mu <= 0.) {
            throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
        }
        log_ratio += beta
            * (local_tuple - partner_tuple)
            * std::log(partner_mu / local_mu);
    }
    if (local_J != partner_J) {
        log_ratio += (partner_J - local_J) * (local_plaquette - partner_plaquette);
    }
    return log_ratio;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis, LatticeT>::validate_parallel_tempering_swap_weight_debug(
    const Config& config,
    std::vector<double>* log_weight_trace
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("PT swap-weight validation is implemented only for the Boost Lattice backend.");
    } else {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("PT swap-weight validation is implemented only for basis=z.");
        } else {
            if (config.pt_spec.ladder_s.size() < 2) {
                throw std::invalid_argument("PT swap-weight validation requires at least two ladder values.");
            }
            if (!config.pt_spec.ladder_lmbda.empty()
                && config.pt_spec.ladder_lmbda.size() != config.pt_spec.ladder_s.size()) {
                throw std::invalid_argument("PT lambda ladder size must match the h ladder size.");
            }
            if (config.param_spec.mu != 1.0 || config.param_spec.J != 1.0) {
                throw std::invalid_argument("PT ladder requires mu=J=1.");
            }

            auto rng_a = std::make_shared<RNG>();
            auto rng_b = std::make_shared<RNG>();
            rng_a->set_seed(config.sim_spec.seed == 0 ? 17 : config.sim_spec.seed);
            rng_b->set_seed(config.sim_spec.seed == 0 ? 29 : config.sim_spec.seed + 1);
            rng->set_seed(config.sim_spec.seed == 0 ? 43 : config.sim_spec.seed + 2);
            LatticeT local_lat(config.lat_spec, rng_a);
            LatticeT partner_lat(config.lat_spec, rng_b);

            const auto& ladder_lmbda = config.pt_spec.ladder_lmbda.empty()
                ? config.pt_spec.ladder_s : config.pt_spec.ladder_lmbda;
            const double local_h = config.pt_spec.ladder_s[0];
            const double partner_h = config.pt_spec.ladder_s[1];
            const double local_lmbda = ladder_lmbda[0];
            const double partner_lmbda = ladder_lmbda[1];
            double local_energy = total_integrated_pot_energy(local_lat, local_h, 1.0, 1.0, local_lmbda);
            double partner_energy = total_integrated_pot_energy(partner_lat, partner_h, 1.0, 1.0, partner_lmbda);
            double acc_ratio = 1.;
            double max_deviation = 0.;
            const std::int64_t steps =
                std::max(config.sim_spec.N_thermalization, std::int64_t{0})
                + static_cast<std::int64_t>(std::max(config.sim_spec.N_samples, 1))
                    * static_cast<std::int64_t>(std::max(config.sim_spec.N_between_samples, 1));
            const std::int64_t swap_period = parallel_tempering_swap_period_moves(config.pt_spec, config.lat_spec);
            for (std::int64_t step = 1; step <= steps; ++step) {
                metropolis_step(local_lat, local_energy, acc_ratio, config.lat_spec.beta, local_h, 1.0, 1.0, local_lmbda);
                metropolis_step(partner_lat, partner_energy, acc_ratio, config.lat_spec.beta, partner_h, 1.0, 1.0, partner_lmbda);
                if (step % swap_period == 0) {
                    const double cached = parallel_tempering_log_weight_ratio(
                        local_lat, partner_lat, local_h, partner_h, 1.0, 1.0,
                        local_lmbda, partner_lmbda, 1.0, 1.0, true);
                    const double brute = parallel_tempering_log_weight_ratio(
                        local_lat, partner_lat, local_h, partner_h, 1.0, 1.0,
                        local_lmbda, partner_lmbda, 1.0, 1.0, false);
                    if (log_weight_trace != nullptr) {
                        log_weight_trace->emplace_back(cached);
                    }
                    max_deviation = std::max(max_deviation, std::abs(cached - brute));
                }
            }
            return max_deviation;
        }
    }
}

// M2 test scaffolding (see the doc comment on the declaration). Does not modify
// parallel_tempering_log_weight_ratio; it only calls it, unmodified, and compares
// its result to an independently-computed expected value.
template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
typename ExtendedToricCodeQMC<Basis, LatticeT>::GeneralizedSwapWeightCheck
ExtendedToricCodeQMC<Basis, LatticeT>::validate_parallel_tempering_swap_weight_generalized_debug(
    const Config& config,
    double local_mu, double partner_mu,
    double local_J, double partner_J
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Generalized PT swap-weight check is implemented only for the Boost Lattice backend.");
    } else {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("Generalized PT swap-weight check is implemented only for basis=z.");
        } else {
            if (config.pt_spec.ladder_s.size() < 2) {
                throw std::invalid_argument("Generalized PT swap-weight check requires at least two ladder values.");
            }
            const auto& ladder_lmbda = config.pt_spec.ladder_lmbda.empty()
                ? config.pt_spec.ladder_s : config.pt_spec.ladder_lmbda;
            if (ladder_lmbda.size() < 2) {
                throw std::invalid_argument("Generalized PT swap-weight check requires at least two lambda ladder values.");
            }

            auto rng_a = std::make_shared<RNG>();
            auto rng_b = std::make_shared<RNG>();
            rng_a->set_seed(config.sim_spec.seed == 0 ? 17 : config.sim_spec.seed);
            rng_b->set_seed(config.sim_spec.seed == 0 ? 29 : config.sim_spec.seed + 1);
            rng->set_seed(config.sim_spec.seed == 0 ? 43 : config.sim_spec.seed + 2);
            LatticeT local_lat(config.lat_spec, rng_a);
            LatticeT partner_lat(config.lat_spec, rng_b);

            const double local_h = config.pt_spec.ladder_s[0];
            const double partner_h = config.pt_spec.ladder_s[1];
            const double local_lmbda = ladder_lmbda[0];
            const double partner_lmbda = ladder_lmbda[1];

            double local_energy = total_integrated_pot_energy(local_lat, local_h, local_mu, local_J, local_lmbda);
            double partner_energy = total_integrated_pot_energy(partner_lat, partner_h, partner_mu, partner_J, partner_lmbda);
            double acc_ratio = 1.;
            const std::int64_t steps =
                std::max(config.sim_spec.N_thermalization, std::int64_t{0})
                + static_cast<std::int64_t>(std::max(config.sim_spec.N_samples, 1))
                    * static_cast<std::int64_t>(std::max(config.sim_spec.N_between_samples, 1));
            for (std::int64_t step = 1; step <= steps; ++step) {
                metropolis_step(local_lat, local_energy, acc_ratio, config.lat_spec.beta, local_h, local_mu, local_J, local_lmbda);
                metropolis_step(partner_lat, partner_energy, acc_ratio, config.lat_spec.beta, partner_h, partner_mu, partner_J, partner_lmbda);
            }

            // Production function, all four couplings (post-M3).
            const double production = parallel_tempering_log_weight_ratio(
                local_lat, partner_lat, local_h, partner_h, local_mu, partner_mu,
                local_lmbda, partner_lmbda, local_J, partner_J, false);

            // Expected: general four-coupling formula, doc/m1_common_factor_swap_weight_derivation.md
            // section 2, evaluated from-scratch (recompute_*) on the exact final configurations.
            const double local_single = local_lat.recompute_non_diag_single_energy_z();
            const double partner_single = partner_lat.recompute_non_diag_single_energy_z();
            const double local_tuple = local_lat.recompute_non_diag_tuple_energy_z();
            const double partner_tuple = partner_lat.recompute_non_diag_tuple_energy_z();
            const double local_edge = local_lat.recompute_total_integrated_edge_energy();
            const double partner_edge = partner_lat.recompute_total_integrated_edge_energy();
            const double local_plaquette = local_lat.recompute_total_integrated_plaquette_energy();
            const double partner_plaquette = partner_lat.recompute_total_integrated_plaquette_energy();
            const double beta = config.lat_spec.beta;

            double expected = 0.;
            if (local_h != partner_h) {
                expected += beta * (local_single - partner_single) * std::log(partner_h / local_h);
            }
            if (local_mu != partner_mu) {
                expected += beta * (local_tuple - partner_tuple) * std::log(partner_mu / local_mu);
            }
            if (local_lmbda != partner_lmbda) {
                expected += (partner_lmbda - local_lmbda) * (local_edge - partner_edge);
            }
            if (local_J != partner_J) {
                expected += (partner_J - local_J) * (local_plaquette - partner_plaquette);
            }

            return GeneralizedSwapWeightCheck{production, expected};
        }
    }
}

// M3 non-circular checks (see the doc comment on the declaration). Does not call
// parallel_tempering_log_weight_ratio and does not reuse the M1 derivation's
// expression.
template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
typename ExtendedToricCodeQMC<Basis, LatticeT>::SingleReplicaDiagnostics
ExtendedToricCodeQMC<Basis, LatticeT>::validate_single_replica_diagnostics_debug(
    const Config& config, double h, double mu, double J, double lmbda, double eps
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Single-replica diagnostics are implemented only for the Boost Lattice backend.");
    } else {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("Single-replica diagnostics are implemented only for basis=z.");
        } else {
            auto rng_a = std::make_shared<RNG>();
            rng_a->set_seed(config.sim_spec.seed == 0 ? 17 : config.sim_spec.seed);
            rng->set_seed(config.sim_spec.seed == 0 ? 43 : config.sim_spec.seed + 2);
            LatticeT lat(config.lat_spec, rng_a);

            double energy = total_integrated_pot_energy(lat, h, mu, J, lmbda);
            double acc_ratio = 1.;
            const std::int64_t steps =
                std::max(config.sim_spec.N_thermalization, std::int64_t{0})
                + static_cast<std::int64_t>(std::max(config.sim_spec.N_samples, 1))
                    * static_cast<std::int64_t>(std::max(config.sim_spec.N_between_samples, 1));
            for (std::int64_t step = 1; step <= steps; ++step) {
                metropolis_step(lat, energy, acc_ratio, config.lat_spec.beta, h, mu, J, lmbda);
            }

            const double pot_lmbda_plus = total_integrated_pot_energy(lat, h, mu, J, lmbda + eps);
            const double pot_lmbda_minus = total_integrated_pot_energy(lat, h, mu, J, lmbda - eps);
            const double finite_diff_dlmbda = (pot_lmbda_plus - pot_lmbda_minus) / (2. * eps);

            const double pot_J_plus = total_integrated_pot_energy(lat, h, mu, J + eps, lmbda);
            const double pot_J_minus = total_integrated_pot_energy(lat, h, mu, J - eps, lmbda);
            const double finite_diff_dJ = (pot_J_plus - pot_J_minus) / (2. * eps);

            const double E_edge = lat.cached_total_integrated_edge_energy();
            const double E_plaquette = lat.cached_total_integrated_plaquette_energy();
            const double beta = config.lat_spec.beta;
            const double beta_times_single = beta * lat.get_non_diag_single_energy_z();
            const double beta_times_tuple = beta * lat.get_non_diag_tuple_energy_z();
            const auto kLkR = lat.get_kL_kR_single();
            const double kL_plus_kR_single = kLkR.real() + kLkR.imag();
            const double recompute_beta_times_tuple = beta * lat.recompute_non_diag_tuple_energy_z();

            return SingleReplicaDiagnostics{
                finite_diff_dlmbda, finite_diff_dJ, E_edge, E_plaquette,
                beta_times_single, beta_times_tuple, kL_plus_kR_single, recompute_beta_times_tuple
            };
        }
    }
}

// M7'' exact-identity check (see the doc comment on the declaration). Does not
// route any configuration to another rung -- swap acceptance is never applied.
// Calls the PRODUCTION parallel_tempering_log_weight_ratio directly.
template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
typename ExtendedToricCodeQMC<Basis, LatticeT>::WeightRatioIdentityCheck
ExtendedToricCodeQMC<Basis, LatticeT>::validate_swap_weight_ratio_identity_debug(
    const Config& config_i, const Config& config_j
) {
    if constexpr (!std::same_as<LatticeT, Lattice>) {
        throw std::invalid_argument("Weight-ratio identity check is implemented only for the Boost Lattice backend.");
    } else {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("Weight-ratio identity check is implemented only for basis=z.");
        } else {
            auto rng_i = std::make_shared<RNG>();
            rng_i->set_seed(config_i.sim_spec.seed == 0 ? 17 : static_cast<std::uint64_t>(config_i.sim_spec.seed));
            auto rng_j = std::make_shared<RNG>();
            rng_j->set_seed(config_j.sim_spec.seed == 0 ? 29 : static_cast<std::uint64_t>(config_j.sim_spec.seed));
            LatticeT lat_i(config_i.lat_spec, rng_i);
            LatticeT lat_j(config_j.lat_spec, rng_j);

            const double h_i = config_i.param_spec.h, mu_i = config_i.param_spec.mu,
                J_i = config_i.param_spec.J, lmbda_i = config_i.param_spec.lmbda;
            const double h_j = config_j.param_spec.h, mu_j = config_j.param_spec.mu,
                J_j = config_j.param_spec.J, lmbda_j = config_j.param_spec.lmbda;

            double energy_i = total_integrated_pot_energy(lat_i, h_i, mu_i, J_i, lmbda_i);
            double energy_j = total_integrated_pot_energy(lat_j, h_j, mu_j, J_j, lmbda_j);
            double acc_i = 1., acc_j = 1.;

            // Independent thermalization, no exchange of any kind between the two chains.
            for (std::int64_t step = 0; step < config_i.sim_spec.N_thermalization; ++step) {
                metropolis_step(lat_i, energy_i, acc_i, config_i.lat_spec.beta, h_i, mu_i, J_i, lmbda_i);
            }
            for (std::int64_t step = 0; step < config_j.sim_spec.N_thermalization; ++step) {
                metropolis_step(lat_j, energy_j, acc_j, config_j.lat_spec.beta, h_j, mu_j, J_j, lmbda_j);
            }

            const std::int64_t N = std::max(static_cast<std::int64_t>(config_i.sim_spec.N_samples), std::int64_t{1});
            const std::int64_t between = std::max(static_cast<std::int64_t>(config_i.sim_spec.N_between_samples), std::int64_t{1});

            std::vector<double> logR;
            logR.reserve(static_cast<size_t>(N));
            for (std::int64_t s = 0; s < N; ++s) {
                for (std::int64_t k = 0; k < between; ++k) {
                    metropolis_step(lat_i, energy_i, acc_i, config_i.lat_spec.beta, h_i, mu_i, J_i, lmbda_i);
                    metropolis_step(lat_j, energy_j, acc_j, config_j.lat_spec.beta, h_j, mu_j, J_j, lmbda_j);
                }
                // "local" = i, "partner" = j, the exact production function; use_cached
                // matches how the real inline blocks call it during actual swap checks.
                const double log_r = parallel_tempering_log_weight_ratio(
                    lat_i, lat_j, h_i, h_j, mu_i, mu_j, lmbda_i, lmbda_j, J_i, J_j, true);
                logR.push_back(log_r);
            }

            std::vector<double> R(logR.size());
            double acceptance_mean = 0.;
            for (std::size_t k = 0; k < logR.size(); ++k) {
                R[k] = std::exp(logR[k]);
                acceptance_mean += std::min(1.0, R[k]);
            }
            acceptance_mean /= static_cast<double>(R.size());

            auto boot_rng = std::make_shared<RNG>();
            boot_rng->set_seed(config_i.sim_spec.seed == 0 ? 999
                : static_cast<std::uint64_t>(config_i.sim_spec.seed) + 999);
            const auto [R_mean, R_sem, R_binder, R_binder_sem] =
                paratoric::statistics::get_bootstrap_statistics(R, boot_rng);
            (void)R_binder; (void)R_binder_sem;

            double logR_mean = 0.;
            for (double v : logR) logR_mean += v;
            logR_mean /= static_cast<double>(logR.size());
            double logR_var = 0.;
            for (double v : logR) logR_var += (v - logR_mean) * (v - logR_mean);
            logR_var /= static_cast<double>(logR.size() > 1 ? logR.size() - 1 : 1);

            return WeightRatioIdentityCheck{
                R_mean, R_sem, acceptance_mean, logR_mean, std::sqrt(logR_var), logR.size()
            };
        }
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::tuple<double, double>
ExtendedToricCodeQMC<Basis, LatticeT>::integrated_pot_energy_diff_single_spin_flip_edge(
    LatticeT& lat, double h, double mu, double J, double lmbda, 
    const typename LatticeT::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache
) {
    double delta_energy_single = 0.;
    double bare_energy_single = 0.;
    if (total_cache) {
        bare_energy_single = -2*lat.get_potential_edge_energy(edg);
    } else {
        bare_energy_single = lat.integrated_edge_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip);
    }

    if constexpr (Basis == 'x') {
        delta_energy_single = -h * bare_energy_single;
    } else if constexpr (Basis == 'z') {
        delta_energy_single = -lmbda * bare_energy_single;
    }

    return {delta_energy_single, bare_energy_single};
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::tuple<double, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallIndexVector, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallEnergyVector> 
ExtendedToricCodeQMC<Basis, LatticeT>::integrated_pot_energy_diff_single_spin_flip_tuple(
    LatticeT& lat, double h, double mu, double J, double lmbda, 
    const typename LatticeT::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache
) {
    double delta_energy_tuple = 0.;
    if constexpr (Basis == 'x') {
        const auto& [bare_energy, star_centers, bare_star_potential_energy_diffs] 
        = lat.integrated_star_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip, total_cache);
        delta_energy_tuple = -mu * bare_energy; // No "/ 2"!
        //for (double& x : bare_star_potential_energy_diffs) x *= -mu;
        return {delta_energy_tuple, std::move(star_centers), std::move(bare_star_potential_energy_diffs)};
    } else if constexpr (Basis == 'z') {
        const auto& [bare_energy, plaquette_indices, bare_plaquette_potential_energy_diffs] 
        = lat.integrated_plaquette_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip, total_cache);
        delta_energy_tuple = -J * bare_energy; // No "/ 2"!
        //for (double& x : bare_plaquette_potential_energy_diffs) x *= -J;
        return {delta_energy_tuple, std::move(plaquette_indices), std::move(bare_plaquette_potential_energy_diffs)};
    } 
    return {0., SmallIndexVector{}, SmallEnergyVector{}};
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::tuple<double, std::span<const typename LatticeT::Edge>, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallEnergyVector> 
ExtendedToricCodeQMC<Basis, LatticeT>::integrated_pot_energy_diff_tuple_flip_edge(
    LatticeT& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
    double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache,
    bool interval_has_no_inner_flips
) {
    UNUSED(tuple_index);
    UNUSED(mu);
    UNUSED(J);
    double delta_energy_single = 0.;
    SmallEnergyVector bare_energy_single_vector;
    bare_energy_single_vector.reserve(tuple_edges.size());
    double bare_energy_edg = 0.;
    for (const typename LatticeT::Edge& edg : tuple_edges) {
        if (total_cache) { 
            bare_energy_edg = -2*lat.get_potential_edge_energy(edg);
        } else if (interval_has_no_inner_flips) {
            bare_energy_edg = lat.integrated_edge_energy_diff_no_inner_flips(
                edg, imag_time_spin_flip, imag_time_next_spin_flip
            );
        } else {
            bare_energy_edg = lat.integrated_edge_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip);
        }
        bare_energy_single_vector.emplace_back(bare_energy_edg);

        if constexpr (Basis == 'x') {
            delta_energy_single += -h * bare_energy_edg;
        } else if constexpr (Basis == 'z') {
            delta_energy_single += -lmbda * bare_energy_edg;
        }
    }
    return {delta_energy_single, tuple_edges, std::move(bare_energy_single_vector)};
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::tuple<double, std::span<const typename LatticeT::Edge>, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallEnergyVector> 
ExtendedToricCodeQMC<Basis, LatticeT>::integrated_pot_energy_diff_combination_flip_edge(
    LatticeT& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
    double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
    double tau_left, double tau_right, 
    const SmallBoolVector& create_vector, bool tuple_destroy
) {
    double energy_single_diff = 0.; 
    SmallEnergyVector bare_energy_single_vector;
    bare_energy_single_vector.reserve(tuple_edges.size());
    for (size_t i = 0; i < tuple_edges.size(); ++i) {
        const auto edg = tuple_edges[i];

        double bare_energy_edg = lat.integrated_edge_energy_diff_combination(
            edg,
            tau_left,
            tau_right,
            imag_time_tuple_flip,
            imag_time_spin_flips[i]
        );
        bare_energy_single_vector.emplace_back(bare_energy_edg);
        if constexpr (Basis == 'x') {
            energy_single_diff += -h * bare_energy_edg;
        }  else if constexpr (Basis == 'z') {
            energy_single_diff += -lmbda * bare_energy_edg;
        }
    }

    return {energy_single_diff, tuple_edges, std::move(bare_energy_single_vector)};
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
std::tuple<double, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallIndexVector, typename ExtendedToricCodeQMC<Basis, LatticeT>::SmallEnergyVector> 
ExtendedToricCodeQMC<Basis, LatticeT>::integrated_pot_energy_diff_combination_flip_tuple(
    LatticeT& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
    double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
    double tau_left, double tau_right, 
    const SmallBoolVector& create_vector, bool tuple_destroy
) {
    double energy_tuple_diff = 0.; 
    if constexpr (Basis == 'x') {
        const auto& [bare_energy, star_centers, bare_star_potential_energy_diffs] 
        = lat.integrated_star_energy_diff_combination(
            tuple_index, tau_left, tau_right, 
            std::span<const double>(imag_time_spin_flips.data(), imag_time_spin_flips.size()), 
            imag_time_tuple_flip
        );
        energy_tuple_diff += -mu * bare_energy;
        return {energy_tuple_diff, std::move(star_centers), std::move(bare_star_potential_energy_diffs)};
    }  else if constexpr (Basis == 'z') {
        const auto& [bare_energy, plaquette_indices, bare_plaquette_potential_energy_diffs] = 
        lat.integrated_plaquette_energy_diff_combination(
            tuple_index, tau_left, tau_right, 
            std::span<const double>(imag_time_spin_flips.data(), imag_time_spin_flips.size()), 
            imag_time_tuple_flip
        );
        energy_tuple_diff += -J * bare_energy;
        return {energy_tuple_diff, std::move(plaquette_indices), std::move(bare_plaquette_potential_energy_diffs)};
    }
    return {0., SmallIndexVector{}, SmallEnergyVector{}};
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::combination_flip(
    LatticeT& lat, double h, double mu, 
    int tuple_index, std::span<const typename LatticeT::Edge> tuple_edges, 
    double imag_time_tuple_flip, const SmallEnergyVector& imag_time_spin_flips, 
    const SmallBoolVector& create_vector, bool tuple_destroy
) {
    UNUSED(h);
    UNUSED(mu);
    if (tuple_destroy) {
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            const auto edg = tuple_edges[i];

            if (create_vector[i]) {
                lat.insert_single_spin_flip(edg, imag_time_spin_flips[i]);
            } else {
                lat.delete_single_spin_flip(edg, lat.get_spin_flip_index(edg, imag_time_spin_flips[i]));
            }
        }
        lat.delete_tuple_flip(tuple_index, tuple_edges, imag_time_tuple_flip);
    } else {
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            const auto edg = tuple_edges[i];

            if (create_vector[i]) {
                lat.insert_single_spin_flip(edg, imag_time_spin_flips[i]);
            } else {
                lat.delete_single_spin_flip(edg, lat.get_spin_flip_index(edg, imag_time_spin_flips[i]));
            }
        }
        lat.insert_tuple_flip(tuple_index, tuple_edges, imag_time_tuple_flip);
    } 
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_double_single_spin_flip(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_double_single_spin_flip!";
#endif

    if (Basis == 'x' && (lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    const double rnd_create_destroy = uniform_dist(*rng);

#ifndef NDEBUG
    const auto& [rand_edge, source, target] = lat.get_random_edge();
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Randomly chose edge between vertices {} and {}.", source, target);
#else
    const auto rand_edge = lat.get_random_edge_descriptor();
#endif
    
    std::span<const double> single_spin_flips = lat.get_single_spin_flips(rand_edge);
    int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Found {} single spin flips.", single_spin_flip_count);
#endif

    if (rnd_create_destroy < 0.5) { // destroys a pair of single spin flips
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Trying to DESTROY single spin flip pair.";
#endif
        if (single_spin_flip_count > 1) [[likely]] {
            const int random_spin_flip_index = random_index(single_spin_flip_count - 1);

            const double imag_time_spin_flip = single_spin_flips[random_spin_flip_index];

            const double imag_time_next_spin_flip = single_spin_flips[random_spin_flip_index+1];

            double imag_time_next_next_spin_flip = beta;

            if (random_spin_flip_index != single_spin_flip_count - 2) [[likely]] {
                imag_time_next_next_spin_flip = single_spin_flips[random_spin_flip_index+2];
            }

            double imag_time_prev_spin_flip = 0.;

            if (random_spin_flip_index != 0) [[likely]] {
                imag_time_prev_spin_flip = single_spin_flips[random_spin_flip_index-1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Imaginary time of random single spin flip: {}; previous: {}; next: {}; next next: {}", 
                imag_time_spin_flip, imag_time_prev_spin_flip, imag_time_next_spin_flip, imag_time_next_next_spin_flip);
#endif            
            const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
            = integrated_pot_energy_diff_single_spin_flip_edge(
                lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, imag_time_next_spin_flip, false
            );
            const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
            = integrated_pot_energy_diff_single_spin_flip_tuple(
                lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, imag_time_next_spin_flip, false
            );
            const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
            if constexpr (Basis == 'x') {
                acc_ratio = 2./(lmbda * lmbda * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip) 
                * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip)) * std::exp(-integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = 2./(h * h * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip) 
                * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip)) * std::exp(-integrated_pot_energy_diff);
            }
            

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif   
            
            if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- ACCEPTED.";
#endif   
                lat.delete_double_single_spin_flip(rand_edge, imag_time_spin_flip, imag_time_next_spin_flip);
                integrated_pot_energy += integrated_pot_energy_diff;
                lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                    int tuple_index = pot_energy_tuple_indices[i];
                    double potential_tuple_energy_diff = pot_energy_diffs[i];
                    if constexpr (Basis == 'x') {
                        lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                    } else if constexpr (Basis == 'z') {
                        lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                    }
                }
            } 
        } else [[unlikely]] {  
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.;
        }
    } else { // create a pair of single spin flips
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Trying to CREATE single spin flip pair.";
#endif
        double tau_left, tau_right;
        int random_spin_flip_index = 0;

        if (single_spin_flip_count == 0) {
            tau_left = 0.;
            tau_right = beta;
        } else {
            random_spin_flip_index = random_index(single_spin_flip_count + 1);

            if (random_spin_flip_index == 0) [[unlikely]] {
                tau_left = 0.; // not really necessary
                tau_right = single_spin_flips[0];
            } else if (random_spin_flip_index == single_spin_flip_count) [[unlikely]] {
                tau_left = single_spin_flips[random_spin_flip_index-1];
                tau_right = beta;
            } else [[likely]] {
                tau_left = single_spin_flips[random_spin_flip_index-1];
                tau_right = single_spin_flips[random_spin_flip_index];
            }
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif

        double tau_1 = 0., tau_2 = 0.;

        tau_1 = uniform_real(tau_left, tau_right);
        tau_2 = uniform_real(tau_left, tau_right);

        if (tau_2 < tau_1) {
            std::swap(tau_1, tau_2);
        }

        if (std::abs(tau_2 - tau_1) < PRECISION 
        || std::abs(tau_1 - tau_left) < PRECISION 
        || std::abs(tau_2 - tau_right) < PRECISION
        ) [[unlikely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Random numbers very close to each other.";
#endif
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        } else [[likely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- tau_1: {}; tau_2: {}", tau_1, tau_2);
#endif
            const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
            = integrated_pot_energy_diff_single_spin_flip_edge(lat, h, mu, J, lmbda, rand_edge, tau_1, tau_2, false);
            const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
            = integrated_pot_energy_diff_single_spin_flip_tuple(lat, h, mu, J, lmbda, rand_edge, tau_1, tau_2, false);
            const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
            if constexpr (Basis == 'x') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * lmbda * lmbda * std::exp(-integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * h * h * std::exp(-integrated_pot_energy_diff);
            }
            

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif   

            if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- ACCEPTED.";
#endif  
                lat.insert_double_single_spin_flip(rand_edge, tau_1, tau_2);
                integrated_pot_energy += integrated_pot_energy_diff;
                lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                    int tuple_index = pot_energy_tuple_indices[i];
                    double potential_tuple_energy_diff = pot_energy_diffs[i];
                    if constexpr (Basis == 'x') {
                        lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                    } else if constexpr (Basis == 'z') {
                        lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                    }
                }
            } 
        }  
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_single_spin_flip_move(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_single_spin_flip_move!";
#endif

    if (Basis == 'x' && (lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

#ifndef NDEBUG
    const auto& [rand_edge, source, target] = lat.get_random_edge();
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Randomly chose edge between vertices {} and {}.", source, target);    
#else
    const auto rand_edge = lat.get_random_edge_descriptor();
#endif

    std::span<const double> single_spin_flips = lat.get_single_spin_flips(rand_edge);
    int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Found {} single spin flips.", single_spin_flip_count);  
#endif    

    if (single_spin_flip_count != 0) [[likely]] {
        const int random_spin_flip_index = random_index(single_spin_flip_count);

        const double imag_time_spin_flip = single_spin_flips[random_spin_flip_index];

        const double imag_time_next_spin_flip = lat.flip_next_imag_time(rand_edge, imag_time_spin_flip);

        const double imag_time_prev_spin_flip = lat.flip_prev_imag_time(rand_edge, imag_time_spin_flip);

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Imaginary time of random single spin flip: {}; previous: {}; next: {}", imag_time_spin_flip, imag_time_prev_spin_flip, imag_time_next_spin_flip);
#endif  

        const int random_spin_flip_index_lat = lat.get_spin_flip_index(rand_edge, imag_time_spin_flip);

        if (imag_time_prev_spin_flip < imag_time_spin_flip && imag_time_spin_flip < imag_time_next_spin_flip) {
            const double new_imag_time = uniform_real(imag_time_prev_spin_flip, imag_time_next_spin_flip);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- New imaginary time: {}", new_imag_time);
#endif  

            if (std::abs(new_imag_time - imag_time_spin_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                double integrated_pot_energy_diff_edge = 0.;
                double bare_pot_energy_diff_edge = 0.;
                //double integrated_pot_energy_diff_tuple = 0.;
                SmallIndexVector pot_energy_tuple_indices;
                SmallEnergyVector pot_energy_diffs;
                if (new_imag_time > imag_time_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    pot_energy_tuple_indices = std::move(pot_energy_tuple_indices_tmp);
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
                } else {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    pot_energy_tuple_indices = std::move(pot_energy_tuple_indices_tmp);
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
                }
                acc_ratio = std::exp(-integrated_pot_energy_diff);

#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif              
                    lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                    integrated_pot_energy += integrated_pot_energy_diff;
                    lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                    for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                        int tuple_index = pot_energy_tuple_indices[i];
                        double potential_tuple_energy_diff = pot_energy_diffs[i];
                        if constexpr (Basis == 'x') {
                            lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                        } else if constexpr (Basis == 'z') {
                            lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                        }
                    }
                } 

            }
        } else { // Potentially cross beta
            const double new_imag_time = modulo(uniform_real(imag_time_prev_spin_flip, beta + imag_time_next_spin_flip), beta);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- New imaginary time: {}", new_imag_time);
#endif  
            
            if (std::abs(new_imag_time - imag_time_spin_flip) < PRECISION || std::abs(new_imag_time - imag_time_prev_spin_flip) < PRECISION || std::abs(new_imag_time - imag_time_next_spin_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                double integrated_pot_energy_diff_edge = 0.;
                double bare_pot_energy_diff_edge = 0.;
                //double integrated_pot_energy_diff_tuple = 0.;
                //std::vector<int> pot_energy_tuple_indices;
                //std::vector<double> pot_energy_diffs;

                if (random_spin_flip_index_lat == 0 && new_imag_time < imag_time_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(-integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 && imag_time_spin_flip < new_imag_time) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat == 0 && imag_time_spin_flip < new_imag_time && new_imag_time < imag_time_next_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 
                && imag_time_spin_flip > new_imag_time && new_imag_time > imag_time_prev_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat == 0 
                && imag_time_spin_flip < new_imag_time && new_imag_time > imag_time_next_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, beta, false
                    );
                    auto [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, beta, false
                    );
                    if (imag_time_spin_flip > 0) {
                        auto tmp = integrated_pot_energy_diff_single_spin_flip_edge(
                            lat, h, mu, J, lmbda, rand_edge, 0., imag_time_spin_flip, false
                        );
                        integrated_pot_energy_diff_edge += std::get<0>(tmp);
                        bare_pot_energy_diff_edge += std::get<1>(tmp);
                        const auto& [integrated_pot_energy_diff_tuple_2, pot_energy_tuple_indices_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_single_spin_flip_tuple(
                            lat, h, mu, J, lmbda, rand_edge, 0., imag_time_spin_flip, false
                        );
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff_tuple += integrated_pot_energy_diff_tuple_2;
                    }
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED. Flipping spin.";
#endif 
                        lat.flip_spin(rand_edge);
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 
                && imag_time_spin_flip > new_imag_time && new_imag_time < imag_time_prev_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, beta, false
                    );
                    auto [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, beta, false
                    );
                    if (new_imag_time > 0) {
                        auto tmp = integrated_pot_energy_diff_single_spin_flip_edge(
                            lat, h, mu, J, lmbda, rand_edge, 0., new_imag_time, false
                        );
                        integrated_pot_energy_diff_edge += std::get<0>(tmp);
                        bare_pot_energy_diff_edge += std::get<1>(tmp);
                        const auto& [integrated_pot_energy_diff_tuple_2, pot_energy_tuple_indices_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_single_spin_flip_tuple(
                            lat, h, mu, J, lmbda, rand_edge, 0., new_imag_time, false
                        );
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff_tuple += integrated_pot_energy_diff_tuple_2;
                    }
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED. Flipping spin.";
#endif 
                        lat.flip_spin(rand_edge);
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                }
            }
        }
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_global_single_spin_flip(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_global_single_spin_flip!";
#endif

#ifndef NDEBUG
    const auto& [rand_edge, source, target] = lat.get_random_edge();
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- Randomly chose edge between vertices {} and {}.", source, target);
#else
    const auto rand_edge = lat.get_random_edge_descriptor();
#endif

    const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
    = integrated_pot_energy_diff_single_spin_flip_edge(
        lat, h, mu, J, lmbda, rand_edge, 0., beta, true
    );
    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
    = integrated_pot_energy_diff_single_spin_flip_tuple(
        lat, h, mu, J, lmbda, rand_edge, 0., beta, true
    );
    const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif  

    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_global_single_spin_flip --- ACCEPTED.";
#endif 
        lat.flip_spin(rand_edge);
        integrated_pot_energy += integrated_pot_energy_diff;
        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
            int tuple_index = pot_energy_tuple_indices[i];
            double potential_tuple_energy_diff = pot_energy_diffs[i];
            if constexpr (Basis == 'x') {
                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
            } else if constexpr (Basis == 'z') {
                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
            }
        } 
    } 
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_global_tuple_flip(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_global_tuple_flip!";
#endif

    int random_tuple = -1;
    std::span<const typename LatticeT::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_tuple_flip --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_tuple_flip --- Randomly chose star with center {}", random_tuple);
    }
#endif

    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
    = integrated_pot_energy_diff_tuple_flip_edge(lat, h, mu, J, lmbda, random_tuple, tuple_edges, 0, beta, true);

    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif  

    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_global_tuple_flip --- ACCEPTED.";
#endif 
        integrated_pot_energy += integrated_pot_energy_diff;
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            auto edg = tuple_edges[i];
            lat.flip_spin(edg);
            lat.add_potential_edge_energy(edg, pot_energy_diffs[i]);
        }
    } 

}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_double_tuple_flip(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_double_tuple_flip!";
#endif

    if (Basis == 'x' && (J == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    const double rnd_create_destroy = uniform_dist(*rng);
    
    int random_tuple = -1;
    std::span<const typename LatticeT::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Randomly chose star with center ", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Found {} tuple flips.", tuple_flip_count);
#endif  

    if (rnd_create_destroy < 0.5) { // destroy tuples
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Trying to DESTROY tuple flip pair.";
#endif
        if (tuple_flip_count > 1) [[likely]] {
            const int random_tuple_flip_index = random_index(tuple_flip_count - 1);

            const double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];

            const double imag_time_next_tuple_flip = tuple_flips[random_tuple_flip_index+1];

            double imag_time_next_next_tuple_flip = beta;

            if (random_tuple_flip_index != tuple_flip_count - 2) [[likely]] {
                imag_time_next_next_tuple_flip = tuple_flips[random_tuple_flip_index+2];
            }

            double imag_time_prev_tuple_flip = 0.;

            if (random_tuple_flip_index != 0) [[likely]] {
                imag_time_prev_tuple_flip = tuple_flips[random_tuple_flip_index-1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Imaginary time of random tuple flip: {}; previous: {}; next: {}; next next: {}", imag_time_tuple_flip, imag_time_prev_tuple_flip, imag_time_next_tuple_flip, imag_time_next_next_tuple_flip);
#endif         

            const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
            = integrated_pot_energy_diff_tuple_flip_edge(
                lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                imag_time_tuple_flip, imag_time_next_tuple_flip, false
            );

            if constexpr (Basis == 'x') {
                acc_ratio = 2./(J * J * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip) 
                * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip)) 
                * std::exp(- integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = 2./(mu * mu * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip) 
                * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip)) 
                * std::exp(- integrated_pot_energy_diff);
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- acceptance ratio: ", acc_ratio);
#endif  
            
            if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- ACCEPTED.";
#endif  
                lat.delete_double_tuple_flip(
                    random_tuple, tuple_edges, imag_time_tuple_flip, imag_time_next_tuple_flip
                );
                integrated_pot_energy += integrated_pot_energy_diff;
                for (size_t i = 0; i < tuple_edges.size(); ++i) {
                    lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                }
            }
        } else [[unlikely]] {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.;
        }
    } else { // create tuples
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Trying to CREATE tuple flip pair.";
#endif
        double tau_left, tau_right;
        int random_tuple_flip_index = 0;

        if (tuple_flip_count > 0) [[likely]] {
            random_tuple_flip_index = random_index(tuple_flip_count + 1);

            if (random_tuple_flip_index == 0) {
                tau_left = 0.; //not really necessary
                tau_right = tuple_flips[0];
            } else if (random_tuple_flip_index == tuple_flip_count) {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = beta;
            } else {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = tuple_flips[random_tuple_flip_index];
            }
        } else [[unlikely]] {
            tau_left = 0.;
            tau_right = beta;
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif

        double tau_1 = 0., tau_2 = 0.;

        tau_1 = uniform_real(tau_left, tau_right);
        tau_2 = uniform_real(tau_left, tau_right);
        if (tau_2 < tau_1) {
            std::swap(tau_1, tau_2);
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- tau_1: {}; tau_2: {}", tau_1, tau_2);
#endif

        if (std::abs(tau_2 - tau_1) < PRECISION 
        || std::abs(tau_1 - tau_left) < PRECISION 
        || std::abs(tau_2 - tau_right) < PRECISION
        ) [[unlikely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Random numbers very close to each other.";
#endif
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        } else {
            const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
            = integrated_pot_energy_diff_tuple_flip_edge(
                lat, h, mu, J, lmbda, random_tuple, tuple_edges, tau_1, tau_2, false
            );

            if constexpr (Basis == 'x') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * J * J * std::exp(- integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * mu * mu * std::exp(- integrated_pot_energy_diff);
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- acceptance ratio: {}", acc_ratio);
#endif 

            if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- ACCEPTED.";
#endif  
                lat.insert_double_tuple_flip(random_tuple, tuple_edges, tau_1, tau_2);
                integrated_pot_energy += integrated_pot_energy_diff;
                for (size_t i = 0; i < tuple_edges.size(); ++i) {
                    lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                }
            } 
        }
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_single_tuple_flip_move(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_single_tuple_flip_move!";
#endif

    if (Basis == 'x' && (J == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    int random_tuple = -1;
    std::span<const typename LatticeT::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) [[unlikely]] return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose star with center {}", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Found {} tuple flips.", tuple_flip_count);
#endif  

    if (tuple_flip_count > 0) [[likely]] {
        const int random_tuple_flip_index = random_index(tuple_flip_count);

        const double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];

        const auto [tau_left, tau_right] = lat.tuple_flip_window(tuple_edges, imag_time_tuple_flip);

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("ropolis_step_single_tuple_flip_move --- Imaginary time of random tuple flip: {}; tau_left: {}; tau_right: {}", imag_time_tuple_flip, tau_left, tau_right);
#endif  

        if (tau_left < imag_time_tuple_flip && imag_time_tuple_flip < tau_right) { // not cross beta
            const double new_imag_time = uniform_real(tau_left, tau_right);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- New imaginary time: {}", new_imag_time);
#endif 

            if (std::abs(new_imag_time - imag_time_tuple_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                std::span<const typename LatticeT::Edge> pot_energy_edges;
                SmallEnergyVector pot_energy_diffs;
                if (new_imag_time > imag_time_tuple_flip) {
                    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false, true
                    );
                    pot_energy_edges = pot_energy_edges_tmp;
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge;
                } else {
                    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false, true
                    );
                    pot_energy_edges = pot_energy_edges_tmp;
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge;
                }
                acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                    lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                    integrated_pot_energy += integrated_pot_energy_diff;
                    for (size_t i = 0; i < tuple_edges.size(); ++i) {
                        lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                    }
                } 

            }
        } else { // Potentially cross beta
            const double new_imag_time = modulo(uniform_real(tau_left, beta + tau_right), beta);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- New imaginary time: {}", new_imag_time);
#endif 

            //double integrated_pot_energy_diff = 0.;

            if (std::abs(new_imag_time - imag_time_tuple_flip) < PRECISION 
            || std::abs(new_imag_time - tau_left) < PRECISION 
            || std::abs(new_imag_time - tau_right) < PRECISION
            ) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else if (tau_left > imag_time_tuple_flip) { // Potentially over beta left
                if (new_imag_time < imag_time_tuple_flip) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false, true
                    );
                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time > imag_time_tuple_flip && new_imag_time < tau_right) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false, true
                    );
                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time > imag_time_tuple_flip && new_imag_time > tau_left) { // Move over beta
                    auto [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, beta, false, true
                    );
                    if (imag_time_tuple_flip != 0) {
                        const auto& [integrated_pot_energy_diff_2, pot_energy_edges_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_tuple_flip_edge(
                            lat, h, mu, J, lmbda, random_tuple, tuple_edges,
                             0., imag_time_tuple_flip, false, true
                        );
                        for (size_t i = 0; i < pot_energy_edges.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff += integrated_pot_energy_diff_2;
                    }

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED. Flipping spins.";
#endif  
                        for (const auto& p_edg : tuple_edges) {
                            lat.flip_spin(p_edg);
                        }
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    }
                }
            } else if (tau_right < imag_time_tuple_flip) { // Potentially over beta right
                if (new_imag_time > imag_time_tuple_flip) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false, true
                    );

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time < imag_time_tuple_flip && new_imag_time > tau_left) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false, true
                    );

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time < imag_time_tuple_flip && new_imag_time < tau_right) { // Move over beta
                    auto [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, beta, false, true
                    );
                    if (new_imag_time > 0) {
                        const auto& [integrated_pot_energy_diff_2, pot_energy_edges_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_tuple_flip_edge(
                            lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                            0., new_imag_time, false, true
                        );
                        for (size_t i = 0; i < pot_energy_edges.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff += integrated_pot_energy_diff_2;
                    }

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED. Flipping spins.";
#endif 
                        for (const auto& p_edg : tuple_edges) {
                            lat.flip_spin(p_edg);
                        }
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                }
            }
        }
    }
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step_spin_tuple_combination(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_spin_tuple_combination!";
#endif

    if (Basis == 'x' && (J == 0 || lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0 || h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }
    
    double rnd_create_destroy = uniform_dist(*rng);
    
    int random_tuple = -1;
    std::span<const typename LatticeT::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose star with center {}", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Found {} tuple flips.", tuple_flip_count);
#endif  

    bool tuple_destroy = true;
    int random_tuple_flip_index = -1;
    double tau_new = -1., r = 0., tau_left = 0., tau_right = 0.;

    if (rnd_create_destroy < 0.5) { // Destroys tuple
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Trying to DESTROY tuple flip.";
#endif
        tuple_destroy = true;
        if (tuple_flip_count > 0) [[likely]] {
            random_tuple_flip_index = random_index(tuple_flip_count);

            double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];
            tau_new = imag_time_tuple_flip;

            double imag_time_next_tuple_flip = beta;

            if (random_tuple_flip_index < tuple_flip_count-1) {
                imag_time_next_tuple_flip = tuple_flips[(random_tuple_flip_index+1)];
            }

            tau_right = imag_time_next_tuple_flip;

            double imag_time_prev_tuple_flip = 0.;

            if (random_tuple_flip_index != 0) {
                imag_time_prev_tuple_flip = tuple_flips[(random_tuple_flip_index-1)];
            }
            tau_left = imag_time_prev_tuple_flip;

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Imaginary time of random tuple flip: {}; tau_left: {}; tau_right: {}", imag_time_tuple_flip, tau_left, tau_right);
#endif  
            if constexpr (Basis == 'x') {
                r = 1. / ((imag_time_next_tuple_flip-imag_time_prev_tuple_flip) * J);
            } else if constexpr (Basis == 'z') {
                r = 1. / ((imag_time_next_tuple_flip-imag_time_prev_tuple_flip) * mu);
            }
        } else [[unlikely]] {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        }
    } else { // Create tuple
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Trying to CREATE tuple flip.";
#endif
        tuple_destroy = false;

        random_tuple_flip_index = 0;

        if (tuple_flip_count > 0) [[likely]] {
            random_tuple_flip_index = random_index(tuple_flip_count + 1);

            if (random_tuple_flip_index == 0) {
                tau_left = 0.; // not really necessary
                tau_right = tuple_flips[0];
            } else if (random_tuple_flip_index == tuple_flip_count) {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = beta;
            } else {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = tuple_flips[random_tuple_flip_index];
            }
        } else [[unlikely]] {
            tau_left = 0.;
            tau_right = beta;
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif  

        if (tau_left < tau_right) {
            tau_new = uniform_real(tau_left, tau_right);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Proposed tuple flip imaginary time: {}", tau_new);
#endif 

#ifndef NDEBUG
            if (lat.check_spin_flips_present_tuple(tuple_edges, tau_new)) {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }
#endif

            if (std::abs(tau_new - tau_left) < PRECISION || std::abs(tau_new - tau_right) < PRECISION) [[unlikely]] {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }
            
            if constexpr (Basis == 'x') {
                r = (tau_right-tau_left) * J;
            } else if constexpr (Basis == 'z') {
                r = (tau_right-tau_left) * mu;
            }
        } else {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        }
    }

    SmallEnergyVector r_b;
    SmallBoolVector create_vector;
    SmallEnergyVector flip_times;
    r_b.reserve(tuple_edges.size());
    create_vector.reserve(tuple_edges.size());
    flip_times.reserve(tuple_edges.size());

    // These variables determine the range in which the potential energy can change (potentially :D)
    double tau_right_potential_energy = tau_right;
    double tau_left_potential_energy = tau_left;

    for (size_t i = 0; i < tuple_edges.size(); ++i) {
        rnd_create_destroy = uniform_dist(*rng);
        int tau_spin_flip_index = -1;
        double tau_spin_flip = -1.;
        
        const typename LatticeT::Edge edg = tuple_edges[i];
        
        std::span<const double> single_spin_flips = lat.get_single_spin_flips(edg);
        int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
        int spin = lat.get_spin(edg);
        const auto& [source_v, target_v] = lat.vertices_of_edge(edg);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Spin: {}. Found {} single spin flips.", source_v, target_v, spin, single_spin_flip_count);
#endif  

        if (rnd_create_destroy < 0.5) { // destroy
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Trying to DESTROY single spin flip.", source_v, target_v);
#endif  

            if (single_spin_flip_count == 0) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

            const auto next_it = std::upper_bound(single_spin_flips.begin(), single_spin_flips.end(), tau_new);
            const int imag_time_next_flip_index = static_cast<int>(next_it - single_spin_flips.begin());
            const double imag_time_next_flip = next_it != single_spin_flips.end() ? *next_it : beta;
            const int imag_time_prev_flip_index = next_it != single_spin_flips.begin()
                ? static_cast<int>((next_it - single_spin_flips.begin()) - 1)
                : -1;
            const double imag_time_prev_flip = imag_time_prev_flip_index >= 0
                ? single_spin_flips[static_cast<size_t>(imag_time_prev_flip_index)]
                : 0.;

            create_vector.emplace_back(false);
            rnd_create_destroy = uniform_dist(*rng);

            if (rnd_create_destroy < 0.5) {
                tau_spin_flip = imag_time_prev_flip;
                tau_spin_flip_index = imag_time_prev_flip_index;
            } else {
                tau_spin_flip = imag_time_next_flip;
                tau_spin_flip_index = imag_time_next_flip_index;
            }

            if (tau_spin_flip_index == -1 || tau_spin_flip_index == single_spin_flip_count) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }
            
            double tau_prev_of_chosen = 0.;
            if (tau_spin_flip_index > 0) {
                tau_prev_of_chosen = single_spin_flips[tau_spin_flip_index-1];
            }

            double tau_next_of_chosen = beta;
            if (tau_spin_flip_index < single_spin_flip_count-1) {
                tau_next_of_chosen = single_spin_flips[tau_spin_flip_index+1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Propose to remove single spin flip at {}", source_v, target_v, tau_spin_flip);            
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - previous: {} - next: {}", source_v, target_v, tau_prev_of_chosen, tau_next_of_chosen);
#endif 

            if constexpr (Basis == 'x') {
                r_b.emplace_back(2./((tau_next_of_chosen - tau_prev_of_chosen) * lmbda));
            } else if constexpr (Basis == 'z') {
                r_b.emplace_back(2./((tau_next_of_chosen - tau_prev_of_chosen) * h));
            }

            flip_times.emplace_back(tau_spin_flip);

            if (tau_next_of_chosen > tau_right_potential_energy) {
                tau_right_potential_energy = tau_next_of_chosen;
            }

            if (tau_prev_of_chosen < tau_left_potential_energy) {
                tau_left_potential_energy = tau_prev_of_chosen;
            }
        } else { // create
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Trying to CREATE single spin flip.", source_v, target_v);
#endif  
            
            const auto next_it = std::upper_bound(single_spin_flips.begin(), single_spin_flips.end(), tau_new);
            const double imag_time_prev_flip = next_it != single_spin_flips.begin()
                ? *std::prev(next_it)
                : 0.;
            const double imag_time_next_flip = next_it != single_spin_flips.end() ? *next_it : beta;

            create_vector.emplace_back(true);

            tau_spin_flip = uniform_real(imag_time_prev_flip, imag_time_next_flip);

            if (std::abs(tau_spin_flip - imag_time_prev_flip) < PRECISION 
            || std::abs(tau_spin_flip - imag_time_next_flip) < PRECISION
            ) [[unlikely]] {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

            if (std::abs(tau_spin_flip - tau_new) < PRECISION) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Propose to create single spin flip at {}", source_v, target_v, tau_spin_flip);
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - previous: {} - next: {}", source_v, target_v, imag_time_prev_flip, imag_time_next_flip);
#endif 
            
            if constexpr (Basis == 'x') {
                r_b.emplace_back((imag_time_next_flip - imag_time_prev_flip) * lmbda / 2.); 
            } else if constexpr (Basis == 'z') {
                r_b.emplace_back((imag_time_next_flip - imag_time_prev_flip) * h / 2.); 
            }

            flip_times.emplace_back(tau_spin_flip);

            if (imag_time_next_flip > tau_right_potential_energy) {
                tau_right_potential_energy = imag_time_next_flip;
            }

            if (imag_time_prev_flip < tau_left_potential_energy) {
                tau_left_potential_energy = imag_time_prev_flip;
            }     
        }
    }

    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges, pot_energy_edges_diffs] 
    = integrated_pot_energy_diff_combination_flip_edge(
        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
        tau_new, flip_times, tau_left_potential_energy, tau_right_potential_energy, 
        create_vector, tuple_destroy
    );
    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_tuple_diffs] 
    = integrated_pot_energy_diff_combination_flip_tuple(
        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
        tau_new, flip_times, tau_left_potential_energy, tau_right_potential_energy, 
        create_vector, tuple_destroy
    );
    const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff_edge: {}", integrated_pot_energy_diff_edge);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff_tuple: {}", integrated_pot_energy_diff_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff: {}", integrated_pot_energy_diff);
#endif 

    acc_ratio = std::exp(- integrated_pot_energy_diff) * r * std::accumulate(r_b.begin(), r_b.end(), 1., std::multiplies<double>());

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- acceptance ratio: {}", acc_ratio);
#endif  

    if (accept(acc_ratio)) {
        record_update_acceptance();
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- ACCEPTED.";
#endif 
        combination_flip(lat, h, mu, random_tuple, tuple_edges, tau_new, flip_times, create_vector, tuple_destroy); 
        integrated_pot_energy += integrated_pot_energy_diff;
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            lat.add_potential_edge_energy(pot_energy_edges[i], pot_energy_edges_diffs[i]);
        }
        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
            int tuple_index = pot_energy_tuple_indices[i];
            double potential_tuple_energy_diff = pot_energy_tuple_diffs[i];
            if constexpr (Basis == 'x') {
                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
            } else if constexpr (Basis == 'z') {
                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
            }
        } 
    } 
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis, LatticeT>::metropolis_step(
    LatticeT& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {
    const int rnd = random_index(7);
    active_update_type_ = rnd;
    acc_ratio = 0.;
    if (rnd < 1) {
        metropolis_step_double_single_spin_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 2) {
        metropolis_step_single_spin_flip_move(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 3) {
        metropolis_step_global_single_spin_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 4) {
        metropolis_step_global_tuple_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 5) {
        metropolis_step_double_tuple_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 6) {
        metropolis_step_single_tuple_flip_move(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else {
        metropolis_step_spin_tuple_combination(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    }  
    record_update_attempt(rnd, acc_ratio);
    active_update_type_ = -1;

#ifndef NDEBUG
    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, h, mu, J, lmbda
    );

    BOOST_LOG_TRIVIAL(debug) << std::format("Integrated potential energy --- Cached: {}, Actual: {}", integrated_pot_energy, integrated_pot_energy_check);
#endif  
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis, LatticeT>::calculate_autocorrelation_time_with_warning(
    const std::vector<double>& obs_real,
    const std::string& observable_name,
    bool has_hysteresis_context,
    size_t hysteresis_point,
    double h,
    double lmbda
) {
    const double autocorrelation_time = paratoric::statistics::get_autocorrelation_time(
        paratoric::statistics::get_autocorrelation_function(obs_real)
    );
    const size_t sample_count = obs_real.size();
    const double warning_threshold =
        AUTOCORRELATION_WARNING_SAMPLE_FRACTION * static_cast<double>(sample_count);

    if (sample_count > 0 && autocorrelation_time > warning_threshold) {
        const std::string hysteresis_context = has_hysteresis_context
            ? std::format(" at hysteresis point {} (h={}, lmbda={})", hysteresis_point, h, lmbda)
            : "";

        BOOST_LOG_TRIVIAL(warning)
            << "Autocorrelation time for observable \""
            << observable_name << "\"" << hysteresis_context
            << " is " << autocorrelation_time
            << ", greater than " << AUTOCORRELATION_WARNING_SAMPLE_FRACTION
            << " * number of samples (" << warning_threshold
            << " for " << sample_count << " samples).";
    }

    return autocorrelation_time;
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis, LatticeT>::get_thermalization(
    const Config& config
) {

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }
    
    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (config.param_spec.lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (config.param_spec.h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    const auto thermalization_count = static_cast<size_t>(std::max(config.sim_spec.N_thermalization, std::int64_t{0}));
    std::vector<double> acc_ratio_vector;
    acc_ratio_vector.reserve(thermalization_count);
    // Vector to store observable results for all snapshots
    std::vector<std::vector<std::variant< std::complex<double>, double>>> observable_vector;
    observable_vector.reserve(config.sim_spec.observables.size());
    for (const auto& obs_func : config.sim_spec.observables) {
        UNUSED(obs_func);
        observable_vector.emplace_back();
        observable_vector.back().reserve(thermalization_count);
    } 

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);

    // Initialize Lattice
    auto lat = LatticeT(config.lat_spec, rng);
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    double acc_ratio = 1.;

    std::int64_t total_metropolis_step_count = 0;
    const std::int64_t reset_potential_energy_count =
        static_cast<std::int64_t>(lat.get_edge_count()) * 10000;

    for (std::int64_t i = 0; i < config.sim_spec.N_thermalization; ++i) {
        ++total_metropolis_step_count;
        metropolis_step(
            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
            config.param_spec.h, config.param_spec.mu, 
            config.param_spec.J, config.param_spec.lmbda
        );

        if (reset_potential_energy_count > 0 && total_metropolis_step_count % reset_potential_energy_count == 0) [[unlikely]] {
            // avoid accumulation of small numerical errors leading to bias
            lat.init_potential_energy();
            integrated_pot_energy = total_integrated_pot_energy(
                lat, config.param_spec.h, config.param_spec.mu, 
                config.param_spec.J, config.param_spec.lmbda
            );
        }

        lat.rotate_imag_time();
        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            observable_vector[k].emplace_back(
                obs_func_vec[k](lat, config.param_spec.h, config.param_spec.lmbda, config.param_spec.mu, config.param_spec.J)
            );
        }
        acc_ratio_vector.emplace_back(acc_ratio);

        if (config.out_spec.save_snapshots && i%10000 == 0) [[unlikely]] {
            lat.update_spin_string();
        }
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    if (config.out_spec.save_snapshots) {
        lat.write_graph("snapshots", config.out_spec.path_out);
    }

    return Result{
        .series=std::move(observable_vector), 
        .acc_ratio=std::move(acc_ratio_vector)
    };                                     
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis, LatticeT>::get_sample(
    const Config& config
) { 

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }

    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (config.param_spec.lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (config.param_spec.h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    bool pt_enabled = config.pt_spec.enabled || !config.pt_spec.ladder_s.empty();
    int pt_world_size = 1;
    int pt_rank = 0;
#ifdef PARATORIC_HAS_MPI
    mpi_detail::MpiFinalizeGuard mpi_finalize_guard{};
    if (pt_enabled) {
        int mpi_initialized = 0;
        mpi_detail::fail_on_mpi_error(MPI_Initialized(&mpi_initialized), "MPI_Initialized");
        if (!mpi_initialized) {
            mpi_detail::fail_on_mpi_error(MPI_Init(nullptr, nullptr), "MPI_Init");
            mpi_finalize_guard.active = true;
        } else {
            mpi_finalize_guard.active = false;
        }
        mpi_detail::fail_on_mpi_error(MPI_Comm_size(MPI_COMM_WORLD, &pt_world_size), "MPI_Comm_size");
        mpi_detail::fail_on_mpi_error(MPI_Comm_rank(MPI_COMM_WORLD, &pt_rank), "MPI_Comm_rank");
    }
#else
    if (pt_enabled) {
        throw std::invalid_argument("Parallel tempering requires an MPI-enabled build.");
    }
#endif
    if (pt_enabled) {
        if constexpr (!std::same_as<LatticeT, Lattice> || Basis != 'z') {
            throw std::invalid_argument("Parallel tempering requires basis=z and lattice_backend=boost.");
        }
        if (config.sim_spec.custom_therm) {
            throw std::invalid_argument("Parallel tempering does not support custom_therm.");
        }
        // M9: checkpoint/restart is now implemented for PT (pt_ladder_index and the
        // four runtime couplings persist, checkpoint format version 4). The blanket
        // throw that used to sit here is gone.
        if (config.param_spec.mu != 1.0 || config.param_spec.J != 1.0) {
            throw std::invalid_argument("Parallel tempering ladder requires mu=J=1.");
        }
        if (config.pt_spec.ladder_s.size() != static_cast<size_t>(pt_world_size)
            || (!config.pt_spec.ladder_lmbda.empty()
                && config.pt_spec.ladder_lmbda.size() != static_cast<size_t>(pt_world_size))) {
            throw std::invalid_argument("PT h and optional lambda ladder sizes must equal the MPI world size.");
        }
        if ((!config.pt_spec.ladder_mu.empty()
                && config.pt_spec.ladder_mu.size() != static_cast<size_t>(pt_world_size))
            || (!config.pt_spec.ladder_J.empty()
                && config.pt_spec.ladder_J.size() != static_cast<size_t>(pt_world_size))) {
            throw std::invalid_argument("PT optional mu/J ladder sizes must equal the MPI world size.");
        }
    }
    const std::int64_t pt_swap_period_moves =
        pt_enabled ? parallel_tempering_swap_period_moves(config.pt_spec, config.lat_spec) : 1;

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);
    auto obs_type_vec = get_obs_type_vec(config.sim_spec.observables);
    
    // Vector to store observable results for all snapshots
    const auto sample_count = static_cast<size_t>(std::max(config.sim_spec.N_samples, 0));
    std::vector<std::vector< std::variant< std::complex<double>, double> >> observable_vector;
    observable_vector.reserve(config.sim_spec.observables.size());
    std::vector<double> observable_mean_vector(config.sim_spec.observables.size(), 0.), 
                        observable_std_vector(config.sim_spec.observables.size(), 0.), 
                        binder_mean_vector(config.sim_spec.observables.size(), 0.), 
                        binder_std_vector(config.sim_spec.observables.size(), 0.), 
                        observable_autocorrelation_time_vector(config.sim_spec.observables.size(), 0.);
    for (const auto& obs_func : config.sim_spec.observables) {
        UNUSED(obs_func);
        observable_vector.emplace_back();
        observable_vector.back().reserve(sample_count);
    } 

    if ((config.out_spec.checkpoint_every_samples > 0 || config.out_spec.checkpoint_every_seconds > 0)
        && config.out_spec.checkpoint_path.empty()) {
        throw std::invalid_argument("Checkpointing requires a non-empty checkpoint_path.");
    }

    // Initialize Lattice
    auto lat = LatticeT(config.lat_spec, rng);

    int pt_ladder_index = pt_enabled ? pt_rank : 0;
    double runtime_h = pt_enabled ? config.pt_spec.ladder_s[static_cast<size_t>(pt_ladder_index)] : config.param_spec.h;
    double runtime_lmbda = pt_enabled
        ? (config.pt_spec.ladder_lmbda.empty()
            ? runtime_h : config.pt_spec.ladder_lmbda[static_cast<size_t>(pt_ladder_index)])
        : config.param_spec.lmbda;
    // Common-factor tempering: falls back to config.param_spec.mu/.J (unchanged) when
    // ladder_mu/ladder_J are empty, so untempered runs are bit-for-bit unaffected.
    double runtime_mu = (pt_enabled && !config.pt_spec.ladder_mu.empty())
        ? config.pt_spec.ladder_mu[static_cast<size_t>(pt_ladder_index)]
        : config.param_spec.mu;
    double runtime_J = (pt_enabled && !config.pt_spec.ladder_J.empty())
        ? config.pt_spec.ladder_J[static_cast<size_t>(pt_ladder_index)]
        : config.param_spec.J;
    ParallelTemperingDiagnostics pt_diagnostics{
        .enabled = pt_enabled,
#ifdef PARATORIC_HAS_MPI
        .mpi_compiled = true,
#else
        .mpi_compiled = false,
#endif
        .world_size = pt_world_size,
        .rank = pt_rank,
        .initial_ladder_index = pt_ladder_index,
        .final_ladder_index = pt_ladder_index,
        .initial_s = runtime_h,
        .final_s = runtime_h,
        .initial_mu = runtime_mu,
        .final_mu = runtime_mu,
        .initial_J = runtime_J,
        .final_J = runtime_J,
        .initial_lmbda = runtime_lmbda,
        .final_lmbda = runtime_lmbda
    };
    if (pt_enabled) {
        pt_diagnostics.swap_attempted_by_link.assign(static_cast<size_t>(pt_world_size - 1), 0);
        pt_diagnostics.swap_accepted_by_link.assign(static_cast<size_t>(pt_world_size - 1), 0);
        // M10: same length as the two counters above, one bin per ladder link.
        pt_diagnostics.R_count_by_link.assign(static_cast<size_t>(pt_world_size - 1), 0);
        pt_diagnostics.R_sum_by_link.assign(static_cast<size_t>(pt_world_size - 1), 0.);
        pt_diagnostics.R_sumsq_by_link.assign(static_cast<size_t>(pt_world_size - 1), 0.);
    }
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda
    );
    double acc_ratio = 1.;
    std::int64_t total_metropolis_step_count = 0;
    AcceptanceDiagnostics production_acceptance;
    std::int64_t completed_samples = 0;

    if (!config.out_spec.restart_from.empty()) {
        auto checkpoint_state = read_sample_checkpoint(config.out_spec.restart_from, config, lat);
        if (pt_enabled) {
#ifdef PARATORIC_HAS_MPI
            // A PT checkpoint is a set of rank-local files, so validate the set
            // before restoring any state or entering the production loop. Version
            // 2/3 files use completed_samples as an implicit legacy marker; v4
            // stores the same value explicitly.
            std::vector<std::int64_t> checkpoint_markers(static_cast<size_t>(pt_world_size));
            std::vector<std::int64_t> checkpoint_samples(static_cast<size_t>(pt_world_size));
            mpi_detail::fail_on_mpi_error(
                MPI_Allgather(&checkpoint_state.global_checkpoint_marker, 1, MPI_INT64_T,
                              checkpoint_markers.data(), 1, MPI_INT64_T, MPI_COMM_WORLD),
                "MPI_Allgather(checkpoint marker)"
            );
            mpi_detail::fail_on_mpi_error(
                MPI_Allgather(&checkpoint_state.completed_samples, 1, MPI_INT64_T,
                              checkpoint_samples.data(), 1, MPI_INT64_T, MPI_COMM_WORLD),
                "MPI_Allgather(checkpoint completed samples)"
            );
            for (int rank = 0; rank < pt_world_size; ++rank) {
                if (checkpoint_markers[static_cast<size_t>(rank)] != checkpoint_markers.front()
                    || checkpoint_samples[static_cast<size_t>(rank)] != checkpoint_samples.front()
                    || checkpoint_markers[static_cast<size_t>(rank)]
                        != checkpoint_samples[static_cast<size_t>(rank)]) {
                    throw std::runtime_error(
                        "PT checkpoint global marker mismatch across rank files; refusing restart."
                    );
                }
            }

            const int restored_ladder_index = checkpoint_state.has_pt_state
                ? checkpoint_state.pt_ladder_index : pt_rank;
            std::vector<int> restored_ladder_indices(static_cast<size_t>(pt_world_size));
            mpi_detail::fail_on_mpi_error(
                MPI_Allgather(&restored_ladder_index, 1, MPI_INT,
                              restored_ladder_indices.data(), 1, MPI_INT, MPI_COMM_WORLD),
                "MPI_Allgather(checkpoint ladder index)"
            );
            // This is intentionally at LOAD time, before any production sampling.
            (void)parallel_tempering_rank_of_ladder_index(restored_ladder_indices);
#else
            throw std::invalid_argument("Parallel tempering restart requires an MPI-enabled build.");
#endif
        }
        completed_samples = checkpoint_state.completed_samples;
        total_metropolis_step_count = checkpoint_state.production_metropolis_steps;
        integrated_pot_energy = checkpoint_state.integrated_pot_energy;
        acc_ratio = checkpoint_state.acc_ratio;
        production_acceptance = std::move(checkpoint_state.production_acceptance);
        observable_vector = std::move(checkpoint_state.observable_vector);
        if (observable_vector.size() != config.sim_spec.observables.size()) {
            throw std::runtime_error("Checkpoint observable count does not match this run.");
        }
        // M9: restore the ladder position and post-swap runtime couplings a PT run
        // left off at. A version-2 checkpoint has none of this (has_pt_state stays
        // false); pt_ladder_index/runtime_h/mu/J/lmbda then keep the values already
        // computed above (pt_ladder_index = pt_rank, runtime couplings = launch
        // values) -- exactly today's pre-M9 behaviour, deliberately unchanged for old
        // files.
        if (checkpoint_state.has_pt_state) {
            pt_ladder_index = checkpoint_state.pt_ladder_index;
            runtime_h = checkpoint_state.runtime_h;
            runtime_mu = checkpoint_state.runtime_mu;
            runtime_J = checkpoint_state.runtime_J;
            runtime_lmbda = checkpoint_state.runtime_lmbda;
        }
        if (checkpoint_state.has_pt_diagnostics) {
            const auto expected_links = static_cast<size_t>(std::max(pt_world_size - 1, 0));
            const auto& loaded = checkpoint_state.pt_diagnostics;
            if (pt_enabled && (
                    loaded.swap_attempted_by_link.size() != expected_links
                    || loaded.swap_accepted_by_link.size() != expected_links
                    || loaded.R_count_by_link.size() != expected_links
                    || loaded.R_sum_by_link.size() != expected_links
                    || loaded.R_sumsq_by_link.size() != expected_links
                    || loaded.ladder_index_by_sample.size() != static_cast<size_t>(completed_samples))) {
                throw std::runtime_error("PT checkpoint diagnostic shapes do not match the MPI world or sample count.");
            }
            pt_diagnostics = checkpoint_state.pt_diagnostics;
            pt_diagnostics.enabled = pt_enabled;
            pt_diagnostics.mpi_compiled = true;
            pt_diagnostics.world_size = pt_world_size;
            pt_diagnostics.rank = pt_rank;
        }
        pt_diagnostics.initial_ladder_index = pt_ladder_index;
        pt_diagnostics.initial_s = runtime_h;
        pt_diagnostics.initial_mu = runtime_mu;
        pt_diagnostics.initial_J = runtime_J;
        pt_diagnostics.initial_lmbda = runtime_lmbda;
    } else {
        if (config.sim_spec.custom_therm) {
            // Pre-Thermalization 
            for (std::int64_t i = 0; i < config.sim_spec.N_thermalization; ++i) {
                metropolis_step(
                    lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                    config.param_spec.h_therm, config.param_spec.mu, config.param_spec.J, 
                    config.param_spec.lmbda_therm
                );
            }

            // For sampling first order hysteresis
            double h_end = config.param_spec.h_therm;
            if (std::abs(config.param_spec.h - config.param_spec.h_therm) > PRECISION) {
                for (int i = 9; i > -1; --i) {
                    h_end = config.param_spec.h + (config.param_spec.h_therm-config.param_spec.h) * i / 10.;
                    lat.init_potential_energy();
                    lat.rotate_imag_time();
                    integrated_pot_energy = total_integrated_pot_energy(
                        lat, h_end, config.param_spec.mu, 
                        config.param_spec.J, config.param_spec.lmbda_therm
                    );
                    for (std::int64_t j = 0; j < config.sim_spec.N_thermalization / 10.; ++j) {
                        metropolis_step(
                            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h_end, 
                            config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda_therm
                        );
                    }
                }
            }
            double lmbda_end = config.param_spec.lmbda_therm;
            if (std::abs(config.param_spec.lmbda - config.param_spec.lmbda_therm) > PRECISION) {
                for (int i = 9; i > -1; --i) {
                    lmbda_end = config.param_spec.lmbda + (config.param_spec.lmbda_therm-config.param_spec.lmbda) * i / 10.;
                    lat.init_potential_energy();
                    lat.rotate_imag_time();
                    integrated_pot_energy = total_integrated_pot_energy(
                        lat, h_end, config.param_spec.mu, 
                        config.param_spec.J, lmbda_end
                    );
                    for (std::int64_t j = 0; j < config.sim_spec.N_thermalization / 10.; ++j) {
                        metropolis_step(
                            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h_end, 
                            config.param_spec.mu, config.param_spec.J, lmbda_end
                        );
                    }
                }
            }
        } else {
            // Thermalization 
            for (std::int64_t i = 0; i < config.sim_spec.N_thermalization; ++i) {
                metropolis_step(
                    lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                    runtime_h, runtime_mu, runtime_J, runtime_lmbda
                );
                if constexpr (std::same_as<LatticeT, Lattice>) {
#ifdef PARATORIC_HAS_MPI
                if (pt_enabled && ((i + 1) % pt_swap_period_moves) == 0) {
                    const int phase = static_cast<int>(((i + 1) / pt_swap_period_moves) & 1);
                    std::vector<int> ladder_index_by_rank(static_cast<size_t>(pt_world_size), -1);
                    mpi_detail::fail_on_mpi_error(
                        MPI_Allgather(&pt_ladder_index, 1, MPI_INT,
                                      ladder_index_by_rank.data(), 1, MPI_INT,
                                      MPI_COMM_WORLD),
                        "MPI_Allgather(pt_ladder_index)"
                    );
                    const auto partner_rank = parallel_tempering_partner_rank(
                        pt_rank, ladder_index_by_rank, phase);
                    if (partner_rank) {
                        const int partner = *partner_rank;
                        ++pt_diagnostics.swap_attempted;
                        double partner_h = 0.;
                        double partner_lmbda = 0.;
                        double partner_mu = 0.;
                        double partner_J = 0.;
                        int partner_index = -1;
                        const double local_h = runtime_h;
                        const double local_lmbda = runtime_lmbda;
                        const double local_mu = runtime_mu;
                        const double local_J = runtime_J;
                        const int local_index = pt_ladder_index;
                        // M10 power test for the routing guard: send a ladder index that is not
                        // the one this rank holds, so the partner's guard has something to catch.
                        const int sent_index = config.pt_spec.break_routing_for_test
                            ? local_index + 1 : local_index;
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_h, 1, MPI_DOUBLE, partner, 7101,
                                         &partner_h, 1, MPI_DOUBLE, partner, 7101,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_h)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_lmbda, 1, MPI_DOUBLE, partner, 7105,
                                         &partner_lmbda, 1, MPI_DOUBLE, partner, 7105,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_lmbda)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_mu, 1, MPI_DOUBLE, partner, 7109,
                                         &partner_mu, 1, MPI_DOUBLE, partner, 7109,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_mu)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_J, 1, MPI_DOUBLE, partner, 7110,
                                         &partner_J, 1, MPI_DOUBLE, partner, 7110,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_J)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&sent_index, 1, MPI_INT, partner, 7102,
                                         &partner_index, 1, MPI_INT, partner, 7102,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_index)"
                        );
                        const int link_index = std::min(local_index, partner_index);
                        if (link_index >= 0 && link_index < pt_world_size - 1) {
                            ++pt_diagnostics.swap_attempted_by_link[static_cast<size_t>(link_index)];
                        }
                        // M10 routing guard (Feynman). Always on: four integer comparisons per
                        // attempt. The Allgather above told this rank which ladder index EVERY
                        // rank currently holds. If the payload that arrives from `partner` does
                        // not carry that rank's ladder index, the data came from somewhere other
                        // than the intended partner, and every number in the weight below belongs
                        // to the wrong rung. The M4 reverse-move balance check cannot see this,
                        // because it re-reads the same received values in both directions. A
                        // two-rung ladder cannot exercise it at all: with one link there is only
                        // one possible partner, so any routing is trivially correct. This is the
                        // check that has content once the ladder is wider than two rungs.
                        if (partner_index != ladder_index_by_rank[static_cast<size_t>(partner)]
                            || (local_index - partner_index != 1 && partner_index - local_index != 1)) {
                            throw std::runtime_error(
                                "PT swap routing check failed: rank " + std::to_string(pt_rank)
                                + " (ladder index " + std::to_string(local_index)
                                + ") received ladder index " + std::to_string(partner_index)
                                + " from rank " + std::to_string(partner)
                                + ", but the Allgather reported ladder index "
                                + std::to_string(ladder_index_by_rank[static_cast<size_t>(partner)])
                                + " for that rank.");
                        }
                        const double local_single = lat.get_non_diag_single_energy_z();
                        const double local_edge = lat.cached_total_integrated_edge_energy();
                        const double local_tuple = lat.get_non_diag_tuple_energy_z();
                        const double local_plaquette = lat.cached_total_integrated_plaquette_energy();
                        double partner_single = 0.;
                        double partner_edge = 0.;
                        double partner_tuple = 0.;
                        double partner_plaquette = 0.;
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_single, 1, MPI_DOUBLE, partner, 7103,
                                         &partner_single, 1, MPI_DOUBLE, partner, 7103,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_single)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_edge, 1, MPI_DOUBLE, partner, 7104,
                                         &partner_edge, 1, MPI_DOUBLE, partner, 7104,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_edge)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_tuple, 1, MPI_DOUBLE, partner, 7111,
                                         &partner_tuple, 1, MPI_DOUBLE, partner, 7111,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_tuple)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&local_plaquette, 1, MPI_DOUBLE, partner, 7112,
                                         &partner_plaquette, 1, MPI_DOUBLE, partner, 7112,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(local_plaquette)"
                        );
                        // M6 follow-up (Feynman): the shared function guards h and mu
                        // positivity before taking their log (kink-count exchange
                        // weights); neither inline block had that guard. A hot rung
                        // with a small or zero coupling sends this log to -inf or NaN
                        // silently -- nothing throws, the swap just never accepts, or
                        // accepts on a NaN comparison. Matches the shared function's
                        // guard exactly; lmbda and J take no log, so they don't need
                        // it (mirrors the shared function, which doesn't guard them
                        // either).
                        if (local_h <= 0. || partner_h <= 0.) {
                            throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
                        }
                        // M10 power test (Feynman). Exchanges the roles of the RECEIVED
                        // partner_edge and partner_plaquette values. Those two are the exact pair
                        // the BLIND SPOT comment below names: both are configuration-attached
                        // aggregates that arrive over MPI, and the balance check re-reads both in
                        // the same roles in both directions, so it stays at zero under this
                        // break. Off by default; a check that does not FAIL with this flag on has
                        // no power (memory feedback_measure_check_power).
                        if (config.pt_spec.break_transport_for_test) {
                            std::swap(partner_edge, partner_plaquette);
                        }
                        double log_r = config.lat_spec.beta
                            * (local_single - partner_single) * std::log(partner_h / local_h)
                            + (config.pt_spec.break_lmbda_term_for_test
                                ? 0.
                                : (partner_lmbda - local_lmbda) * (local_edge - partner_edge));
                        // Common-factor tempering terms; zero contribution (and no new
                        // floating-point operations) when mu/J are untempered, matching
                        // the h/lmbda guard pattern above (M1 derivation section 2).
                        if (local_mu != partner_mu) {
                            if (local_mu <= 0. || partner_mu <= 0.) {
                                throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
                            }
                            log_r += config.lat_spec.beta
                                * (local_tuple - partner_tuple) * std::log(partner_mu / local_mu);
                        }
                        if (local_J != partner_J) {
                            log_r += (partner_J - local_J) * (local_plaquette - partner_plaquette);
                        }
                        // M4 reverse-move balance check (debug-mode, guarded by
                        // validate_swap_weight so it costs nothing when disabled):
                        // an INDEPENDENTLY re-typed expression, not a call to the
                        // forward formula with arguments swapped. Configuration-
                        // attached quantities (local_single/partner_single/
                        // local_tuple/partner_tuple/local_edge/partner_edge/
                        // local_plaquette/partner_plaquette) are UNCHANGED, because a
                        // swap moves coupling assignment, not configuration; only the
                        // coupling roles (h/mu/lmbda/J) are swapped. If every term is
                        // correct, log_r_reverse == -log_r exactly, by the same
                        // algebra as doc/m1_common_factor_swap_weight_derivation.md.
                        //
                        // BLIND SPOT (Feynman, M5 review): forward and reverse both
                        // read the SAME already-exchanged local_*/partner_* values.
                        // If a quantity were mis-transported -- a wrong MPI tag
                        // pairing sending local_tuple where local_plaquette is
                        // expected, say -- both directions would use the same wrong
                        // number and balance_err would still read zero. This check
                        // validates the ALGEBRA, not the TRANSPORT. The two-rung
                        // marginal test at M7 is what covers transport.
                        if (config.pt_spec.validate_swap_weight) {
                            double log_r_reverse = config.lat_spec.beta
                                * (local_single - partner_single) * std::log(local_h / partner_h)
                                + (local_lmbda - partner_lmbda) * (local_edge - partner_edge);
                            if (local_mu != partner_mu) {
                                log_r_reverse += config.lat_spec.beta
                                    * (local_tuple - partner_tuple) * std::log(local_mu / partner_mu);
                            }
                            if (local_J != partner_J) {
                                log_r_reverse += (local_J - partner_J) * (local_plaquette - partner_plaquette);
                            }
                            pt_diagnostics.max_swap_balance_err = std::max(
                                pt_diagnostics.max_swap_balance_err, std::abs(log_r + log_r_reverse));
                            // M8 (Feynman): cross-check the inline log_r (what production
                            // actually executes) against parallel_tempering_log_weight_ratio_from_scalars
                            // -- the SAME scalar core the shared function (what the debug
                            // validators and M7'' exercise) delegates to -- on the SAME
                            // live, MPI-exchanged local_*/partner_* values. There is no
                            // partner_lat object here (only the scalars came over MPI), so
                            // this calls the scalar core directly rather than the
                            // Lattice-object wrapper. Closes the gap M3 found: these were
                            // two separately hand-maintained copies of the same formula.
                            const double shared_log_r = parallel_tempering_log_weight_ratio_from_scalars(
                                config.lat_spec.beta, local_h, partner_h, local_mu, partner_mu,
                                local_lmbda, partner_lmbda, local_J, partner_J,
                                local_single, partner_single, local_tuple, partner_tuple,
                                local_edge, partner_edge, local_plaquette, partner_plaquette);
                            pt_diagnostics.max_shared_function_deviation = std::max(
                                pt_diagnostics.max_shared_function_deviation, std::abs(log_r - shared_log_r));
                        }
                        // M10 (Feynman): accumulate the exact identity <R> = 1 for this link, with
                        // R = exp(log_r) assembled from the live MPI-exchanged partner data. This is
                        // the two-rung identity of M7'' carried into a real ladder of any width, so
                        // it tests the partner choice, the transport and the algebra together
                        // (memory reference_pt_swap_weight_identity). It is only an identity while
                        // each replica stays on its own rung, so it is meaningful with
                        // never_accept_swaps set. Both ranks of a pair compute the SAME log_r, so
                        // only the lower rank records and each attempt is counted once.
                        if (config.pt_spec.validate_swap_weight && pt_rank < partner
                            && link_index >= 0 && link_index < pt_world_size - 1) {
                            const double R_value = std::exp(log_r);
                            const size_t R_bin = static_cast<size_t>(link_index);
                            ++pt_diagnostics.R_count_by_link[R_bin];
                            pt_diagnostics.R_sum_by_link[R_bin] += R_value;
                            pt_diagnostics.R_sumsq_by_link[R_bin] += R_value * R_value;
                        }
                        int accepted = 0;
                        if (pt_rank < partner) {
                            const double u = std::max(uniform_dist(*rng), std::numeric_limits<double>::min());
                            accepted = (log_r >= 0. || std::log(u) < log_r) ? 1 : 0;
                            // M10: identity mode still ATTEMPTS every swap and still draws u, so the
                            // RNG stream is unchanged. It only never ACCEPTS, which keeps every
                            // replica on its own rung.
                            if (config.pt_spec.never_accept_swaps) { accepted = 0; }
                            mpi_detail::fail_on_mpi_error(
                                MPI_Send(&accepted, 1, MPI_INT, partner, 7106, MPI_COMM_WORLD),
                                "MPI_Send(accepted)"
                            );
                        } else {
                            mpi_detail::fail_on_mpi_error(
                                MPI_Recv(&accepted, 1, MPI_INT, partner, 7106,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                                "MPI_Recv(accepted)"
                            );
                        }
                        if (accepted) {
                            ++pt_diagnostics.swap_accepted;
                            if (link_index >= 0 && link_index < pt_world_size - 1) {
                                ++pt_diagnostics.swap_accepted_by_link[static_cast<size_t>(link_index)];
                            }
                            runtime_h = partner_h;
                            runtime_lmbda = partner_lmbda;
                            runtime_mu = partner_mu;
                            runtime_J = partner_J;
                            pt_ladder_index = partner_index;
                            integrated_pot_energy = total_integrated_pot_energy(
                                lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda);
                        }
                    }
                }
#endif
                } else if (pt_enabled) {
                    throw std::invalid_argument("Parallel tempering requires the Boost Lattice backend.");
                }
            }
        }
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    const std::int64_t reset_potential_energy_count =
        static_cast<std::int64_t>(lat.get_edge_count()) * 100000;
    start_acceptance_diagnostics(production_acceptance, config.out_spec.restart_from.empty());
    auto last_checkpoint_time = std::chrono::steady_clock::now();
    bool checkpoint_pending = false;
#ifdef PARATORIC_HAS_MPI
    const bool checkpointing_enabled = config.out_spec.checkpoint_every_samples > 0
        || config.out_spec.checkpoint_every_seconds > 0;
#endif
    pt_diagnostics.ladder_index_by_sample.reserve(sample_count);

    for (std::int64_t i = completed_samples; i < config.sim_spec.N_samples; ++i) {
        start_acceptance_block();
        for (std::int64_t j = 0; j < config.sim_spec.N_between_samples; ++j) {
            ++total_metropolis_step_count;
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, runtime_h,
                runtime_mu, runtime_J, runtime_lmbda
            );
            if constexpr (std::same_as<LatticeT, Lattice>) {
#ifdef PARATORIC_HAS_MPI
            if (pt_enabled && (total_metropolis_step_count % pt_swap_period_moves) == 0) {
                const int phase = static_cast<int>((total_metropolis_step_count / pt_swap_period_moves) & 1);
                std::vector<int> ladder_index_by_rank(static_cast<size_t>(pt_world_size), -1);
                mpi_detail::fail_on_mpi_error(
                    MPI_Allgather(&pt_ladder_index, 1, MPI_INT,
                                  ladder_index_by_rank.data(), 1, MPI_INT,
                                  MPI_COMM_WORLD),
                    "MPI_Allgather(pt_ladder_index)"
                );
                const auto partner_rank = parallel_tempering_partner_rank(
                    pt_rank, ladder_index_by_rank, phase);
                if (partner_rank) {
                    const int partner = *partner_rank;
                    ++pt_diagnostics.swap_attempted;
                    double partner_h = 0.;
                    double partner_lmbda = 0.;
                    double partner_mu = 0.;
                    double partner_J = 0.;
                    int partner_index = -1;
                    const double local_h = runtime_h;
                    const double local_lmbda = runtime_lmbda;
                    const double local_mu = runtime_mu;
                    const double local_J = runtime_J;
                    const int local_index = pt_ladder_index;
                    // M10 power test for the routing guard: send a ladder index that is not
                    // the one this rank holds, so the partner's guard has something to catch.
                    const int sent_index = config.pt_spec.break_routing_for_test
                        ? local_index + 1 : local_index;
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_h, 1, MPI_DOUBLE, partner, 7201,
                                     &partner_h, 1, MPI_DOUBLE, partner, 7201,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_h)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_lmbda, 1, MPI_DOUBLE, partner, 7207,
                                     &partner_lmbda, 1, MPI_DOUBLE, partner, 7207,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_lmbda)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_mu, 1, MPI_DOUBLE, partner, 7209,
                                     &partner_mu, 1, MPI_DOUBLE, partner, 7209,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_mu)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_J, 1, MPI_DOUBLE, partner, 7210,
                                     &partner_J, 1, MPI_DOUBLE, partner, 7210,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_J)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&sent_index, 1, MPI_INT, partner, 7202,
                                     &partner_index, 1, MPI_INT, partner, 7202,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_index)"
                    );
                    const int link_index = std::min(local_index, partner_index);
                    if (link_index >= 0 && link_index < pt_world_size - 1) {
                        ++pt_diagnostics.swap_attempted_by_link[static_cast<size_t>(link_index)];
                    }
                    // M10 routing guard (Feynman). Always on: four integer comparisons per
                    // attempt. The Allgather above told this rank which ladder index EVERY
                    // rank currently holds. If the payload that arrives from `partner` does
                    // not carry that rank's ladder index, the data came from somewhere other
                    // than the intended partner, and every number in the weight below belongs
                    // to the wrong rung. The M4 reverse-move balance check cannot see this,
                    // because it re-reads the same received values in both directions. A
                    // two-rung ladder cannot exercise it at all: with one link there is only
                    // one possible partner, so any routing is trivially correct. This is the
                    // check that has content once the ladder is wider than two rungs.
                    if (partner_index != ladder_index_by_rank[static_cast<size_t>(partner)]
                        || (local_index - partner_index != 1 && partner_index - local_index != 1)) {
                        throw std::runtime_error(
                            "PT swap routing check failed: rank " + std::to_string(pt_rank)
                            + " (ladder index " + std::to_string(local_index)
                            + ") received ladder index " + std::to_string(partner_index)
                            + " from rank " + std::to_string(partner)
                            + ", but the Allgather reported ladder index "
                            + std::to_string(ladder_index_by_rank[static_cast<size_t>(partner)])
                            + " for that rank.");
                    }
                    const double local_single = lat.get_non_diag_single_energy_z();
                    const double local_edge = lat.cached_total_integrated_edge_energy();
                    const double local_tuple = lat.get_non_diag_tuple_energy_z();
                    const double local_plaquette = lat.cached_total_integrated_plaquette_energy();
                    double partner_single = 0.;
                    double partner_edge = 0.;
                    double partner_tuple = 0.;
                    double partner_plaquette = 0.;
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_single, 1, MPI_DOUBLE, partner, 7203,
                                     &partner_single, 1, MPI_DOUBLE, partner, 7203,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_single)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_edge, 1, MPI_DOUBLE, partner, 7204,
                                     &partner_edge, 1, MPI_DOUBLE, partner, 7204,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_edge)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_tuple, 1, MPI_DOUBLE, partner, 7211,
                                     &partner_tuple, 1, MPI_DOUBLE, partner, 7211,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_tuple)"
                    );
                    mpi_detail::fail_on_mpi_error(
                        MPI_Sendrecv(&local_plaquette, 1, MPI_DOUBLE, partner, 7212,
                                     &partner_plaquette, 1, MPI_DOUBLE, partner, 7212,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                        "MPI_Sendrecv(local_plaquette)"
                    );
                    // M6 follow-up: same positivity guard as the thermalization-loop
                    // copy of this block (see its comment for why).
                    if (local_h <= 0. || partner_h <= 0.) {
                        throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
                    }
                    // M10 power test (Feynman). Exchanges the roles of the RECEIVED
                    // partner_edge and partner_plaquette values. Those two are the exact pair
                    // the BLIND SPOT comment below names: both are configuration-attached
                    // aggregates that arrive over MPI, and the balance check re-reads both in
                    // the same roles in both directions, so it stays at zero under this
                    // break. Off by default; a check that does not FAIL with this flag on has
                    // no power (memory feedback_measure_check_power).
                    if (config.pt_spec.break_transport_for_test) {
                        std::swap(partner_edge, partner_plaquette);
                    }
                    double log_r = config.lat_spec.beta
                        * (local_single - partner_single) * std::log(partner_h / local_h)
                        + (config.pt_spec.break_lmbda_term_for_test
                            ? 0.
                            : (partner_lmbda - local_lmbda) * (local_edge - partner_edge));
                    if (local_mu != partner_mu) {
                        if (local_mu <= 0. || partner_mu <= 0.) {
                            throw std::invalid_argument("PT ladder values must be positive for kink-count exchange weights.");
                        }
                        log_r += config.lat_spec.beta
                            * (local_tuple - partner_tuple) * std::log(partner_mu / local_mu);
                    }
                    if (local_J != partner_J) {
                        log_r += (partner_J - local_J) * (local_plaquette - partner_plaquette);
                    }
                    if (config.pt_spec.validate_swap_weight) {
                        const double brute_local_single = lat.recompute_non_diag_single_energy_z();
                        const double brute_local_edge = lat.recompute_total_integrated_edge_energy();
                        const double brute_local_tuple = lat.recompute_non_diag_tuple_energy_z();
                        const double brute_local_plaquette = lat.recompute_total_integrated_plaquette_energy();
                        double brute_partner_single = 0.;
                        double brute_partner_edge = 0.;
                        double brute_partner_tuple = 0.;
                        double brute_partner_plaquette = 0.;
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&brute_local_single, 1, MPI_DOUBLE, partner, 7205,
                                         &brute_partner_single, 1, MPI_DOUBLE, partner, 7205,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(brute_local_single)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&brute_local_edge, 1, MPI_DOUBLE, partner, 7206,
                                         &brute_partner_edge, 1, MPI_DOUBLE, partner, 7206,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(brute_local_edge)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&brute_local_tuple, 1, MPI_DOUBLE, partner, 7213,
                                         &brute_partner_tuple, 1, MPI_DOUBLE, partner, 7213,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(brute_local_tuple)"
                        );
                        mpi_detail::fail_on_mpi_error(
                            MPI_Sendrecv(&brute_local_plaquette, 1, MPI_DOUBLE, partner, 7214,
                                         &brute_partner_plaquette, 1, MPI_DOUBLE, partner, 7214,
                                         MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Sendrecv(brute_local_plaquette)"
                        );
                        double brute_log_r = config.lat_spec.beta
                            * (brute_local_single - brute_partner_single) * std::log(partner_h / local_h)
                            + (partner_lmbda - local_lmbda) * (brute_local_edge - brute_partner_edge);
                        if (local_mu != partner_mu) {
                            brute_log_r += config.lat_spec.beta
                                * (brute_local_tuple - brute_partner_tuple) * std::log(partner_mu / local_mu);
                        }
                        if (local_J != partner_J) {
                            brute_log_r += (partner_J - local_J) * (brute_local_plaquette - brute_partner_plaquette);
                        }
                        pt_diagnostics.max_swap_log_weight_deviation = std::max(
                            pt_diagnostics.max_swap_log_weight_deviation,
                            std::abs(log_r - brute_log_r));
                        // M4 reverse-move balance check, independently re-typed (see
                        // the comment on the thermalization-loop copy of this block
                        // for the derivation). BLIND SPOT: validates the algebra, not
                        // the MPI transport -- see that comment for why.
                        double log_r_reverse = config.lat_spec.beta
                            * (local_single - partner_single) * std::log(local_h / partner_h)
                            + (local_lmbda - partner_lmbda) * (local_edge - partner_edge);
                        if (local_mu != partner_mu) {
                            log_r_reverse += config.lat_spec.beta
                                * (local_tuple - partner_tuple) * std::log(local_mu / partner_mu);
                        }
                        if (local_J != partner_J) {
                            log_r_reverse += (local_J - partner_J) * (local_plaquette - partner_plaquette);
                        }
                        pt_diagnostics.max_swap_balance_err = std::max(
                            pt_diagnostics.max_swap_balance_err, std::abs(log_r + log_r_reverse));
                        // M8: cross-check against the scalar core (see the
                        // thermalization-loop copy of this block for why there is no
                        // partner_lat here).
                        const double shared_log_r = parallel_tempering_log_weight_ratio_from_scalars(
                            config.lat_spec.beta, local_h, partner_h, local_mu, partner_mu,
                            local_lmbda, partner_lmbda, local_J, partner_J,
                            local_single, partner_single, local_tuple, partner_tuple,
                            local_edge, partner_edge, local_plaquette, partner_plaquette);
                        pt_diagnostics.max_shared_function_deviation = std::max(
                            pt_diagnostics.max_shared_function_deviation, std::abs(log_r - shared_log_r));
                    }
                    // M10 (Feynman): accumulate the exact identity <R> = 1 for this link, with
                    // R = exp(log_r) assembled from the live MPI-exchanged partner data. This is
                    // the two-rung identity of M7'' carried into a real ladder of any width, so
                    // it tests the partner choice, the transport and the algebra together
                    // (memory reference_pt_swap_weight_identity). It is only an identity while
                    // each replica stays on its own rung, so it is meaningful with
                    // never_accept_swaps set. Both ranks of a pair compute the SAME log_r, so
                    // only the lower rank records and each attempt is counted once.
                    if (config.pt_spec.validate_swap_weight && pt_rank < partner
                        && link_index >= 0 && link_index < pt_world_size - 1) {
                        const double R_value = std::exp(log_r);
                        const size_t R_bin = static_cast<size_t>(link_index);
                        ++pt_diagnostics.R_count_by_link[R_bin];
                        pt_diagnostics.R_sum_by_link[R_bin] += R_value;
                        pt_diagnostics.R_sumsq_by_link[R_bin] += R_value * R_value;
                    }
                    int accepted = 0;
                    if (pt_rank < partner) {
                        const double u = std::max(uniform_dist(*rng), std::numeric_limits<double>::min());
                        accepted = (log_r >= 0. || std::log(u) < log_r) ? 1 : 0;
                        // M10: identity mode still ATTEMPTS every swap and still draws u, so the
                        // RNG stream is unchanged. It only never ACCEPTS, which keeps every
                        // replica on its own rung.
                        if (config.pt_spec.never_accept_swaps) { accepted = 0; }
                        mpi_detail::fail_on_mpi_error(
                            MPI_Send(&accepted, 1, MPI_INT, partner, 7208, MPI_COMM_WORLD),
                            "MPI_Send(accepted)"
                        );
                    } else {
                        mpi_detail::fail_on_mpi_error(
                            MPI_Recv(&accepted, 1, MPI_INT, partner, 7208,
                                     MPI_COMM_WORLD, MPI_STATUS_IGNORE),
                            "MPI_Recv(accepted)"
                        );
                    }
                    if (accepted) {
                        ++pt_diagnostics.swap_accepted;
                        if (link_index >= 0 && link_index < pt_world_size - 1) {
                            ++pt_diagnostics.swap_accepted_by_link[static_cast<size_t>(link_index)];
                        }
                        runtime_h = partner_h;
                        runtime_lmbda = partner_lmbda;
                        runtime_mu = partner_mu;
                        runtime_J = partner_J;
                        pt_ladder_index = partner_index;
                        integrated_pot_energy = total_integrated_pot_energy(
                            lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda);
                    }
                }
            }
#endif
            } else if (pt_enabled) {
                throw std::invalid_argument("Parallel tempering requires the Boost Lattice backend.");
            }
            if (reset_potential_energy_count > 0 && total_metropolis_step_count % reset_potential_energy_count == 0) [[unlikely]] {
                // avoid accumulation of small numerical errors leading to bias
                lat.init_potential_energy();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda
                );
            }
        }
        finish_acceptance_block();

        lat.rotate_imag_time();
        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            observable_vector[k].emplace_back(
                obs_func_vec[k](lat, runtime_h, runtime_lmbda, runtime_mu, runtime_J)
            );
        }
        pt_diagnostics.ladder_index_by_sample.emplace_back(pt_ladder_index);

        // TELEMETRY (Feynman, 2026-08-22). A tempering run writes its HDF5 only at the
        // end and has no checkpoint, so a job that overruns its wall limit is a total
        // loss: jobs 15939774 and 15939776 cost 704 core-hours and produced no file.
        // This line lets a run be judged, and stopped, while it is still going. It is
        // printed by the rank that currently holds the COLDEST rung, which is the only
        // rung we measure, and it carries the two numbers that decide a run: the total
        // round-trip count and whether the cold-rung observable changes sign.
        if (!pt_diagnostics.ladder_index_by_sample.empty()) {
            const size_t n_taken = pt_diagnostics.ladder_index_by_sample.size();
            const size_t period = 5000;   // 20000 was too coarse: the L=32 probe (12000 samples) printed nothing at all
            const int cold_index = static_cast<int>(pt_world_size) - 1;
            if (pt_world_size > 1 && pt_ladder_index == cold_index && n_taken % period == 0) {
                size_t traverses = 0;
                int last_end = -1;
                for (int v : pt_diagnostics.ladder_index_by_sample) {
                    if (v == 0 || v == cold_index) {
                        if (last_end != -1 && v != last_end) ++traverses;
                        last_end = v;
                    }
                }
                double cold_mean = 0.0;
                size_t n_cold = 0, sign_changes = 0;
                int prev_sign = 0;
                if (!observable_vector.empty()) {
                    for (size_t i = 0; i < observable_vector[0].size(); ++i) {
                        if (pt_diagnostics.ladder_index_by_sample[i] != cold_index) continue;
                        const auto& var = observable_vector[0][i];
                        const double v = std::holds_alternative<double>(var)
                            ? std::get<double>(var)
                            : std::get<std::complex<double>>(var).real();
                        cold_mean += v; ++n_cold;
                        const int sg = (v > 0) - (v < 0);
                        if (prev_sign != 0 && sg != 0 && sg != prev_sign) ++sign_changes;
                        if (sg != 0) prev_sign = sg;
                    }
                }
                std::cout << "PROGRESS samples=" << n_taken
                          << " swap_attempts=" << pt_diagnostics.swap_attempted
                          << " traverses=" << traverses
                          << " round_trips=" << (traverses / 2)
                          << " cold_n=" << n_cold
                          << " cold_mean=" << (n_cold ? cold_mean / static_cast<double>(n_cold) : 0.0)
                          << " cold_sign_changes=" << sign_changes
                          << std::endl;
            }
        }

        if (config.out_spec.save_snapshots) {
            lat.update_spin_string();
        }

        completed_samples = i + 1;
        const auto checkpoint_now = std::chrono::steady_clock::now();
        bool checkpoint_requested = checkpoint_due(
            config, completed_samples, checkpoint_now, last_checkpoint_time);
#ifdef PARATORIC_HAS_MPI
        if (pt_enabled && checkpointing_enabled) {
            const int local_request = checkpoint_requested ? 1 : 0;
            int global_request = 0;
            mpi_detail::fail_on_mpi_error(
                MPI_Allreduce(&local_request, &global_request, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD),
                "MPI_Allreduce(checkpoint request)"
            );
            checkpoint_requested = global_request != 0;
        }
#endif
        if (checkpoint_requested) {
            // Keep the timer synchronized after the collective decision; otherwise
            // only the first rank whose wall clock fired would reset its timer.
            last_checkpoint_time = checkpoint_now;
            checkpoint_pending = true;
        }
        const bool at_swap_boundary = !pt_enabled
            || (total_metropolis_step_count % pt_swap_period_moves == 0);
        if (checkpoint_pending && (at_swap_boundary || completed_samples == config.sim_spec.N_samples)) {
#ifdef PARATORIC_HAS_MPI
            if (pt_enabled) {
                // All ranks have reached the same completed-sample/swap boundary
                // before any rank starts serializing its rank-local state.
                mpi_detail::fail_on_mpi_error(
                    MPI_Barrier(MPI_COMM_WORLD), "MPI_Barrier(checkpoint)"
                );
            }
#endif
            write_sample_checkpoint(
                config.out_spec.checkpoint_path,
                config,
                lat,
                SampleCheckpointState{
                    .completed_samples = completed_samples,
                    .production_metropolis_steps = total_metropolis_step_count,
                    .integrated_pot_energy = integrated_pot_energy,
                    .acc_ratio = acc_ratio,
                    .production_acceptance = production_acceptance,
                    .observable_vector = observable_vector,
                    // M9: persist wherever swaps have carried this replica.
                    .pt_ladder_index = pt_ladder_index,
                    .runtime_h = runtime_h,
                    .runtime_mu = runtime_mu,
                    .runtime_J = runtime_J,
                    .runtime_lmbda = runtime_lmbda,
                    .global_checkpoint_marker = completed_samples
                        + (pt_enabled && config.pt_spec.break_checkpoint_marker_for_test && pt_rank == 0 ? 1 : 0),
                    .has_pt_diagnostics = pt_enabled,
                    .pt_diagnostics = pt_diagnostics
                }
            );
            checkpoint_pending = false;
        }
    }
    stop_acceptance_diagnostics();

    if (config.out_spec.save_snapshots) {
        lat.write_graph("snapshots", config.out_spec.path_out);
    }

    for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
        if (obs_type_vec[k] == "real") {
            std::vector<double> obs_real;
            const auto& series = observable_vector[k];
            obs_real.reserve(series.size());
            for (auto const& x : series) {
                obs_real.emplace_back(std::get<double>(x));
            }

            const auto& [observable_mean, observable_std, binder_mean, binder_std] 
            = paratoric::statistics::get_bootstrap_statistics(obs_real, rng, config.sim_spec.N_resamples);

            observable_mean_vector[k] = observable_mean;
            observable_std_vector[k] = observable_std;
            binder_mean_vector[k] = binder_mean;
            binder_std_vector[k] = binder_std;
            observable_autocorrelation_time_vector[k] 
            = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
        } else if (obs_type_vec[k] == "fredenhagen_marcu") {
            const auto& series = observable_vector[k];
            const size_t N = series.size();
            std::vector<double> obs_real, obs_imag;
            obs_real.reserve(N);
            obs_imag.reserve(N);

            for (auto const& v : series) {
                if (auto p = std::get_if<std::complex<double>>(&v)) {
                    // v holds a complex<double>
                    obs_real.push_back(p->real());
                    obs_imag.push_back(p->imag());
                }
                else {
                    // v must hold a double
                    double d = std::get<double>(v);
                    obs_real.push_back(d);
                    obs_imag.push_back(0.0);
                }
            }

            const auto& [observable_mean, observable_std, binder_mean, binder_std] 
            = paratoric::statistics::get_bootstrap_statistics_fm(obs_real, obs_imag, rng, config.sim_spec.N_resamples);

            observable_mean_vector[k] = observable_mean;
            observable_std_vector[k] = observable_std;
            binder_mean_vector[k] = binder_mean;
            binder_std_vector[k] = binder_std;
            observable_autocorrelation_time_vector[k] 
            = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
        } else if (obs_type_vec[k] == "susceptibility") {
            const auto& series = observable_vector[k];
            const size_t N = series.size();
            std::vector<double> obs_real, obs_imag;
            obs_real.reserve(N);
            obs_imag.reserve(N);

            for (auto const& v : series) {
                if (auto p = std::get_if<std::complex<double>>(&v)) {
                    // v holds a complex<double>
                    obs_real.push_back(p->real());
                    obs_imag.push_back(p->imag());
                }
                else {
                    // v must hold a double
                    double d = std::get<double>(v);
                    obs_real.push_back(d);
                    obs_imag.push_back(0.0);
                }
            }
            //TODO fix this, every observable should just define their susceptibility function
            if ((config.sim_spec.observables[k] == "sigma_z_static_susceptibility" && Basis == 'x')) {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_susceptibility(
                    obs_real, config.lat_spec.beta, runtime_lmbda, 
                    lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
            } else if (config.sim_spec.observables[k] == "sigma_x_static_susceptibility" && Basis == 'z') {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_susceptibility(
                    obs_real, config.lat_spec.beta, runtime_h, 
                    lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
            } else if ((config.sim_spec.observables[k] == "sigma_z_dynamical_susceptibility" && Basis == 'x')
                        || (config.sim_spec.observables[k] == "sigma_x_dynamical_susceptibility" && Basis == 'z')) {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_dynamical_susceptibility(
                    obs_real, obs_imag, lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
            } else {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics_susceptibility(
                    obs_real, obs_imag, rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(obs_real, config.sim_spec.observables[k]);
            }
        }
    }

    integrated_pot_energy_check = total_integrated_pot_energy(
        lat, runtime_h, runtime_mu, runtime_J, runtime_lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    pt_diagnostics.final_ladder_index = pt_ladder_index;
    pt_diagnostics.final_s = runtime_h;
    pt_diagnostics.final_mu = runtime_mu;
    pt_diagnostics.final_J = runtime_J;
    pt_diagnostics.final_lmbda = runtime_lmbda;

    return Result{
        .series=std::move(observable_vector), 
        .production_acceptance=std::move(production_acceptance),
        .pt_diagnostics=std::move(pt_diagnostics),
        .mean=std::move(observable_mean_vector), 
        .mean_std=std::move(observable_std_vector), 
        .binder=std::move(binder_mean_vector), 
        .binder_std=std::move(binder_std_vector), 
        .tau_int=std::move(observable_autocorrelation_time_vector)
    };                                       
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis, LatticeT>::get_core_update_benchmark(
    const Config& config
) {
    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }

    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (config.param_spec.lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (config.param_spec.h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    auto lat = LatticeT(config.lat_spec, rng);
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    double acc_ratio = 1.;

    AcceptanceDiagnostics diagnostics;
    start_acceptance_diagnostics(diagnostics);
    start_acceptance_block();

    const std::int64_t thermalization_updates = std::max(config.sim_spec.N_thermalization, std::int64_t{0});
    const std::int64_t sample_updates =
        static_cast<std::int64_t>(std::max(config.sim_spec.N_samples, 0))
        * static_cast<std::int64_t>(std::max(config.sim_spec.N_between_samples, 0));
    const std::int64_t total_updates = thermalization_updates + sample_updates;

    for (std::int64_t i = 0; i < total_updates; ++i) {
        metropolis_step(
            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta,
            config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
        );
    }

    finish_acceptance_block();
    stop_acceptance_diagnostics();

    const double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format(
            "Integrated potential energy mismatch. {} does not match {}.",
            integrated_pot_energy,
            integrated_pot_energy_check));
    }

    return Result{
        .production_acceptance=std::move(diagnostics)
    };
}

template<char Basis, typename LatticeT>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis, LatticeT>::get_hysteresis(
    const Config& config
) {

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }
    
    double h{}, lmbda{};
    
    if (!config.param_spec.h_hys.empty()) {
        h = config.param_spec.h_hys.front();
    } else {
        throw std::invalid_argument("h_hys must be non-empty.");
    }

    if (!config.param_spec.lmbda_hys.empty()) {
        lmbda = config.param_spec.lmbda_hys.front();
    } else {
        throw std::invalid_argument("lmbda_hys must be non-empty.");
    }

    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);
    auto obs_type_vec = get_obs_type_vec(config.sim_spec.observables);
    
    // Vector to store observable results for all snapshots for all parameters
    std::vector<std::vector<std::vector<std::variant< std::complex<double>, double>>>> hys_vector;
    std::vector<std::vector<double>> hys_mean,
                                     hys_mean_std,
                                     hys_binder,
                                     hys_binder_std,
                                     hys_autocorrelation_time;

    // Initialize Lattice
    auto lat = LatticeT(config.lat_spec, rng);
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    double acc_ratio = 1.;

    // Thermalization 
    for (std::int64_t i = 0; i < config.sim_spec.N_thermalization; ++i) {
        metropolis_step(
            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
            config.param_spec.h, config.param_spec.mu, config.param_spec.J, 
            config.param_spec.lmbda
        );
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    std::int64_t total_metropolis_step_count = 0;
    const std::int64_t reset_potential_energy_count =
        static_cast<std::int64_t>(lat.get_edge_count()) * 100000;

    for (size_t n = 0; n < std::min( config.param_spec.h_hys.size(), std::min(config.param_spec.lmbda_hys.size(), config.out_spec.paths_out.size()) ); n++) {
        // Vector to store observable results for all snapshots
        const auto sample_count = static_cast<size_t>(std::max(config.sim_spec.N_samples, 0));
        std::vector<std::vector<std::variant< std::complex<double>, double>>> observable_vector;
        observable_vector.reserve(config.sim_spec.observables.size());
        std::vector<double> observable_mean_vector(config.sim_spec.observables.size(), 0.), 
                            observable_std_vector(config.sim_spec.observables.size(), 0.), 
                            binder_mean_vector(config.sim_spec.observables.size(), 0.), 
                            binder_std_vector(config.sim_spec.observables.size(), 0.), 
                            observable_autocorrelation_time_vector(config.sim_spec.observables.size(), 0.);
        for (const auto& obs_func : config.sim_spec.observables) {
            UNUSED(obs_func);
            observable_vector.emplace_back();
            observable_vector.back().reserve(sample_count);
        } 
        auto path_out = config.out_spec.paths_out [ n ];
        h = config.param_spec.h_hys[ n ];
        lmbda = config.param_spec.lmbda_hys[ n ];

        lat.init_potential_energy();
        lat.rotate_imag_time();
        integrated_pot_energy = total_integrated_pot_energy(
            lat, h, config.param_spec.mu, 
            config.param_spec.J, lmbda
        );

        if constexpr (Basis == 'x') {
            if (config.param_spec.J < 0) {
                throw std::invalid_argument("J must be non-negative in the x-basis.");
            } else if (lmbda < 0) {
                throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
            }
        } else if constexpr (Basis == 'z') {
            if (config.param_spec.mu < 0) {
                throw std::invalid_argument("mu must be non-negative in the z-basis.");
            } else if (h < 0) {
                throw std::invalid_argument("h must be non-negative in the z-basis.");
            }
        }
        
        std::int64_t N_rethermalization = static_cast<std::int64_t>(config.sim_spec.N_thermalization/4);

        for (std::int64_t t = 0; t < N_rethermalization; ++t) {
            ++total_metropolis_step_count;
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h, 
                config.param_spec.mu, config.param_spec.J, lmbda
            );
            if (reset_potential_energy_count > 0 && total_metropolis_step_count % reset_potential_energy_count == 0) [[unlikely]] {
                // avoid accumulation of small numerical errors leading to bias
                lat.init_potential_energy();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, h, config.param_spec.mu, 
                    config.param_spec.J, lmbda
                );
            }
        }

        for (std::int64_t i = 0; i < config.sim_spec.N_samples; ++i) {
            for (std::int64_t j = 0; j < config.sim_spec.N_between_samples; ++j) {
                ++total_metropolis_step_count;
                metropolis_step(
                    lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                    h, config.param_spec.mu, config.param_spec.J, lmbda
                );
                if (reset_potential_energy_count > 0 && total_metropolis_step_count % reset_potential_energy_count == 0) [[unlikely]] {
                    // avoid accumulation of small numerical errors leading to bias
                    lat.init_potential_energy();
                    integrated_pot_energy = total_integrated_pot_energy(
                        lat, h, config.param_spec.mu, 
                        config.param_spec.J, lmbda
                    );
                }
            }

            lat.rotate_imag_time();
            for (size_t k = 0; k < config.sim_spec.observables.size(); k++)
                observable_vector[k].emplace_back(obs_func_vec[k](lat, h, lmbda, config.param_spec.mu, config.param_spec.J));

            if (config.out_spec.save_snapshots)
                lat.update_spin_string();
        }

        if (config.out_spec.save_snapshots) {
            lat.write_graph("snapshots", config.out_spec.path_out);
        }

        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            if (obs_type_vec[k] == "real") {
                std::vector<double> obs_real;
                const auto& series = observable_vector[k];
                obs_real.reserve(series.size());
                for (auto const& x : series) {
                // if you know it's always a double here, use get<>
                obs_real.emplace_back(std::get<double>(x));
                }

                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics(obs_real, rng, config.sim_spec.N_resamples);

                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(
                    obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                );
            } else if (obs_type_vec[k] == "fredenhagen_marcu") {
                const auto& series = observable_vector[k];
                const size_t N = series.size();
                std::vector<double> obs_real, obs_imag;
                obs_real.reserve(N);
                obs_imag.reserve(N);

                for (auto const& v : series) {
                    if (auto p = std::get_if<std::complex<double>>(&v)) {
                        // v holds a complex<double>
                        obs_real.push_back(p->real());
                        obs_imag.push_back(p->imag());
                    }
                    else {
                        // v must hold a double
                        double d = std::get<double>(v);
                        obs_real.push_back(d);
                        obs_imag.push_back(0.0);
                    }
                }

                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics_fm(obs_real, obs_imag, rng, config.sim_spec.N_resamples);

                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = calculate_autocorrelation_time_with_warning(
                    obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                );
            } else if (obs_type_vec[k] == "susceptibility") {
                const auto& series = observable_vector[k];
                const size_t N = series.size();
                std::vector<double> obs_real, obs_imag;
                obs_real.reserve(N);
                obs_imag.reserve(N);

                for (auto const& v : series) {
                    if (auto p = std::get_if<std::complex<double>>(&v)) {
                        // v holds a complex<double>
                        obs_real.push_back(p->real());
                        obs_imag.push_back(p->imag());
                    }
                    else {
                        // v must hold a double
                        double d = std::get<double>(v);
                        obs_real.push_back(d);
                        obs_imag.push_back(0.0);
                    }
                }

                if ((config.sim_spec.observables[k] == "sigma_z_static_susceptibility" && Basis == 'x')) {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_susceptibility(
                        obs_real, config.lat_spec.beta, lmbda, 
                        lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = calculate_autocorrelation_time_with_warning(
                        obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                    );
                } else if (config.sim_spec.observables[k] == "sigma_x_static_susceptibility" && Basis == 'z') {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_susceptibility(
                        obs_real, config.lat_spec.beta, h, 
                        lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = calculate_autocorrelation_time_with_warning(
                        obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                    );
                } else if ((config.sim_spec.observables[k] == "sigma_z_dynamical_susceptibility" && Basis == 'x')
                            || (config.sim_spec.observables[k] == "sigma_x_dynamical_susceptibility" && Basis == 'z')) {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_dynamical_susceptibility(
                        obs_real, obs_imag, lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = calculate_autocorrelation_time_with_warning(
                        obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                    );
                } else {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::get_bootstrap_statistics_susceptibility(
                        obs_real, obs_imag, rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = calculate_autocorrelation_time_with_warning(
                        obs_real, config.sim_spec.observables[k], true, n, h, lmbda
                    );
                }
            } 
        }

        hys_vector.emplace_back( std::move(observable_vector) );
        hys_mean.emplace_back(std::move(observable_mean_vector));
        hys_mean_std.emplace_back(std::move(observable_std_vector));
        hys_binder.emplace_back(std::move(binder_mean_vector));
        hys_binder_std.emplace_back(std::move(binder_std_vector));
        hys_autocorrelation_time.emplace_back(std::move(observable_autocorrelation_time_vector));

        integrated_pot_energy_check = total_integrated_pot_energy(
            lat, h, config.param_spec.mu, config.param_spec.J, lmbda
        );

        if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
            throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
        }
    }

    return Result{
        .series_hys=std::move(hys_vector), 
        .mean_hys=std::move(hys_mean), 
        .mean_std_hys=std::move(hys_mean_std), 
        .binder_hys=std::move(hys_binder), 
        .binder_std_hys=std::move(hys_binder_std), 
        .tau_int_hys=std::move(hys_autocorrelation_time)
    };
}

} // namespace paratoric 
