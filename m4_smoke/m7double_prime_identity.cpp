// M7'' (Feynman, replacing the marginal-distribution route): exact identity check.
// <R> = 1 for R = W_j(C_i)*W_i(C_j) / (W_i(C_i)*W_j(C_j)), C_i and C_j drawn
// INDEPENDENTLY from their own equilibrium distributions (swap acceptance is NEVER
// applied -- nothing is ever routed anywhere). See the doc comment on
// validate_swap_weight_ratio_identity_debug's declaration in extended_toric_code_qmc.hpp
// for the derivation. No MPI needed -- this is deliberately a single-process check.
//
// NOTE: <1/R> is NOT a second identity. R is symmetric under exchanging the two replicas, so
// there is exactly one identity here (see the corrected doc comment on the declaration). Do not
// add a <1/R> check back in.
//
// Build/run standalone (no MPI libs needed):
//   clang++ -std=gnu++23 -Iinclude -Isrc m4_smoke/m7double_prime_identity.cpp \
//     build-m0/libparatoric_core.a <link libs, see other drivers, minus MPI> \
//     -o m7double_prime_identity
//   ./m7double_prime_identity
// N_thermalization and N_between_samples are raw Metropolis-move counts in this
// driver. For the project's 25-sweep spacing at size L, pass N_between_samples
// = 25 * 2 * L * L; N_samples is the number of checkpoints.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

paratoric::LatSpec make_lat_spec(int system_size, double beta) {
    return paratoric::LatSpec{
        .basis = 'z', .lattice_type = "square", .system_size = system_size, .beta = beta,
        .boundaries = "open", .default_spin = 1, .lattice_backend = "boost"
    };
}

paratoric::Config make_config(int system_size, double beta, double s, double c,
                              std::int64_t N_thermalization, std::int64_t N_samples,
                              std::int64_t N_between_samples, int seed) {
    paratoric::ParamSpec param_spec{.mu = 1.0 * c, .h = s * c, .J = 1.0 * c, .lmbda = s * c};
    paratoric::SimSpec sim_spec{
        .N_samples = static_cast<int>(N_samples), .N_thermalization = N_thermalization,
        .N_between_samples = static_cast<int>(N_between_samples), .N_resamples = 200,
        .seed = seed, .observables = {}
    };
    return paratoric::Config{
        sim_spec, param_spec, make_lat_spec(system_size, beta),
        paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}
    };
}

void run_point(int system_size, double beta, double s, double c_i, double c_j,
               std::int64_t N_thermalization, std::int64_t N_samples,
               std::int64_t N_between_samples, int seed_i, int seed_j) {
    auto config_i = make_config(system_size, beta, s, c_i, N_thermalization,
                                N_samples, N_between_samples, seed_i); // rung A
    auto config_j = make_config(system_size, beta, s, c_j, N_thermalization,
                                N_samples, N_between_samples, seed_j); // rung B, independent seed

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.validate_swap_weight_ratio_identity_debug(config_i, config_j);

    const double R_dev = (result.R_mean - 1.0) / result.R_sem;

    std::printf("=== system_size=%d beta=%.2f s=%.6f c_i=%.6f c_j=%.6f ===\n",
        system_size, beta, s, c_i, c_j);
    std::printf("  n=%zu\n", result.n);
    std::printf("  <R>    = %.6f +- %.6f   deviation from 1: %.4f sigma\n",
        result.R_mean, result.R_sem, R_dev);
    std::printf("  <acceptance> = %.6f\n", result.acceptance_mean);
    std::printf("  log(R): mean=%.6f  stddev=%.6f\n", result.logR_mean, result.logR_stddev);
    std::printf("\n");
}

template <typename T>
T parse_argument(const char* value, const char* name);

template <>
std::int64_t parse_argument<std::int64_t>(const char* value, const char* name) {
    char* end = nullptr;
    const long long parsed = std::strtoll(value, &end, 10);
    if (end == value || *end != '\0') {
        throw std::invalid_argument(std::string("invalid ") + name + ": " + value);
    }
    return static_cast<std::int64_t>(parsed);
}

template <>
double parse_argument<double>(const char* value, const char* name) {
    char* end = nullptr;
    const double parsed = std::strtod(value, &end);
    if (end == value || *end != '\0') {
        throw std::invalid_argument(std::string("invalid ") + name + ": " + value);
    }
    return parsed;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 11) {
        std::fprintf(stderr,
            "usage: %s <system_size> <beta> <s> <c_i> <c_j> "
            "<N_thermalization> <N_samples> <N_between_samples> <seed_i> <seed_j>\n",
            argv[0]);
        return 2;
    }
    try {
        const auto system_size = static_cast<int>(parse_argument<std::int64_t>(argv[1], "system_size"));
        const auto beta = parse_argument<double>(argv[2], "beta");
        const auto s = parse_argument<double>(argv[3], "s");
        const auto c_i = parse_argument<double>(argv[4], "c_i");
        const auto c_j = parse_argument<double>(argv[5], "c_j");
        const auto N_thermalization = parse_argument<std::int64_t>(argv[6], "N_thermalization");
        const auto N_samples = parse_argument<std::int64_t>(argv[7], "N_samples");
        const auto N_between_samples = parse_argument<std::int64_t>(argv[8], "N_between_samples");
        const auto seed_i = static_cast<int>(parse_argument<std::int64_t>(argv[9], "seed_i"));
        const auto seed_j = static_cast<int>(parse_argument<std::int64_t>(argv[10], "seed_j"));
        if (system_size <= 0 || beta <= 0.0 || N_thermalization < 0 || N_samples <= 0
            || N_between_samples <= 0) {
            throw std::invalid_argument("system_size/beta must be positive; N_samples and N_between_samples must be positive; N_thermalization must be nonnegative");
        }
        run_point(system_size, beta, s, c_i, c_j, N_thermalization, N_samples,
                  N_between_samples, seed_i, seed_j);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "error: %s\n", error.what());
        return 2;
    }
    return 0;
}
