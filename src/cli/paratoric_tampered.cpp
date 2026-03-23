// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2025  Simon Mathias Linsel, Lode Pollet

#include "io/io.hpp"
#include "paratoric/types/types.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef PARATORIC_HAS_MPI
#include <mpi.h>
#endif

namespace po = boost::program_options;

namespace {

constexpr int kCliStyle =
    po::command_line_style::default_style
    | po::command_line_style::allow_long_disguise;

std::vector<std::string> normalize_observables(const std::vector<std::string>& raw_tokens) {
    std::vector<std::string> normalized;
    for (const auto& token : raw_tokens) {
        std::string current;
        for (unsigned char ch : token) {
            if (ch == ',' || ch == ';' || ch == ':' || std::isspace(ch)) {
                if (!current.empty()) {
                    normalized.push_back(current);
                    current.clear();
                }
            } else {
                current.push_back(static_cast<char>(ch));
            }
        }
        if (!current.empty()) {
            normalized.push_back(current);
        }
    }
    return normalized;
}

std::vector<double> parse_numeric_list_tokens(const std::vector<std::string>& raw_tokens) {
    std::vector<double> values;
    for (const auto& token : raw_tokens) {
        std::string current;
        for (unsigned char ch : token) {
            if (ch == ',' || std::isspace(ch)) {
                if (!current.empty()) {
                    values.emplace_back(std::stod(current));
                    current.clear();
                }
            } else {
                current.push_back(static_cast<char>(ch));
            }
        }
        if (!current.empty()) {
            values.emplace_back(std::stod(current));
        }
    }
    return values;
}

std::string resolve_config_path(int argc, char** argv) {
    std::string config_path_opt;
    std::string config_path_positional;

    // qmc_worm-style: first positional argument can be the config file
    if (argc > 1) {
        const std::string first_arg = argv[1];
        if (!first_arg.empty() && first_arg[0] != '-') {
            config_path_positional = first_arg;
        }
    }

    // explicit --config path, supports "--config FILE" and "--config=FILE"
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--config") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--config requires a file path argument.");
            }
            config_path_opt = argv[i + 1];
            break;
        }
        const std::string prefix = "--config=";
        if (arg.rfind(prefix, 0) == 0) {
            config_path_opt = arg.substr(prefix.size());
            break;
        }
    }

    if (!config_path_opt.empty() && !config_path_positional.empty() && config_path_opt != config_path_positional) {
        throw std::invalid_argument(
            std::format("Conflicting config paths: --config={} vs positional={}.",
                        config_path_opt, config_path_positional));
    }

    if (!config_path_opt.empty()) {
        return config_path_opt;
    }
    return config_path_positional;
}

struct MpiSession {
#ifdef PARATORIC_HAS_MPI
    // Initialize/finalize MPI only for this process lifetime if not already managed externally.
    explicit MpiSession(int* argc, char*** argv) {
        int initialized = 0;
        MPI_Initialized(&initialized);
        if (!initialized) {
            MPI_Init(argc, argv);
            owns_session_ = true;
        }
    }

    ~MpiSession() {
        int finalized = 0;
        MPI_Finalized(&finalized);
        if (owns_session_ && !finalized) {
            MPI_Finalize();
        }
    }

private:
    bool owns_session_ = false;
#else
    explicit MpiSession(int*, char***) {}
#endif
};

} // namespace

int main(int argc, char** argv) {
    try {
        std::string simulation = "etc_sample";
        int N_samples = 1000;
        int N_thermalization = 10000;
        int N_between_samples = 1000;
        double beta = 16.;
        double h_constant = 0.0;
        double h_constant_therm = std::numeric_limits<double>::quiet_NaN();
        std::vector<double> h_hys;
        double mu_constant = 1.0;
        double J_constant = 1.0;
        double lmbda_constant = 0.0;
        double lmbda_constant_therm = std::numeric_limits<double>::quiet_NaN();
        std::vector<double> lmbda_hys;
        int N_resamples = 1000;
        bool custom_therm = false;
        std::vector<std::string> observables_raw;
        int seed = 0;
        char basis = 'x';
        std::string lattice_type = "square";
        int system_size = 16;
        std::string boundaries = "periodic";
        int default_spin = +1;
        std::filesystem::path path_output_directory = ".";
        std::string folder_name;
        std::vector<std::string> folder_names;
        bool save_snapshots = false;
        bool full_time_series = false;
        int process_index = 0;
        std::string config_file_positional;

        // PT options (validated here, execution deferred to next step)
        bool pt_enabled = false;
        std::string pt_parameter;
        int pt_replicas = 0;
        int pt_swap_period = 0;
        double pt_ladder_min = 0.0;
        double pt_ladder_max = 0.0;
        std::vector<std::string> pt_ladder_values_raw;
        bool pt_trace_enabled = false;
        int pt_trace_interval = 0;

        po::options_description desc("Run extended toric code QMC simulation (tampered CLI)");
        desc.add_options()
            ("help,h", "Display help message")
            ("config", po::value<std::string>(), "Path to params.ini configuration file.")
            ("simulation,sim", po::value(&simulation), "Simulation you want to run.")
            ("N_samples,Ns", po::value(&N_samples), "Number of samples taken during the simulation.")
            ("N_thermalization,Nth", po::value(&N_thermalization), "Number of thermalization steps.")
            ("N_between_samples,Nbs", po::value(&N_between_samples), "Number of steps between samples.")
            ("beta,bet", po::value(&beta), "Inverse temperature.")
            ("h_constant,hc", po::value(&h_constant), "Value of the constant h (electric field term).")
            ("h_hysteresis,hhys", po::value(&h_hys)->multitoken(), "Hysteresis values of the constant h (electric field term).")
            ("h_constant_therm,hct", po::value(&h_constant_therm), "Value of the constant h for thermalization (electric field term).")
            ("mu_constant,muc", po::value(&mu_constant), "Value of the constant mu (star term).")
            ("J_constant,Jc", po::value(&J_constant), "Value of the constant J (plaquette term).")
            ("lmbda_constant,lmbdac", po::value(&lmbda_constant), "Value of the constant lmbda (gauge field term).")
            ("lmbda_hysteresis,lmbdahys", po::value(&lmbda_hys)->multitoken(), "Hysteresis values of the constant lmbda (gauge field term).")
            ("lmbda_constant_therm,lmbdact", po::value(&lmbda_constant_therm), "Value of the constant lmbda for thermalization (gauge field term).")
            ("N_resamples,Nr", po::value(&N_resamples), "Number of bootstrap resamples.")
            ("custom_therm,cth", po::value(&custom_therm), "Whether custom thermalization is used (to probe hysteresis).")
            ("observables,obs", po::value(&observables_raw)->multitoken(), "Observables that are measured.")
            ("seed,s", po::value(&seed), "Seed for the pseudorandom number generator.")
            ("basis,bas", po::value(&basis), "Spin basis (\"x\" or \"z\").")
            ("output_directory,outdir", po::value(&path_output_directory), "Directory where the output is stored.")
            ("folder_name,fn", po::value(&folder_name), "Name of the output subfolder.")
            ("folder_names,fns", po::value(&folder_names)->multitoken(), "Directories where hysteresis output is stored.")
            ("snapshots,snap", po::value(&save_snapshots), "Whether snapshots should be saved.")
            ("full_time_series,fts", po::value(&full_time_series), "Whether full time series should be saved.")
            ("process_index,procid", po::value(&process_index), "Identifier of process (for debugging).")
            ("lattice_type,lat", po::value(&lattice_type), "Type of lattice used.")
            ("system_size,L", po::value(&system_size), "System size of lattice (one coordinate).")
            ("boundaries,bound", po::value(&boundaries), "Boundary condition of the lattice (periodic or open).")
            ("default_spin,dsp", po::value(&default_spin), "Default spin (electric field) for lattice initialization.")
            ("pt_enabled", po::value(&pt_enabled), "Enable parallel tempering mode.")
            ("pt_parameter", po::value(&pt_parameter), "Tempered parameter name in z-basis PT (h, mu, J, or lmbda).")
            ("pt_replicas", po::value(&pt_replicas), "Number of PT replicas.")
            ("pt_swap_period", po::value(&pt_swap_period), "Swap attempt interval in local update steps.")
            ("pt_ladder_min", po::value(&pt_ladder_min), "Lower bound of PT ladder.")
            ("pt_ladder_max", po::value(&pt_ladder_max), "Upper bound of PT ladder.")
            ("pt_ladder_values", po::value(&pt_ladder_values_raw)->multitoken(), "Explicit PT ladder values (comma/space separated). Overrides min/max when set.")
            ("pt_trace_enabled", po::value(&pt_trace_enabled), "Enable detailed PT swap trace logging to output_test.txt.")
            ("pt_trace_interval", po::value(&pt_trace_interval), "PT swap trace interval in local update steps (used when pt_trace_enabled=true).");

        po::options_description hidden("Hidden options");
        hidden.add_options()
            ("config_file", po::value(&config_file_positional), "Positional params.ini file");

        po::options_description all_options("All options");
        all_options.add(desc).add(hidden);

        po::positional_options_description positional;
        positional.add("config_file", 1);

        const std::string config_path = resolve_config_path(argc, argv);

        // Read INI first so values become defaults that CLI can override.
        po::variables_map vm_config;
        if (!config_path.empty()) {
            std::ifstream cfg(config_path);
            if (!cfg) {
                throw std::runtime_error(std::format("Could not open config file: {}", config_path));
            }
            // strict parsing: unknown keys in INI throw
            po::store(po::parse_config_file(cfg, desc, false), vm_config);
            po::notify(vm_config);
        }

        // Parse CLI separately; typed_value bindings update target variables on notify().
        po::command_line_parser full_parser(argc, argv);
        full_parser.options(all_options).positional(positional).style(kCliStyle);
        po::variables_map vm_cli;
        po::store(full_parser.run(), vm_cli);
        po::notify(vm_cli);

        if (vm_config.count("help") || vm_cli.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        const std::vector<std::string> observables = normalize_observables(observables_raw);
        const std::vector<double> pt_ladder_values = parse_numeric_list_tokens(pt_ladder_values_raw);

        paratoric::PTSpec pt_spec = {
            .enabled = pt_enabled,
            .parameter = pt_parameter,
            .replicas = pt_replicas,
            .swap_period = pt_swap_period,
            .ladder_min = pt_ladder_min,
            .ladder_max = pt_ladder_max,
            .ladder_values = pt_ladder_values,
            .trace_enabled = pt_trace_enabled,
            .trace_interval = pt_trace_interval
        };

        std::unique_ptr<MpiSession> mpi_session;
        if (pt_spec.enabled) {
            // Delay MPI init until PT is requested to keep non-PT CLI behavior unchanged.
            mpi_session = std::make_unique<MpiSession>(&argc, &argv);
        }

        if (pt_spec.enabled) {
            if (pt_spec.parameter.empty()) {
                throw std::invalid_argument("pt_parameter must be set when pt_enabled=true.");
            }
            if (pt_spec.replicas < 2) {
                throw std::invalid_argument("pt_replicas must be >= 2 when pt_enabled=true.");
            }
            if (pt_spec.swap_period < 1) {
                throw std::invalid_argument("pt_swap_period must be >= 1 when pt_enabled=true.");
            }
            if (pt_spec.ladder_values.empty() && !(pt_spec.ladder_max > pt_spec.ladder_min)) {
                throw std::invalid_argument("pt_ladder_max must be greater than pt_ladder_min when pt_enabled=true and pt_ladder_values is not provided.");
            }
            if (!pt_spec.ladder_values.empty() &&
                static_cast<int>(pt_spec.ladder_values.size()) != pt_spec.replicas) {
                throw std::invalid_argument("pt_ladder_values length must equal pt_replicas when provided.");
            }
            if (pt_spec.trace_enabled && pt_spec.trace_interval < 1) {
                throw std::invalid_argument("pt_trace_interval must be >= 1 when pt_trace_enabled=true.");
            }
            if (simulation != "etc_sample") {
                throw std::invalid_argument("PT scaffold currently supports simulation=etc_sample only.");
            }
            if (custom_therm) {
                throw std::invalid_argument("PT scaffold currently requires custom_therm=false.");
            }
            if (basis != 'z') {
                throw std::invalid_argument("PT scaffold currently supports basis=z only.");
            }
#ifndef PARATORIC_HAS_MPI
            throw std::invalid_argument("pt_enabled=true requires MPI build support. Reconfigure with -DPARATORIC_LINK_MPI=ON.");
#else
            // Mirror rank into process_index so logs map one-to-one to MPI replicas.
            int rank = 0;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            process_index = rank;
#endif
        }

        BOOST_LOG_TRIVIAL(info) << "Subprocess [ID=" << process_index
                                << "] SPAWNED with [beta=" << beta
                                << "], [mu=" << mu_constant
                                << "], [J=" << J_constant
                                << "], [h=" << h_constant
                                << "], [lambda=" << lmbda_constant << "].";

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        paratoric::LatSpec lat_spec = {
            basis,
            lattice_type,
            system_size,
            beta,
            boundaries,
            default_spin
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
            .observables = observables
        };
        paratoric::OutSpec out_spec = {
            .path_out = path_output_directory,
            .folder_name = folder_name,
            .folder_names = folder_names,
            .save_snapshots = save_snapshots,
            .full_time_series = full_time_series
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
        } else {
            throw std::invalid_argument(std::format("Unknown simulation type: {}", simulation));
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        const auto dt = end - begin;
        const auto days = std::chrono::duration_cast<std::chrono::days>(dt);
        const std::chrono::hh_mm_ss<std::chrono::microseconds> hms{
            floor<std::chrono::microseconds>(dt - days)};

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
