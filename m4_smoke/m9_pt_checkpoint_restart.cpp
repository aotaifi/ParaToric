// M9 requirement 5 (Feynman): a REAL multi-rank PT job, local, no cluster. Lets
// swaps move replicas off their home rungs (swap_period=1, common factor c=1.3, the
// settings established as giving healthy acceptance in M7'/M7''), checkpoints,
// restarts, and confirms pt_ladder_index and all four runtime couplings come back
// exactly as they were when the checkpoint was written -- not re-derived from the
// MPI rank (the bug this milestone fixes).
//
// Each rank checkpoints to its OWN file (checkpoint_path/restart_from are per-rank,
// suffixed by rank) -- the library has no rank-awareness of its own for this, so the
// caller must give each rank a distinct file, exactly as a real multi-rank production
// script would need to.
//
// mpirun -np 2 ./m9_pt_checkpoint_restart

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <mpi.h>
#include <cstdio>
#include <string>

namespace {

paratoric::Config make_config(int rank, int N_samples, bool restart, const std::string& ckpt_path) {
    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 3, .beta = 0.5,
        .boundaries = "open", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec{
        .N_samples = N_samples, .N_thermalization = 2000, .N_between_samples = 8,
        .N_resamples = 20, .seed = 700000 + rank, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = {0.20, 0.26},
        .ladder_lmbda = {0.25, 0.325},
        .ladder_mu = {1.0, 1.3},
        .ladder_J = {1.0, 1.3},
        .swap_period = 1,
        .validate_swap_weight = true
    };
    paratoric::OutSpec out_spec{};
    if (restart) {
        out_spec.restart_from = ckpt_path;
    } else {
        out_spec.checkpoint_path = ckpt_path;
        out_spec.checkpoint_every_samples = N_samples; // one checkpoint, at the end of this prefix run
    }
    return paratoric::Config{sim_spec, param_spec, lat_spec, out_spec, pt_spec};
}

} // namespace

int main() {
    MPI_Init(nullptr, nullptr);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const std::string ckpt_path = "/var/tmp/a.otaifi/m9_checkpoint_test/pt_checkpoint_rank" + std::to_string(rank) + ".bin";

    // Prefix run: 53 samples, checkpoint once at the end.
    auto prefix_config = make_config(rank, 53, false, ckpt_path);
    auto mc_prefix = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto prefix = mc_prefix.get_sample(prefix_config);

    std::printf("[rank %d] BEFORE checkpoint (prefix run final state): "
        "ladder_index=%d runtime_h=%.10f runtime_mu=%.10f runtime_J=%.10f runtime_lmbda=%.10f "
        "swap_attempted=%llu swap_accepted=%llu\n",
        rank, prefix.pt_diagnostics.final_ladder_index,
        prefix.pt_diagnostics.final_s, prefix.pt_diagnostics.final_mu,
        prefix.pt_diagnostics.final_J, prefix.pt_diagnostics.final_lmbda,
        static_cast<unsigned long long>(prefix.pt_diagnostics.swap_attempted),
        static_cast<unsigned long long>(prefix.pt_diagnostics.swap_accepted));

    MPI_Barrier(MPI_COMM_WORLD);

    // Restart: same total sample budget continues (only 1 more sample here -- the
    // point is the FIRST moment after restart, i.e. initial_ladder_index/initial_*).
    auto restart_config = make_config(rank, 54, true, ckpt_path);
    auto mc_restart = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto resumed = mc_restart.get_sample(restart_config);

    std::printf("[rank %d] AFTER restart (initial state read back from checkpoint): "
        "ladder_index=%d runtime_h=%.10f runtime_mu=%.10f runtime_J=%.10f runtime_lmbda=%.10f\n",
        rank, resumed.pt_diagnostics.initial_ladder_index, resumed.pt_diagnostics.initial_s,
        resumed.pt_diagnostics.initial_mu, resumed.pt_diagnostics.initial_J, resumed.pt_diagnostics.initial_lmbda);

    const bool ladder_index_matches =
        resumed.pt_diagnostics.initial_ladder_index == prefix.pt_diagnostics.final_ladder_index;
    const bool h_matches = resumed.pt_diagnostics.initial_s == prefix.pt_diagnostics.final_s;
    const bool mu_matches = resumed.pt_diagnostics.initial_mu == prefix.pt_diagnostics.final_mu;
    const bool J_matches = resumed.pt_diagnostics.initial_J == prefix.pt_diagnostics.final_J;
    const bool lmbda_matches = resumed.pt_diagnostics.initial_lmbda == prefix.pt_diagnostics.final_lmbda;
    std::printf("[rank %d] ladder_index_matches=%s h_matches=%s mu_matches=%s J_matches=%s lmbda_matches=%s\n",
        rank, ladder_index_matches ? "true" : "false", h_matches ? "true" : "false",
        mu_matches ? "true" : "false", J_matches ? "true" : "false", lmbda_matches ? "true" : "false");
    const bool ladder_index_h_mu_J_lmbda_all_match =
        ladder_index_matches && h_matches && mu_matches && J_matches && lmbda_matches;

    MPI_Finalize();
    return ladder_index_h_mu_J_lmbda_all_match ? 0 : 1;
}
