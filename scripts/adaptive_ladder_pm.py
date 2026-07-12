#!/usr/bin/env python3
"""PM acceptance-rate ladder builder for ParaToric parallel tempering.

This ports the axis-agnostic spacing idea from the reference project's
``temp_grid::adapt_betas`` to ParaToric's current self-dual ``s = h = lmbda``
ladder.  It only proposes ladder points from per-link acceptance rates; it does
not touch swap payloads or beta/time rescaling.
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence


TARGET_LOW = 0.20
TARGET_HIGH = 0.40


@dataclass(frozen=True)
class AdaptResult:
    previous_ladder: list[float]
    raw_acceptance: list[float]
    smoothed_acceptance: list[float]
    proposed_ladder: list[float]
    iterations_run: int
    stopped_by: str
    min_spacing_applied: bool
    span_adjustment_applied: bool
    raw_bottleneck_link: int | None
    raw_bottleneck_acceptance: float | None
    bottleneck_link: int | None
    bottleneck_acceptance: float | None


def finite_acceptances(values: Iterable[float]) -> list[float]:
    return [float(v) for v in values if math.isfinite(float(v))]


def validate_ladder(ladder: Sequence[float]) -> None:
    if len(ladder) < 2:
        raise ValueError("ladder must contain at least two points")
    for idx, value in enumerate(ladder):
        if not math.isfinite(value):
            raise ValueError(f"ladder[{idx}] is not finite")
    for idx in range(len(ladder) - 1):
        if not ladder[idx + 1] > ladder[idx]:
            raise ValueError(f"ladder must be strictly increasing at link {idx}")


def validate_acceptance(acceptance: Sequence[float], ladder: Sequence[float]) -> None:
    if len(acceptance) != len(ladder) - 1:
        raise ValueError("acceptance length must equal len(ladder)-1")
    for idx, value in enumerate(acceptance):
        if not math.isfinite(value):
            continue
        if value < 0.0 or value > 1.0:
            raise ValueError(f"acceptance[{idx}] must be in [0, 1]")


def smooth_acceptance(acceptance: Sequence[float], window_radius: int) -> list[float]:
    if window_radius < 0:
        raise ValueError("window_radius must be non-negative")
    out: list[float] = []
    n = len(acceptance)
    global_valid = finite_acceptances(acceptance)
    fallback = sum(global_valid) / len(global_valid) if global_valid else 0.0
    for idx in range(n):
        lo = max(0, idx - window_radius)
        hi = min(n, idx + window_radius + 1)
        vals = finite_acceptances(acceptance[lo:hi])
        out.append(sum(vals) / len(vals) if vals else fallback)
    return out


def pm_spacing_update(ladder: Sequence[float], acceptance: Sequence[float]) -> list[float]:
    """Reference PM update: new gap_i is proportional to old_gap_i * p_i/mean(p)."""
    validate_ladder(ladder)
    validate_acceptance(acceptance, ladder)

    valid = finite_acceptances(acceptance)
    mean_p = sum(valid) / len(valid) if valid else 0.0
    if not math.isfinite(mean_p) or mean_p <= 1e-12:
        return list(ladder)

    gaps = [ladder[idx + 1] - ladder[idx] for idx in range(len(ladder) - 1)]
    weighted: list[float] = []
    for idx, gap in enumerate(gaps):
        p = acceptance[idx]
        if not math.isfinite(p) or p <= 1e-12:
            p = mean_p
        weighted.append(max(0.0, gap) * (p / mean_p))

    weighted_sum = sum(weighted)
    span = ladder[-1] - ladder[0]
    if weighted_sum <= 1e-12 or span <= 0.0:
        return list(ladder)

    out = [float(ladder[0])]
    for gap in weighted:
        out.append(out[-1] + span * gap / weighted_sum)
    out[-1] = float(ladder[-1])
    return out


def blend_ladders(previous: Sequence[float], target: Sequence[float], alpha: float) -> list[float]:
    if len(previous) != len(target):
        raise ValueError("ladder lengths differ")
    if not 0.0 <= alpha <= 1.0:
        raise ValueError("alpha must be in [0, 1]")
    return [(1.0 - alpha) * old + alpha * new for old, new in zip(previous, target)]


def enforce_min_spacing(
    ladder: Sequence[float],
    *,
    min_spacing_abs: float = 0.0,
    min_spacing_rel: float = 0.0,
) -> tuple[list[float], bool]:
    validate_ladder(ladder)
    min_spacing_abs = max(0.0, min_spacing_abs)
    min_spacing_rel = max(0.0, min_spacing_rel)
    if min_spacing_abs == 0.0 and min_spacing_rel == 0.0:
        return list(ladder), False

    n_edges = len(ladder) - 1
    span = ladder[-1] - ladder[0]
    raw_gaps = [max(0.0, ladder[idx + 1] - ladder[idx]) for idx in range(n_edges)]
    min_gaps = [
        max(min_spacing_abs, min_spacing_rel * max(abs(ladder[idx]), abs(ladder[idx + 1])))
        for idx in range(n_edges)
    ]
    sum_min = sum(min_gaps)
    if sum_min > span and sum_min > 0.0:
        scale = span / sum_min
        min_gaps = [gap * scale for gap in min_gaps]
        sum_min = span

    extra = max(0.0, span - sum_min)
    weights = [max(0.0, raw - floor) for raw, floor in zip(raw_gaps, min_gaps)]
    weight_sum = sum(weights)
    if weight_sum <= 1e-12:
        weights = [1.0] * n_edges
        weight_sum = float(n_edges)

    out = [float(ladder[0])]
    for floor, weight in zip(min_gaps, weights):
        out.append(out[-1] + floor + extra * weight / weight_sum)
    out[-1] = float(ladder[-1])
    changed = any(abs(a - b) > 1e-14 for a, b in zip(out, ladder))
    return out, changed


def adapt_span_to_target(
    ladder: Sequence[float],
    smoothed_acceptance: Sequence[float],
    *,
    target_low: float,
    target_high: float,
    max_span_step: float,
    lower_bound: float | None,
    upper_bound: float | None,
) -> tuple[list[float], bool]:
    valid = finite_acceptances(smoothed_acceptance)
    if not valid:
        return list(ladder), False
    mean_p = sum(valid) / len(valid)
    target_mid = 0.5 * (target_low + target_high)
    if target_mid <= 0.0:
        return list(ladder), False
    if target_low <= mean_p <= target_high:
        return list(ladder), False

    factor = math.sqrt(mean_p / target_mid)
    if mean_p < target_low:
        factor = math.sqrt(max(mean_p, 1e-12) / target_mid)
    factor = min(max(factor, 1.0 - max_span_step), 1.0 + max_span_step)

    center = 0.5 * (ladder[0] + ladder[-1])
    half_span = 0.5 * (ladder[-1] - ladder[0]) * factor
    lo = center - half_span
    hi = center + half_span
    if lower_bound is not None and lo < lower_bound:
        hi += lower_bound - lo
        lo = lower_bound
    if upper_bound is not None and hi > upper_bound:
        lo -= hi - upper_bound
        hi = upper_bound
    if lower_bound is not None:
        lo = max(lo, lower_bound)
    if upper_bound is not None:
        hi = min(hi, upper_bound)
    if not hi > lo:
        return list(ladder), False

    old_span = ladder[-1] - ladder[0]
    new_span = hi - lo
    out = [lo + (value - ladder[0]) * new_span / old_span for value in ladder]
    out[0] = lo
    out[-1] = hi
    changed = any(abs(a - b) > 1e-14 for a, b in zip(out, ladder))
    return out, changed


def adapt_ladder(
    ladder: Sequence[float],
    acceptance: Sequence[float],
    *,
    alpha: float = 0.35,
    window_radius: int = 1,
    min_spacing_abs: float = 0.0,
    min_spacing_rel: float = 0.0,
    max_iterations: int = 4,
    tolerance: float = 0.0,
    target_low: float = TARGET_LOW,
    target_high: float = TARGET_HIGH,
    max_span_step: float = 0.35,
    lower_bound: float | None = None,
    upper_bound: float | None = None,
) -> AdaptResult:
    validate_ladder(ladder)
    validate_acceptance(acceptance, ladder)
    if max_iterations < 1:
        raise ValueError("max_iterations must be >= 1")
    if tolerance < 0.0:
        raise ValueError("tolerance must be non-negative")
    if not 0.0 <= max_span_step <= 1.0:
        raise ValueError("max_span_step must be in [0, 1]")

    smoothed = smooth_acceptance(acceptance, window_radius)
    current = list(ladder)
    any_min_spacing = False
    any_span_adjustment = False
    stopped_by = "max_iterations"
    iterations_run = 0

    for iteration in range(1, max_iterations + 1):
        raw = pm_spacing_update(current, smoothed)
        raw, span_changed = adapt_span_to_target(
            raw,
            smoothed,
            target_low=target_low,
            target_high=target_high,
            max_span_step=max_span_step,
            lower_bound=lower_bound,
            upper_bound=upper_bound,
        )
        candidate = blend_ladders(current, raw, alpha)
        candidate, min_changed = enforce_min_spacing(
            candidate,
            min_spacing_abs=min_spacing_abs,
            min_spacing_rel=min_spacing_rel,
        )
        delta = max(abs(a - b) for a, b in zip(candidate, current))
        current = candidate
        any_span_adjustment = any_span_adjustment or span_changed
        any_min_spacing = any_min_spacing or min_changed
        iterations_run = iteration
        if tolerance > 0.0 and delta <= tolerance:
            stopped_by = "tolerance"
            break

    raw_pairs = [(idx, val) for idx, val in enumerate(acceptance) if math.isfinite(val)]
    if raw_pairs:
        raw_bottleneck_link, raw_bottleneck_acceptance = min(raw_pairs, key=lambda item: item[1])
    else:
        raw_bottleneck_link, raw_bottleneck_acceptance = None, None

    smoothed_pairs = [(idx, val) for idx, val in enumerate(smoothed) if math.isfinite(val)]
    if smoothed_pairs:
        bottleneck_link, bottleneck_acceptance = min(smoothed_pairs, key=lambda item: item[1])
    else:
        bottleneck_link, bottleneck_acceptance = None, None

    return AdaptResult(
        previous_ladder=list(ladder),
        raw_acceptance=list(acceptance),
        smoothed_acceptance=smoothed,
        proposed_ladder=current,
        iterations_run=iterations_run,
        stopped_by=stopped_by,
        min_spacing_applied=any_min_spacing,
        span_adjustment_applied=any_span_adjustment,
        raw_bottleneck_link=raw_bottleneck_link,
        raw_bottleneck_acceptance=raw_bottleneck_acceptance,
        bottleneck_link=bottleneck_link,
        bottleneck_acceptance=bottleneck_acceptance,
    )


def parse_float_list(text: str) -> list[float]:
    path = Path(text)
    if path.exists():
        raw = path.read_text(encoding="utf-8")
    else:
        raw = text
    parts = raw.replace(",", " ").split()
    if not parts:
        raise ValueError("empty numeric list")
    return [float(part) for part in parts]


def synthetic_cases() -> dict[str, AdaptResult]:
    base = [0.10, 0.18, 0.26, 0.34, 0.42, 0.50]
    common = dict(alpha=0.35, window_radius=1, min_spacing_abs=0.01, max_iterations=4, lower_bound=0.0)
    return {
        "uniform_high_acceptance": adapt_ladder(base, [0.60, 0.60, 0.60, 0.60, 0.60], **common),
        "single_bad_bottleneck": adapt_ladder(base, [0.30, 0.32, 0.005, 0.31, 0.29], **common),
    }


def run_from_acceptance(args: argparse.Namespace) -> int:
    result = adapt_ladder(
        parse_float_list(args.ladder),
        parse_float_list(args.acceptance),
        alpha=args.alpha,
        window_radius=args.window_radius,
        min_spacing_abs=args.min_spacing_abs,
        min_spacing_rel=args.min_spacing_rel,
        max_iterations=args.max_iterations,
        tolerance=args.tolerance,
        target_low=args.target_low,
        target_high=args.target_high,
        max_span_step=args.max_span_step,
        lower_bound=args.lower_bound,
        upper_bound=args.upper_bound,
    )
    print(json.dumps(asdict(result), indent=2, sort_keys=True))
    return 0


def run_validate_synthetic(_args: argparse.Namespace) -> int:
    for name, result in synthetic_cases().items():
        old_gaps = [b - a for a, b in zip(result.previous_ladder, result.previous_ladder[1:])]
        new_gaps = [b - a for a, b in zip(result.proposed_ladder, result.proposed_ladder[1:])]
        print(f"{name}:")
        print(f"  previous={result.previous_ladder}")
        print(f"  smoothed_acceptance={result.smoothed_acceptance}")
        print(f"  proposed={result.proposed_ladder}")
        print(f"  old_gaps={old_gaps}")
        print(f"  new_gaps={new_gaps}")
        print(
            "  "
            f"raw_bottleneck_link={result.raw_bottleneck_link} "
            f"raw_bottleneck_acceptance={result.raw_bottleneck_acceptance} "
            f"bottleneck_link={result.bottleneck_link} "
            f"bottleneck_acceptance={result.bottleneck_acceptance} "
            f"iterations={result.iterations_run} stopped_by={result.stopped_by} "
            f"min_spacing={int(result.min_spacing_applied)} "
            f"span_adjustment={int(result.span_adjustment_applied)}"
        )
    return 0


def self_test() -> int:
    base = [0.10, 0.20, 0.30, 0.40, 0.50]
    raw = pm_spacing_update(base, [0.95, 0.01, 0.95, 0.95])
    damped = adapt_ladder(
        base,
        [0.95, 0.01, 0.95, 0.95],
        alpha=0.2,
        window_radius=0,
        max_iterations=1,
    ).proposed_ladder
    raw_jump = abs(raw[2] - base[2])
    damped_jump = abs(damped[2] - base[2])
    assert damped_jump < raw_jump

    collapsed, changed = enforce_min_spacing([0.0, 0.001, 0.999, 1.0], min_spacing_abs=0.10)
    assert changed
    assert min(b - a for a, b in zip(collapsed, collapsed[1:])) >= 0.10 - 1e-12

    capped = adapt_ladder(
        base,
        [0.95, 0.01, 0.95, 0.01],
        alpha=0.5,
        window_radius=0,
        max_iterations=3,
        tolerance=0.0,
    )
    assert capped.iterations_run == 3
    assert capped.stopped_by == "max_iterations"

    smoothed = smooth_acceptance([0.30, 0.95, 0.30], window_radius=1)
    assert smoothed[1] < 0.95

    cases = synthetic_cases()
    uniform = cases["uniform_high_acceptance"]
    assert uniform.proposed_ladder[-1] - uniform.proposed_ladder[0] > 0.50 - 0.10
    bottleneck = cases["single_bad_bottleneck"]
    gaps = [b - a for a, b in zip(bottleneck.proposed_ladder, bottleneck.proposed_ladder[1:])]
    assert bottleneck.raw_bottleneck_link == 2
    assert min(gaps[1:4]) < max(gaps)
    print("adaptive_ladder_pm self-test PASS")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="PM adaptive s-ladder builder for ParaToric PT.")
    sub = parser.add_subparsers(dest="command")

    propose = sub.add_parser("from-acceptance", help="propose a new ladder from per-link acceptance rates")
    propose.add_argument("--ladder", required=True, help="comma/space list or path containing current s ladder")
    propose.add_argument("--acceptance", required=True, help="comma/space list or path containing per-link acceptance")
    propose.add_argument("--alpha", type=float, default=0.35, help="blend weight for the PM proposal")
    propose.add_argument("--window-radius", type=int, default=1, help="moving-average radius over links")
    propose.add_argument("--min-spacing-abs", type=float, default=0.0)
    propose.add_argument("--min-spacing-rel", type=float, default=0.0)
    propose.add_argument("--max-iterations", type=int, default=4)
    propose.add_argument("--tolerance", type=float, default=0.0)
    propose.add_argument("--target-low", type=float, default=TARGET_LOW)
    propose.add_argument("--target-high", type=float, default=TARGET_HIGH)
    propose.add_argument("--max-span-step", type=float, default=0.35)
    propose.add_argument("--lower-bound", type=float)
    propose.add_argument("--upper-bound", type=float)
    propose.set_defaults(func=run_from_acceptance)

    validate = sub.add_parser("validate-synthetic", help="run the required synthetic ladder scenarios")
    validate.set_defaults(func=run_validate_synthetic)

    check = sub.add_parser("self-test", help="run built-in synthetic assertions")
    check.set_defaults(func=lambda _args: self_test())
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
