// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2025  Simon Mathias Linsel, Lode Pollet

#pragma once

#include "lattice/lattice.hpp"
#include "mcmc/parallel_tempering.hpp"
#include "paratoric/types/types.hpp"
#include "rng/rng.hpp"
#include "statistics/autocorrelation.hpp"
#include "statistics/bootstrap.hpp"

#include <boost/log/core.hpp> 
#include <boost/log/expressions.hpp> 
#include <boost/log/trivial.hpp> 

#include <algorithm> 
#include <chrono>
#include <cmath>
#include <concepts>
#include <complex>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <span>
#include <tuple>
#include <variant>
#include <vector>

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace paratoric {

template<char B>
concept ValidBasis = (B == 'x' || B == 'z');

template<char Basis>
requires ValidBasis<Basis>
class ExtendedToricCodeQMC {
    public:
        using RNG = paratoric::rng::RNG;

        ExtendedToricCodeQMC(std::shared_ptr<RNG> rng = nullptr) 
        : rng(rng ? std::move(rng) : std::make_shared<RNG>()) {};

        ~ExtendedToricCodeQMC() = default;

        ExtendedToricCodeQMC(ExtendedToricCodeQMC const&) = default;
        ExtendedToricCodeQMC& operator=(ExtendedToricCodeQMC const&) = default;

        /**
         * @brief This method will run a QMC thermalization of the extended toric code with the specified parameters and return observables and acceptance ratio diagnostics.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h the Hamiltonian parameter (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda the Hamiltonian parameter (gauge field term)
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.path_out output directory for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every 10000th snapshot will be saved)
         * 
         * @return Result             the result object.
         * @return Result.series      Time series (per snapshot) of all requested observables,
         *                            measured during thermalization. Each observable’s entry
         *                            contains one value per recorded snapshot, in time order.
         * @return Result.acc_ratio   Time series of Monte Carlo acceptance ratios.
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_thermalization(
            const Config& config
        );

        /**
         * @brief This method will run a QMC simulation of the extended toric code with the specified parameters and return observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_samples the number of snapshots
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_between_samples the number of steps between snapshots
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.custom_therm if custom thermalization is used (to probe hysteresis)
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h the Hamiltonian parameter (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda the Hamiltonian parameter (gauge field term)
         * @param config.param_spec.h_therm the thermalization value of h, used if custom_therm enabled
         * @param config.param_spec.lmbda_therm the thermalization value of lmbda, used if custom_therm enabled
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.path_out output directory for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every snapshot will be saved)
         * 
         * @return Result             the result object.
         * @return Result.series      Full counting statistics of all requested observables in the input order,
         *                            Each observable’s entry contains one value per recorded snapshot, 
         *                            in time order.
         * @return Result.mean        Bootstrap observable means
         * @return Result.mean_std    Bootstrap standard errors of the mean
         * @return Result.binder      Bootstrap binder ratios
         * @return Result.binder_std  Bootstrap standard errors of the binder ratios
         * @return Result.tau_int     Estimated integrated autocorrelation times
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_samples > 0
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_sample(
            const Config& config
        );

        /**
         * @brief This method will run a QMC hysteresis simulation of the extended toric code with the specified parameters and return observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param config the configuration object
         * @param config.sim_spec.N_samples the number of snapshots
         * @param config.sim_spec.N_thermalization the number of thermalization steps
         * @param config.sim_spec.N_between_samples the number of steps between snapshots
         * @param config.sim_spec.N_resamples the number of bootstrap resamples
         * @param config.sim_spec.observables all observables that are calculated for every snapshot
         * @param config.sim_spec.seed the seed for the pseudorandom number generator
         * @param config.param_spec.mu the Hamiltonian parameter (star term)
         * @param config.param_spec.h_hys the Hamiltonian parameters (electric field term)
         * @param config.param_spec.J the Hamiltonian parameter (plaquette term)
         * @param config.param_spec.lmbda_hys the Hamiltonian parameters (gauge field term)
         * @param config.lat_spec.basis eigenbasis of the spins, either 'x' or 'z'
         * @param config.lat_spec.lattice_type the lattice type, e.g. "triangular"
         * @param config.lat_spec.system_size the system size of the lattice (in one dimension)
         * @param config.lat_spec.beta inverse temperature
         * @param config.lat_spec.boundaries the boundary condition of the lattice (periodic, open)
         * @param config.lat_spec.default_spin the default spin on the links (1 or -1)
         * @param config.out_spec.paths_out all output directories for snapshots 
         * @param config.out_spec.save_snapshots whether snapshots should be saved (every snapshot will be saved)
         * 
         * @return Result                 the result object.
         * @return Result.series_hys      Full counting statistics of all requested observables.
         *                                Each element in the outer vector represents one parameter point (h_hys, lmbda_hys)
         *                                Each element in the middle vectors represent one observables in the order of input.
         *                                The inner vector are the full counting statistics in time order.
         * @return Result.mean_hys        Bootstrap observable means. 
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.mean_std_hys    Bootstrap standard errors of the mean.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.binder_hys      Bootstrap binder ratios.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.binder_std_hys  Bootstrap standard errors of the binder ratios.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * @return Result.tau_int_hys     Estimated integrated autocorrelation times.
         *                                Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
         *                                Each element in the inner vectors represents one observables in the order of input.
         * 
         * @throws std::invalid_argument  If an input is inconsistent (e.g., beta <= 0,
         *                                unknown basis, unsupported lattice/boundary).
         * @throws std::runtime_error     On RNG initialization failure or I/O errors
         *                                when saving snapshots.
         * 
         * @pre config.sim_spec.N_samples > 0
         * @pre config.sim_spec.N_thermalization > 0
         * @pre config.lat_spec.beta > 0
         * @pre config.lat_spec.basis in {'x','z'}
         * 
         */
        Result get_hysteresis(
            const Config& config
        );
        
        /**
         * @brief Here we "assemble" the types of the observables. Most of the observables are probably reals.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param observables contains the names of the observables
         * 
         * @return vector of observable types in the order of occurance in observables.
         * 
         */
        std::vector<std::string> get_obs_type_vec(const std::vector<std::string>& observables);
    
    private:
        std::function<double(Lattice&, double, double, double, double)> 
        percolation_probability_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.percolation_probability(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        plaquette_percolation_probability_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.plaquette_percolation_probability(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        cube_percolation_probability 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.cube_percolation_probability(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        percolation_strength_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.percolation_strength(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        plaquette_percolation_strength_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.plaquette_percolation_strength(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        string_number_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.get_string_count(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        largest_cluster_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.largest_cluster(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        largest_plaquette_cluster_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.largest_plaquette_cluster(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        anyon_count_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.get_anyon_count(); 
        };

        std::function<std::complex<double>(Lattice&, double, double, double, double)> 
        fredenhagen_marcu_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            return lat.fredenhagen_marcu(); 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        staggered_imaginary_times_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_staggered_imaginary_times_plaquette(); 
            else return lat.get_staggered_imaginary_times_star();
        };

        std::function<double(Lattice&, double, double, double, double)> 
        energy_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return - lat.get_diag_single_energy() * h - lat.get_diag_tuple_energy_x() * mu 
                - lat.get_non_diag_single_energy_x() - lat.get_non_diag_tuple_energy_x();
            }
            else {
                return - lat.get_diag_single_energy() * lmbda - lat.get_diag_tuple_energy_z() * J 
                - lat.get_non_diag_single_energy_z() - lat.get_non_diag_tuple_energy_z();
            } 
        };

        std::function<double(Lattice&, double, double, double, double)> 
        energy_h_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_diag_single_energy() * h; 
            else return - lat.get_non_diag_single_energy_z();
        };

        std::function<double(Lattice&, double, double, double, double)> 
        energy_mu_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_diag_tuple_energy_x() * mu; 
            else return - lat.get_non_diag_tuple_energy_z();
        };

        std::function<double(Lattice&, double, double, double, double)> 
        energy_lmbda_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_non_diag_single_energy_x(); 
            else return - lat.get_diag_single_energy() * lmbda;
        };

        std::function<double(Lattice&, double, double, double, double)> 
        energy_J_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return - lat.get_non_diag_tuple_energy_x(); 
            else return - lat.get_diag_tuple_energy_z() * J;
        }; 

        std::function<double(Lattice&, double, double, double, double)> 
        sigma_x_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return lat.get_diag_single_energy()/static_cast<double>(lat.get_edge_count()); 
            } else {
                return lat.get_non_diag_single_energy_z()/static_cast<double>(lat.get_edge_count() * h);
            }
        };

        // Susceptibility estimators assume the caller rotates imaginary time at most once per measurement step.
        std::function<std::complex<double>(Lattice&, double, double, double, double)> 
        sigma_x_static_susceptibility_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_diag_M_M(); 
            else return lat.get_non_diag_M_M();
        };

        std::function<std::complex<double>(Lattice&, double, double, double, double)> 
        sigma_x_dynamical_susceptibility_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_diag_dynamical_M_M();
            else return lat.get_kL_kR_single() / static_cast<double>(std::sqrt(2) * h);
        };

        std::function<double(Lattice&, double, double, double, double)> 
        sigma_z_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_non_diag_single_energy_x()/static_cast<double>(lat.get_edge_count() * lmbda); 
            else 
                return (lat.get_diag_single_energy()/static_cast<double>(lat.get_edge_count()));
        };

        std::function<std::complex<double>(Lattice&, double, double, double, double)> 
        sigma_z_static_susceptibility_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') return lat.get_non_diag_M_M(); 
            else return lat.get_diag_M_M();
        };

        std::function<std::complex<double>(Lattice&, double, double, double, double)> 
        sigma_z_dynamical_susceptibility_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_kL_kR_single() / static_cast<double>(std::sqrt(2) * lmbda);
            else 
                return lat.get_diag_dynamical_M_M();
        };

        std::function<double(Lattice&, double, double, double, double)> 
        star_x_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_diag_tuple_energy_x()/static_cast<double>(lat.get_vertex_count()); 
            else 
                return lat.get_non_diag_tuple_energy_z()/static_cast<double>(lat.get_vertex_count() * mu);
        };

        std::function<double(Lattice&, double, double, double, double)> 
        plaquette_z_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') 
                return lat.get_non_diag_tuple_energy_x()/static_cast<double>(lat.get_plaquette_count() * J); 
            else 
                return lat.get_diag_tuple_energy_z()/static_cast<double>(lat.get_plaquette_count());
        };

        std::function<double(Lattice&, double, double, double, double)> 
        delta_obs = [](Lattice& lat, double h, double lmbda, double mu, double J) { 
            if constexpr (Basis == 'x') {
                return - lat.get_diag_tuple_energy_x()/static_cast<double>(lat.get_vertex_count()) 
                + lat.get_non_diag_tuple_energy_x()/static_cast<double>(lat.get_plaquette_count() * J); 
            } else {
                return - lat.get_non_diag_tuple_energy_z()/static_cast<double>(lat.get_vertex_count() * mu) 
                + lat.get_diag_tuple_energy_z()/static_cast<double>(lat.get_plaquette_count());
            }
        };

        std::function<double(Lattice&, double, double, double, double)> 
        anyon_density_obs 
        = [](Lattice& lat, double h, double lmbda, double mu, double J) {
            if constexpr (Basis == 'x') 
                return lat.get_anyon_count()/static_cast<double>(lat.get_vertex_count()); 
            else 
                return lat.get_anyon_count()/static_cast<double>(lat.get_plaquette_count()); 
        };

        // This structure contains the name, the type (e.g. real, i.e. just a double) and the function of an observable.
        struct Obs {
            public:
                std::string obs_name;
                std::string obs_type;
                std::function<
                    std::variant< std::complex<double>, double>(Lattice&, double, double, double, double)
                > obs_func;
        };

        // This vector contains the names of the observables (input as vector of strings: observables) and connects them to the appropriate function.  
        std::vector<Obs> obs_vec = {
            {"anyon_count", "real", anyon_count_obs},
            {"anyon_density", "real", anyon_density_obs},
            {"cube_percolation_probability", "real", cube_percolation_probability},
            {"delta", "real", delta_obs},
            {"energy", "real", energy_obs},
            {"energy_h", "real", energy_h_obs},
            {"energy_lmbda", "real", energy_lmbda_obs},
            {"energy_J", "real", energy_J_obs},
            {"energy_mu", "real", energy_mu_obs},
            {"fredenhagen_marcu", "fredenhagen_marcu", fredenhagen_marcu_obs},
            {"largest_cluster", "real", largest_cluster_obs},
            {"largest_plaquette_cluster", "real", largest_plaquette_cluster_obs},
            {"percolation_probability", "real", percolation_probability_obs},
            {"percolation_strength", "real", percolation_strength_obs},
            {"plaquette_percolation_probability", "real", plaquette_percolation_probability_obs},
            {"plaquette_percolation_strength", "real", plaquette_percolation_strength_obs},
            {"plaquette_z", "real", plaquette_z_obs},
            {"sigma_x", "real", sigma_x_obs},
            {"sigma_x_static_susceptibility", "susceptibility", sigma_x_static_susceptibility_obs},
            // TODO fix
            {"sigma_x_dynamical_susceptibility", "susceptibility", sigma_x_dynamical_susceptibility_obs},
            {"sigma_z", "real", sigma_z_obs},
            {"sigma_z_static_susceptibility", "susceptibility", sigma_z_static_susceptibility_obs},
            // TODO fix
            {"sigma_z_dynamical_susceptibility", "susceptibility", sigma_z_dynamical_susceptibility_obs},
            {"staggered_imaginary_times", "real", staggered_imaginary_times_obs},
            {"star_x", "real", star_x_obs},
            {"string_number", "real", string_number_obs}
            };
        
        /**
         * @brief Here we "assemble" the lambda functions for the observables. We use this vector to calculate the observables.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param observables contains the names of the observables
         * 
         * @return vector of lambda functions which can be applied to a lattice to extract the obervables.
         * 
         */
        std::vector<std::function<std::variant< std::complex<double>, double>(Lattice&, double, double, double, double)>>
        get_obs_func_vec(
            const std::vector<std::string>& observables
        );

        std::shared_ptr<RNG> rng;
        std::uniform_real_distribution<double> uniform_dist{0., 1.};
        std::uniform_int_distribution<int> mc_update_dist{0, 6};
        static constexpr double PRECISION = std::numeric_limits<double>::epsilon();

        /**
         * @brief Return the total potential energy (integrated over imaginary time) of the lattice lat.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         * @return the total integrated potential energy
         * 
         */
        static double total_integrated_pot_energy(Lattice& lat, double h, double mu, double J, double lmbda);

        /**
         * @brief Return the potential EDGE energy DIFFERENCE (integrated over imaginary time) when flipping the spin between imag_time_spin_flip and imag_time_next_spin_flip 
         * on the edge between vertices source_v and target_v on the lattice lat.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * @param source_v the source vertex of the edge of interest
         * @param target_v the target vertex of the edge of interest
         * @param imag_time_spin_flip the lower bound for imaginary time
         * @param imag_time_next_spin_flip the upper bound for imaginary time
         * @param total_cache              If true, use cached integrated energies
         *                                 and return @c -2*cache per affected tuple.
         *                                 This is a constant-time fast path intended
         *                                 for full-period flips (e.g., [0, β]).
         *                                 Do not set for partial intervals.
         * 
         * @return the integrated potential energy difference
         * 
         */
        static std::tuple<double, double> 
        integrated_pot_energy_diff_single_spin_flip_edge(
            Lattice& lat, double h, double mu, double J, double lmbda, 
            const Lattice::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache
        );

        /**
         * @brief Integrated tuple–energy difference for a single spin flip on an edge.
         *
         * Computes the tuple (star/plaquette) energy change, integrated over the
         * imaginary-time interval [@p imag_time_spin_flip, @p imag_time_next_spin_flip],
         * when the spin on @p edg is flipped across that interval. The affected tuples
         * are:
         *  - Basis 'x': the two STARS at the endpoints of @p edg.
         *  - Basis 'z': the PLAQUETTES adjacent to @p edg.
         *
         * Returns the per-tuple **bare** energy differences and the **coupled** scalar
         * sum used for acceptance. Couplings are applied only to the scalar:
         *  - Basis 'x': @c delta = -mu * sum(bare_diffs)
         *  - Basis 'z': @c delta = -J  * sum(bare_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                      Lattice object.
         * @param h                        Electric-field coupling (unused here).
         * @param mu                       Star coupling (used if Basis=='x').
         * @param J                        Plaquette coupling (used if Basis=='z').
         * @param lmbda                    Gauge-field coupling (unused here).
         * @param edg                      Edge whose spin is flipped.
         * @param imag_time_spin_flip      Lower bound of imaginary time (inclusive).
         * @param imag_time_next_spin_flip Upper bound of imaginary time (inclusive).
         * @param total_cache              If true, use cached integrated tuple energies
         *                                 and return @c -2*cache per affected tuple.
         *                                 This is a constant-time fast path intended
         *                                 for full-period flips (e.g., [0, β]).
         *                                 Do not set for partial intervals.
         *
         * @return std::tuple<
         *           double,                 // coupled scalar delta (see above)
         *           std::vector<int>,       // tuple indices: star centers (Basis 'x')
         *                                   // or plaquette indices (Basis 'z')
         *           std::vector<double>     // per-tuple **bare** energy differences on [t1,t2],
         *                                   // aligned with the index vector
         *         >
         *
         * @pre @p imag_time_next_spin_flip > @p imag_time_spin_flip. Interval must be non-zero.
         * @note Per-tuple values are bare; apply couplings only when forming totals or acceptance ratios.
         *       The order of indices matches the lattice adjacency.
         */
        static std::tuple<double, std::vector<int>, std::vector<double>> 
        integrated_pot_energy_diff_single_spin_flip_tuple(
            Lattice& lat, double h, double mu, double J, double lmbda, 
            const Lattice::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache
        );

        /**
         * @brief Integrated EDGE–energy difference for flipping all spins of a tuple.
         *
         * Computes the (bare) edge–energy change for each edge in @p tuple_edges,
         * integrated over [@p imag_time_spin_flip, @p imag_time_next_spin_flip],
         * when the spins on those edges are flipped across that interval. Returns the
         * per-edge **bare** differences and the **coupled** scalar sum used for
         * acceptance. Couplings are applied only to the scalar:
         *  - Basis 'x': @c delta = -h * sum(bare_edge_diffs)
         *  - Basis 'z': @c delta = -lmbda * sum(bare_edge_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                       Lattice object.
         * @param h                         Electric-field coupling (used if Basis=='x').
         * @param mu                        Star coupling (unused here).
         * @param J                         Plaquette coupling (unused here).
         * @param lmbda                     Gauge-field coupling (used if Basis=='z').
         * @param tuple_index               Index of the tuple (for bookkeeping).
         * @param tuple_edges               Edges belonging to the tuple; order is preserved.
         * @param imag_time_spin_flip       Lower bound of imaginary time (inclusive).
         * @param imag_time_next_spin_flip  Upper bound of imaginary time (inclusive).
         * @param total_cache               If true, use cached integrated edge energies and
         *                                  return @c -2 * cache for each edge. This is a
         *                                  constant-time fast path intended for full-period
         *                                  flips (e.g., [0, β]). Do not set for partial intervals.
         *
         * @return std::tuple<
         *           double,                         // coupled scalar delta (see above)
         *           std::span<Lattice::Edge>,     // echo of @p tuple_edges (same order)
         *           std::vector<double>             // per-edge **bare** energy differences on [t1,t2],
         *                                           // aligned with the returned edge vector
         *         >
         *
         * @pre @p imag_time_next_spin_flip > @p imag_time_spin_flip (non-zero interval).
         * @note Per-edge values are bare; apply couplings only when forming totals or acceptance ratios.
         * @note The second return component mirrors the input edges by value; consider
         *       using a view/reference in hot paths to avoid copies.
         */
        static std::tuple<double, std::span<const Lattice::Edge>, std::vector<double>> 
        integrated_pot_energy_diff_tuple_flip_edge(
            Lattice& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
            double imag_time_spin_flip, double imag_time_next_spin_flip, 
            bool total_cache
        );

        /**
         * @brief Integrated EDGE–energy difference for a “combination” tuple update.
         *
         * Computes, for each edge in @p tuple_edges, the (bare) edge–energy change
         * integrated over [@p tau_left, @p tau_right] when a tuple flip occurs at
         * @p imag_time_tuple_flip and a single–spin flip on that same edge occurs at
         * @p imag_time_spin_flips[i]. The per–edge bare differences are summed into a
         * **coupled** scalar used for acceptance:
         *  - Basis 'x': @c delta = -h      * sum(bare_edge_diffs)
         *  - Basis 'z': @c delta = -lmbda  * sum(bare_edge_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                   Lattice object.
         * @param h                     Electric–field coupling (used if Basis=='x').
         * @param mu                    Star coupling (unused here).
         * @param J                     Plaquette coupling (unused here).
         * @param lmbda                 Gauge–field coupling (used if Basis=='z').
         * @param tuple_index           Index of the tuple (for bookkeeping).
         * @param tuple_edges           Edges belonging to the tuple; order is preserved.
         * @param imag_time_tuple_flip  Imaginary time of the tuple flip.
         * @param imag_time_spin_flips  Per–edge single–flip times aligned with @p tuple_edges;
         *                              @c imag_time_spin_flips[i] is the time on @c tuple_edges[i].
         * @param tau_left              Lower bound of the integration interval (inclusive).
         * @param tau_right             Upper bound of the integration interval (inclusive).
         * @param create_vector         For each edge, whether a single flip is created (true)
         *                              or removed (false). (Currently not used in the integral.)
         * @param tuple_destroy         Whether the tuple flip is destroyed (true) or created (false).
         *                              (Currently not used in the integral.)
         *
         * @return std::tuple<
         *           double,                         // coupled scalar delta (see above)
         *           std::span<Lattice::Edge>,     // echo of @p tuple_edges (same order)
         *           std::vector<double>             // per–edge **bare** energy differences on [tau_left,tau_right],
         *                                           // aligned with the returned edge vector
         *         >
         *
         * @pre @p tau_right > @p tau_left.  Interval must be non-zero.
         * @pre @p imag_time_spin_flips.size() == @p tuple_edges.size().
         *
         * @note Per–edge values are **bare**; couplings are applied only to the scalar sum.
         * @note The local schedule per edge is {(@p imag_time_tuple_flip, tuple), (@p imag_time_spin_flips[i], single)}.
         *       Equal–time flips are combined by parity; order does not affect the integral.
         * @note The parameters @p create_vector and @p tuple_destroy are accepted for interface
         *       symmetry but are not currently used in the computation.
         */
        static std::tuple<double, std::span<const Lattice::Edge>, std::vector<double>> 
        integrated_pot_energy_diff_combination_flip_edge(
            Lattice& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
            double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
            double tau_left, double tau_right, 
            const std::vector<bool>& create_vector, bool tuple_destroy
        );

        /**
         * @brief Integrated TUPLE–energy difference for a “combination” update.
         *
         * Computes the (bare) tuple energy change integrated over
         * [@p tau_left, @p tau_right] when a tuple flip occurs at
         * @p imag_time_tuple_flip and, on the same tuple, single–spin flips occur on
         * its edges at the times in @p imag_time_spin_flips. The affected tuples are:
         *  - Basis 'x' → STARS at the endpoints of the tuple edges.
         *  - Basis 'z' → PLAQUETTES adjacent to the tuple edges.
         *
         * Returns the per-tuple **bare** energy differences (aligned with the returned
         * index vector) and the **coupled** scalar sum used for acceptance:
         *  - Basis 'x': @c delta = -mu * sum(bare_tuple_diffs)
         *  - Basis 'z': @c delta = -J  * sum(bare_tuple_diffs)
         *
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         *
         * @param lat                   Lattice object.
         * @param h                     Electric-field coupling (unused here).
         * @param mu                    Star coupling (used if Basis=='x').
         * @param J                     Plaquette coupling (used if Basis=='z').
         * @param lmbda                 Gauge-field coupling (unused here).
         * @param tuple_index           Index of the tuple (bookkeeping).
         * @param tuple_edges           Edges that form the tuple; order is preserved.
         * @param imag_time_tuple_flip  Imaginary time of the tuple flip event.
         * @param imag_time_spin_flips  Per-edge single-flip times aligned with
         *                              @p tuple_edges; @c imag_time_spin_flips[i] is
         *                              the time on @c tuple_edges[i].
         * @param tau_left              Lower bound of the integration interval (inclusive).
         * @param tau_right             Upper bound of the integration interval (inclusive).
         * @param create_vector         For each edge, whether a single flip is created (true)
         *                              or removed (false). (Accepted for symmetry; not used here.)
         * @param tuple_destroy         Whether the tuple flip is destroyed (true) or created (false).
         *                              (Accepted for symmetry; not used here.)
         *
         * @return std::tuple<
         *           double,                 // coupled scalar delta (see above)
         *           std::vector<int>,       // tuple indices: star centers (Basis 'x')
         *                                   // or plaquette indices (Basis 'z')
         *           std::vector<double>     // per-tuple **bare** energy differences on [tau_left,tau_right],
         *                                   // aligned with the index vector
         *         >
         *
         * @pre @p tau_right > @p tau_left (non-zero interval).
         * @pre @p imag_time_spin_flips.size() == @p tuple_edges.size().
         *
         * @note “Bare” means couplings are NOT applied to the per-tuple values.
         *       Couplings are applied only to the returned scalar @c delta.
         * @note Equal-time flips are combined by parity; their order does not affect the integral.
         */
        static std::tuple<double, std::vector<int>, std::vector<double>> 
        integrated_pot_energy_diff_combination_flip_tuple(
            Lattice& lat, double h, double mu, double J, double lmbda, 
            int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
            double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
            double tau_left, double tau_right, 
            const std::vector<bool>& create_vector, bool tuple_destroy
        );

        /**
         * @brief Perform tuple combination update, where both tuple and 
         * single spin flips are created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param tuple_edges the vector which stores the edges of the tuple
         * @param imag_time_tuple_flip the imaginary time of the tuple flip that is created or destroyed
         * @param imag_time_spin_flips vector which contains the imaginary times of the single spin flips 
         * that are created or destroyed.
         * @param create_vector vector which contains the information if single spin flip is added (1) 
         * or removed (0) on the tuple edges. Order of edges is identical to imag_time_spin_flips.
         * @param tuple_destroy if the tuple is destroyed (1) or created (0)
         * 
         */
        static void combination_flip(
            Lattice& lat, double h, double mu, 
            int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
            double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
            const std::vector<bool>& create_vector, bool tuple_destroy
        );

        /**
         * @brief Metropolis-Hastings update where a pair of single spin flips is created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_double_single_spin_flip(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a single spin flip is moved in imaginary time.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_single_spin_flip_move(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where the spin is flipped globally on a random edge.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_global_single_spin_flip(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );


        /**
         * @brief Metropolis-Hastings update where the spin is flipped globally on a random tuple.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_global_tuple_flip(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a pair of tuple flips is created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_double_tuple_flip(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a tuple flip is moved in imaginary time.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_single_tuple_flip_move(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings update where a tuple flip and single spin flips on the same tuple are created/destroyed.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step_spin_tuple_combination(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        /**
         * @brief Metropolis-Hastings helper function that distributes updates randomly.
         * 
         * @tparam Basis eigenbasis of the spins, either 'x' or 'z'
         * @param lat the lattice object
         * @param integrated_pot_energy the integrated potential energy of the system. Used for diagnostics.
         * @param acc_ratio the update acceptance ratio. If update is abandoned before calculating the acceptance probability, 
         * it is set to 0. Used for diagnostics
         * @param beta the inverse temperature 
         * @param h the Hamiltonian parameter (electric field term)
         * @param mu the Hamiltonian parameter (star term)
         * @param J the Hamiltonian parameter (plaquette term)
         * @param lmbda the Hamiltonian parameter (gauge field term)
         * 
         */
        void metropolis_step(
            Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
            double h, double mu, double J, double lmbda
        );

        template<typename T>
        requires std::integral<T> || std::floating_point<T>
        constexpr T modulo(T a, T b) {
            if constexpr (std::integral<T>) {
                T result = a % b;
                return result >= 0 ? result : result + b;
            } else {
                T result = std::fmod(a, b);
                return result >= 0 ? result : result + b;
            }
        }

        template<std::floating_point T>
        constexpr bool almost_equal(
            T a, T b, 
            T rel_tol = std::numeric_limits<T>::epsilon(), 
            T abs_tol = std::numeric_limits<T>::min()
        ) {
            T diff = std::abs(a - b);

            if (diff <= abs_tol) return true;

            T largest = std::max(std::abs(a), std::abs(b));
            return diff <= largest * rel_tol;
        }
};

template<char Basis>
requires ValidBasis<Basis>
std::vector<std::function<std::variant< std::complex<double>, double>(Lattice&, double, double, double, double)>> 
ExtendedToricCodeQMC<Basis>::get_obs_func_vec(const std::vector<std::string>& observables) {
    std::vector<std::function<std::variant< std::complex<double>, double>(Lattice&, double, double, double, double)>> result;
    for (const auto& obs_name : observables) {
        bool obs_found = false;
        for (const auto& obs : obs_vec) {
            if (obs.obs_name == obs_name) {
                result.emplace_back( obs.obs_func );
                obs_found = true;
                break;
            } 
        }
        if (!obs_found) {
            throw std::invalid_argument(std::string("The observable \"") + obs_name + std::string("\" does not exist!"));
        }
    }
    return result;
}

template<char Basis>
requires ValidBasis<Basis>
std::vector<std::string> 
ExtendedToricCodeQMC<Basis>::get_obs_type_vec(const std::vector<std::string>& observables) {
    std::vector<std::string> result;
    for (const auto& obs_name : observables) {
        bool obs_found = false;
        for (const auto& obs : obs_vec) {
            if (obs.obs_name == obs_name) {
                result.emplace_back( obs.obs_type );
                obs_found = true;
                break;
            } 
        }
        if (!obs_found) {
            throw std::invalid_argument(std::string("The observable \"") + obs_name + std::string("\" does not exist!"));
        }
    }
    return result;
}

template<char Basis>
requires ValidBasis<Basis>
double ExtendedToricCodeQMC<Basis>::total_integrated_pot_energy(
    Lattice& lat, double h, double mu, double J, double lmbda
) {
    double integrated_potential_energy = 0.;
    if constexpr (Basis == 'x') {
        integrated_potential_energy = - lat.total_integrated_edge_energy() * h - lat.total_integrated_star_energy() * mu;
    } else if constexpr (Basis == 'z') {
        integrated_potential_energy = - lat.total_integrated_edge_energy() * lmbda - lat.total_integrated_plaquette_energy() * J;
    }
    return integrated_potential_energy;
}

template<char Basis>
requires ValidBasis<Basis>
std::tuple<double, double> 
ExtendedToricCodeQMC<Basis>::integrated_pot_energy_diff_single_spin_flip_edge(
    Lattice& lat, double h, double mu, double J, double lmbda, 
    const Lattice::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache
) {
    double delta_energy_single = 0.;
    double bare_energy_single = 0.;
    if (total_cache) {
        bare_energy_single = -2*lat.get_potential_edge_energy(edg);
    } else {
        bare_energy_single = lat.integrated_edge_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip);
    }

    if constexpr (Basis == 'x') {
        delta_energy_single = -h * bare_energy_single;
    } else if constexpr (Basis == 'z') {
        delta_energy_single = -lmbda * bare_energy_single;
    }

    return {delta_energy_single, bare_energy_single};
}

template<char Basis>
requires ValidBasis<Basis>
std::tuple<double, std::vector<int>, std::vector<double>> 
ExtendedToricCodeQMC<Basis>::integrated_pot_energy_diff_single_spin_flip_tuple(
    Lattice& lat, double h, double mu, double J, double lmbda, 
    const Lattice::Edge& edg, double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache
) {
    double delta_energy_tuple = 0.;
    if constexpr (Basis == 'x') {
        const auto& [bare_energy, star_centers, bare_star_potential_energy_diffs] 
        = lat.integrated_star_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip, total_cache);
        delta_energy_tuple = -mu * bare_energy; // No "/ 2"!
        //for (double& x : bare_star_potential_energy_diffs) x *= -mu;
        return {delta_energy_tuple, std::move(star_centers), std::move(bare_star_potential_energy_diffs)};
    } else if constexpr (Basis == 'z') {
        const auto& [bare_energy, plaquette_indices, bare_plaquette_potential_energy_diffs] 
        = lat.integrated_plaquette_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip, total_cache);
        delta_energy_tuple = -J * bare_energy; // No "/ 2"!
        //for (double& x : bare_plaquette_potential_energy_diffs) x *= -J;
        return {delta_energy_tuple, std::move(plaquette_indices), std::move(bare_plaquette_potential_energy_diffs)};
    } 
    return {0., std::vector<int>{}, std::vector<double>{}};
}

template<char Basis>
requires ValidBasis<Basis>
std::tuple<double, std::span<const Lattice::Edge>, std::vector<double>> 
ExtendedToricCodeQMC<Basis>::integrated_pot_energy_diff_tuple_flip_edge(
    Lattice& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
    double imag_time_spin_flip, double imag_time_next_spin_flip, 
    bool total_cache
) {
    UNUSED(mu);
    UNUSED(J);
    double delta_energy_single = 0.;
    std::vector<double> bare_energy_single_vector;
    double bare_energy_edg = 0.;
    for (const Lattice::Edge& edg : tuple_edges) {
        if (total_cache) { 
            bare_energy_edg = -2*lat.get_potential_edge_energy(edg);
        } else {
            bare_energy_edg = lat.integrated_edge_energy_diff(edg, imag_time_spin_flip, imag_time_next_spin_flip);
        }
        bare_energy_single_vector.emplace_back(bare_energy_edg);

        if constexpr (Basis == 'x') {
            delta_energy_single += -h * bare_energy_edg;
        } else if constexpr (Basis == 'z') {
            delta_energy_single += -lmbda * bare_energy_edg;
        }
    }
    return {delta_energy_single, tuple_edges, std::move(bare_energy_single_vector)};
}

template<char Basis>
requires ValidBasis<Basis>
std::tuple<double, std::span<const Lattice::Edge>, std::vector<double>> 
ExtendedToricCodeQMC<Basis>::integrated_pot_energy_diff_combination_flip_edge(
    Lattice& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
    double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
    double tau_left, double tau_right, 
    const std::vector<bool>& create_vector, bool tuple_destroy
) {
    double energy_single_diff = 0.; 
    std::vector<double> bare_energy_single_vector;
    for (size_t i = 0; i < tuple_edges.size(); ++i) {
        const auto edg = tuple_edges[i];

        double bare_energy_edg = lat.integrated_edge_energy_diff_combination(
            edg,
            tau_left,
            tau_right,
            imag_time_tuple_flip,
            imag_time_spin_flips[i]
        );
        bare_energy_single_vector.emplace_back(bare_energy_edg);
        if constexpr (Basis == 'x') {
            energy_single_diff += -h * bare_energy_edg;
        }  else if constexpr (Basis == 'z') {
            energy_single_diff += -lmbda * bare_energy_edg;
        }
    }

    return {energy_single_diff, tuple_edges, std::move(bare_energy_single_vector)};
}

template<char Basis>
requires ValidBasis<Basis>
std::tuple<double, std::vector<int>, std::vector<double>> 
ExtendedToricCodeQMC<Basis>::integrated_pot_energy_diff_combination_flip_tuple(
    Lattice& lat, double h, double mu, double J, double lmbda, 
    int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
    double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
    double tau_left, double tau_right, 
    const std::vector<bool>& create_vector, bool tuple_destroy
) {
    double energy_tuple_diff = 0.; 
    if constexpr (Basis == 'x') {
        const auto& [bare_energy, star_centers, bare_star_potential_energy_diffs] 
        = lat.integrated_star_energy_diff_combination(
            tuple_index, tau_left, tau_right, imag_time_spin_flips, imag_time_tuple_flip
        );
        energy_tuple_diff += -mu * bare_energy;
        return {energy_tuple_diff, std::move(star_centers), std::move(bare_star_potential_energy_diffs)};
    }  else if constexpr (Basis == 'z') {
        const auto& [bare_energy, plaquette_indices, bare_plaquette_potential_energy_diffs] = 
        lat.integrated_plaquette_energy_diff_combination(
            tuple_index, tau_left, tau_right, imag_time_spin_flips, imag_time_tuple_flip
        );
        energy_tuple_diff += -J * bare_energy;
        return {energy_tuple_diff, std::move(plaquette_indices), std::move(bare_plaquette_potential_energy_diffs)};
    }
    return {0., std::vector<int>{}, std::vector<double>{}};
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::combination_flip(
    Lattice& lat, double h, double mu, 
    int tuple_index, std::span<const Lattice::Edge> tuple_edges, 
    double imag_time_tuple_flip, const std::vector<double>& imag_time_spin_flips, 
    const std::vector<bool>& create_vector, bool tuple_destroy
) {
    UNUSED(h);
    UNUSED(mu);
    if (tuple_destroy) {
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            const auto edg = tuple_edges[i];

            if (create_vector[i]) {
                lat.insert_single_spin_flip(edg, imag_time_spin_flips[i]);
            } else {
                lat.delete_single_spin_flip(edg, lat.get_spin_flip_index(edg, imag_time_spin_flips[i]));
            }
        }
        lat.delete_tuple_flip(tuple_index, tuple_edges, imag_time_tuple_flip);
    } else {
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            const auto edg = tuple_edges[i];

            if (create_vector[i]) {
                lat.insert_single_spin_flip(edg, imag_time_spin_flips[i]);
            } else {
                lat.delete_single_spin_flip(edg, lat.get_spin_flip_index(edg, imag_time_spin_flips[i]));
            }
        }
        lat.insert_tuple_flip(tuple_index, tuple_edges, imag_time_tuple_flip);
    } 
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_double_single_spin_flip(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_double_single_spin_flip!";
#endif

    if (Basis == 'x' && (lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    const double rnd_create_destroy = uniform_dist(*rng);

    const auto& [rand_edge, source, target] = lat.get_random_edge();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Randomly chose edge between vertices {} and {}.", source, target);
#endif
    
    std::span<const double> single_spin_flips = lat.get_single_spin_flips(rand_edge);
    int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Found {} single spin flips.", single_spin_flip_count);
#endif

    if (rnd_create_destroy < 0.5) { // destroys a pair of single spin flips
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Trying to DESTROY single spin flip pair.";
#endif
        if (single_spin_flip_count > 1) [[likely]] {
            std::uniform_int_distribution<int> random_spin_flip_dist(0, single_spin_flip_count - 2);

            const int random_spin_flip_index = random_spin_flip_dist(*rng);

            const double imag_time_spin_flip = single_spin_flips[random_spin_flip_index];

            const double imag_time_next_spin_flip = single_spin_flips[random_spin_flip_index+1];

            double imag_time_next_next_spin_flip = beta;

            if (random_spin_flip_index != single_spin_flip_count - 2) [[likely]] {
                imag_time_next_next_spin_flip = single_spin_flips[random_spin_flip_index+2];
            }

            double imag_time_prev_spin_flip = 0.;

            if (random_spin_flip_index != 0) [[likely]] {
                imag_time_prev_spin_flip = single_spin_flips[random_spin_flip_index-1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- Imaginary time of random single spin flip: {}; previous: {}; next: {}; next next: {}", 
                imag_time_spin_flip, imag_time_prev_spin_flip, imag_time_next_spin_flip, imag_time_next_next_spin_flip);
#endif            
            const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
            = integrated_pot_energy_diff_single_spin_flip_edge(
                lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, imag_time_next_spin_flip, false
            );
            const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
            = integrated_pot_energy_diff_single_spin_flip_tuple(
                lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, imag_time_next_spin_flip, false
            );
            const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
            if constexpr (Basis == 'x') {
                acc_ratio = 2./(lmbda * lmbda * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip) 
                * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip)) * std::exp(-integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = 2./(h * h * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip) 
                * (imag_time_next_next_spin_flip - imag_time_prev_spin_flip)) * std::exp(-integrated_pot_energy_diff);
            }
            

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif   
            
            if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- ACCEPTED.";
#endif   
                lat.delete_double_single_spin_flip(rand_edge, imag_time_spin_flip, imag_time_next_spin_flip);
                integrated_pot_energy += integrated_pot_energy_diff;
                lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                    int tuple_index = pot_energy_tuple_indices[i];
                    double potential_tuple_energy_diff = pot_energy_diffs[i];
                    if constexpr (Basis == 'x') {
                        lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                    } else if constexpr (Basis == 'z') {
                        lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                    }
                }
            } 
        } else [[unlikely]] {  
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.;
        }
    } else { // create a pair of single spin flips
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Trying to CREATE single spin flip pair.";
#endif
        double tau_left, tau_right;
        int random_spin_flip_index = 0;

        if (single_spin_flip_count == 0) {
            tau_left = 0.;
            tau_right = beta;
        } else {
            std::uniform_int_distribution<int> random_spin_flip_dist(0, single_spin_flip_count);
            random_spin_flip_index = random_spin_flip_dist(*rng);

            if (random_spin_flip_index == 0) [[unlikely]] {
                tau_left = 0.; // not really necessary
                tau_right = single_spin_flips[0];
            } else if (random_spin_flip_index == single_spin_flip_count) [[unlikely]] {
                tau_left = single_spin_flips[random_spin_flip_index-1];
                tau_right = beta;
            } else [[likely]] {
                tau_left = single_spin_flips[random_spin_flip_index-1];
                tau_right = single_spin_flips[random_spin_flip_index];
            }
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif

        double tau_1 = 0., tau_2 = 0.;

        std::uniform_real_distribution<double> new_times_dist(tau_left, tau_right);
        tau_1 = new_times_dist(*rng);
        tau_2 = new_times_dist(*rng);

        if (tau_2 < tau_1) {
            std::swap(tau_1, tau_2);
        }

        if (std::abs(tau_2 - tau_1) < PRECISION 
        || std::abs(tau_1 - tau_left) < PRECISION 
        || std::abs(tau_2 - tau_right) < PRECISION
        ) [[unlikely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- Random numbers very close to each other.";
#endif
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        } else [[likely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- tau_1: {}; tau_2: {}", tau_1, tau_2);
#endif
            const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
            = integrated_pot_energy_diff_single_spin_flip_edge(lat, h, mu, J, lmbda, rand_edge, tau_1, tau_2, false);
            const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
            = integrated_pot_energy_diff_single_spin_flip_tuple(lat, h, mu, J, lmbda, rand_edge, tau_1, tau_2, false);
            const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
            if constexpr (Basis == 'x') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * lmbda * lmbda * std::exp(-integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * h * h * std::exp(-integrated_pot_energy_diff);
            }
            

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif   

            if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_single_spin_flip --- ACCEPTED.";
#endif  
                lat.insert_double_single_spin_flip(rand_edge, tau_1, tau_2);
                integrated_pot_energy += integrated_pot_energy_diff;
                lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                    int tuple_index = pot_energy_tuple_indices[i];
                    double potential_tuple_energy_diff = pot_energy_diffs[i];
                    if constexpr (Basis == 'x') {
                        lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                    } else if constexpr (Basis == 'z') {
                        lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                    }
                }
            } 
        }  
    }
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_single_spin_flip_move(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_single_spin_flip_move!";
#endif

    if (Basis == 'x' && (lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    const auto& [rand_edge, source, target] = lat.get_random_edge();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Randomly chose edge between vertices {} and {}.", source, target);    
#endif

    std::span<const double> single_spin_flips = lat.get_single_spin_flips(rand_edge);
    int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Found {} single spin flips.", single_spin_flip_count);  
#endif    

    if (single_spin_flip_count != 0) [[likely]] {
        std::uniform_int_distribution<int> random_spin_flip_dist(0, single_spin_flip_count - 1);

        const int random_spin_flip_index = random_spin_flip_dist(*rng);

        const double imag_time_spin_flip = single_spin_flips[random_spin_flip_index];

        const double imag_time_next_spin_flip = lat.flip_next_imag_time(rand_edge, imag_time_spin_flip);

        const double imag_time_prev_spin_flip = lat.flip_prev_imag_time(rand_edge, imag_time_spin_flip);

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- Imaginary time of random single spin flip: {}; previous: {}; next: {}", imag_time_spin_flip, imag_time_prev_spin_flip, imag_time_next_spin_flip);
#endif  

        const int random_spin_flip_index_lat = lat.get_spin_flip_index(rand_edge, imag_time_spin_flip);

        if (imag_time_prev_spin_flip < imag_time_spin_flip && imag_time_spin_flip < imag_time_next_spin_flip) {
            std::uniform_real_distribution<double> new_time_dist(imag_time_prev_spin_flip, imag_time_next_spin_flip);
            const double new_imag_time = new_time_dist(*rng);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- New imaginary time: {}", new_imag_time);
#endif  

            if (std::abs(new_imag_time - imag_time_spin_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                double integrated_pot_energy_diff_edge = 0.;
                double bare_pot_energy_diff_edge = 0.;
                //double integrated_pot_energy_diff_tuple = 0.;
                std::vector<int> pot_energy_tuple_indices;
                std::vector<double> pot_energy_diffs;
                if (new_imag_time > imag_time_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    pot_energy_tuple_indices = std::move(pot_energy_tuple_indices_tmp);
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
                } else {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    pot_energy_tuple_indices = std::move(pot_energy_tuple_indices_tmp);
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;
                }
                acc_ratio = std::exp(-integrated_pot_energy_diff);

#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif              
                    lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                    integrated_pot_energy += integrated_pot_energy_diff;
                    lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                    for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                        int tuple_index = pot_energy_tuple_indices[i];
                        double potential_tuple_energy_diff = pot_energy_diffs[i];
                        if constexpr (Basis == 'x') {
                            lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                        } else if constexpr (Basis == 'z') {
                            lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                        }
                    }
                } 

            }
        } else { // Potentially cross beta
            std::uniform_real_distribution<double> new_time_dist(imag_time_prev_spin_flip, beta + imag_time_next_spin_flip);
            const double new_imag_time = modulo(new_time_dist(*rng), beta);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- New imaginary time: {}", new_imag_time);
#endif  
            
            if (std::abs(new_imag_time - imag_time_spin_flip) < PRECISION || std::abs(new_imag_time - imag_time_prev_spin_flip) < PRECISION || std::abs(new_imag_time - imag_time_next_spin_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                double integrated_pot_energy_diff_edge = 0.;
                double bare_pot_energy_diff_edge = 0.;
                //double integrated_pot_energy_diff_tuple = 0.;
                //std::vector<int> pot_energy_tuple_indices;
                //std::vector<double> pot_energy_diffs;

                if (random_spin_flip_index_lat == 0 && new_imag_time < imag_time_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(-integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 && imag_time_spin_flip < new_imag_time) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat == 0 && imag_time_spin_flip < new_imag_time && new_imag_time < imag_time_next_spin_flip) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, new_imag_time, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 
                && imag_time_spin_flip > new_imag_time && new_imag_time > imag_time_prev_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, imag_time_spin_flip, false
                    );
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED.";
#endif 
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat == 0 
                && imag_time_spin_flip < new_imag_time && new_imag_time > imag_time_next_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, beta, false
                    );
                    auto [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, new_imag_time, beta, false
                    );
                    if (imag_time_spin_flip > 0) {
                        auto tmp = integrated_pot_energy_diff_single_spin_flip_edge(
                            lat, h, mu, J, lmbda, rand_edge, 0., imag_time_spin_flip, false
                        );
                        integrated_pot_energy_diff_edge += std::get<0>(tmp);
                        bare_pot_energy_diff_edge += std::get<1>(tmp);
                        const auto& [integrated_pot_energy_diff_tuple_2, pot_energy_tuple_indices_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_single_spin_flip_tuple(
                            lat, h, mu, J, lmbda, rand_edge, 0., imag_time_spin_flip, false
                        );
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff_tuple += integrated_pot_energy_diff_tuple_2;
                    }
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED. Flipping spin.";
#endif 
                        lat.flip_spin(rand_edge);
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                } else if (random_spin_flip_index_lat > 0 
                && imag_time_spin_flip > new_imag_time && new_imag_time < imag_time_prev_spin_flip
                ) {
                    std::tie(integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge) 
                    = integrated_pot_energy_diff_single_spin_flip_edge(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, beta, false
                    );
                    auto [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
                    = integrated_pot_energy_diff_single_spin_flip_tuple(
                        lat, h, mu, J, lmbda, rand_edge, imag_time_spin_flip, beta, false
                    );
                    if (new_imag_time > 0) {
                        auto tmp = integrated_pot_energy_diff_single_spin_flip_edge(
                            lat, h, mu, J, lmbda, rand_edge, 0., new_imag_time, false
                        );
                        integrated_pot_energy_diff_edge += std::get<0>(tmp);
                        bare_pot_energy_diff_edge += std::get<1>(tmp);
                        const auto& [integrated_pot_energy_diff_tuple_2, pot_energy_tuple_indices_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_single_spin_flip_tuple(
                            lat, h, mu, J, lmbda, rand_edge, 0., new_imag_time, false
                        );
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff_tuple += integrated_pot_energy_diff_tuple_2;
                    }
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_spin_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_spin_flip_move --- ACCEPTED. Flipping spin.";
#endif 
                        lat.flip_spin(rand_edge);
                        lat.move_spin_flip(rand_edge, random_spin_flip_index_lat, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
                        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
                            int tuple_index = pot_energy_tuple_indices[i];
                            double potential_tuple_energy_diff = pot_energy_diffs[i];
                            if constexpr (Basis == 'x') {
                                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
                            } else if constexpr (Basis == 'z') {
                                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
                            }
                        }
                    } 
                }
            }
        }
    }
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_global_single_spin_flip(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_global_single_spin_flip!";
#endif

    const auto& [rand_edge, source, target] = lat.get_random_edge();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- Randomly chose edge between vertices {} and {}.", source, target);
#endif

    const auto& [integrated_pot_energy_diff_edge, bare_pot_energy_diff_edge] 
    = integrated_pot_energy_diff_single_spin_flip_edge(
        lat, h, mu, J, lmbda, rand_edge, 0., beta, true
    );
    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_diffs] 
    = integrated_pot_energy_diff_single_spin_flip_tuple(
        lat, h, mu, J, lmbda, rand_edge, 0., beta, true
    );
    const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif  

    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_global_single_spin_flip --- ACCEPTED.";
#endif 
        lat.flip_spin(rand_edge);
        integrated_pot_energy += integrated_pot_energy_diff;
        lat.add_potential_edge_energy(rand_edge, bare_pot_energy_diff_edge);
        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
            int tuple_index = pot_energy_tuple_indices[i];
            double potential_tuple_energy_diff = pot_energy_diffs[i];
            if constexpr (Basis == 'x') {
                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
            } else if constexpr (Basis == 'z') {
                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
            }
        } 
    } 
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_global_tuple_flip(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_global_tuple_flip!";
#endif

    int random_tuple = -1;
    std::span<const Lattice::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_tuple_flip --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_tuple_flip --- Randomly chose star with center {}", random_tuple);
    }
#endif

    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
    = integrated_pot_energy_diff_tuple_flip_edge(lat, h, mu, J, lmbda, random_tuple, tuple_edges, 0, beta, true);

    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_global_single_spin_flip --- acceptance ratio: {}", acc_ratio);
#endif  

    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_global_tuple_flip --- ACCEPTED.";
#endif 
        integrated_pot_energy += integrated_pot_energy_diff;
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            auto edg = tuple_edges[i];
            lat.flip_spin(edg);
            lat.add_potential_edge_energy(edg, pot_energy_diffs[i]);
        }
    } 

}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_double_tuple_flip(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_double_tuple_flip!";
#endif

    if (Basis == 'x' && (J == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    const double rnd_create_destroy = uniform_dist(*rng);
    
    int random_tuple = -1;
    std::span<const Lattice::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Randomly chose star with center ", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Found {} tuple flips.", tuple_flip_count);
#endif  

    if (rnd_create_destroy < 0.5) { // destroy tuples
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Trying to DESTROY tuple flip pair.";
#endif
        if (tuple_flip_count > 1) [[likely]] {
            std::uniform_int_distribution<int> random_tuple_flip_dist(0, tuple_flip_count - 2);

            const int random_tuple_flip_index = random_tuple_flip_dist(*rng);

            const double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];

            const double imag_time_next_tuple_flip = tuple_flips[random_tuple_flip_index+1];

            double imag_time_next_next_tuple_flip = beta;

            if (random_tuple_flip_index != tuple_flip_count - 2) [[likely]] {
                imag_time_next_next_tuple_flip = tuple_flips[random_tuple_flip_index+2];
            }

            double imag_time_prev_tuple_flip = 0.;

            if (random_tuple_flip_index != 0) [[likely]] {
                imag_time_prev_tuple_flip = tuple_flips[random_tuple_flip_index-1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- Imaginary time of random tuple flip: {}; previous: {}; next: {}; next next: {}", imag_time_tuple_flip, imag_time_prev_tuple_flip, imag_time_next_tuple_flip, imag_time_next_next_tuple_flip);
#endif         

            const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
            = integrated_pot_energy_diff_tuple_flip_edge(
                lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                imag_time_tuple_flip, imag_time_next_tuple_flip, false
            );

            if constexpr (Basis == 'x') {
                acc_ratio = 2./(J * J * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip) 
                * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip)) 
                * std::exp(- integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = 2./(mu * mu * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip) 
                * (imag_time_next_next_tuple_flip - imag_time_prev_tuple_flip)) 
                * std::exp(- integrated_pot_energy_diff);
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- acceptance ratio: ", acc_ratio);
#endif  
            
            if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- ACCEPTED.";
#endif  
                lat.delete_double_tuple_flip(
                    random_tuple, tuple_edges, imag_time_tuple_flip, imag_time_next_tuple_flip
                );
                integrated_pot_energy += integrated_pot_energy_diff;
                for (size_t i = 0; i < tuple_edges.size(); ++i) {
                    lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                }
            }
        } else [[unlikely]] {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.;
        }
    } else { // create tuples
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Trying to CREATE tuple flip pair.";
#endif
        double tau_left, tau_right;
        int random_tuple_flip_index = 0;

        if (tuple_flip_count > 0) [[likely]] {
            std::uniform_int_distribution<int> random_tuple_flip_dist(0, tuple_flip_count);
            random_tuple_flip_index = random_tuple_flip_dist(*rng);

            if (random_tuple_flip_index == 0) {
                tau_left = 0.; //not really necessary
                tau_right = tuple_flips[0];
            } else if (random_tuple_flip_index == tuple_flip_count) {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = beta;
            } else {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = tuple_flips[random_tuple_flip_index];
            }
        } else [[unlikely]] {
            tau_left = 0.;
            tau_right = beta;
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif

        double tau_1 = 0., tau_2 = 0.;

        std::uniform_real_distribution<double> new_times_dist(tau_left, tau_right);
        tau_1 = new_times_dist(*rng);
        tau_2 = new_times_dist(*rng);
        if (tau_2 < tau_1) {
            std::swap(tau_1, tau_2);
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- tau_1: {}; tau_2: {}", tau_1, tau_2);
#endif

        if (std::abs(tau_2 - tau_1) < PRECISION 
        || std::abs(tau_1 - tau_left) < PRECISION 
        || std::abs(tau_2 - tau_right) < PRECISION
        ) [[unlikely]] {
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- Random numbers very close to each other.";
#endif
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        } else {
            const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
            = integrated_pot_energy_diff_tuple_flip_edge(
                lat, h, mu, J, lmbda, random_tuple, tuple_edges, tau_1, tau_2, false
            );

            if constexpr (Basis == 'x') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * J * J * std::exp(- integrated_pot_energy_diff);
            } else if constexpr (Basis == 'z') {
                acc_ratio = ((tau_right - tau_left)*(tau_right - tau_left))/2. 
                * mu * mu * std::exp(- integrated_pot_energy_diff);
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_double_tuple_flip --- acceptance ratio: {}", acc_ratio);
#endif 

            if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_double_tuple_flip --- ACCEPTED.";
#endif  
                lat.insert_double_tuple_flip(random_tuple, tuple_edges, tau_1, tau_2);
                integrated_pot_energy += integrated_pot_energy_diff;
                for (size_t i = 0; i < tuple_edges.size(); ++i) {
                    lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                }
            } 
        }
    }
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_single_tuple_flip_move(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_single_tuple_flip_move!";
#endif

    if (Basis == 'x' && (J == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    int random_tuple = -1;
    std::span<const Lattice::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) [[unlikely]] return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose star with center {}", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Found {} tuple flips.", tuple_flip_count);
#endif  

    if (tuple_flip_count > 0) [[likely]] {
        std::uniform_int_distribution<int> random_tuple_flip_dist(0, tuple_flip_count - 1);
        const int random_tuple_flip_index = random_tuple_flip_dist(*rng);

        const double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];

        std::vector<double> imag_times_next_flips = lat.flip_next_imag_times_tuple(tuple_edges, imag_time_tuple_flip);

        std::vector<double> imag_times_prev_flips = lat.flip_prev_imag_times_tuple(tuple_edges, imag_time_tuple_flip);

        double tau_right = imag_times_next_flips[0];
        if (tau_right < imag_time_tuple_flip) {
            tau_right += beta;
        }

        for (double& imag_time : imag_times_next_flips) {
            if (imag_time < imag_time_tuple_flip) {
                imag_time += beta;
            }
            if (imag_time < tau_right) {
                tau_right = imag_time;
            }
        }
        tau_right = modulo(tau_right, beta);

        double tau_left = imag_times_prev_flips[0];
        if (tau_left > imag_time_tuple_flip) {
            tau_left -= beta;
        }

        for (double& imag_time : imag_times_prev_flips) {
            if (imag_time > imag_time_tuple_flip) {
                imag_time -= beta;
            }
            if (imag_time > tau_left) {
                tau_left = imag_time;
            }
        }
        tau_left = modulo(tau_left, beta);

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("ropolis_step_single_tuple_flip_move --- Imaginary time of random tuple flip: {}; tau_left: {}; tau_right: {}", imag_time_tuple_flip, tau_left, tau_right);
#endif  

        if (tau_left < imag_time_tuple_flip && imag_time_tuple_flip < tau_right) { // not cross beta
            std::uniform_real_distribution<double> new_time_dist(tau_left, tau_right);
            const double new_imag_time = new_time_dist(*rng);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- New imaginary time: {}", new_imag_time);
#endif 

            if (std::abs(new_imag_time - imag_time_tuple_flip) < PRECISION) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else {
                double integrated_pot_energy_diff = 0.;
                std::span<const Lattice::Edge> pot_energy_edges;
                std::vector<double> pot_energy_diffs;
                if (new_imag_time > imag_time_tuple_flip) {
                    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false
                    );
                    pot_energy_edges = pot_energy_edges_tmp;
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge;
                } else {
                    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges_tmp, pot_energy_diffs_tmp] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false
                    );
                    pot_energy_edges = pot_energy_edges_tmp;
                    pot_energy_diffs = std::move(pot_energy_diffs_tmp);
                    integrated_pot_energy_diff = integrated_pot_energy_diff_edge;
                }
                acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                    lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                    integrated_pot_energy += integrated_pot_energy_diff;
                    for (size_t i = 0; i < tuple_edges.size(); ++i) {
                        lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                    }
                } 

            }
        } else { // Potentially cross beta
            std::uniform_real_distribution<double> new_time_dist(tau_left, beta + tau_right);
            const double new_imag_time = modulo(new_time_dist(*rng), beta);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- New imaginary time: {}", new_imag_time);
#endif 

            //double integrated_pot_energy_diff = 0.;

            if (std::abs(new_imag_time - imag_time_tuple_flip) < PRECISION 
            || std::abs(new_imag_time - tau_left) < PRECISION 
            || std::abs(new_imag_time - tau_right) < PRECISION
            ) {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            } else if (tau_left > imag_time_tuple_flip) { // Potentially over beta left
                if (new_imag_time < imag_time_tuple_flip) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false
                    );
                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time > imag_time_tuple_flip && new_imag_time < tau_right) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false
                    );
                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time > imag_time_tuple_flip && new_imag_time > tau_left) { // Move over beta
                    auto [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, beta, false
                    );
                    if (imag_time_tuple_flip != 0) {
                        const auto& [integrated_pot_energy_diff_2, pot_energy_edges_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_tuple_flip_edge(
                            lat, h, mu, J, lmbda, random_tuple, tuple_edges,
                             0., imag_time_tuple_flip, false
                        );
                        for (size_t i = 0; i < pot_energy_edges.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff += integrated_pot_energy_diff_2;
                    }

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED. Flipping spins.";
#endif  
                        for (const auto& p_edg : tuple_edges) {
                            lat.flip_spin(p_edg);
                        }
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    }
                }
            } else if (tau_right < imag_time_tuple_flip) { // Potentially over beta right
                if (new_imag_time > imag_time_tuple_flip) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, new_imag_time, false
                    );

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time < imag_time_tuple_flip && new_imag_time > tau_left) { // Normal move
                    const auto& [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        new_imag_time, imag_time_tuple_flip, false
                    );

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED.";
#endif  
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, true);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                } else if (new_imag_time < imag_time_tuple_flip && new_imag_time < tau_right) { // Move over beta
                    auto [integrated_pot_energy_diff, pot_energy_edges, pot_energy_diffs] 
                    = integrated_pot_energy_diff_tuple_flip_edge(
                        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                        imag_time_tuple_flip, beta, false
                    );
                    if (new_imag_time > 0) {
                        const auto& [integrated_pot_energy_diff_2, pot_energy_edges_2, pot_energy_diffs_2] 
                        = integrated_pot_energy_diff_tuple_flip_edge(
                            lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
                            0., new_imag_time, false
                        );
                        for (size_t i = 0; i < pot_energy_edges.size(); ++i) {
                            pot_energy_diffs[i] += pot_energy_diffs_2[i];
                        }
                        integrated_pot_energy_diff += integrated_pot_energy_diff_2;
                    }

                    acc_ratio = std::exp(- integrated_pot_energy_diff);

#ifndef NDEBUG
                    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- acceptance ratio: {}", acc_ratio);
#endif  

                    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
                        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_single_tuple_flip_move --- ACCEPTED. Flipping spins.";
#endif 
                        for (const auto& p_edg : tuple_edges) {
                            lat.flip_spin(p_edg);
                        }
                        lat.move_tuple_flip(random_tuple, tuple_edges, imag_time_tuple_flip, new_imag_time, false);
                        integrated_pot_energy += integrated_pot_energy_diff;
                        for (size_t i = 0; i < tuple_edges.size(); ++i) {
                            lat.add_potential_edge_energy(tuple_edges[i], pot_energy_diffs[i]);
                        }
                    } 
                }
            }
        }
    }
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step_spin_tuple_combination(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << "";
    BOOST_LOG_TRIVIAL(debug) << "# Welcome to MC update metropolis_step_spin_tuple_combination!";
#endif

    if (Basis == 'x' && (J == 0 || lmbda == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }

    if (Basis == 'z' && (mu == 0 || h == 0)) {
        // The acceptance ratio is set to zero for diagnostics
        acc_ratio = 0.; 
        return;
    }
    
    double rnd_create_destroy = uniform_dist(*rng);
    
    int random_tuple = -1;
    std::span<const Lattice::Edge> tuple_edges;

    if constexpr (Basis == 'x') {
        random_tuple = lat.get_random_plaquette_index();
        tuple_edges = lat.get_plaquette_edges(random_tuple);
    } else if constexpr (Basis == 'z') {
        random_tuple = lat.get_random_vertex();
        tuple_edges = lat.get_star_edges(random_tuple);
    }
    if (tuple_edges.empty()) return;

#ifndef NDEBUG
    if constexpr (Basis == 'x') {
        const auto tuple_vertex_pairs = lat.get_plaquette_vertex_pairs(random_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Randomly chose plaquette with index {} and edges {}", random_tuple, tuple_vertex_pairs);
    } else if constexpr (Basis == 'z') {
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_single_tuple_flip_move --- Randomly chose star with center {}", random_tuple);
    }
#endif

    std::span<const double> tuple_flips = lat.get_tuple_spin_flips(random_tuple);
    int tuple_flip_count = tuple_flips.size();

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Found {} tuple flips.", tuple_flip_count);
#endif  

    bool tuple_destroy = true;
    int random_tuple_flip_index = -1;
    double tau_new = -1., r = 0., tau_left = 0., tau_right = 0.;

    if (rnd_create_destroy < 0.5) { // Destroys tuple
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Trying to DESTROY tuple flip.";
#endif
        tuple_destroy = true;
        if (tuple_flip_count > 0) [[likely]] {
            std::uniform_int_distribution<int> random_tuple_flip_dist(0, tuple_flip_count - 1);
            random_tuple_flip_index = random_tuple_flip_dist(*rng);

            double imag_time_tuple_flip = tuple_flips[random_tuple_flip_index];
            tau_new = imag_time_tuple_flip;

            double imag_time_next_tuple_flip = beta;

            if (random_tuple_flip_index < tuple_flip_count-1) {
                imag_time_next_tuple_flip = tuple_flips[(random_tuple_flip_index+1)];
            }

            tau_right = imag_time_next_tuple_flip;

            double imag_time_prev_tuple_flip = 0.;

            if (random_tuple_flip_index != 0) {
                imag_time_prev_tuple_flip = tuple_flips[(random_tuple_flip_index-1)];
            }
            tau_left = imag_time_prev_tuple_flip;

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Imaginary time of random tuple flip: {}; tau_left: {}; tau_right: {}", imag_time_tuple_flip, tau_left, tau_right);
#endif  
            if constexpr (Basis == 'x') {
                r = 1. / ((imag_time_next_tuple_flip-imag_time_prev_tuple_flip) * J);
            } else if constexpr (Basis == 'z') {
                r = 1. / ((imag_time_next_tuple_flip-imag_time_prev_tuple_flip) * mu);
            }
        } else [[unlikely]] {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        }
    } else { // Create tuple
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Trying to CREATE tuple flip.";
#endif
        tuple_destroy = false;

        random_tuple_flip_index = 0;

        if (tuple_flip_count > 0) [[likely]] {
            std::uniform_int_distribution<int> random_tuple_flip_dist(0, tuple_flip_count);
            random_tuple_flip_index = random_tuple_flip_dist(*rng);

            if (random_tuple_flip_index == 0) {
                tau_left = 0.; // not really necessary
                tau_right = tuple_flips[0];
            } else if (random_tuple_flip_index == tuple_flip_count) {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = beta;
            } else {
                tau_left = tuple_flips[random_tuple_flip_index-1];
                tau_right = tuple_flips[random_tuple_flip_index];
            }
        } else [[unlikely]] {
            tau_left = 0.;
            tau_right = beta;
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- tau_left: {}; tau_right: {}", tau_left, tau_right);
#endif  

        if (tau_left < tau_right) {
            std::uniform_real_distribution<double> new_time_dist(tau_left, tau_right);
            tau_new = new_time_dist(*rng);

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- Proposed tuple flip imaginary time: {}", tau_new);
#endif 

            if (lat.check_spin_flips_present_tuple(tuple_edges, tau_new)) {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

            if (std::abs(tau_new - tau_left) < PRECISION || std::abs(tau_new - tau_right) < PRECISION) [[unlikely]] {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }
            
            if constexpr (Basis == 'x') {
                r = (tau_right-tau_left) * J;
            } else if constexpr (Basis == 'z') {
                r = (tau_right-tau_left) * mu;
            }
        } else {
            // The acceptance ratio is set to zero for diagnostics
            acc_ratio = 0.; 
            return;
        }
    }

    std::vector<double> r_b;
    std::vector<bool> create_vector;
    std::vector<double> flip_times;

    // These variables determine the range in which the potential energy can change (potentially :D)
    double tau_right_potential_energy = tau_right;
    double tau_left_potential_energy = tau_left;

    for (size_t i = 0; i < tuple_edges.size(); ++i) {
        rnd_create_destroy = uniform_dist(*rng);
        int tau_spin_flip_index = -1;
        double tau_spin_flip = -1.;
        
        const Lattice::Edge edg = tuple_edges[i];
        
        std::span<const double> single_spin_flips = lat.get_single_spin_flips(edg);
        int single_spin_flip_count = single_spin_flips.size();

#ifndef NDEBUG
        int spin = lat.get_spin(edg);
        const auto& [source_v, target_v] = lat.vertices_of_edge(edg);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Spin: {}. Found {} single spin flips.", source_v, target_v, spin, single_spin_flip_count);
#endif  

        if (rnd_create_destroy < 0.5) { // destroy
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Trying to DESTROY single spin flip.", source_v, target_v);
#endif  

            if (single_spin_flip_count == 0) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

            int imag_time_prev_flip_index = -1;
            double imag_time_prev_flip = 0.;
            for (int j = 0; j < single_spin_flip_count; ++j) {
                const double tau = single_spin_flips[j];
                if (tau > tau_new) {
                    break;
                } 
                imag_time_prev_flip = tau;
                imag_time_prev_flip_index = j;
                if (j == single_spin_flip_count - 1) [[unlikely]] {
                    imag_time_prev_flip_index = single_spin_flip_count-1;
                    break;
                }
            }

            int imag_time_next_flip_index = 0;
            double imag_time_next_flip = 0.;
            for (int j = 0; j < single_spin_flip_count; ++j) {
                const double tau = single_spin_flips[j];
                imag_time_next_flip_index = j;
                if (tau > tau_new) {
                    imag_time_next_flip = tau;
                    break;
                } 
                if (j == single_spin_flip_count - 1) [[unlikely]] {
                    imag_time_next_flip_index = single_spin_flip_count;
                    imag_time_next_flip = beta;
                    break;
                }
            }

            create_vector.emplace_back(false);
            rnd_create_destroy = uniform_dist(*rng);

            if (rnd_create_destroy < 0.5) {
                tau_spin_flip = imag_time_prev_flip;
                tau_spin_flip_index = imag_time_prev_flip_index;
            } else {
                tau_spin_flip = imag_time_next_flip;
                tau_spin_flip_index = imag_time_next_flip_index;
            }

            if (tau_spin_flip_index == -1 || tau_spin_flip_index == single_spin_flip_count) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }
            
            double tau_prev_of_chosen = 0.;
            if (tau_spin_flip_index > 0) {
                tau_prev_of_chosen = single_spin_flips[tau_spin_flip_index-1];
            }

            double tau_next_of_chosen = beta;
            if (tau_spin_flip_index < single_spin_flip_count-1) {
                tau_next_of_chosen = single_spin_flips[tau_spin_flip_index+1];
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Propose to remove single spin flip at {}", source_v, target_v, tau_spin_flip);            
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - previous: {} - next: {}", source_v, target_v, tau_prev_of_chosen, tau_next_of_chosen);
#endif 

            if constexpr (Basis == 'x') {
                r_b.emplace_back(2./((tau_next_of_chosen - tau_prev_of_chosen) * lmbda));
            } else if constexpr (Basis == 'z') {
                r_b.emplace_back(2./((tau_next_of_chosen - tau_prev_of_chosen) * h));
            }

            flip_times.emplace_back(tau_spin_flip);

            if (tau_next_of_chosen > tau_right_potential_energy) {
                tau_right_potential_energy = tau_next_of_chosen;
            }

            if (tau_prev_of_chosen < tau_left_potential_energy) {
                tau_left_potential_energy = tau_prev_of_chosen;
            }
        } else { // create
#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Trying to CREATE single spin flip.", source_v, target_v);
#endif  
            
            double imag_time_prev_flip = 0.;

            if (single_spin_flip_count > 0) {
                for (int j = 0; j < single_spin_flip_count; ++j) {
                    const double tau = single_spin_flips[j];
                    if (tau > tau_new) {
                        break;
                    } 
                    imag_time_prev_flip = tau;
                    if (j == single_spin_flip_count - 1) [[unlikely]] {
                        break;
                    }
                }
            }

            double imag_time_next_flip = 0.;

            if (single_spin_flip_count > 0) {
                for (int j = 0; j < single_spin_flip_count; ++j) {
                    const double tau = single_spin_flips[j];
                    if (tau > tau_new) {
                        imag_time_next_flip = tau;
                        break;
                    } 
                    if (j == single_spin_flip_count - 1) [[unlikely]] {
                        imag_time_next_flip = beta;
                        break;
                    }
                }
            } else {
                imag_time_next_flip = beta;
            }

            create_vector.emplace_back(true);

            std::uniform_real_distribution<double> new_time_single_flip_dist(imag_time_prev_flip, imag_time_next_flip);
            tau_spin_flip = new_time_single_flip_dist(*rng);

            if (std::abs(tau_spin_flip - imag_time_prev_flip) < PRECISION 
            || std::abs(tau_spin_flip - imag_time_next_flip) < PRECISION
            ) [[unlikely]] {
#ifndef NDEBUG
                BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- Random numbers very close to each other.";
#endif
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

            if (std::abs(tau_spin_flip - tau_new) < PRECISION) [[unlikely]] {
                // The acceptance ratio is set to zero for diagnostics
                acc_ratio = 0.; 
                return;
            }

#ifndef NDEBUG
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - Propose to create single spin flip at {}", source_v, target_v, tau_spin_flip);
            BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- edge between vertices {} and {} - previous: {} - next: {}", source_v, target_v, imag_time_prev_flip, imag_time_next_flip);
#endif 
            
            if constexpr (Basis == 'x') {
                r_b.emplace_back((imag_time_next_flip - imag_time_prev_flip) * lmbda / 2.); 
            } else if constexpr (Basis == 'z') {
                r_b.emplace_back((imag_time_next_flip - imag_time_prev_flip) * h / 2.); 
            }

            flip_times.emplace_back(tau_spin_flip);

            if (imag_time_next_flip > tau_right_potential_energy) {
                tau_right_potential_energy = imag_time_next_flip;
            }

            if (imag_time_prev_flip < tau_left_potential_energy) {
                tau_left_potential_energy = imag_time_prev_flip;
            }     
        }
    }

    const auto& [integrated_pot_energy_diff_edge, pot_energy_edges, pot_energy_edges_diffs] 
    = integrated_pot_energy_diff_combination_flip_edge(
        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
        tau_new, flip_times, tau_left_potential_energy, tau_right_potential_energy, 
        create_vector, tuple_destroy
    );
    const auto& [integrated_pot_energy_diff_tuple, pot_energy_tuple_indices, pot_energy_tuple_diffs] 
    = integrated_pot_energy_diff_combination_flip_tuple(
        lat, h, mu, J, lmbda, random_tuple, tuple_edges, 
        tau_new, flip_times, tau_left_potential_energy, tau_right_potential_energy, 
        create_vector, tuple_destroy
    );
    const double integrated_pot_energy_diff = integrated_pot_energy_diff_edge + integrated_pot_energy_diff_tuple;

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff_edge: {}", integrated_pot_energy_diff_edge);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff_tuple: {}", integrated_pot_energy_diff_tuple);
        BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- integrated_pot_energy_diff: {}", integrated_pot_energy_diff);
#endif 

    acc_ratio = std::exp(- integrated_pot_energy_diff) * r * std::accumulate(r_b.begin(), r_b.end(), 1., std::multiplies<double>());

#ifndef NDEBUG
    BOOST_LOG_TRIVIAL(debug) << std::format("metropolis_step_spin_tuple_combination --- acceptance ratio: {}", acc_ratio);
#endif  

    if (uniform_dist(*rng) < (acc_ratio > 1. ? 1. : acc_ratio)) {
#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "metropolis_step_spin_tuple_combination --- ACCEPTED.";
#endif 
        combination_flip(lat, h, mu, random_tuple, tuple_edges, tau_new, flip_times, create_vector, tuple_destroy); 
        integrated_pot_energy += integrated_pot_energy_diff;
        for (size_t i = 0; i < tuple_edges.size(); ++i) {
            lat.add_potential_edge_energy(pot_energy_edges[i], pot_energy_edges_diffs[i]);
        }
        for (size_t i = 0; i < pot_energy_tuple_indices.size(); ++i) {
            int tuple_index = pot_energy_tuple_indices[i];
            double potential_tuple_energy_diff = pot_energy_tuple_diffs[i];
            if constexpr (Basis == 'x') {
                lat.add_potential_star_energy(tuple_index, potential_tuple_energy_diff);
            } else if constexpr (Basis == 'z') {
                lat.add_potential_plaquette_energy(tuple_index, potential_tuple_energy_diff);
            }
        } 
    } 
}

template<char Basis>
requires ValidBasis<Basis>
void ExtendedToricCodeQMC<Basis>::metropolis_step(
    Lattice& lat, double& integrated_pot_energy, double& acc_ratio, double beta, 
    double h, double mu, double J, double lmbda
) {
    const int rnd = mc_update_dist(*rng);
    if (rnd < 1) {
        metropolis_step_double_single_spin_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 2) {
        metropolis_step_single_spin_flip_move(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 3) {
        metropolis_step_global_single_spin_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 4) {
        metropolis_step_global_tuple_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 5) {
        metropolis_step_double_tuple_flip(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else if (rnd < 6) {
        metropolis_step_single_tuple_flip_move(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    } else {
        metropolis_step_spin_tuple_combination(lat, integrated_pot_energy, acc_ratio, beta, h, mu, J, lmbda);
    }  

#ifndef NDEBUG
    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, h, mu, J, lmbda
    );

    BOOST_LOG_TRIVIAL(debug) << std::format("Integrated potential energy --- Cached: {}, Actual: {}", integrated_pot_energy, integrated_pot_energy_check);
#endif  
}

template<char Basis>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis>::get_thermalization(
    const Config& config
) {

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }
    
    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (config.param_spec.lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (config.param_spec.h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    std::vector<double> acc_ratio_vector;
    // Vector to store observable results for all snapshots
    std::vector<std::vector<std::variant< std::complex<double>, double>>> observable_vector;
    std::vector<std::variant< std::complex<double>, double>> obs_temp;
    for (const auto& obs_func : config.sim_spec.observables) {
        UNUSED(obs_func);
        observable_vector.emplace_back( obs_temp );
    } 

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);
    auto obs_type_vec = get_obs_type_vec(config.sim_spec.observables);

    // Initialize Lattice
    auto lat = Lattice(config.lat_spec, rng);
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    double acc_ratio = 1.;

    int total_metropolis_step_count = 0;
    int reset_potential_energy_count = static_cast<int>(lat.get_edge_count()*10000);

    for (int i = 0; i < config.sim_spec.N_thermalization; ++i) {
        ++total_metropolis_step_count;
        metropolis_step(
            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
            config.param_spec.h, config.param_spec.mu, 
            config.param_spec.J, config.param_spec.lmbda
        );

        if (total_metropolis_step_count % reset_potential_energy_count == 0) {
            // avoid accumulation of small numerical errors leading to bias
            lat.init_potential_energy();
            lat.rotate_imag_time();
            integrated_pot_energy = total_integrated_pot_energy(
                lat, config.param_spec.h, config.param_spec.mu, 
                config.param_spec.J, config.param_spec.lmbda
            );
        }
        bool rotated_for_susceptibility = false;
        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            if (!rotated_for_susceptibility && obs_type_vec[k] == "susceptibility") {
                lat.rotate_imag_time();
                rotated_for_susceptibility = true;
            }
            observable_vector[k].emplace_back(
                obs_func_vec[k](lat, config.param_spec.h, config.param_spec.lmbda, config.param_spec.mu, config.param_spec.J)
            );
        }
        acc_ratio_vector.emplace_back(acc_ratio);

        if (config.out_spec.save_snapshots && i%10000 == 0) [[unlikely]] {
            lat.update_spin_string();
        }
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    if (config.out_spec.save_snapshots) {
        lat.write_graph("snapshots", config.out_spec.path_out);
    }

    return Result{.series=observable_vector, .acc_ratio=acc_ratio_vector};                                     
}

template<char Basis>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis>::get_sample(
    const Config& config
) { 

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }

    // Build PT runtime state once per simulation call.
    auto pt_ctx = pt_detail::init_context(config);

    // Runtime couplings (possibly tempered per rank).
    double h_runtime = config.param_spec.h;
    double mu_runtime = config.param_spec.mu;
    double J_runtime = config.param_spec.J;
    double lmbda_runtime = config.param_spec.lmbda;
    pt_detail::apply_tempered_parameter(pt_ctx, h_runtime, mu_runtime, J_runtime, lmbda_runtime);

    if (pt_ctx.enabled) {
        if constexpr (Basis != 'z') {
            throw std::invalid_argument("PT currently supports only basis=z.");
        }
    }

    if constexpr (Basis == 'x') {
        if (J_runtime < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (lmbda_runtime < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (mu_runtime < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (h_runtime < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    if (pt_ctx.enabled) {
        BOOST_LOG_TRIVIAL(info) << std::format(
            "[PT] rank {}/{} initial {}={}",
            pt_ctx.rank, pt_ctx.world_size, pt_ctx.parameter, pt_ctx.tempered_value
        );
        pt_detail::initialize_trace_file_if_requested(config, pt_ctx);
    }

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);
    auto obs_type_vec = get_obs_type_vec(config.sim_spec.observables);

    const int pt_ladder_size = pt_ctx.enabled ? pt_ctx.world_size : 1;
    const size_t observable_count = config.sim_spec.observables.size();
    const bool keep_full_series = config.out_spec.full_time_series;
    const bool keep_acc_ratio_series = config.out_spec.full_time_series;

    // Vector to store observable results for all snapshots (kept only when requested).
    std::vector<std::vector< std::variant< std::complex<double>, double> >> observable_vector(observable_count);
    // Always keep typed samples for statistics to avoid variant unpack pass at the end.
    std::vector<std::vector<double>> stats_real_samples(observable_count);
    std::vector<std::vector<double>> stats_imag_samples(observable_count);
    std::vector<int> obs_kind(observable_count, 0); // 0=real, 1=fredenhagen_marcu, 2=susceptibility
    bool has_susceptibility_obs = false;

    const size_t sample_reserve = static_cast<size_t>(std::max(0, config.sim_spec.N_samples));
    for (size_t k = 0; k < observable_count; ++k) {
        if (obs_type_vec[k] == "fredenhagen_marcu") {
            obs_kind[k] = 1;
        } else if (obs_type_vec[k] == "susceptibility") {
            obs_kind[k] = 2;
            has_susceptibility_obs = true;
        }
        stats_real_samples[k].reserve(sample_reserve);
        if (obs_kind[k] != 0) {
            stats_imag_samples[k].reserve(sample_reserve);
        }
        if (keep_full_series) {
            observable_vector[k].reserve(sample_reserve);
        }
    }

    // PT-binned local real-observable samples: [observable_index][ladder_index].
    std::vector<std::vector<std::vector<double>>> pt_binned_local_real_samples;
    // PT-binned local FM observable components: [observable_index][ladder_index].
    std::vector<std::vector<std::vector<double>>> pt_binned_local_fm_real_samples;
    std::vector<std::vector<std::vector<double>>> pt_binned_local_fm_imag_samples;
    std::vector<std::vector<std::vector<double>>> pt_binned_local_fm_abs_samples;
    std::vector<double> acc_ratio_vector;
    if (keep_acc_ratio_series) {
        const size_t between = static_cast<size_t>(std::max(1, config.sim_spec.N_between_samples));
        acc_ratio_vector.reserve(sample_reserve * between);
    }
    std::vector<double> observable_mean_vector(config.sim_spec.observables.size(), 0.), 
                        observable_std_vector(config.sim_spec.observables.size(), 0.), 
                        binder_mean_vector(config.sim_spec.observables.size(), 0.), 
                        binder_std_vector(config.sim_spec.observables.size(), 0.), 
                        observable_autocorrelation_time_vector(config.sim_spec.observables.size(), 0.);
    if (pt_ctx.enabled) {
        pt_binned_local_real_samples.resize(
            observable_count,
            std::vector<std::vector<double>>(static_cast<size_t>(pt_ladder_size))
        );
        pt_binned_local_fm_real_samples.resize(
            observable_count,
            std::vector<std::vector<double>>(static_cast<size_t>(pt_ladder_size))
        );
        pt_binned_local_fm_imag_samples.resize(
            observable_count,
            std::vector<std::vector<double>>(static_cast<size_t>(pt_ladder_size))
        );
        pt_binned_local_fm_abs_samples.resize(
            observable_count,
            std::vector<std::vector<double>>(static_cast<size_t>(pt_ladder_size))
        );
    }

    // Initialize Lattice
    auto lat = Lattice(config.lat_spec, rng);
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, h_runtime, mu_runtime, J_runtime, lmbda_runtime
    );
    double acc_ratio = 1.;

    if (config.sim_spec.custom_therm) {
        if (pt_ctx.enabled) {
            throw std::invalid_argument("PT scaffold currently requires custom_therm=false.");
        }
        // Pre-Thermalization 
        for (int i = 0; i < config.sim_spec.N_thermalization; ++i) {
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                config.param_spec.h_therm, config.param_spec.mu, config.param_spec.J, 
                config.param_spec.lmbda_therm
            );
        }

        // For sampling first order hysteresis
        double h_end = config.param_spec.h_therm;
        if (std::abs(config.param_spec.h - config.param_spec.h_therm) > PRECISION) {
            for (int i = 9; i > -1; --i) {
                h_end = config.param_spec.h + (config.param_spec.h_therm-config.param_spec.h) * i / 10.;
                lat.init_potential_energy();
                lat.rotate_imag_time();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, h_end, config.param_spec.mu, 
                    config.param_spec.J, config.param_spec.lmbda_therm
                );
                for (int j = 0; j < config.sim_spec.N_thermalization / 10.; ++j) {
                    metropolis_step(
                        lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h_end, 
                        config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda_therm
                    );
                }
            }
        }
        double lmbda_end = config.param_spec.lmbda_therm;
        if (std::abs(config.param_spec.lmbda - config.param_spec.lmbda_therm) > PRECISION) {
            for (int i = 9; i > -1; --i) {
                lmbda_end = config.param_spec.lmbda + (config.param_spec.lmbda_therm-config.param_spec.lmbda) * i / 10.;
                lat.init_potential_energy();
                lat.rotate_imag_time();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, h_end, config.param_spec.mu, 
                    config.param_spec.J, lmbda_end
                );
                for (int j = 0; j < config.sim_spec.N_thermalization / 10.; ++j) {
                    metropolis_step(
                        lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h_end, 
                        config.param_spec.mu, config.param_spec.J, lmbda_end
                    );
                }
            }
        }
    } else {
        // Thermalization 
        for (int i = 0; i < config.sim_spec.N_thermalization; ++i) {
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                h_runtime, mu_runtime, J_runtime, lmbda_runtime
            );
        }
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, h_runtime, mu_runtime, J_runtime, lmbda_runtime
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    int total_metropolis_step_count = 0;
    int reset_potential_energy_count = static_cast<int>(lat.get_edge_count()*100000);
    std::uint64_t pt_swap_attempted_local = 0;
    std::uint64_t pt_swap_accepted_local = 0;
    std::uint64_t pt_swap_rejected_local = 0;

    std::vector<std::uint64_t> pt_visit_counts_local;
    std::vector<std::uint64_t> pt_feedback_up_local;
    std::vector<std::uint64_t> pt_feedback_dn_local;
    int pt_feedback_source_end_local = -1;
    std::uint64_t pt_round_trip_local = 0;
    std::uint64_t pt_round_trip_strict_local = 0;
    int pt_round_trip_last_end = -1;
    int pt_round_trip_strict_start_end = -1;
    bool pt_round_trip_strict_seen_opposite = false;
    if (pt_ctx.enabled) {
        pt_visit_counts_local.assign(static_cast<size_t>(pt_ladder_size), 0);
        pt_feedback_up_local.assign(static_cast<size_t>(pt_ladder_size), 0);
        pt_feedback_dn_local.assign(static_cast<size_t>(pt_ladder_size), 0);
    }

    for (int i = 0; i < config.sim_spec.N_samples; ++i) {
        for (int j = 0; j < config.sim_spec.N_between_samples; ++j) {
            ++total_metropolis_step_count;
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h_runtime, 
                mu_runtime, J_runtime, lmbda_runtime
            );
            // PT hook lives at sweep scheduling level, not inside local Metropolis kernels.
            pt_detail::maybe_attempt_swap(
                config,
                pt_ctx,
                lat,
                *rng,
                total_metropolis_step_count,
                h_runtime,
                mu_runtime,
                J_runtime,
                lmbda_runtime,
                integrated_pot_energy,
                pt_swap_attempted_local,
                pt_swap_accepted_local,
                pt_swap_rejected_local
            );
            if (keep_acc_ratio_series) acc_ratio_vector.emplace_back(acc_ratio);
            if (total_metropolis_step_count % reset_potential_energy_count == 0) {
                // avoid accumulation of small numerical errors leading to bias
                lat.init_potential_energy();
                lat.rotate_imag_time();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, h_runtime, mu_runtime, J_runtime, lmbda_runtime
                );
            }
        }

        int current_ladder_index = 0;
        if (pt_ctx.enabled) {
            current_ladder_index = pt_ctx.tempered_index;
            if (current_ladder_index < 0 || current_ladder_index >= pt_ladder_size) {
                throw std::runtime_error(
                    std::format("Invalid PT ladder index {} at measurement step.", current_ladder_index)
                );
            }
            pt_visit_counts_local[static_cast<size_t>(current_ladder_index)] += 1;
            if (current_ladder_index == 0 || current_ladder_index == pt_ladder_size - 1) {
                const int end_marker = (current_ladder_index == 0) ? 0 : 1;
                if (pt_round_trip_last_end != -1 && pt_round_trip_last_end != end_marker) {
                    ++pt_round_trip_local;
                }
                pt_round_trip_last_end = end_marker;

                // Strict round trip: count complete end->opposite_end->same_end cycles.
                if (pt_round_trip_strict_start_end == -1) {
                    pt_round_trip_strict_start_end = end_marker;
                    pt_round_trip_strict_seen_opposite = false;
                } else if (end_marker != pt_round_trip_strict_start_end) {
                    pt_round_trip_strict_seen_opposite = true;
                } else if (pt_round_trip_strict_seen_opposite) {
                    ++pt_round_trip_strict_local;
                    pt_round_trip_strict_seen_opposite = false;
                }

                // Feedback label from the most recently visited endpoint.
                pt_feedback_source_end_local = end_marker;
            }
            if (pt_feedback_source_end_local == 0) {
                pt_feedback_up_local[static_cast<size_t>(current_ladder_index)] += 1;
            } else if (pt_feedback_source_end_local == 1) {
                pt_feedback_dn_local[static_cast<size_t>(current_ladder_index)] += 1;
            }
        }

        bool rotated_for_susceptibility = false;
        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            if (has_susceptibility_obs && !rotated_for_susceptibility && obs_kind[k] == 2) {
                lat.rotate_imag_time();
                rotated_for_susceptibility = true;
            }

            auto obs_value = obs_func_vec[k](lat, h_runtime, lmbda_runtime, mu_runtime, J_runtime);
            if (keep_full_series) {
                observable_vector[k].emplace_back(obs_value);
            }

            double re = 0.0;
            double im = 0.0;
            if (const auto* c = std::get_if<std::complex<double>>(&obs_value)) {
                re = c->real();
                im = c->imag();
            } else {
                re = std::get<double>(obs_value);
                if (obs_kind[k] != 0) {
                    im = 0.0;
                }
            }
            stats_real_samples[k].emplace_back(re);
            if (obs_kind[k] != 0) {
                stats_imag_samples[k].emplace_back(im);
            }

            if (pt_ctx.enabled && obs_kind[k] == 0) {
                pt_binned_local_real_samples[k][static_cast<size_t>(current_ladder_index)]
                    .emplace_back(re);
            } else if (pt_ctx.enabled && obs_kind[k] == 1) {
                const double absv = std::hypot(re, im);
                const size_t ladder_idx = static_cast<size_t>(current_ladder_index);
                pt_binned_local_fm_real_samples[k][ladder_idx].emplace_back(re);
                pt_binned_local_fm_imag_samples[k][ladder_idx].emplace_back(im);
                pt_binned_local_fm_abs_samples[k][ladder_idx].emplace_back(absv);
            }
        }

        if (config.out_spec.save_snapshots) {
            lat.update_spin_string();
        }
    }

#ifdef PARATORIC_HAS_MPI
    if (pt_ctx.enabled) {
        std::uint64_t attempted_sum = 0;
        std::uint64_t accepted_sum = 0;
        std::uint64_t rejected_sum = 0;
        MPI_Reduce(
            &pt_swap_attempted_local,
            &attempted_sum,
            1,
            MPI_UINT64_T,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
        MPI_Reduce(
            &pt_swap_accepted_local,
            &accepted_sum,
            1,
            MPI_UINT64_T,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
        MPI_Reduce(
            &pt_swap_rejected_local,
            &rejected_sum,
            1,
            MPI_UINT64_T,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
        if (pt_ctx.rank == 0) {
            if (config.pt_spec.trace_enabled && config.pt_spec.trace_interval > 0) {
                std::ofstream trace_file(pt_detail::swap_trace_file_path(config), std::ios::app);
                if (trace_file) {
                    const std::uint64_t attempted_pairs = attempted_sum;
                    const std::uint64_t accepted_pairs = accepted_sum;
                    const std::uint64_t rejected_pairs = rejected_sum;
                    const double ratio_pairs = attempted_pairs > 0
                        ? static_cast<double>(accepted_pairs) / static_cast<double>(attempted_pairs)
                        : 0.0;
                    trace_file << "summary attempted_local_sum=" << attempted_sum
                               << " accepted_local_sum=" << accepted_sum
                               << " rejected_local_sum=" << rejected_sum
                               << " attempted_pairs=" << attempted_pairs
                               << " accepted_pairs=" << accepted_pairs
                               << " rejected_pairs=" << rejected_pairs
                               << " acceptance_ratio_pairs=" << ratio_pairs
                               << '\n';
                }
            }
        }

        if (pt_ctx.enabled) {
            const int ladder_size = pt_ctx.world_size;
            std::vector<std::uint64_t> visit_counts_total;
            if (pt_ctx.rank == 0) {
                visit_counts_total.assign(static_cast<size_t>(ladder_size), 0);
            }
            MPI_Reduce(
                pt_visit_counts_local.data(),
                pt_ctx.rank == 0 ? visit_counts_total.data() : nullptr,
                ladder_size,
                MPI_UINT64_T,
                MPI_SUM,
                0,
                MPI_COMM_WORLD
            );

            std::vector<std::uint64_t> round_trip_counts;
            std::vector<std::uint64_t> strict_round_trip_counts;
            if (pt_ctx.rank == 0) {
                round_trip_counts.assign(static_cast<size_t>(pt_ctx.world_size), 0);
                strict_round_trip_counts.assign(static_cast<size_t>(pt_ctx.world_size), 0);
            }
            MPI_Gather(
                &pt_round_trip_local,
                1,
                MPI_UINT64_T,
                pt_ctx.rank == 0 ? round_trip_counts.data() : nullptr,
                1,
                MPI_UINT64_T,
                0,
                MPI_COMM_WORLD
            );
            MPI_Gather(
                &pt_round_trip_strict_local,
                1,
                MPI_UINT64_T,
                pt_ctx.rank == 0 ? strict_round_trip_counts.data() : nullptr,
                1,
                MPI_UINT64_T,
                0,
                MPI_COMM_WORLD
            );

            if (pt_ctx.rank == 0) {
                const auto visits_path = pt_detail::ladder_visit_file_path(config);
                std::ofstream visits_file(visits_path);
                if (visits_file) {
                    visits_file << "# ladder_index tempered_value visit_count visit_fraction\n";
                    const std::uint64_t total_visits = std::accumulate(
                        visit_counts_total.begin(), visit_counts_total.end(), std::uint64_t{0}
                    );
                    for (int idx = 0; idx < ladder_size; ++idx) {
                        const auto count = visit_counts_total[static_cast<size_t>(idx)];
                        const double frac = total_visits > 0
                            ? static_cast<double>(count) / static_cast<double>(total_visits)
                            : 0.0;
                        visits_file << idx << '\t'
                                    << pt_ctx.ladder_values[static_cast<size_t>(idx)] << '\t'
                                    << count << '\t'
                                    << frac << '\n';
                    }
                }

                const auto roundtrip_path = pt_detail::round_trip_file_path(config);
                std::ofstream roundtrip_file(roundtrip_path);
                if (roundtrip_file) {
                    roundtrip_file << "# rank end_to_end_trips strict_round_trips\n";
                    for (int r = 0; r < pt_ctx.world_size; ++r) {
                        roundtrip_file << r << '\t'
                                       << round_trip_counts[static_cast<size_t>(r)]
                                       << '\t'
                                       << strict_round_trip_counts[static_cast<size_t>(r)]
                                       << '\n';
                    }
                }
            }

            std::vector<std::uint64_t> feedback_up_total;
            std::vector<std::uint64_t> feedback_dn_total;
            if (pt_ctx.rank == 0) {
                feedback_up_total.assign(static_cast<size_t>(ladder_size), 0);
                feedback_dn_total.assign(static_cast<size_t>(ladder_size), 0);
            }
            MPI_Reduce(
                pt_feedback_up_local.data(),
                pt_ctx.rank == 0 ? feedback_up_total.data() : nullptr,
                ladder_size,
                MPI_UINT64_T,
                MPI_SUM,
                0,
                MPI_COMM_WORLD
            );
            MPI_Reduce(
                pt_feedback_dn_local.data(),
                pt_ctx.rank == 0 ? feedback_dn_total.data() : nullptr,
                ladder_size,
                MPI_UINT64_T,
                MPI_SUM,
                0,
                MPI_COMM_WORLD
            );

            if (pt_ctx.rank == 0) {
                const auto feedback_path = pt_detail::feedback_diag_file_path(config);
                std::ofstream feedback_file(feedback_path);
                if (feedback_file) {
                    std::vector<double> f_beta(static_cast<size_t>(ladder_size), 1.0);
                    std::vector<double> f_drop(
                        ladder_size > 1 ? static_cast<size_t>(ladder_size - 1) : 0,
                        0.0
                    );
                    std::uint64_t visits_sum = 0;
                    std::uint64_t visits_min = std::numeric_limits<std::uint64_t>::max();
                    int bottleneck_pair = -1;
                    double bottleneck_drop = std::numeric_limits<double>::infinity();
                    for (int idx = 0; idx < ladder_size; ++idx) {
                        const auto n_up = feedback_up_total[static_cast<size_t>(idx)];
                        const auto n_dn = feedback_dn_total[static_cast<size_t>(idx)];
                        const auto visits_both = n_up + n_dn;
                        visits_sum += visits_both;
                        visits_min = std::min(visits_min, visits_both);
                        if (visits_both > 0) {
                            f_beta[static_cast<size_t>(idx)]
                                = static_cast<double>(n_up) / static_cast<double>(visits_both);
                        } else if (idx > 0) {
                            f_beta[static_cast<size_t>(idx)] = f_beta[static_cast<size_t>(idx - 1)];
                        }
                    }
                    for (int idx = 0; idx + 1 < ladder_size; ++idx) {
                        const double drop = f_beta[static_cast<size_t>(idx)]
                                          - f_beta[static_cast<size_t>(idx + 1)];
                        f_drop[static_cast<size_t>(idx)] = drop;
                        if (drop < bottleneck_drop) {
                            bottleneck_drop = drop;
                            bottleneck_pair = idx;
                        }
                    }
                    const double visits_avg = ladder_size > 0
                        ? static_cast<double>(visits_sum) / static_cast<double>(ladder_size)
                        : 0.0;
                    const auto strict_roundtrip_sum = std::accumulate(
                        strict_round_trip_counts.begin(),
                        strict_round_trip_counts.end(),
                        std::uint64_t{0}
                    );
                    feedback_file << "# ladder_index tempered_value n_up n_dn visits_both f_beta f_drop_to_next\n";
                    feedback_file << "# summary visits_avg=" << visits_avg
                                  << " visits_min=" << visits_min
                                  << " bottleneck_pair=" << bottleneck_pair
                                  << " bottleneck_drop=" << bottleneck_drop
                                  << " strict_roundtrip_sum=" << strict_roundtrip_sum
                                  << '\n';
                    for (int idx = 0; idx < ladder_size; ++idx) {
                        const auto n_up = feedback_up_total[static_cast<size_t>(idx)];
                        const auto n_dn = feedback_dn_total[static_cast<size_t>(idx)];
                        const auto visits_both = n_up + n_dn;
                        feedback_file << idx << '\t'
                                      << pt_ctx.ladder_values[static_cast<size_t>(idx)] << '\t'
                                      << n_up << '\t'
                                      << n_dn << '\t'
                                      << visits_both << '\t'
                                      << f_beta[static_cast<size_t>(idx)] << '\t';
                        if (idx + 1 < ladder_size) {
                            feedback_file << f_drop[static_cast<size_t>(idx)];
                        }
                        feedback_file << '\n';
                    }
                }
            }
        }
    }
#endif

#ifdef PARATORIC_HAS_MPI
    if (pt_ctx.enabled) {
        const size_t observable_count = config.sim_spec.observables.size();
        const size_t ladder_size = static_cast<size_t>(pt_ladder_size);
        const double nan_value = std::numeric_limits<double>::quiet_NaN();

        std::vector<std::vector<std::uint64_t>> binned_count(
            observable_count, std::vector<std::uint64_t>(ladder_size, 0)
        );
        std::vector<std::vector<double>> binned_mean(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_mean_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_binder(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_binder_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_tau(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_re_mean(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<std::uint64_t>> binned_fm_count(
            observable_count, std::vector<std::uint64_t>(ladder_size, 0)
        );
        std::vector<std::vector<double>> binned_fm_re_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_re_binder(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_re_binder_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_re_tau(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_im_mean(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_im_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_im_binder(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_im_binder_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_im_tau(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_abs_mean(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_abs_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_abs_binder(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_abs_binder_std(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );
        std::vector<std::vector<double>> binned_fm_abs_tau(
            observable_count, std::vector<double>(ladder_size, nan_value)
        );

        for (size_t k = 0; k < observable_count; ++k) {
            if (obs_type_vec[k] != "real") {
                continue;
            }

            for (size_t b = 0; b < ladder_size; ++b) {
                const auto& local_values = pt_binned_local_real_samples[k][b];
                const int local_n = static_cast<int>(local_values.size());

                std::vector<int> recv_counts;
                if (pt_ctx.rank == 0) {
                    recv_counts.resize(static_cast<size_t>(pt_ctx.world_size), 0);
                }
                MPI_Gather(
                    &local_n,
                    1,
                    MPI_INT,
                    pt_ctx.rank == 0 ? recv_counts.data() : nullptr,
                    1,
                    MPI_INT,
                    0,
                    MPI_COMM_WORLD
                );

                std::vector<int> displs;
                int total_n = 0;
                if (pt_ctx.rank == 0) {
                    displs.resize(static_cast<size_t>(pt_ctx.world_size), 0);
                    for (int r = 0; r < pt_ctx.world_size; ++r) {
                        displs[static_cast<size_t>(r)] = total_n;
                        total_n += recv_counts[static_cast<size_t>(r)];
                    }
                }

                std::vector<double> gathered_values;
                if (pt_ctx.rank == 0 && total_n > 0) {
                    gathered_values.resize(static_cast<size_t>(total_n), 0.0);
                }
                MPI_Gatherv(
                    local_n > 0 ? local_values.data() : nullptr,
                    local_n,
                    MPI_DOUBLE,
                    (pt_ctx.rank == 0 && total_n > 0) ? gathered_values.data() : nullptr,
                    pt_ctx.rank == 0 ? recv_counts.data() : nullptr,
                    pt_ctx.rank == 0 ? displs.data() : nullptr,
                    MPI_DOUBLE,
                    0,
                    MPI_COMM_WORLD
                );

                if (pt_ctx.rank == 0) {
                    binned_count[k][b] = static_cast<std::uint64_t>(total_n);
                    if (total_n <= 0) {
                        continue;
                    }

                    if (total_n == 1) {
                        const double v = gathered_values.front();
                        binned_mean[k][b] = v;
                        binned_mean_std[k][b] = 0.0;
                        binned_binder[k][b] = 1.0;
                        binned_binder_std[k][b] = 0.0;
                        binned_tau[k][b] = 0.5;
                        continue;
                    }

                    const auto& [mean_v, mean_std_v, binder_v, binder_std_v]
                        = paratoric::statistics::get_bootstrap_statistics(
                            gathered_values, rng, config.sim_spec.N_resamples
                        );
                    binned_mean[k][b] = mean_v;
                    binned_mean_std[k][b] = mean_std_v;
                    binned_binder[k][b] = binder_v;
                    binned_binder_std[k][b] = binder_std_v;
                    binned_tau[k][b] = paratoric::statistics::get_autocorrelation_time(
                        paratoric::statistics::get_autocorrelation_function(gathered_values)
                    );
                }
            }
        }

        auto reduce_binned_component = [&](const std::vector<std::vector<std::vector<double>>>& local_samples,
                                           std::vector<std::vector<double>>& out_mean,
                                           std::vector<std::vector<double>>& out_std,
                                           std::vector<std::vector<double>>& out_binder,
                                           std::vector<std::vector<double>>& out_binder_std,
                                           std::vector<std::vector<double>>& out_tau) {
            for (size_t k = 0; k < observable_count; ++k) {
                if (obs_type_vec[k] != "fredenhagen_marcu") {
                    continue;
                }
                for (size_t b = 0; b < ladder_size; ++b) {
                    const auto& local_values = local_samples[k][b];
                    const int local_n = static_cast<int>(local_values.size());

                    std::vector<int> recv_counts;
                    if (pt_ctx.rank == 0) {
                        recv_counts.resize(static_cast<size_t>(pt_ctx.world_size), 0);
                    }
                    MPI_Gather(
                        &local_n,
                        1,
                        MPI_INT,
                        pt_ctx.rank == 0 ? recv_counts.data() : nullptr,
                        1,
                        MPI_INT,
                        0,
                        MPI_COMM_WORLD
                    );

                    std::vector<int> displs;
                    int total_n = 0;
                    if (pt_ctx.rank == 0) {
                        displs.resize(static_cast<size_t>(pt_ctx.world_size), 0);
                        for (int r = 0; r < pt_ctx.world_size; ++r) {
                            displs[static_cast<size_t>(r)] = total_n;
                            total_n += recv_counts[static_cast<size_t>(r)];
                        }
                    }

                    std::vector<double> gathered_values;
                    if (pt_ctx.rank == 0 && total_n > 0) {
                        gathered_values.resize(static_cast<size_t>(total_n), 0.0);
                    }
                    MPI_Gatherv(
                        local_n > 0 ? local_values.data() : nullptr,
                        local_n,
                        MPI_DOUBLE,
                        (pt_ctx.rank == 0 && total_n > 0) ? gathered_values.data() : nullptr,
                        pt_ctx.rank == 0 ? recv_counts.data() : nullptr,
                        pt_ctx.rank == 0 ? displs.data() : nullptr,
                        MPI_DOUBLE,
                        0,
                        MPI_COMM_WORLD
                    );

                    if (pt_ctx.rank == 0) {
                        binned_fm_count[k][b] = static_cast<std::uint64_t>(total_n);
                        if (total_n <= 0) {
                            continue;
                        }

                        if (total_n == 1) {
                            const double v = gathered_values.front();
                            out_mean[k][b] = v;
                            out_std[k][b] = 0.0;
                            out_binder[k][b] = 1.0;
                            out_binder_std[k][b] = 0.0;
                            out_tau[k][b] = 0.5;
                            continue;
                        }

                        const auto& [mean_v, mean_std_v, binder_v, binder_std_v]
                            = paratoric::statistics::get_bootstrap_statistics(
                                gathered_values, rng, config.sim_spec.N_resamples
                            );
                        out_mean[k][b] = mean_v;
                        out_std[k][b] = mean_std_v;
                        out_binder[k][b] = binder_v;
                        out_binder_std[k][b] = binder_std_v;
                        out_tau[k][b] = paratoric::statistics::get_autocorrelation_time(
                            paratoric::statistics::get_autocorrelation_function(gathered_values)
                        );
                    }
                }
            }
        };

        reduce_binned_component(
            pt_binned_local_fm_real_samples,
            binned_fm_re_mean,
            binned_fm_re_std,
            binned_fm_re_binder,
            binned_fm_re_binder_std,
            binned_fm_re_tau
        );
        reduce_binned_component(
            pt_binned_local_fm_imag_samples,
            binned_fm_im_mean,
            binned_fm_im_std,
            binned_fm_im_binder,
            binned_fm_im_binder_std,
            binned_fm_im_tau
        );
        reduce_binned_component(
            pt_binned_local_fm_abs_samples,
            binned_fm_abs_mean,
            binned_fm_abs_std,
            binned_fm_abs_binder,
            binned_fm_abs_binder_std,
            binned_fm_abs_tau
        );

        if (pt_ctx.rank == 0) {
            std::ofstream binned_file(
                pt_detail::binned_observable_file_path(config),
                std::ios::out | std::ios::trunc
            );
            if (binned_file) {
                binned_file << "# PT merged observable statistics by tempered ladder index\n";
                binned_file << "# columns: observable ladder_index tempered_value count mean mean_error binder binder_error tau_int\n";
                for (size_t k = 0; k < observable_count; ++k) {
                    if (obs_type_vec[k] != "real") {
                        continue;
                    }
                    for (size_t b = 0; b < ladder_size; ++b) {
                        const double tempered_value = !pt_ctx.ladder_values.empty()
                            ? pt_ctx.ladder_values[b]
                            : config.pt_spec.ladder_min;
                        binned_file << config.sim_spec.observables[k]
                                    << '\t' << b
                                    << '\t' << tempered_value
                                    << '\t' << binned_count[k][b]
                                    << '\t' << binned_mean[k][b]
                                    << '\t' << binned_mean_std[k][b]
                                    << '\t' << binned_binder[k][b]
                                    << '\t' << binned_binder_std[k][b]
                                    << '\t' << binned_tau[k][b]
                                    << '\n';
                    }
                }
                for (size_t k = 0; k < observable_count; ++k) {
                    if (obs_type_vec[k] != "fredenhagen_marcu") {
                        continue;
                    }
                    for (size_t b = 0; b < ladder_size; ++b) {
                        const double tempered_value = !pt_ctx.ladder_values.empty()
                            ? pt_ctx.ladder_values[b]
                            : config.pt_spec.ladder_min;
                        auto write_fm_component = [&](const std::string& suffix,
                                                      const std::vector<std::vector<double>>& means,
                                                      const std::vector<std::vector<double>>& stds,
                                                      const std::vector<std::vector<double>>& binders,
                                                      const std::vector<std::vector<double>>& binder_stds,
                                                      const std::vector<std::vector<double>>& taus) {
                            binned_file << (config.sim_spec.observables[k] + suffix)
                                        << '\t' << b
                                        << '\t' << tempered_value
                                        << '\t' << binned_fm_count[k][b]
                                        << '\t' << means[k][b]
                                        << '\t' << stds[k][b]
                                        << '\t' << binders[k][b]
                                        << '\t' << binder_stds[k][b]
                                        << '\t' << taus[k][b]
                                        << '\n';
                        };
                        write_fm_component(
                            "_re",
                            binned_fm_re_mean,
                            binned_fm_re_std,
                            binned_fm_re_binder,
                            binned_fm_re_binder_std,
                            binned_fm_re_tau
                        );
                        write_fm_component(
                            "_im",
                            binned_fm_im_mean,
                            binned_fm_im_std,
                            binned_fm_im_binder,
                            binned_fm_im_binder_std,
                            binned_fm_im_tau
                        );
                        write_fm_component(
                            "_abs",
                            binned_fm_abs_mean,
                            binned_fm_abs_std,
                            binned_fm_abs_binder,
                            binned_fm_abs_binder_std,
                            binned_fm_abs_tau
                        );
                    }
                }
            }
        }
    }
#endif

    if (config.out_spec.save_snapshots) {
        lat.write_graph("snapshots", config.out_spec.path_out);
    }

    for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
        const auto& obs_real = stats_real_samples[k];
        const auto& obs_imag = stats_imag_samples[k];

        if (obs_type_vec[k] == "real") {
            const auto& [observable_mean, observable_std, binder_mean, binder_std] 
            = paratoric::statistics::get_bootstrap_statistics(obs_real, rng, config.sim_spec.N_resamples);

            observable_mean_vector[k] = observable_mean;
            observable_std_vector[k] = observable_std;
            binder_mean_vector[k] = binder_mean;
            binder_std_vector[k] = binder_std;
            observable_autocorrelation_time_vector[k] 
            = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
        } else if (obs_type_vec[k] == "fredenhagen_marcu") {
            const auto& [observable_mean, observable_std, binder_mean, binder_std] 
            = paratoric::statistics::get_bootstrap_statistics_fm(obs_real, obs_imag, rng, config.sim_spec.N_resamples);

            observable_mean_vector[k] = observable_mean;
            observable_std_vector[k] = observable_std;
            binder_mean_vector[k] = binder_mean;
            binder_std_vector[k] = binder_std;
            observable_autocorrelation_time_vector[k] 
            = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
        } else if (obs_type_vec[k] == "susceptibility") {
            //TODO fix this, every observable should just define their susceptibility function
            if ((config.sim_spec.observables[k] == "sigma_z_static_susceptibility" && Basis == 'x')) {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_susceptibility(
                    obs_real, config.lat_spec.beta, lmbda_runtime, 
                    lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            } else if (config.sim_spec.observables[k] == "sigma_x_static_susceptibility" && Basis == 'z') {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_susceptibility(
                    obs_real, config.lat_spec.beta, h_runtime, 
                    lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            } else if ((config.sim_spec.observables[k] == "sigma_z_dynamical_susceptibility" && Basis == 'x')
                        || (config.sim_spec.observables[k] == "sigma_x_dynamical_susceptibility" && Basis == 'z')) {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::bootstrap_offdiag_dynamical_susceptibility(
                    obs_real, obs_imag, lat.get_edge_count(), rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            } else {
                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics_susceptibility(
                    obs_real, obs_imag, rng, config.sim_spec.N_resamples
                );
                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            }
        }
    }

    integrated_pot_energy_check = total_integrated_pot_energy(
        lat, h_runtime, mu_runtime, J_runtime, lmbda_runtime
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    return Result{
        .series=observable_vector, 
        .acc_ratio=acc_ratio_vector,
        .mean=observable_mean_vector, 
        .mean_std=observable_std_vector, 
        .binder=binder_mean_vector, 
        .binder_std=binder_std_vector, 
        .tau_int=observable_autocorrelation_time_vector
    };                                       
}

template<char Basis>
requires ValidBasis<Basis>
Result ExtendedToricCodeQMC<Basis>::get_hysteresis(
    const Config& config
) {

    if constexpr (Basis != 'x' && Basis != 'z') {
        throw std::invalid_argument("Basis must be either \"x\" or \"z\".");
    }

    if (Basis != config.lat_spec.basis) {
        throw std::invalid_argument("Template parameter basis and config.lat_spec basis must match.");
    }
    
    double h{}, lmbda{};
    
    if (!config.param_spec.h_hys.empty()) {
        h = config.param_spec.h_hys.front();
    } else {
        throw std::invalid_argument("h_hys must be non-empty.");
    }

    if (!config.param_spec.lmbda_hys.empty()) {
        lmbda = config.param_spec.lmbda_hys.front();
    } else {
        throw std::invalid_argument("lmbda_hys must be non-empty.");
    }

    if constexpr (Basis == 'x') {
        if (config.param_spec.J < 0) {
            throw std::invalid_argument("J must be non-negative in the x-basis.");
        } else if (lmbda < 0) {
            throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
        }
    } else if constexpr (Basis == 'z') {
        if (config.param_spec.mu < 0) {
            throw std::invalid_argument("mu must be non-negative in the z-basis.");
        } else if (h < 0) {
            throw std::invalid_argument("h must be non-negative in the z-basis.");
        }
    }

    if (config.sim_spec.seed != 0) rng->set_seed(config.sim_spec.seed);

    auto obs_func_vec = get_obs_func_vec(config.sim_spec.observables);
    auto obs_type_vec = get_obs_type_vec(config.sim_spec.observables);
    
    // Vector to store observable results for all snapshots for all parameters
    std::vector<std::vector<std::vector<std::variant< std::complex<double>, double>>>> hys_vector;
    std::vector<std::vector<double>> hys_mean,
                                     hys_mean_std,
                                     hys_binder,
                                     hys_binder_std,
                                     hys_autocorrelation_time;

    // Initialize Lattice
    auto lat = Lattice(config.lat_spec, rng);
    
    double integrated_pot_energy = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );
    double acc_ratio = 1.;

    // Thermalization 
    for (int i = 0; i < config.sim_spec.N_thermalization; ++i) {
        metropolis_step(
            lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
            config.param_spec.h, config.param_spec.mu, config.param_spec.J, 
            config.param_spec.lmbda
        );
    }

    double integrated_pot_energy_check = total_integrated_pot_energy(
        lat, config.param_spec.h, config.param_spec.mu, config.param_spec.J, config.param_spec.lmbda
    );

    if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
        throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
    }

    int total_metropolis_step_count = 0;
    int reset_potential_energy_count = static_cast<int>(lat.get_edge_count()*100000);

    for (size_t n = 0; n < std::min( config.param_spec.h_hys.size(), std::min(config.param_spec.lmbda_hys.size(), config.out_spec.paths_out.size()) ); n++) {
        // Vector to store observable results for all snapshots
        std::vector<std::vector<std::variant< std::complex<double>, double>>> observable_vector;
        std::vector<double> observable_mean_vector(config.sim_spec.observables.size(), 0.), 
                            observable_std_vector(config.sim_spec.observables.size(), 0.), 
                            binder_mean_vector(config.sim_spec.observables.size(), 0.), 
                            binder_std_vector(config.sim_spec.observables.size(), 0.), 
                            observable_autocorrelation_time_vector(config.sim_spec.observables.size(), 0.);
        std::vector<std::variant< std::complex<double>, double>> obs_temp;
        for (const auto& obs_func : config.sim_spec.observables) {
            UNUSED(obs_func);
            observable_vector.emplace_back( obs_temp );
        } 
        auto path_out = config.out_spec.paths_out [ n ];
        h = config.param_spec.h_hys[ n ];
        lmbda = config.param_spec.lmbda_hys[ n ];

        lat.init_potential_energy();
        lat.rotate_imag_time();
        integrated_pot_energy = total_integrated_pot_energy(
            lat, h, config.param_spec.mu, 
            config.param_spec.J, lmbda
        );

        if constexpr (Basis == 'x') {
            if (config.param_spec.J < 0) {
                throw std::invalid_argument("J must be non-negative in the x-basis.");
            } else if (lmbda < 0) {
                throw std::invalid_argument("lmbda must be non-negative in the x-basis.");
            }
        } else if constexpr (Basis == 'z') {
            if (config.param_spec.mu < 0) {
                throw std::invalid_argument("mu must be non-negative in the z-basis.");
            } else if (h < 0) {
                throw std::invalid_argument("h must be non-negative in the z-basis.");
            }
        }
        
        int N_rethermalization = static_cast<int>(config.sim_spec.N_thermalization/4);

        for (int t = 0; t < N_rethermalization; ++t) {
            ++total_metropolis_step_count;
            metropolis_step(
                lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, h, 
                config.param_spec.mu, config.param_spec.J, lmbda
            );
            if (total_metropolis_step_count % reset_potential_energy_count == 0) {
                // avoid accumulation of small numerical errors leading to bias
                lat.init_potential_energy();
                lat.rotate_imag_time();
                integrated_pot_energy = total_integrated_pot_energy(
                    lat, h, config.param_spec.mu, 
                    config.param_spec.J, lmbda
                );
            }
        }

        for (int i = 0; i < config.sim_spec.N_samples; ++i) {
            for (int j = 0; j < config.sim_spec.N_between_samples; ++j) {
                ++total_metropolis_step_count;
                metropolis_step(
                    lat, integrated_pot_energy, acc_ratio, config.lat_spec.beta, 
                    h, config.param_spec.mu, config.param_spec.J, lmbda
                );
                if (total_metropolis_step_count % reset_potential_energy_count == 0) {
                    // avoid accumulation of small numerical errors leading to bias
                    lat.init_potential_energy();
                    lat.rotate_imag_time();
                    integrated_pot_energy = total_integrated_pot_energy(
                        lat, h, config.param_spec.mu, 
                        config.param_spec.J, lmbda
                    );
                }
            }

            bool rotated_for_susceptibility = false;
            for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
                if (!rotated_for_susceptibility && obs_type_vec[k] == "susceptibility") {
                    lat.rotate_imag_time();
                    rotated_for_susceptibility = true;
                }
                observable_vector[k].emplace_back(obs_func_vec[k](lat, h, lmbda, config.param_spec.mu, config.param_spec.J));
            }

            if (config.out_spec.save_snapshots)
                lat.update_spin_string();
        }

        hys_vector.emplace_back( observable_vector );

        if (config.out_spec.save_snapshots) {
            lat.write_graph("snapshots", config.out_spec.path_out);
        }

        for (size_t k = 0; k < config.sim_spec.observables.size(); k++) {
            if (obs_type_vec[k] == "real") {
                std::vector<double> obs_real;
                const auto& series = observable_vector[k];
                obs_real.reserve(series.size());
                for (auto const& x : series) {
                // if you know it's always a double here, use get<>
                obs_real.emplace_back(std::get<double>(x));
                }

                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics(obs_real, rng, config.sim_spec.N_resamples);

                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            } else if (obs_type_vec[k] == "fredenhagen_marcu") {
                const auto& series = observable_vector[k];
                const size_t N = series.size();
                std::vector<double> obs_real, obs_imag;
                obs_real.reserve(N);
                obs_imag.reserve(N);

                for (auto const& v : series) {
                    if (auto p = std::get_if<std::complex<double>>(&v)) {
                        // v holds a complex<double>
                        obs_real.push_back(p->real());
                        obs_imag.push_back(p->imag());
                    }
                    else {
                        // v must hold a double
                        double d = std::get<double>(v);
                        obs_real.push_back(d);
                        obs_imag.push_back(0.0);
                    }
                }

                const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                = paratoric::statistics::get_bootstrap_statistics_fm(obs_real, obs_imag, rng, config.sim_spec.N_resamples);

                observable_mean_vector[k] = observable_mean;
                observable_std_vector[k] = observable_std;
                binder_mean_vector[k] = binder_mean;
                binder_std_vector[k] = binder_std;
                observable_autocorrelation_time_vector[k] 
                = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
            } else if (obs_type_vec[k] == "susceptibility") {
                const auto& series = observable_vector[k];
                const size_t N = series.size();
                std::vector<double> obs_real, obs_imag;
                obs_real.reserve(N);
                obs_imag.reserve(N);

                for (auto const& v : series) {
                    if (auto p = std::get_if<std::complex<double>>(&v)) {
                        // v holds a complex<double>
                        obs_real.push_back(p->real());
                        obs_imag.push_back(p->imag());
                    }
                    else {
                        // v must hold a double
                        double d = std::get<double>(v);
                        obs_real.push_back(d);
                        obs_imag.push_back(0.0);
                    }
                }

                if ((config.sim_spec.observables[k] == "sigma_z_static_susceptibility" && Basis == 'x')) {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_susceptibility(
                        obs_real, config.lat_spec.beta, lmbda, 
                        lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
                } else if (config.sim_spec.observables[k] == "sigma_x_static_susceptibility" && Basis == 'z') {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_susceptibility(
                        obs_real, config.lat_spec.beta, h, 
                        lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
                } else if ((config.sim_spec.observables[k] == "sigma_z_dynamical_susceptibility" && Basis == 'x')
                            || (config.sim_spec.observables[k] == "sigma_x_dynamical_susceptibility" && Basis == 'z')) {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::bootstrap_offdiag_dynamical_susceptibility(
                        obs_real, obs_imag, lat.get_edge_count(), rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
                } else {
                    const auto& [observable_mean, observable_std, binder_mean, binder_std] 
                    = paratoric::statistics::get_bootstrap_statistics_susceptibility(
                        obs_real, obs_imag, rng, config.sim_spec.N_resamples
                    );
                    observable_mean_vector[k] = observable_mean;
                    observable_std_vector[k] = observable_std;
                    binder_mean_vector[k] = binder_mean;
                    binder_std_vector[k] = binder_std;
                    observable_autocorrelation_time_vector[k] 
                    = paratoric::statistics::get_autocorrelation_time(paratoric::statistics::get_autocorrelation_function(obs_real));
                }
            } 
        }

        hys_mean.emplace_back(observable_mean_vector);
        hys_mean_std.emplace_back(observable_std_vector);
        hys_binder.emplace_back(binder_mean_vector);
        hys_binder_std.emplace_back(binder_std_vector);
        hys_autocorrelation_time.emplace_back(observable_autocorrelation_time_vector);

        integrated_pot_energy_check = total_integrated_pot_energy(
            lat, h, config.param_spec.mu, config.param_spec.J, lmbda
        );

        if (!almost_equal(integrated_pot_energy, integrated_pot_energy_check, 1e-5, 1e-13)) {
            throw std::runtime_error(std::format("Integrated potential energy mismatch. {} does not match {}.", integrated_pot_energy, integrated_pot_energy_check));
        }
    }

    return Result{
        .series_hys=hys_vector, 
        .mean_hys=hys_mean, 
        .mean_std_hys=hys_mean_std, 
        .binder_hys=hys_binder, 
        .binder_std_hys=hys_binder_std, 
        .tau_int_hys=hys_autocorrelation_time
    };
}

} // namespace paratoric 
