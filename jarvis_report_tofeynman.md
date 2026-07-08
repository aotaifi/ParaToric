# Jarvis → Feynman report channel (ParaToric)

Jarvis appends here; Feynman reads, never writes. Entries tagged
`thesis:<id> | plan:<plan_id> | type:PROPOSED PLAN | RESULT | REVISION-RESPONSE`.

---

## thesis:T1 | plan:T1-P1 | type:PROPOSED PLAN

**2026-07-08.** No `feynman_thesis_tojarvis.md` exists yet for this project — this is a
cold bootstrap of the Feynman/Jarvis channel here. Proposing thesis T1 as the framing for
the M0 infrastructure milestone; Feynman should ratify/reword the thesis itself (this entry
only proposes the milestone breakdown under it).

**Proposed thesis T1:** The universality class of the toric-code multicritical point
(self-dual (2+1)D Z2 gauge-Higgs; competing scenarios: first-order, multicritical XY*
(Bonati-Pelissetto-Vicari PRB 105,165138), new self-dual class (Somoza-Serna-Nahum PRX
11,041008)) can be decided by large-L FSS with beta=L, IF Markov-chain sampling efficiency
(currently autocorrelation-bound, not CPU-bound) can be raised enough to make the ladder
tractable. M0 tests that IF.

**Basis (already written, this session inherits it, does not re-derive it):**
- `doc/multicritical_universality_plan.md` — full milestone plan M0–M4, literature, budget.
- `doc/multicritical_execution_harness.md` — hard gates/invariants (I1–I6, G0–G4).
- `doc/run_tracker_multicritical.md` — current-state card; carries a 2026-07-08 physics
  pass (self-dual line resolved, BPV/SSN geometry match confirmed, and risk flags R1–R5,
  notably **R1: XY vs new-class is likely underpowered at any reachable L** — the decidable
  claim is continuous-vs-first-order, not exponent identity). Feynman should weigh R1 when
  ratifying: M0 is infrastructure that is worth building regardless of R1 (checkpoint/PT are
  needed for any FSS at this size), but the M3/M4 claim scope should already be bounded to
  "continuous vs first-order (+ exponents consistent with XY/SSN, not distinguishing them)"
  per that risk note.

**Milestone breakdown proposed for plan T1-P1 (= plan's M0, gate order from the harness):**
1. G0.1 baseline reproduction (rebuild `build-current-clang-portable`, rerun the L=40
   beta=80 smoke from `performance_report.md`, match acceptance fraction ~1% and energy
   within errors) + G0.3 existing test suite green. Log in tracker.
2. G0.2 fast-math A/B: one L=40 pair, `PARATORIC_ENABLE_FAST_MATH` ON vs OFF, 4 seeds each,
   all observable means z<3. Log in tracker; FAIL => production uses OFF.
3. Checkpoint/restart for the Boost `Lattice` backend (event vectors, spins, RNG state,
   cached energies/counters). Validate per G1.1 (bit-faithful or z<3 statistically
   identical; event-vector sort invariants after load).
4. Parallel tempering (MPI, `PARATORIC_LINK_MPI` hook already in CMake; O(1) swap weights
   from cached kink counts + integrated potentials). Validate per G1.2 (swap weight vs
   brute force at L=8, <1e-8) and G1.3 (PT-on vs PT-off physics agreement, z<3) before any
   large run.
5. G2 pilot: PT ladder through the multicritical region, L=120 beta=120, <=2 days walltime,
   >=4 chains, 1 node, explicit memory request, concise-alert wrapper. GO if tau_worst drops
   >=5x vs the L120 baseline (tau~9000-10000); record decision + projected M2 budget in the
   tracker. **STOP for PI review before any FSS production regardless of GO/NO-GO** — this
   is already a hard stop in the harness (G2), independent of the Feynman gate.

**Execution mode once gated:** dispatched to Codex workers (Clark for the checkpoint/PT
implementation, Martina review, James/Jones for the gate-validation runs and pilot staging)
per JARVIS.md — not executed by Jarvis directly. No workers dispatched yet; awaiting this
plan's `status: GO` in `feynman_thesis_tojarvis.md` plus the PI's explicit "go" in Jarvis's
own chat (both required, per the two-go gate).

**Status: awaiting thesis ratification + plan verdict. Jarvis is idle on this project until
both gate conditions are met.**
