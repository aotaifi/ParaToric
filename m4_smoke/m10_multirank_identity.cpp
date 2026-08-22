// M10 (Feynman): the multi-rank identity test named at channel entry 2026-08-19h
// item 7. Every swap-weight check built before this one ran on a SINGLE PAIR:
// the M4 balance check re-reads the same received values in both directions and
// so cannot see mis-transported data, and the M7'' identity ran in one process on
// two configurations. Neither has any content once the ladder is wider than two
// rungs.
//
// What this driver does: it runs the REAL ladder under real MPI ranks with swap
// ACCEPTANCE DISABLED (pt_spec.never_accept_swaps). Every swap is still attempted
// and every weight is still assembled from genuinely MPI-transported partner
// data, but no replica ever leaves its rung, which is the condition under which
// the exact identity <R> = 1 holds. The identity is then accumulated PER LINK and
// printed as a per-link table, so a single bad link cannot hide inside a healthy
// average (memory feedback_specs_name_evidence_not_verdicts).
//
// Power: pass break=1 to turn on pt_spec.break_transport_for_test, which
// exchanges the roles of the received partner_edge and partner_plaquette. That is
// exactly the mis-transport the M4 balance check is blind to. This driver must
// FAIL under that flag, otherwise it has no power (memory
// feedback_measure_check_power).
//
//   mpirun -np 3 ./m10_multirank_identity 6 6.0 0.39 1.15 200000 400 2000 4242 0
//   mpirun -np 5 ./m10_multirank_identity 6 6.0 0.39 1.15 200000 400 2000 4242 0
//   mpirun -np 3 ./m10_multirank_identity 6 6.0 0.39 1.15 200000 400 2000 4242 1
//   mpirun -np 3 ./m10_multirank_identity 6 6.0 0.39 1.15 200000 400 2000 4242 2
//
// Arguments: system_size beta s_base c_step N_thermalization N_samples
//            N_between_samples seed break_mode
// The ladder is the coupled (beta,s) ladder of M1: rung k carries the common
// factor c^k on all four couplings, mu = c^k, h = s_base*c^k, J = c^k,
// lmbda = s_base*c^k.

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
                "usage: mpirun -np <ranks> %s <system_size> <beta> <s_base> <c_step> "
                "<N_thermalization> <N_samples> <N_between_samples> <seed> <break_mode>\n",
                argv[0]);
        }
        MPI_Finalize();
        return 2;
    }

    const int system_size = std::atoi(argv[1]);
    const double beta = std::atof(argv[2]);
    const double s_base = std::atof(argv[3]);
    const double c_step = std::atof(argv[4]);
    const int N_thermalization = std::atoi(argv[5]);
    const int N_samples = std::atoi(argv[6]);
    const int N_between_samples = std::atoi(argv[7]);
    const int seed = std::atoi(argv[8]);
    // break: 0 = none, 1 = mis-transport (partner_edge/partner_plaquette exchanged),
    // 2 = mis-routing (a wrong ladder index is sent, so the routing guard must throw).
    const int break_mode = std::atoi(argv[9]);
    const bool break_transport = break_mode == 1;
    const bool break_routing = break_mode == 2;

    std::vector<double> ladder_s(static_cast<size_t>(world_size));
    std::vector<double> ladder_lmbda(static_cast<size_t>(world_size));
    std::vector<double> ladder_mu(static_cast<size_t>(world_size));
    std::vector<double> ladder_J(static_cast<size_t>(world_size));
    for (int k = 0; k < world_size; ++k) {
        const double c = std::pow(c_step, k);
        ladder_mu[static_cast<size_t>(k)] = c;
        ladder_J[static_cast<size_t>(k)] = c;
        ladder_s[static_cast<size_t>(k)] = s_base * c;
        ladder_lmbda[static_cast<size_t>(k)] = s_base * c;
    }

    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = system_size, .beta = beta,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = s_base, .J = 1.0, .lmbda = s_base};
    paratoric::SimSpec sim_spec{
        .N_samples = N_samples, .N_thermalization = N_thermalization,
        .N_between_samples = N_between_samples, .N_resamples = 4,
        .seed = seed, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = ladder_s,
        .ladder_lmbda = ladder_lmbda,
        .ladder_mu = ladder_mu,
        .ladder_J = ladder_J,
        // R must be sampled from EQUILIBRIUM, and successive attempts a few moves
        // apart are the same configuration seen twice. The project's spacing is 25
        // sweeps, that is 25 * 2 * L * L raw Metropolis moves, so the swap period is
        // set to that here rather than to a small constant. A short period does not
        // bias the weight, it destroys the error bar and the effective sample count.
        .swap_period = static_cast<int>(25 * 2 * system_size * system_size),
        .validate_swap_weight = true,
        .never_accept_swaps = true,
        .break_transport_for_test = break_transport,
        .break_routing_for_test = break_routing
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
        std::printf("ranks=%d links=%zu system_size=%d beta=%.4f s_base=%.6f c_step=%.6f break=%d\n",
            world_size, link_count, system_size, beta, s_base, c_step, break_mode);
        // Printed so the claim that the M4 balance check is BLIND to a mis-transport
        // is a measured number here, not an assertion in a comment.
        std::printf("max_swap_balance_err=%.17g max_shared_function_deviation=%.17g "
                    "max_swap_log_weight_deviation=%.17g\n",
            result.pt_diagnostics.max_swap_balance_err,
            result.pt_diagnostics.max_shared_function_deviation,
            result.pt_diagnostics.max_swap_log_weight_deviation);
        std::printf("swap_accepted_total=%llu (must be 0 in identity mode)\n", total_accepted);
        std::printf("%-6s %-10s %-16s %-16s %-12s\n", "link", "n", "<R>", "sem", "sigma");
        int worst_link = -1;
        double worst_sigma = 0.;
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
            if (std::abs(sigma) > std::abs(worst_sigma)) {
                worst_sigma = sigma;
                worst_link = static_cast<int>(k);
            }
        }
        std::printf("worst_link=%d worst_sigma=%.4f\n", worst_link, worst_sigma);
    }

    MPI_Finalize();
    return 0;
}
