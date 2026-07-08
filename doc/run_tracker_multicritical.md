# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-07 (rewrite this card in place; durable results -> plan/report).

## Status
- Phase: pre-M0. Plan + harness written (Fable 5 review session, 2026-07-07);
  no campaign code changes, no campaign runs yet.
- Next action: M0 — checkpoint/restart, then PT, then G2 pilot (L=120 ladder).

## Gate log (append one line per gate decision)
| date | gate | evidence | result | decision |
|---|---|---|---|---|
| 2026-07-07 | pre | code review (chat) + performance_report.md | — | plan approved by user |

## Baseline anchors (do not re-derive)
- L200 b400 Boost production: 80k samples / 11.8 d; tau(percolation_strength)
  ~2200; Neff ~18 (~1.5 Neff/day). t_sample ~ 12.7 s.
- L120 b100 multicritical region: 50k samples / 3.1 d; tau ~9000-10000 near
  mu~2.2-2.25, h~0.08-0.09; Neff 2-3 (~0.8 Neff/day).
- L1000 b400 Boost hour-scale: 2.12e9 attempts / ~1.4 h, MaxRSS 3.2 GB —
  memory is not the blocker.
- G2 GO bar: PT pilot tau_worst drop >= 5x vs L120 baseline.

## Open questions for M1
- Confirm MCP line geometry vs Bonati et al. PRB 105, 165138 (first-order +
  2 Ising lines meeting) in our (h, lambda) diagram before placing FSS points.
- Exact self-dual-line parametrization used by the code (x<->z basis mapping).
