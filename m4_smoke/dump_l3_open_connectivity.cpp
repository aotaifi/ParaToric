// M7' step 1 helper (L=3, open boundaries), not part of any production path: dumps
// the exact edge/star/plaquette connectivity of ParaToric's own Boost lattice backend,
// so the exact-diagonalization Hamiltonian is built from the SAME combinatorics the
// QMC uses, not a hand-derived guess. Chosen size per Feynman (M7' review): L=2
// periodic is degenerate (confirmed, see dump_l2_connectivity.cpp); L=4 and L=3
// periodic are out of reach for a full-spectrum dense diagonalization (4.3e9 and
// 2.6e5 states respectively); L=3 OPEN has 12 edges / 4096 states, a dense
// diagonalization in seconds, 4 plaquettes, 9 stars, no doubled edges.
//
// No MPI needed. Build/run standalone:
//   clang++ -std=gnu++23 -Iinclude -Isrc m4_smoke/dump_l3_open_connectivity.cpp \
//     build-mpi/libparatoric_core.a <same link libs as other smoke drivers> \
//     -o dump_l3_open_connectivity
//   ./dump_l3_open_connectivity

#include "lattice/lattice.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

int main() {
    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 3, .beta = 8.0,
        .boundaries = "open", .default_spin = 1, .lattice_backend = "boost"
    };
    auto rng = std::make_shared<paratoric::rng::RNG>();
    rng->set_seed(1);
    paratoric::Lattice lat(lat_spec, rng);

    const int n_vertices = lat.get_vertex_count();
    const int n_edges = lat.get_edge_count();
    std::printf("n_vertices=%d n_edges=%d\n", n_vertices, n_edges);

    std::vector<paratoric::Lattice::Edge> canonical_edges;
    auto edge_index = [&](const paratoric::Lattice::Edge& e) -> int {
        for (std::size_t i = 0; i < canonical_edges.size(); ++i) {
            if (canonical_edges[i] == e) return static_cast<int>(i);
        }
        canonical_edges.push_back(e);
        return static_cast<int>(canonical_edges.size()) - 1;
    };

    std::printf("STARS (vertex : degree : [edge_index](vertex_pair) ...)\n");
    std::vector<int> star_degree(static_cast<size_t>(n_vertices), 0);
    for (int v = 0; v < n_vertices; ++v) {
        const auto edges = lat.get_star_edges(v);
        star_degree[static_cast<size_t>(v)] = static_cast<int>(edges.size());
        std::printf("  vertex %d (degree %zu):", v, edges.size());
        for (const auto& e : edges) {
            const auto [a, b] = lat.vertices_of_edge(e);
            std::printf(" [%d](%d,%d)", edge_index(e), a, b);
        }
        std::printf("\n");
    }

    const int n_plaquettes = lat.get_plaquette_count();
    std::printf("PLAQUETTES (n_plaquettes=%d) (index : [edge_index](vertex_pair) ...)\n", n_plaquettes);
    std::vector<std::vector<int>> plaquette_edge_sets;
    for (int p = 0; p < n_plaquettes; ++p) {
        const auto edges = lat.get_plaquette_edges(p);
        if (edges.empty()) {
            continue;
        }
        std::printf("  plaquette %d:", p);
        std::vector<int> idxs;
        for (const auto& e : edges) {
            const auto [a, b] = lat.vertices_of_edge(e);
            const int idx = edge_index(e);
            idxs.push_back(idx);
            std::printf(" [%d](%d,%d)", idx, a, b);
        }
        std::printf("\n");
        plaquette_edge_sets.push_back(idxs);
    }
    std::printf("n_plaquettes_found=%zu\n", plaquette_edge_sets.size());

    std::printf("total distinct edges seen = %zu (expect %d)\n", canonical_edges.size(), n_edges);

    std::printf("EDGE MEMBERSHIP CHECK\n");
    for (std::size_t e = 0; e < canonical_edges.size(); ++e) {
        int star_count = 0;
        for (int v = 0; v < n_vertices; ++v) {
            for (const auto& se : lat.get_star_edges(v)) {
                if (se == canonical_edges[e]) { ++star_count; break; }
            }
        }
        int plaq_count = 0;
        for (const auto& idxs : plaquette_edge_sets) {
            for (int idx : idxs) {
                if (idx == static_cast<int>(e)) { ++plaq_count; break; }
            }
        }
        std::printf("  edge %zu: in %d stars, in %d plaquettes\n", e, star_count, plaq_count);
    }

    std::printf("PLAQUETTE DISTINCTNESS CHECK\n");
    for (std::size_t i = 0; i < plaquette_edge_sets.size(); ++i) {
        for (std::size_t j = i + 1; j < plaquette_edge_sets.size(); ++j) {
            auto a = plaquette_edge_sets[i];
            auto b = plaquette_edge_sets[j];
            std::sort(a.begin(), a.end());
            std::sort(b.begin(), b.end());
            if (a == b) {
                std::printf("  plaquette %zu and %zu have the SAME edge set\n", i, j);
            }
        }
    }
    std::printf("(no output above this line under PLAQUETTE DISTINCTNESS CHECK means all distinct)\n");

    return 0;
}
