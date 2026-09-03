// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#include "io/io.hpp"
#include "logging/trivial_log.hpp"
#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace po = boost::program_options;

int main(int argc, char **argv) {
    try {
        std::string simulation;
        int N_samples;
        std::int64_t N_thermalization;
        int N_between_samples;
        double beta;
        double h_constant;
        double h_constant_therm;
        std::vector<double> h_hys;
        double mu_constant;
        double J_constant;
        double lmbda_constant;
        double lmbda_constant_therm;
        std::vector<double> lmbda_hys;
        int N_resamples;
        bool custom_therm;
        std::vector<std::string> obs;
        int seed;
        char basis;
        std::string lattice_type;
        std::string lattice_backend;
        int system_size;
        std::string boundaries;
        int default_spin;
        std::filesystem::path path_output_directory;
        std::string folder_name;
        std::vector<std::string> folder_names;
        bool save_snapshots;
        bool full_time_series;
        std::filesystem::path checkpoint_path;
        std::filesystem::path restart_from;
        int checkpoint_every_samples;
        int checkpoint_every_seconds;
        std::vector<double> pt_ladder;
        double pt_ladder_start;
        double pt_ladder_end;
        int pt_ladder_count;
        double pt_ladder_lmbda_start;
        double pt_ladder_lmbda_end;
        // Common-factor (beta,s) tempering (M5,
        // doc/coupled_tempering_implementation_plan.md). Default OFF: with neither
        // flag given, ladder_mu/ladder_J stay empty and behaviour is unchanged.
        std::vector<double> pt_ladder_scale_values;
        std::vector<double> pt_ladder_mu_values;
        std::vector<double> pt_ladder_J_values;
        double pt_ladder_mu_start;
        double pt_ladder_mu_end;
        double pt_ladder_J_start;
        double pt_ladder_J_end;
        double pt_swap_period_sweeps;
        int pt_swap_period = 0;
        bool pt_validate_swap_weight;
        int process_index;

        po::options_description desc("Run extended toric code QMC simulation");
        desc.add_options()
        ("simulation,sim", po::value(&simulation), "Simulation you want to run.")
        ("N_samples,Ns", po::value(&N_samples), "Number of samples taken during the simulation.")
        ("N_thermalization,Nth", po::value(&N_thermalization), "Number of thermalization steps.")
        ("N_between_samples,Nbs", po::value(&N_between_samples), "Number of steps between samples.")
        ("beta,bet", po::value(&beta), "Inverse temperature.")
        ("h_constant,hc", po::value(&h_constant), "Value of the constant h (electric field term).")
        ("h_hysteresis,hhys", po::value(&h_hys)->multitoken(), "Hysteresis values of the constant h (electric field term).")
        ("h_constant_therm,hct", po::value(&h_constant_therm), "Value of the constant h for thermalization (electric field term).")
        ("mu_constant,muc", po::value(&mu_constant), "Value of the constant mu_constant (star term).")
        ("J_constant,Jc", po::value(&J_constant), "Value of the constant J (plaquette term).")
        ("lmbda_constant,lmbdac", po::value(&lmbda_constant), "Value of the constant lmbda (gauge field term).")
        ("lmbda_hysteresis,lmbdahys", po::value(&lmbda_hys)->multitoken(), "Hysteresis values of the constant lmbda (gauge field term).")
        ("lmbda_constant_therm,lmbdact", po::value(&lmbda_constant_therm), "Value of the constant lmbda for thermalization (gauge field term).")
        ("N_resamples,Nr", po::value(&N_resamples), "Number of bootstrap resamples.")
        ("custom_therm,cth", po::value(&custom_therm), "Whether custom thermalization is used (to probe hysteresis).")
        ("observables,obs", po::value(&obs)->multitoken(), "Observables that are measured.") // can take multiple observables at once
        ("seed,s", po::value(&seed), "Seed for the pseudorandom number generator.")
        ("basis,bas", po::value(&basis), "Spin basis (\"x\" or \"z\").")
        ("output_directory,outdir", po::value(&path_output_directory), "Directory where the output is stored.")
        ("folder_name,fn", po::value(&folder_name), "Name of the output subfolder.")
        ("folder_names,fns", po::value(&folder_names)->multitoken(), "Directories where hysteresis output is stored.")
        ("snapshots,snap", po::value(&save_snapshots), "Whether snapshots should be saved.")
        ("full_time_series,fts", po::value(&full_time_series), "Whether full time series should be saved.")
        ("checkpoint_path", po::value(&checkpoint_path)->default_value({}), "Path for periodic etc_sample checkpoints.")
        ("restart_from", po::value(&restart_from)->default_value({}), "Resume etc_sample from a checkpoint file.")
        ("checkpoint_every_samples", po::value(&checkpoint_every_samples)->default_value(0), "Write an etc_sample checkpoint every N completed samples.")
        ("checkpoint_every_seconds", po::value(&checkpoint_every_seconds)->default_value(0), "Write an etc_sample checkpoint every N seconds, checked at sample boundaries.")
        ("pt_ladder", po::value(&pt_ladder)->multitoken(), "Enable PT with explicit h ladder values; with no lambda ladder, uses the self-dual h=lambda ladder. Requires mu=J=1.")
        ("pt_ladder_start", po::value(&pt_ladder_start)->default_value(std::numeric_limits<double>::quiet_NaN()), "Start of an evenly spaced PT h ladder.")
        ("pt_ladder_end", po::value(&pt_ladder_end)->default_value(std::numeric_limits<double>::quiet_NaN()), "End of an evenly spaced PT h ladder.")
        ("pt_ladder_count", po::value(&pt_ladder_count)->default_value(0), "Number of points in evenly spaced PT h and optional lambda ladders.")
        ("pt_ladder_lmbda_start", po::value(&pt_ladder_lmbda_start)->default_value(std::numeric_limits<double>::quiet_NaN()), "Start of an evenly spaced PT lambda ladder.")
        ("pt_ladder_lmbda_end", po::value(&pt_ladder_lmbda_end)->default_value(std::numeric_limits<double>::quiet_NaN()), "End of an evenly spaced PT lambda ladder.")
        ("pt_ladder_mu", po::value(&pt_ladder_mu_values)->multitoken(), "Explicit PT mu ladder values, one per rung. Use instead of the evenly spaced pt_ladder_mu_start/end pair when the rungs are NOT evenly spaced -- a geometric (constant-ratio) ladder is the measured optimum, and even spacing puts a bottleneck at the cold end.")
        ("pt_ladder_J", po::value(&pt_ladder_J_values)->multitoken(), "Explicit PT J ladder values, one per rung. See pt_ladder_mu.")
        ("pt_ladder_scale", po::value(&pt_ladder_scale_values)->multitoken(), "Common factor c per rung, DECLARED not inferred. Optional; default all ones, in which case every observable is returned unchanged. When given, an observable is divided by c^p with p its measured c exponent, so each rung reports in c=1 UNITS. It does NOT make the rungs physically identical, they stay at different effective temperature. A mismatch against the actual ladder throws.")
        ("pt_ladder_mu_start", po::value(&pt_ladder_mu_start)->default_value(std::numeric_limits<double>::quiet_NaN()), "Start of an evenly spaced PT mu ladder (common-factor tempering). Default off.")
        ("pt_ladder_mu_end", po::value(&pt_ladder_mu_end)->default_value(std::numeric_limits<double>::quiet_NaN()), "End of an evenly spaced PT mu ladder (common-factor tempering). Default off.")
        ("pt_ladder_J_start", po::value(&pt_ladder_J_start)->default_value(std::numeric_limits<double>::quiet_NaN()), "Start of an evenly spaced PT J ladder (common-factor tempering). Default off.")
        ("pt_ladder_J_end", po::value(&pt_ladder_J_end)->default_value(std::numeric_limits<double>::quiet_NaN()), "End of an evenly spaced PT J ladder (common-factor tempering). Default off.")
        ("pt_swap_period_sweeps", po::value(&pt_swap_period_sweeps)->default_value(2.0), "PT neighbor-swap attempt period in sweeps; one sweep is 2*L^2 Metropolis updates.")
        ("pt_swap_period", po::value(&pt_swap_period), "Legacy PT neighbor-swap attempt period in raw Metropolis updates. Do not combine with pt_swap_period_sweeps.")
        ("pt_validate_swap_weight", po::value(&pt_validate_swap_weight)->default_value(false), "Recompute PT swap weights at every swap attempt and record max deviation.")
        ("process_index,procid", po::value(&process_index), "Identifier of process (for debugging).")
        ("lattice_type,lat", po::value(&lattice_type), "Type of lattice used.")
        ("lattice_backend,latbe", po::value(&lattice_backend)->default_value("boost"), "Lattice backend used by benchmark modes (boost or flat_square).")
        ("system_size,L", po::value(&system_size), "System size of lattice (one coordinate).")
        ("boundaries,bound", po::value(&boundaries), "Boundary condition of the lattice (periodic or open).")
        ("default_spin,dsp", po::value(&default_spin), "Default spin (electric field) for lattice initialization.");

        po::variables_map vm;
        po::command_line_parser parser(argc, argv);
        parser.options(desc).style(
            po::command_line_style::default_style
            | po::command_line_style::allow_long_disguise
        );
        po::store(parser.run(), vm);
        po::notify(vm); 

        BOOST_LOG_TRIVIAL(info) << "Subprocess [ID=" << process_index 
                                << "] SPAWNED with [β=" << beta 
                                << "], [μ=" << mu_constant 
                                << "], [J=" << J_constant 
                                << "], [h=" << h_constant 
                                << "], [λ=" << lmbda_constant << "].";

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        paratoric::LatSpec lat_spec = {
            basis,
            lattice_type,
            system_size,
            beta,
            boundaries,
            default_spin,
            lattice_backend
        };
        paratoric::ParamSpec param_spec = {
            .mu = mu_constant,
            .h = h_constant,
            .J = J_constant,
            .lmbda = lmbda_constant,
            .h_therm = h_constant_therm,
            .lmbda_therm = lmbda_constant_therm,
            .h_hys = h_hys,
            .lmbda_hys = lmbda_hys
        };

        paratoric::SimSpec sim_spec = {
            .N_samples = N_samples,
            .N_thermalization = N_thermalization,
            .N_between_samples = N_between_samples,
            .N_resamples = N_resamples,
            .custom_therm = custom_therm,
            .seed = seed,
            .observables = obs
        };
        paratoric::OutSpec out_spec = {
            .path_out = path_output_directory,
            .folder_name = folder_name,
            .folder_names = folder_names,
            .save_snapshots = save_snapshots,
            .full_time_series = full_time_series,
            .checkpoint_path = checkpoint_path,
            .restart_from = restart_from,
            .checkpoint_every_samples = checkpoint_every_samples,
            .checkpoint_every_seconds = checkpoint_every_seconds
        };
        if (pt_ladder.empty() && pt_ladder_count > 0) {
            if (!std::isfinite(pt_ladder_start) || !std::isfinite(pt_ladder_end)) {
                throw std::invalid_argument("pt_ladder_start and pt_ladder_end are required with pt_ladder_count.");
            }
            if (pt_ladder_count == 1) {
                pt_ladder.push_back(pt_ladder_start);
            } else {
                pt_ladder.reserve(static_cast<size_t>(pt_ladder_count));
                for (int i = 0; i < pt_ladder_count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(pt_ladder_count - 1);
                    pt_ladder.push_back(pt_ladder_start + t * (pt_ladder_end - pt_ladder_start));
                }
            }
        }
        if (!pt_ladder.empty()) {
            if (std::any_of(pt_ladder.begin(), pt_ladder.end(), [](double s) { return !std::isfinite(s) || s <= 0.0; })) {
                throw std::invalid_argument("pt_ladder values must be finite and positive.");
            }
            if (!std::is_sorted(pt_ladder.begin(), pt_ladder.end())) {
                throw std::invalid_argument("pt_ladder values must be sorted in ascending order.");
            }
            // RELAXED 2026-09-03 from strictly increasing to NON-DECREASING, so that a
            // CONSTANT h ladder is legal. That is what a (mu - J) ladder at fixed s needs:
            // every rung carries the same h, and the tilt lives entirely in mu and J, which
            // have no sortedness constraint of their own. Descending h still throws, via the
            // is_sorted check above.
            // Safe because NOTHING downstream assumes the ladder is strictly increasing:
            // swap partners are chosen by LADDER INDEX (parallel_tempering_partner_rank),
            // not by coupling value, and the only is_sorted/lower_bound/binary_search calls
            // in the engine act on imaginary-time flip lists in flat_square_lattice.hpp.
        }
        const bool has_pt_ladder_lmbda_start = std::isfinite(pt_ladder_lmbda_start);
        const bool has_pt_ladder_lmbda_end = std::isfinite(pt_ladder_lmbda_end);
        if (has_pt_ladder_lmbda_start != has_pt_ladder_lmbda_end) {
            throw std::invalid_argument("pt_ladder_lmbda_start and pt_ladder_lmbda_end must be supplied together.");
        }
        std::vector<double> pt_ladder_lmbda;
        if (has_pt_ladder_lmbda_start) {
            if (pt_ladder_count <= 0) {
                throw std::invalid_argument("pt_ladder_count must be positive with pt_ladder_lmbda_start and pt_ladder_lmbda_end.");
            }
            pt_ladder_lmbda.reserve(static_cast<size_t>(pt_ladder_count));
            if (pt_ladder_count == 1) {
                pt_ladder_lmbda.push_back(pt_ladder_lmbda_start);
            } else {
                for (int i = 0; i < pt_ladder_count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(pt_ladder_count - 1);
                    pt_ladder_lmbda.push_back(
                        pt_ladder_lmbda_start + t * (pt_ladder_lmbda_end - pt_ladder_lmbda_start));
                }
            }
            if (std::any_of(pt_ladder_lmbda.begin(), pt_ladder_lmbda.end(), [](double lmbda) {
                    return !std::isfinite(lmbda) || lmbda <= 0.0;
                })) {
                throw std::invalid_argument("pt_ladder_lmbda values must be finite and positive.");
            }
            if (pt_ladder.empty()) {
                throw std::invalid_argument("pt_ladder h values are required with a pt_ladder_lmbda range.");
            }
            if (pt_ladder_lmbda.size() != pt_ladder.size()) {
                throw std::invalid_argument("pt_ladder_lmbda and pt_ladder must have the same number of values.");
            }
        }

        // Common-factor (beta,s) tempering: mu and J ladders, same pattern as the
        // lambda ladder above. Default OFF -- neither flag given leaves both vectors
        // empty, and ParallelTemperingSpec falls back to param_spec.mu/.J unchanged
        // (M3's bit-for-bit guardrail).
        const bool has_pt_ladder_mu_start = std::isfinite(pt_ladder_mu_start);
        const bool has_pt_ladder_mu_end = std::isfinite(pt_ladder_mu_end);
        if (has_pt_ladder_mu_start != has_pt_ladder_mu_end) {
            throw std::invalid_argument("pt_ladder_mu_start and pt_ladder_mu_end must be supplied together.");
        }
        std::vector<double> pt_ladder_mu;
        if (!pt_ladder_mu_values.empty()) {
            if (has_pt_ladder_mu_start) {
                throw std::invalid_argument("pt_ladder_mu cannot be combined with pt_ladder_mu_start/pt_ladder_mu_end.");
            }
            pt_ladder_mu = pt_ladder_mu_values;
            if (std::any_of(pt_ladder_mu.begin(), pt_ladder_mu.end(), [](double m) {
                    return !std::isfinite(m) || m <= 0.0;
                })) {
                throw std::invalid_argument("pt_ladder_mu values must be finite and positive.");
            }
            if (pt_ladder.empty()) {
                throw std::invalid_argument("pt_ladder h values are required with pt_ladder_mu.");
            }
            if (pt_ladder_mu.size() != pt_ladder.size()) {
                throw std::invalid_argument("pt_ladder_mu and pt_ladder must have the same number of values.");
            }
        } else if (has_pt_ladder_mu_start) {
            if (pt_ladder_count <= 0) {
                throw std::invalid_argument("pt_ladder_count must be positive with pt_ladder_mu_start and pt_ladder_mu_end.");
            }
            pt_ladder_mu.reserve(static_cast<size_t>(pt_ladder_count));
            if (pt_ladder_count == 1) {
                pt_ladder_mu.push_back(pt_ladder_mu_start);
            } else {
                for (int i = 0; i < pt_ladder_count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(pt_ladder_count - 1);
                    pt_ladder_mu.push_back(pt_ladder_mu_start + t * (pt_ladder_mu_end - pt_ladder_mu_start));
                }
            }
            if (std::any_of(pt_ladder_mu.begin(), pt_ladder_mu.end(), [](double m) {
                    return !std::isfinite(m) || m <= 0.0;
                })) {
                throw std::invalid_argument("pt_ladder_mu values must be finite and positive.");
            }
            if (pt_ladder.empty()) {
                throw std::invalid_argument("pt_ladder h values are required with a pt_ladder_mu range.");
            }
            if (pt_ladder_mu.size() != pt_ladder.size()) {
                throw std::invalid_argument("pt_ladder_mu and pt_ladder must have the same number of values.");
            }
        }

        const bool has_pt_ladder_J_start = std::isfinite(pt_ladder_J_start);
        const bool has_pt_ladder_J_end = std::isfinite(pt_ladder_J_end);
        if (has_pt_ladder_J_start != has_pt_ladder_J_end) {
            throw std::invalid_argument("pt_ladder_J_start and pt_ladder_J_end must be supplied together.");
        }
        std::vector<double> pt_ladder_J;
        if (!pt_ladder_J_values.empty()) {
            if (has_pt_ladder_J_start) {
                throw std::invalid_argument("pt_ladder_J cannot be combined with pt_ladder_J_start/pt_ladder_J_end.");
            }
            pt_ladder_J = pt_ladder_J_values;
            if (std::any_of(pt_ladder_J.begin(), pt_ladder_J.end(), [](double j) {
                    return !std::isfinite(j) || j <= 0.0;
                })) {
                throw std::invalid_argument("pt_ladder_J values must be finite and positive.");
            }
            if (pt_ladder.empty()) {
                throw std::invalid_argument("pt_ladder h values are required with pt_ladder_J.");
            }
            if (pt_ladder_J.size() != pt_ladder.size()) {
                throw std::invalid_argument("pt_ladder_J and pt_ladder must have the same number of values.");
            }
        } else if (has_pt_ladder_J_start) {
            if (pt_ladder_count <= 0) {
                throw std::invalid_argument("pt_ladder_count must be positive with pt_ladder_J_start and pt_ladder_J_end.");
            }
            pt_ladder_J.reserve(static_cast<size_t>(pt_ladder_count));
            if (pt_ladder_count == 1) {
                pt_ladder_J.push_back(pt_ladder_J_start);
            } else {
                for (int i = 0; i < pt_ladder_count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(pt_ladder_count - 1);
                    pt_ladder_J.push_back(pt_ladder_J_start + t * (pt_ladder_J_end - pt_ladder_J_start));
                }
            }
            if (std::any_of(pt_ladder_J.begin(), pt_ladder_J.end(), [](double j) {
                    return !std::isfinite(j) || j <= 0.0;
                })) {
                throw std::invalid_argument("pt_ladder_J values must be finite and positive.");
            }
            if (pt_ladder.empty()) {
                throw std::invalid_argument("pt_ladder h values are required with a pt_ladder_J range.");
            }
            if (pt_ladder_J.size() != pt_ladder.size()) {
                throw std::invalid_argument("pt_ladder_J and pt_ladder must have the same number of values.");
            }
        }

        const bool explicit_pt_swap_period_sweeps = vm.count("pt_swap_period_sweeps") > 0
            && !vm["pt_swap_period_sweeps"].defaulted();
        if (vm.count("pt_swap_period") > 0 && explicit_pt_swap_period_sweeps) {
            throw std::invalid_argument("Use either pt_swap_period_sweeps or legacy pt_swap_period, not both.");
        }
        if (!std::isfinite(pt_swap_period_sweeps) || pt_swap_period_sweeps <= 0.0) {
            throw std::invalid_argument("pt_swap_period_sweeps must be finite and positive.");
        }
        if (vm.count("pt_swap_period") > 0 && pt_swap_period <= 0) {
            throw std::invalid_argument("pt_swap_period must be positive when supplied.");
        }
        paratoric::ParallelTemperingSpec pt_spec = {
            .enabled = !pt_ladder.empty(),
            .ladder_s = pt_ladder,
            .ladder_lmbda = pt_ladder_lmbda,
            .ladder_mu = pt_ladder_mu,
            .ladder_J = pt_ladder_J,
            .ladder_scale = pt_ladder_scale_values,
            .swap_period_sweeps = pt_swap_period_sweeps,
            .swap_period = pt_swap_period,
            .validate_swap_weight = pt_validate_swap_weight
        };

        if (simulation == "etc_sample") {
            auto io = paratoric::IO();
            io.etc_sample(
                paratoric::Config{sim_spec, param_spec, lat_spec, out_spec, pt_spec}
                );
        } else if (simulation == "etc_hysteresis") {
            auto io = paratoric::IO();
            io.etc_hysteresis(
                paratoric::Config{sim_spec, param_spec, lat_spec, out_spec, pt_spec}
                );
        } else if (simulation == "etc_thermalization") {
            auto io = paratoric::IO();
            io.etc_thermalization(
                paratoric::Config{sim_spec, param_spec, lat_spec, out_spec, pt_spec}
                );
        } else if (simulation == "etc_core_update_benchmark") {
            auto run_benchmark = [&](auto& mc) {
                return mc.get_core_update_benchmark(
                    paratoric::Config{sim_spec, param_spec, lat_spec, out_spec}
                );
            };

            paratoric::Result result;
            if (basis == 'x') {
                if (lattice_backend == "boost") {
                    auto mc = paratoric::ExtendedToricCodeQMC<'x'>();
                    result = run_benchmark(mc);
                } else if (lattice_backend == "flat_square") {
                    auto mc = paratoric::ExtendedToricCodeQMC<'x', paratoric::FlatSquareLattice>();
                    result = run_benchmark(mc);
                } else {
                    throw std::invalid_argument("lattice_backend must be boost or flat_square.");
                }
            } else if (basis == 'z') {
                if (lattice_backend == "boost") {
                    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
                    result = run_benchmark(mc);
                } else if (lattice_backend == "flat_square") {
                    auto mc = paratoric::ExtendedToricCodeQMC<'z', paratoric::FlatSquareLattice>();
                    result = run_benchmark(mc);
                } else {
                    throw std::invalid_argument("lattice_backend must be boost or flat_square.");
                }
            } else {
                throw std::invalid_argument("basis must be x or z.");
            }

            const auto& diagnostics = result.production_acceptance;
            const double acceptance_fraction = diagnostics.attempted == 0
                ? 0.0
                : static_cast<double>(diagnostics.accepted) / static_cast<double>(diagnostics.attempted);
            const double mean_acceptance_ratio = diagnostics.attempted == 0
                ? 0.0
                : diagnostics.acceptance_ratio_sum / static_cast<double>(diagnostics.attempted);
            BOOST_LOG_TRIVIAL(info)
                << std::format(
                    "Core update benchmark backend={} attempted={} accepted={} acceptance_fraction={} mean_acceptance_ratio={}",
                    lattice_backend,
                    diagnostics.attempted,
                    diagnostics.accepted,
                    acceptance_fraction,
                    mean_acceptance_ratio);
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        const auto dt = end - begin;
        const auto days = std::chrono::duration_cast<std::chrono::days>(dt);
        const std::chrono::hh_mm_ss<std::chrono::microseconds> hms{floor<std::chrono::microseconds>(dt - days)};

        BOOST_LOG_TRIVIAL(info)
        << std::format(
            "Subprocess [ID={}] COMPLETED in [{}-{:02}:{:02}:{:02}.{:06} h].",
            process_index,
            days.count(),
            hms.hours().count(),
            hms.minutes().count(),
            hms.seconds().count(),
            hms.subseconds().count());

        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "Exception caught: " << exc.what() << std::endl;
        return 2;
    } 
}
