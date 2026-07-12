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

template <char Basis>
void print_checkpoint_readout_for_basis(
    const std::string& label,
    const std::filesystem::path& path,
    int L,
    double h,
    double lmbda,
    int default_spin,
    int n_between_samples,
    const std::vector<std::string>& observables
) {
    const auto underscore = label.rfind('_');
    if (underscore == std::string::npos) {
        throw std::runtime_error("Checkpoint label must end in _<sweeps>: " + label);
    }
    const auto sweeps = std::stoll(label.substr(underscore + 1));
    const auto moves = sweeps * 2LL * L * L;
    if (n_between_samples <= 0 || moves % n_between_samples != 0) {
        throw std::invalid_argument("Checkpoint depth is not divisible by the sample cadence.");
    }
    const paratoric::LatSpec lat_spec{Basis, "square", L, static_cast<double>(L), "periodic", default_spin};
    const paratoric::ParamSpec param_spec{
        .mu = 1.,
        .h = h,
        .J = 1.,
        .lmbda = lmbda
    };
    const paratoric::SimSpec sim_spec{
        .N_samples = static_cast<int>(moves / n_between_samples),
        .N_between_samples = n_between_samples,
        .N_resamples = 1,
        .custom_therm = false,
        .observables = observables
    };
    const paratoric::Config config{
        .sim_spec = sim_spec,
        .param_spec = param_spec,
        .lat_spec = lat_spec
    };
    auto rng = std::make_shared<paratoric::rng::RNG>();
    auto qmc = paratoric::ExtendedToricCodeQMC<Basis, paratoric::Lattice>(rng);
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

void print_checkpoint_readout(
    const std::string& label,
    const std::filesystem::path& path,
    int L,
    double h,
    double lmbda,
    char basis,
    int default_spin,
    int n_between_samples,
    bool electric_only
) {
    const std::vector<std::string> observables = electric_only
        ? std::vector<std::string>{"percolation_probability", "percolation_strength"}
        : std::vector<std::string>{
            "percolation_probability", "percolation_strength",
            "plaquette_percolation_probability", "plaquette_percolation_strength"};
    if (basis == 'z') {
        print_checkpoint_readout_for_basis<'z'>(
            label, path, L, h, lmbda, default_spin, n_between_samples, observables);
    } else if (basis == 'x') {
        print_checkpoint_readout_for_basis<'x'>(
            label, path, L, h, lmbda, default_spin, n_between_samples, observables);
    } else {
        throw std::invalid_argument("Basis must be x or z.");
    }
}

} // namespace

int main(int argc, char** argv) {
    std::cout << std::setprecision(17);
    run_calibration('x');
    run_calibration('z');
    int first_pair = 1;
    int L = 32;
    double h = 0.15;
    double lmbda = 0.3;
    char basis = 'z';
    int default_spin = 1;
    int n_between_samples = 100 * 2 * L * L;
    bool electric_only = false;
    if (argc > 1 && std::string(argv[1]) == "--scan") {
        if (argc < 8) {
            throw std::invalid_argument(
                "Usage: percolation_decisive_experiment --scan L h default_spin n_between_moves label checkpoint [...]"
            );
        }
        L = std::stoi(argv[2]);
        h = std::stod(argv[3]);
        default_spin = std::stoi(argv[4]);
        n_between_samples = std::stoi(argv[5]);
        first_pair = 6;
    } else if (argc > 1 && std::string(argv[1]) == "--basis-scan") {
        if (argc < 10) {
            throw std::invalid_argument(
                "Usage: percolation_decisive_experiment --basis-scan L h lmbda basis default_spin n_between_moves label checkpoint [...]"
            );
        }
        L = std::stoi(argv[2]);
        h = std::stod(argv[3]);
        lmbda = std::stod(argv[4]);
        basis = argv[5][0];
        default_spin = std::stoi(argv[6]);
        n_between_samples = std::stoi(argv[7]);
        first_pair = 8;
        electric_only = true;
    }
    if (argc > first_pair) {
        std::cout << "scan,label,default_spin,moves,base_minus_edges,total_edges,"
                     "base_eprob,base_estrength,base_pprob,base_pstrength,"
                     "rot_eprob,rot_estrength,rot_pprob,rot_pstrength\n";
    }
    for (int i = first_pair; i + 1 < argc; i += 2) {
        print_checkpoint_readout(
            argv[i], argv[i + 1], L, h, lmbda, basis, default_spin,
            n_between_samples, electric_only);
    }
    return 0;
}
