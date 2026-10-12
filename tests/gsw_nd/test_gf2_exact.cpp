// Gate N1: exactness of paratoric::gf2::UniformSolutionSampler (dense and nested-dissection solvers).
// Random sparse homogeneous GF(2) systems; for each, the full solution space is enumerated by brute force and the
// sampler's output is tested (every sample satisfies every row; chi-square of the solution frequencies against uniform).
// usage: test_gf2_exact NSYS NSAMP SEED [STYLE [VARIANT_FILTER]]   (STYLE 1: only plain random rows)
// Prints one line per (solver variant, broken?) with the distribution of the chi-square p-values.
#include "mcmc/gf2_sampler.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

using paratoric::gf2::Solver;
using paratoric::gf2::UniformSolutionSampler;

// regularised upper incomplete gamma Q(a, x) (Numerical Recipes gser/gcf)
static double gammq(double a, double x) {
    if (x <= 0) return 1.0;
    const double gln = std::lgamma(a);
    if (x < a + 1.0) {
        double ap = a, sum = 1.0 / a, del = sum;
        for (int n = 0; n < 10000; ++n) {
            ap += 1.0;
            del *= x / ap;
            sum += del;
            if (std::fabs(del) < std::fabs(sum) * 1e-15) break;
        }
        return 1.0 - sum * std::exp(-x + a * std::log(x) - gln);
    }
    double b = x + 1.0 - a, c = 1.0 / 1e-300, d = 1.0 / b, h = d;
    for (int i = 1; i < 10000; ++i) {
        const double an = -i * (i - a);
        b += 2.0;
        d = an * d + b;
        if (std::fabs(d) < 1e-300) d = 1e-300;
        c = b + an / c;
        if (std::fabs(c) < 1e-300) c = 1e-300;
        d = 1.0 / d;
        const double del = d * c;
        h *= del;
        if (std::fabs(del - 1.0) < 1e-15) break;
    }
    return std::exp(-x + a * std::log(x) - gln) * h;
}

struct System {
    int n = 0;
    std::vector<std::vector<int>> rows;
    std::vector<float> xyz;
    std::vector<std::int32_t> sol_index;  // assignment bits -> solution id or -1
    int n_sol = 0;
};

static System make_system(int idx, std::uint64_t seed, int style) {
    std::mt19937_64 g(seed * 1000003ULL + static_cast<std::uint64_t>(idx));
    System s;
    s.n = 8 + static_cast<int>(g() % 7);  // 8..14
    auto random_row = [&]() {
        const int len = 2 + static_cast<int>(g() % 4);  // 2..5
        std::vector<int> row;
        while (static_cast<int>(row.size()) < len) {
            const int v = static_cast<int>(g() % static_cast<std::uint64_t>(s.n));
            if (std::find(row.begin(), row.end(), v) == row.end()) row.push_back(v);
        }
        return row;
    };
    if (style == 1 || idx % 2 == 1) {
        // plain random rows: mostly peel away, a small core
        const int m = s.n / 3 + static_cast<int>(g() % (static_cast<std::uint64_t>(s.n) * 5 / 12 + 1));  // n/3 .. 3n/4
        for (int r = 0; r < m; ++r) s.rows.push_back(random_row());
    } else {
        // planted: the rows are random sparse vectors orthogonal to k random vectors, so there are more rows than the
        // rank, every variable sits in several rows (a big core) and the solution space has at least 2^k elements
        const int k = 2 + static_cast<int>(g() % 5);  // 2..6
        std::vector<std::uint32_t> basis;
        for (int b = 0; b < k; ++b) basis.push_back(static_cast<std::uint32_t>(g() % (1ULL << s.n)));
        const int m = s.n + static_cast<int>(g() % (static_cast<std::uint64_t>(s.n) + 1));  // n .. 2n
        for (long tries = 0; static_cast<int>(s.rows.size()) < m && tries < 1000000; ++tries) {
            auto row = random_row();
            std::uint32_t mask = 0;
            for (const int v : row) mask |= 1U << v;
            bool ok = true;
            for (const auto b : basis) ok = ok && (std::popcount(b & mask) % 2 == 0);
            if (ok) s.rows.push_back(row);
        }
    }
    for (int v = 0; v < 3 * s.n; ++v) s.xyz.push_back(static_cast<float>(g() % 10000) / 1000.F);
    s.sol_index.assign(std::size_t{1} << s.n, -1);
    for (std::uint32_t a = 0; a < (1U << s.n); ++a) {
        bool ok = true;
        for (const auto& row : s.rows) {
            unsigned par = 0;
            for (const int v : row) par ^= (a >> v) & 1U;
            if (par != 0) { ok = false; break; }
        }
        if (ok) s.sol_index[a] = s.n_sol++;
    }
    return s;
}

struct Variant {
    const char* name;
    Solver solver;
    int leaf, dense_vars, dense_rows;
};

int main(int argc, char** argv) {
    const int nsys = argc > 1 ? std::atoi(argv[1]) : 200;
    const long nsamp = argc > 2 ? std::atol(argv[2]) : 200000;
    const std::uint64_t seed = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 1;
    const int style = argc > 4 ? std::atoi(argv[4]) : 0;
    const std::string only = argc > 5 ? argv[5] : "";
    const Variant variants[] = {
        {"dense", Solver::dense, 64, 4096, 0},
        {"nd_sparse", Solver::nested_dissection, 2, 0, 0},      // ND order, sparse elimination all the way
        {"nd_mixed", Solver::nested_dissection, 2, 4, 0},       // sparse until 4 variables are left, then dense
        {"nd_rows", Solver::nested_dissection, 3, 0, 3},        // sparse until 3 rows are left, then dense
        {"nd_alldense", Solver::nested_dissection, 64, 4096, 0} // everything goes to the dense routine via the ND path
    };
    std::vector<System> systems;
    for (int i = 0; i < nsys; ++i) systems.push_back(make_system(i, seed, style));
    std::printf("# %d systems, %ld samples each, seed %llu; solution-space sizes: ", nsys, nsamp, static_cast<unsigned long long>(seed));
    int smin = 1 << 30, smax = 0, nS1 = 0;
    for (const auto& s : systems) { smin = std::min(smin, s.n_sol); smax = std::max(smax, s.n_sol); nS1 += s.n_sol == 1; }
    std::printf("min %d max %d, systems with a unique solution (untestable) %d\n", smin, smax, nS1);
    struct Summary {
        int n = 0, lt3 = 0, lt6 = 0;
        double minp = -1, pks = 1;
        int hist[10] = {0};
    };
    auto summarise = [](std::vector<double> p) {
        Summary r;
        std::sort(p.begin(), p.end());
        r.n = static_cast<int>(p.size());
        double ks = 0;
        for (int i = 0; i < r.n; ++i) {
            const double v = p[static_cast<std::size_t>(i)];
            r.hist[std::min(9, static_cast<int>(v * 10))]++;
            r.lt3 += v < 1e-3;
            r.lt6 += v < 1e-6;
            ks = std::max({ks, (i + 1.0) / r.n - v, v - static_cast<double>(i) / r.n});
        }
        if (r.n > 0) {
            r.minp = p[0];
            const double sn = std::sqrt(static_cast<double>(r.n));
            const double lam = (sn + 0.12 + 0.11 / sn) * ks;
            double pks = 0;
            for (int j = 1; j < 100; ++j) pks += 2.0 * ((j & 1) ? 1 : -1) * std::exp(-2.0 * j * j * lam * lam);
            r.pks = std::min(1.0, std::max(0.0, pks));
        }
        return r;
    };
    int total_fail = 0;
    for (const auto& var : variants) {
        if (!only.empty() && std::string(var.name).find(only) == std::string::npos) continue;
        for (int broken = 0; broken < 2; ++broken) {
            std::mt19937_64 rng(seed + 977);
            std::vector<double> p_all, p_core;
            int violated = 0;
            long nd_dense_rows_seen = 0;
            int max_front = 0;
            for (int i = 0; i < nsys; ++i) {
                const System& s = systems[static_cast<std::size_t>(i)];
                UniformSolutionSampler smp;
                smp.solver = var.solver;
                smp.nd_leaf = var.leaf;
                smp.nd_dense_vars = var.dense_vars;
                smp.nd_dense_rows = var.dense_rows;
                smp.test_break = broken;
                smp.reset(s.n);
                for (const auto& row : s.rows) {
                    for (const int v : row) smp.add(v);
                    smp.end_row();
                }
                for (int v = 0; v < s.n; ++v) smp.coord(v, s.xyz[3 * v], s.xyz[3 * v + 1], s.xyz[3 * v + 2]);
                std::vector<std::uint8_t> x;
                std::vector<long> count(static_cast<std::size_t>(s.n_sol), 0);
                bool bad = false, has_core = false;
                for (long k = 0; k < nsamp; ++k) {
                    const auto st = smp.sample(rng, x);
                    if (k == 0) {
                        has_core = st.core_rows > 0;
                    }
                    max_front = std::max(max_front, st.nd_max_front);
                    nd_dense_rows_seen += st.nd_dense_rows;
                    if (!smp.satisfied_by(x)) { bad = true; ++violated; break; }
                    std::uint32_t a = 0;
                    for (int v = 0; v < s.n; ++v) a |= static_cast<std::uint32_t>(x[static_cast<std::size_t>(v)]) << v;
                    const auto id = s.sol_index[a];
                    if (id < 0) { bad = true; ++violated; break; }
                    ++count[static_cast<std::size_t>(id)];
                }
                if (bad || s.n_sol < 2) continue;
                const double e = static_cast<double>(nsamp) / s.n_sol;
                double chi = 0;
                for (const long c : count) chi += (c - e) * (c - e) / e;
                const double p = gammq(0.5 * (s.n_sol - 1), 0.5 * chi);
                p_all.push_back(p);
                if (has_core) p_core.push_back(p);
            }
            const Summary a = summarise(p_all), c = summarise(p_core);
            std::printf("%-12s %-8s violations %d | all: tested %3d p<1e-3 %2d p<1e-6 %2d min p %.1e KS p %.3f | core>0: tested %3d p<1e-3 %2d p<1e-6 %2d min p %.1e KS p %.3f\n",
                        var.name, broken ? "BROKEN" : "correct", violated, a.n, a.lt3, a.lt6, a.minp, a.pks, c.n, c.lt3, c.lt6, c.minp, c.pks);
            std::printf("             hist(all):");
            for (int h : a.hist) std::printf(" %d", h);
            std::printf("   hist(core):");
            for (int h : c.hist) std::printf(" %d", h);
            std::printf("   nd max front %d, mean dense rows/sample %.2f\n", max_front,
                        static_cast<double>(nd_dense_rows_seen) / (static_cast<double>(nsys) * static_cast<double>(nsamp)));
            const bool pass = broken ? (c.lt6 > 0) : (violated == 0 && a.minp > 1e-5 && a.pks > 0.001 && c.pks > 0.001);
            if (!pass) { std::printf("   ^^^ %s\n", broken ? "broken arm NOT detected" : "FAIL"); ++total_fail; }
        }
    }
    std::printf("N1_RESULT %s (unexpected outcomes: %d)\n", total_fail == 0 ? "PASS" : "FAIL", total_fail);
    return total_fail == 0 ? 0 : 1;
}
