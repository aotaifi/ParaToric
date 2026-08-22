// M7' step 2 (Feynman review): the gate before the ladder runs at all. A plain
// single chain (no PT) at L=3 square OPEN boundaries must reproduce the exact
// thermal mean energy from m4_smoke/exact_l3_open.py.
//
// Thermalization validated by I5 (doc/paratoric_run_manual.md): cold-start
// (default_spin=+1) vs hot-start (=-1) must AGREE within combined error. Never
// judged by within-chain statistics alone -- that is exactly what went wrong
// in the original (VOID) M7 attempt.
//
// Error bars: result.mean_std[0] and result.tau_int[0], both produced by the
// framework's own stationary-bootstrap machinery (statistics/bootstrap.hpp,
// "Autocorrelation effects are included in the stationary bootstrap") -- NOT a
// hand-rolled var/N. Neff = N_samples / (2*tau_int), per rule G4/B4.
//
// No MPI needed. Build/run standalone (see other m4_smoke drivers for link line).

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <cmath>
#include <cstdio>

namespace {

paratoric::Config make_config(int default_spin, int seed) {
    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 3, .beta = 8.0,
        .boundaries = "open", .default_spin = default_spin, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec{
        .N_samples = 10000, .N_thermalization = 200000, .N_between_samples = 8,
        .N_resamples = 200, .seed = seed, .observables = {"energy"}
    };
    return paratoric::Config{
        sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}
    };
}

void report(const char* label, const paratoric::Result& r) {
    const double mean = r.mean[0];
    const double sem = r.mean_std[0];
    const double tau = r.tau_int[0];
    const double Neff = static_cast<double>(10000) / (2.0 * tau);
    std::printf("%s: mean=%.6f sem=%.6f tau_int=%.4f Neff=%.2f\n", label, mean, sem, tau, Neff);
}

} // namespace

int main() {
    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();

    auto cold_config = make_config(+1, 314159);
    const auto cold = mc.get_sample(cold_config);
    report("COLD (default_spin=+1)", cold);

    auto hot_config = make_config(-1, 271828);
    const auto hot = mc.get_sample(hot_config);
    report("HOT  (default_spin=-1)", hot);

    const double diff = cold.mean[0] - hot.mean[0];
    const double combined = std::sqrt(cold.mean_std[0] * cold.mean_std[0] + hot.mean_std[0] * hot.mean_std[0]);
    const double sigma = combined > 0. ? diff / combined : 0.;
    std::printf("I5 CHECK: cold-hot diff=%.6f combined_err=%.6f diff_in_sigma=%.4f\n", diff, combined, sigma);

    const double exact = -13.6711924202; // from m4_smoke/exact_l3_open.py
    std::printf("EXACT reference: %.10f\n", exact);
    for (const auto* pair : {&cold, &hot}) {
        const double d = pair->mean[0] - exact;
        const double s = pair->mean_std[0] > 0. ? d / pair->mean_std[0] : 0.;
        std::printf("  vs exact: diff=%.6f diff_in_sigma=%.4f\n", d, s);
    }

    return 0;
}
