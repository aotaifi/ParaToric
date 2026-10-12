// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace paratoric::gf2 {

// How the core of the system is eliminated: one dense bit-packed matrix, or sparse elimination in a nested-dissection
// order of the variables' space-time coordinates followed by a dense elimination of what is left.
enum class Solver { dense, nested_dissection };

/**
 * @brief Uniform sampler for the solutions of a homogeneous sparse linear system over GF(2).
 *
 * Rows are lists of variable indices whose XOR must vanish. The system is assumed consistent (it always is, the
 * zero vector solves it). The solution is uniform over the whole solution space, in three stages:
 *   1. rows are reduced (repeated indices cancel) and identical rows are kept once;
 *   2. XORSAT peeling: a variable that occurs in exactly one remaining row can always be set last to satisfy that
 *      row, so row and variable are removed; this repeats until a core is left;
 *   3. the core is eliminated densely with 64-bit words (forward elimination to echelon form, uniform free
 *      variables, back-substitution by popcount parities).
 * Variables outside every row are uniform bits, the peeled variables are then fixed in reverse order. The map
 * (core solution, free bits) -> full solution is a bijection, so the result is uniform. All buffers are reused
 * between calls.
 *
 * Solver::nested_dissection replaces stage 3 by sparse Gaussian elimination in a chosen column order. The order is
 * recursive coordinate bisection of the core variables on the coordinates given with coord(): split the box along
 * its longest side, put the variables of the lower half that share a row with the upper half into a separator, and
 * order left part, right part, separator (a nested-dissection order, so the fill-in stays inside the boxes). For each
 * variable in that order the shortest still-unused row containing it becomes its pivot row and is XORed into the
 * other rows containing it. Once few rows (nd_dense_rows) or few variables (nd_dense_vars) are left, the remaining
 * rows go through the dense routine of stage 3. Back-substitution then runs in reverse order: a variable with a
 * pivot row is determined by it (its other entries come later in the order), every other variable is a free
 * uniform bit. The pivot choice and the column order are arbitrary: for any choice the rows form an echelon system
 * of the same solution space, and the map (free bits) -> solution is a bijection from the uniform measure on
 * {0,1}^{#free} onto the solutions, so the result is uniform whatever the order is. The order only changes the cost.
 */
class UniformSolutionSampler {
public:
    struct Stats {
        int rows = 0;       // rows after cancelling and deduplication
        int rank = 0;       // rank of the system (peeled rows plus core rank)
        int core_rows = 0;
        int core_vars = 0;
        int nd_max_front = 0;   // nested dissection: most unused rows containing one pivot variable
        int nd_dense_rows = 0;  // rows handed to the dense routine
        int nd_dense_cols = 0;  // their distinct variables
        std::uint64_t nd_work = 0;  // sum of the lengths of the two rows in each sparse XOR
    };

    Solver solver = Solver::dense;
    int nd_leaf = 64;            // boxes with at most this many core variables are not split further
    int nd_dense_vars = 4096;    // switch to the dense routine when at most this many variables are not yet eliminated
    int nd_dense_rows = 0;       // ... or when at most this many unused non-empty rows are left (0: off)
    int test_break = 0;          // test hook: 1 sets every free core variable to 0 (a deliberately broken sampler)

    // Wall-clock seconds accumulated over all sample() calls, and the largest front seen.
    double seconds_peel = 0.;
    double seconds_order = 0.;
    double seconds_sparse = 0.;
    double seconds_dense = 0.;
    std::uint64_t nd_work_sum = 0;
    std::uint64_t nd_dense_rows_sum = 0;
    std::uint64_t nd_dense_cols_sum = 0;
    int nd_max_front_seen = 0;
    std::uint64_t calls = 0;

    void reset(int variable_count) {
        n_var_ = variable_count;
        if (solver == Solver::nested_dissection) {
            coord_.assign(3 * static_cast<std::size_t>(variable_count), 0.F);
        }
        row_start_.assign(1, 0);
        row_vars_.clear();
        row_hash_.clear();
        scratch_.clear();
        table_.assign(std::max<std::size_t>(1024, table_.size()), -1);
        table_used_ = 0;
    }

    // Toggle variable v in the row being built (adding a variable twice removes it).
    void add(int v) { scratch_.push_back(v); }

    // Space-time position of variable v, used by the nested-dissection order only (ignored by the dense solver).
    // Call after reset() for the variables that can end up in the core; the others keep position 0.
    void coord(int v, float x, float y, float tau) {
        if (solver == Solver::nested_dissection) {
            float* c = coord_.data() + 3 * static_cast<std::size_t>(v);
            c[0] = x;
            c[1] = y;
            c[2] = tau;
        }
    }

    // Close the current row: cancel repeated indices, drop empty rows and rows identical to an earlier one.
    void end_row() {
        std::sort(scratch_.begin(), scratch_.end());
        std::size_t out = 0;
        for (std::size_t i = 0; i < scratch_.size();) {
            std::size_t j = i + 1;
            while (j < scratch_.size() && scratch_[j] == scratch_[i]) ++j;
            if (((j - i) & 1U) != 0U) {
                scratch_[out++] = scratch_[i];
            }
            i = j;
        }
        scratch_.resize(out);
        if (out == 0) {
            return;
        }
        std::uint64_t hash = 0x9e3779b97f4a7c15ULL ^ out;
        for (const int v : scratch_) {
            hash = mix(hash ^ static_cast<std::uint64_t>(v));
        }
        if (table_used_ * 2 >= table_.size()) {
            grow_table();
        }
        const std::size_t mask = table_.size() - 1;
        std::size_t slot = static_cast<std::size_t>(hash) & mask;
        while (table_[slot] >= 0) {
            const auto r = static_cast<std::size_t>(table_[slot]);
            if (row_hash_[r] == hash && same_row(r)) {
                scratch_.clear();
                return;
            }
            slot = (slot + 1) & mask;
        }
        table_[slot] = static_cast<std::int32_t>(row_hash_.size());
        ++table_used_;
        row_hash_.push_back(hash);
        row_vars_.insert(row_vars_.end(), scratch_.begin(), scratch_.end());
        row_start_.push_back(static_cast<std::int32_t>(row_vars_.size()));
        scratch_.clear();
    }

    int row_count() const { return static_cast<int>(row_hash_.size()); }

    // True if x satisfies every stored row (test hook).
    bool satisfied_by(const std::vector<std::uint8_t>& x) const {
        for (std::size_t r = 0; r < row_hash_.size(); ++r) {
            std::uint8_t s = 0;
            for (std::int32_t k = row_start_[r]; k < row_start_[r + 1]; ++k) {
                s ^= x[static_cast<std::size_t>(row_vars_[static_cast<std::size_t>(k)])];
            }
            if (s != 0) {
                return false;
            }
        }
        return true;
    }

    // Draw a uniform solution into x (one byte per variable, 0 or 1).
    template<class URBG>
    Stats sample(URBG& rng, std::vector<std::uint8_t>& x) {
        const int nr = row_count();
        const int nv = n_var_;
        Stats stats;
        stats.rows = nr;
        const auto t_begin = std::chrono::steady_clock::now();
        ++calls;

        // variable -> rows incidence
        deg_.assign(static_cast<std::size_t>(nv), 0);
        for (const int v : row_vars_) {
            ++deg_[static_cast<std::size_t>(v)];
        }
        inc_start_.assign(static_cast<std::size_t>(nv) + 1, 0);
        for (int v = 0; v < nv; ++v) {
            inc_start_[static_cast<std::size_t>(v) + 1] = inc_start_[static_cast<std::size_t>(v)] + deg_[static_cast<std::size_t>(v)];
        }
        inc_.resize(row_vars_.size());
        fill_pos_.assign(inc_start_.begin(), inc_start_.end() - 1);
        for (int r = 0; r < nr; ++r) {
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                inc_[static_cast<std::size_t>(fill_pos_[static_cast<std::size_t>(row_vars_[static_cast<std::size_t>(k)])]++)] = r;
            }
        }

        // peeling
        alive_.assign(static_cast<std::size_t>(nr), 1);
        peeled_var_.clear();
        peeled_row_.clear();
        stack_.clear();
        for (int v = 0; v < nv; ++v) {
            if (deg_[static_cast<std::size_t>(v)] == 1) {
                stack_.push_back(v);
            }
        }
        while (!stack_.empty()) {
            const int v = stack_.back();
            stack_.pop_back();
            if (deg_[static_cast<std::size_t>(v)] != 1) {
                continue;
            }
            int r = -1;
            for (int k = inc_start_[static_cast<std::size_t>(v)]; k < inc_start_[static_cast<std::size_t>(v) + 1]; ++k) {
                if (alive_[static_cast<std::size_t>(inc_[static_cast<std::size_t>(k)])] != 0) {
                    r = inc_[static_cast<std::size_t>(k)];
                    break;
                }
            }
            alive_[static_cast<std::size_t>(r)] = 0;
            peeled_var_.push_back(v);
            peeled_row_.push_back(r);
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                const int u = row_vars_[static_cast<std::size_t>(k)];
                if (--deg_[static_cast<std::size_t>(u)] == 1) {
                    stack_.push_back(u);
                }
            }
        }

        // core variables: those still in an alive row
        x.assign(static_cast<std::size_t>(nv), 0);
        state_.assign(static_cast<std::size_t>(nv), 0);  // 0 unset, 1 set (core or free), 2 peeled
        core_index_.assign(static_cast<std::size_t>(nv), -1);
        core_var_.clear();
        int core_rows = 0;
        for (int r = 0; r < nr; ++r) {
            if (alive_[static_cast<std::size_t>(r)] == 0) {
                continue;
            }
            ++core_rows;
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                const int v = row_vars_[static_cast<std::size_t>(k)];
                if (core_index_[static_cast<std::size_t>(v)] < 0) {
                    core_index_[static_cast<std::size_t>(v)] = static_cast<std::int32_t>(core_var_.size());
                    core_var_.push_back(v);
                }
            }
        }
        const int nc = static_cast<int>(core_var_.size());
        stats.core_rows = core_rows;
        stats.core_vars = nc;
        int core_rank = 0;
        const auto t_peeled = std::chrono::steady_clock::now();
        seconds_peel += std::chrono::duration<double>(t_peeled - t_begin).count();
        if (core_rows > 0) {
            if (solver == Solver::nested_dissection) {
                core_rank = solve_core_nd(rng, core_rows, nc, x, state_, stats);
            } else {
                core_rank = solve_core(rng, core_rows, nc, x, state_);
                seconds_dense += std::chrono::duration<double>(std::chrono::steady_clock::now() - t_peeled).count();
            }
        }
        for (const int v : peeled_var_) {
            state_[static_cast<std::size_t>(v)] = 2;
        }

        // variables in no row (and not peeled): uniform bits
        std::uint64_t bits = 0;
        int bits_left = 0;
        for (int v = 0; v < nv; ++v) {
            if (state_[static_cast<std::size_t>(v)] == 0) {
                if (bits_left == 0) {
                    bits = rng();
                    bits_left = 64;
                }
                x[static_cast<std::size_t>(v)] = static_cast<std::uint8_t>(bits & 1U);
                bits >>= 1;
                --bits_left;
            }
        }

        // peeled variables, last peeled first
        for (std::size_t i = peeled_var_.size(); i-- > 0;) {
            const int v = peeled_var_[i];
            const int r = peeled_row_[i];
            std::uint8_t s = 0;
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                const int u = row_vars_[static_cast<std::size_t>(k)];
                if (u != v) {
                    s ^= x[static_cast<std::size_t>(u)];
                }
            }
            x[static_cast<std::size_t>(v)] = s;
        }
        stats.rank = core_rank + static_cast<int>(peeled_var_.size());
        return stats;
    }

private:
    static std::uint64_t mix(std::uint64_t z) {
        z += 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    bool same_row(std::size_t r) const {
        const auto begin = row_vars_.begin() + row_start_[r];
        const auto end = row_vars_.begin() + row_start_[r + 1];
        return static_cast<std::size_t>(end - begin) == scratch_.size() && std::equal(begin, end, scratch_.begin());
    }

    void grow_table() {
        table_.assign(table_.size() * 2, -1);
        const std::size_t mask = table_.size() - 1;
        for (std::size_t r = 0; r < row_hash_.size(); ++r) {
            std::size_t slot = static_cast<std::size_t>(row_hash_[r]) & mask;
            while (table_[slot] >= 0) {
                slot = (slot + 1) & mask;
            }
            table_[slot] = static_cast<std::int32_t>(r);
        }
    }

    // Dense elimination of the core. Writes the core variables into x and marks them set. Returns the rank.
    template<class URBG>
    int solve_core(URBG& rng, int nrows, int nc, std::vector<std::uint8_t>& x, std::vector<std::uint8_t>& state) {
        const auto W = static_cast<std::size_t>((nc + 63) / 64);
        const auto R = static_cast<std::size_t>(nrows);
        dense_.assign(R * W, 0);
        std::size_t row = 0;
        for (int r = 0; r < static_cast<int>(alive_.size()); ++r) {
            if (alive_[static_cast<std::size_t>(r)] == 0) {
                continue;
            }
            std::uint64_t* m = dense_.data() + row * W;
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                const auto c = static_cast<std::size_t>(core_index_[static_cast<std::size_t>(row_vars_[static_cast<std::size_t>(k)])]);
                m[c >> 6] ^= std::uint64_t{1} << (c & 63);
            }
            ++row;
        }

        const int rank = eliminate_dense(R, nc, rng);
        for (int j = 0; j < nc; ++j) {
            const int v = core_var_[static_cast<std::size_t>(j)];
            x[static_cast<std::size_t>(v)] = static_cast<std::uint8_t>((xc_[static_cast<std::size_t>(j) >> 6] >> (j & 63)) & 1U);
            state[static_cast<std::size_t>(v)] = 1;
        }
        return rank;
    }

    // ---- nested-dissection solver ------------------------------------------------------------------------------

    // Fill order_ (position -> core index) and pos_of_ (core index -> position).
    void build_nd_order(int nc) {
        const auto n = static_cast<std::size_t>(nc);
        perm_.resize(n);
        for (std::size_t j = 0; j < n; ++j) {
            perm_[j] = static_cast<std::int32_t>(j);
        }
        order_.clear();
        order_.reserve(n);
        if (nc > nd_dense_vars) {  // otherwise everything goes to the dense routine and the order does not matter
            cxyz_.resize(3 * n);
            for (std::size_t j = 0; j < n; ++j) {
                const float* c = coord_.data() + 3 * static_cast<std::size_t>(core_var_[j]);
                cxyz_[3 * j] = c[0];
                cxyz_[3 * j + 1] = c[1];
                cxyz_[3 * j + 2] = c[2];
            }
            label_.assign(n, -1);
            label_counter_ = 0;
            nd_recurse(0, nc);
        } else {
            order_.assign(perm_.begin(), perm_.end());
        }
        pos_of_.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            pos_of_[static_cast<std::size_t>(order_[i])] = static_cast<std::int32_t>(i);
        }
    }

    // Order perm_[lo, hi): left part, right part, separator (appended to order_).
    void nd_recurse(int lo, int hi) {
        const int n = hi - lo;
        if (n <= std::max(nd_leaf, 1)) {
            order_.insert(order_.end(), perm_.begin() + lo, perm_.begin() + hi);
            return;
        }
        int axis = 0;
        float best = -1.F;
        for (int d = 0; d < 3; ++d) {
            float vmin = cxyz_[3 * static_cast<std::size_t>(perm_[static_cast<std::size_t>(lo)]) + static_cast<std::size_t>(d)];
            float vmax = vmin;
            for (int i = lo; i < hi; ++i) {
                const float c = cxyz_[3 * static_cast<std::size_t>(perm_[static_cast<std::size_t>(i)]) + static_cast<std::size_t>(d)];
                vmin = std::min(vmin, c);
                vmax = std::max(vmax, c);
            }
            if (vmax - vmin > best) {
                best = vmax - vmin;
                axis = d;
            }
        }
        const int mid = lo + n / 2;
        std::nth_element(perm_.begin() + lo, perm_.begin() + mid, perm_.begin() + hi,
                         [&](std::int32_t a, std::int32_t b) {
                             return cxyz_[3 * static_cast<std::size_t>(a) + static_cast<std::size_t>(axis)] <
                                    cxyz_[3 * static_cast<std::size_t>(b) + static_cast<std::size_t>(axis)];
                         });
        const std::int32_t id_upper = ++label_counter_;
        for (int i = mid; i < hi; ++i) {
            label_[static_cast<std::size_t>(perm_[static_cast<std::size_t>(i)])] = id_upper;
        }
        std::vector<std::int32_t> sep;
        int kept = lo;
        for (int i = lo; i < mid; ++i) {
            const std::int32_t j = perm_[static_cast<std::size_t>(i)];
            if (touches_label(j, id_upper)) {
                sep.push_back(j);
            } else {
                perm_[static_cast<std::size_t>(kept++)] = j;
            }
        }
        nd_recurse(lo, kept);
        nd_recurse(mid, hi);
        order_.insert(order_.end(), sep.begin(), sep.end());
    }

    // True if core variable j shares an alive row with a core variable labelled id.
    bool touches_label(std::int32_t j, std::int32_t id) const {
        const auto v = static_cast<std::size_t>(core_var_[static_cast<std::size_t>(j)]);
        for (int a = inc_start_[v]; a < inc_start_[v + 1]; ++a) {
            const int r = inc_[static_cast<std::size_t>(a)];
            if (alive_[static_cast<std::size_t>(r)] == 0) {
                continue;
            }
            for (int k = row_start_[static_cast<std::size_t>(r)]; k < row_start_[static_cast<std::size_t>(r) + 1]; ++k) {
                const auto u = static_cast<std::size_t>(core_index_[static_cast<std::size_t>(row_vars_[static_cast<std::size_t>(k)])]);
                if (label_[u] == id) {
                    return true;
                }
            }
        }
        return false;
    }

    static bool row_contains(const std::vector<std::int32_t>& row, std::int32_t q) {
        return std::binary_search(row.begin(), row.end(), q);
    }

    // Add row r to the list of rows containing column q. Lists are cleaned lazily: when one reaches its capacity,
    // rows that were pivoted or no longer contain q are dropped, and duplicates (a row that left and re-entered).
    void push_col(std::int32_t q, std::int32_t r) {
        auto& c = col_[static_cast<std::size_t>(q)];
        if (c.size() >= ccap_[static_cast<std::size_t>(q)]) {
            std::size_t out = 0;
            for (const std::int32_t s : c) {
                if (rowstate_[static_cast<std::size_t>(s)] == 0 && row_contains(nd_rows_[static_cast<std::size_t>(s)], q)) {
                    c[out++] = s;
                }
            }
            c.resize(out);
            std::sort(c.begin(), c.end());
            c.erase(std::unique(c.begin(), c.end()), c.end());
            ccap_[static_cast<std::size_t>(q)] = std::max<std::size_t>(16, 2 * c.size());
        }
        c.push_back(r);
    }

    // Sparse elimination in the nested-dissection order, then the dense routine on the rows that remain.
    template<class URBG>
    int solve_core_nd(URBG& rng, int nrows, int nc, std::vector<std::uint8_t>& x, std::vector<std::uint8_t>& state,
                      Stats& stats) {
        using clock = std::chrono::steady_clock;
        const auto t0 = clock::now();
        build_nd_order(nc);
        const auto t1 = clock::now();

        const auto R = static_cast<std::size_t>(nrows);
        const auto N = static_cast<std::size_t>(nc);
        if (nd_rows_.size() < R) {
            nd_rows_.resize(R);
        }
        if (col_.size() < N) {
            col_.resize(N);
        }
        for (std::size_t q = 0; q < N; ++q) {
            col_[q].clear();
        }
        rowstate_.assign(R, 0);  // 0 active, 1 pivot row, 2 emptied
        stamp_.assign(R, -1);
        {
            std::size_t row = 0;
            for (std::size_t r = 0; r < alive_.size(); ++r) {
                if (alive_[r] == 0) {
                    continue;
                }
                auto& pr = nd_rows_[row];
                pr.clear();
                for (int k = row_start_[r]; k < row_start_[r + 1]; ++k) {
                    pr.push_back(pos_of_[static_cast<std::size_t>(core_index_[static_cast<std::size_t>(row_vars_[static_cast<std::size_t>(k)])])]);
                }
                std::sort(pr.begin(), pr.end());
                for (const std::int32_t q : pr) {
                    col_[static_cast<std::size_t>(q)].push_back(static_cast<std::int32_t>(row));
                }
                ++row;
            }
        }
        ccap_.resize(N);
        for (std::size_t q = 0; q < N; ++q) {
            ccap_[q] = std::max<std::size_t>(16, 2 * col_[q].size());
        }
        pivot_row_.assign(N, -1);

        std::uint64_t bits = 0;
        int bits_left = 0;
        auto free_bit = [&]() -> std::uint8_t {
            if (bits_left == 0) {
                bits = rng();
                bits_left = 64;
            }
            const auto b = static_cast<std::uint8_t>(bits & 1U);
            bits >>= 1;
            --bits_left;
            return test_break == 1 ? std::uint8_t{0} : b;
        };

        // forward elimination, column by column in the nested-dissection order
        int active = nrows;
        int p = 0;
        int pivots = 0;
        int front_max = 0;
        std::uint64_t work = 0;
        for (; p < nc; ++p) {
            if (active == 0 || nc - p <= nd_dense_vars || active <= nd_dense_rows) {
                break;
            }
            auto& cl = col_[static_cast<std::size_t>(p)];
            cand_.clear();
            for (const std::int32_t r : cl) {
                if (rowstate_[static_cast<std::size_t>(r)] != 0 || stamp_[static_cast<std::size_t>(r)] == p ||
                    !row_contains(nd_rows_[static_cast<std::size_t>(r)], p)) {
                    continue;
                }
                stamp_[static_cast<std::size_t>(r)] = p;
                cand_.push_back(r);
            }
            cl.clear();
            if (cand_.empty()) {
                continue;  // no row reaches this variable: it is free
            }
            front_max = std::max(front_max, static_cast<int>(cand_.size()));
            std::int32_t piv = cand_[0];
            for (const std::int32_t r : cand_) {
                if (nd_rows_[static_cast<std::size_t>(r)].size() < nd_rows_[static_cast<std::size_t>(piv)].size()) {
                    piv = r;
                }
            }
            rowstate_[static_cast<std::size_t>(piv)] = 1;
            pivot_row_[static_cast<std::size_t>(p)] = piv;
            --active;
            ++pivots;
            const auto& pr = nd_rows_[static_cast<std::size_t>(piv)];
            for (const std::int32_t r : cand_) {
                if (r == piv) {
                    continue;
                }
                auto& a = nd_rows_[static_cast<std::size_t>(r)];
                tmp_.clear();
                std::size_t i = 0;
                std::size_t j = 0;
                while (i < a.size() || j < pr.size()) {
                    if (j >= pr.size() || (i < a.size() && a[i] < pr[j])) {
                        tmp_.push_back(a[i++]);
                    } else if (i >= a.size() || pr[j] < a[i]) {
                        tmp_.push_back(pr[j]);
                        push_col(pr[j], r);
                        ++j;
                    } else {
                        ++i;
                        ++j;
                    }
                }
                work += a.size() + pr.size();
                a.swap(tmp_);
                if (a.empty()) {
                    rowstate_[static_cast<std::size_t>(r)] = 2;
                    --active;
                }
            }
        }
        const int p_switch = p;
        const auto t2 = clock::now();

        // dense routine on the rows that are left: columns = the distinct variables in them
        dcol_.assign(N, -1);
        dcol_pos_.clear();
        for (std::size_t r = 0; r < R; ++r) {
            if (rowstate_[r] != 0) {
                continue;
            }
            for (const std::int32_t q : nd_rows_[r]) {
                if (dcol_[static_cast<std::size_t>(q)] < 0) {
                    dcol_[static_cast<std::size_t>(q)] = 0;
                    dcol_pos_.push_back(q);
                }
            }
        }
        std::sort(dcol_pos_.begin(), dcol_pos_.end());
        for (std::size_t c = 0; c < dcol_pos_.size(); ++c) {
            dcol_[static_cast<std::size_t>(dcol_pos_[c])] = static_cast<std::int32_t>(c);
        }
        const auto ncd = static_cast<int>(dcol_pos_.size());
        int dense_rank = 0;
        int rows_d = 0;
        pos_x_.assign(N, 0);
        if (active > 0) {
            const auto Wd = static_cast<std::size_t>((ncd + 63) / 64);
            dense_.assign(static_cast<std::size_t>(active) * Wd, 0);
            std::size_t row = 0;
            for (std::size_t r = 0; r < R; ++r) {
                if (rowstate_[r] != 0) {
                    continue;
                }
                std::uint64_t* m = dense_.data() + row * Wd;
                for (const std::int32_t q : nd_rows_[r]) {
                    const auto c = static_cast<std::size_t>(dcol_[static_cast<std::size_t>(q)]);
                    m[c >> 6] ^= std::uint64_t{1} << (c & 63);
                }
                ++row;
            }
            rows_d = active;
            dense_rank = eliminate_dense(static_cast<std::size_t>(active), ncd, rng);
        }
        for (auto q = static_cast<std::size_t>(p_switch); q < N; ++q) {
            const std::int32_t c = dcol_[q];
            if (c >= 0) {
                pos_x_[q] = static_cast<std::uint8_t>((xc_[static_cast<std::size_t>(c) >> 6] >> (c & 63)) & 1U);
            } else {
                pos_x_[q] = free_bit();
            }
        }
        // back-substitution in reverse order of the sparse phase
        for (int q = p_switch; q-- > 0;) {
            const std::int32_t r = pivot_row_[static_cast<std::size_t>(q)];
            if (r < 0) {
                pos_x_[static_cast<std::size_t>(q)] = free_bit();
                continue;
            }
            std::uint8_t s = 0;
            for (const std::int32_t u : nd_rows_[static_cast<std::size_t>(r)]) {
                if (u != q) {
                    s ^= pos_x_[static_cast<std::size_t>(u)];
                }
            }
            pos_x_[static_cast<std::size_t>(q)] = s;
        }
        for (std::size_t q = 0; q < N; ++q) {
            const int v = core_var_[static_cast<std::size_t>(order_[q])];
            x[static_cast<std::size_t>(v)] = pos_x_[q];
            state[static_cast<std::size_t>(v)] = 1;
        }
        const auto t3 = clock::now();

        stats.nd_max_front = front_max;
        stats.nd_dense_rows = rows_d;
        stats.nd_dense_cols = ncd;
        stats.nd_work = work;
        nd_work_sum += work;
        nd_dense_rows_sum += static_cast<std::uint64_t>(rows_d);
        nd_dense_cols_sum += static_cast<std::uint64_t>(ncd);
        nd_max_front_seen = std::max(nd_max_front_seen, front_max);
        seconds_order += std::chrono::duration<double>(t1 - t0).count();
        seconds_sparse += std::chrono::duration<double>(t2 - t1).count();
        seconds_dense += std::chrono::duration<double>(t3 - t2).count();
        return pivots + dense_rank;
    }

    // Eliminate the dense_ matrix (R rows, nc columns, row stride ceil(nc/64) words) and draw a uniform solution of
    // it into xc_ (bit c = column c). Forward elimination to echelon form, uniform free columns, pivots by
    // back-substitution with popcount parities. Returns the rank.
    template<class URBG>
    int eliminate_dense(std::size_t R, int nc, URBG& rng) {
        const auto W = static_cast<std::size_t>((nc + 63) / 64);
        pivot_col_.clear();
        std::size_t rank = 0;
        for (std::size_t col = 0; col < static_cast<std::size_t>(nc) && rank < R; ++col) {
            const std::size_t w = col >> 6;
            const std::uint64_t bit = std::uint64_t{1} << (col & 63);
            std::size_t pr = rank;
            while (pr < R && (dense_[pr * W + w] & bit) == 0) {
                ++pr;
            }
            if (pr == R) {
                continue;
            }
            if (pr != rank) {
                std::swap_ranges(dense_.begin() + static_cast<std::ptrdiff_t>(pr * W),
                                 dense_.begin() + static_cast<std::ptrdiff_t>((pr + 1) * W),
                                 dense_.begin() + static_cast<std::ptrdiff_t>(rank * W));
            }
            const std::uint64_t* piv = dense_.data() + rank * W;
            for (std::size_t r = rank + 1; r < R; ++r) {
                std::uint64_t* m = dense_.data() + r * W;
                if ((m[w] & bit) != 0) {
                    for (std::size_t k = w; k < W; ++k) {
                        m[k] ^= piv[k];
                    }
                }
            }
            pivot_col_.push_back(static_cast<std::int32_t>(col));
            ++rank;
        }

        // free variables uniform, pivots by back-substitution
        xc_.assign(W, 0);
        for (std::size_t k = 0; k < W; ++k) {
            xc_[k] = rng();
        }
        if ((nc & 63) != 0) {
            xc_[W - 1] &= (std::uint64_t{1} << (nc & 63)) - 1;
        }
        for (const auto c : pivot_col_) {
            xc_[static_cast<std::size_t>(c) >> 6] &= ~(std::uint64_t{1} << (c & 63));
        }
        if (test_break == 1) {
            std::fill(xc_.begin(), xc_.end(), std::uint64_t{0});  // deliberately broken: free columns all 0
        }
        for (std::size_t i = rank; i-- > 0;) {
            const auto c = static_cast<std::size_t>(pivot_col_[i]);
            const std::uint64_t* m = dense_.data() + i * W;
            std::uint64_t parity = 0;
            for (std::size_t k = c >> 6; k < W; ++k) {
                parity ^= m[k] & xc_[k];
            }
            if ((std::popcount(parity) & 1) != 0) {
                xc_[c >> 6] |= std::uint64_t{1} << (c & 63);
            }
        }
        return static_cast<int>(rank);
    }

    int n_var_ = 0;
    std::vector<std::int32_t> row_start_{0};
    std::vector<std::int32_t> row_vars_;
    std::vector<std::uint64_t> row_hash_;
    std::vector<std::int32_t> scratch_;
    std::vector<std::int32_t> table_;
    std::size_t table_used_ = 0;

    std::vector<std::int32_t> deg_, inc_start_, inc_, fill_pos_, stack_;
    std::vector<std::int32_t> peeled_var_, peeled_row_;
    std::vector<std::uint8_t> alive_, state_;
    std::vector<std::int32_t> core_index_, core_var_, pivot_col_;
    std::vector<std::uint64_t> dense_, xc_;

    // nested dissection
    std::vector<float> coord_, cxyz_;
    std::vector<std::int32_t> perm_, order_, pos_of_, label_, stamp_, pivot_row_, cand_, tmp_, dcol_, dcol_pos_;
    std::int32_t label_counter_ = 0;
    std::vector<std::vector<std::int32_t>> nd_rows_, col_;
    std::vector<std::size_t> ccap_;
    std::vector<std::uint8_t> rowstate_, pos_x_;
};

}  // namespace paratoric::gf2
