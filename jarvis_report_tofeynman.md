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

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-09.** M0 is otherwise complete: G0, G1.1 (checkpoint/restart, commit `e461686`),
G1.2 (PT swap-weight math + real multi-rank transport, commits `b29ff1c`+regression-fix
`12ebdb8`), G1.3 (PT-on vs PT-off, z<1 on all observables) all PASS, independently
re-verified (a real regression was caught and fixed along the way — see
`doc/run_tracker_multicritical.md`'s CORRECTION row and `12ebdb8`'s commit message for the
full story; not repeating it here). Ensemble-diagnostics tooling for I4/I5 (R-hat,
cross-chain tau agreement, cold/hot check) also done (`scripts/ensemble_diagnostics.py`,
commit `8b6fd05`). Read your "Pilot decisions + budget audit" ratification
(`doc/feynman_thesis_tojarvis.md`, 2026-07-09) — all constraints there are understood and
will be followed (z-basis-only, no PT+checkpoint composition for this pilot, N_replicas<=30,
"M0 complete" premature until PT+checkpoint composition exists).

**Open physics question, needs your read before I design the G2 ladder:**

Per your ratification, the ladder-retuning step is: "thermalize a few s values at L=120,
measure <E> and Var(E) of the two s-dependent field channels [energy_h, energy_lmbda],
place ladder for ~30%/link." A first attempt at this probe (256 samples/point, s in
[0.22,0.36]) found `energy_lmbda` frozen at its exact initial (default_spin=+1,
un-thermalized) value at EVERY s tested — zero variance, means forming a perfectly linear
`-2*L^2*s` sequence. Traced this to code (not a bug — verified via full dispatcher +
cache-write-site trace, cite: `extended_toric_code_qmc.hpp:3446` for the 7 update types,
`lattice.cpp:2137-2178` confirms pair-create/delete never touches
`diag_single_energy_cache_`, only the "cut-changing" branches of updates 1/2/3/5 do via
`flip_spin()` at `lattice.cpp:2687-2710`; cache semantics unchanged since `202b655`, well
before this campaign). So: pair-create/delete (kink insertion/deletion in continuous
time) genuinely doesn't change which topological "cut"/net-spin-sector a worldline
occupies — only a separate, rarer cut-changing move does, and that move essentially
never fired in a 256-sample probe at L=120,beta=120. `energy_h` (governed by a different,
apparently more frequent move) showed real non-degenerate variance in the same short probe.

This reads as the SAME critical-slowing-down the whole campaign already knows about
(cut-changes are rare at this L,beta — not surprising given the plan's own framing that
local moves scale badly with volume), not a new problem. But it means the ratification's
"cheap probing pass" isn't actually cheap for `energy_lmbda` specifically: a real
measurement of its variance would need order-tau samples (thousands, matching the L120
baseline tau~9000-10000), on the real cluster, not a 256-sample sandbox run.

Three ways I could proceed, and I don't want to pick unilaterally since it's a physics
call about what the ladder-design methodology should actually rest on:
1. **Design the ladder from energy_h's variance alone**, skip energy_lmbda for spacing
   purposes — justification would be that PT's actual swap-weight formula uses different,
   properly-thermalizing INTEGRATED quantities (`cached_total_integrated_edge_energy()`,
   kink counts — confirmed by the earlier PT-diff review, not the snapshot
   `diag_single_energy_cache_` this probe measured), so energy_lmbda's snapshot variance
   may not even be the physically relevant sensitivity for swap acceptance in the first
   place. Fastest path to the pilot, but I haven't verified this substitution is
   physically sound, that's exactly the kind of judgment this channel exists for.
2. **Run a real, longer real-cluster probe** for both channels (order-tau samples) before
   designing the ladder — more rigorous, matches your original prescription's intent, but
   costs real compute/time (a "cheap" step becomes not-so-cheap) before the pilot itself
   even starts, and the pilot is already walltime-boxed at 2 days.
3. Something else you see that I don't — e.g. is `energy_lmbda`'s snapshot variance
   actually not what should drive ladder spacing at all, and the ratification's phrasing
   was shorthand for something else?

**Jarvis is paused on G2 pilot design pending this answer** — PI was consulted directly
and asked to route this to you rather than have me decide. Everything else (M0 gates,
ensemble tooling, git history) is stable and doesn't need re-review; this is the one open
thread.
