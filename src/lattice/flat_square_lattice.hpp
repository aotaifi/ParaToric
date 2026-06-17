// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"

#include <boost/container/small_vector.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <numeric>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace paratoric {

class FlatSquareLattice {
public:
    using RNG = paratoric::rng::RNG;
    using Edge = int;
    using SmallIndexVector = boost::container::small_vector<int, 8>;
    using SmallEnergyVector = boost::container::small_vector<double, 8>;
    using VertexPair = std::pair<int, int>;

    explicit FlatSquareLattice(const LatSpec& lat_spec, std::shared_ptr<RNG> rng_in = nullptr)
        : rng(rng_in ? std::move(rng_in) : std::make_shared<RNG>()),
          BASIS(lat_spec.basis),
          SYSTEM_SIZE(lat_spec.system_size),
          BETA(lat_spec.beta),
          DEFAULT_SPIN(lat_spec.default_spin) {
        if (BASIS != 'x' && BASIS != 'z') {
            throw std::invalid_argument("FlatSquareLattice supports basis x or z");
        }
        if (lat_spec.lattice_type != "square") {
            throw std::invalid_argument("FlatSquareLattice supports square lattices only");
        }
        if (lat_spec.boundaries != "periodic") {
            throw std::invalid_argument("FlatSquareLattice supports periodic boundaries only");
        }
        if (SYSTEM_SIZE <= 0) {
            throw std::invalid_argument("FlatSquareLattice requires positive system_size");
        }
        if (DEFAULT_SPIN != 1 && DEFAULT_SPIN != -1) {
            throw std::invalid_argument("FlatSquareLattice requires default_spin +/-1");
        }

        V = SYSTEM_SIZE * SYSTEM_SIZE;
        E = 2 * V;
        P = V;
        edges_.resize(static_cast<std::size_t>(E));
        star_edges_.resize(static_cast<std::size_t>(V));
        plaquette_edges_.resize(static_cast<std::size_t>(P));
        plaquette_vertex_pairs_.resize(static_cast<std::size_t>(P));
        star_flips_.resize(static_cast<std::size_t>(V));
        plaquette_flips_.resize(static_cast<std::size_t>(P));
        potential_star_energy_.assign(static_cast<std::size_t>(V), 0.0);
        potential_plaquette_energy_.assign(static_cast<std::size_t>(P), 0.0);

        build_square_periodic_();
        init_observable_caches_();
        init_potential_energy();
    }

    FlatSquareLattice() = default;

    int get_vertex_count() const { return V; }
    int get_edge_count() const { return E; }
    int get_plaquette_count() const { return P; }
    double get_beta() const { return BETA; }

    int get_spin(const Edge& edge) const { return edge_data_(edge).spin; }
    int get_string_count() const { return static_cast<int>((E - diag_single_energy_cache_) / 2); }

    double get_diag_single_energy() const { return static_cast<double>(diag_single_energy_cache_); }
    double get_diag_tuple_energy_x() const { return static_cast<double>(diag_tuple_energy_x_cache_); }
    double get_diag_tuple_energy_z() const { return static_cast<double>(diag_tuple_energy_z_cache_); }
    int get_anyon_count() const { return anyon_count_cache_; }
    double get_non_diag_single_energy_x() const { return static_cast<double>(total_single_spin_flip_count_) / BETA; }
    double get_non_diag_single_energy_z() const { return static_cast<double>(total_single_spin_flip_count_) / BETA; }
    double get_non_diag_tuple_energy_x() const { return static_cast<double>(total_plaquette_flip_count_) / BETA; }
    double get_non_diag_tuple_energy_z() const { return static_cast<double>(total_star_flip_count_) / BETA; }

    double percolation_probability() const { throw_unsupported_("percolation_probability"); }
    double plaquette_percolation_probability() const { throw_unsupported_("plaquette_percolation_probability"); }
    double cube_percolation_probability() const { throw_unsupported_("cube_percolation_probability"); }
    double percolation_strength() const { throw_unsupported_("percolation_strength"); }
    double plaquette_percolation_strength() const { throw_unsupported_("plaquette_percolation_strength"); }
    double largest_cluster() const { throw_unsupported_("largest_cluster"); }
    double largest_plaquette_cluster() const { throw_unsupported_("largest_plaquette_cluster"); }
    std::complex<double> fredenhagen_marcu() const { throw_unsupported_("fredenhagen_marcu"); }
    double get_staggered_imaginary_times_plaquette() const { throw_unsupported_("get_staggered_imaginary_times_plaquette"); }
    double get_staggered_imaginary_times_star() const { throw_unsupported_("get_staggered_imaginary_times_star"); }
    std::complex<double> get_diag_M_M() const { throw_unsupported_("get_diag_M_M"); }
    std::complex<double> get_diag_dynamical_M_M() const { throw_unsupported_("get_diag_dynamical_M_M"); }
    std::complex<double> get_non_diag_M_M() const { throw_unsupported_("get_non_diag_M_M"); }
    double get_kL_kR_single() const { throw_unsupported_("get_kL_kR_single"); }
    void rotate_imag_time() {
        std::uniform_real_distribution<double> new_times_dist(0.0, BETA);
        const double tau_0 = new_times_dist(*rng);

        for (Edge edge = 0; edge < E; ++edge) {
            auto& data = edge_data_(edge);
            const auto spin_pivot = std::lower_bound(data.spin_flips.begin(), data.spin_flips.end(), tau_0);
            const auto single_pivot = std::lower_bound(data.single_spin_flips.begin(), data.single_spin_flips.end(), tau_0);
            const auto pivot_index = static_cast<std::size_t>(spin_pivot - data.spin_flips.begin());
            if ((pivot_index & 1U) != 0U) {
                flip_spin(edge);
            }
            rotate_times_(data.spin_flips, spin_pivot, tau_0, BETA);
            rotate_times_(data.single_spin_flips, single_pivot, tau_0, BETA);
        }

        if (BASIS == 'x') {
            for (auto& flips : plaquette_flips_) {
                const auto pivot = std::lower_bound(flips.begin(), flips.end(), tau_0);
                rotate_times_(flips, pivot, tau_0, BETA);
            }
        } else {
            for (auto& flips : star_flips_) {
                const auto pivot = std::lower_bound(flips.begin(), flips.end(), tau_0);
                rotate_times_(flips, pivot, tau_0, BETA);
            }
        }
    }
    void update_spin_string() { throw_unsupported_("update_spin_string"); }
    void write_graph(const std::string&, const std::filesystem::path&) const { throw_unsupported_("write_graph"); }

    std::tuple<Edge, int, int> get_random_edge() {
        const Edge edge = get_random_edge_descriptor();
        const auto [source, target] = vertices_of_edge(edge);
        return {edge, source, target};
    }

    Edge get_random_edge_descriptor() {
        return static_cast<Edge>(paratoric::rng::uniform_index(*rng, static_cast<std::uint64_t>(E)));
    }

    int get_random_vertex() {
        return static_cast<int>(paratoric::rng::uniform_index(*rng, static_cast<std::uint64_t>(V)));
    }

    int get_random_plaquette_index() {
        return static_cast<int>(paratoric::rng::uniform_index(*rng, static_cast<std::uint64_t>(P)));
    }

    Edge edge_in_between(int v_1, int v_2) const {
        const auto [x1, y1] = coords_(v_1);
        const auto [x2, y2] = coords_(v_2);
        if (y1 == y2 && mod_(x1 + 1) == x2) return horizontal_edge_(x1, y1);
        if (y1 == y2 && mod_(x2 + 1) == x1) return horizontal_edge_(x2, y2);
        if (x1 == x2 && mod_(y1 + 1) == y2) return vertical_edge_(x1, y1);
        if (x1 == x2 && mod_(y2 + 1) == y1) return vertical_edge_(x2, y2);
        throw std::runtime_error(std::format("FlatSquareLattice: no edge between vertices {} and {}", v_1, v_2));
    }

    std::pair<int, int> vertices_of_edge(const Edge& edge) const {
        check_edge_(edge);
        if (edge < V) {
            const int y = edge / SYSTEM_SIZE;
            const int x = edge % SYSTEM_SIZE;
            return {vertex_(x, y), vertex_(mod_(x + 1), y)};
        }
        const int local = edge - V;
        const int y = local / SYSTEM_SIZE;
        const int x = local % SYSTEM_SIZE;
        return {vertex_(x, y), vertex_(x, mod_(y + 1))};
    }

    std::span<const Edge> get_star_edges(int center_index) const {
        const auto& edges = star_edges_[checked_index_(center_index, V)];
        return {edges.data(), edges.size()};
    }

    std::span<const Edge> get_plaquette_edges(int plaquette_index) const {
        const auto& edges = plaquette_edges_[checked_index_(plaquette_index, P)];
        return {edges.data(), edges.size()};
    }

    std::vector<VertexPair> get_plaquette_vertex_pairs(int plaquette_index) const {
        return plaquette_vertex_pairs_[checked_index_(plaquette_index, P)];
    }

    std::span<const double> get_single_spin_flips(const Edge& edge) const {
        const auto& flips = edge_data_(edge).single_spin_flips;
        return {flips.data(), flips.size()};
    }

    std::span<const double> get_tuple_spin_flips(int tuple_index) const {
        if (BASIS == 'x') {
            const auto& flips = plaquette_flips_[checked_index_(tuple_index, P)];
            return {flips.data(), flips.size()};
        }
        const auto& flips = star_flips_[checked_index_(tuple_index, V)];
        return {flips.data(), flips.size()};
    }

    int get_spin_flip_index(const Edge& edge, double tau) const {
        return checked_time_index_(edge_data_(edge).spin_flips, tau, "get_spin_flip_index");
    }

    int get_single_spin_flip_index(const Edge& edge, double tau) const {
        return checked_time_index_(edge_data_(edge).single_spin_flips, tau, "get_single_spin_flip_index");
    }

    double flip_next_imag_time(const Edge& edge, double tau) const {
        const auto& flips = edge_data_(edge).spin_flips;
        if (flips.empty()) return tau;
        const auto it = std::upper_bound(flips.begin(), flips.end(), tau);
        return it == flips.end() ? flips.front() : *it;
    }

    double flip_prev_imag_time(const Edge& edge, double tau) const {
        const auto& flips = edge_data_(edge).spin_flips;
        if (flips.empty()) return tau;
        const auto it = std::lower_bound(flips.begin(), flips.end(), tau);
        return it == flips.begin() ? flips.back() : *std::prev(it);
    }

    std::pair<double, double> tuple_flip_window(std::span<const Edge> tuple_edges, double tau) const {
        double tau_left = 0.0;
        double tau_right = BETA;
        for (const Edge edge : tuple_edges) {
            const double previous = flip_prev_imag_time(edge, tau);
            const double next = flip_next_imag_time(edge, tau);
            if (previous < tau) tau_left = std::max(tau_left, previous);
            if (next > tau) tau_right = std::min(tau_right, next);
        }
        return {tau_left, tau_right};
    }

    void insert_single_spin_flip(const Edge& edge, double tau) {
        insert_sorted_(edge_data_(edge).spin_flips, tau);
        insert_sorted_(edge_data_(edge).single_spin_flips, tau);
        ++total_single_spin_flip_count_;
    }

    void insert_double_single_spin_flip(const Edge& edge, double tau_left, double tau_right) {
        insert_single_spin_flip(edge, tau_left);
        insert_single_spin_flip(edge, tau_right);
    }

    void delete_single_spin_flip(const Edge& edge, int spin_flip_index) {
        auto& data = edge_data_(edge);
        const double tau = data.spin_flips.at(static_cast<std::size_t>(spin_flip_index));
        data.spin_flips.erase(data.spin_flips.begin() + spin_flip_index);
        erase_time_(data.single_spin_flips, tau, "delete_single_spin_flip");
        --total_single_spin_flip_count_;
    }

    void delete_double_single_spin_flip(const Edge& edge, double tau_left, double tau_right) {
        delete_time_(edge_data_(edge).spin_flips, tau_right, "delete_double_single_spin_flip");
        delete_time_(edge_data_(edge).spin_flips, tau_left, "delete_double_single_spin_flip");
        delete_time_(edge_data_(edge).single_spin_flips, tau_right, "delete_double_single_spin_flip");
        delete_time_(edge_data_(edge).single_spin_flips, tau_left, "delete_double_single_spin_flip");
        total_single_spin_flip_count_ -= 2;
    }

    void insert_tuple_flip(int tuple_index, std::span<const Edge> tuple_edges, double tau) {
        for (const Edge edge : tuple_edges) {
            insert_sorted_(edge_data_(edge).spin_flips, tau);
        }
        insert_tuple_time_(tuple_index, tau);
    }

    void insert_double_tuple_flip(int tuple_index, std::span<const Edge> tuple_edges, double tau_left, double tau_right) {
        insert_tuple_flip(tuple_index, tuple_edges, tau_left);
        insert_tuple_flip(tuple_index, tuple_edges, tau_right);
    }

    void delete_tuple_flip(int tuple_index, std::span<const Edge> tuple_edges, double tau) {
        for (const Edge edge : tuple_edges) {
            delete_time_(edge_data_(edge).spin_flips, tau, "delete_tuple_flip");
        }
        delete_tuple_time_(tuple_index, tau);
    }

    void delete_double_tuple_flip(int tuple_index, std::span<const Edge> tuple_edges, double tau_left, double tau_right) {
        delete_tuple_flip(tuple_index, tuple_edges, tau_right);
        delete_tuple_flip(tuple_index, tuple_edges, tau_left);
    }

    void move_spin_flip(const Edge& edge, int spin_flip_index, double tau_new, bool no_move_over_beta) {
        auto& data = edge_data_(edge);
        const double tau_old = data.spin_flips.at(static_cast<std::size_t>(spin_flip_index));
        if (no_move_over_beta) {
            data.spin_flips[static_cast<std::size_t>(spin_flip_index)] = tau_new;
            data.single_spin_flips[static_cast<std::size_t>(get_single_spin_flip_index(edge, tau_old))] = tau_new;
        } else {
            delete_single_spin_flip(edge, spin_flip_index);
            insert_single_spin_flip(edge, tau_new);
        }
    }

    void move_tuple_flip(int tuple_index, std::span<const Edge> tuple_edges, double tau_old, double tau_new, bool no_move_over_beta) {
        if (no_move_over_beta) {
            for (const Edge edge : tuple_edges) {
                auto& flips = edge_data_(edge).spin_flips;
                flips[static_cast<std::size_t>(checked_time_index_(flips, tau_old, "move_tuple_flip"))] = tau_new;
            }
            auto& tuple_flips = tuple_flips_(tuple_index);
            tuple_flips[static_cast<std::size_t>(checked_time_index_(tuple_flips, tau_old, "move_tuple_flip"))] = tau_new;
        } else {
            delete_tuple_flip(tuple_index, tuple_edges, tau_old);
            insert_tuple_flip(tuple_index, tuple_edges, tau_new);
        }
    }

    bool check_spin_flips_present_tuple(std::span<const Edge> tuple_edges, double tau) const {
        for (const Edge edge : tuple_edges) {
            const auto& flips = edge_data_(edge).spin_flips;
            if (std::binary_search(flips.begin(), flips.end(), tau)) return true;
        }
        return false;
    }

    void flip_spin(const Edge& edge) {
        auto& data = edge_data_(edge);
        const int old_spin = data.spin;
        diag_single_energy_cache_ += -2 * old_spin;

        for (const int star : data.star_indices) {
            const int old_product = tuple_product_(star_edges_[static_cast<std::size_t>(star)]);
            diag_tuple_energy_x_cache_ += -2 * old_product;
            if (BASIS == 'x') anyon_count_cache_ += old_product;
        }
        for (const int plaquette : data.plaquette_indices) {
            const int old_product = tuple_product_(plaquette_edges_[static_cast<std::size_t>(plaquette)]);
            diag_tuple_energy_z_cache_ += -2 * old_product;
            if (BASIS == 'z') anyon_count_cache_ += old_product;
        }

        data.spin *= -1;
    }

    double get_potential_edge_energy(const Edge& edge) const { return edge_data_(edge).integrated_edge_energy; }
    void set_potential_edge_energy(const Edge& edge, double potential_energy) { edge_data_(edge).integrated_edge_energy = potential_energy; }
    void add_potential_edge_energy(const Edge& edge, double diff) { edge_data_(edge).integrated_edge_energy += diff; }
    double get_potential_star_energy(int star_index) const { return potential_star_energy_[checked_index_(star_index, V)]; }
    void set_potential_star_energy(int star_index, double potential_energy) { potential_star_energy_[checked_index_(star_index, V)] = potential_energy; }
    void add_potential_star_energy(int star_index, double diff) { potential_star_energy_[checked_index_(star_index, V)] += diff; }
    double get_potential_plaquette_energy(int plaquette_index) const { return potential_plaquette_energy_[checked_index_(plaquette_index, P)]; }
    void set_potential_plaquette_energy(int plaquette_index, double potential_energy) { potential_plaquette_energy_[checked_index_(plaquette_index, P)] = potential_energy; }
    void add_potential_plaquette_energy(int plaquette_index, double diff) { potential_plaquette_energy_[checked_index_(plaquette_index, P)] += diff; }

    double integrated_edge_energy(const Edge& edge, double tau_1, double tau_2) const {
        if (tau_1 == tau_2) throw std::invalid_argument("integrated_edge_energy: zero interval");
        if (tau_2 < tau_1) return integrated_edge_energy(edge, tau_1, BETA) + integrated_edge_energy(edge, 0.0, tau_2);
        return integrate_piecewise_(edge_data_(edge).spin, edge_data_(edge).spin_flips, tau_1, tau_2);
    }

    double integrated_edge_energy_diff(const Edge& edge, double tau_1, double tau_2) const {
        return -2.0 * integrated_edge_energy(edge, tau_1, tau_2);
    }

    double integrated_edge_energy_diff_no_inner_flips(const Edge& edge, double tau_1, double tau_2) const {
        return integrated_edge_energy_diff(edge, tau_1, tau_2);
    }

    double integrated_edge_energy_diff_combination(const Edge& edge, double tau_1, double tau_2, double flip_1, double flip_2) const {
        std::vector<std::pair<double, int>> lookup{{flip_1, 0}, {flip_2, 0}};
        std::sort(lookup.begin(), lookup.end());
        return integrated_edge_energy_diff_combination(edge, tau_1, tau_2, lookup);
    }

    double integrated_edge_energy_diff_combination(const Edge& edge, double tau_1, double tau_2, std::vector<std::pair<double, int>>& spin_flip_lookup) const {
        if (tau_1 == tau_2) throw std::invalid_argument("integrated_edge_energy_diff_combination: zero interval");
        if (tau_2 < tau_1) {
            return integrated_edge_energy_diff_combination(edge, tau_1, BETA, spin_flip_lookup)
                 + integrated_edge_energy_diff_combination(edge, 0.0, tau_2, spin_flip_lookup);
        }

        std::sort(spin_flip_lookup.begin(), spin_flip_lookup.end());
        double odd_integral = 0.0;
        bool odd = false;
        double previous = tau_1;
        for (std::size_t i = 0; i < spin_flip_lookup.size();) {
            const double tau = spin_flip_lookup[i].first;
            if (tau < tau_1) {
                ++i;
                continue;
            }
            if (tau >= tau_2) break;
            if (odd && previous < tau) {
                odd_integral += integrated_edge_energy(edge, previous, tau);
            }
            bool toggles = false;
            do {
                toggles = !toggles;
                ++i;
            } while (i < spin_flip_lookup.size() && spin_flip_lookup[i].first == tau);
            if (toggles) odd = !odd;
            previous = tau;
        }
        if (odd && previous < tau_2) {
            odd_integral += integrated_edge_energy(edge, previous, tau_2);
        }
        return -2.0 * odd_integral;
    }

    double integrated_tuple_energy(std::span<const Edge> tuple_edges, double tau_1, double tau_2) const {
        return integrated_tuple_energy_from_flips_(tuple_edges, tau_1, tau_2, false);
    }

    double integrated_tuple_energy_single_flips(std::span<const Edge> tuple_edges, double tau_1, double tau_2) const {
        return integrated_tuple_energy_from_flips_(tuple_edges, tau_1, tau_2, true);
    }

    double integrated_tuple_energy_diff(std::span<const Edge> tuple_edges, double tau_1, double tau_2) const {
        return -2.0 * integrated_tuple_energy(tuple_edges, tau_1, tau_2);
    }

    double integrated_tuple_energy_diff_single_flips(std::span<const Edge> tuple_edges, double tau_1, double tau_2) const {
        return -2.0 * integrated_tuple_energy_single_flips(tuple_edges, tau_1, tau_2);
    }

    std::tuple<double, SmallIndexVector, SmallEnergyVector> integrated_star_energy_diff(
        const Edge& edge, double tau_1, double tau_2, bool total_cache
    ) const {
        SmallIndexVector star_indices;
        SmallEnergyVector diffs;
        double energy = 0.0;
        for (const int star : edge_data_(edge).star_indices) {
            const double diff = total_cache
                ? -2.0 * get_potential_star_energy(star)
                : (BASIS == 'x'
                    ? integrated_tuple_energy_diff_single_flips(get_star_edges(star), tau_1, tau_2)
                    : integrated_tuple_energy_diff(get_star_edges(star), tau_1, tau_2));
            star_indices.emplace_back(star);
            diffs.emplace_back(diff);
            energy += diff;
        }
        return {energy, std::move(star_indices), std::move(diffs)};
    }

    std::tuple<double, SmallIndexVector, SmallEnergyVector> integrated_plaquette_energy_diff(
        const Edge& edge, double tau_1, double tau_2, bool total_cache
    ) const {
        SmallIndexVector plaquette_indices;
        SmallEnergyVector diffs;
        double energy = 0.0;
        for (const int plaquette : edge_data_(edge).plaquette_indices) {
            const double diff = total_cache
                ? -2.0 * get_potential_plaquette_energy(plaquette)
                : (BASIS == 'z'
                    ? integrated_tuple_energy_diff_single_flips(get_plaquette_edges(plaquette), tau_1, tau_2)
                    : integrated_tuple_energy_diff(get_plaquette_edges(plaquette), tau_1, tau_2));
            plaquette_indices.emplace_back(plaquette);
            diffs.emplace_back(diff);
            energy += diff;
        }
        return {energy, std::move(plaquette_indices), std::move(diffs)};
    }

    double total_integrated_edge_energy() const {
        double energy = 0.0;
        for (Edge edge = 0; edge < E; ++edge) {
            energy += integrated_edge_energy(edge, 0.0, BETA);
        }
        return energy;
    }

    double total_integrated_star_energy() const {
        double energy = 0.0;
        for (int star = 0; star < V; ++star) {
            energy += (BASIS == 'x')
                ? integrated_tuple_energy_single_flips(get_star_edges(star), 0.0, BETA)
                : integrated_tuple_energy(get_star_edges(star), 0.0, BETA);
        }
        return energy;
    }

    double total_integrated_plaquette_energy() const {
        double energy = 0.0;
        for (int plaquette = 0; plaquette < P; ++plaquette) {
            energy += (BASIS == 'z')
                ? integrated_tuple_energy_single_flips(get_plaquette_edges(plaquette), 0.0, BETA)
                : integrated_tuple_energy(get_plaquette_edges(plaquette), 0.0, BETA);
        }
        return energy;
    }

    std::tuple<double, SmallIndexVector, SmallEnergyVector> integrated_star_energy_diff_combination(
        int plaquette_index,
        double tau_1,
        double tau_2,
        std::span<const double> spin_flip_lookup,
        double tuple_tau
    ) const {
        if (tau_1 == tau_2) throw std::invalid_argument("integrated_star_energy_diff_combination: zero interval");

        const auto plaquette_edges = get_plaquette_edges(plaquette_index);
        SmallIndexVector star_centers;
        star_centers.reserve(4);
        for (const Edge edge : plaquette_edges) {
            const auto [source, target] = vertices_of_edge(edge);
            star_centers.emplace_back(source);
            star_centers.emplace_back(target);
        }
        std::sort(star_centers.begin(), star_centers.end());
        star_centers.erase(std::unique(star_centers.begin(), star_centers.end()), star_centers.end());

        SmallEnergyVector diffs;
        diffs.reserve(star_centers.size());
        double energy = 0.0;
        std::vector<std::pair<double, int>> local_flips;
        local_flips.reserve(1 + plaquette_edges.size());
        for (const int star : star_centers) {
            local_flips.clear();
            insert_sorted_event_(local_flips, tuple_tau, 1);
            for (std::size_t i = 0; i < plaquette_edges.size(); ++i) {
                const auto [source, target] = vertices_of_edge(plaquette_edges[i]);
                if (source == star || target == star) {
                    insert_sorted_event_(local_flips, spin_flip_lookup[i], 0);
                }
            }
            const double diff = integrated_tuple_energy_diff_combination_from_flips_(
                get_star_edges(star), tau_1, tau_2, local_flips, BASIS == 'x'
            );
            diffs.emplace_back(diff);
            energy += diff;
        }
        return {energy, std::move(star_centers), std::move(diffs)};
    }

    std::tuple<double, SmallIndexVector, SmallEnergyVector> integrated_plaquette_energy_diff_combination(
        int star_index,
        double tau_1,
        double tau_2,
        std::span<const double> spin_flip_lookup,
        double tuple_tau
    ) const {
        if (tau_1 == tau_2) throw std::invalid_argument("integrated_plaquette_energy_diff_combination: zero interval");

        const auto star_edges = get_star_edges(star_index);
        SmallIndexVector plaquettes;
        plaquettes.reserve(4);
        for (const Edge edge : star_edges) {
            for (const int plaquette : edge_data_(edge).plaquette_indices) {
                plaquettes.emplace_back(plaquette);
            }
        }
        std::sort(plaquettes.begin(), plaquettes.end());
        plaquettes.erase(std::unique(plaquettes.begin(), plaquettes.end()), plaquettes.end());

        SmallEnergyVector diffs;
        diffs.reserve(plaquettes.size());
        double energy = 0.0;
        std::vector<std::pair<double, int>> local_flips;
        local_flips.reserve(1 + star_edges.size());
        for (const int plaquette : plaquettes) {
            local_flips.clear();
            insert_sorted_event_(local_flips, tuple_tau, 1);
            for (std::size_t i = 0; i < star_edges.size(); ++i) {
                const auto& edge_plaquettes = edge_data_(star_edges[i]).plaquette_indices;
                if (edge_plaquettes[0] == plaquette || edge_plaquettes[1] == plaquette) {
                    insert_sorted_event_(local_flips, spin_flip_lookup[i], 0);
                }
            }
            const double diff = integrated_tuple_energy_diff_combination_from_flips_(
                get_plaquette_edges(plaquette), tau_1, tau_2, local_flips, BASIS == 'z'
            );
            diffs.emplace_back(diff);
            energy += diff;
        }
        return {energy, std::move(plaquettes), std::move(diffs)};
    }

    void init_potential_energy() {
        for (Edge edge = 0; edge < E; ++edge) {
            set_potential_edge_energy(edge, integrated_edge_energy(edge, 0.0, BETA));
        }
        for (int star = 0; star < V; ++star) {
            set_potential_star_energy(star, integrated_tuple_energy_single_flips(get_star_edges(star), 0.0, BETA));
        }
        for (int plaquette = 0; plaquette < P; ++plaquette) {
            set_potential_plaquette_energy(plaquette, integrated_tuple_energy_single_flips(get_plaquette_edges(plaquette), 0.0, BETA));
        }
    }

private:
    struct EdgeData {
        int spin = 1;
        std::vector<double> spin_flips;
        std::vector<double> single_spin_flips;
        double integrated_edge_energy = 0.0;
        std::array<int, 2> star_indices{};
        std::array<int, 2> plaquette_indices{};
    };

    std::shared_ptr<RNG> rng{std::make_shared<RNG>()};
    char BASIS = 'x';
    int SYSTEM_SIZE = 0;
    double BETA = 0.0;
    int DEFAULT_SPIN = 1;
    int V = 0;
    int E = 0;
    int P = 0;
    std::vector<EdgeData> edges_;
    std::vector<std::array<Edge, 4>> star_edges_;
    std::vector<std::array<Edge, 4>> plaquette_edges_;
    std::vector<std::vector<VertexPair>> plaquette_vertex_pairs_;
    std::vector<std::vector<double>> star_flips_;
    std::vector<std::vector<double>> plaquette_flips_;
    std::vector<double> potential_star_energy_;
    std::vector<double> potential_plaquette_energy_;
    int diag_single_energy_cache_ = 0;
    int diag_tuple_energy_x_cache_ = 0;
    int diag_tuple_energy_z_cache_ = 0;
    int anyon_count_cache_ = 0;
    std::int64_t total_single_spin_flip_count_ = 0;
    std::int64_t total_plaquette_flip_count_ = 0;
    std::int64_t total_star_flip_count_ = 0;

    [[noreturn]] static void throw_unsupported_(const char* method) {
        throw std::runtime_error(std::format("FlatSquareLattice does not support {} yet", method));
    }

    int mod_(int value) const {
        value %= SYSTEM_SIZE;
        return value < 0 ? value + SYSTEM_SIZE : value;
    }

    int vertex_(int x, int y) const { return y * SYSTEM_SIZE + x; }
    std::pair<int, int> coords_(int vertex) const { return {vertex % SYSTEM_SIZE, vertex / SYSTEM_SIZE}; }
    Edge horizontal_edge_(int x, int y) const { return y * SYSTEM_SIZE + x; }
    Edge vertical_edge_(int x, int y) const { return V + y * SYSTEM_SIZE + x; }

    static void insert_sorted_(std::vector<double>& values, double tau) {
        values.insert(std::upper_bound(values.begin(), values.end(), tau), tau);
    }

    static double modulo_time_(double value, double period) {
        value = std::fmod(value, period);
        return value < 0.0 ? value + period : value;
    }

    static void rotate_times_(
        std::vector<double>& values,
        std::vector<double>::iterator pivot,
        double tau_0,
        double beta
    ) {
        std::rotate(values.begin(), pivot, values.end());
        for (double& tau : values) {
            tau = modulo_time_(tau - tau_0, beta);
            if (tau == 0.0) tau += std::numeric_limits<double>::epsilon();
        }
    }

    static void insert_sorted_event_(std::vector<std::pair<double, int>>& values, double tau, int type) {
        values.insert(
            std::upper_bound(
                values.begin(),
                values.end(),
                std::pair<double, int>{tau, type},
                [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; }
            ),
            {tau, type}
        );
    }

    static void erase_time_(std::vector<double>& values, double tau, const char* context) {
        const auto it = std::lower_bound(values.begin(), values.end(), tau);
        if (it == values.end() || *it != tau) {
            throw std::runtime_error(std::format("{}: missing flip at {}", context, tau));
        }
        values.erase(it);
    }

    static void delete_time_(std::vector<double>& values, double tau, const char* context) {
        erase_time_(values, tau, context);
    }

    static int checked_time_index_(const std::vector<double>& values, double tau, const char* context) {
        const auto it = std::lower_bound(values.begin(), values.end(), tau);
        if (it == values.end() || *it != tau) {
            throw std::runtime_error(std::format("{}: missing flip at {}", context, tau));
        }
        return static_cast<int>(it - values.begin());
    }

    std::size_t checked_index_(int index, int size) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range(std::format("FlatSquareLattice index {} out of range {}", index, size));
        }
        return static_cast<std::size_t>(index);
    }

    void check_edge_(Edge edge) const {
        if (edge < 0 || edge >= E) {
            throw std::out_of_range(std::format("FlatSquareLattice edge {} out of range {}", edge, E));
        }
    }

    EdgeData& edge_data_(Edge edge) {
        check_edge_(edge);
        return edges_[static_cast<std::size_t>(edge)];
    }

    const EdgeData& edge_data_(Edge edge) const {
        check_edge_(edge);
        return edges_[static_cast<std::size_t>(edge)];
    }

    std::vector<double>& tuple_flips_(int tuple_index) {
        return BASIS == 'x'
            ? plaquette_flips_[checked_index_(tuple_index, P)]
            : star_flips_[checked_index_(tuple_index, V)];
    }

    const std::vector<double>& tuple_flips_(int tuple_index) const {
        return BASIS == 'x'
            ? plaquette_flips_[checked_index_(tuple_index, P)]
            : star_flips_[checked_index_(tuple_index, V)];
    }

    void insert_tuple_time_(int tuple_index, double tau) {
        insert_sorted_(tuple_flips_(tuple_index), tau);
        if (BASIS == 'x') ++total_plaquette_flip_count_;
        else ++total_star_flip_count_;
    }

    void delete_tuple_time_(int tuple_index, double tau) {
        delete_time_(tuple_flips_(tuple_index), tau, "delete_tuple_flip");
        if (BASIS == 'x') --total_plaquette_flip_count_;
        else --total_star_flip_count_;
    }

    void build_square_periodic_() {
        for (int y = 0; y < SYSTEM_SIZE; ++y) {
            for (int x = 0; x < SYSTEM_SIZE; ++x) {
                const int v = vertex_(x, y);
                star_edges_[static_cast<std::size_t>(v)] = {
                    horizontal_edge_(x, y),
                    horizontal_edge_(mod_(x - 1), y),
                    vertical_edge_(x, y),
                    vertical_edge_(x, mod_(y - 1))
                };

                const int p = v;
                plaquette_edges_[static_cast<std::size_t>(p)] = {
                    horizontal_edge_(x, y),
                    horizontal_edge_(x, mod_(y + 1)),
                    vertical_edge_(x, y),
                    vertical_edge_(mod_(x + 1), y)
                };
                plaquette_vertex_pairs_[static_cast<std::size_t>(p)] = {
                    {vertex_(x, y), vertex_(mod_(x + 1), y)},
                    {vertex_(mod_(x + 1), y), vertex_(mod_(x + 1), mod_(y + 1))},
                    {vertex_(mod_(x + 1), mod_(y + 1)), vertex_(x, mod_(y + 1))},
                    {vertex_(x, mod_(y + 1)), vertex_(x, y)}
                };

                const Edge h = horizontal_edge_(x, y);
                edges_[static_cast<std::size_t>(h)].spin = DEFAULT_SPIN;
                edges_[static_cast<std::size_t>(h)].star_indices = {vertex_(x, y), vertex_(mod_(x + 1), y)};
                edges_[static_cast<std::size_t>(h)].plaquette_indices = {vertex_(x, y), vertex_(x, mod_(y - 1))};

                const Edge vert = vertical_edge_(x, y);
                edges_[static_cast<std::size_t>(vert)].spin = DEFAULT_SPIN;
                edges_[static_cast<std::size_t>(vert)].star_indices = {vertex_(x, y), vertex_(x, mod_(y + 1))};
                edges_[static_cast<std::size_t>(vert)].plaquette_indices = {vertex_(x, y), vertex_(mod_(x - 1), y)};
            }
        }
    }

    int tuple_product_(const std::array<Edge, 4>& tuple_edges) const {
        int product = 1;
        for (const Edge edge : tuple_edges) {
            product *= edge_data_(edge).spin;
        }
        return product;
    }

    void init_observable_caches_() {
        diag_single_energy_cache_ = 0;
        diag_tuple_energy_x_cache_ = 0;
        diag_tuple_energy_z_cache_ = 0;
        anyon_count_cache_ = 0;
        for (const auto& edge : edges_) {
            diag_single_energy_cache_ += edge.spin;
        }
        for (const auto& star : star_edges_) {
            const int product = tuple_product_(star);
            diag_tuple_energy_x_cache_ += product;
            if (BASIS == 'x' && product == -1) ++anyon_count_cache_;
        }
        for (const auto& plaquette : plaquette_edges_) {
            const int product = tuple_product_(plaquette);
            diag_tuple_energy_z_cache_ += product;
            if (BASIS == 'z' && product == -1) ++anyon_count_cache_;
        }
    }

    double integrate_piecewise_(int base_spin, const std::vector<double>& flips, double tau_1, double tau_2) const {
        auto lo = std::lower_bound(flips.begin(), flips.end(), tau_1);
        auto hi = std::upper_bound(lo, flips.end(), tau_2);
        int spin = ((lo - flips.begin()) & 1) ? -base_spin : base_spin;
        double energy = 0.0;
        double previous = tau_1;
        for (auto it = lo; it != hi; ++it) {
            energy += (*it - previous) * spin;
            spin *= -1;
            previous = *it;
        }
        energy += (tau_2 - previous) * spin;
        return energy;
    }

    double integrated_tuple_energy_from_flips_(std::span<const Edge> tuple_edges, double tau_1, double tau_2, bool single_only) const {
        if (tau_1 == tau_2) throw std::invalid_argument("integrated_tuple_energy: zero interval");
        if (tau_2 < tau_1) {
            return integrated_tuple_energy_from_flips_(tuple_edges, tau_1, BETA, single_only)
                 + integrated_tuple_energy_from_flips_(tuple_edges, 0.0, tau_2, single_only);
        }

        std::vector<double> times;
        times.reserve(16);
        for (const Edge edge : tuple_edges) {
            const auto& flips = single_only ? edge_data_(edge).single_spin_flips : edge_data_(edge).spin_flips;
            const auto lo = std::lower_bound(flips.begin(), flips.end(), tau_1);
            const auto hi = std::upper_bound(lo, flips.end(), tau_2);
            times.insert(times.end(), lo, hi);
        }
        std::sort(times.begin(), times.end());
        times.erase(std::unique(times.begin(), times.end()), times.end());

        int product = 1;
        for (const Edge edge : tuple_edges) {
            const auto& flips = single_only ? edge_data_(edge).single_spin_flips : edge_data_(edge).spin_flips;
            const auto lo = std::lower_bound(flips.begin(), flips.end(), tau_1);
            product *= ((lo - flips.begin()) & 1) ? -edge_data_(edge).spin : edge_data_(edge).spin;
        }

        double energy = 0.0;
        double previous = tau_1;
        for (const double tau : times) {
            energy += (tau - previous) * product;
            for (const Edge edge : tuple_edges) {
                const auto& flips = single_only ? edge_data_(edge).single_spin_flips : edge_data_(edge).spin_flips;
                const auto range = std::equal_range(flips.begin(), flips.end(), tau);
                if (((range.second - range.first) & 1) != 0) {
                    product *= -1;
                }
            }
            previous = tau;
        }
        energy += (tau_2 - previous) * product;
        return energy;
    }

    double integrated_tuple_energy_diff_combination_from_flips_(
        std::span<const Edge> tuple_edges,
        double tau_1,
        double tau_2,
        const std::vector<std::pair<double, int>>& spin_flip_lookup,
        bool single_only
    ) const {
        if (spin_flip_lookup.empty()) {
            return 0.0;
        }
        if (tau_2 < tau_1) {
            return integrated_tuple_energy_diff_combination_from_flips_(tuple_edges, tau_1, BETA, spin_flip_lookup, single_only)
                 + integrated_tuple_energy_diff_combination_from_flips_(tuple_edges, 0.0, tau_2, spin_flip_lookup, single_only);
        }

        const bool tuple_flip_toggles = ((spin_flip_lookup.size() & 1) == 0);
        auto event_toggles = [&](const std::pair<double, int>& event) {
            return (event.second != 1) || tuple_flip_toggles;
        };

        bool odd = false;
        std::size_t index = 0;
        while (index < spin_flip_lookup.size() && spin_flip_lookup[index].first < tau_1) {
            const double tau = spin_flip_lookup[index].first;
            bool toggles = false;
            do {
                if (event_toggles(spin_flip_lookup[index])) toggles = !toggles;
                ++index;
            } while (index < spin_flip_lookup.size() && spin_flip_lookup[index].first == tau);
            if (toggles) odd = !odd;
        }

        double odd_integral = 0.0;
        double previous = tau_1;
        while (index < spin_flip_lookup.size()) {
            const double tau = spin_flip_lookup[index].first;
            if (tau >= tau_2) {
                break;
            }
            if (odd && previous < tau) {
                odd_integral += integrated_tuple_energy_from_flips_(tuple_edges, previous, tau, single_only);
            }

            bool toggles = false;
            do {
                if (event_toggles(spin_flip_lookup[index])) toggles = !toggles;
                ++index;
            } while (index < spin_flip_lookup.size() && spin_flip_lookup[index].first == tau);
            if (toggles) odd = !odd;
            previous = tau;
        }
        if (odd && previous < tau_2) {
            odd_integral += integrated_tuple_energy_from_flips_(tuple_edges, previous, tau_2, single_only);
        }

        return -2.0 * odd_integral;
    }
};

} // namespace paratoric
