// M4 smoke driver, not part of the checked-in test suite: exercises the REAL
// inline PT swap blocks (get_thermalization / get_sample) under actual MPI ranks,
// via the public API directly (no CLI flags needed for ladder_mu/ladder_J since
// those are not wired to the CLI yet -- that's M5). Local run only, no cluster:
//   mpirun -np 2 ./pt_balance_check_smoke
// Prints pt_diagnostics.max_swap_balance_err from rank 0.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <mpi.h>
#include <cstdio>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec{
        .N_samples = 4, .N_thermalization = 400, .N_between_samples = 4,
        .N_resamples = 4, .seed = 909090, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        // M8: common factor c=1.3 on all four couplings, so the cross-check exercises
        // every term (h, mu, lmbda, J), not just mu.
        .ladder_s = {0.20, 0.26},
        .ladder_lmbda = {0.25, 0.325},
        .ladder_mu = {1.0, 1.3},
        .ladder_J = {1.0, 1.3},
        .swap_period = 4,
        .validate_swap_weight = true
    };
    paratoric::Config config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec};

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.get_sample(config);

    if (rank == 0) {
        std::printf("max_swap_balance_err=%.17g\n", result.pt_diagnostics.max_swap_balance_err);
        std::printf("max_swap_log_weight_deviation=%.17g\n", result.pt_diagnostics.max_swap_log_weight_deviation);
        std::printf("max_shared_function_deviation=%.17g\n", result.pt_diagnostics.max_shared_function_deviation);
        std::printf("swap_attempted=%llu swap_accepted=%llu\n",
            static_cast<unsigned long long>(result.pt_diagnostics.swap_attempted),
            static_cast<unsigned long long>(result.pt_diagnostics.swap_accepted));
    }

    MPI_Finalize();
    return 0;
}
