// M11 persistence gate: real multi-rank active PT restart versus an
// uninterrupted run. Run with at least 16 ranks, for example:
//   srun --mpi=pmix -n 16 ./m9_pt_checkpoint_restart
//
// The deliberately broken leg perturbs one rank's persisted completed-sample
// marker. A correct load-time collective check must reject that file set.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <mpi.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace {

constexpr int kRanks = 16;
constexpr int kCheckpointSamples = 24;
constexpr int kTotalSamples = 48;
const std::filesystem::path kCheckpointRoot =
    "/tmp/a.otaifi/m11_pt_checkpoint_gate";

paratoric::Config make_config(int rank, int samples, bool restart,
                              bool break_marker) {
    std::vector<double> ladder;
    ladder.reserve(kRanks);
    for (int i = 0; i < kRanks; ++i) {
        ladder.emplace_back(0.20 + 0.01 * static_cast<double>(i));
    }

    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 4, .beta = 4.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.20};
    paratoric::SimSpec sim_spec{
        .N_samples = samples, .N_thermalization = 256, .N_between_samples = 16,
        .N_resamples = 16, .seed = 811000 + rank,
        .observables = {"energy", "sigma_z", "percolation_probability"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = ladder,
        .swap_period = 8,
        .validate_swap_weight = true
    };
    pt_spec.break_checkpoint_marker_for_test = break_marker;

    paratoric::OutSpec out_spec{};
    const auto checkpoint_path = kCheckpointRoot /
        ("pt_checkpoint_rank" + std::to_string(rank) + ".bin");
    if (restart) {
        out_spec.restart_from = checkpoint_path;
    } else {
        out_spec.checkpoint_path = checkpoint_path;
        out_spec.checkpoint_every_samples = kCheckpointSamples;
    }
    return paratoric::Config{sim_spec, param_spec, lat_spec, out_spec, pt_spec};
}

bool same_series(const paratoric::Result& a, const paratoric::Result& b) {
    return a.series == b.series;
}

bool same_acceptance(const paratoric::AcceptanceDiagnostics& a,
                    const paratoric::AcceptanceDiagnostics& b) {
    return a.attempted == b.attempted && a.accepted == b.accepted
        && a.acceptance_ratio_sum == b.acceptance_ratio_sum
        && a.attempted_by_update == b.attempted_by_update
        && a.accepted_by_update == b.accepted_by_update
        && a.acceptance_ratio_sum_by_update == b.acceptance_ratio_sum_by_update
        && a.block_attempted == b.block_attempted
        && a.block_accepted == b.block_accepted
        && a.block_acceptance_ratio_sum == b.block_acceptance_ratio_sum;
}

bool same_pt_gate_diagnostics(const paratoric::Result& a,
                              const paratoric::Result& b) {
    const auto& x = a.pt_diagnostics;
    const auto& y = b.pt_diagnostics;
    return x.swap_attempted == y.swap_attempted
        && x.swap_accepted == y.swap_accepted
        && x.max_swap_log_weight_deviation == y.max_swap_log_weight_deviation
        && x.max_swap_balance_err == y.max_swap_balance_err
        && x.max_shared_function_deviation == y.max_shared_function_deviation
        && x.swap_attempted_by_link == y.swap_attempted_by_link
        && x.swap_accepted_by_link == y.swap_accepted_by_link
        && x.R_count_by_link == y.R_count_by_link
        && x.R_sum_by_link == y.R_sum_by_link
        && x.R_sumsq_by_link == y.R_sumsq_by_link
        && x.ladder_index_by_sample == y.ladder_index_by_sample;
}

void remove_checkpoint_root(int rank) {
    if (rank == 0) {
        std::filesystem::remove_all(kCheckpointRoot);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

} // namespace

int main() {
    MPI_Init(nullptr, nullptr);
    int rank = 0;
    int world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if (world_size < kRanks) {
        if (rank == 0) {
            std::printf("FAIL: m11 gate requires at least %d ranks, got %d\n",
                        kRanks, world_size);
        }
        MPI_Finalize();
        return 2;
    }

    try {
        remove_checkpoint_root(rank);

        // Same seed and parameters as the prefix/restart pair, but one
        // uninterrupted trajectory to the full depth.
        auto mc_full = paratoric::ExtendedToricCodeQMC<'z'>();
        const auto uninterrupted = mc_full.get_sample(
            make_config(rank, kTotalSamples, false, false));

        MPI_Barrier(MPI_COMM_WORLD);
        auto mc_prefix = paratoric::ExtendedToricCodeQMC<'z'>();
        const auto prefix = mc_prefix.get_sample(
            make_config(rank, kCheckpointSamples, false, false));
        MPI_Barrier(MPI_COMM_WORLD);

        auto mc_restart = paratoric::ExtendedToricCodeQMC<'z'>();
        const auto restarted = mc_restart.get_sample(
            make_config(rank, kTotalSamples, true, false));

        const bool local_pass = same_series(uninterrupted, restarted)
            && same_acceptance(uninterrupted.production_acceptance,
                               restarted.production_acceptance)
            && same_pt_gate_diagnostics(uninterrupted, restarted)
            && prefix.series.size() == uninterrupted.series.size()
            && prefix.series.front().size() == kCheckpointSamples
            && restarted.pt_diagnostics.ladder_index_by_sample.size() == kTotalSamples;
        int local_pass_int = local_pass ? 1 : 0;
        int pass_count = 0;
        MPI_Allreduce(&local_pass_int, &pass_count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        if (rank == 0) {
            const auto& d = uninterrupted.pt_diagnostics;
            std::printf(
                "GATE_PASS=%s ranks=%d format_version=%u marker=%d samples=%d "
                "observable_values=%zu ladder_trace=%zu swap_attempted=%llu "
                "swap_accepted=%llu max_log_dev=%.17g max_balance=%.17g "
                "max_shared=%.17g\n",
                pass_count == world_size ? "true" : "false", world_size,
                paratoric::checkpoint_detail::SAMPLE_CHECKPOINT_VERSION,
                kCheckpointSamples, kTotalSamples,
                uninterrupted.series.empty() ? 0 : uninterrupted.series.front().size(),
                d.ladder_index_by_sample.size(),
                static_cast<unsigned long long>(d.swap_attempted),
                static_cast<unsigned long long>(d.swap_accepted),
                d.max_swap_log_weight_deviation, d.max_swap_balance_err,
                d.max_shared_function_deviation);
            for (std::size_t link = 0; link < d.swap_attempted_by_link.size(); ++link) {
                const auto attempted = d.swap_attempted_by_link[link];
                const auto accepted = d.swap_accepted_by_link[link];
                const double acceptance = attempted == 0
                    ? 0. : static_cast<double>(accepted) / static_cast<double>(attempted);
                const double r_mean = d.R_count_by_link[link] == 0
                    ? 0. : d.R_sum_by_link[link] / static_cast<double>(d.R_count_by_link[link]);
                std::printf("LINK %zu attempted=%llu accepted=%llu acceptance=%.17g "
                            "R_count=%llu R_mean=%.17g\n", link,
                            static_cast<unsigned long long>(attempted),
                            static_cast<unsigned long long>(accepted), acceptance,
                            static_cast<unsigned long long>(d.R_count_by_link[link]), r_mean);
            }
        }

        // Deliberately broken version: one rank writes marker=completed_samples+1.
        // Restart must fail collectively before any production sampling.
        MPI_Barrier(MPI_COMM_WORLD);
        remove_checkpoint_root(rank);
        auto mc_broken_prefix = paratoric::ExtendedToricCodeQMC<'z'>();
        (void)mc_broken_prefix.get_sample(
            make_config(rank, kCheckpointSamples, false, true));
        MPI_Barrier(MPI_COMM_WORLD);

        int local_marker_failure = 0;
        try {
            auto mc_broken_restart = paratoric::ExtendedToricCodeQMC<'z'>();
            (void)mc_broken_restart.get_sample(
                make_config(rank, kTotalSamples, true, false));
        } catch (const std::exception& error) {
            local_marker_failure = 1;
            if (rank == 0) {
                std::printf("DELIBERATE_BREAK_ERROR=%s\n", error.what());
            }
        }
        int marker_failure_count = 0;
        MPI_Allreduce(&local_marker_failure, &marker_failure_count, 1,
                      MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        if (rank == 0) {
            std::printf("DELIBERATE_BREAK_FAIL=%s ranks_rejected=%d expected=%d "
                        "cause=marker_mismatch\n",
                        marker_failure_count == world_size ? "true" : "false",
                        marker_failure_count, world_size);
        }

        MPI_Finalize();
        return (pass_count == world_size && marker_failure_count == world_size) ? 0 : 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[rank %d] unexpected m11 gate error: %s\n", rank, error.what());
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
}
