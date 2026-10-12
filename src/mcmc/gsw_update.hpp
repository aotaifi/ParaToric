// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include "lattice/lattice.hpp"
#include "mcmc/gf2_sampler.hpp"
#include "rng/rng.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace paratoric {

/**
 * @brief Continuous-time generalised Swendsen-Wang update for the extended toric code in the sigma^z basis.
 *
 * H = -mu sum A_s - J sum B_p - h sum sigma^x - lmbda sum sigma^z. All four terms get an Edwards-Sokal
 * decomposition:
 *   h     cut points on every edge = its kinks plus a Poisson(h) process; a cut may or may not carry a kink
 *   mu    cut points on every vertex = its star events plus a Poisson(mu) process; idem
 *   lmbda bond points on every edge, Poisson(2 lmbda) on the stretches with sigma^z = +1; sigma^z = +1 is pinned there
 *   J     bond points on every plaquette, Poisson(2 J) on the stretches with B_p = +1; B_p = +1 is pinned there
 * Given the cuts and bonds every admissible configuration has the same weight, so the new configuration is a
 * uniformly random solution of a linear system over GF(2). Variables: s (segments of an edge between its cuts),
 * g (segments of a vertex between its cuts) and a twist bit t_v per vertex (odd number of star events at v).
 * sigma^z(e, tau) = s_e + g_u + g_w (mod 2), the twist of an edge is t_u + t_w, and a twisted timeline flips its
 * value on [0, first cut). The result replaces all kinks, star events and the spins at tau = 0; the caller is
 * responsible for refreshing whatever it caches (Lattice::gsw_rebuild_derived() already refreshes the lattice's
 * own caches and integrated energies). Geometry comes from the lattice's own plaquette and star edge lists, so
 * open boundaries work as well. Reference implementation and validation history: tools/esworm/gsw.js.
 */
class GswUpdater {
public:
    struct Stats {
        int variables = 0;
        int rows = 0;
        int rank = 0;
        int core_rows = 0;
        int core_vars = 0;
    };

    // Multiplies the J bond rate. Must stay 1; the validation uses 0.5 as a deliberately broken arm.
    double j_bond_rate_factor = 1.0;
    // Test hook: check before solving that the current configuration satisfies the system that was built.
    bool verify_current_state = false;
    // Linear solver: dense elimination of the whole core, or nested-dissection order (see gf2_sampler.hpp).
    gf2::Solver solver = gf2::Solver::dense;
    int nd_leaf = 64;
    int nd_dense_vars = 4096;
    int nd_dense_rows = 0;
    const gf2::UniformSolutionSampler& linear_solver() const { return solver_; }

    // Wall-clock seconds accumulated in the three phases, and the number of steps.
    double seconds_build = 0.;
    double seconds_solve = 0.;
    double seconds_rebuild = 0.;
    std::uint64_t steps = 0;
    std::uint64_t core_rows_sum = 0;
    std::uint64_t variables_sum = 0;

    Stats step(Lattice& lat, paratoric::rng::RNG& rng, double beta, double h, double mu, double J, double lmbda) {
        using clock = std::chrono::steady_clock;
        const auto t_begin = clock::now();
        const auto& topo = lat.gsw_topology();
        const int nV = topo.vertex_count;
        const int nE = topo.edge_count;
        const int nP = topo.plaquette_count;

        // ---- cut points and variable layout
        vcut_off_.assign(static_cast<std::size_t>(nV) + 1, 0);
        vcuts_.clear();
        for (int v = 0; v < nV; ++v) {
            append_cuts(lat.gsw_star_events(v), mu, beta, rng, vcuts_);
            vcut_off_[static_cast<std::size_t>(v) + 1] = static_cast<std::int32_t>(vcuts_.size());
        }
        ecut_off_.assign(static_cast<std::size_t>(nE) + 1, 0);
        ecuts_.clear();
        for (int e = 0; e < nE; ++e) {
            append_cuts(lat.gsw_kinks(e), h, beta, rng, ecuts_);
            ecut_off_[static_cast<std::size_t>(e) + 1] = static_cast<std::int32_t>(ecuts_.size());
        }
        g_off_.resize(static_cast<std::size_t>(nV));
        s_off_.resize(static_cast<std::size_t>(nE));
        int n_var = 0;
        for (int v = 0; v < nV; ++v) {
            g_off_[static_cast<std::size_t>(v)] = n_var;
            n_var += std::max(1, vcut_count(v));
        }
        for (int e = 0; e < nE; ++e) {
            s_off_[static_cast<std::size_t>(e)] = n_var;
            n_var += std::max(1, ecut_count(e));
        }
        t_off_ = n_var;
        n_var += nV;

        // ---- equations
        solver_.solver = solver;
        solver_.nd_leaf = nd_leaf;
        solver_.nd_dense_vars = nd_dense_vars;
        solver_.nd_dense_rows = nd_dense_rows;
        solver_.reset(n_var);
        if (solver == gf2::Solver::nested_dissection) {
            assign_coordinates(topo, beta);
        }
        if (lmbda > 0.) {
            const double rate = 2. * lmbda;
            for (int e = 0; e < nE; ++e) {
                const int u = topo.edge_source[static_cast<std::size_t>(e)];
                const int w = topo.edge_target[static_cast<std::size_t>(e)];
                const auto& flips = lat.gsw_all_flips(e);
                bool plus = lat.gsw_spin0(e) == 1;
                double t0 = 0.;
                for (std::size_t i = 0; i <= flips.size(); ++i) {
                    const double t1 = i < flips.size() ? flips[i] : beta;
                    if (plus) {
                        for (double t = t0 + exp_rand(rng, rate); t < t1; t += exp_rand(rng, rate)) {
                            solver_.add(edge_segment(e, t));
                            solver_.add(vertex_segment(u, t));
                            solver_.add(vertex_segment(w, t));
                            if (is_pre(ecuts_, ecut_off_, e, t)) {
                                solver_.add(t_off_ + u);
                                solver_.add(t_off_ + w);
                            }
                            if (is_pre(vcuts_, vcut_off_, u, t)) {
                                solver_.add(t_off_ + u);
                            }
                            if (is_pre(vcuts_, vcut_off_, w, t)) {
                                solver_.add(t_off_ + w);
                            }
                            solver_.end_row();
                        }
                    }
                    plus = !plus;
                    t0 = t1;
                }
            }
        }
        if (J > 0.) {
            const double rate = 2. * J * j_bond_rate_factor;
            for (int p = 0; p < nP; ++p) {
                const int first = topo.plaquette_offset[static_cast<std::size_t>(p)];
                const int last = topo.plaquette_offset[static_cast<std::size_t>(p) + 1];
                plaquette_flips_.clear();
                int product = 1;
                for (int k = first; k < last; ++k) {
                    const int e = topo.plaquette_edge[static_cast<std::size_t>(k)];
                    product *= lat.gsw_spin0(e);
                    const auto& kinks = lat.gsw_kinks(e);
                    plaquette_flips_.insert(plaquette_flips_.end(), kinks.begin(), kinks.end());
                }
                std::sort(plaquette_flips_.begin(), plaquette_flips_.end());
                bool plus = product == 1;
                double t0 = 0.;
                for (std::size_t i = 0; i <= plaquette_flips_.size(); ++i) {
                    const double t1 = i < plaquette_flips_.size() ? plaquette_flips_[i] : beta;
                    if (plus) {
                        for (double t = t0 + exp_rand(rng, rate); t < t1; t += exp_rand(rng, rate)) {
                            for (int k = first; k < last; ++k) {
                                const int e = topo.plaquette_edge[static_cast<std::size_t>(k)];
                                solver_.add(edge_segment(e, t));
                                if (is_pre(ecuts_, ecut_off_, e, t)) {
                                    solver_.add(t_off_ + topo.edge_source[static_cast<std::size_t>(e)]);
                                    solver_.add(t_off_ + topo.edge_target[static_cast<std::size_t>(e)]);
                                }
                            }
                            solver_.end_row();
                        }
                    }
                    plus = !plus;
                    t0 = t1;
                }
            }
        }
        // a twist needs a cut: a vertex without cuts has t_v = 0, an edge without cuts has t_u + t_w = 0
        for (int v = 0; v < nV; ++v) {
            if (vcut_count(v) == 0) {
                solver_.add(t_off_ + v);
                solver_.end_row();
            }
        }
        for (int e = 0; e < nE; ++e) {
            if (ecut_count(e) == 0) {
                solver_.add(t_off_ + topo.edge_source[static_cast<std::size_t>(e)]);
                solver_.add(t_off_ + topo.edge_target[static_cast<std::size_t>(e)]);
                solver_.end_row();
            }
        }
        if (verify_current_state) {
            current_state(lat, topo);
            if (!solver_.satisfied_by(x_)) {
                throw std::logic_error("gSW: the current configuration violates the constraint system it generated.");
            }
        }
        const auto t_built = clock::now();

        // ---- uniform solution
        const auto sol = solver_.sample(rng, x_);
        const auto t_solved = clock::now();

        // ---- write the new configuration back
        const auto& x = x_;
        auto twist = [&](int v) { return x[static_cast<std::size_t>(t_off_ + v)]; };
        for (int v = 0; v < nV; ++v) {
            auto& events = lat.gsw_star_events(v);
            events.clear();
            const int m = vcut_count(v);
            if (m == 0) {
                continue;
            }
            const double* c = vcuts_.data() + vcut_off_[static_cast<std::size_t>(v)];
            const int off = g_off_[static_cast<std::size_t>(v)];
            for (int k = 0; k < m; ++k) {
                const std::uint8_t before = k == 0 ? static_cast<std::uint8_t>(x[static_cast<std::size_t>(off + m - 1)] ^ twist(v))
                                                    : x[static_cast<std::size_t>(off + k - 1)];
                if (before != x[static_cast<std::size_t>(off + k)]) {
                    events.push_back(c[k]);
                }
            }
        }
        auto vertex_at_zero = [&](int v) -> std::uint8_t {
            const int m = vcut_count(v);
            const int off = g_off_[static_cast<std::size_t>(v)];
            return m == 0 ? x[static_cast<std::size_t>(off)]
                          : static_cast<std::uint8_t>(x[static_cast<std::size_t>(off + m - 1)] ^ twist(v));
        };
        for (int e = 0; e < nE; ++e) {
            const int u = topo.edge_source[static_cast<std::size_t>(e)];
            const int w = topo.edge_target[static_cast<std::size_t>(e)];
            auto& kinks = lat.gsw_kinks(e);
            kinks.clear();
            const int m = ecut_count(e);
            const int off = s_off_[static_cast<std::size_t>(e)];
            std::uint8_t s0 = 0;
            if (m > 0) {
                const double* c = ecuts_.data() + ecut_off_[static_cast<std::size_t>(e)];
                s0 = static_cast<std::uint8_t>(x[static_cast<std::size_t>(off + m - 1)] ^ twist(u) ^ twist(w));
                for (int k = 0; k < m; ++k) {
                    const std::uint8_t before = k == 0 ? s0 : x[static_cast<std::size_t>(off + k - 1)];
                    if (before != x[static_cast<std::size_t>(off + k)]) {
                        kinks.push_back(c[k]);
                    }
                }
            } else {
                s0 = x[static_cast<std::size_t>(off)];
            }
            lat.gsw_set_spin0(e, (s0 ^ vertex_at_zero(u) ^ vertex_at_zero(w)) != 0 ? -1 : 1);
        }
        lat.gsw_rebuild_derived();
        const auto t_done = clock::now();

        seconds_build += std::chrono::duration<double>(t_built - t_begin).count();
        seconds_solve += std::chrono::duration<double>(t_solved - t_built).count();
        seconds_rebuild += std::chrono::duration<double>(t_done - t_solved).count();
        ++steps;
        core_rows_sum += static_cast<std::uint64_t>(sol.core_rows);
        variables_sum += static_cast<std::uint64_t>(n_var);
        return Stats{n_var, sol.rows, sol.rank, sol.core_rows, sol.core_vars};
    }

private:
    static double exp_rand(paratoric::rng::RNG& rng, double rate) {
        // u in (0, 1): never 0, so the waiting time is finite and positive
        const double u = (static_cast<double>(rng() >> 11) + 0.5) * 0x1.0p-53;
        return -std::log(u) / rate;
    }

    // base (sorted) merged with a Poisson process of the given rate on (0, beta), appended to out
    void append_cuts(const std::vector<double>& base, double rate, double beta, paratoric::rng::RNG& rng,
                     std::vector<double>& out) {
        poisson_.clear();
        if (rate > 0.) {
            for (double t = exp_rand(rng, rate); t < beta; t += exp_rand(rng, rate)) {
                poisson_.push_back(t);
            }
        }
        std::merge(base.begin(), base.end(), poisson_.begin(), poisson_.end(), std::back_inserter(out));
    }

    int vcut_count(int v) const { return vcut_off_[static_cast<std::size_t>(v) + 1] - vcut_off_[static_cast<std::size_t>(v)]; }
    int ecut_count(int e) const { return ecut_off_[static_cast<std::size_t>(e) + 1] - ecut_off_[static_cast<std::size_t>(e)]; }

    // Segment k of a timeline with m cuts spans [c[k], c[k+1]); the last one wraps: [c[m-1], beta) and [0, c[0]).
    static int segment_index(const double* c, int m, double tau) {
        if (m == 0) {
            return 0;
        }
        const int k = static_cast<int>(std::upper_bound(c, c + m, tau) - c);  // number of cuts <= tau
        return k == 0 ? m - 1 : k - 1;
    }
    int edge_segment(int e, double tau) const {
        return s_off_[static_cast<std::size_t>(e)] +
               segment_index(ecuts_.data() + ecut_off_[static_cast<std::size_t>(e)], ecut_count(e), tau);
    }
    int vertex_segment(int v, double tau) const {
        return g_off_[static_cast<std::size_t>(v)] +
               segment_index(vcuts_.data() + vcut_off_[static_cast<std::size_t>(v)], vcut_count(v), tau);
    }
    // tau lies in [0, first cut) of the wrap segment, where a twisted timeline has flipped its value
    static bool is_pre(const std::vector<double>& cuts, const std::vector<std::int32_t>& off, int i, double tau) {
        return off[static_cast<std::size_t>(i) + 1] > off[static_cast<std::size_t>(i)] &&
               tau < cuts[static_cast<std::size_t>(off[static_cast<std::size_t>(i)])];
    }

    // Space-time position of every variable for the nested-dissection order of the solver: edge segments sit at the
    // edge midpoint, vertex segments and twist bits at their vertex (twist bits at tau = 0), segments at the midpoint
    // of their tau interval (a wrapping segment at the midpoint of its wrapped interval). tau is scaled by
    // (spatial extent) / beta, so that a box of the space-time system is as long in tau as in space. Periodic
    // wrap-around is ignored: variables coupled across the seam simply end up in separators.
    void assign_coordinates(const Lattice::GswTopology& topo, double beta) {
        const int nV = topo.vertex_count;
        double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
        for (int v = 0; v < nV; ++v) {
            xmin = std::min(xmin, topo.vertex_x[static_cast<std::size_t>(v)]);
            xmax = std::max(xmax, topo.vertex_x[static_cast<std::size_t>(v)]);
            ymin = std::min(ymin, topo.vertex_y[static_cast<std::size_t>(v)]);
            ymax = std::max(ymax, topo.vertex_y[static_cast<std::size_t>(v)]);
        }
        const double scale = (std::max(xmax - xmin, ymax - ymin) + 1.) / beta;
        for (int v = 0; v < nV; ++v) {
            const auto vx = static_cast<float>(topo.vertex_x[static_cast<std::size_t>(v)]);
            const auto vy = static_cast<float>(topo.vertex_y[static_cast<std::size_t>(v)]);
            const int m = vcut_count(v);
            const double* c = vcuts_.data() + vcut_off_[static_cast<std::size_t>(v)];
            for (int k = 0; k < std::max(1, m); ++k) {
                solver_.coord(g_off_[static_cast<std::size_t>(v)] + k, vx, vy, static_cast<float>(segment_mid(c, m, k, beta) * scale));
            }
            solver_.coord(t_off_ + v, vx, vy, 0.F);
        }
        for (int e = 0; e < topo.edge_count; ++e) {
            const auto u = static_cast<std::size_t>(topo.edge_source[static_cast<std::size_t>(e)]);
            const auto w = static_cast<std::size_t>(topo.edge_target[static_cast<std::size_t>(e)]);
            const auto ex = static_cast<float>(0.5 * (topo.vertex_x[u] + topo.vertex_x[w]));
            const auto ey = static_cast<float>(0.5 * (topo.vertex_y[u] + topo.vertex_y[w]));
            const int m = ecut_count(e);
            const double* c = ecuts_.data() + ecut_off_[static_cast<std::size_t>(e)];
            for (int k = 0; k < std::max(1, m); ++k) {
                solver_.coord(s_off_[static_cast<std::size_t>(e)] + k, ex, ey, static_cast<float>(segment_mid(c, m, k, beta) * scale));
            }
        }
    }
    // midpoint of segment k of a timeline with m cuts c (the last segment wraps around beta)
    static double segment_mid(const double* c, int m, int k, double beta) {
        if (m == 0) {
            return 0.5 * beta;
        }
        const double a = c[k];
        const double b = k + 1 < m ? c[k + 1] : c[0] + beta;
        const double t = 0.5 * (a + b);
        return t >= beta ? t - beta : t;
    }

    // The configuration before the update, written in the variables of the system (test hook).
    void current_state(Lattice& lat, const Lattice::GswTopology& topo) {
        x_.assign(static_cast<std::size_t>(solver_variable_count(topo)), 0);
        for (int v = 0; v < topo.vertex_count; ++v) {
            const auto& old = lat.gsw_star_events(v);
            const int m = vcut_count(v);
            const double* c = vcuts_.data() + vcut_off_[static_cast<std::size_t>(v)];
            std::size_t j = 0;
            std::uint8_t parity = 0;
            for (int k = 0; k < m; ++k) {
                if (j < old.size() && old[j] == c[k]) {
                    parity ^= 1U;
                    ++j;
                }
                x_[static_cast<std::size_t>(g_off_[static_cast<std::size_t>(v)] + k)] = parity;
            }
            x_[static_cast<std::size_t>(t_off_ + v)] = parity;
        }
        for (int e = 0; e < topo.edge_count; ++e) {
            const auto& old = lat.gsw_kinks(e);
            const int m = ecut_count(e);
            const double* c = ecuts_.data() + ecut_off_[static_cast<std::size_t>(e)];
            std::size_t j = 0;
            std::uint8_t value = lat.gsw_spin0(e) == -1 ? 1 : 0;
            if (m == 0) {
                x_[static_cast<std::size_t>(s_off_[static_cast<std::size_t>(e)])] = value;
            }
            for (int k = 0; k < m; ++k) {
                if (j < old.size() && old[j] == c[k]) {
                    value ^= 1U;
                    ++j;
                }
                x_[static_cast<std::size_t>(s_off_[static_cast<std::size_t>(e)] + k)] = value;
            }
        }
    }
    int solver_variable_count(const Lattice::GswTopology& topo) const { return t_off_ + topo.vertex_count; }

    gf2::UniformSolutionSampler solver_;
    std::vector<std::uint8_t> x_;
    std::vector<double> vcuts_, ecuts_, poisson_, plaquette_flips_;
    std::vector<std::int32_t> vcut_off_, ecut_off_, g_off_, s_off_;
    int t_off_ = 0;
};

}  // namespace paratoric
