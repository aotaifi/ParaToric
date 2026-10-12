// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace paratoric::gf2 {

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
 */
class UniformSolutionSampler {
public:
    struct Stats {
        int rows = 0;       // rows after cancelling and deduplication
        int rank = 0;       // rank of the system (peeled rows plus core rank)
        int core_rows = 0;
        int core_vars = 0;
    };

    void reset(int variable_count) {
        n_var_ = variable_count;
        row_start_.assign(1, 0);
        row_vars_.clear();
        row_hash_.clear();
        scratch_.clear();
        table_.assign(std::max<std::size_t>(1024, table_.size()), -1);
        table_used_ = 0;
    }

    // Toggle variable v in the row being built (adding a variable twice removes it).
    void add(int v) { scratch_.push_back(v); }

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
        if (core_rows > 0) {
            core_rank = solve_core(rng, core_rows, nc, x, state_);
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

        // forward elimination to echelon form
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
        for (int j = 0; j < nc; ++j) {
            const int v = core_var_[static_cast<std::size_t>(j)];
            x[static_cast<std::size_t>(v)] = static_cast<std::uint8_t>((xc_[static_cast<std::size_t>(j) >> 6] >> (j & 63)) & 1U);
            state[static_cast<std::size_t>(v)] = 1;
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
};

}  // namespace paratoric::gf2
