// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#define BOOST_TEST_MODULE TestFlatSquareLattice

#include "lattice/flat_square_lattice.hpp"

#include <boost/test/unit_test.hpp>

#include <array>
#include <cmath>

namespace paratoric {

BOOST_AUTO_TEST_CASE(flat_square_periodic_counts) {
    const LatSpec spec{'x', "square", 4, 6.0, "periodic", 1};
    FlatSquareLattice lat(spec);

    BOOST_CHECK_EQUAL(lat.get_vertex_count(), 16);
    BOOST_CHECK_EQUAL(lat.get_edge_count(), 32);
    BOOST_CHECK_EQUAL(lat.get_plaquette_count(), 16);
    BOOST_CHECK_EQUAL(lat.get_diag_single_energy(), 32.0);
    BOOST_CHECK_EQUAL(lat.get_diag_tuple_energy_x(), 16.0);
    BOOST_CHECK_EQUAL(lat.get_diag_tuple_energy_z(), 16.0);
}

BOOST_AUTO_TEST_CASE(flat_square_periodic_edge_lookup_and_flip) {
    const LatSpec spec{'x', "square", 4, 6.0, "periodic", 1};
    FlatSquareLattice lat(spec);

    const auto edge = lat.edge_in_between(0, 1);
    const auto [source, target] = lat.vertices_of_edge(edge);
    BOOST_CHECK_EQUAL(source, 0);
    BOOST_CHECK_EQUAL(target, 1);

    lat.flip_spin(edge);
    BOOST_CHECK_EQUAL(lat.get_spin(edge), -1);
    BOOST_CHECK_EQUAL(lat.get_diag_single_energy(), 30.0);
    BOOST_CHECK_EQUAL(lat.get_diag_tuple_energy_x(), 12.0);
    BOOST_CHECK_EQUAL(lat.get_anyon_count(), 2);
}

BOOST_AUTO_TEST_CASE(flat_square_periodic_single_and_tuple_flips) {
    const LatSpec spec{'x', "square", 4, 6.0, "periodic", 1};
    FlatSquareLattice lat(spec);

    const auto edge = lat.edge_in_between(0, 1);
    lat.insert_double_single_spin_flip(edge, 1.0, 2.0);
    BOOST_CHECK_EQUAL(lat.get_single_spin_flips(edge).size(), 2);
    BOOST_CHECK_CLOSE(lat.integrated_edge_energy(edge, 0.0, 6.0), 4.0, 1e-12);

    const auto plaquette_edges = lat.get_plaquette_edges(0);
    lat.insert_tuple_flip(0, plaquette_edges, 3.0);
    BOOST_CHECK_EQUAL(lat.get_tuple_spin_flips(0).size(), 1);
    for (const auto tuple_edge : plaquette_edges) {
        BOOST_CHECK(lat.get_spin_flip_index(tuple_edge, 3.0) >= 0);
    }

    lat.delete_tuple_flip(0, plaquette_edges, 3.0);
    BOOST_CHECK_EQUAL(lat.get_tuple_spin_flips(0).size(), 0);
}

BOOST_AUTO_TEST_CASE(flat_square_periodic_core_energy_helpers) {
    const LatSpec spec{'x', "square", 4, 6.0, "periodic", 1};
    FlatSquareLattice lat(spec);

    BOOST_CHECK_CLOSE(lat.total_integrated_edge_energy(), 32.0 * 6.0, 1e-12);
    BOOST_CHECK_CLOSE(lat.total_integrated_star_energy(), 16.0 * 6.0, 1e-12);
    BOOST_CHECK_CLOSE(lat.total_integrated_plaquette_energy(), 16.0 * 6.0, 1e-12);

    const auto plaquette_edges = lat.get_plaquette_edges(0);
    const std::array<double, 4> plaquette_single_times{1.0, 1.5, 2.0, 2.5};
    const auto [star_sum, star_indices, star_diffs] =
        lat.integrated_star_energy_diff_combination(0, 0.5, 3.5, plaquette_single_times, 2.25);
    BOOST_CHECK_EQUAL(star_indices.size(), star_diffs.size());
    BOOST_CHECK_EQUAL(star_indices.size(), 4);
    BOOST_CHECK(std::isfinite(star_sum));

    const auto star_edges = lat.get_star_edges(0);
    const std::array<double, 4> star_single_times{1.0, 1.5, 2.0, 2.5};
    const auto [plaquette_sum, plaquette_indices, plaquette_diffs] =
        lat.integrated_plaquette_energy_diff_combination(0, 0.5, 3.5, star_single_times, 2.25);
    BOOST_CHECK_EQUAL(plaquette_indices.size(), plaquette_diffs.size());
    BOOST_CHECK_EQUAL(plaquette_indices.size(), 4);
    BOOST_CHECK(std::isfinite(plaquette_sum));

    BOOST_CHECK_EQUAL(plaquette_edges.size(), 4);
    BOOST_CHECK_EQUAL(star_edges.size(), 4);
}

} // namespace paratoric
