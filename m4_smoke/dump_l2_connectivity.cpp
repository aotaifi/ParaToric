// M7' step 1 helper, not part of any production path: dumps the exact edge/star/
// plaquette connectivity of ParaToric's own Boost lattice backend for L=2 square
// periodic, so the exact-diagonalization Hamiltonian is built from the SAME
// combinatorics the QMC uses, not a hand-derived guess (Feynman's warning: L=2
// periodic wraparound is exactly where a hand-derived Hamiltonian could silently
// diverge from the code's own convention).
//
// No MPI needed. Build/run standalone:
//   clang++ -std=gnu++23 -Iinclude -Isrc m4_smoke/dump_l2_connectivity.cpp \
//     build-mpi/libparatoric_core.a <same link libs as other smoke drivers> \
//     -o dump_l2_connectivity
//   ./dump_l2_connectivity

#include "lattice/lattice.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"

#include <cstdio>
#include <memory>

int main() {
    paratoric::LatSpec lat_spec{
        .basis = 'z', .lattice_type = "square", .system_size = 2, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    auto rng = std::make_shared<paratoric::rng::RNG>();
    rng->set_seed(1);
    paratoric::Lattice lat(lat_spec, rng);

    const int n_vertices = lat.get_vertex_count();
    const int n_edges = lat.get_edge_count();
    std::printf("n_vertices=%d n_edges=%d\n", n_vertices, n_edges);

    // Vertex pairs alone do NOT uniquely identify an edge at L=2 (periodic wraparound
    // gives two geometrically distinct parallel edges between some adjacent vertex
    // pairs -- confirmed: vertex 0's star lists (0,1) and (0,2) each TWICE). Assign a
    // canonical index by first-seen Edge object identity instead.
    std::vector<paratoric::Lattice::Edge> canonical_edges;
    auto edge_index = [&](const paratoric::Lattice::Edge& e) -> int {
        for (std::size_t i = 0; i < canonical_edges.size(); ++i) {
            if (canonical_edges[i] == e) return static_cast<int>(i);
        }
        canonical_edges.push_back(e);
        return static_cast<int>(canonical_edges.size()) - 1;
    };

    std::printf("STARS (vertex : [edge_index](vertex_pair) ...)\n");
    for (int v = 0; v < n_vertices; ++v) {
        std::printf("  vertex %d:", v);
        for (const auto& e : lat.get_star_edges(v)) {
            const auto [a, b] = lat.vertices_of_edge(e);
            std::printf(" [%d](%d,%d)", edge_index(e), a, b);
        }
        std::printf("\n");
    }

    std::printf("PLAQUETTES (index : [edge_index](vertex_pair) ...)\n");
    for (int p = 0; p < n_vertices; ++p) {
        const auto edges = lat.get_plaquette_edges(p);
        if (edges.empty()) {
            std::printf("  plaquette %d: EMPTY (index assumption wrong here)\n", p);
            continue;
        }
        std::printf("  plaquette %d:", p);
        for (const auto& e : edges) {
            const auto [a, b] = lat.vertices_of_edge(e);
            std::printf(" [%d](%d,%d)", edge_index(e), a, b);
        }
        std::printf("\n");
    }

    std::printf("total distinct edges seen = %zu (expect %d)\n", canonical_edges.size(), n_edges);

    return 0;
}
