// M9 requirement 7 (Feynman): empirical proof that a checkpoint written by the
// PRE-M9 code (version 2, no PT ladder state) still loads correctly with the POST-M9
// code. The checkpoint file this reads, /var/tmp/a.otaifi/m9_checkpoint_test/
// pre_m9_checkpoint.bin, was produced by literally running the pre-M9 binary (built
// from a separate git worktree at 8758b9f, the base commit before any of this
// session's changes) via its CLI -- not hand-constructed, not asserted from reading
// the code. Same parameters as boost_sample_checkpoint_restart_is_bit_faithful.
//
// No MPI needed (non-PT). Build/run standalone, see other drivers for the link line.

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <cstdio>
#include <filesystem>
#include <variant>

namespace {
double get_double(const std::variant<std::complex<double>, double>& v) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    return std::get<std::complex<double>>(v).real();
}
} // namespace

int main() {
    const std::filesystem::path pre_m9_checkpoint =
        "/var/tmp/a.otaifi/m9_checkpoint_test/pre_m9_checkpoint.bin";
    if (!std::filesystem::exists(pre_m9_checkpoint)) {
        std::printf("FAIL: pre-M9 checkpoint file not found at %s\n", pre_m9_checkpoint.c_str());
        return 1;
    }

    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 4, .beta = 3.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec{.mu = 0.9, .h = 0.7, .J = 1.1, .lmbda = 0.6};
    paratoric::SimSpec full_sim_spec{
        .N_samples = 10, .N_thermalization = 12, .N_between_samples = 8,
        .N_resamples = 8, .seed = 424242, .observables = {"energy"}
    };

    auto mc_full = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto full = mc_full.get_sample(
        paratoric::Config{full_sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}}
    );

    paratoric::OutSpec restart_out_spec{};
    restart_out_spec.restart_from = pre_m9_checkpoint;
    auto mc_resumed = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto resumed = mc_resumed.get_sample(
        paratoric::Config{full_sim_spec, param_spec, lat_spec, restart_out_spec, paratoric::ParallelTemperingSpec{}}
    );

    std::printf("Loaded pre-M9 (version 2) checkpoint successfully with post-M9 code.\n");
    std::printf("full.series[0].size()=%zu resumed.series[0].size()=%zu\n",
        full.series[0].size(), resumed.series[0].size());

    bool all_match = full.series[0].size() == resumed.series[0].size();
    for (std::size_t i = 0; all_match && i < full.series[0].size(); ++i) {
        const double a = get_double(full.series[0][i]);
        const double b = get_double(resumed.series[0][i]);
        if (a != b) {
            std::printf("MISMATCH at sample %zu: full=%.17g resumed=%.17g\n", i, a, b);
            all_match = false;
        }
    }
    std::printf("bit_faithful=%s\n", all_match ? "true" : "false");
    std::printf("resumed.production_acceptance.attempted=%llu full.production_acceptance.attempted=%llu\n",
        static_cast<unsigned long long>(resumed.production_acceptance.attempted),
        static_cast<unsigned long long>(full.production_acceptance.attempted));

    return all_match ? 0 : 1;
}
