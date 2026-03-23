#pragma once

#include "lattice/lattice.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <format>
#include <limits>
#include <random>
#include <string>
#include <vector>

#ifdef PARATORIC_HAS_MPI
#include <mpi.h>
#endif

namespace paratoric::pt_detail {

struct PTContext {
    bool enabled = false;
    int rank = 0;
    int world_size = 1;
    std::string parameter{};
    double tempered_value = 0.0;
    int tempered_index = 0;
    std::vector<double> ladder_values{};
};

inline std::string to_lower_copy(std::string text) {
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
    );
    return text;
}

inline bool uses_log_ratio_exchange(const PTContext& ctx) {
    return ctx.parameter == "h" || ctx.parameter == "mu";
}

inline int odd_even_partner(int rank, int world_size, int phase) {
    int partner = -1;
    if (phase == 0) {
        if (rank % 2 == 0 && rank + 1 < world_size) {
            partner = rank + 1;
        } else if (rank % 2 == 1) {
            partner = rank - 1;
        }
    } else {
        if (rank % 2 == 1 && rank + 1 < world_size) {
            partner = rank + 1;
        } else if (rank % 2 == 0) {
            partner = rank - 1;
        }
    }
    if (partner < 0 || partner >= world_size) {
        return -1;
    }
    return partner;
}

inline std::filesystem::path swap_trace_file_path(const Config& config) {
    const auto parent = config.out_spec.path_out.parent_path();
    if (!parent.empty()) {
        return parent / "output_test.txt";
    }
    return config.out_spec.path_out / "output_test.txt";
}

inline std::filesystem::path binned_observable_file_path(const Config& config) {
    const auto parent = config.out_spec.path_out.parent_path();
    if (!parent.empty()) {
        return parent / "pt_observables_by_ladder.tsv";
    }
    return config.out_spec.path_out / "pt_observables_by_ladder.tsv";
}

inline std::filesystem::path ladder_visit_file_path(const Config& config) {
    const auto parent = config.out_spec.path_out.parent_path();
    if (!parent.empty()) {
        return parent / "pt_ladder_visits.tsv";
    }
    return config.out_spec.path_out / "pt_ladder_visits.tsv";
}

inline std::filesystem::path round_trip_file_path(const Config& config) {
    const auto parent = config.out_spec.path_out.parent_path();
    if (!parent.empty()) {
        return parent / "pt_round_trips.tsv";
    }
    return config.out_spec.path_out / "pt_round_trips.tsv";
}

inline std::filesystem::path feedback_diag_file_path(const Config& config) {
    const auto parent = config.out_spec.path_out.parent_path();
    if (!parent.empty()) {
        return parent / "pt_feedback_diagnostics.tsv";
    }
    return config.out_spec.path_out / "pt_feedback_diagnostics.tsv";
}

inline double& select_tempered_parameter(
    const PTContext& ctx,
    double& h,
    double& mu,
    double& J,
    double& lmbda
) {
    if (ctx.parameter == "h") {
        return h;
    }
    if (ctx.parameter == "mu") {
        return mu;
    }
    if (ctx.parameter == "j") {
        return J;
    }
    return lmbda;
}

inline PTContext init_context(const Config& config) {
    PTContext ctx{};
    if (!config.pt_spec.enabled) {
        return ctx;
    }

    if (config.lat_spec.basis != 'z') {
        throw std::invalid_argument("PT currently supports basis=z only.");
    }
    if (config.pt_spec.replicas < 2) {
        throw std::invalid_argument("pt_replicas must be >= 2 when PT is enabled.");
    }
    if (config.pt_spec.swap_period < 1) {
        throw std::invalid_argument("pt_swap_period must be >= 1 when PT is enabled.");
    }
    if (config.pt_spec.ladder_values.empty()
        && !(config.pt_spec.ladder_max > config.pt_spec.ladder_min)) {
        throw std::invalid_argument("pt_ladder_max must be larger than pt_ladder_min when PT is enabled.");
    }

    ctx.parameter = to_lower_copy(config.pt_spec.parameter);
    if (ctx.parameter == "lambda") {
        ctx.parameter = "lmbda";
    }
    if (ctx.parameter != "h" && ctx.parameter != "mu"
        && ctx.parameter != "j" && ctx.parameter != "lmbda") {
        throw std::invalid_argument(
            std::format("Unsupported pt_parameter '{}'. Supported in z-basis PT: h, mu, J, lmbda.",
                        config.pt_spec.parameter));
    }

    const bool requires_non_negative_ladder = ctx.parameter == "h" || ctx.parameter == "mu";
    if (requires_non_negative_ladder) {
        if (!config.pt_spec.ladder_values.empty()) {
            for (double value : config.pt_spec.ladder_values) {
                if (value < 0.0) {
                    throw std::invalid_argument(
                        std::format("All pt_ladder_values must be >= 0 when pt_parameter={}.",
                                    config.pt_spec.parameter)
                    );
                }
            }
        } else if (config.pt_spec.ladder_min < 0.0) {
            throw std::invalid_argument(
                std::format("pt_ladder_min must be >= 0 when pt_parameter={}.", config.pt_spec.parameter)
            );
        }
    }

#ifdef PARATORIC_HAS_MPI
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized) {
        throw std::runtime_error("PT requested but MPI is not initialized.");
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &ctx.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ctx.world_size);

    if (ctx.world_size != config.pt_spec.replicas) {
        throw std::invalid_argument(
            std::format("pt_replicas ({}) must match MPI world size ({}).",
                        config.pt_spec.replicas, ctx.world_size));
    }

    if (!config.pt_spec.ladder_values.empty()) {
        if (static_cast<int>(config.pt_spec.ladder_values.size()) != ctx.world_size) {
            throw std::invalid_argument(
                std::format("pt_ladder_values length ({}) must match MPI world size ({}).",
                            config.pt_spec.ladder_values.size(), ctx.world_size));
        }
        ctx.ladder_values = config.pt_spec.ladder_values;
        for (int i = 1; i < ctx.world_size; ++i) {
            if (!(ctx.ladder_values[static_cast<size_t>(i)]
                  > ctx.ladder_values[static_cast<size_t>(i - 1)])) {
                throw std::invalid_argument("pt_ladder_values must be strictly increasing.");
            }
        }
    } else {
        ctx.ladder_values.resize(static_cast<size_t>(ctx.world_size), config.pt_spec.ladder_min);
        for (int i = 0; i < ctx.world_size; ++i) {
            const double fraction = static_cast<double>(i)
                                  / static_cast<double>(ctx.world_size - 1);
            ctx.ladder_values[static_cast<size_t>(i)] =
                config.pt_spec.ladder_min
                + fraction * (config.pt_spec.ladder_max - config.pt_spec.ladder_min);
        }
    }
    ctx.tempered_index = ctx.rank;
    ctx.tempered_value = ctx.ladder_values[static_cast<size_t>(ctx.tempered_index)];
#else
    throw std::invalid_argument("PT requested but ParaToric was built without MPI support.");
#endif

    ctx.enabled = true;
    return ctx;
}

inline void apply_tempered_parameter(
    const PTContext& ctx,
    double& h,
    double& mu,
    double& J,
    double& lmbda
) {
    if (!ctx.enabled) {
        return;
    }
    select_tempered_parameter(ctx, h, mu, J, lmbda) = ctx.tempered_value;
}

inline double exchange_statistic_zbasis(
    const PTContext& ctx,
    Lattice& lat
) {
    if (ctx.parameter == "h") {
        return lat.get_non_diag_single_energy_z();
    }
    if (ctx.parameter == "mu") {
        return lat.get_non_diag_tuple_energy_z();
    }
    if (ctx.parameter == "j") {
        return lat.total_integrated_plaquette_energy();
    }
    return lat.total_integrated_edge_energy();
}

inline void initialize_trace_file_if_requested(const Config& config, const PTContext& ctx) {
#ifdef PARATORIC_HAS_MPI
    if (!ctx.enabled || ctx.rank != 0) {
        return;
    }
    if (!(config.pt_spec.trace_enabled && config.pt_spec.trace_interval > 0)) {
        return;
    }

    std::ofstream trace_file(swap_trace_file_path(config), std::ios::out | std::ios::trunc);
    if (trace_file) {
        trace_file << "# PT swap trace\n";
        trace_file << "# fields: step phase pair accepted parameter_before statistic logR parameter_after\n";
    }
    MPI_Barrier(MPI_COMM_WORLD);
#else
    UNUSED(config);
    UNUSED(ctx);
#endif
}

inline void maybe_attempt_swap(
    const Config& config,
    PTContext& ctx,
    Lattice& lat,
    rng::RNG& rng_engine,
    int total_metropolis_step_count,
    double& h_runtime,
    double& mu_runtime,
    double& J_runtime,
    double& lmbda_runtime,
    double& integrated_pot_energy,
    std::uint64_t& swap_attempted_local,
    std::uint64_t& swap_accepted_local,
    std::uint64_t& swap_rejected_local
) {
    if (!ctx.enabled) {
        return;
    }
    if (config.lat_spec.basis != 'z') {
        throw std::invalid_argument("PT swaps are currently implemented only for basis=z.");
    }
    if (total_metropolis_step_count <= 0
        || (total_metropolis_step_count % config.pt_spec.swap_period) != 0) {
        return;
    }

    const int phase = (total_metropolis_step_count / config.pt_spec.swap_period) & 1;
    const int partner = odd_even_partner(ctx.rank, ctx.world_size, phase);

#ifdef PARATORIC_HAS_MPI
    int accepted = -1;
    double& local_parameter_runtime = select_tempered_parameter(ctx, h_runtime, mu_runtime, J_runtime, lmbda_runtime);
    const double local_parameter_before = local_parameter_runtime;
    double partner_parameter_before = std::numeric_limits<double>::quiet_NaN();
    const int local_index_before = ctx.tempered_index;
    int partner_index_before = -1;
    const double local_statistic = exchange_statistic_zbasis(ctx, lat);
    double partner_statistic = std::numeric_limits<double>::quiet_NaN();
    double log_r = std::numeric_limits<double>::quiet_NaN();

    if (partner >= 0) {
        constexpr int kTagPtParameter = 4901;
        MPI_Sendrecv(
            &local_parameter_before, 1, MPI_DOUBLE, partner, kTagPtParameter,
            &partner_parameter_before, 1, MPI_DOUBLE, partner, kTagPtParameter,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );
        constexpr int kTagPtIndex = 4903;
        MPI_Sendrecv(
            &local_index_before, 1, MPI_INT, partner, kTagPtIndex,
            &partner_index_before, 1, MPI_INT, partner, kTagPtIndex,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );

        constexpr int kTagPtStatistic = 4902;
        MPI_Sendrecv(
            &local_statistic, 1, MPI_DOUBLE, partner, kTagPtStatistic,
            &partner_statistic, 1, MPI_DOUBLE, partner, kTagPtStatistic,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE
        );

        if (local_parameter_before == partner_parameter_before) {
            log_r = 0.0;
        } else if (uses_log_ratio_exchange(ctx)
                   && (local_parameter_before == 0.0 || partner_parameter_before == 0.0)) {
            constexpr double kLogRCap = 1e6;
            if (local_statistic == partner_statistic) {
                log_r = 0.0;
            } else if (local_parameter_before == 0.0) {
                log_r = (local_statistic > partner_statistic) ? kLogRCap : -kLogRCap;
            } else {
                log_r = (local_statistic < partner_statistic) ? kLogRCap : -kLogRCap;
            }
        } else if (uses_log_ratio_exchange(ctx)) {
            log_r = config.lat_spec.beta
                  * (local_statistic - partner_statistic)
                  * std::log(partner_parameter_before / local_parameter_before);
        } else {
            log_r = (partner_parameter_before - local_parameter_before)
                  * (local_statistic - partner_statistic);
        }

        accepted = 0;
        if (ctx.rank < partner) {
            ++swap_attempted_local;
            std::uniform_real_distribution<double> uniform_dist{0.0, 1.0};
            const double random_uniform = uniform_dist(rng_engine);
            const double log_uniform = std::log(std::max(random_uniform, std::numeric_limits<double>::min()));
            if (log_r >= 0.0 || log_uniform < log_r) {
                accepted = 1;
            }
            if (accepted == 1) {
                ++swap_accepted_local;
            } else {
                ++swap_rejected_local;
            }
        }
        const int decision_root = std::min(ctx.rank, partner);
        MPI_Bcast(&accepted, 1, MPI_INT, decision_root, MPI_COMM_WORLD);

        if (accepted == 1) {
            if (ctx.parameter == "j") {
                integrated_pot_energy += -(partner_parameter_before - local_parameter_before) * local_statistic;
            } else if (ctx.parameter == "lmbda") {
                integrated_pot_energy += -(partner_parameter_before - local_parameter_before) * local_statistic;
            }

            local_parameter_runtime = partner_parameter_before;
            ctx.tempered_value = local_parameter_runtime;
            ctx.tempered_index = partner_index_before;
        }
    }

    const bool emit_trace = config.pt_spec.trace_enabled
        && config.pt_spec.trace_interval > 0
        && (total_metropolis_step_count % config.pt_spec.trace_interval) == 0;
    if (emit_trace) {
        const double local_parameter_after = local_parameter_runtime;

        std::vector<int> gathered_partners;
        std::vector<int> gathered_accepted;
        std::vector<double> gathered_parameter_before;
        std::vector<double> gathered_parameter_after;
        std::vector<double> gathered_statistic;
        std::vector<double> gathered_log_r;
        if (ctx.rank == 0) {
            gathered_partners.resize(static_cast<size_t>(ctx.world_size), -1);
            gathered_accepted.resize(static_cast<size_t>(ctx.world_size), -1);
            gathered_parameter_before.resize(static_cast<size_t>(ctx.world_size), std::numeric_limits<double>::quiet_NaN());
            gathered_parameter_after.resize(static_cast<size_t>(ctx.world_size), std::numeric_limits<double>::quiet_NaN());
            gathered_statistic.resize(static_cast<size_t>(ctx.world_size), std::numeric_limits<double>::quiet_NaN());
            gathered_log_r.resize(static_cast<size_t>(ctx.world_size), std::numeric_limits<double>::quiet_NaN());
        }
        MPI_Gather(&partner, 1, MPI_INT,
                   ctx.rank == 0 ? gathered_partners.data() : nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&accepted, 1, MPI_INT,
                   ctx.rank == 0 ? gathered_accepted.data() : nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(&local_parameter_before, 1, MPI_DOUBLE,
                   ctx.rank == 0 ? gathered_parameter_before.data() : nullptr, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&local_parameter_after, 1, MPI_DOUBLE,
                   ctx.rank == 0 ? gathered_parameter_after.data() : nullptr, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&local_statistic, 1, MPI_DOUBLE,
                   ctx.rank == 0 ? gathered_statistic.data() : nullptr, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&log_r, 1, MPI_DOUBLE,
                   ctx.rank == 0 ? gathered_log_r.data() : nullptr, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if (ctx.rank == 0) {
            std::ofstream trace_file(swap_trace_file_path(config), std::ios::app);
            if (trace_file) {
                trace_file << "step=" << total_metropolis_step_count
                           << " phase=" << phase
                           << " parameter=" << ctx.parameter;
                bool has_pairs = false;
                for (int r = 0; r < ctx.world_size; ++r) {
                    const int p = gathered_partners[static_cast<size_t>(r)];
                    if (p >= 0 && r < p) {
                        has_pairs = true;
                        trace_file << " pair=(" << r << "<->" << p << ")"
                                   << " accepted=" << gathered_accepted[static_cast<size_t>(r)]
                                   << " parameter_before=("
                                   << gathered_parameter_before[static_cast<size_t>(r)] << ","
                                   << gathered_parameter_before[static_cast<size_t>(p)] << ")"
                                   << " statistic=("
                                   << gathered_statistic[static_cast<size_t>(r)] << ","
                                   << gathered_statistic[static_cast<size_t>(p)] << ")"
                                   << " logR=" << gathered_log_r[static_cast<size_t>(r)]
                                   << " parameter_after=("
                                   << gathered_parameter_after[static_cast<size_t>(r)] << ","
                                   << gathered_parameter_after[static_cast<size_t>(p)] << ")";
                    }
                }
                if (!has_pairs) {
                    trace_file << " pairs=none";
                }
                trace_file << '\n';
            }
        }
    }

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format(
        "[PT] step={} phase={} rank={} partner={} {}={} swap_attempted_local={} swap_accepted_local={} swap_rejected_local={}",
        total_metropolis_step_count,
        phase,
        ctx.rank,
        partner,
        ctx.parameter,
        local_parameter_runtime,
        swap_attempted_local,
        swap_accepted_local,
        swap_rejected_local
    );
#endif
#else
    UNUSED(config);
    UNUSED(ctx);
    UNUSED(lat);
    UNUSED(rng_engine);
    UNUSED(total_metropolis_step_count);
    UNUSED(h_runtime);
    UNUSED(mu_runtime);
    UNUSED(J_runtime);
    UNUSED(lmbda_runtime);
    UNUSED(integrated_pot_energy);
    UNUSED(swap_attempted_local);
    UNUSED(swap_accepted_local);
    UNUSED(swap_rejected_local);
#endif
}

} // namespace paratoric::pt_detail
