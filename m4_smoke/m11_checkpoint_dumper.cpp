// Read-only checkpoint dumper for salvaging the per-rank observable series.
//
// Usage:
//   m11_checkpoint_dumper /path/to/rankN.bin
//
// Output is three parseable lines: one metadata line, one series-length line,
// and one space-separated delta_tau_avg_series line. The loader used by this
// tool is the production checkpoint reader; this executable never opens the
// supplied checkpoint for writing.

#include "mcmc/extended_toric_code_qmc.hpp"

#include <algorithm>
#include <complex>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <variant>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: m11_checkpoint_dumper CHECKPOINT_PATH\n";
        return 2;
    }

    try {
        paratoric::ExtendedToricCodeQMC<'z'> qmc;
        const auto dump = qmc.read_checkpoint_for_dump(argv[1]);
        const auto observable_it = std::find(
            dump.observables.begin(), dump.observables.end(), "delta_tau_avg");
        if (observable_it == dump.observables.end()) {
            throw std::runtime_error("Checkpoint does not contain delta_tau_avg.");
        }
        const auto observable_index = static_cast<std::size_t>(
            std::distance(dump.observables.begin(), observable_it));
        if (observable_index >= dump.series.size()) {
            throw std::runtime_error("Checkpoint observable series count is inconsistent.");
        }

        const auto& delta_series = dump.series[observable_index];
        std::cout << std::setprecision(17)
                  << "completed_samples=" << dump.completed_samples
                  << " pt_ladder_index=" << dump.pt_ladder_index
                  << " runtime_h=" << dump.runtime_h
                  << " runtime_mu=" << dump.runtime_mu
                  << " runtime_J=" << dump.runtime_J
                  << " runtime_lmbda=" << dump.runtime_lmbda << '\n';
        std::cout << "delta_tau_avg_series_length=" << delta_series.size() << '\n';
        std::cout << "delta_tau_avg_series=";
        for (const auto& value : delta_series) {
            const auto* real_value = std::get_if<double>(&value);
            if (real_value == nullptr) {
                throw std::runtime_error("delta_tau_avg series contains a complex value.");
            }
            std::cout << *real_value << ' ';
        }
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "checkpoint_dump_error=" << error.what() << '\n';
        return 1;
    }
}
