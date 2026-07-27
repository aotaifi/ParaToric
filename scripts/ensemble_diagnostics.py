#!/usr/bin/env python3
"""Ensemble diagnostics for ParaToric I4/I5 gates.

R-hat uses the original Gelman-Rubin variance estimator for K equal-length
chains: B = n Var(chain means), W = mean(within-chain variances),
var_hat = ((n - 1) / n) W + B / n, Rhat = sqrt(var_hat / W).
Reference: Gelman and Rubin, Statistical Science 7, 457-472 (1992).
"""

from __future__ import annotations

import argparse
import glob
import hashlib
import math
import re
import sys
import tempfile
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

import h5py
import numpy as np
from scipy.interpolate import CubicSpline, PchipInterpolator
from scipy.optimize import brentq
from scipy.signal import find_peaks
from scipy.stats import chi2, gaussian_kde

REPO_ROOT = Path(__file__).resolve().parents[1]
SHARED_QMC = REPO_ROOT.parents[0] / "ai-workflow" / "qmc_common"
if str(SHARED_QMC) not in sys.path:
    sys.path.insert(0, str(SHARED_QMC))

from hdf5_readers import read_paratoric_observable, real_array  # noqa: E402


RESULTS_ROOT = "/simulation/results"
PLATEAU_CONSTANT_P_THRESHOLD = 0.05
SPREAD_SCALING_MIN_WINDOW_RATIO = 100.0
SPREAD_NOISE_RATIO_MIN = 0.5
SPREAD_NOISE_RATIO_MAX = 2.0
OBSERVABLE_DEGENERATE_ZERO_FRACTION = 0.5
STEP2A_LEGACY_TSV_FILENAMES = (
    "step2a_depth_metrics.tsv",
    "step2a_duality.tsv",
    "step2a_energy_gap.tsv",
    "step2a_energy_bimodality.tsv",
)
STEP2A_LEGACY_SELF_TEST_SHA256 = {
    "step2a_depth_metrics.tsv": "1c763276787d9f8a9431eae796b171672ced5ac1a409c0331ccfdf15ab3eed98",
    "step2a_duality.tsv": "9e2548f73e69d1ad3496f782db14f55d6b6331719ef4d2b401aca1a217b63184",
    "step2a_energy_gap.tsv": "9696489aedcb6c265c9bf2a76c84f5110977de183a4d2cf2698c9a9fd5496e07",
    "step2a_energy_bimodality.tsv": "9b299ddce393de489f8cece0f69d2fe0cb81a0560fdbf2354de0d93567b67a5c",
}


@dataclass(frozen=True)
class ChainStats:
    path: Path
    observable: str
    mean: float
    stored_error: float
    tau: float
    n: int | None
    within_variance: float | None
    series: np.ndarray | None = None
    pt_initial_ladder_index: int | None = None
    pt_initial_s: float | None = None
    pt_ladder_index_by_sample: np.ndarray | None = None


@dataclass(frozen=True)
class BlockingTauEstimate:
    """One-chain blocking-plateau autocorrelation estimate in sample units."""

    tau: float
    error: float
    status: str
    suggested_block_size: float
    block_size: int
    ladder: tuple[tuple[int, float, float], ...]


@dataclass(frozen=True)
class BlockingTauSummary:
    """Per-point aggregation of independently estimated chain autocorrelations."""

    tau_median: float
    tau_min: float
    tau_max: float
    neff_median: float
    status: str
    suggested_block_size: float
    estimates: tuple[BlockingTauEstimate, ...]


def blocking_plateau_candidate(
    triple: tuple[tuple[int, float, float], tuple[int, float, float], tuple[int, float, float]]
) -> bool:
    """Apply overlap, slope-z, and constant-fit gates to three rungs.

    The three points must overlap on adjacent ladder edges and have a weighted
    slope-z below 1.0 when x=log2(b).  This reuses the campaign's existing
    ``weighted_slope_z`` machinery and rejects a statistically significant
    monotonic rise even when its terminal error bars are large.  The same
    three-point window must also satisfy the existing ``weighted_constant_p``
    test with p > 0.05; both gates are required for a plateau claim.
    """
    overlap = all(
        abs(left[1] - right[1]) <= left[2] + right[2]
        for left, right in ((triple[0], triple[1]), (triple[1], triple[2]))
    )
    if not overlap or any(point[2] <= 0.0 for point in triple):
        return False
    x = np.log2(np.array([point[0] for point in triple], dtype=float))
    values = np.array([point[1] for point in triple], dtype=float)
    errors = np.array([point[2] for point in triple], dtype=float)
    slope_z = weighted_slope_z(x, values, errors)
    constant_p = weighted_constant_p(values, errors)
    return (
        math.isfinite(slope_z)
        and slope_z < 1.0
        and math.isfinite(constant_p)
        and constant_p > PLATEAU_CONSTANT_P_THRESHOLD
    )


def expand_inputs(patterns: Iterable[str]) -> list[Path]:
    paths: list[Path] = []
    for pattern in patterns:
        matches = sorted(glob.glob(pattern))
        paths.extend(Path(match) for match in matches)
    seen: set[Path] = set()
    unique: list[Path] = []
    for path in paths:
        resolved = path.resolve()
        if resolved not in seen:
            seen.add(resolved)
            unique.append(resolved)
    return unique


def available_observables(paths: Iterable[Path]) -> list[str]:
    common: set[str] | None = None
    for path in paths:
        with h5py.File(path, "r") as handle:
            if RESULTS_ROOT not in handle:
                raise KeyError(f"{path}: missing {RESULTS_ROOT}")
            names = {
                name
                for name, item in handle[RESULTS_ROOT].items()
                if isinstance(item, h5py.Group) and "mean" in item
            }
        common = names if common is None else common & names
    return sorted(common or set())


def read_chain(
    path: Path,
    observable: str,
    samples_override: int | None = None,
    include_pt_diagnostics: bool = False,
) -> ChainStats:
    with h5py.File(path, "r") as handle:
        group = handle[RESULTS_ROOT]
        data = read_paratoric_observable(group, observable)
        series = real_array(handle[f"{RESULTS_ROOT}/{observable}/series"]) if (
            f"{RESULTS_ROOT}/{observable}/series" in handle
        ) else None
        pt_initial_index = pt_initial_s = pt_labels = None
        if include_pt_diagnostics:
            pt_root = "/simulation/diagnostics/parallel_tempering"
            pt_initial_index = int(handle[f"{pt_root}/initial_ladder_index"][()])
            pt_initial_s = float(handle[f"{pt_root}/initial_s"][()])
            pt_labels = np.asarray(handle[f"{pt_root}/ladder_index_by_sample"], dtype=int)
    if not data:
        raise KeyError(f"{path}: observable {observable!r} not found")

    mean = float(data[f"{observable}_mean"])
    error = float(data[f"{observable}_error"])
    tau = float(data[f"{observable}_autocorrelation_time"])
    n = data.get(f"{observable}_series_n")
    n_int = int(n) if n is not None else samples_override
    within_variance: float | None = None
    second_moment = data.get(f"{observable}_series_second_moment")
    if n_int is not None and second_moment is not None and n_int > 1:
        population_var = max(0.0, float(second_moment) - mean * mean)
        within_variance = population_var * n_int / (n_int - 1)
    return ChainStats(
        path, observable, mean, error, tau, n_int, within_variance, series,
        pt_initial_index, pt_initial_s, pt_labels,
    )


def gelman_rubin_rhat(chains: list[ChainStats]) -> float | None:
    if len(chains) < 2:
        return None
    ns = {chain.n for chain in chains}
    if None in ns or len(ns) != 1:
        return None
    n = next(iter(ns))
    if n is None or n < 2:
        return None
    variances = [chain.within_variance for chain in chains]
    if any(var is None or not math.isfinite(var) or var <= 0.0 for var in variances):
        return None

    means = np.array([chain.mean for chain in chains], dtype=float)
    b = float(n) * float(np.var(means, ddof=1))
    w = float(np.mean(np.array(variances, dtype=float)))
    var_hat = ((float(n) - 1.0) / float(n)) * w + b / float(n)
    return math.sqrt(var_hat / w)


def error_is_tau_corrected(mode: str) -> bool:
    """Return whether ParaToric's stored mean_error is already honest.

    ``mean_error`` is produced by get_bootstrap_statistics(), whose stationary
    bootstrap explicitly includes autocorrelation effects.  Thus ``auto`` is
    deliberately identical to ``tau-corrected`` for the ParaToric obs.h5
    schema.  ``naive`` exists for legacy/external files which stored a plain
    IID standard error.
    """
    return mode in {"auto", "tau-corrected"}


def honest_error(chain: ChainStats, mode: str = "auto") -> float:
    """Return the uncertainty to use in z scores.

    For an IID/naive stored error, use sqrt(2*tau_int) to replace N by
    N_eff=N/(2*tau_int).  Do not apply that factor to ParaToric's stationary-
    bootstrap mean_error, which already includes autocorrelation.
    """
    if error_is_tau_corrected(mode):
        return chain.stored_error
    if not math.isfinite(chain.tau) or chain.tau <= 0.0:
        return math.nan
    return chain.stored_error * math.sqrt(2.0 * chain.tau)


def max_pairwise_z(chains: list[ChainStats], error_mode: str = "auto") -> float | None:
    max_z: float | None = None
    for i, left in enumerate(chains):
        for right in chains[i + 1 :]:
            denom = math.hypot(honest_error(left, error_mode), honest_error(right, error_mode))
            if denom <= 0.0 or not math.isfinite(denom):
                return None
            z = abs(left.mean - right.mean) / denom
            max_z = z if max_z is None else max(max_z, z)
    return max_z


def combined_z(left: ChainStats, right: ChainStats, error_mode: str = "auto") -> float | None:
    denom = math.hypot(honest_error(left, error_mode), honest_error(right, error_mode))
    if denom <= 0.0 or not math.isfinite(denom):
        return None
    return abs(left.mean - right.mean) / denom


def tau_ratio(chains: list[ChainStats]) -> float | None:
    taus = [chain.tau for chain in chains if math.isfinite(chain.tau) and chain.tau > 0.0]
    if len(taus) != len(chains):
        return None
    return max(taus) / min(taus)


def neff(chain: ChainStats) -> float | None:
    if chain.n is None or not math.isfinite(chain.tau) or chain.tau <= 0.0:
        return None
    return float(chain.n) / (2.0 * chain.tau)


def passfail(ok: bool) -> str:
    return "PASS" if ok else "FLAG"


def format_float(value: float | None, digits: int = 6) -> str:
    if value is None:
        return "NA"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.{digits}g}"


def run_ensemble(args: argparse.Namespace) -> int:
    paths = expand_inputs(args.paths)
    if len(paths) < 2:
        raise SystemExit("Need at least two HDF5 paths for ensemble diagnostics.")

    observables = args.observables or available_observables(paths)
    if not observables:
        raise SystemExit("No common observables found.")

    overall_ok = True
    error_source = "stored stationary-bootstrap error (already tau-corrected)" if error_is_tau_corrected(args.stored_error) else "naive stored error × sqrt(2*tau)"
    print("formula: Gelman-Rubin 1992 Rhat, sqrt((((n-1)/n)W + B/n)/W)")
    print(f"z-score error: {error_source}")
    print(f"chains: {len(paths)}")
    for observable in observables:
        chains = [read_chain(path, observable, args.samples) for path in paths]
        rhat = gelman_rubin_rhat(chains)
        zmax = max_pairwise_z(chains, args.stored_error)
        tratio = tau_ratio(chains)

        k_ok = len(chains) >= args.min_chains
        z_ok = zmax is not None and zmax < args.z_threshold
        tau_ok = tratio is not None and tratio <= args.tau_ratio_threshold
        rhat_ok = rhat is not None and rhat < args.rhat_threshold

        neff_values = [neff(chain) for chain in chains]
        sample_ok = all(
            chain.n is not None
            and math.isfinite(chain.tau)
            and chain.tau > 0.0
            and chain.n >= args.samples_per_tau * chain.tau
            for chain in chains
        )
        neff_ok = all(value is not None and value >= args.min_neff for value in neff_values)
        observable_ok = k_ok and z_ok and tau_ok and sample_ok and neff_ok
        overall_ok = overall_ok and observable_ok

        print(f"\nobservable {observable}: {passfail(observable_ok)}")
        print(f"  chains>= {args.min_chains}: {passfail(k_ok)} K={len(chains)}")
        print(
            f"  means z<{args.z_threshold:g}: {passfail(z_ok)} "
            f"max_pairwise_z={format_float(zmax)}"
        )
        print(
            f"  Rhat<{args.rhat_threshold:g}: {passfail(rhat_ok)} "
            f"Rhat={format_float(rhat)}"
        )
        print(
            f"  tau ratio<={args.tau_ratio_threshold:g}: {passfail(tau_ok)} "
            f"max/min={format_float(tratio)}"
        )
        print(
            f"  N>={args.samples_per_tau:g}*tau: {passfail(sample_ok)} "
            f"N=[{', '.join(str(chain.n) if chain.n is not None else 'NA' for chain in chains)}]"
        )
        print(
            f"  Neff>={args.min_neff:g}: {passfail(neff_ok)} "
            f"Neff=[{', '.join(format_float(value) for value in neff_values)}]"
        )
        for chain in chains:
            print(
                "    "
                f"{chain.path}: mean={format_float(chain.mean)} "
                f"stored_err={format_float(chain.stored_error)} "
                f"honest_err={format_float(honest_error(chain, args.stored_error))} "
                f"tau={format_float(chain.tau)} "
                f"N={chain.n if chain.n is not None else 'NA'} "
                f"within_var={format_float(chain.within_variance)}"
            )

    return 0 if overall_ok else 1


def run_cold_hot(args: argparse.Namespace) -> int:
    observables = args.observables
    if not observables:
        observables = available_observables([Path(args.cold), Path(args.hot)])
    overall_ok = True
    for observable in observables:
        cold = read_chain(Path(args.cold).resolve(), observable, args.samples)
        hot = read_chain(Path(args.hot).resolve(), observable, args.samples)
        z = combined_z(cold, hot, args.stored_error)
        ok = z is not None and z < args.z_threshold
        overall_ok = overall_ok and ok
        print(
            f"observable {observable}: {passfail(ok)} cold_hot_z={format_float(z)} "
            f"cold_mean={format_float(cold.mean)} hot_mean={format_float(hot.mean)} "
            f"cold_stored_err={format_float(cold.stored_error)} "
            f"hot_stored_err={format_float(hot.stored_error)} "
            f"cold_honest_err={format_float(honest_error(cold, args.stored_error))} "
            f"hot_honest_err={format_float(honest_error(hot, args.stored_error))} "
            f"cold_Neff={format_float(neff(cold))} hot_Neff={format_float(neff(hot))}"
        )
    return 0 if overall_ok else 1


def strength_binder_cumulant(series: np.ndarray) -> float:
    """Return U=1-<m^4>/(3<m^2>^2) from the recorded raw series."""
    if series.size < 2:
        raise ValueError("Binder cumulant needs at least two samples")
    second = float(np.mean(series * series))
    if not math.isfinite(second) or second <= 0.0:
        raise ValueError("Binder cumulant is undefined when <m^2> is non-positive")
    return 1.0 - float(np.mean(series**4)) / (3.0 * second * second)


def strength_raw_ratio(series: np.ndarray) -> float:
    """Return r=<m^4>/<m^2>^2 from the recorded raw series."""
    if series.size < 2:
        raise ValueError("Raw ratio needs at least two samples")
    second = float(np.mean(series * series))
    if not math.isfinite(second) or second <= 0.0:
        raise ValueError("Raw ratio is undefined when <m^2> is non-positive")
    return float(np.mean(series**4)) / (second * second)


def exact_zero_fraction(chains: list[np.ndarray]) -> float:
    """Return the pooled fraction of samples that are exactly zero."""
    values = np.concatenate(chains)
    if values.size == 0:
        raise ValueError("Zero fraction needs at least one sample")
    return float(np.count_nonzero(values == 0.0) / values.size)


def per_chain_raw_ratio_spread(chains: list[np.ndarray]) -> tuple[list[float], float]:
    """Return individual raw ratios and their max-min spread."""
    ratios = [strength_raw_ratio(chain) for chain in chains]
    return ratios, float(max(ratios) - min(ratios))


def spread_window_scaling_status(
    small_spread: float,
    small_samples: int,
    large_spread: float,
    large_samples: int,
    spread_gate: float,
) -> tuple[str, float]:
    """Classify ratio spread using the required 1/sqrt(window) power check.

    The legacy magnitude threshold remains the severity threshold only after a
    statistically powered scaling comparison rules out pure estimation noise.
    Without a >=100x comparison the spread gate is explicitly unresolved.
    """
    if (
        small_samples < 1
        or large_samples <= small_samples
        or large_samples / small_samples < SPREAD_SCALING_MIN_WINDOW_RATIO
        or not math.isfinite(small_spread)
        or not math.isfinite(large_spread)
    ):
        return "SPREAD-UNRESOLVED", math.nan
    predicted = small_spread * math.sqrt(small_samples / large_samples)
    if predicted == 0.0:
        if large_spread == 0.0:
            return "PASS", 1.0
        return ("FAIL" if large_spread > spread_gate else "PASS"), math.inf
    observed_over_noise = large_spread / predicted
    if SPREAD_NOISE_RATIO_MIN <= observed_over_noise <= SPREAD_NOISE_RATIO_MAX:
        return "PASS", observed_over_noise
    return ("FAIL" if small_spread > spread_gate else "PASS"), observed_over_noise


def integrated_autocorrelation_time(series: np.ndarray) -> float:
    """Estimate tau_int with an initial-positive-sequence autocovariance sum."""
    values = np.asarray(series, dtype=float)
    values = values - np.mean(values)
    n = values.size
    variance = float(np.dot(values, values)) / n
    if n < 4 or not math.isfinite(variance) or variance <= 0.0:
        return 0.5
    padded = 1 << (2 * n - 1).bit_length()
    spectrum = np.fft.rfft(values, padded)
    autocovariance = np.fft.irfft(spectrum * np.conjugate(spectrum), padded)[:n]
    autocorrelation = autocovariance / autocovariance[0]
    positive = np.flatnonzero(autocorrelation[1:] <= 0.0)
    cutoff = int(positive[0] + 1) if positive.size else n - 1
    return max(0.5, 0.5 + float(np.sum(autocorrelation[1 : cutoff + 1])))


def blocking_plateau_tau(series: np.ndarray) -> BlockingTauEstimate:
    """Estimate tau_int from a geometric blocking plateau in sample units.

    For each b=1,2,4,... with at least 16 complete blocks, this evaluates
    0.5*b*Var(block means)/Var(series).  The variance-only relative error is
    sqrt(2/(n_blocks-1)).  A plateau is the first three consecutive ladder
    points whose adjacent one-standard-error intervals overlap.  To prevent
    inflated terminal error bars from accepting a rising curve, the same
    weighted slope-z test used by depth-ramp constant/slope readouts must also
    satisfy slope-z < 1.0 for x=log2(b).  The middle point is reported,
    avoiding both the first potentially pre-asymptotic point and the noisiest
    terminal point.  If no such triple exists, b_max is a lower bound and the
    explicit TAU-UNRESOLVED status is returned.
    """
    values = np.asarray(series, dtype=float)
    n = values.size
    if n < 16:
        raise ValueError("Blocking tau needs at least 16 samples")
    if not np.all(np.isfinite(values)):
        raise ValueError("Blocking tau needs finite samples")
    variance = float(np.var(values, ddof=1))
    if variance <= 0.0:
        raise ValueError("Blocking tau is undefined for a constant series")

    ladder: list[tuple[int, float, float]] = []
    block_size = 1
    while n // block_size >= 16:
        n_blocks = n // block_size
        block_means = values[: n_blocks * block_size].reshape(n_blocks, block_size).mean(axis=1)
        tau = 0.5 * block_size * float(np.var(block_means, ddof=1)) / variance
        tau = max(0.0, tau)
        error = tau * math.sqrt(2.0 / (n_blocks - 1.0))
        ladder.append((block_size, tau, error))
        block_size *= 2

    for index in range(len(ladder) - 2):
        triple = ladder[index:index + 3]
        if blocking_plateau_candidate((triple[0], triple[1], triple[2])):
            middle = triple[1]
            return BlockingTauEstimate(
                tau=middle[1], error=middle[2], status="TAU-RESOLVED",
                suggested_block_size=2.0 * middle[1],
                block_size=middle[0], ladder=tuple(ladder),
            )
    terminal = ladder[-1]
    return BlockingTauEstimate(
        tau=terminal[1], error=terminal[2], status="TAU-UNRESOLVED",
        suggested_block_size=2.0 * terminal[1],
        block_size=terminal[0], ladder=tuple(ladder),
    )


def summarize_blocking_taus(estimates: list[BlockingTauEstimate], samples_per_chain: int) -> BlockingTauSummary:
    """Return chain tau spread and I4 status across independent chains.

    Resolved points use the median; if any chain is unresolved, the reported
    tau lower bound is the maximum chain value and Neff is its corresponding
    upper bound, never a median mixing plateau and lower-bound estimates.
    """
    if not estimates:
        raise ValueError("Need at least one blocking tau estimate")
    if samples_per_chain < 1:
        raise ValueError("samples_per_chain must be positive")
    taus = np.array([estimate.tau for estimate in estimates], dtype=float)
    has_unresolved = any(estimate.status == "TAU-UNRESOLVED" for estimate in estimates)
    tau_median = float(np.max(taus) if has_unresolved else np.median(taus))
    tau_min = float(np.min(taus))
    tau_max = float(np.max(taus))
    statuses: list[str] = []
    if has_unresolved:
        statuses.append("TAU-UNRESOLVED")
    if tau_min <= 0.0 or tau_max / tau_min > 2.0:
        statuses.append("TAU-SPREAD-FLAG")
    if not statuses:
        statuses.append("TAU-RESOLVED")
    neff_value = math.inf if tau_median <= 0.0 else float(samples_per_chain) / (2.0 * tau_median)
    return BlockingTauSummary(
        tau_median=tau_median, tau_min=tau_min, tau_max=tau_max,
        neff_median=neff_value,
        status=";".join(statuses),
        suggested_block_size=max(estimate.suggested_block_size for estimate in estimates),
        estimates=tuple(estimates),
    )


def between_chain_error(chain_means: np.ndarray, bootstrap_error: float) -> tuple[float, float]:
    """Return population std(chain means)/sqrt(K) and its error ratio.

    This deliberately tau-free cross-check captures a slow mode whose decay
    time is comparable to the run length, which a within-chain autocorrelation
    estimator can silently underestimate.  It is blind to a drift common to all
    chains, so it never replaces a stationarity/G1b check.
    """
    means = np.asarray(chain_means, dtype=float)
    if means.size < 2 or not np.all(np.isfinite(means)):
        raise ValueError("Between-chain error needs at least two finite chain means")
    error = float(np.std(means, ddof=0) / math.sqrt(means.size))
    ratio = error / bootstrap_error if bootstrap_error > 0.0 and math.isfinite(bootstrap_error) else math.nan
    return error, ratio


def suffix_stationarity_p_from_chains(
    chains: list[np.ndarray], suffix_windows: int
) -> tuple[float, str, int]:
    """Return p/status/remainder for terminal windows.

    Windows are built with forward indices.  Any leading remainder is absorbed
    into the first window, so no samples are silently dropped; the returned
    remainder is printed in the validity table.
    """
    if suffix_windows < 2:
        raise ValueError("suffix_windows must be at least two")
    window_size = min(chain.size for chain in chains) // suffix_windows
    if window_size < 2:
        raise ValueError("Suffix windows are too short")
    values: list[float] = []
    errors: list[float] = []
    remainder = min(chain.size for chain in chains) % suffix_windows
    for index in range(suffix_windows):
        first_end = window_size + remainder
        start = 0 if index == 0 else first_end + (index - 1) * window_size
        end = first_end if index == 0 else start + window_size
        window_means = np.array(
            [np.mean(chain[start:end]) for chain in chains],
            dtype=float,
        )
        _, error = equal_chain_mean_jackknife([np.asarray([mean]) for mean in window_means])
        values.append(float(np.mean(window_means)))
        errors.append(error)
    error_array = np.asarray(errors, dtype=float)
    if np.any(error_array <= 0.0) or not np.all(np.isfinite(error_array)):
        return math.nan, "WINDOW-DEGENERATE", remainder
    return weighted_constant_p(np.asarray(values), error_array), "OK", remainder


def block_bootstrap_strength_ratio_error(
    series: np.ndarray, tau: float, resamples: int, rng: np.random.Generator, quantity: str
) -> float:
    """Autocorrelation-aware block-bootstrap error for a strength ratio quantity.

    Contiguous blocks have width ceil(2*tau_int).  Resampling their second- and
    fourth-moment sums preserves the within-block correlation without materializing
    an N-sample bootstrap trajectory for every replicate.
    """
    block_size = max(1, int(math.ceil(2.0 * tau)))
    n_blocks = series.size // block_size
    if n_blocks < 2:
        raise ValueError("Need at least two autocorrelation-sized blocks for Binder error")
    trimmed = series[: n_blocks * block_size].reshape(n_blocks, block_size)
    second_sums = np.sum(trimmed * trimmed, axis=1)
    fourth_sums = np.sum(trimmed**4, axis=1)
    choices = rng.integers(0, n_blocks, size=(resamples, n_blocks))
    second = np.mean(second_sums[choices], axis=1) / block_size
    fourth = np.mean(fourth_sums[choices], axis=1) / block_size
    ratio = fourth / (second * second)
    replicates = 1.0 - ratio / 3.0 if quantity == "binder-u" else ratio
    return float(np.std(replicates, ddof=1))


def ratio_influence_series(series: np.ndarray) -> np.ndarray:
    """Return the delta-method influence series for r=<P^4>/<P^2>^2.

    A nonlinear ratio is a scalar estimator, not a literal time series.  Its
    autocorrelation time is therefore defined from the first-order influence
    function whose integrated covariance controls the ratio's sampling error.
    Additive constants are omitted because they do not affect autocorrelation.
    """
    values = np.asarray(series, dtype=float)
    second = float(np.mean(values**2))
    fourth = float(np.mean(values**4))
    if not math.isfinite(second) or second <= 0.0:
        raise ValueError("Raw-ratio influence is undefined when <P^2> is non-positive")
    return values**4 / second**2 - 2.0 * fourth * values**2 / second**3


def chain_tau_jackknife(
    chains: list[np.ndarray], transform: Callable[[np.ndarray], np.ndarray]
) -> tuple[float, float, np.ndarray]:
    """Mean chain tau and delete-one-chain jackknife error.

    Independent chains are kept separate so artificial joins cannot create or
    erase long-lag covariance.  The returned uncertainty is the between-chain
    delete-one-chain jackknife error of the mean tau estimate.
    """
    if len(chains) < 2:
        raise ValueError("A chain tau jackknife needs at least two chains")
    chain_taus = np.array(
        [integrated_autocorrelation_time(transform(chain)) for chain in chains],
        dtype=float,
    )
    estimate = float(np.mean(chain_taus))
    replicates = np.array(
        [np.mean(np.delete(chain_taus, index)) for index in range(len(chain_taus))]
    )
    error = math.sqrt(
        (len(chains) - 1.0) / len(chains)
        * float(np.sum((replicates - np.mean(replicates)) ** 2))
    )
    return estimate, error, chain_taus


def block_bootstrap_pooled_raw_ratio(
    chains: list[np.ndarray], tau: float, resamples: int, rng: np.random.Generator
) -> np.ndarray:
    """Hierarchical block-bootstrap replicates of a pooled raw ratio.

    Blocks never cross independent-chain boundaries.  Resampling chains first
    and blocks within each selected chain incorporates both chain-to-chain and
    serial variation while keeping the calculation at the moment-sum level.
    """
    total_samples = sum(chain.size for chain in chains)
    # Never use blocks shorter than 2*tau.  On multi-million-sample series,
    # deliberately use still-longer blocks to cap the resampled block population;
    # this is conservative and keeps the bootstrap computationally bounded.
    block_size = max(
        1, int(math.ceil(2.0 * tau)), int(math.ceil(total_samples / 4096.0))
    )
    second_sums: list[np.ndarray] = []
    fourth_sums: list[np.ndarray] = []
    for chain in chains:
        n_blocks = chain.size // block_size
        if n_blocks < 2:
            raise ValueError("Need at least two autocorrelation-sized blocks per chain")
        blocks = chain[: n_blocks * block_size].reshape(n_blocks, block_size)
        second_sums.append(np.sum(blocks**2, axis=1))
        fourth_sums.append(np.sum(blocks**4, axis=1))
    replicates = np.empty(resamples, dtype=float)
    # Resample chains first, then blocks within each selected chain.  Resampling
    # only from a pooled block bag would erase between-chain dispersion.
    for replicate in range(resamples):
        selected_chains = rng.integers(0, len(chains), size=len(chains))
        second_total = 0.0
        fourth_total = 0.0
        block_count = 0
        for chain_index in selected_chains:
            chain_second = second_sums[int(chain_index)]
            chain_fourth = fourth_sums[int(chain_index)]
            choices = rng.integers(0, chain_second.size, size=chain_second.size)
            second_total += float(np.sum(chain_second[choices]))
            fourth_total += float(np.sum(chain_fourth[choices]))
            block_count += chain_second.size
        second = second_total / (block_count * block_size)
        fourth = fourth_total / (block_count * block_size)
        replicates[replicate] = fourth / second**2
    return replicates


def jackknife_pooled_statistic(
    chains: list[np.ndarray], statistic: Callable[[np.ndarray], float]
) -> tuple[float, float]:
    """Return a pooled statistic and leave-one-chain-out jackknife error."""
    if len(chains) < 2:
        raise ValueError("A chain jackknife needs at least two chains")
    value = float(statistic(np.concatenate(chains)))
    replicates = np.array(
        [
            statistic(np.concatenate(chains[:index] + chains[index + 1 :]))
            for index in range(len(chains))
        ],
        dtype=float,
    )
    error = math.sqrt(
        (len(chains) - 1.0) / len(chains)
        * float(np.sum((replicates - np.mean(replicates)) ** 2))
    )
    return value, error


def weighted_constant_p(values: np.ndarray, errors: np.ndarray) -> float:
    weights = 1.0 / errors**2
    mean = float(np.sum(weights * values) / np.sum(weights))
    statistic = float(np.sum(weights * (values - mean) ** 2))
    return float(chi2.sf(statistic, values.size - 1)) if values.size > 1 else 1.0


def weighted_slope_z(x: np.ndarray, values: np.ndarray, errors: np.ndarray) -> float:
    if values.size < 2:
        return 0.0
    design = np.column_stack((np.ones(values.size), x))
    precision = np.diag(1.0 / errors**2)
    covariance = np.linalg.inv(design.T @ precision @ design)
    coefficients = covariance @ design.T @ precision @ values
    return abs(float(coefficients[1])) / math.sqrt(float(covariance[1, 1]))


def run_depth_ramp(args: argparse.Namespace) -> int:
    """Read exact-depth windows and identify the earliest stationary suffix."""
    paths = expand_inputs(args.paths)
    depths = np.array(args.checkpoint_depths, dtype=float)
    indices = np.array(args.checkpoint_indices, dtype=int)
    if depths.size != indices.size or depths.size < 2:
        raise SystemExit("Checkpoint depths/indices must have the same length >=2")

    grouped: dict[float, dict[str, list[Path]]] = defaultdict(lambda: defaultdict(list))
    for path in paths:
        field = capture_path_value(path, args.field_regex, "field")
        match = re.search(args.group_regex, path.parent.name)
        if match is None or "group" not in match.groupdict():
            raise ValueError(f"{path}: parent does not match group regex {args.group_regex!r}")
        grouped[field][match.group("group")].append(path)

    print(
        "field\tdepth\tratio\tratio_error\tratio_cold_hot_z\t"
        "density_cold_hot_z\tsuffix_constant_p\tsuffix_slope_z\tpass"
    )
    overall_ok = True
    for field in sorted(grouped):
        if sorted(grouped[field]) != ["cold", "hot"]:
            raise ValueError(f"h={field}: expected cold and hot groups")
        if any(len(grouped[field][group]) < args.min_chains_per_group for group in ("cold", "hot")):
            raise ValueError(f"h={field}: fewer than {args.min_chains_per_group} chains/group")

        ratio_values: list[float] = []
        ratio_errors: list[float] = []
        ratio_z: list[float] = []
        density_z: list[float] = []
        for end in indices:
            group_stats: dict[str, dict[str, tuple[float, float]]] = {}
            all_strength: list[np.ndarray] = []
            for group in ("cold", "hot"):
                strengths: list[np.ndarray] = []
                densities: list[np.ndarray] = []
                for path in grouped[field][group]:
                    start = int(end) - args.window_samples
                    strength_stats = read_chain(path, args.observable)
                    density_stats = read_chain(path, args.density_observable)
                    if strength_stats.series is None or density_stats.series is None:
                        raise KeyError(f"{path}: missing depth-ramp series")
                    strengths.append(strength_stats.series[start:int(end)])
                    sigma_z = density_stats.series[start:int(end)]
                    densities.append((1.0 - sigma_z) / 2.0)
                group_stats[group] = {
                    "ratio": jackknife_pooled_statistic(strengths, strength_raw_ratio),
                    "density": jackknife_pooled_statistic(densities, np.mean),
                }
                all_strength.extend(strengths)
            ratio, ratio_error = jackknife_pooled_statistic(all_strength, strength_raw_ratio)
            ratio_values.append(ratio)
            ratio_errors.append(ratio_error)
            ratio_z.append(
                abs(group_stats["cold"]["ratio"][0] - group_stats["hot"]["ratio"][0])
                / math.hypot(group_stats["cold"]["ratio"][1], group_stats["hot"]["ratio"][1])
            )
            density_z.append(
                abs(group_stats["cold"]["density"][0] - group_stats["hot"]["density"][0])
                / math.hypot(group_stats["cold"]["density"][1], group_stats["hot"]["density"][1])
            )

        ratio_array = np.array(ratio_values)
        error_array = np.array(ratio_errors)
        first_pass: float | None = None
        for offset, depth in enumerate(depths):
            constant_p = weighted_constant_p(ratio_array[offset:], error_array[offset:])
            slope_z = weighted_slope_z(depths[offset:], ratio_array[offset:], error_array[offset:])
            passed = (
                max(ratio_z[offset:]) < args.z_threshold
                and max(density_z[offset:]) < args.z_threshold
                and constant_p >= args.stationarity_p
                and slope_z < args.z_threshold
            )
            if passed and first_pass is None:
                first_pass = float(depth)
            print(
                f"{field:.12g}\t{depth:.12g}\t{ratio_array[offset]:.9g}\t"
                f"{error_array[offset]:.9g}\t{ratio_z[offset]:.6g}\t{density_z[offset]:.6g}\t"
                f"{constant_p:.6g}\t{slope_z:.6g}\t{passfail(passed)}"
            )
        if first_pass is None:
            overall_ok = False
            print(f"summary\th={field:.12g}\tearliest_pass=NONE")
        else:
            print(f"summary\th={field:.12g}\tearliest_pass={first_pass:.12g}")
    return 0 if overall_ok else 1


def step2a_metadata(path: Path) -> tuple[int, float, str, str, int]:
    """Parse the locked STEP 2A run-directory naming convention."""
    match = re.search(
        r"(?:^|_)L(?P<L>[0-9]+)_s(?P<s>[0-9]+p[0-9]+)_basis(?P<basis>[xz])_"
        r"(?P<group>cold|hot)(?P<replica>[0-9]+)_",
        path.parent.name,
    )
    if match is None:
        raise ValueError(f"{path}: parent is not a STEP 2A run directory")
    return (
        int(match.group("L")),
        float(match.group("s").replace("p", ".")),
        match.group("basis"),
        match.group("group"),
        int(match.group("replica")),
    )


def independent_chain_mean_error(chain_means: np.ndarray) -> tuple[float, float]:
    """Mean and standard error from independent chain-level means."""
    values = np.asarray(chain_means, dtype=float)
    if values.size < 2 or not np.all(np.isfinite(values)):
        raise ValueError("Need at least two finite independent chain means")
    return float(np.mean(values)), float(np.std(values, ddof=1) / math.sqrt(values.size))


def safe_suffix_tests(
    depths: np.ndarray, values: np.ndarray, errors: np.ndarray, offset: int
) -> tuple[float, float]:
    """Non-degenerate suffix constant-p and slope-z (at least three points)."""
    suffix_values = values[offset:]
    suffix_errors = errors[offset:]
    if suffix_values.size < 3:
        return math.nan, math.nan
    if np.allclose(suffix_values, suffix_values[0], rtol=0.0, atol=1e-15):
        return 1.0, 0.0
    if np.any(suffix_errors <= 0.0) or not np.all(np.isfinite(suffix_errors)):
        return math.nan, math.nan
    return (
        weighted_constant_p(suffix_values, suffix_errors),
        weighted_slope_z(np.log2(depths[offset:]), suffix_values, suffix_errors),
    )


def classify_energy_gap(rows: list[dict[str, float]], z_threshold: float) -> str:
    """Classify depth evolution without converting an unresolved gap into physics."""
    persistent = [row for row in rows if row["cold_hot_z"] >= z_threshold]
    if not persistent:
        return "NO_PERSISTENT_GAP"
    if rows[-1]["cold_hot_z"] < z_threshold:
        return "SHRINKING_TO_ZERO_UNDERTHERMALIZED"
    tail = rows[-3:]
    if len(tail) == 3 and all(row["cold_hot_z"] >= z_threshold for row in tail[-2:]):
        gaps = np.array([abs(row["cold_mean"] - row["hot_mean"]) for row in tail])
        gap_errors = np.array(
            [math.hypot(row["cold_error"], row["hot_error"]) for row in tail]
        )
        cold = np.array([row["cold_mean"] for row in tail])
        cold_errors = np.array([row["cold_error"] for row in tail])
        hot = np.array([row["hot_mean"] for row in tail])
        hot_errors = np.array([row["hot_error"] for row in tail])
        stable = all(
            np.all(errors > 0.0) and weighted_constant_p(values, errors) >= 0.05
            for values, errors in ((gaps, gap_errors), (cold, cold_errors), (hot, hot_errors))
        )
        if stable:
            return "STABLE_SEPARATED_COEXISTENCE_CANDIDATE"
    previous = max(abs(row["cold_mean"] - row["hot_mean"]) for row in rows[:-1])
    final = abs(rows[-1]["cold_mean"] - rows[-1]["hot_mean"])
    if previous > 0.0 and final <= 0.5 * previous:
        return "SHRINKING_UNDERTHERMALIZED"
    return "PERSISTENT_UNCLASSIFIED_DO_NOT_AUTO_EXTEND"


def energy_bimodality_diagnostic(values: np.ndarray) -> tuple[int, float, float, bool]:
    """Conservative KDE screen for histogram candidates requiring visual review."""
    series = np.asarray(values, dtype=float)
    if series.size > 20000:
        series = series[np.linspace(0, series.size - 1, 20000, dtype=int)]
    standard_deviation = float(np.std(series, ddof=1))
    if series.size < 32 or not math.isfinite(standard_deviation) or standard_deviation <= 0.0:
        return 0, math.nan, math.nan, False
    low, high = np.quantile(series, [0.001, 0.999])
    if not math.isfinite(low) or not math.isfinite(high) or high <= low:
        return 0, math.nan, math.nan, False
    grid = np.linspace(low, high, 512)
    density = gaussian_kde(series)(grid)
    peaks, _ = find_peaks(density, prominence=0.08 * float(np.max(density)), distance=20)
    if peaks.size < 2:
        return int(peaks.size), math.nan, math.nan, False
    strongest = peaks[np.argsort(density[peaks])[-2:]]
    left, right = sorted(int(index) for index in strongest)
    separation_sigma = float((grid[right] - grid[left]) / standard_deviation)
    valley_ratio = float(np.min(density[left:right + 1]) / min(density[left], density[right]))
    candidate = separation_sigma >= 0.75 and valley_ratio <= 0.80
    return int(peaks.size), separation_sigma, valley_ratio, candidate


def step2a_final_audit_window(
    window_mode: str, indices: np.ndarray, window_samples: int
) -> tuple[str, int, int]:
    """Select the one final chain-audit window governed by ``--window-mode``."""
    if window_mode == "fixed":
        return "final_fixed", int(indices[-1] - window_samples), int(indices[-1])
    if window_mode == "interval":
        return "final_interval", int(indices[-2]), int(indices[-1])
    raise ValueError(f"Unknown STEP 2A window mode {window_mode!r}")


def run_selfdual_step2a(args: argparse.Namespace) -> int:
    """Read STEP 2A diagnostics; ``--window-mode`` selects its sole final chain audit."""
    paths = expand_inputs(args.paths)
    if not paths:
        raise SystemExit("No selfdual-step2a input files resolved")
    depths = np.asarray(args.checkpoint_depths, dtype=float)
    indices = np.asarray(args.checkpoint_indices, dtype=int)
    if depths.size != indices.size or depths.size < 3:
        raise SystemExit("Checkpoint depths/indices must have the same length >=3")
    if np.any(np.diff(depths) <= 0) or np.any(np.diff(indices) <= 0):
        raise SystemExit("Checkpoint depths and indices must be strictly increasing")
    if args.spread_gate < 0.0:
        raise SystemExit("spread-gate must be non-negative")
    if args.window_mode == "interval":
        starts = np.concatenate((np.array([indices[0] - args.window_samples]), indices[:-1]))
    else:
        starts = indices - args.window_samples
    if np.any(starts < 0) or np.any(starts >= indices):
        raise SystemExit("Checkpoint windows must have positive length and non-negative starts")

    grouped: dict[tuple[int, float], dict[str, dict[str, list[tuple[int, Path]]]]] = defaultdict(
        lambda: defaultdict(lambda: defaultdict(list))
    )
    for path in paths:
        size, field, basis, group, replica = step2a_metadata(path)
        grouped[(size, field)][basis][group].append((replica, path))

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    histogram_dir = output_dir / "energy_histograms"
    if not args.no_histograms:
        histogram_dir.mkdir(parents=True, exist_ok=True)
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    else:
        plt = None

    observables = (
        "energy",
        "percolation_probability",
        "plaquette_percolation_probability",
        "percolation_strength",
        "plaquette_percolation_strength",
    )
    metrics_path = output_dir / "step2a_depth_metrics.tsv"
    duality_path = output_dir / "step2a_duality.tsv"
    gap_path = output_dir / "step2a_energy_gap.tsv"
    bimodality_path = output_dir / "step2a_energy_bimodality.tsv"
    chain_audit_path = output_dir / "step2a_chain_audit.tsv"
    chain_audit_summary_path = output_dir / "step2a_chain_audit_summary.tsv"
    overall_validity = True
    eligible_duality_failures = 0
    bimodality_candidates = 0
    gap_summaries: list[tuple[int, float, str, str]] = []

    with metrics_path.open("w", encoding="utf-8") as metrics_file, \
         duality_path.open("w", encoding="utf-8") as duality_file, \
         gap_path.open("w", encoding="utf-8") as gap_file, \
         bimodality_path.open("w", encoding="utf-8") as bimodality_file, \
         chain_audit_path.open("w", encoding="utf-8") as chain_audit_file, \
         chain_audit_summary_path.open("w", encoding="utf-8") as chain_audit_summary_file:
        metrics_file.write(
            "L\ts\tbasis\tdepth_sweeps\tobservable\tpooled_mean\tpooled_error\t"
            "cold_mean\tcold_error\thot_mean\thot_error\tcold_hot_z\t"
            "suffix_constant_p\tsuffix_slope_z\tsuffix_pass\n"
        )
        duality_file.write(
            "L\ts\tdepth_sweeps\tpair\tz_mean\tz_error\tx_mean\tx_error\t"
            "duality_z\tgate_eligible\tpass\n"
        )
        gap_file.write(
            "L\ts\tbasis\tdepth_sweeps\tcold_mean\tcold_error\thot_mean\t"
            "hot_error\tabs_gap\tcold_hot_z\tpattern\n"
        )
        bimodality_file.write(
            "L\ts\tbasis\tdepth_sweeps\tsamples\tkde_peaks\t"
            "strongest_peak_separation_sigma\tvalley_ratio\tcandidate\n"
        )
        chain_audit_file.write(
            "L\ts\tbasis\twindow\tobservable\tchain\tsamples\traw_ratio\t"
            "raw_ratio_tau_samples\traw_ratio_tau_sweeps\tobservable_zero_fraction\t"
            "audit_status\thdf5_path\n"
        )
        chain_audit_summary_file.write(
            "L\ts\tbasis\twindow\tobservable\tsamples_per_chain\tratio_min\t"
            "ratio_max\tratio_spread\ttau_min_sweeps\ttau_max_sweeps\t"
            "tau_max_min_factor\tscaling_samples_per_chain\t"
            "spread_observed_over_noise_prediction\tobservable_zero_fraction\t"
            "spread_gate_status\ti4_tau_factor2_gate\t"
            "chain_audit_gate\n"
        )

        for (size, field), by_basis in sorted(grouped.items()):
            if sorted(by_basis) != ["x", "z"]:
                raise ValueError(f"L={size}, s={field}: both x and z bases are required")
            point_summaries: dict[str, dict[str, list[dict[str, float]]]] = {}
            point_series: dict[str, dict[str, dict[str, list[np.ndarray]]]] = {}

            for basis in ("z", "x"):
                if sorted(by_basis[basis]) != ["cold", "hot"]:
                    raise ValueError(f"L={size}, s={field}, basis={basis}: cold+hot required")
                for group in ("cold", "hot"):
                    entries = sorted(by_basis[basis][group])
                    replicas = [replica for replica, _ in entries]
                    if replicas != list(range(1, args.expected_chains_per_group + 1)):
                        raise ValueError(
                            f"L={size}, s={field}, basis={basis}, group={group}: "
                            f"replicas={replicas}, expected 1..{args.expected_chains_per_group}"
                        )
                loaded: dict[str, dict[str, list[np.ndarray]]] = {
                    observable: {"cold": [], "hot": []} for observable in observables
                }
                for group in ("cold", "hot"):
                    for _, path in sorted(by_basis[basis][group]):
                        with h5py.File(path, "r") as handle:
                            for observable in observables:
                                series_path = f"{RESULTS_ROOT}/{observable}/series"
                                if series_path not in handle:
                                    raise KeyError(f"{path}: missing {series_path}")
                                loaded[observable][group].append(real_array(handle[series_path]))
                if any(series.size < indices[-1] for obs in loaded.values() for group in obs.values() for series in group):
                    raise ValueError(f"L={size}, s={field}, basis={basis}: a series ends before final checkpoint")
                point_series[basis] = loaded
                point_summaries[basis] = {}

                physical_observable = (
                    "plaquette_percolation_strength"
                    if basis == "z" else "percolation_strength"
                )
                audit_windows = (step2a_final_audit_window(
                    args.window_mode, indices, args.window_samples,
                ),)
                for window_name, audit_start, audit_end in audit_windows:
                    ordered_chains: list[tuple[str, Path, np.ndarray]] = []
                    for group in ("cold", "hot"):
                        entries = sorted(by_basis[basis][group])
                        for chain_index, ((replica, path), series) in enumerate(
                            zip(entries, loaded[physical_observable][group]), start=1
                        ):
                            if replica != chain_index:
                                raise ValueError(
                                    f"L={size}, s={field}, basis={basis}, group={group}: "
                                    "replica ordering changed during chain audit"
                                )
                            ordered_chains.append((f"{group}{replica}", path, series))

                    scaling_samples = min(series.size for _, _, series in ordered_chains)
                    full_chains = [series[:scaling_samples] for _, _, series in ordered_chains]
                    zero_fraction = exact_zero_fraction(full_chains)
                    degenerate = zero_fraction > OBSERVABLE_DEGENERATE_ZERO_FRACTION
                    audit_rows: list[tuple[str, Path, float, float, str]] = []
                    if degenerate:
                        ratio_min = ratio_max = ratio_spread = math.nan
                        tau_min = tau_max = tau_factor = math.nan
                        scaling_ratio = math.nan
                        spread_status = "OBSERVABLE-DEGENERATE"
                        tau_gate = "NA"
                        audit_gate = "OBSERVABLE-DEGENERATE"
                        audit_rows = [
                            (chain_label, path, math.nan, math.nan, "OBSERVABLE-DEGENERATE")
                            for chain_label, path, _ in ordered_chains
                        ]
                    else:
                        audit_ratios: list[float] = []
                        audit_taus: list[float] = []
                        for chain_label, path, series in ordered_chains:
                            window = series[audit_start:audit_end]
                            try:
                                ratio = strength_raw_ratio(window)
                                tau_samples = integrated_autocorrelation_time(
                                    ratio_influence_series(window)
                                )
                            except ValueError:
                                ratio = math.nan
                                tau_samples = math.nan
                            audit_ratios.append(ratio)
                            audit_taus.append(tau_samples * args.sample_period_sweeps)
                            audit_rows.append((chain_label, path, ratio, tau_samples, "OK"))

                        finite_ratios = np.asarray(
                            [value for value in audit_ratios if math.isfinite(value)], dtype=float
                        )
                        finite_taus = np.asarray(
                            [value for value in audit_taus if math.isfinite(value) and value > 0.0],
                            dtype=float,
                        )
                        ratio_min = (
                            float(np.min(finite_ratios))
                            if finite_ratios.size == len(audit_ratios) else math.nan
                        )
                        ratio_max = (
                            float(np.max(finite_ratios))
                            if finite_ratios.size == len(audit_ratios) else math.nan
                        )
                        tau_min = (
                            float(np.min(finite_taus))
                            if finite_taus.size == len(audit_taus) else math.nan
                        )
                        tau_max = (
                            float(np.max(finite_taus))
                            if finite_taus.size == len(audit_taus) else math.nan
                        )
                        tau_factor = (
                            tau_max / tau_min
                            if math.isfinite(tau_min) and tau_min > 0.0 else math.nan
                        )
                        tau_gate = (
                            passfail(tau_factor <= 2.0)
                            if math.isfinite(tau_factor) else "UNDEFINED"
                        )
                        ratio_spread = ratio_max - ratio_min
                        try:
                            _, full_spread = per_chain_raw_ratio_spread(full_chains)
                        except ValueError:
                            full_spread = math.nan
                        spread_status, scaling_ratio = spread_window_scaling_status(
                            ratio_spread, audit_end - audit_start, full_spread,
                            scaling_samples, args.spread_gate,
                        )
                        if spread_status == "PASS":
                            audit_gate = "PASS" if tau_gate == "PASS" else "FAIL"
                        elif spread_status == "FAIL":
                            audit_gate = "FAIL"
                        else:
                            audit_gate = "SPREAD-UNRESOLVED"
                    for chain_label, path, ratio, tau_samples, detail_status in audit_rows:
                        if detail_status == "OBSERVABLE-DEGENERATE":
                            ratio_text = tau_samples_text = tau_sweeps_text = "NA"
                        else:
                            ratio_text = f"{ratio:.12g}"
                            tau_samples_text = f"{tau_samples:.12g}"
                            tau_sweeps_text = (
                                f"{tau_samples * args.sample_period_sweeps:.12g}"
                            )
                        chain_audit_file.write(
                            f"{size}\t{field:.12g}\t{basis}\t{window_name}\t"
                            f"{physical_observable}\t{chain_label}\t{audit_end - audit_start}\t"
                            f"{ratio_text}\t{tau_samples_text}\t{tau_sweeps_text}\t"
                            f"{zero_fraction:.12g}\t{detail_status}\t{path}\n"
                        )
                    chain_audit_summary_file.write(
                        f"{size}\t{field:.12g}\t{basis}\t{window_name}\t"
                        f"{physical_observable}\t{audit_end - audit_start}\t"
                        f"{ratio_min:.12g}\t{ratio_max:.12g}\t"
                        f"{ratio_spread:.12g}\t{tau_min:.12g}\t"
                        f"{tau_max:.12g}\t{tau_factor:.12g}\t{scaling_samples}\t"
                        f"{scaling_ratio:.12g}\t{zero_fraction:.12g}\t{spread_status}\t"
                        f"{tau_gate}\t{audit_gate}\n"
                    )

                for observable in observables:
                    rows: list[dict[str, float]] = []
                    for start, end in zip(starts, indices):
                        start = int(start)
                        group_stats: dict[str, tuple[float, float]] = {}
                        all_chain_means: list[float] = []
                        for group in ("cold", "hot"):
                            chain_means = np.array(
                                [np.mean(series[start:int(end)]) for series in loaded[observable][group]],
                                dtype=float,
                            )
                            group_stats[group] = independent_chain_mean_error(chain_means)
                            all_chain_means.extend(chain_means.tolist())
                        pooled_mean, pooled_error = independent_chain_mean_error(
                            np.asarray(all_chain_means)
                        )
                        denominator = math.hypot(group_stats["cold"][1], group_stats["hot"][1])
                        cold_hot_z = (
                            abs(group_stats["cold"][0] - group_stats["hot"][0]) / denominator
                            if denominator > 0.0 else
                            (0.0 if group_stats["cold"][0] == group_stats["hot"][0] else math.inf)
                        )
                        rows.append({
                            "pooled_mean": pooled_mean, "pooled_error": pooled_error,
                            "cold_mean": group_stats["cold"][0], "cold_error": group_stats["cold"][1],
                            "hot_mean": group_stats["hot"][0], "hot_error": group_stats["hot"][1],
                            "cold_hot_z": cold_hot_z,
                        })
                    values = np.array([row["pooled_mean"] for row in rows])
                    errors = np.array([row["pooled_error"] for row in rows])
                    for offset, (depth, row) in enumerate(zip(depths, rows)):
                        constant_p, slope_z = safe_suffix_tests(depths, values, errors, offset)
                        suffix_pass = (
                            math.isfinite(constant_p) and constant_p >= args.stationarity_p
                            and math.isfinite(slope_z) and slope_z < args.z_threshold
                            and max(item["cold_hot_z"] for item in rows[offset:]) < args.z_threshold
                        )
                        row["suffix_constant_p"] = constant_p
                        row["suffix_slope_z"] = slope_z
                        row["suffix_pass"] = float(suffix_pass)
                        metrics_file.write(
                            f"{size}\t{field:.12g}\t{basis}\t{depth:.12g}\t{observable}\t"
                            f"{row['pooled_mean']:.12g}\t{row['pooled_error']:.12g}\t"
                            f"{row['cold_mean']:.12g}\t{row['cold_error']:.12g}\t"
                            f"{row['hot_mean']:.12g}\t{row['hot_error']:.12g}\t"
                            f"{row['cold_hot_z']:.12g}\t{constant_p:.12g}\t{slope_z:.12g}\t"
                            f"{passfail(suffix_pass)}\n"
                        )
                    point_summaries[basis][observable] = rows

                energy_rows = point_summaries[basis]["energy"]
                pattern = classify_energy_gap(energy_rows, args.z_threshold)
                gap_summaries.append((size, field, basis, pattern))
                for depth, row in zip(depths, energy_rows):
                    gap_file.write(
                        f"{size}\t{field:.12g}\t{basis}\t{depth:.12g}\t"
                        f"{row['cold_mean']:.12g}\t{row['cold_error']:.12g}\t"
                        f"{row['hot_mean']:.12g}\t{row['hot_error']:.12g}\t"
                        f"{abs(row['cold_mean'] - row['hot_mean']):.12g}\t"
                        f"{row['cold_hot_z']:.12g}\t{pattern}\n"
                    )

                energy_series = loaded["energy"]
                chain_items = [
                    (f"{group}{index + 1}", series)
                    for group in ("cold", "hot")
                    for index, series in enumerate(energy_series[group])
                ]
                for depth, start, end in zip(depths, starts, indices):
                    pooled = np.concatenate(
                        [series[int(start):int(end)] for _, series in chain_items]
                    )
                    peak_count, separation, valley_ratio, candidate = energy_bimodality_diagnostic(pooled)
                    bimodality_candidates += int(candidate)
                    bimodality_file.write(
                        f"{size}\t{field:.12g}\t{basis}\t{depth:.12g}\t{pooled.size}\t"
                        f"{peak_count}\t{separation:.12g}\t{valley_ratio:.12g}\t"
                        f"{'REVIEW' if candidate else 'NO_CANDIDATE'}\n"
                    )

                if plt is not None:
                    figure, axes = plt.subplots(4, 2, figsize=(12, 14), constrained_layout=True)
                    colors = plt.cm.tab10(np.linspace(0.0, 0.9, 8))
                    for axis, depth, start, end in zip(axes.flat, depths, starts, indices):
                        start = int(start)
                        windows = [series[start:int(end)] for _, series in chain_items]
                        pooled = np.concatenate(windows)
                        bins = np.histogram_bin_edges(pooled, bins=args.histogram_bins)
                        axis.hist(pooled, bins=bins, density=True, alpha=0.25, color="black", label="pooled")
                        for color, ((label, _), window) in zip(colors, zip(chain_items, windows)):
                            axis.hist(window, bins=bins, density=True, histtype="step", linewidth=1.0,
                                      color=color, label=label)
                        axis.set_title(f"depth={depth:g} sweeps")
                        axis.set_xlabel("energy")
                        axis.set_ylabel("density")
                    axes.flat[0].legend(ncol=3, fontsize=7)
                    figure.suptitle(f"L={size}, s={field:g}, basis={basis}: pooled + per-chain energy")
                    figure.savefig(
                        histogram_dir / f"L{size}_s{field:.3f}_basis{basis}_energy.png", dpi=150
                    )
                    plt.close(figure)

            dual_pairs = (
                ("z_perc_eq_x_plaq", "percolation_probability", "plaquette_percolation_probability"),
                ("z_plaq_eq_x_perc", "plaquette_percolation_probability", "percolation_probability"),
            )
            for offset, depth in enumerate(depths):
                for pair_name, z_observable, x_observable in dual_pairs:
                    z_row = point_summaries["z"][z_observable][offset]
                    x_row = point_summaries["x"][x_observable][offset]
                    denominator = math.hypot(z_row["pooled_error"], x_row["pooled_error"])
                    duality_z = (
                        abs(z_row["pooled_mean"] - x_row["pooled_mean"]) / denominator
                        if denominator > 0.0 else
                        (0.0 if z_row["pooled_mean"] == x_row["pooled_mean"] else math.inf)
                    )
                    eligible = bool(z_row["suffix_pass"] and x_row["suffix_pass"])
                    passed = eligible and duality_z < args.z_threshold
                    if eligible and not passed:
                        eligible_duality_failures += 1
                    duality_file.write(
                        f"{size}\t{field:.12g}\t{depth:.12g}\t{pair_name}\t"
                        f"{z_row['pooled_mean']:.12g}\t{z_row['pooled_error']:.12g}\t"
                        f"{x_row['pooled_mean']:.12g}\t{x_row['pooled_error']:.12g}\t"
                        f"{duality_z:.12g}\t{int(eligible)}\t{passfail(passed) if eligible else 'NOT_GATED'}\n"
                    )
            if not any(
                all(
                    point_summaries[basis][observable][offset]["suffix_pass"]
                    for basis in ("z", "x") for observable in observables
                )
                for offset in range(len(depths) - 2)
            ):
                overall_validity = False

    print(f"depth_metrics={metrics_path}")
    print(f"duality_table={duality_path}")
    print(f"energy_gap_table={gap_path}")
    print(f"energy_bimodality_table={bimodality_path}")
    print(f"chain_audit_table={chain_audit_path}")
    print(f"chain_audit_summary={chain_audit_summary_path}")
    if not args.no_histograms:
        print(f"energy_histograms={histogram_dir}")
    for size, field, basis, pattern in gap_summaries:
        print(f"energy_gap_pattern L={size} s={field:.12g} basis={basis} {pattern}")
    print(f"eligible_duality_failures={eligible_duality_failures}")
    print(f"bimodality_candidates_requiring_visual_review={bimodality_candidates}")
    print(
        "bimodality_rule=PRESENT_STRONG_FIRST_ORDER_EVIDENCE;"
        "ABSENT_INCONCLUSIVE_NEVER_CONTINUOUS_EVIDENCE"
    )
    if eligible_duality_failures:
        return 2
    return 0 if overall_validity else 1


def run_blocking_validity(args: argparse.Namespace) -> int:
    """Print one full-run blocking validity row per (L,h) point.

    The reported existing error is the pooled-mean jackknife path already used
    by the depth-ramp readout; IPS tau is retained as a secondary cross-check.
    The blocking plateau is the primary tau/Neff estimator.  The suffix p-value
    is a constant-fit test over terminal windows of the retained series,
    matching the existing depth-ramp constant-p convention.
    """
    paths = expand_inputs(args.paths)
    if not paths:
        raise SystemExit("No blocking-validity input files resolved")
    grouped: dict[tuple[int, float], dict[str, list[Path]]] = defaultdict(lambda: defaultdict(list))
    for path in paths:
        size = int(capture_path_value(path, args.size_regex, "size"))
        field = capture_path_value(path, args.field_regex, "field")
        match = re.search(args.group_regex, path.parent.name)
        if match is None or "group" not in match.groupdict():
            raise ValueError(f"{path}: parent does not match group regex {args.group_regex!r}")
        grouped[(size, field)][match.group("group")].append(path)

    print(
        "L\th\tstatus\tretained_samples_per_chain\tchains\t"
        "tau_blocking_sweeps\ttau_chain_range_sweeps\tNeff_median\t"
        "existing_pooled_mean_error\tbetween_chain_error\tbetween_to_existing_ratio\t"
        "cold_hot_z\tsuffix_constant_p\tsuffix_status\tsuffix_remainder_samples\t"
        "ips_tau_samples\tchain_tau_breakdown"
    )
    overall_ok = True
    for (size, field), by_group in sorted(grouped.items()):
        if sorted(by_group) != ["cold", "hot"]:
            raise ValueError(f"L={size}, h={field}: expected cold and hot groups")
        if any(len(by_group[group]) != args.expected_chains_per_group for group in ("cold", "hot")):
            raise ValueError(
                f"L={size}, h={field}: expected exactly {args.expected_chains_per_group} cold and hot chains"
            )
        if any(len(by_group[group]) < args.min_chains_per_group for group in ("cold", "hot")):
            raise ValueError(f"L={size}, h={field}: fewer than {args.min_chains_per_group} chains/group")
        ordered_paths = by_group["cold"] + by_group["hot"]
        chain_stats = [read_chain(path, args.observable) for path in ordered_paths]
        if any(stats.series is None for stats in chain_stats):
            raise KeyError(f"L={size}, h={field}: missing raw series")
        chains = [stats.series[args.discard_samples:] for stats in chain_stats if stats.series is not None]
        lengths = {chain.size for chain in chains}
        if len(lengths) != 1 or next(iter(lengths)) < 16:
            raise ValueError(f"L={size}, h={field}: unequal or too-short retained series")
        estimates = [blocking_plateau_tau(chain) for chain in chains]
        summary = summarize_blocking_taus(estimates, next(iter(lengths)))
        ips_taus = np.array([integrated_autocorrelation_time(chain) for chain in chains])
        ips_tau = float(np.mean(ips_taus))
        _, existing_error = equal_chain_mean_jackknife(chains)
        means = np.array([np.mean(chain) for chain in chains], dtype=float)
        chain_error, error_ratio = between_chain_error(means, existing_error)
        group_means: dict[str, tuple[float, float]] = {}
        for group in ("cold", "hot"):
            values = np.array([np.mean(chain) for chain in (chains[:len(by_group["cold"])] if group == "cold" else chains[len(by_group["cold"]):])])
            group_error = float(np.std(values, ddof=1) / math.sqrt(values.size))
            group_means[group] = (float(np.mean(values)), group_error)
        cold_hot_z = (
            abs(group_means["cold"][0] - group_means["hot"][0])
            / math.hypot(group_means["cold"][1], group_means["hot"][1])
            if group_means["cold"][1] > 0.0 and group_means["hot"][1] > 0.0 else math.nan
        )
        suffix_p, suffix_status, suffix_remainder = suffix_stationarity_p_from_chains(
            chains, args.suffix_windows
        )
        point_ok = summary.status == "TAU-RESOLVED"
        overall_ok = overall_ok and point_ok
        period = args.sample_period_sweeps
        tau_text = f"{summary.tau_median * period:.10g}"
        if "TAU-UNRESOLVED" in summary.status:
            tau_text = f">={tau_text}"
        neff_text = f"{summary.neff_median:.10g}"
        if "TAU-UNRESOLVED" in summary.status:
            neff_text = f"<={neff_text} (tau lower bound)"
        breakdown = ";".join(
            f"{group}{index + 1}:{estimate.status}:"
            f"{'>=' if estimate.status == 'TAU-UNRESOLVED' else ''}{estimate.tau:.10g}"
            for group, group_count, offset in (
                ("cold", len(by_group["cold"]), 0),
                ("hot", len(by_group["hot"]), len(by_group["cold"])),
            )
            for index, estimate in enumerate(summary.estimates[offset:offset + group_count])
        )
        print(
            f"{size}\t{field:.12f}\t{summary.status}\t{next(iter(lengths))}\t{len(chains)}\t"
            f"{tau_text}\t"
            f"{summary.tau_min * period:.10g}:{summary.tau_max * period:.10g}\t"
            f"{neff_text}\t{existing_error:.10g}\t{chain_error:.10g}\t"
            f"{error_ratio:.10g}\t{cold_hot_z:.10g}\t{suffix_p:.10g}\t{suffix_status}\t"
            f"{suffix_remainder}\t{ips_tau:.10g}\t{breakdown}"
        )
    return 0 if overall_ok else 1


def pt_rung_series(paths: list[Path], observable: str) -> tuple[int, np.ndarray, dict[float, np.ndarray]]:
    """Reconstruct stationary observable series at every PT ladder rung."""
    size_values = {
        int(capture_path_value(path, r"(?:^|_)L(?P<size>[0-9]+)(?:_|$)", "size"))
        for path in paths
    }
    if len(size_values) != 1:
        raise ValueError("Each pt-binder-anchor input glob must contain exactly one L")
    ladders: list[np.ndarray] = []
    observables: list[np.ndarray] = []
    h_by_index: dict[int, float] = {}
    for path in paths:
        stats = read_chain(path, observable, include_pt_diagnostics=True)
        if (
            stats.pt_initial_ladder_index is None
            or stats.pt_initial_s is None
            or stats.pt_ladder_index_by_sample is None
            or stats.series is None
        ):
            raise KeyError(f"{path}: missing PT diagnostics or observable series")
        index = stats.pt_initial_ladder_index
        h_by_index[index] = stats.pt_initial_s
        ladders.append(stats.pt_ladder_index_by_sample)
        observables.append(stats.series)
    if len(h_by_index) != len(paths):
        raise ValueError("PT files do not provide one unique initial ladder index per rank")
    labels = np.stack(ladders)
    values = np.stack(observables)
    if labels.shape != values.shape:
        raise ValueError("PT ladder and observable series shapes differ")
    sample_index = np.arange(labels.shape[1])
    reconstructed: dict[float, np.ndarray] = {}
    for index, h_value in h_by_index.items():
        owner_counts = np.sum(labels == index, axis=0)
        if not np.all(owner_counts == 1):
            raise ValueError(f"PT rung {index} is missing or duplicated in a sample")
        owners = np.argmax(labels == index, axis=0)
        reconstructed[h_value] = values[owners, sample_index]
    return next(iter(size_values)), np.array(sorted(reconstructed)), reconstructed


def pt_replica_group_rows(
    paths: list[Path], observable: str, quantity: str = "raw-ratio"
) -> list[dict[str, float | int]]:
    """Return fixed-rung cold/hot statistics with replicas as statistical units.

    Each replica contributes one fixed-rung statistic (the default strength raw
    ratio, or its sample mean).  The origin initial spin, encoded in the PT
    run-directory name, is retained while reconstructing the rung series; it
    must not be replaced by within-replica blocks or an autocorrelation model
    for an I5 group gate.
    """
    if len(paths) < 4:
        raise ValueError("Replica gate needs at least two cold and two hot replicas")
    if quantity not in ("mean", "raw-ratio"):
        raise ValueError(f"Unsupported replica statistic {quantity!r}")
    h_by_index: dict[int, float] = {}
    replica_values: list[np.ndarray] = []
    replica_labels: list[np.ndarray] = []
    replica_groups: list[str] = []
    for path in paths:
        match = re.search(r"(?:^|_)init(?P<spin>-?1)(?:_|$)", path.parent.name)
        if match is None:
            raise ValueError(f"{path}: parent must encode origin init+1 or init-1")
        stats = read_chain(path, observable, include_pt_diagnostics=True)
        if (
            stats.pt_initial_ladder_index is None or stats.pt_initial_s is None
            or stats.pt_ladder_index_by_sample is None or stats.series is None
        ):
            raise KeyError(f"{path}: missing PT diagnostics or observable series")
        if stats.pt_ladder_index_by_sample.shape != stats.series.shape:
            raise ValueError(f"{path}: ladder labels and observable series shapes differ")
        index = stats.pt_initial_ladder_index
        if index in h_by_index:
            raise ValueError(f"{path}: duplicate initial ladder index {index}")
        h_by_index[index] = stats.pt_initial_s
        replica_values.append(stats.series)
        replica_labels.append(stats.pt_ladder_index_by_sample)
        replica_groups.append("cold" if match.group("spin") == "1" else "hot")
    if len(h_by_index) != len(paths):
        raise ValueError("PT files do not provide one unique initial ladder index per rank")
    rows: list[dict[str, float | int]] = []
    for rung, field in sorted(h_by_index.items()):
        means_by_group: dict[str, list[float]] = {"cold": [], "hot": []}
        for values, labels, group in zip(replica_values, replica_labels, replica_groups):
            at_rung = values[labels == rung]
            if at_rung.size == 0:
                raise ValueError(f"PT rung {rung}: replica has no fixed-rung samples")
            statistic = float(np.mean(at_rung)) if quantity == "mean" else strength_raw_ratio(at_rung)
            means_by_group[group].append(statistic)
        if min(len(means_by_group["cold"]), len(means_by_group["hot"])) < 2:
            raise ValueError(f"PT rung {rung}: each origin group needs at least two replicas")
        cold_mean, cold_error = independent_chain_mean_error(np.asarray(means_by_group["cold"]))
        hot_mean, hot_error = independent_chain_mean_error(np.asarray(means_by_group["hot"]))
        gap = cold_mean - hot_mean
        gap_error = math.hypot(cold_error, hot_error)
        gap_z = abs(gap) / gap_error if gap_error > 0.0 else (0.0 if gap == 0.0 else math.inf)
        rows.append({
            "rung": rung, "h": field,
            "cold_replicas": len(means_by_group["cold"]), "cold_mean": cold_mean,
            "cold_error": cold_error, "hot_replicas": len(means_by_group["hot"]),
            "hot_mean": hot_mean, "hot_error": hot_error, "gap_cold_minus_hot": gap,
            "gap_replica_error": gap_error, "gap_replica_z": gap_z,
        })
    return rows


def run_pt_replica_gate(args: argparse.Namespace) -> int:
    """Write the default, replica-level I5 cold/hot gate table for a PT run."""
    paths = expand_inputs(args.pt_paths)
    if not paths:
        raise SystemExit("pt-replica-gate needs PT HDF5 inputs")
    rows = pt_replica_group_rows(paths, args.observable, args.quantity)
    output_rows = [
        "rung\th\tcold_replicas\tcold_mean\tcold_replica_error\thot_replicas\t"
        "hot_mean\thot_replica_error\tgap_cold_minus_hot\tgap_replica_error\t"
        "gap_replica_z\tstatistic\tgroup_gate_basis\tgroup_gate_status"
    ]
    for row in rows:
        passed = float(row["gap_replica_z"]) < args.z_threshold
        output_rows.append(
            f"{row['rung']}\t{float(row['h']):.12g}\t{row['cold_replicas']}\t"
            f"{float(row['cold_mean']):.12g}\t{float(row['cold_error']):.12g}\t"
            f"{row['hot_replicas']}\t{float(row['hot_mean']):.12g}\t"
            f"{float(row['hot_error']):.12g}\t{float(row['gap_cold_minus_hot']):.12g}\t"
            f"{float(row['gap_replica_error']):.12g}\t{float(row['gap_replica_z']):.12g}\t"
            f"{args.quantity}\tREPLICA_LEVEL\t{passfail(passed)}"
        )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(output_rows) + "\n", encoding="utf-8")
    print(
        "group_gate_basis=REPLICA_LEVEL "
        f"(between-origin-init replica {args.quantity} values; no tau/block model)"
    )
    print("within_replica_block_errors=NON_GATE_BASIS")
    print(f"table={args.output}")
    print(f"max_gap_replica_z={max(float(row['gap_replica_z']) for row in rows):.12g}")
    return 0


def run_pt_speedup(args: argparse.Namespace) -> int:
    """Compare reconstructed PT rung series with a no-PT blocking baseline."""
    pt_paths = expand_inputs(args.pt_paths)
    no_pt_paths = expand_inputs(args.no_pt_paths)
    if not pt_paths or not no_pt_paths:
        raise SystemExit("PT speedup needs both PT and no-PT HDF5 inputs")
    size, fields, reconstructed = pt_rung_series(pt_paths, args.observable)
    if size != args.system_size:
        raise ValueError(f"PT paths encode L={size}, expected L={args.system_size}")

    grouped: dict[float, list[Path]] = defaultdict(list)
    for path in no_pt_paths:
        field = capture_path_value(path, args.field_regex, "field")
        grouped[field].append(path)
    rng = np.random.default_rng(args.seed)
    baseline: dict[float, dict[str, object]] = {}
    for field, point_paths in sorted(grouped.items()):
        if len(point_paths) < args.min_no_pt_chains:
            raise ValueError(f"h={field}: only {len(point_paths)} no-PT chains")
        chains: list[np.ndarray] = []
        for path in point_paths:
            stats = read_chain(path, args.observable)
            if stats.series is None:
                raise KeyError(f"{path}: missing {args.observable} series")
            chains.append(stats.series[args.no_pt_discard_samples:])
        lengths = {chain.size for chain in chains}
        if len(lengths) != 1 or next(iter(lengths)) < 16:
            raise ValueError(f"h={field}: unequal or too-short no-PT suffixes")
        estimates = [blocking_plateau_tau(chain) for chain in chains]
        summary = summarize_blocking_taus(estimates, next(iter(lengths)))
        chain_ratios = np.asarray([strength_raw_ratio(chain) for chain in chains])
        baseline[field] = {
            "tau": summary.tau_median * args.no_pt_sample_period_sweeps,
            "tau_lower": "TAU-UNRESOLVED" in summary.status,
            "status": summary.status,
            "ratio": pooled_raw_ratio(chains),
            "ratio_error": float(np.std(chain_ratios, ddof=1) / math.sqrt(chain_ratios.size)),
        }

    baseline_fields = np.asarray(sorted(baseline), dtype=float)
    baseline_taus = np.asarray([baseline[field]["tau"] for field in baseline_fields], dtype=float)
    baseline_ratios = np.asarray([baseline[field]["ratio"] for field in baseline_fields], dtype=float)
    baseline_ratio_errors = np.asarray(
        [baseline[field]["ratio_error"] for field in baseline_fields], dtype=float
    )

    def interpolate_baseline(target: float) -> dict[str, object] | None:
        if target < baseline_fields[0] or target > baseline_fields[-1]:
            return None
        if any(field_matches(target, field) for field in baseline_fields):
            field = next(field for field in baseline_fields if field_matches(target, field))
            return baseline[float(field)]
        high = int(np.searchsorted(baseline_fields, target))
        low = high - 1
        weight = (target - baseline_fields[low]) / (baseline_fields[high] - baseline_fields[low])
        return {
            "tau": float((1.0 - weight) * baseline_taus[low] + weight * baseline_taus[high]),
            "tau_lower": bool(
                baseline[float(baseline_fields[low])]["tau_lower"]
                or baseline[float(baseline_fields[high])]["tau_lower"]
            ),
            "status": "LINEAR_INTERPOLATION",
            "ratio": float((1.0 - weight) * baseline_ratios[low] + weight * baseline_ratios[high]),
            "ratio_error": float(math.hypot(
                (1.0 - weight) * baseline_ratio_errors[low],
                weight * baseline_ratio_errors[high],
            )),
        }

    output_rows = [
        "h\tpt_status\tpt_tau_sweeps\tpt_Neff\tno_pt_status\tno_pt_tau_sweeps\t"
        "no_pt_projected_Neff\tspeedup_status\tspeedup\tpt_raw_ratio\tpt_raw_ratio_error\t"
        "no_pt_raw_ratio\tno_pt_raw_ratio_error\traw_ratio_z"
    ]
    physics_rows = [
        "h\tpt_raw_ratio\tpt_error\tno_pt_raw_ratio\tno_pt_error\tz\t"
        "z_threshold\tG1.3_S4_status"
    ]
    matched: list[dict[str, float | str]] = []
    pt_curve: list[tuple[float, float, float]] = []
    for field in fields:
        series = reconstructed[float(field)]
        estimate = blocking_plateau_tau(series)
        tau_sweeps = estimate.tau * args.pt_sample_period_sweeps
        pt_neff = series.size / (2.0 * estimate.tau)
        pt_ratio = strength_raw_ratio(series)
        pt_ratio_error = block_bootstrap_strength_ratio_error(
            series, estimate.tau, args.resamples, rng, "raw-ratio"
        )
        pt_curve.append((float(field), pt_ratio, pt_ratio_error))
        reference = interpolate_baseline(float(field))
        if reference is None:
            output_rows.append(
                f"{field:.12f}\t{estimate.status}\t{tau_sweeps:.12g}\t{pt_neff:.12g}\t"
                f"NO_BASELINE\tNA\tNA\tNO_BASELINE\tNA\t{pt_ratio:.12g}\t"
                f"{pt_ratio_error:.12g}\tNA\tNA\tNA"
            )
            continue
        no_tau = float(reference["tau"])
        no_neff = args.production_sweeps / (2.0 * no_tau)
        pt_lower = estimate.status == "TAU-UNRESOLVED"
        no_lower = bool(reference["tau_lower"])
        if no_lower and pt_lower:
            speedup_status, speedup_text = "INDETERMINATE_BOTH_TAU_LOWER_BOUNDS", "NA"
        elif no_lower:
            speedup_status, speedup_text = "LOWER_BOUND", f"{no_tau / tau_sweeps:.12g}"
        elif pt_lower:
            speedup_status, speedup_text = "UPPER_BOUND", f"{no_tau / tau_sweeps:.12g}"
        else:
            speedup_status, speedup_text = "RESOLVED", f"{no_tau / tau_sweeps:.12g}"
        no_ratio = float(reference["ratio"])
        no_ratio_error = float(reference["ratio_error"])
        ratio_z = abs(pt_ratio - no_ratio) / math.hypot(pt_ratio_error, no_ratio_error)
        output_rows.append(
            f"{field:.12f}\t{estimate.status}\t{tau_sweeps:.12g}\t{pt_neff:.12g}\t"
            f"{reference['status']}\t{no_tau:.12g}\t{no_neff:.12g}\t{speedup_status}\t"
            f"{speedup_text}\t{pt_ratio:.12g}\t{pt_ratio_error:.12g}\t{no_ratio:.12g}\t"
            f"{no_ratio_error:.12g}\t{ratio_z:.12g}"
        )
        physics_rows.append(
            f"{field:.12f}\t{pt_ratio:.12g}\t{pt_ratio_error:.12g}\t"
            f"{no_ratio:.12g}\t{no_ratio_error:.12g}\t{ratio_z:.12g}\t"
            f"{args.physics_z_threshold:.12g}\t{passfail(ratio_z < args.physics_z_threshold)}"
        )
        matched.append({
            "h": float(field), "pt_neff": pt_neff, "no_neff": no_neff,
            "speedup_status": speedup_status,
        })

    args.table_output.parent.mkdir(parents=True, exist_ok=True)
    args.table_output.write_text("\n".join(output_rows) + "\n", encoding="utf-8")
    args.physics_output.write_text("\n".join(physics_rows) + "\n", encoding="utf-8")
    print(f"table={args.table_output}")
    print(f"physics_table={args.physics_output}")
    if physics_rows[1:]:
        physics_statuses = [line.rsplit("\t", 1)[-1] for line in physics_rows[1:]]
        print(
            "G1.3_S4_physics_z_gate="
            f"{passfail(all(status == 'PASS' for status in physics_statuses))} "
            f"threshold=z<{args.physics_z_threshold:g}"
        )
    print(f"matched_rungs={len(matched)} total_rungs={len(fields)}")
    if args.pt_wall_hours is not None and args.no_pt_wall_hours is not None and matched:
        pt_sum = sum(float(row["pt_neff"]) for row in matched)
        no_sum = sum(float(row["no_neff"]) for row in matched)
        pt_rate = pt_sum / (len(fields) * args.pt_wall_hours)
        no_rate = no_sum / (len(fields) * args.no_pt_wall_hours)
        print(
            f"matched_suite_Neff_per_core_hour PT={pt_rate:.12g} noPT={no_rate:.12g} "
            f"ratio={pt_rate / no_rate:.12g}"
        )

    if args.size20_table is not None:
        reference = np.genfromtxt(args.size20_table, names=True, delimiter="\t", comments="#")
        selected = reference[reference["L"] == 20]
        h20 = np.asarray(selected["h"], dtype=float)
        r20 = np.asarray(selected["raw_ratio"], dtype=float)
        pt_h = np.asarray([row[0] for row in pt_curve])
        pt_r = np.asarray([row[1] for row in pt_curve])
        grid = np.unique(np.concatenate((pt_h, h20)))
        grid = grid[(grid >= args.crossing_h_min) & (grid <= args.crossing_h_max)]
        difference = np.interp(grid, pt_h, pt_r) - np.interp(grid, h20, r20)
        brackets = np.flatnonzero(difference[:-1] * difference[1:] <= 0.0)
        if brackets.size == 1:
            index = int(brackets[0])
            crossing = grid[index] - difference[index] * (
                grid[index + 1] - grid[index]
            ) / (difference[index + 1] - difference[index])
            print(
                f"pt_L10_vs_noPT_L20_linear_crossing={crossing:.12g} "
                f"bracket={grid[index]:.12g}:{grid[index + 1]:.12g}"
            )
        else:
            print(f"pt_L10_vs_noPT_L20_linear_crossing=UNRESOLVED brackets={brackets.size}")
    return 0


def run_pt_capability_gate(args: argparse.Namespace) -> int:
    """Apply G2/F6 capability first, then efficiency only to valid references."""
    pt_paths = expand_inputs(args.pt_paths)
    no_pt_paths = expand_inputs(args.no_pt_paths)
    if not pt_paths or not no_pt_paths:
        raise SystemExit("pt-capability-gate needs PT and no-PT HDF5 inputs")
    size, fields, reconstructed = pt_rung_series(pt_paths, args.observable)
    if size != args.system_size:
        raise ValueError(f"PT paths encode L={size}, expected L={args.system_size}")
    if args.spread_gate < 0.0 or args.capability_small_window_samples < 1:
        raise SystemExit("spread-gate and capability-small-window-samples must be positive")

    grouped: dict[float, list[Path]] = defaultdict(list)
    for path in no_pt_paths:
        grouped[capture_path_value(path, args.field_regex, "field")].append(path)
    baseline: dict[float, dict[str, float | str]] = {}
    for field, point_paths in sorted(grouped.items()):
        if len(point_paths) < args.min_no_pt_chains:
            raise ValueError(f"h={field}: only {len(point_paths)} no-PT chains")
        chains = []
        for path in point_paths:
            stats = read_chain(path, args.observable)
            if stats.series is None:
                raise KeyError(f"{path}: missing {args.observable} series")
            chains.append(stats.series[args.no_pt_discard_samples:])
        lengths = {chain.size for chain in chains}
        retained = next(iter(lengths)) if len(lengths) == 1 else 0
        if retained < max(16, args.capability_small_window_samples * 100):
            raise ValueError(
                f"h={field}: retained samples must be >=100x capability-small-window-samples"
            )
        estimates = [blocking_plateau_tau(chain) for chain in chains]
        summary = summarize_blocking_taus(estimates, retained)
        _, full_spread = per_chain_raw_ratio_spread(chains)
        _, small_spread = per_chain_raw_ratio_spread(
            [chain[:args.capability_small_window_samples] for chain in chains]
        )
        spread_status, noise_ratio = spread_window_scaling_status(
            small_spread, args.capability_small_window_samples, full_spread, retained,
            args.spread_gate,
        )
        tau_factor = summary.tau_max / summary.tau_min if summary.tau_min > 0.0 else math.inf
        i4_status = passfail(math.isfinite(tau_factor) and tau_factor <= 2.0)
        capability = "PASS" if spread_status == "PASS" and i4_status == "PASS" else "CAPABILITY_REQUIRED"
        baseline[field] = {
            "spread": full_spread, "spread_status": spread_status,
            "noise_ratio": noise_ratio, "i4_status": i4_status, "tau_factor": tau_factor,
            "capability": capability,
            "tau_sweeps": summary.tau_median * args.no_pt_sample_period_sweeps,
        }

    rows = [
        "h\tstage1_metric\tno_pt_ratio_spread\tspread_gate_status\t"
        "spread_observed_over_noise\ti4_tau_factor\ti4_factor2_status\tstage1_verdict\t"
        "stage2_metric\tpt_tau_sweeps\tno_pt_tau_sweeps\t"
        "cost_normalized_Neff_per_core_hour_ratio\tstage2_verdict"
    ]
    for field in fields:
        matched = next((value for value in baseline if field_matches(float(field), value)), None)
        if matched is None:
            rows.append(f"{field:.12f}\tNO_MATCH\tNA\tNO_BASELINE\tNA\tNA\tNA\tCAPABILITY_REQUIRED\t"
                        "COST_NORMALIZED_NEFF_PER_CORE_HOUR\tNA\tNA\tUNDEFINED\tUNDEFINED")
            continue
        reference = baseline[matched]
        pt_tau = blocking_plateau_tau(reconstructed[float(field)]).tau * args.pt_sample_period_sweeps
        if reference["capability"] != "PASS":
            efficiency, verdict = "UNDEFINED", "UNDEFINED"
        else:
            # Judgment call for PI/Feynman: retain G2's 5x/2x bands for cost-normalized rate.
            efficiency = (float(reference["tau_sweeps"]) / pt_tau) * (
                args.no_pt_core_count / args.pt_core_count
            ) * (args.no_pt_wall_hours / args.pt_wall_hours)
            verdict = "GO" if efficiency >= 5.0 else ("MARGINAL" if efficiency >= 2.0 else "NO-GO")
        rows.append(
            f"{field:.12f}\tPER_CHAIN_RAW_RATIO_SPREAD_PLUS_I4_TAU_FACTOR2\t"
            f"{float(reference['spread']):.12g}\t{reference['spread_status']}\t"
            f"{float(reference['noise_ratio']):.12g}\t{float(reference['tau_factor']):.12g}\t"
            f"{reference['i4_status']}\t{reference['capability']}\t"
            f"COST_NORMALIZED_NEFF_PER_CORE_HOUR\t{pt_tau:.12g}\t"
            f"{float(reference['tau_sweeps']):.12g}\t"
            f"{efficiency if isinstance(efficiency, str) else f'{efficiency:.12g}'}\t{verdict}"
        )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(rows) + "\n", encoding="utf-8")
    print("stage1_metric=PER_CHAIN_RAW_RATIO_SPREAD_PLUS_I4_TAU_FACTOR2")
    print("stage2_metric=COST_NORMALIZED_NEFF_PER_CORE_HOUR")
    print("stage1_failure=CAPABILITY_REQUIRED; stage2_cost_normalization=UNDEFINED")
    print(f"table={args.output}")
    return 0


def run_pt_binder_anchor(args: argparse.Namespace) -> int:
    """Generate the one Linsel PT strength-ratio crossing anchor figure."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    grouped_paths = [expand_inputs(patterns) for patterns in args.paths]
    if any(not paths for paths in grouped_paths):
        raise SystemExit("Each --paths group must resolve to at least one obs.h5 file")
    rng = np.random.default_rng(args.seed)
    curves: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray]] = {}
    quantity_name = "U" if args.quantity == "binder-u" else "r"
    quantity = strength_binder_cumulant if args.quantity == "binder-u" else strength_raw_ratio
    for paths in grouped_paths:
        size, fields, reconstructed = pt_rung_series(paths, args.observable)
        selected = fields[(fields >= args.h_min) & (fields <= args.h_max)]
        cumulants = np.array([quantity(reconstructed[h]) for h in selected])
        taus = np.array([integrated_autocorrelation_time(reconstructed[h]) for h in selected])
        errors = np.array(
            [block_bootstrap_strength_ratio_error(
                reconstructed[h], tau, args.resamples, rng, args.quantity
            )
             for h, tau in zip(selected, taus)]
        )
        curves[size] = (selected, cumulants, errors)
        print(
            f"L={size} " + " ".join(
                f"h={h:.6f}:{quantity_name}={u:.8f}+/-{e:.8f}:tau={tau:.4g}"
                for h, u, e, tau in zip(selected, cumulants, errors, taus)
            )
        )

    if sorted(curves) != [20, 24, 28]:
        raise SystemExit(f"Expected L=20,24,28; found {sorted(curves)}")
    crossings: list[tuple[int, int, float, float]] = []
    for left, right in ((20, 24), (20, 28), (24, 28)):
        h_left, u_left, _ = curves[left]
        h_right, u_right, _ = curves[right]
        overlap_low = max(h_left[0], h_right[0])
        overlap_high = min(h_left[-1], h_right[-1])
        fields = np.unique(np.concatenate((h_left, h_right)))
        fields = fields[(fields >= overlap_low) & (fields <= overlap_high)]
        difference = np.interp(fields, h_left, u_left) - np.interp(fields, h_right, u_right)
        brackets = np.flatnonzero(difference[:-1] * difference[1:] <= 0.0)
        if brackets.size != 1:
            raise ValueError(f"Expected one crossing for L={left}/{right}; found {brackets.size}")
        i = int(brackets[0])
        crossing = fields[i] - difference[i] * (fields[i + 1] - fields[i]) / (difference[i + 1] - difference[i])
        crossing_u = 0.5 * (
            np.interp(crossing, h_left, u_left) + np.interp(crossing, h_right, u_right)
        )
        crossings.append((left, right, float(crossing), float(crossing_u)))
        print(f"crossing L={left}/{right}: h={crossing:.8f} bracket=[{fields[i]:.6f},{fields[i + 1]:.6f}]")

    figure, axis = plt.subplots(figsize=(7.2, 4.8), constrained_layout=True)
    colors = {20: "#0072B2", 24: "#D55E00", 28: "#009E73"}
    for size in (20, 24, 28):
        fields, cumulants, errors = curves[size]
        axis.errorbar(fields, cumulants, yerr=errors, fmt="o-", capsize=3,
                      linewidth=1.6, markersize=4.5, color=colors[size], label=f"L={size}")
    axis.axvspan(0.333, 0.337, color="0.4", alpha=0.18, label=r"Linsel $h_c=0.335(2)$")
    crossing_offsets = ((-54, 36), (0, 19), (54, 2))
    for offset, (left, right, crossing, crossing_u) in enumerate(crossings):
        axis.annotate(f"{left}/{right}: {crossing:.4f}", xy=(crossing, crossing_u),
                      xytext=crossing_offsets[offset], textcoords="offset points", ha="center",
                      fontsize=8, arrowprops={"arrowstyle": "-", "color": "0.3"})
    if args.quantity == "raw-ratio":
        axis.set_yscale("log")
        ylabel = r"Raw ratio $\langle P^4\rangle/\langle P^2\rangle^2$"
        caption = ("ParaToric reproduces the Linsel et al. (PRB 110, L241101) Ising-line "
                   "benchmark: raw ratio <P^4>/<P^2>^2 of plaquette percolation strength "
                   "for L=20/24/28 at lmbda=0.3, beta=L (log scale, matching Linsel Fig.4a convention).")
    else:
        ylabel = r"Binder cumulant $U_P$"
        caption = ("ParaToric reproduces the Linsel et al. (PRB 110, L241101) Ising-line "
                   "benchmark: Binder crossings for L=20/24/28 at lmbda=0.3, beta=L.")
    axis.set(xlim=(args.h_min, args.h_max), xlabel=r"$h$", ylabel=ylabel)
    axis.grid(axis="y", alpha=0.25)
    axis.legend(loc="best", frameon=True)
    figure.text(0.5, -0.06, caption, ha="center", va="top", wrap=True, fontsize=8.5)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=220, bbox_inches="tight")
    print(f"plot: {output}")
    return 0


def run_crossing_extrapolation(args: argparse.Namespace) -> int:
    """Plot a linear finite-size crossing extrapolation in 1/sqrt(L1*L2)."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    parsed: list[tuple[int, int, float]] = []
    for entry in args.crossing:
        parts = entry.split(":")
        if len(parts) != 3:
            raise SystemExit(
                f"Invalid --crossing {entry!r}; expected LEFT_SIZE:RIGHT_SIZE:VALUE"
            )
        left, right = int(parts[0]), int(parts[1])
        value = float(parts[2])
        if left <= 0 or right <= 0 or left == right or not math.isfinite(value):
            raise SystemExit(f"Invalid --crossing {entry!r}")
        parsed.append((left, right, value))
    if len(parsed) < 3:
        raise SystemExit("Crossing extrapolation needs at least three finite-size points")

    x = np.asarray([1.0 / math.sqrt(left * right) for left, right, _ in parsed])
    y = np.asarray([value for _, _, value in parsed])
    design = np.column_stack((np.ones(x.size), x))
    coefficients, _, _, _ = np.linalg.lstsq(design, y, rcond=None)
    residuals = y - design @ coefficients
    degrees_of_freedom = x.size - design.shape[1]
    if degrees_of_freedom <= 0:
        raise SystemExit("Linear-fit error needs at least one residual degree of freedom")
    residual_variance = float(residuals @ residuals / degrees_of_freedom)
    covariance = residual_variance * np.linalg.inv(design.T @ design)
    intercept, slope = (float(value) for value in coefficients)
    intercept_error = math.sqrt(float(covariance[0, 0]))

    figure, axis = plt.subplots(figsize=(7.2, 5.1), constrained_layout=True)
    x_line = np.linspace(0.0, 1.05 * float(np.max(x)), 200)
    axis.axhspan(
        args.reference - args.reference_error,
        args.reference + args.reference_error,
        color="#999999",
        alpha=0.25,
        label=rf"Linsel ${args.reference:.3f}({int(round(args.reference_error * 1000))})$",
    )
    axis.plot(x_line, intercept + slope * x_line, color="#0072B2", label="linear fit")
    axis.scatter(x, y, color="#D55E00", zorder=3, label="finite-$L$ crossings")
    for x_value, y_value, (left, right, _) in zip(x, y, parsed):
        axis.annotate(
            f"{left}/{right}",
            (x_value, y_value),
            xytext=(4, 5),
            textcoords="offset points",
            fontsize=8,
        )
    axis.errorbar(
        [0.0],
        [intercept],
        yerr=[intercept_error],
        fmt="o",
        capsize=4,
        color="#009E73",
        zorder=4,
        label=rf"$h_c(\infty)={intercept:.4f}\pm{intercept_error:.4f}$ (fit SE)",
    )
    axis.set(
        xlabel=r"$1/L_{\mathrm{eff}}$,  $L_{\mathrm{eff}}=\sqrt{L_1L_2}$",
        ylabel="crossing field",
        xlim=(-0.0025, 1.05 * float(np.max(x))),
    )
    axis.grid(alpha=0.25)
    axis.legend(loc="best", fontsize=8.5)
    caption = (
        "Provisional PT-derived crossing set; sizes are L=20/24/28, not Linsel's "
        "L=10/20/30. Three points leave approximately zero redundancy for a linear "
        "fit. This extrapolation does not close the benchmark."
    )
    figure.text(0.5, -0.055, caption, ha="center", va="top", wrap=True, fontsize=8.3)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=220, bbox_inches="tight")
    print("L_eff_convention=geometric_mean sqrt(L1*L2)")
    print(
        f"linear_fit intercept={intercept:.12g} "
        f"intercept_standard_error={intercept_error:.12g} slope={slope:.12g} "
        f"residual_degrees_of_freedom={degrees_of_freedom}"
    )
    print(f"plot: {output}")
    return 0


def run_ising_line_regression(args: argparse.Namespace) -> int:
    """Assert the D4 Ising-line crossing extrapolation against h_c=0.335(2)."""
    parsed: list[tuple[int, int, float, float]] = []
    for entry in args.crossing:
        parts = entry.split(":")
        if len(parts) != 4:
            raise SystemExit(
                f"Invalid --crossing {entry!r}; expected LEFT_SIZE:RIGHT_SIZE:VALUE:ERROR"
            )
        left, right, value, error = int(parts[0]), int(parts[1]), float(parts[2]), float(parts[3])
        if left <= 0 or right <= 0 or left == right or not math.isfinite(value) or error <= 0.0:
            raise SystemExit(f"Invalid --crossing {entry!r}")
        parsed.append((left, right, value, error))
    if len(parsed) < 3:
        raise SystemExit("Ising-line regression needs at least three finite-size crossings")
    x = np.asarray([1.0 / math.sqrt(left * right) ** args.drift_exponent for left, right, _, _ in parsed])
    y = np.asarray([value for _, _, value, _ in parsed])
    errors = np.asarray([error for _, _, _, error in parsed])
    design = np.column_stack((np.ones(x.size), x))
    weights = np.diag(1.0 / errors**2)
    covariance = np.linalg.inv(design.T @ weights @ design)
    intercept, _slope = (float(value) for value in covariance @ design.T @ weights @ y)
    intercept_error = math.sqrt(float(covariance[0, 0]))
    combined_error = math.hypot(intercept_error, args.reference_error)
    z = abs(intercept - args.reference) / combined_error
    status = passfail(z <= args.sigma_threshold)
    print("D4_metric=INVERSE_VARIANCE_WEIGHTED_FIXED_ISING_DRIFT_EXTRAPOLATION")
    print(
        f"D4_intercept={intercept:.12g} D4_intercept_error={intercept_error:.12g} "
        f"reference={args.reference:.12g}+/-{args.reference_error:.12g} "
        f"z={z:.12g} threshold={args.sigma_threshold:.12g} D4_status={status}"
    )
    # Judgment call for PI/Feynman: default three-sigma compatibility band protects the closed benchmark.
    return 0 if status == "PASS" else 1


def chain_jackknife_statistic(
    chains: list[np.ndarray], statistic: Callable[[np.ndarray], float]
) -> tuple[float, float]:
    """Evaluate a pooled statistic with delete-one-independent-chain error."""
    if len(chains) < 2:
        raise ValueError("Chain jackknife needs at least two independent chains")
    value = statistic(np.concatenate(chains))
    replicates = np.asarray(
        [
            statistic(np.concatenate(chains[:index] + chains[index + 1:]))
            for index in range(len(chains))
        ],
        dtype=float,
    )
    error = math.sqrt(
        (len(chains) - 1.0) / len(chains)
        * float(np.sum((replicates - np.mean(replicates)) ** 2))
    )
    return value, error


def run_selfdual_energy_binder(args: argparse.Namespace) -> int:
    """Read STEP 2A energy Binder curves and report every size-pair crossing."""
    paths = expand_inputs(args.paths)
    if not paths:
        raise SystemExit("No selfdual-energy-binder input files resolved")
    grouped: dict[tuple[int, float, str], list[np.ndarray]] = defaultdict(list)
    for path in paths:
        size, field, basis, _, _ = step2a_metadata(path)
        stats = read_chain(path, "energy")
        if stats.series is None:
            raise KeyError(f"{path}: missing energy series")
        end = stats.series.size if args.end_index is None else args.end_index
        if args.start_index < 0 or end <= args.start_index or end > stats.series.size:
            raise ValueError(
                f"{path}: invalid selected window [{args.start_index}:{end}] "
                f"for {stats.series.size} samples"
            )
        grouped[(size, field, basis)].append(stats.series[args.start_index:end])

    stats_by_point: dict[tuple[int, float, str], tuple[float, float]] = {}
    for key, chains in grouped.items():
        if len(chains) != args.expected_chains:
            raise ValueError(f"{key}: found {len(chains)} chains, expected {args.expected_chains}")
        stats_by_point[key] = chain_jackknife_statistic(chains, strength_binder_cumulant)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as handle:
        handle.write("L\ts\tbasis\tenergy_binder_u\tchain_jackknife_error\tchains\n")
        for (size, field, basis), (value, error) in sorted(stats_by_point.items()):
            handle.write(
                f"{size}\t{field:.12g}\t{basis}\t{value:.12g}\t"
                f"{error:.12g}\t{len(grouped[(size, field, basis)])}\n"
            )

    found = 0
    for basis in sorted({basis for _, _, basis in stats_by_point}):
        sizes = sorted({size for size, _, candidate in stats_by_point if candidate == basis})
        for left_index, left in enumerate(sizes):
            for right in sizes[left_index + 1:]:
                fields = sorted(
                    field for size, field, candidate in stats_by_point
                    if size == left and candidate == basis
                )
                roots: list[tuple[float, float, float]] = []
                for low, high in zip(fields, fields[1:]):
                    low_diff = (
                        stats_by_point[(left, low, basis)][0]
                        - stats_by_point[(right, low, basis)][0]
                    )
                    high_diff = (
                        stats_by_point[(left, high, basis)][0]
                        - stats_by_point[(right, high, basis)][0]
                    )
                    if low_diff == 0.0:
                        roots.append((low, low, low))
                    elif low_diff * high_diff <= 0.0:
                        crossing = low - low_diff * (high - low) / (high_diff - low_diff)
                        roots.append((crossing, low, high))
                if roots:
                    for crossing, low, high in roots:
                        print(
                            f"crossing basis={basis} L={left}/{right} "
                            f"s={crossing:.12g} bracket=[{low:.12g},{high:.12g}]"
                        )
                        found += 1
                else:
                    print(
                        f"crossing basis={basis} L={left}/{right} NONE "
                        f"scan=[{fields[0]:.12g},{fields[-1]:.12g}]"
                    )
    print("binder=U_E=1-<E^4>/(3<E^2>^2), uncentered energy")
    print(f"window=[{args.start_index}:{args.end_index if args.end_index is not None else 'end'}]")
    print(f"table: {output}")
    print(f"crossings_found={found}")
    return 0


def capture_path_value(path: Path, pattern: str, name: str) -> float:
    match = re.search(pattern, path.parent.name)
    if match is None or name not in match.groupdict():
        raise ValueError(
            f"{path}: parent directory does not match {name} regex {pattern!r}"
        )
    return float(match.group(name))


def parse_size_field_values(
    entries: list[str], value_name: str
) -> dict[tuple[int, float | None], int]:
    """Parse SIZE:VALUE or SIZE:FIELD:VALUE command-line entries."""
    parsed: dict[tuple[int, float | None], int] = {}
    for entry in entries:
        parts = entry.split(":")
        if len(parts) == 2:
            size_text, value_text = parts
            field = None
        elif len(parts) == 3:
            size_text, field_text, value_text = parts
            field = float(field_text)
        else:
            raise ValueError(
                f"{value_name} entry {entry!r} must be SIZE:VALUE or SIZE:FIELD:VALUE"
            )
        parsed[(int(size_text), field)] = int(value_text)
    return parsed


def parse_size_fields(entries: list[str]) -> list[tuple[int, float]]:
    parsed: list[tuple[int, float]] = []
    for entry in entries:
        parts = entry.split(":")
        if len(parts) != 2:
            raise ValueError(f"exclude entry {entry!r} must be SIZE:FIELD")
        parsed.append((int(parts[0]), float(parts[1])))
    return parsed


def field_matches(left: float, right: float) -> bool:
    return math.isclose(left, right, rel_tol=0.0, abs_tol=5e-13)


def pooled_raw_ratio(chains: list[np.ndarray]) -> float:
    count = sum(chain.size for chain in chains)
    second = sum(float(np.sum(chain**2)) for chain in chains) / count
    fourth = sum(float(np.sum(chain**4)) for chain in chains) / count
    if second <= 0.0:
        raise ValueError("Pooled raw ratio is undefined when <P^2> is non-positive")
    return fourth / second**2


def equal_chain_mean_jackknife(chains: list[np.ndarray]) -> tuple[float, float]:
    means = np.array([np.mean(chain) for chain in chains], dtype=float)
    value = float(np.mean(means))
    replicates = np.array(
        [np.mean(np.delete(means, index)) for index in range(means.size)]
    )
    error = math.sqrt(
        (means.size - 1.0) / means.size
        * float(np.sum((replicates - np.mean(replicates)) ** 2))
    )
    return value, error


def interpolate_replicates(
    target: float, fields: np.ndarray, values: np.ndarray
) -> np.ndarray:
    """Linearly interpolate bootstrap replicates stored as [field, replicate]."""
    if target <= fields[0]:
        return values[0]
    if target >= fields[-1]:
        return values[-1]
    high = int(np.searchsorted(fields, target))
    low = high - 1
    weight = (target - fields[low]) / (fields[high] - fields[low])
    return (1.0 - weight) * values[low] + weight * values[high]


def linear_crossing_with_bootstrap(
    curves: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]],
    h_min: float,
    h_max: float,
    min_bootstrap_fraction: float,
) -> dict[str, object]:
    """Return the data-bracketed linear crossing and its bootstrap replicates."""
    left_fields, left_ratios, _, left_replicates = curves[10]
    right_fields, right_ratios, _, right_replicates = curves[20]
    common_fields = np.unique(np.concatenate((left_fields, right_fields)))
    common_fields = common_fields[(common_fields >= h_min) & (common_fields <= h_max)]
    central_difference = (
        np.interp(common_fields, left_fields, left_ratios)
        - np.interp(common_fields, right_fields, right_ratios)
    )
    brackets = np.flatnonzero(central_difference[:-1] * central_difference[1:] <= 0.0)
    if brackets.size != 1:
        raise ValueError(f"Expected one L=10/20 linear crossing; found {brackets.size}")
    index = int(brackets[0])
    low, high = common_fields[index:index + 2]
    low_difference, high_difference = central_difference[index:index + 2]
    crossing = low - low_difference * (high - low) / (high_difference - low_difference)
    crossing_value = 0.5 * (
        np.interp(crossing, left_fields, left_ratios)
        + np.interp(crossing, right_fields, right_ratios)
    )

    bootstrap_difference = np.stack([
        interpolate_replicates(candidate, left_fields, left_replicates)
        - interpolate_replicates(candidate, right_fields, right_replicates)
        for candidate in common_fields
    ])
    crossing_replicates = np.full(left_replicates.shape[1], np.nan, dtype=float)
    for replicate in range(crossing_replicates.size):
        differences = bootstrap_difference[:, replicate]
        replicate_brackets = np.flatnonzero(differences[:-1] * differences[1:] <= 0.0)
        candidates: list[float] = []
        for replicate_index in replicate_brackets:
            replicate_low = common_fields[replicate_index]
            replicate_high = common_fields[replicate_index + 1]
            difference_low = differences[replicate_index]
            difference_high = differences[replicate_index + 1]
            candidate = replicate_low - difference_low * (
                replicate_high - replicate_low
            ) / (difference_high - difference_low)
            if math.isfinite(float(candidate)):
                candidates.append(float(candidate))
        if candidates:
            crossing_replicates[replicate] = min(
                candidates, key=lambda candidate: abs(candidate - crossing)
            )
    valid = np.isfinite(crossing_replicates)
    valid_fraction = float(np.mean(valid))
    if valid_fraction < min_bootstrap_fraction:
        raise ValueError(
            f"Only {valid_fraction:.3f} bootstrap replicates retain the central crossing bracket"
        )
    return {
        "crossing": float(crossing),
        "crossing_value": float(crossing_value),
        "bootstrap_error": float(np.std(crossing_replicates[valid], ddof=1)),
        "bootstrap_valid_fraction": valid_fraction,
    }


def interpolation_crossing_roots(
    curves: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]],
    scheme: str,
    window: tuple[float, float],
    h_min: float,
    h_max: float,
) -> tuple[float, ...]:
    """Find every crossing for one interpolation scheme and fit window.

    Polynomial fits are deliberately unweighted: this variation diagnoses the
    functional-form/window sensitivity of the two mean curves rather than letting
    L=20's much smaller pointwise errors suppress the visibly nonmonotonic L=10
    curve.  Multiple roots are retained as interpolation ambiguity, not silently
    reduced to a visually preferred branch.
    """
    functions: list[Callable[[np.ndarray | float], np.ndarray | float]] = []
    low_window, high_window = window
    for size in (10, 20):
        fields, ratios, _, _ = curves[size]
        selected = (fields >= low_window - 5e-13) & (fields <= high_window + 5e-13)
        fields = fields[selected]
        ratios = ratios[selected]
        if scheme == "linear":
            function = lambda target, x=fields, y=ratios: np.interp(target, x, y)
        elif scheme == "pchip":
            function = PchipInterpolator(fields, ratios)
        elif scheme == "natural-cubic":
            function = CubicSpline(fields, ratios, bc_type="natural")
        elif scheme.startswith("polynomial-"):
            degree = int(scheme.rsplit("-", 1)[1])
            if fields.size <= degree:
                return ()
            coefficients = np.polyfit(fields, ratios, degree)
            function = lambda target, c=coefficients: np.polyval(c, target)
        else:
            raise ValueError(f"Unknown interpolation scheme {scheme!r}")
        functions.append(function)

    search_low = max(h_min, low_window)
    search_high = min(h_max, high_window)
    grid = np.linspace(search_low, search_high, 20001)
    differences = np.asarray(functions[0](grid) - functions[1](grid), dtype=float)
    brackets = np.flatnonzero(differences[:-1] * differences[1:] <= 0.0)
    roots: list[float] = []
    for index in brackets:
        low = float(grid[index])
        high = float(grid[index + 1])
        if differences[index] == 0.0:
            root = low
        else:
            root = float(brentq(
                lambda target: float(functions[0](target) - functions[1](target)),
                low,
                high,
            ))
        if not roots or not math.isclose(root, roots[-1], rel_tol=0.0, abs_tol=1e-10):
            roots.append(root)
    return tuple(roots)


def interpolation_systematics(
    curves: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]],
    reference_crossing: float,
    h_min: float,
    h_max: float,
) -> dict[str, object]:
    """Vary interpolation family and window; return selected and all-root spans."""
    all_fields = np.unique(np.concatenate((curves[10][0], curves[20][0])))
    if all_fields.size < 7:
        raise ValueError("Interpolation-systematic scan needs at least seven field values")
    windows = (
        ("all", (float(all_fields[0]), float(all_fields[-1]))),
        ("drop-lowest", (float(all_fields[1]), float(all_fields[-1]))),
        ("central-6", (float(all_fields[2]), float(all_fields[-2]))),
    )
    schemes = (
        "linear", "pchip", "natural-cubic",
        "polynomial-1", "polynomial-2", "polynomial-3",
    )
    choices: list[dict[str, object]] = []
    all_roots: list[float] = []
    selected_roots: list[float] = []
    for window_name, window in windows:
        for scheme in schemes:
            roots = interpolation_crossing_roots(curves, scheme, window, h_min, h_max)
            if not roots:
                choices.append({
                    "window": window_name, "scheme": scheme,
                    "selected": math.nan, "roots": (),
                })
                continue
            selected = min(roots, key=lambda root: abs(root - reference_crossing))
            choices.append({
                "window": window_name, "scheme": scheme,
                "selected": float(selected), "roots": roots,
            })
            selected_roots.append(float(selected))
            all_roots.extend(roots)
    if not all_roots:
        raise ValueError("No interpolation-systematic choice produced a crossing")
    return {
        "choices": choices,
        "selected_min": min(selected_roots),
        "selected_max": max(selected_roots),
        "selected_spread": max(selected_roots) - min(selected_roots),
        "all_root_min": min(all_roots),
        "all_root_max": max(all_roots),
        "all_root_spread": max(all_roots) - min(all_roots),
    }


def run_no_pt_benchmark(args: argparse.Namespace) -> int:
    """Extract no-PT tau diagnostics and plot the Linsel L=10/L=20 anchor."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    paths = expand_inputs(args.paths)
    if not paths:
        raise SystemExit("No no-PT benchmark input files resolved")
    discard_map = parse_size_field_values(args.discard_samples, "discard-samples")
    excluded = parse_size_fields(args.exclude)
    grouped: dict[tuple[int, float], list[Path]] = defaultdict(list)
    for path in paths:
        size = int(capture_path_value(path, args.size_regex, "size"))
        field = capture_path_value(path, args.field_regex, "field")
        grouped[(size, field)].append(path)
    sizes = sorted({size for size, _ in grouped})
    if sizes != [10, 20]:
        raise SystemExit(f"Expected L=10,20; found {sizes}")

    rng = np.random.default_rng(args.seed)
    results: list[dict[str, object]] = []
    bootstrap_by_point: dict[tuple[int, float], np.ndarray] = {}
    for (size, field), point_paths in sorted(grouped.items()):
        is_excluded = any(size == candidate and field_matches(field, candidate_field)
                          for candidate, candidate_field in excluded)
        if is_excluded:
            results.append({"size": size, "field": field, "status": "EXCLUDED_VOID"})
            continue
        if len(point_paths) < args.min_chains:
            raise ValueError(f"L={size}, h={field}: only {len(point_paths)} chains")
        specific = next(
            (value for (candidate_size, candidate_field), value in discard_map.items()
             if candidate_size == size and candidate_field is not None
             and field_matches(field, candidate_field)),
            None,
        )
        discard = specific if specific is not None else discard_map.get((size, None))
        if discard is None:
            raise ValueError(f"L={size}, h={field}: no discard-samples rule")

        strengths: list[np.ndarray] = []
        densities: list[np.ndarray] = []
        for path in point_paths:
            strength_stats = read_chain(path, args.observable)
            density_stats = read_chain(path, args.density_observable)
            if strength_stats.series is None or density_stats.series is None:
                raise KeyError(f"{path}: missing benchmark series")
            strengths.append(strength_stats.series[discard:])
            sigma_z = density_stats.series[discard:]
            densities.append((1.0 - sigma_z) / 2.0)
        lengths = {chain.size for chain in strengths + densities}
        if len(lengths) != 1 or next(iter(lengths)) < 4:
            raise ValueError(f"L={size}, h={field}: unequal or too-short retained series")

        ratio_tau, ratio_tau_error, ratio_chain_taus = chain_tau_jackknife(
            strengths, ratio_influence_series
        )
        density_tau, density_tau_error, density_chain_taus = chain_tau_jackknife(
            densities, lambda values: values
        )
        ratio = pooled_raw_ratio(strengths)
        ratio_bootstrap = block_bootstrap_pooled_raw_ratio(
            strengths, ratio_tau, args.resamples, rng
        )
        ratio_error = float(np.std(ratio_bootstrap, ddof=1))
        density, density_error = equal_chain_mean_jackknife(densities)
        bootstrap_by_point[(size, field)] = ratio_bootstrap
        results.append({
            "size": size,
            "field": field,
            "status": "USED",
            "discard": discard,
            "retained": next(iter(lengths)),
            "chains": len(strengths),
            "ratio": ratio,
            "ratio_error": ratio_error,
            "ratio_tau": ratio_tau * args.sample_period_sweeps,
            "ratio_tau_error": ratio_tau_error * args.sample_period_sweeps,
            "density": density,
            "density_error": density_error,
            "density_tau": density_tau * args.sample_period_sweeps,
            "density_tau_error": density_tau_error * args.sample_period_sweeps,
            "ratio_tau_min": float(np.min(ratio_chain_taus)) * args.sample_period_sweeps,
            "ratio_tau_max": float(np.max(ratio_chain_taus)) * args.sample_period_sweeps,
            "density_tau_min": float(np.min(density_chain_taus)) * args.sample_period_sweeps,
            "density_tau_max": float(np.max(density_chain_taus)) * args.sample_period_sweeps,
        })

    used = [result for result in results if result["status"] == "USED"]
    curves: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]] = {}
    for size in sizes:
        size_results = [result for result in used if result["size"] == size]
        fields = np.array([result["field"] for result in size_results], dtype=float)
        ratios = np.array([result["ratio"] for result in size_results], dtype=float)
        errors = np.array([result["ratio_error"] for result in size_results], dtype=float)
        replicates = np.stack([bootstrap_by_point[(size, field)] for field in fields])
        curves[size] = (fields, ratios, errors, replicates)

    primary_crossing = linear_crossing_with_bootstrap(
        curves, args.crossing_h_min, args.crossing_h_max,
        args.min_crossing_bootstrap_fraction,
    )
    crossing = float(primary_crossing["crossing"])
    crossing_value = float(primary_crossing["crossing_value"])
    crossing_error = float(primary_crossing["bootstrap_error"])
    valid_fraction = float(primary_crossing["bootstrap_valid_fraction"])
    primary_systematics = interpolation_systematics(
        curves, crossing, args.crossing_h_min, args.crossing_h_max
    )

    sensitivity_excluded = parse_size_fields(args.sensitivity_exclude)
    sensitivity_curves = curves
    sensitivity_crossing: dict[str, object] | None = None
    sensitivity_systematics: dict[str, object] | None = None
    if sensitivity_excluded:
        sensitivity_curves = {}
        for size, (fields, ratios, errors, replicates) in curves.items():
            keep = np.array([
                not any(
                    size == candidate_size and field_matches(field, candidate_field)
                    for candidate_size, candidate_field in sensitivity_excluded
                )
                for field in fields
            ])
            sensitivity_curves[size] = (
                fields[keep], ratios[keep], errors[keep], replicates[keep]
            )
        sensitivity_crossing = linear_crossing_with_bootstrap(
            sensitivity_curves, args.crossing_h_min, args.crossing_h_max,
            args.min_crossing_bootstrap_fraction,
        )
        sensitivity_systematics = interpolation_systematics(
            sensitivity_curves, float(sensitivity_crossing["crossing"]),
            args.crossing_h_min, args.crossing_h_max,
        )

    derived: dict[str, float] = {
        "crossing": float(crossing),
        "crossing_error": crossing_error,
        "crossing_valid_fraction": valid_fraction,
        "crossing_systematic_min": float(primary_systematics["all_root_min"]),
        "crossing_systematic_max": float(primary_systematics["all_root_max"]),
        "crossing_systematic_spread": float(primary_systematics["all_root_spread"]),
    }

    header = (
        "L\th\tstatus\tdiscard_samples\tretained_samples_per_chain\tchains\t"
        "raw_ratio\traw_ratio_bootstrap_error\traw_ratio_tau_sweeps\t"
        "raw_ratio_tau_jackknife_error_sweeps\tbase_density\tbase_density_jackknife_error\t"
        "base_density_tau_sweeps\tbase_density_tau_jackknife_error_sweeps\t"
        "raw_ratio_chain_tau_range_sweeps\tbase_density_chain_tau_range_sweeps"
    )
    table_lines = [header]
    for result in results:
        if result["status"] != "USED":
            table_lines.append(
                f'{result["size"]}\t{float(result["field"]):.12f}\t{result["status"]}\t'
                + "\t".join(["NONE"] * 13)
            )
            continue
        table_lines.append(
            f'{result["size"]}\t{float(result["field"]):.12f}\tUSED\t{result["discard"]}\t'
            f'{result["retained"]}\t{result["chains"]}\t{float(result["ratio"]):.10g}\t'
            f'{float(result["ratio_error"]):.10g}\t{float(result["ratio_tau"]):.10g}\t'
            f'{float(result["ratio_tau_error"]):.10g}\t{float(result["density"]):.10g}\t'
            f'{float(result["density_error"]):.10g}\t{float(result["density_tau"]):.10g}\t'
            f'{float(result["density_tau_error"]):.10g}\t'
            f'{float(result["ratio_tau_min"]):.10g}:{float(result["ratio_tau_max"]):.10g}\t'
            f'{float(result["density_tau_min"]):.10g}:{float(result["density_tau_max"]):.10g}'
        )
    table_lines.extend([
        f'# crossing_h\t{derived["crossing"]:.10g}\t{derived["crossing_error"]:.10g}',
        f'# crossing_bootstrap_valid_fraction\t{derived["crossing_valid_fraction"]:.10g}',
        f'# crossing_interpolation_all_root_range\t{derived["crossing_systematic_min"]:.10g}\t{derived["crossing_systematic_max"]:.10g}',
        f'# crossing_interpolation_all_root_spread\t{derived["crossing_systematic_spread"]:.10g}',
    ])
    for choice in primary_systematics["choices"]:
        roots = ",".join(f"{root:.10g}" for root in choice["roots"]) or "NONE"
        selected = choice["selected"]
        selected_text = f"{selected:.10g}" if math.isfinite(float(selected)) else "NONE"
        table_lines.append(
            f'# interpolation_with_all_l20\t{choice["window"]}\t{choice["scheme"]}'
            f'\t{selected_text}\t{roots}'
        )
    if sensitivity_crossing is not None and sensitivity_systematics is not None:
        table_lines.extend([
            f'# crossing_without_sensitivity_points_h\t{float(sensitivity_crossing["crossing"]):.10g}'
            f'\t{float(sensitivity_crossing["bootstrap_error"]):.10g}',
            f'# crossing_without_sensitivity_points_bootstrap_valid_fraction\t'
            f'{float(sensitivity_crossing["bootstrap_valid_fraction"]):.10g}',
            f'# crossing_without_sensitivity_points_interpolation_all_root_range\t'
            f'{float(sensitivity_systematics["all_root_min"]):.10g}\t'
            f'{float(sensitivity_systematics["all_root_max"]):.10g}',
            f'# crossing_without_sensitivity_points_interpolation_all_root_spread\t'
            f'{float(sensitivity_systematics["all_root_spread"]):.10g}',
        ])
        for choice in sensitivity_systematics["choices"]:
            roots = ",".join(f"{root:.10g}" for root in choice["roots"]) or "NONE"
            selected = choice["selected"]
            selected_text = f"{selected:.10g}" if math.isfinite(float(selected)) else "NONE"
            table_lines.append(
                f'# interpolation_without_sensitivity_points\t{choice["window"]}'
                f'\t{choice["scheme"]}\t{selected_text}\t{roots}'
            )
    table_output = Path(args.table_output)
    table_output.parent.mkdir(parents=True, exist_ok=True)
    table_output.write_text("\n".join(table_lines) + "\n", encoding="utf-8")
    print("\n".join(table_lines))

    figure, axis = plt.subplots(figsize=(7.2, 4.8), constrained_layout=True)
    colors = {10: "#0072B2", 20: "#D55E00"}
    all_fields_by_size = {
        size: np.array(sorted(field for candidate_size, field in grouped if candidate_size == size))
        for size in sizes
    }
    for size in sizes:
        fields, ratios, errors, _ = curves[size]
        axis.errorbar(fields, ratios, yerr=errors, fmt="o", capsize=3,
                      linewidth=1.6, markersize=4.5, color=colors[size], label=f"L={size}")
        full_fields = all_fields_by_size[size]
        positions = np.array([
            int(np.flatnonzero(np.isclose(full_fields, field, rtol=0.0, atol=5e-13))[0])
            for field in fields
        ])
        segment_start = 0
        for segment_stop in list(np.flatnonzero(np.diff(positions) > 1) + 1) + [fields.size]:
            axis.plot(fields[segment_start:segment_stop], ratios[segment_start:segment_stop],
                      linewidth=1.6, color=colors[size])
            segment_start = int(segment_stop)
    if sensitivity_crossing is not None:
        old_fields, old_ratios, _, _ = sensitivity_curves[20]
        full_fields = curves[20][0]
        positions = np.array([
            int(np.flatnonzero(np.isclose(full_fields, field, rtol=0.0, atol=5e-13))[0])
            for field in old_fields
        ])
        segment_start = 0
        old_label_used = False
        for segment_stop in list(np.flatnonzero(np.diff(positions) > 1) + 1) + [old_fields.size]:
            axis.plot(
                old_fields[segment_start:segment_stop], old_ratios[segment_start:segment_stop],
                linewidth=1.2, linestyle="--", color="0.35", alpha=0.85,
                label="L=20 without two restored points" if not old_label_used else None,
            )
            old_label_used = True
            segment_start = int(segment_stop)
        restored_fields = np.array([
            field for candidate_size, field in sensitivity_excluded if candidate_size == 20
        ])
        restored_indices = np.array([
            int(np.flatnonzero(np.isclose(curves[20][0], field, rtol=0.0, atol=5e-13))[0])
            for field in restored_fields
        ])
        axis.scatter(
            curves[20][0][restored_indices], curves[20][1][restored_indices],
            marker="D", s=52, facecolor=colors[20], edgecolor="black", linewidth=0.8,
            zorder=5, label="Previously excluded; now validated",
        )
    axis.axvspan(0.333, 0.337, color="0.4", alpha=0.18,
                 label=r"Linsel $h_c=0.335(2)$")
    systematic_low = float(primary_systematics["all_root_min"])
    systematic_high = float(primary_systematics["all_root_max"])
    axis.errorbar(
        crossing, crossing_value,
        xerr=np.array([[crossing - systematic_low], [systematic_high - crossing]]),
        fmt="*", ms=11, capsize=4, color="black",
        label=(fr"Restored linear crossing {crossing:.5f}; "
               fr"interpolation range {systematic_low:.5f}--{systematic_high:.5f}"),
    )
    if sensitivity_crossing is not None:
        old_crossing = float(sensitivity_crossing["crossing"])
        axis.text(
            0.51, 0.025,
            f"with all L=20: {crossing:.7f}\nwithout two points: {old_crossing:.7f}",
            transform=axis.transAxes, fontsize=8, ha="left", va="bottom",
        )
    axis.set_yscale("log")
    axis.set(xlim=(args.h_min, args.h_max), xlabel=r"$h$",
             ylabel=r"Raw ratio $\langle P^4\rangle/\langle P^2\rangle^2$")
    axis.grid(axis="y", alpha=0.25)
    axis.legend(loc="best", frameon=True, fontsize=8.5)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=220, bbox_inches="tight")
    print(f"plot: {output}")
    print(f"table: {table_output}")
    return 0


def run_binder_crossing(args: argparse.Namespace) -> int:
    """Locate adjacent-size crossings from raw strength time series.

    The cumulant is evaluated after concatenating equally weighted independent
    chains at a given (L,h).  The reported crossing is a local linear
    interpolation of U_L(h)-U_L'(h); no claim of a precision fit is implied.
    """
    paths = expand_inputs(args.paths)
    if len(paths) < 2:
        raise SystemExit("Need at least two HDF5 paths for a Binder crossing.")

    grouped: dict[tuple[float, float], list[np.ndarray]] = defaultdict(list)
    for path in paths:
        size = capture_path_value(path, args.size_regex, "size")
        field = capture_path_value(path, args.field_regex, "field")
        stats = read_chain(path, args.observable)
        if stats.series is None:
            raise KeyError(f"{path}: missing raw series")
        grouped[(size, field)].append(stats.series)

    cumulants: dict[tuple[float, float], float] = {}
    for key, chains in grouped.items():
        cumulants[key] = strength_binder_cumulant(np.concatenate(chains))

    sizes = sorted({size for size, _ in cumulants})
    fields_by_size = {
        size: sorted(field for candidate_size, field in cumulants if candidate_size == size)
        for size in sizes
    }
    if len(sizes) < 2:
        raise SystemExit("Binder crossing needs at least two system sizes.")
    if any(fields_by_size[size] != fields_by_size[sizes[0]] for size in sizes[1:]):
        raise SystemExit("All sizes need the same field grid for crossing interpolation.")

    print(f"observable: {args.observable}")
    print("binder: U=1-<m^4>/(3<m^2>^2), evaluated from raw series")
    for size in sizes:
        values = " ".join(
            f"h={field:g}:U={cumulants[(size, field)]:.8g}"
            for field in fields_by_size[size]
        )
        n_chains = [len(grouped[(size, field)]) for field in fields_by_size[size]]
        print(f"L={size:g} chains/point={min(n_chains)}-{max(n_chains)} {values}")

    found = 0
    for left_size, right_size in zip(sizes, sizes[1:]):
        fields = fields_by_size[left_size]
        for low, high in zip(fields, fields[1:]):
            low_diff = cumulants[(left_size, low)] - cumulants[(right_size, low)]
            high_diff = cumulants[(left_size, high)] - cumulants[(right_size, high)]
            if low_diff == 0.0:
                crossing = low
            elif low_diff * high_diff > 0.0:
                continue
            else:
                crossing = low - low_diff * (high - low) / (high_diff - low_diff)
            print(
                f"crossing L={left_size:g}/{right_size:g}: h={crossing:.8g} "
                f"bracket=[{low:g},{high:g}] "
                f"Udiff={low_diff:+.6g}->{high_diff:+.6g}"
            )
            found += 1
    if found == 0:
        print("crossing: none bracketed on supplied field grid")
        return 1
    return 0


def synthetic_self_test() -> int:
    def make_chains(means: list[float], n: int = 1000, variance: float = 1.0) -> list[ChainStats]:
        error = math.sqrt(variance / n)
        return [
            ChainStats(
                path=Path(f"synthetic_chain_{idx}.h5"),
                observable="x",
                mean=mean,
                stored_error=error,
                tau=5.0,
                n=n,
                within_variance=variance,
            )
            for idx, mean in enumerate(means)
        ]

    near = make_chains([0.000, 0.004, -0.003, 0.002])
    ten_se = make_chains([0.0, 0.0, 0.0, 10.0 / math.sqrt(1000.0)])
    offset = make_chains([0.0, 0.0, 0.0, 40.0 / math.sqrt(1000.0)])
    near_rhat = gelman_rubin_rhat(near)
    ten_se_rhat = gelman_rubin_rhat(ten_se)
    offset_rhat = gelman_rubin_rhat(offset)
    assert near_rhat is not None and near_rhat < 1.1
    assert offset_rhat is not None and offset_rhat > 1.1

    # Keep N_eff fixed while N grows tenfold: a naive IID stderr makes the
    # apparent disagreement grow as sqrt(N), while the tau correction keeps
    # it invariant.  This guards the exact failure mode that motivated I4/I6.
    def pair(n: int, tau: float) -> tuple[ChainStats, ChainStats]:
        error = 1.0 / math.sqrt(n)
        return (
            ChainStats(Path(f"synthetic_{n}_cold.h5"), "x", 0.0, error, tau, n, 1.0),
            ChainStats(Path(f"synthetic_{n}_hot.h5"), "x", 0.1, error, tau, n, 1.0),
        )

    low_n = pair(100, 5.0)
    high_n = pair(1000, 50.0)
    raw_low = combined_z(*low_n, error_mode="tau-corrected")
    raw_high = combined_z(*high_n, error_mode="tau-corrected")
    corrected_low = combined_z(*low_n, error_mode="naive")
    corrected_high = combined_z(*high_n, error_mode="naive")
    auto_high = combined_z(*high_n, error_mode="auto")
    assert raw_low is not None and raw_high is not None
    assert corrected_low is not None and corrected_high is not None
    assert auto_high is not None
    assert math.isclose(raw_high / raw_low, math.sqrt(10.0), rel_tol=1e-12)
    assert math.isclose(corrected_high, corrected_low, rel_tol=1e-12)
    assert math.isclose(auto_high, raw_high, rel_tol=1e-12)

    # A long AR(1) chain has tau_int=(1+rho)/(2*(1-rho)); the blocking
    # convention should find its asymptotic variance plateau without using IPS.
    rng = np.random.default_rng(20260720)
    rho = 0.8
    ar1 = np.empty(262144, dtype=float)
    ar1[0] = rng.normal()
    noise_scale = math.sqrt(1.0 - rho * rho)
    for index in range(1, ar1.size):
        ar1[index] = rho * ar1[index - 1] + noise_scale * rng.normal()
    blocking = blocking_plateau_tau(ar1)
    known_tau = (1.0 + rho) / (2.0 * (1.0 - rho))
    assert blocking.status == "TAU-RESOLVED"
    assert math.isclose(blocking.tau, known_tau, rel_tol=0.45)
    assert math.isclose(blocking.suggested_block_size, 2.0 * blocking.tau, rel_tol=1e-12)
    ar1_plateau = next(
        tuple(blocking.ladder[index:index + 3])
        for index in range(len(blocking.ladder) - 2)
        if blocking_plateau_candidate(tuple(blocking.ladder[index:index + 3]))
    )
    ar1_p = weighted_constant_p(
        np.array([point[1] for point in ar1_plateau]),
        np.array([point[2] for point in ar1_plateau]),
    )
    assert ar1_p > PLATEAU_CONSTANT_P_THRESHOLD

    unresolved = blocking_plateau_tau(np.arange(16, dtype=float))
    assert unresolved.status == "TAU-UNRESOLVED"
    spread = summarize_blocking_taus([
        BlockingTauEstimate(1.0, 0.1, "TAU-RESOLVED", 2.0, 8, ((8, 1.0, 0.1),)),
        BlockingTauEstimate(3.0, 0.2, "TAU-RESOLVED", 6.0, 8, ((8, 3.0, 0.2),)),
    ], 128)
    assert spread.status == "TAU-SPREAD-FLAG"
    unresolved_summary = summarize_blocking_taus([
        BlockingTauEstimate(1.0, 0.1, "TAU-RESOLVED", 2.0, 8, ((8, 1.0, 0.1),)),
        BlockingTauEstimate(3.0, 0.2, "TAU-UNRESOLVED", 6.0, 8, ((8, 3.0, 0.2),)),
    ], 128)
    assert unresolved_summary.status == "TAU-UNRESOLVED;TAU-SPREAD-FLAG"
    assert unresolved_summary.tau_median == 3.0

    scaling_noise, noise_ratio = spread_window_scaling_status(0.16, 400, 0.01, 64000, 0.05)
    scaling_signal, signal_ratio = spread_window_scaling_status(0.16, 400, 0.16, 64000, 0.05)
    scaling_unresolved, unresolved_ratio = spread_window_scaling_status(0.16, 400, 0.01, 4000, 0.05)
    assert scaling_noise == "PASS" and 0.5 <= noise_ratio <= 2.0
    assert scaling_signal == "FAIL" and signal_ratio > 2.0
    assert scaling_unresolved == "SPREAD-UNRESOLVED" and math.isnan(unresolved_ratio)
    assert exact_zero_fraction([np.array([0.0, 0.0, 1.0])]) > OBSERVABLE_DEGENERATE_ZERO_FRACTION

    synthetic_means = np.array([1.0, 1.2, 0.8, 1.0], dtype=float)
    between_error, between_ratio = between_chain_error(synthetic_means, 0.1)
    expected_between = float(np.std(synthetic_means, ddof=0) / math.sqrt(synthetic_means.size))
    assert math.isclose(between_error, expected_between, rel_tol=1e-12)
    assert math.isclose(between_ratio, expected_between / 0.1, rel_tol=1e-12)

    # Adversarial long-memory AR(1): its terminal three tau estimates
    # have overlapping inflated error bars but span nearly 100% of the median.
    # The flat-trend safeguard must therefore keep this point unresolved.
    adversarial_rng = np.random.default_rng(9)
    adversarial = np.empty(2048, dtype=float)
    adversarial[0] = adversarial_rng.normal()
    for index in range(1, adversarial.size):
        adversarial[index] = 0.99 * adversarial[index - 1] + adversarial_rng.normal()
    adversarial_result = blocking_plateau_tau(adversarial)
    terminal = adversarial_result.ladder[-3:]
    adjacent_overlap = all(
        abs(terminal[index][1] - terminal[index + 1][1])
        <= terminal[index][2] + terminal[index + 1][2]
        for index in range(2)
    )
    terminal_span = (
        max(point[1] for point in terminal) - min(point[1] for point in terminal)
    ) / np.median([point[1] for point in terminal])
    assert adjacent_overlap and terminal_span > 0.25
    assert adversarial_result.status == "TAU-UNRESOLVED"

    # Grace's explicit 1.000, 1.150, 1.242 monotonic-rise counterexample:
    # adjacent error bars overlap, but the existing slope-z test rejects it.
    monotonic = ((1, 1.000, 0.100), (2, 1.150, 0.100), (4, 1.242, 0.100))
    assert not blocking_plateau_candidate(monotonic)
    monotonic_p = weighted_constant_p(
        np.array([point[1] for point in monotonic]),
        np.array([point[2] for point in monotonic]),
    )
    monotonic_slope_z = weighted_slope_z(
        np.log2(np.array([point[0] for point in monotonic], dtype=float)),
        np.array([point[1] for point in monotonic]),
        np.array([point[2] for point in monotonic]),
    )
    assert monotonic_p > PLATEAU_CONSTANT_P_THRESHOLD and monotonic_slope_z >= 1.0
    remainder_chains = [np.random.default_rng(seed).normal(size=103) for seed in range(4)]
    _, _, suffix_remainder = suffix_stationarity_p_from_chains(remainder_chains, 4)
    assert suffix_remainder == 3

    crossing_fields = np.linspace(0.30, 0.38, 9)
    crossing_curves = {
        10: (
            crossing_fields, 1.0 + crossing_fields, np.full(9, 0.01),
            np.tile((1.0 + crossing_fields)[:, None], (1, 8)),
        ),
        20: (
            crossing_fields, 1.0 + 2.0 * crossing_fields - 0.333,
            np.full(9, 0.01),
            np.tile((1.0 + 2.0 * crossing_fields - 0.333)[:, None], (1, 8)),
        ),
    }
    crossing_scan = interpolation_systematics(crossing_curves, 0.333, 0.32, 0.345)
    assert math.isclose(float(crossing_scan["all_root_min"]), 0.333, abs_tol=1e-10)
    assert math.isclose(float(crossing_scan["all_root_max"]), 0.333, abs_tol=1e-10)

    # The self-dual chain audit is intentionally exercised through its CLI
    # arguments because window selection and TSV gates are user-visible behavior.
    with tempfile.TemporaryDirectory(prefix="ensemble_diagnostics_step2a_") as temporary:
        temporary_root = Path(temporary)
        inputs: list[Path] = []
        for basis in ("z", "x"):
            for group in ("cold", "hot"):
                for replica in range(1, 5):
                    run_dir = temporary_root / (
                        f"L8_s0p200_basis{basis}_{group}{replica}_synthetic"
                    )
                    run_dir.mkdir()
                    path = run_dir / "obs.h5"
                    # Four distinct, nonconstant strengths give a spread far above
                    # the default 0.05 gate while keeping all chain taus finite.
                    pattern = np.array([0.0] * (6 - replica) + [1.0] * (2 + replica))
                    series = np.tile(pattern, 8)
                    with h5py.File(path, "w") as handle:
                        results = handle.require_group(RESULTS_ROOT)
                        for observable in (
                            "energy",
                            "percolation_probability",
                            "plaquette_percolation_probability",
                            "percolation_strength",
                            "plaquette_percolation_strength",
                        ):
                            results.create_group(observable).create_dataset("series", data=series)
                    inputs.append(path)

        def step2a_args(output_dir: Path, window_mode: str, spread_gate: float) -> argparse.Namespace:
            return argparse.Namespace(
                paths=[str(path) for path in inputs],
                checkpoint_depths=[100.0, 200.0, 300.0],
                checkpoint_indices=[32, 48, 64],
                window_samples=8,
                sample_period_sweeps=25.0,
                window_mode=window_mode,
                expected_chains_per_group=4,
                z_threshold=3.0,
                stationarity_p=0.05,
                histogram_bins=40,
                no_histograms=True,
                output_dir=str(output_dir),
                spread_gate=spread_gate,
            )

        fixed_before = temporary_root / "fixed_before"
        fixed_after = temporary_root / "fixed_after"
        interval_output = temporary_root / "interval"
        assert run_selfdual_step2a(step2a_args(fixed_before, "fixed", 0.05)) == 0
        assert run_selfdual_step2a(step2a_args(fixed_after, "fixed", 10.0)) == 0
        # The four legacy TSVs must not change when only the audit gate changes.
        legacy_before = {
            name: hashlib.sha256((fixed_before / name).read_bytes()).hexdigest()
            for name in STEP2A_LEGACY_TSV_FILENAMES
        }
        legacy_after = {
            name: hashlib.sha256((fixed_after / name).read_bytes()).hexdigest()
            for name in STEP2A_LEGACY_TSV_FILENAMES
        }
        assert legacy_before == STEP2A_LEGACY_SELF_TEST_SHA256
        assert legacy_before == legacy_after
        fixed_summary = (fixed_before / "step2a_chain_audit_summary.tsv").read_text(
            encoding="utf-8"
        ).splitlines()
        assert all(line.split("\t")[3] == "final_fixed" for line in fixed_summary[1:])
        assert all(line.split("\t")[15] == "SPREAD-UNRESOLVED" for line in fixed_summary[1:])
        assert all(line.split("\t")[-1] == "SPREAD-UNRESOLVED" for line in fixed_summary[1:])

        assert run_selfdual_step2a(step2a_args(interval_output, "interval", 0.05)) == 0
        interval_summary = (interval_output / "step2a_chain_audit_summary.tsv").read_text(
            encoding="utf-8"
        ).splitlines()
        assert all(line.split("\t")[3] == "final_interval" for line in interval_summary[1:])

        # A degenerate point must suppress every per-chain ratio/tau and must
        # never label a detail row OK merely because the point-level gate fired.
        for path in inputs:
            with h5py.File(path, "a") as handle:
                for observable in ("percolation_strength", "plaquette_percolation_strength"):
                    dataset = handle[f"{RESULTS_ROOT}/{observable}/series"]
                    dataset[...] = 0.0
        degenerate_output = temporary_root / "degenerate"
        assert run_selfdual_step2a(step2a_args(degenerate_output, "fixed", 0.05)) == 0
        degenerate_summary = (
            degenerate_output / "step2a_chain_audit_summary.tsv"
        ).read_text(encoding="utf-8").splitlines()[1:]
        degenerate_detail = (
            degenerate_output / "step2a_chain_audit.tsv"
        ).read_text(encoding="utf-8").splitlines()[1:]
        assert all(line.split("\t")[-1] == "OBSERVABLE-DEGENERATE" for line in degenerate_summary)
        assert all(
            line.split("\t")[7:10] == ["NA", "NA", "NA"]
            and line.split("\t")[11] == "OBSERVABLE-DEGENERATE"
            for line in degenerate_detail
        )

    print(f"synthetic_near_identical_Rhat={near_rhat:.8f}")
    print(f"synthetic_one_chain_10se_Rhat={ten_se_rhat:.8f}")
    print(f"synthetic_one_chain_40se_Rhat={offset_rhat:.8f}")
    print(f"synthetic_naive_z_N100={raw_low:.8f}")
    print(f"synthetic_naive_z_N1000={raw_high:.8f}")
    print(f"synthetic_tau_corrected_z_N100={corrected_low:.8f}")
    print(f"synthetic_tau_corrected_z_N1000={corrected_high:.8f}")
    print(f"synthetic_ar1_blocking_tau={blocking.tau:.8f}")
    print(f"synthetic_ar1_known_tau={known_tau:.8f}")
    print(f"synthetic_ar1_plateau_constant_p={ar1_p:.8f}")
    print(f"synthetic_short_chain_status={unresolved.status}")
    print(f"synthetic_tau_spread_status={spread.status}")
    print(f"synthetic_between_chain_error={between_error:.8f}")
    print(f"synthetic_adversarial_status={adversarial_result.status}")
    print(f"synthetic_adversarial_terminal_span={terminal_span:.8f}")
    print("synthetic_monotonic_24pct_status=REJECTED")
    print(f"synthetic_monotonic_24pct_constant_p={monotonic_p:.8f}")
    print(f"synthetic_monotonic_24pct_slope_z={monotonic_slope_z:.8f}")
    print(f"synthetic_unresolved_max_tau={unresolved_summary.tau_median:.8f}")
    print(f"synthetic_suffix_remainder_samples={suffix_remainder}")
    print(f"synthetic_crossing_systematic_spread={crossing_scan['all_root_spread']:.8g}")
    print("synthetic_step2a_window_selection=PASS")
    print("synthetic_step2a_spread_scaling=PASS")
    print("synthetic_step2a_observable_degenerate=PASS")
    print(
        "synthetic_step2a_legacy_tsv_sha256_values="
        + ",".join(legacy_before[name] for name in STEP2A_LEGACY_TSV_FILENAMES)
    )
    print("synthetic_step2a_legacy_tsv_sha256=PASS")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="ParaToric cross-chain ensemble diagnostics for harness I4/I5."
    )
    subparsers = parser.add_subparsers(dest="command")

    ensemble = subparsers.add_parser("ensemble", help="I4 cross-chain diagnostics")
    ensemble.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    ensemble.add_argument("-o", "--observables", nargs="+", help="Observables to check")
    ensemble.add_argument("--samples", type=int, help="N_samples override when series is absent")
    ensemble.add_argument("--min-chains", type=int, default=4)
    ensemble.add_argument("--z-threshold", type=float, default=3.0)
    ensemble.add_argument("--rhat-threshold", type=float, default=1.1)
    ensemble.add_argument("--tau-ratio-threshold", type=float, default=2.0)
    ensemble.add_argument("--samples-per-tau", type=float, default=100.0)
    ensemble.add_argument("--min-neff", type=float, default=30.0)
    ensemble.add_argument(
        "--stored-error",
        choices=("auto", "tau-corrected", "naive"),
        default="auto",
        help="Interpret mean_error; auto recognizes ParaToric's stationary-bootstrap error.",
    )
    ensemble.set_defaults(func=run_ensemble)

    cold_hot = subparsers.add_parser("cold-hot", help="I5 cold-vs-hot mean agreement")
    cold_hot.add_argument("--cold", required=True, help="Cold-start HDF5 path")
    cold_hot.add_argument("--hot", required=True, help="Hot-start HDF5 path")
    cold_hot.add_argument("-o", "--observables", nargs="+", help="Observables to check")
    cold_hot.add_argument("--samples", type=int, help="N_samples override when series is absent")
    cold_hot.add_argument("--z-threshold", type=float, default=2.0)
    cold_hot.add_argument(
        "--stored-error",
        choices=("auto", "tau-corrected", "naive"),
        default="auto",
        help="Interpret mean_error; auto recognizes ParaToric's stationary-bootstrap error.",
    )
    cold_hot.set_defaults(func=run_cold_hot)

    depth_ramp = subparsers.add_parser(
        "depth-ramp", help="Exact-depth cold/hot and suffix-stationarity readout"
    )
    depth_ramp.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    depth_ramp.add_argument("--checkpoint-depths", nargs="+", type=float, required=True)
    depth_ramp.add_argument("--checkpoint-indices", nargs="+", type=int, required=True)
    depth_ramp.add_argument("--window-samples", type=int, default=1000)
    depth_ramp.add_argument("--observable", default="plaquette_percolation_strength")
    depth_ramp.add_argument("--density-observable", default="sigma_z")
    depth_ramp.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9.]+)(?:_|$)"
    )
    depth_ramp.add_argument(
        "--group-regex", default=r"_(?P<group>cold|hot)[0-9]+_"
    )
    depth_ramp.add_argument("--min-chains-per-group", type=int, default=4)
    depth_ramp.add_argument("--z-threshold", type=float, default=3.0)
    depth_ramp.add_argument("--stationarity-p", type=float, default=0.05)
    depth_ramp.set_defaults(func=run_depth_ramp)

    step2a = subparsers.add_parser(
        "selfdual-step2a",
        help="Self-dual both-basis depth, duality, energy-gap, and histogram readout",
    )
    step2a.add_argument("paths", nargs="+", help="STEP 2A HDF5 paths or shell-style globs")
    step2a.add_argument("--checkpoint-depths", nargs="+", type=float, required=True)
    step2a.add_argument("--checkpoint-indices", nargs="+", type=int, required=True)
    step2a.add_argument("--window-samples", type=int, default=400)
    step2a.add_argument("--sample-period-sweeps", type=float, default=25.0)
    step2a.add_argument(
        "--window-mode", choices=("interval", "fixed"), default="interval",
        help="Use interval or fixed windows for both depth diagnostics and the sole final chain audit",
    )
    step2a.add_argument(
        "--spread-gate", type=float, default=0.05,
        help=("Raw-spread severity threshold after the >=100x 1/sqrt(window) "
              "power check rejects estimation noise (default: 0.05)"),
    )
    step2a.add_argument("--expected-chains-per-group", type=int, default=4)
    step2a.add_argument("--z-threshold", type=float, default=3.0)
    step2a.add_argument("--stationarity-p", type=float, default=0.05)
    step2a.add_argument("--histogram-bins", type=int, default=40)
    step2a.add_argument("--no-histograms", action="store_true")
    step2a.add_argument("--output-dir", required=True)
    step2a.set_defaults(func=run_selfdual_step2a)

    blocking_validity = subparsers.add_parser(
        "blocking-validity",
        help="Full-run blocking tau, tau-free error, cold/hot, and suffix validity table",
    )
    blocking_validity.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    blocking_validity.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength",
        help="Observable with a raw series (default: plaquette_percolation_strength)",
    )
    blocking_validity.add_argument("--discard-samples", type=int, default=0)
    blocking_validity.add_argument("--sample-period-sweeps", type=float, default=25.0)
    blocking_validity.add_argument("--resamples", type=int, default=1000)
    blocking_validity.add_argument("--seed", type=int, default=20260720)
    blocking_validity.add_argument("--suffix-windows", type=int, default=4)
    blocking_validity.add_argument("--min-chains-per-group", type=int, default=4)
    blocking_validity.add_argument("--expected-chains-per-group", type=int, default=4)
    blocking_validity.add_argument(
        "--size-regex", default=r"(?:^|_)L(?P<size>[0-9]+)(?:_|$)",
    )
    blocking_validity.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9]+(?:[.][0-9]+)?)(?:_|$)",
    )
    blocking_validity.add_argument(
        "--group-regex", default=r"_(?P<group>cold|hot)[0-9]+_",
    )
    blocking_validity.set_defaults(func=run_blocking_validity)

    binder = subparsers.add_parser(
        "binder-crossing", help="Strength Binder-cumulant crossings from raw series"
    )
    binder.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    binder.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength",
        help="Strength observable with a raw series (default: plaquette_percolation_strength)",
    )
    binder.add_argument(
        "--size-regex", default=r"(?:^|_)L(?P<size>[0-9]+)(?:_|$)",
        help="Regex matching parent directory with named capture (?P<size>...)",
    )
    binder.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9]+(?:[.][0-9]+)?)(?:_|$)",
        help="Regex matching parent directory with named capture (?P<field>...)",
    )
    binder.set_defaults(func=run_binder_crossing)

    anchor = subparsers.add_parser(
        "pt-binder-anchor", help="Plot autocorrelation-aware PT Binder crossings for Linsel anchor"
    )
    anchor.add_argument(
        "--paths", action="append", nargs="+", required=True,
        help="One quoted obs.h5 glob per L; provide three groups for L=20,24,28.",
    )
    anchor.add_argument("--output", required=True, help="Static PNG or PDF output path")
    anchor.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength",
        help="Strength observable with a raw series (default: plaquette_percolation_strength)",
    )
    anchor.add_argument(
        "--quantity", choices=("binder-u", "raw-ratio"), default="binder-u",
        help="Plot U=1-r/3 (default) or raw r=<P^4>/<P^2>^2 on a log scale.",
    )
    anchor.add_argument("--h-min", type=float, default=0.30)
    anchor.add_argument("--h-max", type=float, default=0.38)
    anchor.add_argument("--resamples", type=int, default=1000)
    anchor.add_argument("--seed", type=int, default=20260716)
    anchor.set_defaults(func=run_pt_binder_anchor)

    extrapolation = subparsers.add_parser(
        "crossing-extrapolation",
        help="Plot a linear crossing extrapolation versus 1/sqrt(L1*L2)",
    )
    extrapolation.add_argument(
        "--crossing", action="append", required=True,
        help="Finite-size point as LEFT_SIZE:RIGHT_SIZE:VALUE; repeat at least three times",
    )
    extrapolation.add_argument("--reference", type=float, default=0.335)
    extrapolation.add_argument("--reference-error", type=float, default=0.002)
    extrapolation.add_argument("--output", required=True)
    extrapolation.set_defaults(func=run_crossing_extrapolation)

    ising_regression = subparsers.add_parser(
        "ising-line-regression",
        help="D4 fixed-Ising-exponent crossing regression against h_c=0.335(2)",
    )
    ising_regression.add_argument(
        "--crossing", action="append", required=True,
        help="Finite-size point LEFT_SIZE:RIGHT_SIZE:VALUE:ERROR; repeat at least three times",
    )
    ising_regression.add_argument("--drift-exponent", type=float, default=2.4166)
    ising_regression.add_argument("--reference", type=float, default=0.335)
    ising_regression.add_argument("--reference-error", type=float, default=0.002)
    ising_regression.add_argument("--sigma-threshold", type=float, default=3.0)
    ising_regression.set_defaults(func=run_ising_line_regression)

    energy_binder = subparsers.add_parser(
        "selfdual-energy-binder",
        help="STEP 2A energy-Binder curves and all size-pair crossing brackets",
    )
    energy_binder.add_argument(
        "paths", nargs="+", help="STEP 2A HDF5 paths or shell-style globs",
    )
    energy_binder.add_argument("--start-index", type=int, default=0)
    energy_binder.add_argument("--end-index", type=int)
    energy_binder.add_argument("--expected-chains", type=int, default=8)
    energy_binder.add_argument("--output", required=True)
    energy_binder.set_defaults(func=run_selfdual_energy_binder)

    speedup = subparsers.add_parser(
        "pt-speedup", help="Fixed-rung blocking-tau PT versus no-PT comparison"
    )
    speedup.add_argument("--pt-paths", nargs="+", required=True)
    speedup.add_argument("--no-pt-paths", nargs="+", required=True)
    speedup.add_argument("--table-output", required=True, type=Path)
    speedup.add_argument("--physics-output", required=True, type=Path)
    speedup.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength"
    )
    speedup.add_argument("--system-size", type=int, default=10)
    speedup.add_argument("--pt-sample-period-sweeps", type=float, default=2.0)
    speedup.add_argument("--no-pt-sample-period-sweeps", type=float, default=25.0)
    speedup.add_argument("--no-pt-discard-samples", type=int, required=True)
    speedup.add_argument("--production-sweeps", type=float, required=True)
    speedup.add_argument("--min-no-pt-chains", type=int, default=8)
    speedup.add_argument("--resamples", type=int, default=1000)
    speedup.add_argument("--seed", type=int, default=20260721)
    speedup.add_argument("--physics-z-threshold", type=float, default=3.0)
    speedup.add_argument("--pt-wall-hours", type=float)
    speedup.add_argument("--no-pt-wall-hours", type=float)
    speedup.add_argument("--size20-table", type=Path)
    speedup.add_argument("--crossing-h-min", type=float, default=0.30)
    speedup.add_argument("--crossing-h-max", type=float, default=0.35)
    speedup.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9]+(?:[.][0-9]+)?)(?:_|$)"
    )
    speedup.set_defaults(func=run_pt_speedup)

    capability = subparsers.add_parser(
        "pt-capability-gate",
        help="G2/F6 capability-first PT gate; efficiency is undefined for broken no-PT references",
    )
    capability.add_argument("--pt-paths", nargs="+", required=True)
    capability.add_argument("--no-pt-paths", nargs="+", required=True)
    capability.add_argument("--output", required=True, type=Path)
    capability.add_argument("-o", "--observable", default="plaquette_percolation_strength")
    capability.add_argument("--system-size", type=int, default=10)
    capability.add_argument("--pt-sample-period-sweeps", type=float, default=100.0)
    capability.add_argument("--no-pt-sample-period-sweeps", type=float, default=25.0)
    capability.add_argument("--no-pt-discard-samples", type=int, required=True)
    capability.add_argument("--min-no-pt-chains", type=int, default=8)
    capability.add_argument("--capability-small-window-samples", type=int, default=9600)
    capability.add_argument("--spread-gate", type=float, default=0.05)
    capability.add_argument("--pt-core-count", type=float, required=True)
    capability.add_argument("--no-pt-core-count", type=float, default=1.0)
    capability.add_argument("--pt-wall-hours", type=float, default=1.0)
    capability.add_argument("--no-pt-wall-hours", type=float, default=1.0)
    capability.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9]+(?:[.][0-9]+)?)(?:_|$)"
    )
    capability.set_defaults(func=run_pt_capability_gate)

    replica_gate = subparsers.add_parser(
        "pt-replica-gate",
        help="Default replica-level fixed-rung cold/hot gate for PT output",
    )
    replica_gate.add_argument("--pt-paths", nargs="+", required=True)
    replica_gate.add_argument("--output", required=True, type=Path)
    replica_gate.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength"
    )
    replica_gate.add_argument(
        "--quantity", choices=("raw-ratio", "mean"), default="raw-ratio",
        help="Per-replica fixed-rung statistic; raw-ratio matches PT speedup/readout.",
    )
    replica_gate.add_argument("--z-threshold", type=float, default=3.0)
    replica_gate.set_defaults(func=run_pt_replica_gate)

    no_pt = subparsers.add_parser(
        "no-pt-benchmark",
        help="No-PT L=10/L=20 tau table and autocorrelation-aware Linsel anchor",
    )
    no_pt.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    no_pt.add_argument("--output", required=True, help="Static PNG or PDF output path")
    no_pt.add_argument("--table-output", required=True, help="TSV diagnostics output path")
    no_pt.add_argument(
        "-o", "--observable", default="plaquette_percolation_strength",
        help="Strength observable with a raw series",
    )
    no_pt.add_argument("--density-observable", default="sigma_z")
    no_pt.add_argument(
        "--size-regex", default=r"(?:^|_)L(?P<size>[0-9]+)(?:_|$)",
    )
    no_pt.add_argument(
        "--field-regex", default=r"(?:^|_)h(?P<field>[0-9]+(?:[.][0-9]+)?)(?:_|$)",
    )
    no_pt.add_argument(
        "--discard-samples", action="append", default=[], required=True,
        help="SIZE:SAMPLES default or SIZE:FIELD:SAMPLES override; repeat as needed",
    )
    no_pt.add_argument(
        "--exclude", action="append", default=[],
        help="Exclude a void SIZE:FIELD point from tau, curve, and crossing",
    )
    no_pt.add_argument(
        "--sensitivity-exclude", action="append", default=[],
        help=("Mark a valid SIZE:FIELD point for a with/without crossing comparison; "
              "the primary curve retains it"),
    )
    no_pt.add_argument("--sample-period-sweeps", type=float, default=25.0)
    no_pt.add_argument("--min-chains", type=int, default=4)
    no_pt.add_argument("--resamples", type=int, default=1000)
    no_pt.add_argument("--seed", type=int, default=20260720)
    no_pt.add_argument("--h-min", type=float, default=0.30)
    no_pt.add_argument("--h-max", type=float, default=0.35)
    no_pt.add_argument("--crossing-h-min", type=float, default=0.32)
    no_pt.add_argument("--crossing-h-max", type=float, default=0.345)
    no_pt.add_argument("--hc-field", type=float, default=0.335362847648)
    no_pt.add_argument("--min-crossing-bootstrap-fraction", type=float, default=0.5)
    no_pt.set_defaults(func=run_no_pt_benchmark)

    self_test = subparsers.add_parser("self-test", help="Run synthetic diagnostics assertions")
    self_test.set_defaults(func=lambda _args: synthetic_self_test())

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if not hasattr(args, "func"):
        parser.print_help()
        return 2
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
