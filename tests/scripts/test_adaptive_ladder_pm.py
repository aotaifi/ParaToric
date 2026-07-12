from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = REPO_ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

import adaptive_ladder_pm as pm  # noqa: E402


class AdaptiveLadderPmTests(unittest.TestCase):
    def test_alpha_blend_damps_large_pm_jump(self) -> None:
        ladder = [0.10, 0.20, 0.30, 0.40, 0.50]
        acceptance = [0.95, 0.01, 0.95, 0.95]
        raw = pm.pm_spacing_update(ladder, acceptance)
        damped = pm.adapt_ladder(
            ladder,
            acceptance,
            alpha=0.2,
            window_radius=0,
            max_iterations=1,
        ).proposed_ladder
        self.assertLess(abs(damped[2] - ladder[2]), abs(raw[2] - ladder[2]))

    def test_min_spacing_prevents_collapse(self) -> None:
        out, changed = pm.enforce_min_spacing([0.0, 0.001, 0.999, 1.0], min_spacing_abs=0.10)
        self.assertTrue(changed)
        self.assertGreaterEqual(min(b - a for a, b in zip(out, out[1:])), 0.10 - 1e-12)

    def test_fixed_iteration_cutoff_stops_noisy_case(self) -> None:
        result = pm.adapt_ladder(
            [0.10, 0.20, 0.30, 0.40, 0.50],
            [0.95, 0.01, 0.95, 0.01],
            alpha=0.5,
            window_radius=0,
            max_iterations=3,
            tolerance=0.0,
        )
        self.assertEqual(result.iterations_run, 3)
        self.assertEqual(result.stopped_by, "max_iterations")

    def test_window_smoothing_reduces_single_link_spike(self) -> None:
        smoothed = pm.smooth_acceptance([0.30, 0.95, 0.30], window_radius=1)
        self.assertLess(smoothed[1], 0.95)
        self.assertAlmostEqual(smoothed[1], (0.30 + 0.95 + 0.30) / 3.0)

    def test_synthetic_uniform_high_acceptance_widens(self) -> None:
        result = pm.synthetic_cases()["uniform_high_acceptance"]
        self.assertGreater(
            result.proposed_ladder[-1] - result.proposed_ladder[0],
            result.previous_ladder[-1] - result.previous_ladder[0],
        )
        self.assertTrue(result.span_adjustment_applied)

    def test_synthetic_bottleneck_is_not_ignored(self) -> None:
        result = pm.synthetic_cases()["single_bad_bottleneck"]
        gaps = [b - a for a, b in zip(result.proposed_ladder, result.proposed_ladder[1:])]
        self.assertEqual(result.raw_bottleneck_link, 2)
        self.assertLess(min(gaps[1:4]), max(gaps))


if __name__ == "__main__":
    unittest.main()
