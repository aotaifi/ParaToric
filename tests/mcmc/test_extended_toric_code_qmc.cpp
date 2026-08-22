// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#define BOOST_TEST_MODULE TestExtendedToricCodeQMC

#include "mcmc/extended_toric_code_qmc.hpp"
#include "paratoric/types/types.hpp"

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <string>

namespace paratoric {

// High field parameter regime outsie of the topological phase
double h = 1.1;  
double h_therm = 0.9;
std::vector<double> h_hys {0.9, 1., 1.1};
double lmbda = 0.8; 
double lmbda_therm = 1.0;
std::vector<double> lmbda_hys {1.0, 1.1, 1.2};
double mu = 0.9; 
double J = 1.1; 

// Low field parameter regime inside of the topological phase
double h_top = 0.1;  
double h_therm_top = 0.0;
std::vector<double> h_hys_top {0.9, 1., 1.1};
double lmbda_top = 0.0; 
double lmbda_therm_top = 0.2;
std::vector<double> lmbda_hys_top {0.05, 0.1, 0.15};
double mu_top = 0.9; 
double J_top = 1.1; 

class RotationRecordingLattice : public Lattice {
public:
    inline static std::uint64_t rotation_index = 0;
    inline static std::vector<std::uint64_t> percolation_probability_indices{};
    inline static std::vector<std::uint64_t> percolation_strength_indices{};
    inline static std::vector<std::uint64_t> plaquette_percolation_probability_indices{};
    inline static std::vector<std::uint64_t> plaquette_percolation_strength_indices{};

    RotationRecordingLattice(const LatSpec& lat_spec, std::shared_ptr<RNG> rng = nullptr)
        : Lattice(lat_spec, std::move(rng)) {}

    void rotate_imag_time() {
        ++rotation_index;
        Lattice::rotate_imag_time();
    }

    double percolation_probability() {
        percolation_probability_indices.emplace_back(rotation_index);
        return Lattice::percolation_probability();
    }

    double percolation_strength() {
        percolation_strength_indices.emplace_back(rotation_index);
        return Lattice::percolation_strength();
    }

    double plaquette_percolation_probability() {
        plaquette_percolation_probability_indices.emplace_back(rotation_index);
        return Lattice::plaquette_percolation_probability();
    }

    double plaquette_percolation_strength() {
        plaquette_percolation_strength_indices.emplace_back(rotation_index);
        return Lattice::plaquette_percolation_strength();
    }

    static void reset_rotation_audit() {
        rotation_index = 0;
        percolation_probability_indices.clear();
        percolation_strength_indices.clear();
        plaquette_percolation_probability_indices.clear();
        plaquette_percolation_strength_indices.clear();
    }
};

// Negative J parameter set
double h_neg = 0.1;  
double h_therm_neg = 0.0;
std::vector<double> h_hys_neg {0.9, 1., 1.1};
double lmbda_neg = 0.0; 
double lmbda_therm_neg = 0.2;
std::vector<double> lmbda_hys_neg {0.05, 0.1, 0.15};
double mu_neg = 0.9; 
double J_neg = -1.1; 

BOOST_AUTO_TEST_CASE(get_thermalization_test_1) {
    auto mc = ExtendedToricCodeQMC<'x'>();;
    LatSpec lat_spec = {
        'x', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda
    };
    SimSpec sim_spec = {
        .N_thermalization = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_thermalization(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(tau_averaged_observables_are_registered_in_both_bases) {
    ExtendedToricCodeQMC<'x'> mc_x;
    ExtendedToricCodeQMC<'z'> mc_z;
    const std::vector<std::string> observables{"plaquette_z_tau_avg", "delta_tau_avg"};
    const auto types_x = mc_x.get_obs_type_vec(observables);
    const auto types_z = mc_z.get_obs_type_vec(observables);

    BOOST_REQUIRE_EQUAL(types_x.size(), 2U);
    BOOST_REQUIRE_EQUAL(types_z.size(), 2U);
    BOOST_CHECK_EQUAL(types_x[0], "real");
    BOOST_CHECK_EQUAL(types_x[1], "real");
    BOOST_CHECK_EQUAL(types_z[0], "real");
    BOOST_CHECK_EQUAL(types_z[1], "real");
}

BOOST_AUTO_TEST_CASE(get_thermalization_test_2) {
    auto mc = ExtendedToricCodeQMC<'x'>();;
    LatSpec lat_spec = {
        'x', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top
    };
    SimSpec sim_spec = {
        .N_thermalization = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_thermalization(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_thermalization_test_3) {
    auto mc = ExtendedToricCodeQMC<'z'>();;
    LatSpec lat_spec = {
        'z', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_neg,
        .h = h_neg,
        .J = J_neg,
        .lmbda = lmbda_neg
    };
    SimSpec sim_spec = {
        .N_thermalization = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_thermalization(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_1) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "square", // lattice type
        6, // system size
        1., // beta --> Test global updates
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(percolation_observables_share_one_post_rotation_slice_per_sample) {
    RotationRecordingLattice::reset_rotation_audit();
    auto mc = ExtendedToricCodeQMC<'x', RotationRecordingLattice>();
    LatSpec lat_spec = {
        'x', "square", 4, 4., "periodic", 1
    };
    ParamSpec param_spec = {
        .mu = 0.9, .h = 0.4, .J = 1.1, .lmbda = 0.3
    };
    SimSpec sim_spec = {
        .N_samples = 3,
        .N_thermalization = 2,
        .N_between_samples = 2,
        .N_resamples = 8,
        .seed = 20260712,
        .observables = {
            "percolation_probability", "percolation_strength",
            "plaquette_percolation_probability", "plaquette_percolation_strength"
        }
    };

    mc.get_sample(Config{sim_spec, param_spec, lat_spec, OutSpec{}});

    const std::array<std::vector<std::uint64_t>*, 4> indices = {
        &RotationRecordingLattice::percolation_probability_indices,
        &RotationRecordingLattice::percolation_strength_indices,
        &RotationRecordingLattice::plaquette_percolation_probability_indices,
        &RotationRecordingLattice::plaquette_percolation_strength_indices
    };
    for (const auto* index : indices) {
        BOOST_REQUIRE_EQUAL(index->size(), static_cast<size_t>(sim_spec.N_samples));
    }
    for (size_t sample = 0; sample < static_cast<size_t>(sim_spec.N_samples); ++sample) {
        const auto expected_index = static_cast<std::uint64_t>(sample + 1);
        for (const auto* index : indices) {
            BOOST_CHECK_EQUAL(index->at(sample), expected_index);
        }
    }
}

BOOST_AUTO_TEST_CASE(get_sample_test_2) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_3) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = true,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_4) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "square", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = true,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_5) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "triangular", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_6) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "triangular", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_7) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "triangular", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_8) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "triangular", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    };
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_9) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "honeycomb", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    }; 
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    // Honeycomb lattice does not yet support fredenhagen_marcu in x-basis and should throw std::invalid_argument in Debug mode
    BOOST_WARN_THROW( auto result = mc.get_sample(Config{sim_spec, param_spec, lat_spec, out_spec});,
        std::invalid_argument
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_10) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "honeycomb", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    }; 
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    // Honeycomb lattice does not yet support fredenhagen_marcu in x-basis and should throw std::invalid_argument in Debug mode
    BOOST_WARN_THROW( auto result = mc.get_sample(Config{sim_spec, param_spec, lat_spec, out_spec});,
        std::invalid_argument
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_11) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "honeycomb", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_12) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "honeycomb", // lattice type
        6, // system size
        10., // beta
        "periodic", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_13) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_14) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_15) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .h = h,
        .J = J,
        .lmbda = lmbda,
        .h_therm = h_therm,          
        .lmbda_therm = lmbda_therm   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_sample_test_16) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .h = h_top,
        .J = J_top,
        .lmbda = lmbda_top,
        .h_therm = h_therm_top,          
        .lmbda_therm = lmbda_therm_top   
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .custom_therm = false,
        .seed = 0,
        .observables = std::vector<std::string>{
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_sample(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_hysteresis_test_1) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .J = J,
        .h_hys = h_hys,
        .lmbda_hys = lmbda_hys
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .paths_out = std::vector<std::filesystem::path> {"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_hysteresis(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_hysteresis_test_2) {
    auto mc = ExtendedToricCodeQMC<'x'>();
    LatSpec lat_spec = {
        'x', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .J = J_top,
        .h_hys = h_hys_top,
        .lmbda_hys = lmbda_hys_top
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .paths_out = std::vector<std::filesystem::path> {"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_hysteresis(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_hysteresis_test_3) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu,
        .J = J,
        .h_hys = h_hys,
        .lmbda_hys = lmbda_hys
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .paths_out = std::vector<std::filesystem::path> {"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_hysteresis(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(get_hysteresis_test_4) {
    auto mc = ExtendedToricCodeQMC<'z'>();
    LatSpec lat_spec = {
        'z', // basis
        "cubic", // lattice type
        6, // system size
        10., // beta
        "open", // boundaries
        1 // default spin
    }; 
    ParamSpec param_spec = {
        .mu = mu_top,
        .J = J_top,
        .h_hys = h_hys_top,
        .lmbda_hys = lmbda_hys_top
    };
    SimSpec sim_spec = {
        .N_samples = 50,
        .N_thermalization = 50,
        .N_between_samples = 50,
        .N_resamples = 100,
        .seed = 0,
        .observables = std::vector<std::string> { 
            "percolation_strength", "percolation_probability", "plaquette_percolation_strength",
            "plaquette_percolation_probability", "largest_cluster", "largest_plaquette_cluster", 
            "string_number", "energy", "energy_h", "energy_mu", "energy_J", "energy_lmbda", 
            "sigma_x", "sigma_z", "star_x", "plaquette_z", "staggered_imaginary_times", 
            "delta", "anyon_count", "anyon_density", "fredenhagen_marcu", 
            "sigma_x_static_susceptibility", "sigma_x_dynamical_susceptibility",
            "sigma_z_static_susceptibility", "sigma_z_dynamical_susceptibility"
        }
    };
    OutSpec out_spec = {
        .paths_out = std::vector<std::filesystem::path> {"./"},
        .save_snapshots = 0
    };

    auto result = mc.get_hysteresis(
        Config{sim_spec, param_spec, lat_spec, out_spec}
    );
}

BOOST_AUTO_TEST_CASE(flat_square_sample_hits_reset_path) {
    auto mc = ExtendedToricCodeQMC<'x', FlatSquareLattice>();
    LatSpec lat_spec = {
        'x',
        "square",
        2,
        4.,
        "periodic",
        1,
        "flat_square"
    };
    ParamSpec param_spec = {
        .mu = 1.0,
        .h = 0.3,
        .J = 1.0,
        .lmbda = 0.2
    };
    SimSpec sim_spec = {
        .N_samples = 2,
        .N_thermalization = 1,
        .N_between_samples = 400000,
        .N_resamples = 10,
        .custom_therm = false,
        .seed = 1234,
        .observables = std::vector<std::string>{
            "energy", "anyon_count", "sigma_x", "star_x", "plaquette_z"
        }
    };
    OutSpec out_spec = {
        .path_out = std::filesystem::path{"./"},
        .save_snapshots = false,
        .full_time_series = false
    };

    auto result = mc.get_sample(Config{sim_spec, param_spec, lat_spec, out_spec});
    BOOST_CHECK_EQUAL(result.production_acceptance.attempted, 800000);
    BOOST_CHECK_EQUAL(result.mean.size(), sim_spec.observables.size());
}

BOOST_AUTO_TEST_CASE(boost_sample_checkpoint_restart_is_bit_faithful) {
    LatSpec lat_spec = {
        'z',
        "square",
        4,
        3.,
        "periodic",
        1,
        "boost"
    };
    ParamSpec param_spec = {
        .mu = 0.9,
        .h = 0.7,
        .J = 1.1,
        .lmbda = 0.6
    };
    SimSpec full_sim_spec = {
        .N_samples = 10,
        .N_thermalization = 12,
        .N_between_samples = 8,
        .N_resamples = 8,
        .custom_therm = false,
        .seed = 424242,
        .observables = std::vector<std::string>{"energy", "string_number", "anyon_count"}
    };
    OutSpec base_out_spec = {
        .path_out = std::filesystem::temp_directory_path(),
        .save_snapshots = false,
        .full_time_series = true
    };

    auto full = ExtendedToricCodeQMC<'z'>().get_sample(
        Config{full_sim_spec, param_spec, lat_spec, base_out_spec}
    );

    const auto checkpoint_path =
        std::filesystem::temp_directory_path() / "paratoric_checkpoint_restart_test.bin";
    std::filesystem::remove(checkpoint_path);

    SimSpec prefix_sim_spec = full_sim_spec;
    prefix_sim_spec.N_samples = 5;
    OutSpec checkpoint_out_spec = base_out_spec;
    checkpoint_out_spec.checkpoint_path = checkpoint_path;
    checkpoint_out_spec.checkpoint_every_samples = 5;
    auto prefix = ExtendedToricCodeQMC<'z'>().get_sample(
        Config{prefix_sim_spec, param_spec, lat_spec, checkpoint_out_spec}
    );
    BOOST_CHECK(std::filesystem::exists(checkpoint_path));
    BOOST_CHECK_EQUAL(prefix.series.front().size(), 5);

    OutSpec restart_out_spec = base_out_spec;
    restart_out_spec.restart_from = checkpoint_path;
    auto resumed = ExtendedToricCodeQMC<'z'>().get_sample(
        Config{full_sim_spec, param_spec, lat_spec, restart_out_spec}
    );

    BOOST_REQUIRE_EQUAL(resumed.series.size(), full.series.size());
    for (size_t obs = 0; obs < full.series.size(); ++obs) {
        BOOST_REQUIRE_EQUAL(resumed.series[obs].size(), full.series[obs].size());
        for (size_t sample = 0; sample < full.series[obs].size(); ++sample) {
            BOOST_CHECK(resumed.series[obs][sample] == full.series[obs][sample]);
        }
    }
    BOOST_CHECK_EQUAL(resumed.production_acceptance.attempted, full.production_acceptance.attempted);
    BOOST_CHECK_EQUAL(resumed.production_acceptance.accepted, full.production_acceptance.accepted);
    BOOST_CHECK(resumed.production_acceptance.block_attempted == full.production_acceptance.block_attempted);

    std::filesystem::remove(checkpoint_path);
}

// M9 (Feynman): extends the checkpoint restart coverage above to the new PT ladder
// state fields (pt_ladder_index, runtime_h/mu/J/lmbda), added in checkpoint format
// version 3. Cannot go through get_sample() with pt_spec.enabled=true here -- PT
// requires an MPI-enabled build, and this test binary is non-MPI -- so this exercises
// the write/read serialization directly via checkpoint_pt_state_round_trip_debug.
// Real multi-rank swap dynamics are covered separately, under mpirun, in
// m4_smoke/m9_pt_checkpoint_restart.cpp. Real pre-M9-vs-post-M9 file compatibility
// (requirement 7) is proven empirically in m4_smoke/m9_backward_compat_check.cpp
// against a checkpoint written by an actual pre-M9 binary -- not asserted here from
// reading the code.
BOOST_AUTO_TEST_CASE(boost_sample_checkpoint_pt_state_round_trips) {
    paratoric::LatSpec lat_spec = {
        'z', "square", 4, 3., "periodic", 1, "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 0.9, .h = 0.7, .J = 1.1, .lmbda = 0.6};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 1, .N_between_samples = 1,
        .N_resamples = 1, .seed = 13579, .observables = {"energy"}
    };
    paratoric::Config config{
        sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}
    };

    const auto checkpoint_path =
        std::filesystem::temp_directory_path() / "paratoric_pt_checkpoint_state_test.bin";
    std::filesystem::remove(checkpoint_path);

    // Values distinct from any launch default (mu=0.9 etc above), so a fallback to
    // launch values would be visibly wrong, not accidentally right.
    const int written_ladder_index = 2;
    const double written_h = 0.3141592;
    const double written_mu = 1.654321;
    const double written_J = 0.271828;
    const double written_lmbda = 0.987654;

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const auto result = mc.checkpoint_pt_state_round_trip_debug(
        config, checkpoint_path, written_ladder_index, written_h, written_mu, written_J, written_lmbda
    );

    BOOST_CHECK(result.has_pt_state);
    BOOST_CHECK_EQUAL(result.pt_ladder_index, written_ladder_index);
    BOOST_CHECK_EQUAL(result.runtime_h, written_h);
    BOOST_CHECK_EQUAL(result.runtime_mu, written_mu);
    BOOST_CHECK_EQUAL(result.runtime_J, written_J);
    BOOST_CHECK_EQUAL(result.runtime_lmbda, written_lmbda);

    std::filesystem::remove(checkpoint_path);
}

BOOST_AUTO_TEST_CASE(boost_pt_swap_weight_matches_bruteforce_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z',
        .lattice_type = "square",
        .system_size = 8,
        .beta = 8.0,
        .boundaries = "periodic",
        .default_spin = 1,
        .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {
        .mu = 1.0,
        .h = 0.25,
        .J = 1.0,
        .lmbda = 0.25
    };
    paratoric::SimSpec sim_spec = {
        .N_samples = 8,
        .N_thermalization = 16,
        .N_between_samples = 8,
        .N_resamples = 8,
        .seed = 2468,
        .observables = {"energy", "sigma_x", "star_x"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.22, 0.30},
        .swap_period = 4,
        .validate_swap_weight = true
    };

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const double max_deviation = mc.validate_parallel_tempering_swap_weight_debug(
        paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec}
    );
    BOOST_TEST_MESSAGE("L=8 PT cached-vs-bruteforce max log-weight deviation: " << max_deviation);
    BOOST_TEST(max_deviation < 1e-8);
}

BOOST_AUTO_TEST_CASE(pt_ladder_index_permutation_validation) {
    BOOST_CHECK_NO_THROW(paratoric::parallel_tempering_rank_of_ladder_index(
        std::vector<int>{0, 1, 2, 3}));
    BOOST_CHECK_NO_THROW(paratoric::parallel_tempering_rank_of_ladder_index(
        std::vector<int>{2, 0, 3, 1}));
    BOOST_CHECK_THROW(paratoric::parallel_tempering_rank_of_ladder_index(
        std::vector<int>{0, 1, 1, 3}), std::invalid_argument);
    BOOST_CHECK_THROW(paratoric::parallel_tempering_rank_of_ladder_index(
        std::vector<int>{0, 1, 5, 3}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(pt_partner_rank_uses_ladder_position) {
    const auto partner = [](int rank, const std::vector<int>& permutation, int phase) {
        return paratoric::parallel_tempering_partner_rank(rank, permutation, phase);
    };

    const std::vector<int> identity{0, 1, 2, 3};
    BOOST_REQUIRE(partner(0, identity, 0));
    BOOST_CHECK_EQUAL(*partner(0, identity, 0), 1);
    BOOST_REQUIRE(partner(2, identity, 0));
    BOOST_CHECK_EQUAL(*partner(2, identity, 0), 3);
    BOOST_REQUIRE(partner(1, identity, 1));
    BOOST_CHECK_EQUAL(*partner(1, identity, 1), 2);
    BOOST_CHECK(!partner(0, identity, 1));
    BOOST_CHECK(!partner(3, identity, 1));

    const std::vector<int> accepted_pair_scramble{1, 0, 3, 2};
    BOOST_REQUIRE(partner(0, accepted_pair_scramble, 1));
    BOOST_CHECK_EQUAL(*partner(0, accepted_pair_scramble, 1), 3);
    BOOST_REQUIRE(partner(3, accepted_pair_scramble, 1));
    BOOST_CHECK_EQUAL(*partner(3, accepted_pair_scramble, 1), 0);
    BOOST_CHECK(!partner(1, accepted_pair_scramble, 1));
    BOOST_CHECK(!partner(2, accepted_pair_scramble, 1));
    const int old_buggy_rank_partner = 0 + 1;
    BOOST_CHECK_NE(old_buggy_rank_partner, *partner(0, accepted_pair_scramble, 1));

    const std::vector<int> two_pair_scramble{2, 3, 0, 1};
    BOOST_REQUIRE(partner(2, two_pair_scramble, 0));
    BOOST_CHECK_EQUAL(*partner(2, two_pair_scramble, 0), 3);
    BOOST_REQUIRE(partner(0, two_pair_scramble, 0));
    BOOST_CHECK_EQUAL(*partner(0, two_pair_scramble, 0), 1);
}

BOOST_AUTO_TEST_CASE(pt_swap_period_sweeps_resolves_to_l2_scaled_moves) {
    paratoric::LatSpec l8_lat_spec = {
        .basis = 'z',
        .lattice_type = "square",
        .system_size = 8,
        .beta = 8.0,
        .boundaries = "periodic",
        .default_spin = 1,
        .lattice_backend = "boost"
    };
    paratoric::LatSpec l120_lat_spec = l8_lat_spec;
    l120_lat_spec.system_size = 120;
    l120_lat_spec.beta = 120.0;

    paratoric::ParallelTemperingSpec default_sweeps = {
        .enabled = true,
        .ladder_s = {0.22, 0.30}
    };
    BOOST_CHECK_EQUAL(paratoric::square_lattice_sweep_size_moves(l8_lat_spec), 128);
    BOOST_CHECK_EQUAL(paratoric::square_lattice_sweep_size_moves(l120_lat_spec), 28800);
    BOOST_CHECK_EQUAL(paratoric::parallel_tempering_swap_period_moves(default_sweeps, l8_lat_spec), 256);
    BOOST_CHECK_EQUAL(paratoric::parallel_tempering_swap_period_moves(default_sweeps, l120_lat_spec), 57600);

    paratoric::ParallelTemperingSpec equivalent_old_l8 = {
        .enabled = true,
        .ladder_s = {0.22, 0.30},
        .swap_period_sweeps = 1000.0 / 128.0
    };
    const auto equivalent_old_l8_moves =
        paratoric::parallel_tempering_swap_period_moves(equivalent_old_l8, l8_lat_spec);
    BOOST_CHECK_EQUAL(equivalent_old_l8_moves, 1000);

    std::vector<std::int64_t> swap_attempt_steps;
    for (std::int64_t step = 1; step <= 3 * equivalent_old_l8_moves; ++step) {
        if (step % equivalent_old_l8_moves == 0) {
            swap_attempt_steps.emplace_back(step);
        }
    }
    BOOST_REQUIRE_EQUAL(swap_attempt_steps.size(), 3);
    BOOST_CHECK_EQUAL(swap_attempt_steps[1] - swap_attempt_steps[0], equivalent_old_l8_moves);
    BOOST_CHECK_EQUAL(swap_attempt_steps[2] - swap_attempt_steps[1], equivalent_old_l8_moves);

    paratoric::ParallelTemperingSpec legacy_raw_moves = {
        .enabled = true,
        .ladder_s = {0.22, 0.30},
        .swap_period_sweeps = 2.0,
        .swap_period = 1000
    };
    BOOST_CHECK_EQUAL(paratoric::parallel_tempering_swap_period_moves(legacy_raw_moves, l120_lat_spec), 1000);
}

BOOST_AUTO_TEST_CASE(boost_pt_swap_weight_matches_bruteforce_l8_alt_seed_ladder) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z',
        .lattice_type = "square",
        .system_size = 8,
        .beta = 8.0,
        .boundaries = "periodic",
        .default_spin = 1,
        .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {
        .mu = 1.0,
        .h = 0.24,
        .J = 1.0,
        .lmbda = 0.24
    };
    paratoric::SimSpec sim_spec = {
        .N_samples = 8,
        .N_thermalization = 12,
        .N_between_samples = 6,
        .N_resamples = 8,
        .seed = 97531,
        .observables = {"energy", "sigma_x", "star_x"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.24, 0.31},
        .swap_period = 3,
        .validate_swap_weight = true
    };

    auto mc = paratoric::ExtendedToricCodeQMC<'z'>();
    const double max_deviation = mc.validate_parallel_tempering_swap_weight_debug(
        paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec}
    );
    BOOST_TEST_MESSAGE("L=8 PT cached-vs-bruteforce max log-weight deviation (alt): " << max_deviation);
    BOOST_TEST(max_deviation < 1e-8);
}

BOOST_AUTO_TEST_CASE(boost_pt_independent_h_lmbda_swap_weight_matches_bruteforce_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.15, .J = 1.0, .lmbda = 0.30};
    paratoric::SimSpec sim_spec = {
        .N_samples = 8, .N_thermalization = 16, .N_between_samples = 8,
        .N_resamples = 8, .seed = 31415, .observables = {"energy", "sigma_x", "star_x"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.15, 0.25},
        .ladder_lmbda = {0.30, 0.30},
        .swap_period = 4,
        .validate_swap_weight = true
    };

    const double max_deviation = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec});
    BOOST_TEST_MESSAGE("L=8 independent h/lambda PT cached-vs-bruteforce max log-weight deviation: " << max_deviation);
    BOOST_TEST(max_deviation < 1e-8);
}

BOOST_AUTO_TEST_CASE(boost_pt_explicit_self_dual_ladders_match_empty_lmbda_fallback_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.22, .J = 1.0, .lmbda = 0.22};
    paratoric::SimSpec sim_spec = {
        .N_samples = 8, .N_thermalization = 16, .N_between_samples = 8,
        .N_resamples = 8, .seed = 86420, .observables = {"energy", "sigma_x", "star_x"}
    };
    paratoric::ParallelTemperingSpec fallback_pt_spec = {
        .enabled = true, .ladder_s = {0.22, 0.30}, .swap_period = 4, .validate_swap_weight = true
    };
    paratoric::ParallelTemperingSpec explicit_pt_spec = fallback_pt_spec;
    explicit_pt_spec.ladder_lmbda = {0.22, 0.30};

    std::vector<double> fallback_trace;
    std::vector<double> explicit_trace;
    const double fallback_deviation = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_debug(
        paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, fallback_pt_spec},
        &fallback_trace);
    const double explicit_deviation = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_debug(
        paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, explicit_pt_spec},
        &explicit_trace);
    BOOST_TEST_MESSAGE("L=8 explicit-self-dual PT max log-weight deviations: fallback="
        << fallback_deviation << ", explicit=" << explicit_deviation);
    BOOST_REQUIRE(!fallback_trace.empty());
    BOOST_REQUIRE_EQUAL(fallback_trace.size(), explicit_trace.size());
    BOOST_CHECK_EQUAL(fallback_deviation, explicit_deviation);
    for (size_t i = 0; i < fallback_trace.size(); ++i) {
        BOOST_CHECK_EQUAL(fallback_trace[i], explicit_trace[i]);
    }
}

// M2/M3 CONSISTENCY checks, not physics validation. Feynman's correction (relayed
// 2026-08-18): these compare the production formula against the SAME M1 derivation
// that M3 implements, so after M3 they compare a formula against itself. They prove a
// term is not MISSING; they cannot catch a wrong sign, wrong coefficient, or the
// wrong accessor in the right slot, because both sides would be wrong identically.
// Before M3 they must FAIL (a term is genuinely missing from the unmodified weight).
// After M3 they pass, which shows no term is missing -- nothing more. Independent
// physics validation is in boost_pt_diagonal_terms_match_finite_difference_l8 and
// boost_pt_off_diagonal_terms_match_raw_kink_counts_l8 below, which do not depend on
// the M1 derivation at all.

BOOST_AUTO_TEST_CASE(boost_pt_mu_only_swap_weight_matches_m1_consistency_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 4000, .N_between_samples = 1,
        .N_resamples = 8, .seed = 24601, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.20, 0.20},
        .ladder_lmbda = {0.25, 0.25},
        .swap_period = 4,
        .validate_swap_weight = true
    };

    const auto result = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_generalized_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec},
            /*local_mu=*/1.0, /*partner_mu=*/1.3, /*local_J=*/1.0, /*partner_J=*/1.0);
    BOOST_TEST_MESSAGE("mu-only case: production=" << result.production_log_ratio
        << " m1_derivation=" << result.m1_derivation_log_ratio);
    // Tolerance, not exact equality: production and the M1 scaffold sum the same
    // terms in different orders (the scaffold is a straight-line sum; production
    // interleaves h/lmbda with the new mu/J branches), so a ~1e-15 relative
    // floating-point difference is expected even when both sides are correct.
    BOOST_TEST(result.production_log_ratio == result.m1_derivation_log_ratio,
        boost::test_tools::tolerance(1e-12));
}

BOOST_AUTO_TEST_CASE(boost_pt_J_only_swap_weight_matches_m1_consistency_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 4000, .N_between_samples = 1,
        .N_resamples = 8, .seed = 112358, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.20, 0.20},
        .ladder_lmbda = {0.25, 0.25},
        .swap_period = 4,
        .validate_swap_weight = true
    };

    const auto result = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_generalized_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec},
            /*local_mu=*/1.0, /*partner_mu=*/1.0, /*local_J=*/1.0, /*partner_J=*/1.3);
    BOOST_TEST_MESSAGE("J-only case: production=" << result.production_log_ratio
        << " m1_derivation=" << result.m1_derivation_log_ratio);
    // Tolerance, not exact equality: production and the M1 scaffold sum the same
    // terms in different orders (the scaffold is a straight-line sum; production
    // interleaves h/lmbda with the new mu/J branches), so a ~1e-15 relative
    // floating-point difference is expected even when both sides are correct.
    BOOST_TEST(result.production_log_ratio == result.m1_derivation_log_ratio,
        boost::test_tools::tolerance(1e-12));
}

BOOST_AUTO_TEST_CASE(boost_pt_common_factor_swap_weight_matches_m1_consistency_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    // c_local = 1.0, c_partner = 1.3 applied to (h0=0.20, mu0=1.0, J0=1.0, lmbda0=0.25).
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 4000, .N_between_samples = 1,
        .N_resamples = 8, .seed = 271828, .observables = {"energy"}
    };
    paratoric::ParallelTemperingSpec pt_spec = {
        .enabled = true,
        .ladder_s = {0.20, 0.26},
        .ladder_lmbda = {0.25, 0.325},
        .swap_period = 4,
        .validate_swap_weight = true
    };

    const auto result = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_parallel_tempering_swap_weight_generalized_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, pt_spec},
            /*local_mu=*/1.0, /*partner_mu=*/1.3, /*local_J=*/1.0, /*partner_J=*/1.3);
    BOOST_TEST_MESSAGE("common-factor case: production=" << result.production_log_ratio
        << " m1_derivation=" << result.m1_derivation_log_ratio);
    // Tolerance, not exact equality: production and the M1 scaffold sum the same
    // terms in different orders (the scaffold is a straight-line sum; production
    // interleaves h/lmbda with the new mu/J branches), so a ~1e-15 relative
    // floating-point difference is expected even when both sides are correct.
    BOOST_TEST(result.production_log_ratio == result.m1_derivation_log_ratio,
        boost::test_tools::tolerance(1e-12));
}

// M3 non-circular check 1/2: diagonal terms (lmbda/edge, J/plaquette) validated
// against total_integrated_pot_energy(...) by central finite difference on a FIXED
// configuration. This is the pre-existing Metropolis potential-energy function, not
// the swap-weight formula or the M1 derivation. total_integrated_pot_energy is affine
// in lmbda and J holding the lattice fixed (lat.total_integrated_edge_energy() and
// lat.total_integrated_plaquette_energy() do not depend on the passed-in couplings),
// so this finite difference is algebraically exact, not merely eps-accurate; a wrong
// sign or a wrong coefficient in the swap weight's diagonal terms would show up as an
// exact, not approximate, mismatch here.
BOOST_AUTO_TEST_CASE(boost_pt_diagonal_terms_match_finite_difference_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 4000, .N_between_samples = 1,
        .N_resamples = 8, .seed = 33221, .observables = {"energy"}
    };

    const auto diag = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_single_replica_diagnostics_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}},
            /*h=*/0.20, /*mu=*/1.0, /*J=*/1.0, /*lmbda=*/0.25, /*eps=*/1e-6);
    BOOST_TEST_MESSAGE("finite_diff_dlmbda=" << diag.finite_diff_dlmbda << " -E_edge=" << -diag.E_edge);
    BOOST_TEST_MESSAGE("finite_diff_dJ=" << diag.finite_diff_dJ << " -E_plaquette=" << -diag.E_plaquette);
    // total_integrated_pot_energy = -lat.total_integrated_edge_energy()*lmbda
    //     - lat.total_integrated_plaquette_energy()*J (extended_toric_code_qmc.hpp,
    //     total_integrated_pot_energy, z-basis branch), so d/dlmbda = -E_edge and
    // d/dJ = -E_plaquette. E_edge/E_plaquette here are the swap weight's own
    // accessors (cached_total_integrated_edge_energy / _plaquette_energy).
    BOOST_TEST(diag.finite_diff_dlmbda == -diag.E_edge, boost::test_tools::tolerance(1e-6));
    BOOST_TEST(diag.finite_diff_dJ == -diag.E_plaquette, boost::test_tools::tolerance(1e-6));
}

// M3 non-circular check 2/2: off-diagonal (kink-count) terms validated against raw
// counts, not through the swap-weight formula.
//  - h/single slot: beta*get_non_diag_single_energy_z() (cached) vs kL+kR from
//    get_kL_kR_single() -- a pre-existing, independently-implemented public accessor
//    (written for the Fredenhagen-Marcu estimator; unrelated to this work) that walks
//    the same single_spin_flips containers by a different code path.
//  - mu/tuple (star) slot: beta*get_non_diag_tuple_energy_z() (cached) vs
//    beta*recompute_non_diag_tuple_energy_z() (from-scratch). LIMITATION, stated
//    plainly: no independently-implemented public counter exists for star flips in
//    this codebase (unlike single flips), so this is a cache-vs-recompute check, the
//    same family as the old "bruteforce" test, not a fully independent count. It is
//    the closest available check for this slot.
BOOST_AUTO_TEST_CASE(boost_pt_off_diagonal_terms_match_raw_kink_counts_l8) {
    paratoric::LatSpec lat_spec = {
        .basis = 'z', .lattice_type = "square", .system_size = 8, .beta = 8.0,
        .boundaries = "periodic", .default_spin = 1, .lattice_backend = "boost"
    };
    paratoric::ParamSpec param_spec = {.mu = 1.0, .h = 0.20, .J = 1.0, .lmbda = 0.25};
    paratoric::SimSpec sim_spec = {
        .N_samples = 1, .N_thermalization = 4000, .N_between_samples = 1,
        .N_resamples = 8, .seed = 55443, .observables = {"energy"}
    };

    const auto diag = paratoric::ExtendedToricCodeQMC<'z'>()
        .validate_single_replica_diagnostics_debug(
            paratoric::Config{sim_spec, param_spec, lat_spec, paratoric::OutSpec{}, paratoric::ParallelTemperingSpec{}},
            /*h=*/0.20, /*mu=*/1.0, /*J=*/1.0, /*lmbda=*/0.25, /*eps=*/1e-6);
    BOOST_TEST_MESSAGE("beta_times_single=" << diag.beta_times_single
        << " kL_plus_kR_single=" << diag.kL_plus_kR_single);
    BOOST_TEST_MESSAGE("beta_times_tuple=" << diag.beta_times_tuple
        << " recompute_beta_times_tuple=" << diag.recompute_beta_times_tuple);
    BOOST_TEST(diag.beta_times_single == diag.kL_plus_kR_single, boost::test_tools::tolerance(1e-9));
    BOOST_TEST(diag.beta_times_tuple == diag.recompute_beta_times_tuple, boost::test_tools::tolerance(1e-9));
}

} // namespace paratoric
