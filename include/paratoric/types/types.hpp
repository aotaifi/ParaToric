// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include <complex>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <variant>
#include <vector>

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace paratoric {

/**
 * @brief Lattice specification
 * 
 * @param basis eigenbasis of the spins, either 'x' or 'z'
 * @param lattice_type the lattice type, e.g. "triangular"
 * @param system_size the system size of the lattice (in one dimension)
 * @param beta inverse temperature
 * @param boundaries the boundary condition of the lattice (periodic, open)
 * @param default_spin the default spin on the links (1 or -1)
 * @param lattice_backend lattice storage/backend implementation (boost, flat_square)
 * 
 */
struct LatSpec {
    char        basis        = 'x';
    std::string lattice_type = "square";
    int         system_size  = 16;
    double      beta         = 16.;
    std::string boundaries   = "periodic";
    int         default_spin = +1;
    std::string lattice_backend = "boost";
};

/**
 * @brief Hamiltonian parameter specification
 * 
 * @param mu the Hamiltonian parameter (star term)
 * @param h the Hamiltonian parameter (electric field term)
 * @param J the Hamiltonian parameter (plaquette term)
 * @param lmbda the Hamiltonian parameter (gauge field term)
 * @param h_therm the thermalization value of h, used if custom_therm enabled
 * @param lmbda_therm the thermalization value of lmbda, used if custom_therm enabled
 * @param h_hys the Hamiltonian hysteresis parameters (electric field term)
 * @param lmbda_hys the Hamiltonian hysteresis parameters (gauge field term)
 * 
 */
struct ParamSpec {
    double mu     = 1.0;
    double h      = 0.0;
    double J      = 1.0;
    double lmbda  = 0.0;
    // optional “thermalization schedule” values when they differ
    double h_therm     = std::numeric_limits<double>::quiet_NaN();
    double lmbda_therm = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> h_hys{};
    std::vector<double> lmbda_hys{};
};

/**
 * @brief Simulation specification
 * 
 * @param N_samples the number of snapshots
 * @param N_thermalization the number of thermalization steps
 * @param N_between_samples the number of steps between snapshots
 * @param N_resamples the number of bootstrap resamples
 * @param custom_therm if custom thermalization is used (to probe hysteresis)
 * @param seed the seed for the pseudorandom number generator
 * @param observables all observables that are calculated for every snapshot
 * 
 */
struct SimSpec {
    int    N_samples                = 1000;     // 0 for thermalization-only
    std::int64_t N_thermalization   = 10000;
    int    N_between_samples        = 1000;
    int    N_resamples              = 1000;
    bool   custom_therm             = false; // used only by sampling
    int    seed                     = 0;     // 0 means random seed
    std::vector<std::string> observables{};
};

/**
 * @brief Output specification
 * 
 * @param path_out the directory where output files will be written to 
 * @param paths_out the directories (hysteresis simulation) where output files will be written to
 * @param folder_name the folder name of the output folder, additional possibility to make the output clearer
 * @param folder_names the folder names of the output folders (hysteresis simulation), additional possibility to make the output clearer
 * @param save_snapshots whether snapshots should be saved
 * @param full_time_series whether full time series should be saved
 * @param checkpoint_path path for periodic etc_sample checkpoint writes
 * @param restart_from path of an etc_sample checkpoint to resume
 * @param checkpoint_every_samples checkpoint period in completed samples
 * @param checkpoint_every_seconds checkpoint period in wall-clock seconds, checked at sample boundaries
 * 
 */
struct OutSpec {
    std::filesystem::path path_out; // single path
    std::vector<std::filesystem::path> paths_out{}; // hysteresis
    std::string folder_name; // single folder
    std::vector<std::string> folder_names{}; // hysteresis
    bool save_snapshots = false;
    bool full_time_series = 0;
    std::filesystem::path checkpoint_path{};
    std::filesystem::path restart_from{};
    int checkpoint_every_samples = 0;
    int checkpoint_every_seconds = 0;
};

struct ParallelTemperingSpec {
    bool enabled = false;
    std::vector<double> ladder_s{}; // h ladder; retained for backwards compatibility.
    std::vector<double> ladder_lmbda{}; // Empty means use ladder_s (self-dual ladder).
    // Common-factor (beta,s) tempering (doc/coupled_tempering_implementation_plan.md).
    // Empty means mu/J are untempered (config.param_spec.mu/.J for every rung), which
    // reproduces pre-existing behaviour bit for bit -- this is the M3 guardrail.
    std::vector<double> ladder_mu{};
    std::vector<double> ladder_J{};
    double swap_period_sweeps = 2.0;
    int swap_period = 0; // Legacy raw Metropolis-move period; 0 means use sweeps.
    bool validate_swap_weight = false;
    // M10 (Feynman): multi-rank identity mode. Every swap is still ATTEMPTED and every
    // weight is still assembled from genuinely MPI-transported partner data, but the
    // swap is never ACCEPTED. Each replica therefore stays on its own rung and
    // equilibrates there, which is the condition under which the exact identity
    // <R> = 1 holds (memory reference_pt_swap_weight_identity). That makes the identity
    // testable at EVERY link of a ladder of any width, not only on a single pair.
    // Off by default; production never sets it.
    bool never_accept_swaps = false;
    // M10 power test. Exchanges the roles of the RECEIVED partner_edge and
    // partner_plaquette values. That is exactly the mis-transport the M4 reverse-move
    // balance check is blind to, because that check re-reads the same received values
    // in both directions (see the BLIND SPOT comment in the swap block). A multi-rank
    // identity check that does not FAIL under this flag has no power and must not be
    // trusted. Off by default; production never sets it.
    bool break_transport_for_test = false;
    // M10 power test for the routing guard. Sends a ladder index one higher than the
    // one this rank actually holds, so the value that arrives at the partner no longer
    // matches what the Allgather reported for this rank. The routing guard must throw.
    // Off by default; production never sets it.
    bool break_routing_for_test = false;
};

/**
 * @brief Configuration for starting a simulation
 * 
 * @param sim_spec Simulation parameters (N_samples, seed, ...) 
 * @param param_spec Hamiltonian parameters
 * @param lat_spec Lattice parameters (system_size, beta, ...)
 * @param out_spec Output parameters (path_out, folder_name, ...)
 * 
 */
struct Config{
    SimSpec sim_spec{};
    ParamSpec param_spec{};
    LatSpec lat_spec{};
    OutSpec out_spec{};
    ParallelTemperingSpec pt_spec{};
};

struct AcceptanceDiagnostics {
    std::uint64_t attempted = 0;
    std::uint64_t accepted = 0;
    double acceptance_ratio_sum = 0.;
    std::vector<std::uint64_t> attempted_by_update = std::vector<std::uint64_t>(7, 0);
    std::vector<std::uint64_t> accepted_by_update = std::vector<std::uint64_t>(7, 0);
    std::vector<double> acceptance_ratio_sum_by_update = std::vector<double>(7, 0.);
    std::vector<std::uint64_t> block_attempted{};
    std::vector<std::uint64_t> block_accepted{};
    std::vector<double> block_acceptance_ratio_sum{};
};

struct ParallelTemperingDiagnostics {
    bool enabled = false;
    bool mpi_compiled = false;
    int world_size = 1;
    int rank = 0;
    int initial_ladder_index = 0;
    int final_ladder_index = 0;
    double initial_s = 0.; // runtime_h
    double final_s = 0.;   // runtime_h
    // M9: the other three runtime couplings, mirroring initial_s/final_s above.
    // Needed to verify checkpoint restart restores ALL FOUR runtime couplings, not
    // just h -- initial_s/final_s alone could not distinguish that from a bug that
    // restored the ladder index and h but left mu/J/lmbda at their launch values.
    double initial_mu = 0.;
    double final_mu = 0.;
    double initial_J = 0.;
    double final_J = 0.;
    double initial_lmbda = 0.;
    double final_lmbda = 0.;
    std::uint64_t swap_attempted = 0;
    std::uint64_t swap_accepted = 0;
    double max_swap_log_weight_deviation = 0.;
    // M4 reverse-move balance check: |logR_forward + logR_reverse|, max over all
    // attempted swaps in both the thermalization and production loops. Populated
    // only when pt_spec.validate_swap_weight is set. See
    // doc/coupled_tempering_implementation_plan.md M4 and
    // code_ruby_duy_pinning_lab/worm.update.cpp:832-853 for the pattern.
    double max_swap_balance_err = 0.;
    // M8 (Feynman): |inline log_r - parallel_tempering_log_weight_ratio(...)| for the
    // SAME inputs, max over all attempted swaps in both loops. The shared function is
    // not what production executes (the two inline blocks are hand-maintained
    // separate copies, M3/M4); this closes that gap by cross-checking the copy that
    // runs against the copy the debug validators and M7'' exercise, on the same live
    // MPI-exchanged data. Populated only when pt_spec.validate_swap_weight is set.
    double max_shared_function_deviation = 0.;
    std::vector<std::uint64_t> swap_attempted_by_link{};
    std::vector<std::uint64_t> swap_accepted_by_link{};
    // M10 (Feynman): per-link accumulators for the exact identity <R> = 1, with
    // R = exp(log_r) built from the live MPI-exchanged partner data. Populated only
    // when validate_swap_weight is set. Kept PER LINK rather than pooled, so the
    // per-link table can be printed and a single bad link cannot hide inside a
    // healthy average.
    std::vector<std::uint64_t> R_count_by_link{};
    std::vector<double> R_sum_by_link{};
    std::vector<double> R_sumsq_by_link{};
    std::vector<int> ladder_index_by_sample{};
};

/**
 * @brief Result of a simulation
 * 
 * @param series          Full counting statistics of all requested observables in the input order,
 *                        Each observable’s entry contains one value per recorded snapshot, 
 *                        in time order.
 * @param acc_ratio       Time series of Monte Carlo acceptance ratios.
 * @param mean            Bootstrap observable means
 * @param mean_std        Bootstrap standard errors of the mean
 * @param binder          Bootstrap binder ratios
 * @param binder_std      Bootstrap standard errors of the binder ratios
 * @param tau_int         Estimated integrated autocorrelation times
 * @param series_hys      Full counting statistics of all requested observables.
 *                        Each element in the outer vector represents one parameter point (h_hys, lmbda_hys)
 *                        Each element in the middle vectors represent one observables in the order of input.
 *                        The inner vector are the full counting statistics in time order.
 * @param mean_hys        Bootstrap observable means. 
 *                        Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
 *                        Each element in the inner vectors represents one observables in the order of input.
 * @param mean_std_hys    Bootstrap standard errors of the mean.
 *                        Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
 *                        Each element in the inner vectors represents one observables in the order of input.
 * @param binder_hys      Bootstrap binder ratios.
 *                        Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
 *                        Each element in the inner vectors represents one observables in the order of input.
 * @param binder_std_hys  Bootstrap standard errors of the binder ratios.
 *                        Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
 *                        Each element in the inner vectors represents one observables in the order of input.
 * @param tau_int_hys     Estimated integrated autocorrelation times.
 *                        Each element of the outer vector represents one parameter point (h_hys, lmbda_hys).
 *                        Each element in the inner vectors represents one observables in the order of input.
 * 
 */
struct Result {
    std::vector<std::vector<std::variant<std::complex<double>, double>>> series{};
    std::vector<double> acc_ratio{};
    AcceptanceDiagnostics production_acceptance{};
    ParallelTemperingDiagnostics pt_diagnostics{};
    std::vector<double> mean{}, mean_std{}, binder{}, binder_std{}, tau_int{};
    std::vector<std::vector<std::vector<std::variant<std::complex<double>, double>>>> series_hys{};
    std::vector<std::vector<double>> mean_hys{}, mean_std_hys{}, binder_hys{}, binder_std_hys{}, tau_int_hys{};
};

} // namespace paratoric
