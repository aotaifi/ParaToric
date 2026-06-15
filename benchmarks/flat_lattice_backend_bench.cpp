#include "lattice/lattice.hpp"
#include "paratoric/types/types.hpp"

#include <sys/resource.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct FlatSquarePeriodic {
    int L = 0;
    int vertices = 0;
    int edges = 0;
    int plaquettes = 0;
    std::vector<std::int8_t> edge_spin;
    std::vector<std::array<int, 4>> star_edges;
    std::vector<std::array<int, 4>> plaquette_edges;

    explicit FlatSquarePeriodic(int system_size)
        : L(system_size),
          vertices(system_size * system_size),
          edges(2 * system_size * system_size),
          plaquettes(system_size * system_size),
          edge_spin(static_cast<std::size_t>(edges), 1),
          star_edges(static_cast<std::size_t>(vertices)),
          plaquette_edges(static_cast<std::size_t>(plaquettes)) {
        if (L <= 0) {
            throw std::invalid_argument("L must be positive");
        }

        for (int y = 0; y < L; ++y) {
            for (int x = 0; x < L; ++x) {
                const int v = vertex(x, y);
                const int right = horizontal_edge(x, y);
                const int left = horizontal_edge(mod(x - 1), y);
                const int up = vertical_edge(x, y);
                const int down = vertical_edge(x, mod(y - 1));
                star_edges[static_cast<std::size_t>(v)] = {right, left, up, down};

                const int p = v;
                const int bottom = horizontal_edge(x, y);
                const int top = horizontal_edge(x, mod(y + 1));
                const int left_side = vertical_edge(x, y);
                const int right_side = vertical_edge(mod(x + 1), y);
                plaquette_edges[static_cast<std::size_t>(p)] = {bottom, top, left_side, right_side};
            }
        }
    }

    int mod(int value) const {
        value %= L;
        return value < 0 ? value + L : value;
    }

    int vertex(int x, int y) const {
        return y * L + x;
    }

    int horizontal_edge(int x, int y) const {
        return y * L + x;
    }

    int vertical_edge(int x, int y) const {
        return L * L + y * L + x;
    }

    void flip_edge(int edge) {
        edge_spin[static_cast<std::size_t>(edge)] *= -1;
    }

    std::int64_t diag_single_sum() const {
        return std::accumulate(edge_spin.begin(), edge_spin.end(), std::int64_t{0});
    }

    std::int64_t star_product_sum() const {
        std::int64_t sum = 0;
        for (const auto& edges_for_star : star_edges) {
            int product = 1;
            for (const int edge : edges_for_star) {
                product *= edge_spin[static_cast<std::size_t>(edge)];
            }
            sum += product;
        }
        return sum;
    }

    std::int64_t plaquette_product_sum() const {
        std::int64_t sum = 0;
        for (const auto& edges_for_plaquette : plaquette_edges) {
            int product = 1;
            for (const int edge : edges_for_plaquette) {
                product *= edge_spin[static_cast<std::size_t>(edge)];
            }
            sum += product;
        }
        return sum;
    }
};

struct Options {
    std::string backend = "boost";
    int L = 400;
    int sweeps = 1;
    int repeats = 1;
    int seed = 1;
};

double seconds_since(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

long max_rss_kb() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need_value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string("missing value for ") + name);
            }
            return argv[++i];
        };
        if (arg == "--backend") {
            options.backend = need_value("--backend");
        } else if (arg == "--L") {
            options.L = std::stoi(need_value("--L"));
        } else if (arg == "--sweeps") {
            options.sweeps = std::stoi(need_value("--sweeps"));
        } else if (arg == "--repeats") {
            options.repeats = std::stoi(need_value("--repeats"));
        } else if (arg == "--seed") {
            options.seed = std::stoi(need_value("--seed"));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: flat_lattice_backend_bench --backend boost|flat --L 400 --sweeps 1 --repeats 1 --seed 1\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }
    return options;
}

void run_flat(const Options& options) {
    std::mt19937_64 rng(static_cast<std::uint64_t>(options.seed));
    for (int repeat = 0; repeat < options.repeats; ++repeat) {
        const auto construct_start = Clock::now();
        FlatSquarePeriodic lattice(options.L);
        const double construct_seconds = seconds_since(construct_start);

        std::uniform_int_distribution<int> edge_dist(0, lattice.edges - 1);
        const int updates = options.sweeps * lattice.edges;
        const auto update_start = Clock::now();
        for (int i = 0; i < updates; ++i) {
            lattice.flip_edge(edge_dist(rng));
        }
        const double update_seconds = seconds_since(update_start);

        const auto observe_start = Clock::now();
        const auto diag = lattice.diag_single_sum();
        const auto stars = lattice.star_product_sum();
        const auto plaquettes = lattice.plaquette_product_sum();
        const double observe_seconds = seconds_since(observe_start);

        std::cout
            << "backend=flat"
            << " repeat=" << repeat
            << " L=" << options.L
            << " vertices=" << lattice.vertices
            << " edges=" << lattice.edges
            << " plaquettes=" << lattice.plaquettes
            << " updates=" << updates
            << " construct_s=" << construct_seconds
            << " update_s=" << update_seconds
            << " observe_s=" << observe_seconds
            << " checksum=" << (diag + stars + plaquettes)
            << " maxrss_kb=" << max_rss_kb()
            << '\n';
    }
}

void run_boost(const Options& options) {
    for (int repeat = 0; repeat < options.repeats; ++repeat) {
        const paratoric::LatSpec spec{
            'x',
            "square",
            options.L,
            1.0,
            "periodic",
            1
        };

        const auto construct_start = Clock::now();
        paratoric::Lattice lattice(spec);
        const double construct_seconds = seconds_since(construct_start);

        const int edge_count = lattice.get_edge_count();
        const int updates = options.sweeps * edge_count;
        const auto update_start = Clock::now();
        for (int i = 0; i < updates; ++i) {
            lattice.flip_spin(lattice.get_random_edge_descriptor());
        }
        const double update_seconds = seconds_since(update_start);

        const auto observe_start = Clock::now();
        const auto diag = lattice.get_diag_single_energy();
        const auto stars = lattice.get_diag_tuple_energy_x();
        const auto plaquettes = lattice.get_diag_tuple_energy_z();
        const double observe_seconds = seconds_since(observe_start);

        std::cout
            << "backend=boost"
            << " repeat=" << repeat
            << " L=" << options.L
            << " vertices=" << lattice.get_vertex_count()
            << " edges=" << edge_count
            << " plaquettes=" << lattice.get_plaquette_count()
            << " updates=" << updates
            << " construct_s=" << construct_seconds
            << " update_s=" << update_seconds
            << " observe_s=" << observe_seconds
            << " checksum=" << (diag + stars + plaquettes)
            << " maxrss_kb=" << max_rss_kb()
            << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        if (options.backend == "flat") {
            run_flat(options);
        } else if (options.backend == "boost") {
            run_boost(options);
        } else {
            throw std::invalid_argument("--backend must be boost or flat");
        }
    } catch (const std::exception& error) {
        std::cerr << "flat_lattice_backend_bench: " << error.what() << '\n';
        return 2;
    }
    return 0;
}
