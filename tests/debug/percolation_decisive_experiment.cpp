// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis

#include "lattice/lattice.hpp"
#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

struct PercolationReadout {
    double electric_probability = 0.;
    double electric_strength = 0.;
    double plaquette_probability = 0.;
    double plaquette_strength = 0.;
};

PercolationReadout measure(paratoric::Lattice& lat) {
    return {
        lat.percolation_probability(),
        lat.percolation_strength(),
        lat.plaquette_percolation_probability(),
        lat.plaquette_percolation_strength()
    };
}

void print_readout(const PercolationReadout& r) {
    std::cout
        << r.electric_probability << ','
        << r.electric_strength << ','
        << r.plaquette_probability << ','
        << r.plaquette_strength;
}

void flip_vertical_winding_string(paratoric::Lattice& lat, int L, int x) {
    for (int y = 0; y < L; ++y) {
        const int v = y * L + x;
        const int next = ((y + 1) % L) * L + x;
        lat.flip_spin(lat.edge_in_between(v, next));
    }
}

void flip_random_edges(paratoric::Lattice& lat, int L, unsigned seed) {
    std::mt19937 rng(seed);
    std::bernoulli_distribution flip(0.5);
    for (int y = 0; y < L; ++y) {
        for (int x = 0; x < L; ++x) {
            const int v = y * L + x;
            const int right = y * L + ((x + 1) % L);
            const int down = ((y + 1) % L) * L + x;
            if (flip(rng)) {
                lat.flip_spin(lat.edge_in_between(v, right));
            }
            if (flip(rng)) {
                lat.flip_spin(lat.edge_in_between(v, down));
            }
        }
    }
}

void run_calibration(char basis) {
    constexpr int L = 6;
    const paratoric::LatSpec plus{basis, "square", L, 6., "periodic", 1};
    const paratoric::LatSpec minus{basis, "square", L, 6., "periodic", -1};

    auto all_plus = paratoric::Lattice(plus);
    auto all_minus = paratoric::Lattice(minus);
    auto winding = paratoric::Lattice(plus);
    auto dense_random = paratoric::Lattice(plus);
    flip_vertical_winding_string(winding, L, 0);
    flip_random_edges(dense_random, L, 20260712u);

    std::cout << "calibration,basis,label,eprob,estrength,pprob,pstrength\n";
    for (auto [label, lat] : std::vector<std::pair<std::string, paratoric::Lattice*>>{
             {"all_plus", &all_plus},
             {"all_minus", &all_minus},
             {"winding_string", &winding},
             {"dense_random", &dense_random}}) {
        std::cout << "calibration," << basis << ',' << label << ',';
        print_readout(measure(*lat));
        std::cout << '\n';
    }
}

void print_checkpoint_readout(const std::string& label, const std::filesystem::path& path) {
    const auto underscore = label.rfind('_');
    if (underscore == std::string::npos) {
        throw std::runtime_error("Checkpoint label must end in _<sweeps>: " + label);
    }
    const auto sweeps = std::stoll(label.substr(underscore + 1));
    const auto moves = static_cast<int>(sweeps * 2LL * 32LL * 32LL);
    const paratoric::LatSpec lat_spec{'z', "square", 32, 32., "periodic", label.find("spin-1") == 0 ? -1 : 1};
    const paratoric::ParamSpec param_spec{
        .mu = 1.,
        .h = 0.15,
        .J = 1.,
        .lmbda = 0.3
    };
    const paratoric::SimSpec sim_spec{
        .N_samples = 1,
        .N_between_samples = moves,
        .N_resamples = 1,
        .custom_therm = false,
        .observables = {
            "percolation_probability",
            "percolation_strength",
            "plaquette_percolation_probability",
            "plaquette_percolation_strength"
        }
    };
    const paratoric::Config config{
        .sim_spec = sim_spec,
        .param_spec = param_spec,
        .lat_spec = lat_spec
    };
    auto rng = std::make_shared<paratoric::rng::RNG>();
    auto qmc = paratoric::ExtendedToricCodeQMC<'z', paratoric::Lattice>(rng);
    auto lat = paratoric::Lattice(lat_spec, rng);
    const auto production_moves = qmc.load_sample_checkpoint_lattice_debug(path, config, lat);
    const auto base_minus_edges = lat.get_string_count();
    const auto total_edges = lat.get_edge_count();
    const auto base = measure(lat);
    lat.rotate_imag_time();
    const auto rotated = measure(lat);

    std::cout << "scan," << label << ',' << lat_spec.default_spin << ','
              << production_moves << ',' << base_minus_edges
              << ',' << total_edges << ',';
    print_readout(base);
    std::cout << ',';
    print_readout(rotated);
    std::cout << '\n';
}

} // namespace

int main(int argc, char** argv) {
    std::cout << std::setprecision(17);
    run_calibration('x');
    run_calibration('z');
    if (argc > 1) {
        std::cout << "scan,label,default_spin,moves,base_minus_edges,total_edges,"
                     "base_eprob,base_estrength,base_pprob,base_pstrength,"
                     "rot_eprob,rot_estrength,rot_pprob,rot_pstrength\n";
    }
    for (int i = 1; i + 1 < argc; i += 2) {
        print_checkpoint_readout(argv[i], argv[i + 1]);
    }
    return 0;
}
