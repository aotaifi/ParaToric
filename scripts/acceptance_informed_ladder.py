#!/usr/bin/env python3
"""Build a larger PT ladder from a measured physical-coordinate acceptance profile.

For neighboring rungs, use the Gaussian-overlap approximation

    acceptance ~= erfc(sigma_delta / 2),
    sigma_delta ~= dh * sqrt(Var(E)).

The inferred local thermodynamic metric is therefore
``2 * erfcinv(acceptance) / dh``.  Its logarithm is smoothed with a Gaussian
kernel in the physical ladder coordinate, then the requested number of rungs is
placed at equal increments of integrated metric.  Unlike PM redistribution,
the output rung count may be larger than the measured input count.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
from pathlib import Path
from typing import Sequence


def parse_float_list(text: str) -> list[float]:
    path = Path(text)
    raw = path.read_text(encoding="utf-8") if path.exists() else text
    values = [float(part) for part in raw.replace(",", " ").split()]
    if not values:
        raise ValueError("empty numeric list")
    return values


def validate(ladder: Sequence[float], acceptance: Sequence[float], target_count: int) -> None:
    if len(ladder) < 2:
        raise ValueError("ladder must contain at least two rungs")
    if len(acceptance) != len(ladder) - 1:
        raise ValueError("acceptance length must equal len(ladder)-1")
    if target_count < 2:
        raise ValueError("target_count must be at least two")
    if any(not math.isfinite(value) for value in ladder):
        raise ValueError("ladder contains a non-finite value")
    if any(right <= left for left, right in zip(ladder, ladder[1:])):
        raise ValueError("ladder must be strictly increasing")
    if any(not math.isfinite(value) or value < 0.0 or value > 1.0 for value in acceptance):
        raise ValueError("acceptances must be finite and in [0, 1]")


def erfcinv_unit(probability: float) -> float:
    """Return erfc^-1(p) for p in (0, 1), using monotone bisection."""
    if not 0.0 < probability < 1.0:
        raise ValueError("erfcinv_unit requires 0 < p < 1")
    lo = 0.0
    hi = 1.0
    while math.erfc(hi) > probability:
        hi *= 2.0
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        if math.erfc(mid) > probability:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def inferred_metric(
    ladder: Sequence[float],
    acceptance: Sequence[float],
    *,
    acceptance_floor: float,
    acceptance_ceiling: float,
) -> tuple[list[float], list[float]]:
    midpoints: list[float] = []
    metric: list[float] = []
    for left, right, raw_acceptance in zip(ladder, ladder[1:], acceptance):
        clipped = min(max(raw_acceptance, acceptance_floor), acceptance_ceiling)
        sigma_delta = 2.0 * erfcinv_unit(clipped)
        midpoints.append(0.5 * (left + right))
        metric.append(sigma_delta / (right - left))
    return midpoints, metric


def smooth_log_metric(
    x: float,
    midpoints: Sequence[float],
    metric: Sequence[float],
    bandwidth: float,
) -> float:
    weighted_sum = 0.0
    weight_sum = 0.0
    for midpoint, value in zip(midpoints, metric):
        distance = (x - midpoint) / bandwidth
        weight = math.exp(-0.5 * distance * distance)
        weighted_sum += weight * math.log(value)
        weight_sum += weight
    if weight_sum <= 0.0:
        raise ValueError("smoothing produced zero total weight")
    return math.exp(weighted_sum / weight_sum)


def build_ladder(
    ladder: Sequence[float],
    acceptance: Sequence[float],
    *,
    target_count: int,
    bandwidth: float | None = None,
    grid_size: int = 20001,
    acceptance_floor: float = 1.0e-4,
    acceptance_ceiling: float = 0.999,
) -> dict[str, object]:
    validate(ladder, acceptance, target_count)
    if grid_size < 1001:
        raise ValueError("grid_size must be at least 1001")
    if not 0.0 < acceptance_floor < acceptance_ceiling < 1.0:
        raise ValueError("acceptance clipping must satisfy 0 < floor < ceiling < 1")

    gaps = [right - left for left, right in zip(ladder, ladder[1:])]
    if bandwidth is None:
        bandwidth = statistics.median(gaps)
    if bandwidth <= 0.0:
        raise ValueError("bandwidth must be positive")

    midpoints, raw_metric = inferred_metric(
        ladder,
        acceptance,
        acceptance_floor=acceptance_floor,
        acceptance_ceiling=acceptance_ceiling,
    )
    lo = float(ladder[0])
    hi = float(ladder[-1])
    dx = (hi - lo) / (grid_size - 1)
    coordinates = [lo + idx * dx for idx in range(grid_size)]
    density = [smooth_log_metric(x, midpoints, raw_metric, bandwidth) for x in coordinates]

    cumulative = [0.0]
    for left_density, right_density in zip(density, density[1:]):
        cumulative.append(cumulative[-1] + 0.5 * (left_density + right_density) * dx)
    total_length = cumulative[-1]
    if total_length <= 0.0:
        raise ValueError("integrated metric is not positive")

    proposed = [lo]
    grid_index = 1
    for rung_index in range(1, target_count - 1):
        target = total_length * rung_index / (target_count - 1)
        while cumulative[grid_index] < target:
            grid_index += 1
        c0 = cumulative[grid_index - 1]
        c1 = cumulative[grid_index]
        fraction = (target - c0) / (c1 - c0)
        proposed.append(coordinates[grid_index - 1] + fraction * dx)
    proposed.append(hi)

    return {
        "method": "equal_integrated_smoothed_thermodynamic_metric",
        "input_rung_count": len(ladder),
        "target_rung_count": target_count,
        "bandwidth": bandwidth,
        "grid_size": grid_size,
        "acceptance_floor": acceptance_floor,
        "acceptance_ceiling": acceptance_ceiling,
        "input_ladder": list(ladder),
        "input_acceptance": list(acceptance),
        "link_midpoints": midpoints,
        "inferred_metric": raw_metric,
        "proposed_ladder": proposed,
    }


def locally_refine_ladder(
    ladder: Sequence[float], *, window_lo: float, window_hi: float, factor: int
) -> dict[str, object]:
    """Subdivide only links wholly contained in a measured bottleneck window."""
    validate(ladder, [0.5] * (len(ladder) - 1), len(ladder))
    if not math.isfinite(window_lo) or not math.isfinite(window_hi) or window_hi <= window_lo:
        raise ValueError("local refinement window must be finite and increasing")
    if factor < 2:
        raise ValueError("local refinement factor must be at least two")
    proposed = [float(ladder[0])]
    refined_links: list[int] = []
    tolerance = 1.0e-12
    for link, (left, right) in enumerate(zip(ladder, ladder[1:])):
        if left >= window_lo - tolerance and right <= window_hi + tolerance:
            refined_links.append(link)
            gap = (right - left) / factor
            proposed.extend(left + part * gap for part in range(1, factor))
        proposed.append(float(right))
    if not refined_links:
        raise ValueError("local refinement window contains no complete ladder link")
    return {
        "method": "sharp_local_link_subdivision",
        "input_rung_count": len(ladder),
        "target_rung_count": len(proposed),
        "window_lo": window_lo,
        "window_hi": window_hi,
        "refinement_factor": factor,
        "refined_links": refined_links,
        "input_ladder": list(ladder),
        "proposed_ladder": proposed,
    }


def self_test() -> int:
    ladder = [0.0, 0.25, 0.50, 0.75, 1.0]
    acceptance = [0.30, 0.04, 0.30, 0.30]
    result = build_ladder(ladder, acceptance, target_count=9, bandwidth=0.12)
    proposed = result["proposed_ladder"]
    assert isinstance(proposed, list)
    assert len(proposed) == 9
    assert proposed[0] == 0.0 and proposed[-1] == 1.0
    assert all(right > left for left, right in zip(proposed, proposed[1:]))
    central = [right - left for left, right in zip(proposed, proposed[1:]) if 0.25 <= 0.5 * (left + right) <= 0.50]
    outer = [right - left for left, right in zip(proposed, proposed[1:]) if 0.75 <= 0.5 * (left + right)]
    assert central and outer and statistics.mean(central) < statistics.mean(outer)
    local = locally_refine_ladder(
        ladder, window_lo=0.25, window_hi=0.75, factor=3
    )["proposed_ladder"]
    assert isinstance(local, list)
    assert len(local) == 9
    assert local[:2] == [0.0, 0.25]
    assert local[-2:] == [0.75, 1.0]
    print("acceptance_informed_ladder self-test PASS")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ladder", help="comma/space list or file containing measured rungs")
    parser.add_argument("--acceptance", help="comma/space list or file containing measured link acceptance")
    parser.add_argument("--target-count", type=int, help="requested output rung count")
    parser.add_argument("--bandwidth", type=float, help="Gaussian smoothing bandwidth in ladder units")
    parser.add_argument("--grid-size", type=int, default=20001)
    parser.add_argument("--acceptance-floor", type=float, default=1.0e-4)
    parser.add_argument("--acceptance-ceiling", type=float, default=0.999)
    parser.add_argument("--output", type=Path, help="write full JSON result to this path")
    parser.add_argument("--ladder-output", type=Path, help="write proposed rungs, one per line")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument(
        "--local-refinement",
        nargs=3,
        metavar=("WINDOW_LO", "WINDOW_HI", "FACTOR"),
        help="sharply subdivide only complete links within the given window",
    )
    args = parser.parse_args(argv)

    if args.self_test:
        return self_test()
    if args.local_refinement:
        if args.ladder is None:
            parser.error("--ladder is required with --local-refinement")
        window_lo, window_hi = (float(value) for value in args.local_refinement[:2])
        factor = int(args.local_refinement[2])
        result = locally_refine_ladder(
            parse_float_list(args.ladder),
            window_lo=window_lo,
            window_hi=window_hi,
            factor=factor,
        )
    else:
        if args.ladder is None or args.acceptance is None or args.target_count is None:
            parser.error("--ladder, --acceptance, and --target-count are required")
        result = build_ladder(
            parse_float_list(args.ladder),
            parse_float_list(args.acceptance),
            target_count=args.target_count,
            bandwidth=args.bandwidth,
            grid_size=args.grid_size,
            acceptance_floor=args.acceptance_floor,
            acceptance_ceiling=args.acceptance_ceiling,
        )
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    if args.ladder_output:
        proposed = result["proposed_ladder"]
        assert isinstance(proposed, list)
        args.ladder_output.write_text("".join(f"{value:.12f}\n" for value in proposed), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
