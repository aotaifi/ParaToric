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
import math
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import h5py
import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[1]
SHARED_QMC = REPO_ROOT.parents[0] / "ai-workflow" / "qmc_common"
if str(SHARED_QMC) not in sys.path:
    sys.path.insert(0, str(SHARED_QMC))

from hdf5_readers import read_paratoric_observable, real_array  # noqa: E402


RESULTS_ROOT = "/simulation/results"


@dataclass(frozen=True)
class ChainStats:
    path: Path
    observable: str
    mean: float
    stored_error: float
    tau: float
    n: int | None
    within_variance: float | None


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


def read_chain(path: Path, observable: str, samples_override: int | None = None) -> ChainStats:
    with h5py.File(path, "r") as handle:
        group = handle[RESULTS_ROOT]
        data = read_paratoric_observable(group, observable)
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
    return ChainStats(path, observable, mean, error, tau, n_int, within_variance)


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


def capture_path_value(path: Path, pattern: str, name: str) -> float:
    match = re.search(pattern, path.parent.name)
    if match is None or name not in match.groupdict():
        raise ValueError(
            f"{path}: parent directory does not match {name} regex {pattern!r}"
        )
    return float(match.group(name))


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
        with h5py.File(path, "r") as handle:
            series_path = f"{RESULTS_ROOT}/{args.observable}/series"
            if series_path not in handle:
                raise KeyError(f"{path}: missing raw series {series_path}")
            grouped[(size, field)].append(real_array(handle[series_path]))

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
    print(f"synthetic_near_identical_Rhat={near_rhat:.8f}")
    print(f"synthetic_one_chain_10se_Rhat={ten_se_rhat:.8f}")
    print(f"synthetic_one_chain_40se_Rhat={offset_rhat:.8f}")
    print(f"synthetic_naive_z_N100={raw_low:.8f}")
    print(f"synthetic_naive_z_N1000={raw_high:.8f}")
    print(f"synthetic_tau_corrected_z_N100={corrected_low:.8f}")
    print(f"synthetic_tau_corrected_z_N1000={corrected_high:.8f}")
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

    self_test = subparsers.add_parser("self-test", help="Run synthetic R-hat assertions")
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
