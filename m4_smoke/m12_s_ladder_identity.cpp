// M12 (Feynman): the identity gate for a ladder in s at FIXED beta, named at channel
// entry 2026-08-28g items 8 and 9 and re-routed to the cluster by entry 2026-08-28h.
//
// Every ladder this project has run before moves in the COMMON FACTOR: rung k carries
// c^k on all four couplings, which is the beta direction. M10 validates that ladder.
// It says nothing about this one. Here beta is FIXED, mu = J = 1 on every rung, and
// only h and lmbda move, so the swap weight uses exactly TWO of its four terms. The
// mu and J terms are identically absent, not merely small.
//
// The identity <R> = 1 holds only while each replica stays on its own rung, so this
// driver runs with pt_spec.never_accept_swaps. Every swap is still ATTEMPTED and every
// weight is still assembled from genuinely MPI-transported partner data. The identity
// is accumulated PER LINK, so one bad link cannot hide inside a healthy average
// (memory feedback_specs_name_evidence_not_verdicts).
//
// Power: break=1 sets pt_spec.break_lmbda_term_for_test, which drops the lmbda
// contribution from the swap weight and keeps the h contribution. That is ONE of the
// two live terms. This driver must FAIL under that flag, otherwise the gate has no
// power over the s direction (memory feedback_measure_check_power).
//
//   mpirun -np 7 ./m12_s_ladder_identity 16 16.0 0.333 1.035572 2000 20000 800 4242 0
//   mpirun -np 7 ./m12_s_ladder_identity 16 16.0 0.333 1.035572 2000 20000 800 4242 1
//
// Arguments: system_size beta s_center ratio N_thermalization N_samples
//            N_between_samples seed break_mode
// The ladder is GEOMETRIC and centred so that s_center falls exactly on the middle
// rung: rung k carries s = s_center * ratio^(k - world_size/2), h = lmbda = s,
// mu = J = 1. The rung count is the rank count and must be ODD.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <mpi.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    int world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (argc != 10) {
        if (rank == 0) {
            std::fprintf(stderr,
                "usage: mpirun -np <ranks> %s <system_size> <beta> <s_center> <ratio> "
                "<N_thermalization> <N_samples> <N_between_samples> <seed> <break_mode>\n",
                argv[0]);
        }
        MPI_Finalize();
        return 2;
    }

    const int system_size = std::atoi(argv[1]);
    const double beta = std::atof(argv[2]);
    const double s_center = std::atof(argv[3]);
    const double ratio = std::atof(argv[4]);
    const int N_thermalization = std::atoi(argv[5]);
    const int N_samples = std::atoi(argv[6]);
    const int N_between_samples = std::atoi(argv[7]);
    const int seed = std::atoi(argv[8]);
    // break: 0 = none, 1 = drop the lmbda term and keep the h term.
    const int break_mode = std::atoi(argv[9]);

    if (world_size < 3 || world_size % 2 == 0) {
        if (rank == 0) {
            std::fprintf(stderr, "rank count must be odd and at least 3 so s_center falls on a rung; got %d\n",
                         world_size);
        }
        MPI_Finalize();
        return 2;
    }

    // Rungs are built so that s_center sits EXACTLY on the middle rung. The ladder is
    // geometric, which is the project's standing rule: even spacing puts a bottleneck
    // at one end and that is the one failure mode that destroys the transport law.
    const int centre = world_size / 2;
    std::vector<double> ladder_s(static_cast<size_t>(world_size));
    std::vector<double> ladder_lmbda(static_cast<size_t>(world_size));
    for (int k = 0; k < world_size; ++k) {
        const double s_k = s_center * std::pow(ratio, k - centre);
        ladder_s[static_cast<size_t>(k)] = s_k;
        ladder_lmbda[static_cast<size_t>(k)] = s_k;
    }
    // ladder_mu and ladder_J are left EMPTY on purpose. Empty means mu and J are
    // untempered and are taken from param_spec on every rung, which is exactly the
    // "mu = J = 1 on every rung" the directive requires.

    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = system_size, .beta = beta,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = s_center, .J = 1.0, .lmbda = s_center};
    // The identity <R> = 1 averages over the PRODUCT of the two rungs equilibrium
    // distributions, so neighbouring replicas must be INDEPENDENT. Without a rank
    // offset every rank runs the same RNG stream from the same start configuration,
    // and correlated partners break the assumption the identity rests on.
    paratoric::SimSpec sim_spec{
        .N_samples = N_samples, .N_thermalization = N_thermalization,
        .N_between_samples = N_between_samples, .N_resamples = 4,
        .seed = seed + rank, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = ladder_s,
        .ladder_lmbda = ladder_lmbda,
        // R must be sampled from EQUILIBRIUM. Successive attempts a few moves apart are
        // the same configuration seen twice, which does not bias the weight but destroys
        // the error bar and the effective sample count. The project's spacing is 25
        // sweeps, that is 25 * 2 * L * L raw Metropolis moves.
        .swap_period = static_cast<int>(25 * 2 * system_size * system_size),
        .validate_swap_weight = true,
        .never_accept_swaps = true,
        .break_lmbda_term_for_test = (break_mode == 1)
    };
    paratoric::Config config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec};

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.get_sample(config);

    const size_t link_count = static_cast<size_t>(world_size - 1);
    std::vector<unsigned long long> local_count(link_count, 0);
    for (size_t k = 0; k < link_count; ++k) {
        local_count[k] = static_cast<unsigned long long>(result.pt_diagnostics.R_count_by_link[k]);
    }
    std::vector<unsigned long long> total_count(link_count, 0);
    std::vector<double> total_sum(link_count, 0.);
    std::vector<double> total_sumsq(link_count, 0.);
    MPI_Reduce(local_count.data(), total_count.data(), static_cast<int>(link_count),
               MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(result.pt_diagnostics.R_sum_by_link.data(), total_sum.data(),
               static_cast<int>(link_count), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(result.pt_diagnostics.R_sumsq_by_link.data(), total_sumsq.data(),
               static_cast<int>(link_count), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    unsigned long long local_accepted = result.pt_diagnostics.swap_accepted;
    unsigned long long total_accepted = 0;
    MPI_Reduce(&local_accepted, &total_accepted, 1, MPI_UNSIGNED_LONG_LONG,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("ranks=%d links=%zu system_size=%d beta=%.4f s_center=%.6f ratio=%.6f break=%d seed=%d "
                    "N_samples=%d N_between_samples=%d N_thermalization=%d\n",
            world_size, link_count, system_size, beta, s_center, ratio, break_mode, seed,
            N_samples, N_between_samples, N_thermalization);
        std::printf("%-6s %-14s %-14s %-8s %-8s\n", "rung", "s", "lmbda", "mu", "J");
        for (int k = 0; k < world_size; ++k) {
            std::printf("%-6d %-14.6f %-14.6f %-8.4f %-8.4f\n",
                k, ladder_s[static_cast<size_t>(k)], ladder_lmbda[static_cast<size_t>(k)],
                param_spec.mu, param_spec.J);
        }
        std::printf("max_swap_balance_err=%.17g max_shared_function_deviation=%.17g "
                    "max_swap_log_weight_deviation=%.17g\n",
            result.pt_diagnostics.max_swap_balance_err,
            result.pt_diagnostics.max_shared_function_deviation,
            result.pt_diagnostics.max_swap_log_weight_deviation);
        std::printf("swap_accepted_total=%llu (must be 0 in identity mode)\n", total_accepted);
        std::printf("%-6s %-10s %-16s %-16s %-12s\n", "link", "n", "<R>", "sem", "sigma");
        int worst_link = -1;
        double worst_sigma = 0.;
        int links_over_3sigma = 0;
        for (size_t k = 0; k < link_count; ++k) {
            const double n = static_cast<double>(total_count[k]);
            if (n < 2.) {
                std::printf("%-6zu %-10llu %-16s %-16s %-12s\n", k, total_count[k], "n/a", "n/a", "n/a");
                continue;
            }
            const double mean = total_sum[k] / n;
            const double var = (total_sumsq[k] / n - mean * mean) * n / (n - 1.);
            const double sem = std::sqrt(var / n);
            const double sigma = (sem > 0.) ? (mean - 1.) / sem : 0.;
            std::printf("%-6zu %-10llu %-16.8f %-16.8f %-12.4f\n", k, total_count[k], mean, sem, sigma);
            if (std::abs(sigma) > 3.) { ++links_over_3sigma; }
            if (std::abs(sigma) > std::abs(worst_sigma)) {
                worst_sigma = sigma;
                worst_link = static_cast<int>(k);
            }
        }
        std::printf("worst_link=%d worst_sigma=%.4f links_over_3sigma=%d of %zu\n",
            worst_link, worst_sigma, links_over_3sigma, link_count);
    }

    MPI_Finalize();
    return 0;
}
