// M7' final run (Feynman review, round 3), two-rung common-factor PT ladder, L=3
// square open, compared against the exact thermal reference at TWO operating points
// (beta=0.5, beta=0.25), pre-registered BEFORE any run in
// m4_smoke/exact_l3_open_powercheck.py. beta=8 was rejected: the system freezes into
// its ground state there and a common-factor ladder's rungs share eigenvectors,
// making a rung mix-up invisible.
//
// Round 3 additions, both from Feynman's review of round 2:
//  - N_samples raised 10x (10000 -> 100000) to sharpen the power check.
//  - A second observable, "energy_mu" (extended_toric_code_qmc.hpp:706-709, z basis:
//    -get_non_diag_tuple_energy_z(), whose ensemble mean is -mu*<sum_v A_v>_thermal
//    by the kink-count identity), the one directly conjugate to the coupling being
//    broken in the power check. Exact reference in
//    m4_smoke/exact_l3_open_mu_observable.py.
//  - A plain, no-PT chain at rung A's couplings, both operating points, both
//    observables, to test whether the null-case offset seen in round 2 (both null
//    cases sitting ~0.26-0.29 below exact, same sign, two temperatures) lives in the
//    QMC/error-bar side or in the tempering/pooling path.
//
// Round 1/2 requirements still applied: grouping by LADDER POSITION gathered via MPI
// and interleaved BY SAMPLE INDEX (true time order, not rank-concatenated); tau_int
// via statistics/autocorrelation.hpp; bootstrap SEM via statistics/bootstrap.hpp
// (stationary bootstrap, autocorrelation-aware); acceptance per link and traversal
// count reported, near-zero acceptance voids a point; null case at both points.
//
// Local run only, no cluster: mpirun -np 2 ./m7prime_final

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"
#include "statistics/autocorrelation.hpp"
#include "statistics/bootstrap.hpp"

#include <mpi.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace {

constexpr int N_SAMPLES = 100000;
constexpr int N_THERMALIZATION = 200000;
constexpr int N_BETWEEN_SAMPLES = 8;

double get_double(const std::variant<std::complex<double>, double>& v) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    return std::get<std::complex<double>>(v).real();
}

struct ArmStats { double mean; double sem; double tau; double Neff; std::size_t n; };

ArmStats bootstrap_stats(const std::vector<double>& data, std::shared_ptr<paratoric::rng::RNG> rng) {
    if (data.size() < 2) return {0., 0., 0., 0., data.size()};
    auto acf = paratoric::statistics::get_autocorrelation_function(data);
    const double tau = paratoric::statistics::get_autocorrelation_time(acf);
    const auto [mean, sem, binder, binder_sem] = paratoric::statistics::get_bootstrap_statistics(data, rng);
    (void)binder; (void)binder_sem;
    const double Neff = tau > 0. ? static_cast<double>(data.size()) / (2.0 * tau) : static_cast<double>(data.size());
    return {mean, sem, tau, Neff, data.size()};
}

void print_stat_vs_exact(const char* obs_label, const ArmStats& s, double exact) {
    const double diff = s.mean - exact;
    const double sigma = s.sem > 0. ? diff / s.sem : 0.;
    std::printf("  [%s] n=%zu mean=%.6f sem=%.6f tau_int=%.4f Neff=%.2f | exact=%.6f diff=%.6f diff_in_sigma=%.4f\n",
        obs_label, s.n, s.mean, s.sem, s.tau, s.Neff, exact, diff, sigma);
    if (s.Neff < 30.0) {
        std::printf("    *** WARNING: Neff < 30 for %s ***\n", obs_label);
    }
}

paratoric::LatSpec make_lat_spec(double beta) {
    return paratoric::LatSpec{
        .basis = 'z', .lattice_type = "square", .system_size = 3, .beta = beta,
        .boundaries = "open", .default_spin = 1, .lattice_backend = "boost"
    };
}

paratoric::Config make_pt_config(double beta, double c_local, double c_partner, int seed_for_this_rank) {
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec{
        .N_samples = N_SAMPLES, .N_thermalization = N_THERMALIZATION, .N_between_samples = N_BETWEEN_SAMPLES,
        .N_resamples = 200, .seed = seed_for_this_rank, .observables = {"energy", "energy_mu"}
    };
    paratoric::ParallelTemperingSpec pt_spec{
        .enabled = true,
        .ladder_s = {0.20 * c_local, 0.20 * c_partner},
        .ladder_lmbda = {0.25 * c_local, 0.25 * c_partner},
        .ladder_mu = {1.0 * c_local, 1.0 * c_partner},
        .ladder_J = {1.0 * c_local, 1.0 * c_partner},
        .swap_period = 1, // M7' round 4 (Feynman): swap dominant over local re-thermalization
        .validate_swap_weight = true
    };
    return paratoric::Config{sim_spec, param_spec, make_lat_spec(beta), paratoric::OutSpec{}, pt_spec};
}

paratoric::Config make_plain_config(double beta, int seed) {
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec{
        .N_samples = N_SAMPLES, .N_thermalization = N_THERMALIZATION, .N_between_samples = N_BETWEEN_SAMPLES,
        .N_resamples = 200, .seed = seed, .observables = {"energy", "energy_mu"}
    };
    return paratoric::Config{
        sim_spec, param_spec, make_lat_spec(beta), paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}
    };
}

void run_ladder_case(const char* label, double beta, double c_partner,
                      double exact_energy_A, double exact_energy_mu_A,
                      int rank, int world_size, int base_seed) {
    auto config = make_pt_config(beta, 1.0, c_partner, base_seed + rank);
    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.get_sample(config);

    const int N = config.sim_spec.N_samples;
    const auto& ladder_trace = result.pt_diagnostics.ladder_index_by_sample;
    const auto& energy_series = result.series[0];
    const auto& energy_mu_series = result.series[1];

    std::vector<int> local_idx(static_cast<size_t>(N));
    std::vector<double> local_energy(static_cast<size_t>(N));
    std::vector<double> local_energy_mu(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        local_idx[static_cast<size_t>(i)] = ladder_trace[static_cast<size_t>(i)];
        local_energy[static_cast<size_t>(i)] = get_double(energy_series[static_cast<size_t>(i)]);
        local_energy_mu[static_cast<size_t>(i)] = get_double(energy_mu_series[static_cast<size_t>(i)]);
    }

    std::vector<int> all_idx;
    std::vector<double> all_energy, all_energy_mu;
    if (rank == 0) {
        all_idx.resize(static_cast<size_t>(N) * static_cast<size_t>(world_size));
        all_energy.resize(all_idx.size());
        all_energy_mu.resize(all_idx.size());
    }
    MPI_Gather(local_idx.data(), N, MPI_INT, rank == 0 ? all_idx.data() : nullptr, N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(local_energy.data(), N, MPI_DOUBLE, rank == 0 ? all_energy.data() : nullptr, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(local_energy_mu.data(), N, MPI_DOUBLE, rank == 0 ? all_energy_mu.data() : nullptr, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    bool saw0 = false, saw1 = false;
    for (int idx : local_idx) { if (idx == 0) saw0 = true; if (idx == 1) saw1 = true; }
    const int full_traversal_local = (saw0 && saw1) ? 1 : 0;
    int global_traversal_sum = 0;
    MPI_Reduce(&full_traversal_local, &global_traversal_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<double> pooled_energy, pooled_energy_mu;
        for (int i = 0; i < N; ++i) {
            for (int r = 0; r < world_size; ++r) {
                const size_t idx = static_cast<size_t>(r) * static_cast<size_t>(N) + static_cast<size_t>(i);
                if (all_idx[idx] == 0) {
                    pooled_energy.push_back(all_energy[idx]);
                    pooled_energy_mu.push_back(all_energy_mu[idx]);
                    break;
                }
            }
        }

        auto boot_rng = std::make_shared<paratoric::rng::RNG>();
        boot_rng->set_seed(static_cast<std::uint64_t>(base_seed) + 999);
        const auto stat_e = bootstrap_stats(pooled_energy, boot_rng);
        auto boot_rng2 = std::make_shared<paratoric::rng::RNG>();
        boot_rng2->set_seed(static_cast<std::uint64_t>(base_seed) + 1999);
        const auto stat_em = bootstrap_stats(pooled_energy_mu, boot_rng2);

        const auto& attempted = result.pt_diagnostics.swap_attempted_by_link;
        const auto& accepted = result.pt_diagnostics.swap_accepted_by_link;
        double overall_acc = result.pt_diagnostics.swap_attempted > 0
            ? static_cast<double>(result.pt_diagnostics.swap_accepted) / static_cast<double>(result.pt_diagnostics.swap_attempted)
            : 0.0;

        std::printf("=== %s (beta=%.2f) ===\n", label, beta);
        print_stat_vs_exact("energy", stat_e, exact_energy_A);
        print_stat_vs_exact("energy_mu", stat_em, exact_energy_mu_A);
        std::printf("  swap_attempted=%llu swap_accepted=%llu overall_acceptance=%.4f\n",
            static_cast<unsigned long long>(result.pt_diagnostics.swap_attempted),
            static_cast<unsigned long long>(result.pt_diagnostics.swap_accepted), overall_acc);
        for (std::size_t i = 0; i < attempted.size(); ++i) {
            const double la = attempted[i] > 0 ? static_cast<double>(accepted[i]) / static_cast<double>(attempted[i]) : 0.0;
            std::printf("    link[%zu]: attempted=%llu accepted=%llu acceptance=%.4f\n", i,
                static_cast<unsigned long long>(attempted[i]), static_cast<unsigned long long>(accepted[i]), la);
        }
        std::printf("  full_traversal_replicas=%d/%d\n", global_traversal_sum, world_size);
        if (overall_acc < 0.02) {
            std::printf("  *** VOID: acceptance near zero, swap weight not exercised ***\n");
        }
        std::printf("\n");
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

void run_plain_case(const char* label, double beta, double exact_energy_A, double exact_energy_mu_A,
                     int rank, int seed) {
    if (rank != 0) return; // no PT, no MPI needed -- rank 0 only
    auto config = make_plain_config(beta, seed);
    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.get_sample(config);

    ArmStats stat_e{result.mean[0], result.mean_std[0], result.tau_int[0],
        static_cast<double>(config.sim_spec.N_samples) / (2.0 * result.tau_int[0]), result.series[0].size()};
    ArmStats stat_em{result.mean[1], result.mean_std[1], result.tau_int[1],
        static_cast<double>(config.sim_spec.N_samples) / (2.0 * result.tau_int[1]), result.series[1].size()};

    std::printf("=== %s (beta=%.2f) ===\n", label, beta);
    print_stat_vs_exact("energy", stat_e, exact_energy_A);
    print_stat_vs_exact("energy_mu", stat_em, exact_energy_mu_A);
    std::printf("\n");
}

} // namespace

int main() {
    MPI_Init(nullptr, nullptr);
    int rank = 0, world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Exact references: energy from exact_l3_open_powercheck.py, energy_mu from
    // exact_l3_open_mu_observable.py, both at rung A's couplings (c=1.0).
    const double exact_energy_A_b050 = -6.6109;
    const double exact_energy_A_b025 = -3.5011;
    const double exact_energy_mu_A_b050 = -4.166190;
    const double exact_energy_mu_A_b025 = -2.207297;

    run_plain_case("PLAIN CHAIN (no PT) beta=0.5", 0.5, exact_energy_A_b050, exact_energy_mu_A_b050, rank, 111111);
    run_plain_case("PLAIN CHAIN (no PT) beta=0.25", 0.25, exact_energy_A_b025, exact_energy_mu_A_b025, rank, 222222);

    run_ladder_case("NULL beta=0.5 (rungs identical)", 0.5, 1.0,
        exact_energy_A_b050, exact_energy_mu_A_b050, rank, world_size, 500000);
    run_ladder_case("REAL beta=0.5 (c=1.0 vs 1.3, swap_period=1)", 0.5, 1.3,
        exact_energy_A_b050, exact_energy_mu_A_b050, rank, world_size, 600000);
    run_ladder_case("NULL beta=0.25 (rungs identical)", 0.25, 1.0,
        exact_energy_A_b025, exact_energy_mu_A_b025, rank, world_size, 700000);
    run_ladder_case("REAL beta=0.25 (c=1.0 vs 1.3, swap_period=1)", 0.25, 1.3,
        exact_energy_A_b025, exact_energy_mu_A_b025, rank, world_size, 800000);

    MPI_Finalize();
    return 0;
}
