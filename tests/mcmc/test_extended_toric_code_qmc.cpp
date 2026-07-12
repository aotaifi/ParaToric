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

} // namespace paratoric
