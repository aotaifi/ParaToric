// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#include "io/io.hpp"

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <H5Cpp.h>  

#include <algorithm>   // std::min
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>  

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace paratoric {

void IO::etc_sample(
    const Config& config
    ) {

    std::string folder_name_new;
    if (!config.out_spec.folder_name.empty()){
        folder_name_new = config.out_spec.folder_name;
    } else {
        folder_name_new = config.lat_spec.lattice_type + "_" + std::to_string(config.lat_spec.system_size) + "_" + config.lat_spec.boundaries + "_"
            + std::to_string(config.lat_spec.beta);
    }
    std::filesystem::path path_folder_name(folder_name_new);
    std::filesystem::path path_out = config.out_spec.path_out / path_folder_name;
    std::filesystem::create_directories(path_out);
    
    Result result_spec;
    std::vector<std::string> obs_types;
    if (config.lat_spec.basis == 'x') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'x'>>();
        result_spec = mc->get_sample(
            Config{config.sim_spec, config.param_spec, config.lat_spec, 
                OutSpec{.path_out=path_out, .save_snapshots=config.out_spec.save_snapshots, .full_time_series=config.out_spec.full_time_series}}
        ); 
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    } else if (config.lat_spec.basis == 'z') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'z'>>();
        result_spec = mc->get_sample(
            Config{config.sim_spec, config.param_spec, config.lat_spec, 
                OutSpec{.path_out=path_out, .save_snapshots=config.out_spec.save_snapshots, .full_time_series=config.out_spec.full_time_series}}
        ); 
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    }
    const auto& result = result_spec.series;
    const auto& obs_means = result_spec.mean;
    const auto& obs_std = result_spec.mean_std;
    const auto& binders_means = result_spec.binder;
    const auto& binders_std = result_spec.binder_std;
    const auto& autocorrelation_time = result_spec.tau_int;

    std::filesystem::path hdf5_name("obs.h5");
    std::filesystem::path hdf5_path_out = path_out / hdf5_name;

    H5::H5File file{ hdf5_path_out, H5F_ACC_TRUNC };

    H5::Group sim_grp = getOrCreateGroup(file, "simulation");
    H5::Group results_grp = getOrCreateGroup(sim_grp, "results");

    for (size_t i = 0; i < config.sim_spec.observables.size(); ++i) {
        const auto& obs_name = config.sim_spec.observables[i];
        const auto& obs_type = obs_types[i];
        const auto& obs_result_vector = result[i];

        H5::Group obs_grp = getOrCreateGroup(results_grp, obs_name);

        if (obs_type == "real" || obs_type == "fredenhagen_marcu" || obs_type == "susceptibility") {
            if (config.out_spec.full_time_series) {
                hsize_t dims[1] = { obs_result_vector.size() };
                H5::DataSpace dataspace{ 1, dims };

                H5::CompType complex_data_type(sizeof(obs_result_vector[0]));
                complex_data_type.insertMember( "r", 0, H5::PredType::NATIVE_DOUBLE);
                complex_data_type.insertMember( "i", sizeof(double), H5::PredType::NATIVE_DOUBLE);

                H5::DataSet dataset = obs_grp.createDataSet("series", complex_data_type, dataspace);

                dataset.write(obs_result_vector.data(), complex_data_type);
            }
        } else {
            throw std::runtime_error(std::format("Observable type \"{}\" is not supported.", obs_type));
        }

        H5::DataSpace scalar_space(H5S_SCALAR);
        auto ds_means = obs_grp.createDataSet("mean", H5::PredType::NATIVE_DOUBLE, scalar_space);
        ds_means.write(&(obs_means[i]), H5::PredType::NATIVE_DOUBLE);
        auto ds_mean_std = obs_grp.createDataSet("mean_error", H5::PredType::NATIVE_DOUBLE, scalar_space);
        ds_mean_std.write(&(obs_std[i]), H5::PredType::NATIVE_DOUBLE);
        auto ds_binder = obs_grp.createDataSet("binder", H5::PredType::NATIVE_DOUBLE, scalar_space);
        ds_binder.write(&(binders_means[i]), H5::PredType::NATIVE_DOUBLE);
        auto ds_binder_std = obs_grp.createDataSet("binder_error", H5::PredType::NATIVE_DOUBLE, scalar_space);
        ds_binder_std.write(&(binders_std[i]), H5::PredType::NATIVE_DOUBLE);
        auto ds_autocorrelation = obs_grp.createDataSet("autocorrelation_time", H5::PredType::NATIVE_DOUBLE, scalar_space);
        ds_autocorrelation.write(&(autocorrelation_time[i]), H5::PredType::NATIVE_DOUBLE);
    }

    const auto& diagnostics = result_spec.production_acceptance;
    H5::Group diagnostics_grp = getOrCreateGroup(sim_grp, "diagnostics");
    H5::Group acceptance_grp = getOrCreateGroup(diagnostics_grp, "production_acceptance");
    H5::DataSpace scalar_space(H5S_SCALAR);
    auto ds_attempted = acceptance_grp.createDataSet("attempted", H5::PredType::NATIVE_UINT64, scalar_space);
    ds_attempted.write(&(diagnostics.attempted), H5::PredType::NATIVE_UINT64);
    auto ds_accepted = acceptance_grp.createDataSet("accepted", H5::PredType::NATIVE_UINT64, scalar_space);
    ds_accepted.write(&(diagnostics.accepted), H5::PredType::NATIVE_UINT64);
    const double acceptance_fraction = diagnostics.attempted > 0
        ? static_cast<double>(diagnostics.accepted) / static_cast<double>(diagnostics.attempted)
        : 0.;
    const double mean_acceptance_ratio = diagnostics.attempted > 0
        ? diagnostics.acceptance_ratio_sum / static_cast<double>(diagnostics.attempted)
        : 0.;
    auto ds_acceptance_fraction = acceptance_grp.createDataSet("acceptance_fraction", H5::PredType::NATIVE_DOUBLE, scalar_space);
    ds_acceptance_fraction.write(&acceptance_fraction, H5::PredType::NATIVE_DOUBLE);
    auto ds_mean_acceptance_ratio = acceptance_grp.createDataSet("mean_acceptance_ratio", H5::PredType::NATIVE_DOUBLE, scalar_space);
    ds_mean_acceptance_ratio.write(&mean_acceptance_ratio, H5::PredType::NATIVE_DOUBLE);

    if (!diagnostics.attempted_by_update.empty()) {
        hsize_t dims[1] = { diagnostics.attempted_by_update.size() };
        H5::DataSpace dataspace{ 1, dims };
        acceptance_grp.createDataSet("attempted_by_update", H5::PredType::NATIVE_UINT64, dataspace)
            .write(diagnostics.attempted_by_update.data(), H5::PredType::NATIVE_UINT64);
        acceptance_grp.createDataSet("accepted_by_update", H5::PredType::NATIVE_UINT64, dataspace)
            .write(diagnostics.accepted_by_update.data(), H5::PredType::NATIVE_UINT64);

        std::vector<double> mean_ratio_by_update(diagnostics.attempted_by_update.size(), 0.);
        for (size_t i = 0; i < diagnostics.attempted_by_update.size(); ++i) {
            if (diagnostics.attempted_by_update[i] > 0) {
                mean_ratio_by_update[i] = diagnostics.acceptance_ratio_sum_by_update[i]
                    / static_cast<double>(diagnostics.attempted_by_update[i]);
            }
        }
        acceptance_grp.createDataSet("mean_acceptance_ratio_by_update", H5::PredType::NATIVE_DOUBLE, dataspace)
            .write(mean_ratio_by_update.data(), H5::PredType::NATIVE_DOUBLE);
    }

    if (!diagnostics.block_attempted.empty()) {
        hsize_t dims[1] = { diagnostics.block_attempted.size() };
        H5::DataSpace dataspace{ 1, dims };
        acceptance_grp.createDataSet("block_attempted", H5::PredType::NATIVE_UINT64, dataspace)
            .write(diagnostics.block_attempted.data(), H5::PredType::NATIVE_UINT64);
        acceptance_grp.createDataSet("block_accepted", H5::PredType::NATIVE_UINT64, dataspace)
            .write(diagnostics.block_accepted.data(), H5::PredType::NATIVE_UINT64);

        std::vector<double> block_acceptance_fraction(diagnostics.block_attempted.size(), 0.);
        std::vector<double> block_mean_acceptance_ratio(diagnostics.block_attempted.size(), 0.);
        for (size_t i = 0; i < diagnostics.block_attempted.size(); ++i) {
            if (diagnostics.block_attempted[i] > 0) {
                block_acceptance_fraction[i] = static_cast<double>(diagnostics.block_accepted[i])
                    / static_cast<double>(diagnostics.block_attempted[i]);
                block_mean_acceptance_ratio[i] = diagnostics.block_acceptance_ratio_sum[i]
                    / static_cast<double>(diagnostics.block_attempted[i]);
            }
        }
        acceptance_grp.createDataSet("block_acceptance_fraction", H5::PredType::NATIVE_DOUBLE, dataspace)
            .write(block_acceptance_fraction.data(), H5::PredType::NATIVE_DOUBLE);
        acceptance_grp.createDataSet("block_mean_acceptance_ratio", H5::PredType::NATIVE_DOUBLE, dataspace)
            .write(block_mean_acceptance_ratio.data(), H5::PredType::NATIVE_DOUBLE);
    }
}

void IO::etc_hysteresis(
    const Config& config
    ) {

    std::vector<std::filesystem::path> paths_out;

    for (auto& folder_name : config.out_spec.folder_names) {
        std::filesystem::path path_folder_name(folder_name);
        std::filesystem::path path_out = config.out_spec.path_out / path_folder_name;
        std::filesystem::create_directories(path_out);
        paths_out.emplace_back( std::move(path_out) );
    }

    Result result_spec;
    std::vector<std::string> obs_types;
    if (config.lat_spec.basis == 'x') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'x'>>();
        result_spec = mc->get_hysteresis(
            Config{config.sim_spec, config.param_spec, config.lat_spec, OutSpec{.paths_out=paths_out, .save_snapshots=config.out_spec.save_snapshots, .full_time_series=config.out_spec.full_time_series}}
        );
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    } else if (config.lat_spec.basis == 'z') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'z'>>();
        result_spec = mc->get_hysteresis(
            Config{config.sim_spec, config.param_spec, config.lat_spec, OutSpec{.paths_out=paths_out, .save_snapshots=config.out_spec.save_snapshots, .full_time_series=config.out_spec.full_time_series}}
        );
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    }

    const auto& results = result_spec.series_hys;
    const auto& hys_means = result_spec.mean_hys;
    const auto& hys_means_std = result_spec.mean_std_hys;
    const auto& hys_binders = result_spec.binder_hys;
    const auto& hys_binders_std = result_spec.binder_std_hys;
    const auto& hys_autocorrelation_time = result_spec.tau_int_hys;

    for (size_t n = 0; n < std::min( config.param_spec.h_hys.size(), std::min(config.param_spec.lmbda_hys.size(), paths_out.size()) ); n++) {
        const auto& path_out = paths_out[ n ];
        const auto& result = results[ n ];
        const auto& obs_means = hys_means[ n ];
        const auto& obs_std = hys_means_std[ n ];
        const auto& binders_means = hys_binders[ n ];
        const auto& binders_std = hys_binders_std[ n ];
        const auto& autocorrelation_time = hys_autocorrelation_time[ n ];

        std::filesystem::path hdf5_name("obs.h5");
        std::filesystem::path hdf5_path_out = path_out / hdf5_name;

        H5::H5File file{ hdf5_path_out, H5F_ACC_TRUNC };

        H5::Group sim_grp = getOrCreateGroup(file, "simulation");
        H5::Group results_grp = getOrCreateGroup(sim_grp, "results");

        for (size_t i = 0; i < config.sim_spec.observables.size(); ++i) {
            const auto& obs_name = config.sim_spec.observables[i];
            const auto& obs_type = obs_types[i];
            const auto& obs_result_vector = result[i];

            H5::Group obs_grp = getOrCreateGroup(results_grp, obs_name);

            if (obs_type == "real" || obs_type == "fredenhagen_marcu" || obs_type == "susceptibility") {
                if (config.out_spec.full_time_series) {
                    hsize_t dims[1] = { obs_result_vector.size() };
                    H5::DataSpace dataspace{ 1, dims };

                    H5::CompType complex_data_type(sizeof(obs_result_vector[0]));
                    complex_data_type.insertMember( "r", 0, H5::PredType::NATIVE_DOUBLE);
                    complex_data_type.insertMember( "i", sizeof(double), H5::PredType::NATIVE_DOUBLE);

                    H5::DataSet dataset = obs_grp.createDataSet("series", complex_data_type, dataspace);

                    dataset.write(obs_result_vector.data(), complex_data_type);
                }
            } else {
                throw std::runtime_error(std::format("Observable type \"{}\" is not supported.", obs_type));
            }

            H5::DataSpace scalar_space(H5S_SCALAR);
            auto ds_means = obs_grp.createDataSet("mean", H5::PredType::NATIVE_DOUBLE, scalar_space);
            ds_means.write(&(obs_means[i]), H5::PredType::NATIVE_DOUBLE);
            auto ds_mean_std = obs_grp.createDataSet("mean_error", H5::PredType::NATIVE_DOUBLE, scalar_space);
            ds_mean_std.write(&(obs_std[i]), H5::PredType::NATIVE_DOUBLE);
            auto ds_binder = obs_grp.createDataSet("binder", H5::PredType::NATIVE_DOUBLE, scalar_space);
            ds_binder.write(&(binders_means[i]), H5::PredType::NATIVE_DOUBLE);
            auto ds_binder_std = obs_grp.createDataSet("binder_error", H5::PredType::NATIVE_DOUBLE, scalar_space);
            ds_binder_std.write(&(binders_std[i]), H5::PredType::NATIVE_DOUBLE);
            auto ds_autocorrelation_time = obs_grp.createDataSet("autocorrelation_time", H5::PredType::NATIVE_DOUBLE, scalar_space);
            ds_autocorrelation_time.write(&(autocorrelation_time[i]), H5::PredType::NATIVE_DOUBLE);
        }
    }
}

void IO::etc_thermalization(
    const Config& config
    ) { 

    std::string folder_name_new;
    if (!config.out_spec.folder_name.empty()){
        folder_name_new = config.out_spec.folder_name;
    } else {
        folder_name_new = config.lat_spec.lattice_type + "_" + std::to_string(config.lat_spec.system_size) + "_" + config.lat_spec.boundaries + "_"
            + std::to_string(config.lat_spec.beta);
    }
    std::filesystem::path path_folder_name(folder_name_new);
    std::filesystem::path path_out = config.out_spec.path_out / path_folder_name;
    std::filesystem::create_directories(path_out);

    Result result_spec;
    std::vector<std::string> obs_types;
    if (config.lat_spec.basis == 'x') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'x'>>();
        result_spec = result_spec = mc->get_thermalization(
            Config{config.sim_spec, config.param_spec, config.lat_spec, OutSpec{.path_out=path_out, .save_snapshots=config.out_spec.save_snapshots}}
        );
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    } else if (config.lat_spec.basis == 'z') {
        auto mc = std::make_unique<ExtendedToricCodeQMC<'z'>>();
        result_spec = result_spec = mc->get_thermalization(
            Config{config.sim_spec, config.param_spec, config.lat_spec, OutSpec{.path_out=path_out, .save_snapshots=config.out_spec.save_snapshots}}
        );
        obs_types = mc->get_obs_type_vec(config.sim_spec.observables);
    }

    const auto& obs_result = result_spec.series;
    const auto& acc_ratio_result = result_spec.acc_ratio;

    std::filesystem::path hdf5_name("obs.h5");
    std::filesystem::path hdf5_path_out = path_out / hdf5_name;

    H5::H5File file{ hdf5_path_out, H5F_ACC_TRUNC };

    H5::Group sim_grp = getOrCreateGroup(file, "simulation");
    H5::Group results_grp = getOrCreateGroup(sim_grp, "results");

    for (size_t i = 0; i < config.sim_spec.observables.size(); ++i) {
        const auto& obs_name = config.sim_spec.observables[i];
        const auto& obs_type = obs_types[i];
        const auto& obs_result_vector = obs_result[i];

        H5::Group obs_grp = getOrCreateGroup(results_grp, obs_name);

        if (obs_type == "real" || obs_type == "fredenhagen_marcu" || obs_type == "susceptibility") {
            hsize_t dims[1] = { obs_result_vector.size() };
            H5::DataSpace dataspace{ 1, dims };

            H5::CompType complex_data_type(sizeof(obs_result_vector[0]));
            complex_data_type.insertMember( "r", 0, H5::PredType::NATIVE_DOUBLE);
            complex_data_type.insertMember( "i", sizeof(double), H5::PredType::NATIVE_DOUBLE);

            H5::DataSet dataset = obs_grp.createDataSet("series", complex_data_type, dataspace);

            dataset.write(obs_result_vector.data(), complex_data_type);
        } else {
            throw std::runtime_error(std::format("Observable type \"{}\" is not supported.", obs_type));
        }
    }


    hsize_t dims[1] = { acc_ratio_result.size() };
    H5::DataSpace dataspace{ 1, dims };

    auto dtype = H5::PredType::NATIVE_DOUBLE;

    H5::DataSet dataset = results_grp.createDataSet("acc_ratio", dtype, dataspace);

    dataset.write(acc_ratio_result.data(), dtype);
}

} // namespace paratoric
