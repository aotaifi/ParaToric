// ============================================================================
// RESULTS FROM THIS DRIVER ARE VOID. Do not cite any number it prints as
// evidence about the swap weight, in either direction.
//
// Power check (Feynman, M7 review): broke the mu term exactly as in M4 and
// reran. The broken weight scored 0.62 combined-sigma against the correct
// weight's 0.91 -- the test got QUIETER when the physics was wrong, not
// louder. It has no demonstrated power to catch the bug it exists to catch.
//
// Root cause, traced and confirmed: the reference arm was never thermalized
// to a validated equilibrium. Its mean moved ~25 energy units between 400 and
// 16000 samples (a chain still relaxing, not noise -- confirmed by segment
// means of the raw time series). Thermalizing 25x harder narrowed but did not
// fix it: the NULL CASE (both rungs identical, which must be trivial) then
// disagreed at 8.84 combined-sigma, proof the naive var/N SEM was not
// measuring what it was being used to measure.
//
// doc/paratoric_run_manual.md, which AGENTS.md lists as mandatory reading
// before judging any run and which this driver was written without
// consulting: rule I5 forbids judging thermalization by within-chain
// statistics alone ("can DETECT a problem but can NEVER establish
// convergence") -- the only valid check is cold-start (default_spin=+1)
// versus hot-start (=-1) agreement, on BOTH arms. Rule G4 requires judging
// sampling by Neff of the worst observable (N>=100*tau, Neff>=30), not raw
// sample count. This driver uses cold starts only, a single seed per arm, and
// an uncorrected var/N SEM -- exactly the failure the manual's pitfall
// catalog (C1, C7) exists to prevent.
//
// The scope call on how to fix this (exact-diagonalization reference vs. a
// full I5/G4-compliant cold+hot PT run) is with the PI, via Feynman. Do not
// resume work on this driver without that decision.
// ============================================================================
//
// Below this line is the ORIGINAL design comment, describing what the driver
// was intended to do, kept for context on what a corrected version should
// still accomplish once the scope call above is made.
//
// M7 smoke driver (decisive physics test), not part of the checked-in test suite:
// compares the marginal distribution AT LADDER POSITION A of a two-rung common-factor
// PT run against an independent single chain at rung A's exact couplings. A wrong
// swap weight or a mis-paired MPI tag makes PT sample the wrong distribution and
// nothing else (not the unit tests, not the M4 balance check) reveals it.
//
// Local run only, no cluster: mpirun -np 2 ./m7_rung_marginal_test
//
// IMPORTANT: config.sim_spec.seed is NOT auto-differentiated per MPI rank by the
// library (extended_toric_code_qmc.hpp: `if (config.sim_spec.seed != 0)
// rng->set_seed(config.sim_spec.seed);`, same expression regardless of rank). Each
// rank here is given base_seed + rank explicitly so the two replicas are not seeded
// with correlated randomness.
//
// Grouping is by LADDER POSITION (pt_diagnostics.ladder_index_by_sample[i] == 0),
// not by MPI rank -- replicas move between ranks, so grouping by rank would pool
// samples from whichever rung a replica happened to be visiting, invalidating the
// comparison.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <mpi.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

namespace {

double get_double(const std::variant<std::complex<double>, double>& v) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    return std::get<std::complex<double>>(v).real();
}

struct MeanSem { double mean; double sem; std::size_t n; };

// Naive sample mean / standard error of the mean (var/N), NOT a tau_int-corrected
// bootstrap SEM. Stated plainly: a real production comparison would want the
// bootstrap treatment doc/paratoric_run_manual.md prescribes; this is a decisive
// smoke check at L=8, minutes-scale, not a publication-grade error bar.
MeanSem naive_mean_sem(const std::vector<double>& x) {
    const std::size_t n = x.size();
    if (n == 0) return {0., 0., 0};
    double sum = 0.;
    for (double v : x) sum += v;
    const double mean = sum / static_cast<double>(n);
    if (n < 2) return {mean, 0., n};
    double var = 0.;
    for (double v : x) var += (v - mean) * (v - mean);
    var /= static_cast<double>(n - 1);
    return {mean, std::sqrt(var / static_cast<double>(n)), n};
}

paratoric::LatSpec make_lat_spec() {
    return paratoric::LatSpec{
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
}

paratoric::ParamSpec make_param_spec() {
    // Rung A base couplings (c=1.0).
    return paratoric::ParamSpec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
}

paratoric::Config make_pt_config(double c_local, double c_partner, int seed_for_this_rank) {
    paratoric::SimSpec sim_spec{
        .N_samples = 400, .N_thermalization = 50000, .N_between_samples = 8,
        .N_resamples = 20, .seed = seed_for_this_rank, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = {0.20 * c_local, 0.20 * c_partner},
        .ladder_lmbda = {0.25 * c_local, 0.25 * c_partner},
        .ladder_mu = {1.0 * c_local, 1.0 * c_partner},
        .ladder_J = {1.0 * c_local, 1.0 * c_partner},
        .swap_period = 4,
        .validate_swap_weight = true
    };
    return paratoric::Config{sim_spec, make_param_spec(), make_lat_spec(), paratoric::OutSpec{}, pt_spec};
}

paratoric::Config make_reference_config(int seed) {
    // Reference SEM was 1.753 against PT's 0.275 at N_samples=400 (ratio ~6.37), so
    // the combined error was dominated by the arm we care least about. Raised by
    // ~6.37^2 =~ 41x here so the two arms contribute comparably (Feynman, M7 review).
    paratoric::SimSpec sim_spec{
        .N_samples = 16000, .N_thermalization = 50000, .N_between_samples = 8,
        .N_resamples = 20, .seed = seed, .observables = {"energy"}
    };
    return paratoric::Config{
        sim_spec, make_param_spec(), make_lat_spec(), paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}
    };
}

void run_case(const char* label, double c_partner, int rank, int world_size, int base_seed) {
    auto config = make_pt_config(1.0, c_partner, base_seed + rank);
    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.get_sample(config);

    std::vector<double> local_rungA_energy;
    const auto& ladder_trace = result.pt_diagnostics.ladder_index_by_sample;
    const auto& energy_series = result.series[0];
    for (std::size_t i = 0; i < ladder_trace.size(); ++i) {
        if (ladder_trace[i] == 0) {
            local_rungA_energy.push_back(get_double(energy_series[i]));
        }
    }

    {
        std::ofstream f("/tmp/m7_rank_" + std::to_string(rank) + ".txt");
        for (double v : local_rungA_energy) f << v << "\n";
    }
    bool saw0 = false, saw1 = false;
    for (int idx : ladder_trace) {
        if (idx == 0) saw0 = true;
        if (idx == 1) saw1 = true;
    }
    const int full_traversal_local = (saw0 && saw1) ? 1 : 0;

    MPI_Barrier(MPI_COMM_WORLD);

    int global_traversal_sum = 0;
    MPI_Reduce(&full_traversal_local, &global_traversal_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<double> pooled;
        for (int r = 0; r < world_size; ++r) {
            std::ifstream f("/tmp/m7_rank_" + std::to_string(r) + ".txt");
            double v;
            while (f >> v) pooled.push_back(v);
        }
        const auto pt_stat = naive_mean_sem(pooled);

        auto ref_config = make_reference_config(base_seed + 7654321); // distinct from PT seeds
        const auto ref_result = mc.get_sample(ref_config);
        const double ref_mean = ref_result.mean[0];
        const double ref_sem = ref_result.mean_std[0];

        // DIAGNOSTIC (temporary): check for within-chain drift, segment means over
        // the raw time series (not bootstrap), to see if the reference chain is
        // still relaxing rather than sampling around a stationary mean.
        {
            const auto& raw = ref_result.series[0];
            const std::size_t n = raw.size();
            const std::size_t seg = n / 4;
            for (int s = 0; s < 4; ++s) {
                double sum = 0.;
                for (std::size_t i = static_cast<std::size_t>(s) * seg;
                     i < static_cast<std::size_t>(s + 1) * seg; ++i) {
                    sum += get_double(raw[i]);
                }
                std::printf("  [diag] segment %d/4 mean = %.6f (samples %zu-%zu)\n",
                    s + 1, sum / static_cast<double>(seg),
                    static_cast<std::size_t>(s) * seg, static_cast<std::size_t>(s + 1) * seg);
            }
        }

        const double combined_err = std::sqrt(pt_stat.sem * pt_stat.sem + ref_sem * ref_sem);
        const double diff = pt_stat.mean - ref_mean;
        const double sigma = combined_err > 0. ? diff / combined_err : 0.;

        std::printf("=== %s ===\n", label);
        std::printf("rung-A pooled (PT):  mean=%.6f sem=%.6f n=%zu\n", pt_stat.mean, pt_stat.sem, pt_stat.n);
        std::printf("reference (no PT):   mean=%.6f sem=%.6f n=%d\n", ref_mean, ref_sem,
            ref_config.sim_spec.N_samples);
        std::printf("diff=%.6f combined_err=%.6f diff_in_combined_sigma=%.4f\n", diff, combined_err, sigma);
        std::printf("swap_attempted=%llu swap_accepted=%llu acceptance=%.4f\n",
            static_cast<unsigned long long>(result.pt_diagnostics.swap_attempted),
            static_cast<unsigned long long>(result.pt_diagnostics.swap_accepted),
            result.pt_diagnostics.swap_attempted > 0
                ? static_cast<double>(result.pt_diagnostics.swap_accepted)
                    / static_cast<double>(result.pt_diagnostics.swap_attempted)
                : 0.0);
        for (std::size_t i = 0; i < result.pt_diagnostics.swap_attempted_by_link.size(); ++i) {
            const auto att = result.pt_diagnostics.swap_attempted_by_link[i];
            const auto acc = result.pt_diagnostics.swap_accepted_by_link[i];
            std::printf("  link[%zu]: attempted=%llu accepted=%llu acceptance=%.4f\n", i,
                static_cast<unsigned long long>(att), static_cast<unsigned long long>(acc),
                att > 0 ? static_cast<double>(acc) / static_cast<double>(att) : 0.0);
        }
        std::printf("full_traversal_replicas=%d/%d\n\n", global_traversal_sum, world_size);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

} // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    int world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    run_case("NULL CASE: both rungs identical (c=1.0, c=1.0)", 1.0, rank, world_size, 100000);
    run_case("REAL CASE: common factor c=1.0 vs c=1.3, all four couplings", 1.3, rank, world_size, 200000);

    MPI_Finalize();
    return 0;
}
