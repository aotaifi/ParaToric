#!/usr/bin/env python3
"""Offline parallel-tempering ladder diagnostics for ParaToric HDF5 output."""

from __future__ import annotations

import argparse
import glob
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import h5py
import numpy as np


PT_GROUP = "/simulation/diagnostics/parallel_tempering"
TRAJECTORY_NAME = "ladder_index_by_sample"


@dataclass(frozen=True)
class ReplicaDiagnostics:
    path: Path
    rank: int | None
    initial_ladder_index: int | None
    final_ladder_index: int | None
    world_size: int
    sample_count: int
    visited_low: bool
    visited_high: bool
    visited_both: bool
    endpoint_span_count: int
    reference_low_to_high_count: int
    strict_roundtrips: int
    tau_ergodic_sweeps: list[float]
    n_up: list[int]
    n_down: list[int]


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


def scalar_int(group: h5py.Group, name: str) -> int | None:
    if name not in group:
        return None
    return int(np.asarray(group[name][()]).item())


def read_ladder_trajectory(path: Path) -> tuple[np.ndarray, int | None, int | None, int | None, int | None]:
    with h5py.File(path, "r") as handle:
        if PT_GROUP not in handle:
            raise KeyError(f"{path}: missing {PT_GROUP}")
        group = handle[PT_GROUP]
        if TRAJECTORY_NAME not in group:
            raise KeyError(f"{path}: missing {PT_GROUP}/{TRAJECTORY_NAME}")
        trajectory = np.asarray(group[TRAJECTORY_NAME][()], dtype=int)
        rank = scalar_int(group, "rank")
        world_size = scalar_int(group, "world_size")
        initial = scalar_int(group, "initial_ladder_index")
        final = scalar_int(group, "final_ladder_index")
    return trajectory, rank, world_size, initial, final


def infer_world_size(trajectory: np.ndarray, h5_world_size: int | None, explicit_world_size: int | None) -> int:
    if explicit_world_size is not None:
        world_size = explicit_world_size
    elif h5_world_size is not None:
        world_size = h5_world_size
    else:
        world_size = int(np.max(trajectory)) + 1
    if world_size < 2:
        raise ValueError(f"world_size must be >=2, got {world_size}")
    if np.any(trajectory < 0) or np.any(trajectory >= world_size):
        raise ValueError("trajectory contains ladder indices outside [0, world_size)")
    return world_size


def endpoint_span_count(trajectory: np.ndarray, top: int) -> int:
    """Count endpoint-to-opposite-end spans, less strict than a closed cycle."""
    last_endpoint: int | None = None
    count = 0
    for value in trajectory:
        rung = int(value)
        if rung != 0 and rung != top:
            continue
        if last_endpoint is None:
            last_endpoint = rung
        elif rung != last_endpoint:
            count += 1
            last_endpoint = rung
    return count


def reference_low_to_high_count(trajectory: np.ndarray, top: int) -> int:
    """Match the older coupledpinning-style non-strict low->high endpoint count."""
    went_low = False
    count = 0
    for value in trajectory:
        rung = int(value)
        if rung == 0:
            went_low = True
        elif rung == top and went_low:
            count += 1
            went_low = False
    return count


def strict_roundtrip_sample_gaps(trajectory: np.ndarray, top: int) -> list[int]:
    """Count closed endpoint cycles: low->high->low or high->low->high.

    This is the offline analogue of the coupledpinning visited_both_ranks
    pattern, generalized symmetrically so either endpoint can be the anchor.
    """
    anchor_endpoint: int | None = None
    saw_opposite = False
    start_sample: int | None = None
    gaps: list[int] = []

    for sample_index, value in enumerate(trajectory):
        rung = int(value)
        if rung != 0 and rung != top:
            continue
        if anchor_endpoint is None:
            anchor_endpoint = rung
            start_sample = sample_index
            saw_opposite = False
            continue
        if rung != anchor_endpoint:
            saw_opposite = True
            continue
        if saw_opposite and start_sample is not None:
            gaps.append(sample_index - start_sample)
            start_sample = sample_index
            saw_opposite = False
    return gaps


def flow_counts(trajectory: np.ndarray, world_size: int) -> tuple[list[int], list[int]]:
    """Count crossings through each rung toward higher or lower ladder index."""
    n_up = [0 for _ in range(world_size)]
    n_down = [0 for _ in range(world_size)]
    for left, right in zip(trajectory[:-1], trajectory[1:]):
        a = int(left)
        b = int(right)
        if a == b:
            continue
        step = 1 if b > a else -1
        for rung in range(a + step, b + step, step):
            if step > 0:
                n_up[rung] += 1
            else:
                n_down[rung] += 1
    return n_up, n_down


def analyze_trajectory(
    path: Path,
    sample_period_sweeps: float,
    explicit_world_size: int | None = None,
) -> ReplicaDiagnostics:
    trajectory, rank, h5_world_size, initial, final = read_ladder_trajectory(path)
    world_size = infer_world_size(trajectory, h5_world_size, explicit_world_size)
    top = world_size - 1
    gaps = strict_roundtrip_sample_gaps(trajectory, top)
    n_up, n_down = flow_counts(trajectory, world_size)
    visited_low = bool(np.any(trajectory == 0))
    visited_high = bool(np.any(trajectory == top))
    return ReplicaDiagnostics(
        path=path,
        rank=rank,
        initial_ladder_index=initial,
        final_ladder_index=final,
        world_size=world_size,
        sample_count=int(trajectory.size),
        visited_low=visited_low,
        visited_high=visited_high,
        visited_both=visited_low and visited_high,
        endpoint_span_count=endpoint_span_count(trajectory, top),
        reference_low_to_high_count=reference_low_to_high_count(trajectory, top),
        strict_roundtrips=len(gaps),
        tau_ergodic_sweeps=[gap * sample_period_sweeps for gap in gaps],
        n_up=n_up,
        n_down=n_down,
    )


def sample_period_sweeps(args: argparse.Namespace) -> float:
    if args.sample_period_sweeps is not None:
        value = args.sample_period_sweeps
    else:
        if args.system_size is None or args.n_between_samples is None:
            raise SystemExit(
                "Pass --sample-period-sweeps or both --system-size and --n-between-samples."
            )
        value = float(args.n_between_samples) / (2.0 * float(args.system_size) ** 2)
    if not math.isfinite(value) or value <= 0.0:
        raise SystemExit("sample period in sweeps must be finite and positive.")
    return value


def format_float(value: float | None, digits: int = 6) -> str:
    if value is None:
        return "NA"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.{digits}g}"


def run_analyze(args: argparse.Namespace) -> int:
    paths = expand_inputs(args.paths)
    if not paths:
        raise SystemExit("No HDF5 paths matched.")
    period = sample_period_sweeps(args)
    replicas = [analyze_trajectory(path, period, args.world_size) for path in paths]
    world_sizes = {replica.world_size for replica in replicas}
    if len(world_sizes) != 1:
        raise SystemExit(f"Mixed world sizes found: {sorted(world_sizes)}")
    world_size = replicas[0].world_size
    total_strict = sum(replica.strict_roundtrips for replica in replicas)
    total_span = sum(replica.endpoint_span_count for replica in replicas)
    total_reference = sum(replica.reference_low_to_high_count for replica in replicas)
    visited_both = sum(1 for replica in replicas if replica.visited_both)
    all_tau = [tau for replica in replicas for tau in replica.tau_ergodic_sweeps]
    total_up = [sum(replica.n_up[rung] for replica in replicas) for rung in range(world_size)]
    total_down = [sum(replica.n_down[rung] for replica in replicas) for rung in range(world_size)]
    mean_tau = float(np.mean(all_tau)) if all_tau else None
    median_tau = float(np.median(all_tau)) if all_tau else None

    print("formula: strict cycles are closed endpoint visits low->high->low or high->low->high")
    print(f"sample_period_sweeps={format_float(period)}")
    print(f"replicas={len(replicas)} world_size={world_size}")
    print(
        f"summary strict_roundtrips={total_strict} endpoint_spans={total_span} "
        f"reference_low_to_high={total_reference} visited_both_replicas={visited_both}"
    )
    print(
        f"summary tau_ergodic_sweeps_mean={format_float(mean_tau)} "
        f"median={format_float(median_tau)} count={len(all_tau)}"
    )
    print("rung\tn_up\tn_down\tf_up")
    for rung, up, down in zip(range(world_size), total_up, total_down):
        visits = up + down
        f_up = float(up) / float(visits) if visits else None
        print(f"{rung}\t{up}\t{down}\t{format_float(f_up)}")
    print("replica\trank\tinitial\tfinal\tvisited_low\tvisited_high\tvisited_both\tendpoint_spans\treference_low_to_high\tstrict_roundtrips\ttau_ergodic_sweeps")
    for idx, replica in enumerate(sorted(replicas, key=lambda item: (-1 if item.rank is None else item.rank, str(item.path)))):
        taus = ",".join(format_float(tau) for tau in replica.tau_ergodic_sweeps) or "NA"
        print(
            f"{idx}\t{replica.rank if replica.rank is not None else 'NA'}\t"
            f"{replica.initial_ladder_index if replica.initial_ladder_index is not None else 'NA'}\t"
            f"{replica.final_ladder_index if replica.final_ladder_index is not None else 'NA'}\t"
            f"{int(replica.visited_low)}\t{int(replica.visited_high)}\t{int(replica.visited_both)}\t"
            f"{replica.endpoint_span_count}\t{replica.reference_low_to_high_count}\t"
            f"{replica.strict_roundtrips}\t{taus}"
        )
    return 0


def synthetic_self_test() -> int:
    top = 3
    period = 2.5
    trajectory = np.array([1, 0, 1, 3, 2, 0, 1, 3, 2, 0, 1, 3, 3], dtype=int)
    gaps = strict_roundtrip_sample_gaps(trajectory, top)
    assert gaps == [4, 4], gaps
    assert endpoint_span_count(trajectory, top) == 5
    assert reference_low_to_high_count(trajectory, top) == 3
    up, down = flow_counts(trajectory, top + 1)
    assert up == [0, 3, 3, 3], up
    assert down == [3, 2, 2, 0], down
    taus = [gap * period for gap in gaps]
    assert taus == [10.0, 10.0], taus
    print("synthetic_strict_roundtrips=2")
    print("synthetic_endpoint_spans=5")
    print("synthetic_reference_low_to_high=3")
    print("synthetic_tau_ergodic_sweeps=10,10")
    print("synthetic_flow_n_up=0,3,3,3")
    print("synthetic_flow_n_down=3,2,2,0")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Offline ParaToric PT ladder trajectory diagnostics."
    )
    subparsers = parser.add_subparsers(dest="command")

    analyze = subparsers.add_parser("analyze", help="Analyze PT HDF5 ladder trajectories")
    analyze.add_argument("paths", nargs="+", help="HDF5 paths or shell-style globs")
    analyze.add_argument("--world-size", type=int, help="Override ladder world size")
    analyze.add_argument("--sample-period-sweeps", type=float, help="Sweeps between stored trajectory samples")
    analyze.add_argument("--system-size", type=int, help="L for square lattice; used with --n-between-samples")
    analyze.add_argument("--n-between-samples", type=int, help="Raw Metropolis updates between stored samples")
    analyze.set_defaults(func=run_analyze)

    self_test = subparsers.add_parser("self-test", help="Run synthetic PT trajectory assertions")
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
