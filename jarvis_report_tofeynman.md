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

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-12.** Following your Issue-1/Issue-2 answer (2026-07-10): ran the wide-s
sector-rate scan you specified (step 1 of your finalized ordered plan), redesigned once
(v1 structurally couldn't measure sector changes at all — used `--full_time_series 0` by
mistake; v2 fixed that and used parallel independent chains instead of one long serial
chain per s-value, for wall-clock efficiency, PI-approved). v2 (Slurm array `15034719`)
ran cleanly: 8 s-values x 8 chains, 1500 samples/chain, real per-sample
`percolation_probability`/`percolation_strength`/`anyon_count` series recorded this time.
57/64 tasks done as of this report (7/8 s-values with full 8-chain coverage: s=0.10
through 0.46; s=0.50 partial).

**Result: null across the entire tested range.** `percolation_probability` and
`percolation_strength` are EXACTLY 0.0 for every completed chain at every s-value tested
(0.10 through 0.46, plus 2 partial chains at 0.50) — zero percolation events in 84000+
samples analyzed. `anyon_count`'s per-sample series is not plateau-like (91-97% of
adjacent samples differ, longest constant run 3-4 samples) — it's tracking ordinary local
anyon-number noise, not discrete winding/topological-sector jumps, so it can't serve as a
sector-change proxy either, confirming your Issue-1 read that this observable was never
the right tool for this question.

**The open question is genuinely physics, not engineering**: is [0.10, 0.50] on the
self-dual line (mu=J=1, h=lmbda=s) simply outside the multicritical/deconfinement region
at L=120 (i.e. the whole tested range sits in the confined phase, and the actual
transition is at higher s, lower s, or requires a different centering entirely) — or does
`percolation_probability`/`percolation_strength` genuinely need many more than 1500
samples/chain to register even one percolating cluster at this L (a sample-size issue, not
a wrong-region issue)? I don't have the physics grounding to distinguish these from the
data alone. The old (pre-self-dual-line) baseline anchor in the tracker references
mu~2.2-2.25, h~0.08-0.09 as the "multicritical region," but that's a different
parametrization (mu fixed at 1 now) and I don't think it's directly usable as a prior for
where s* sits on the self-dual line — flagging that mismatch too in case it's relevant to
locating the right range.

**Jarvis is paused here, not proceeding to step 2 (ladder tuning) or resubmitting anything
further, pending your read.** Full data at
`/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711/` if useful; tracker row
dated 2026-07-12 has the complete numeric breakdown.

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-12.** Executed your MASTER PRE-PILOT SEQUENCE items 0 (D1) and 1 (observable
validation). D1 is done and independently verified — see below for a process note. Step 1
(the Linsel POP benchmark) ran but came back INCONCLUSIVE, with a specific new physics
puzzle that needs your read before I touch anything else in the sequence.

### D1 (swap_period in sweeps) — DONE, commit `8e54db2`, both a code fix and a real
process lesson
Implemented exactly as specified in `doc/pt_architecture.md` Sec 3/6: new primary CLI
`--pt_swap_period_sweeps` (default 2.0), `swap_period_moves = round(sweeps * 2*L^2)`,
legacy `--pt_swap_period` kept as an explicit override. Independently verified twice
(clean rebuilds by both the implementer and an independent reviewer) — confirmed used at
both thermalization and production swap-trigger gates, formula and CLI validation both
correct. G1.2/G1.3's L=8 "6/4 round-trips" result stands as-is; it simply never
transferred to L=120 as previously assumed (0.035 sweeps/swap there vs 7.8 at L=8).

Process note, since it revises something I'd have otherwise reported as settled: the
implementation also needed a Boost.Log/clang++-20 compatibility shim
(`src/logging/trivial_log.hpp`). An EARLIER session had introduced then reverted an almost
identical shim, on the verdict "not needed for the default clang build." That verdict
turned out to be a stale-incremental-build false negative — confirmed this time by
`git archive`-extracting the pre-D1 commit into a bare directory and rebuilding fresh:
`boost/log` genuinely triggers a reproducible clang20+Boost-1.75-MPL incompatibility
(`lock_access_mode::unlocked_access` outside its `mpl::integral_c` valid range) with
nothing to do with today's changes. Logged in `ai-workflow/jarvis_selfreview.md`.

### Step 1 (Linsel POP benchmark) — INCONCLUSIVE, new finding needs your read
Setup exactly per your spec: lmbda=0.3 fixed (off self-dual line), mu=J=1, z-basis, no PT,
L={16,24,32}, beta=L, h scanned across h_c=0.335(2) at {0.15,0.25,0.30,0.335,0.37,0.42,
0.50}. Both `percolation_probability`/`percolation_strength` (the "electric" channel) and
`plaquette_percolation_probability`/`plaquette_percolation_strength` (the "plaquette"
channel) measured. Full table: `/scratch/a/A.Otaifi/paratoric_linsel_percolation_
benchmark_20260712/linsel_pop_long_summary.tsv`.

**Finding 1**: the "electric" `percolation_*` channel is essentially zero across the
ENTIRE tested range, including h=0.15 (deep into what should be the deconfined phase per
Linsel). It never shows the expected large-POP-at-small-h behavior at all. This channel
does not look like Linsel's POP in this setup.

**Finding 2 (the one I actually need your read on)**: the `plaquette_percolation_*`
channel IS large at small h (0.970 at L=32, h=0.15, one run) — the right qualitative shape
— but shows SEVERE initial-spin-condition sensitivity at the exact same physical point:
0.970 (`default_spin=-1` start) vs 0.132 (`default_spin=+1` start) at L=32, h=0.15, same
everything else. That's not noise-level disagreement; it looks like either (a) genuine
non-equilibration even at these small sizes (which would be surprising for ordinary
short-range criticality with local updates, but not impossible if something about this
observable's approach to equilibrium is unusually slow), (b) a bug in how the plaquette
percolation observable is computed or how its state resets/carries over depending on the
initial condition, or (c) a parameter/basis convention mismatch putting this point in a
different physical regime than intended (e.g. is `default_spin` supposed to be physically
irrelevant here the way it would be for a symmetric Hamiltonian, or does our convention
actually break that symmetry at finite h/lmbda in a way I'm not accounting for?). I don't
have the grounding to tell these apart.

James (the dispatched worker) correctly stopped here rather than forcing a channel
identification or a "confirmed correct" conclusion — did not proceed to relocating s* on
the self-dual line.

**Jarvis is paused on the whole master sequence pending your read of Finding 2** — this
gates whether Step 1 can even be resolved, and everything downstream (locating s*, D2/D4/D3,
the pilot) depends on trusting a percolation observable. PI was consulted and asked me to
route this to you rather than dispatch further investigation myself.

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-12.** PI relayed your "STEP-1 BLOCKER RESOLVED: percolation reads the frozen
tau=0 base" diagnosis and said proceed. Ran your prescribed confirmatory test (step 1 of
the resolution steps, no code change) before touching anything: rerun ELECTRIC
`percolation_probability`/`percolation_strength` at L=32, h=0.15, lmbda=0.3, mu=J=1,
`default_spin=-1` (the earlier benchmark used +1, giving ~0). Your prediction: frozen
all-down base should give POP~1, mirroring how the all-up base gave ~0.

**Result: REFUTED as stated.** 3 seeds, all exactly `percolation_probability=0,
percolation_strength=0` — same as `default_spin=+1`. Job `15037168_[0-2]` (3 seeds),
notifier `15037169`, evidence root `/scratch/a/A.Otaifi/paratoric_electric_defaultspin_
minus_confirm_20260712/`. Sizing looks adequate for a fair test (N_thermalization=1e6
attempts ~488 sweeps at L=32; N_between_samples=2000 ~1 sweep — not obviously
under-thermalized), so I don't think this is a trivial "ran it too short" confound, but I
haven't independently verified that assumption.

**I have a candidate explanation but didn't want to act on it without your read**: the
ELECTRIC channel's percolating objects are presumably domain-wall/string-like structures
relative to some reference; a perfectly UNIFORM base configuration (all +1 OR all -1)
might be trivially string-free either way — i.e. both signs giving exactly 0 could be
physically unsurprising for the electric representation specifically, for a reason
unrelated to your rotate_imag_time diagnosis, while the PLAQUETTE channel's asymmetry
(0.970 vs 0.132 — the actual Finding 2 that started this) remains the real evidence for
the frozen-base mechanism. If that's right, my test simply picked a channel where the
frozen-base prediction is trivially null both ways, not a real disconfirmation of your
diagnosis. But I'm not confident in this reasoning and don't want to proceed to the
rotate_imag_time code fix on a diagnosis whose confirmatory test just came back
unexpected, without your read on whether this refutes, complicates, or is simply
orthogonal to the root-cause finding.

**Paused again, this specific sub-question only** — not re-litigating anything else in
the master sequence, just this confirmatory-test result before Clark starts on the fix.

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-12.** Read your retraction ("STEP-1 BLOCKER — diagnosis REFUTED, CALIBRATE THE
OBSERVABLE FIRST") — good call, agreed the rotate_imag_time story wasn't earned yet. PI
said proceed. Ran your item 1 (calibration unit test, pure code, no QMC/thermalization/
beta) before anything else: `tests/lattice/test_lattice.cpp::percolation_observable_
calibration_known_static_configs`, commit `52de6a5`. Constructed 4 hand-crafted L=6
configs and measured electric (`percolation_probability`/`percolation_strength`) and
plaquette-dual channels directly:

| config | electric POP/POS | plaquette POP/POS |
|---|---|---|
| all +1 | 0 / 0 | 0 / 0 |
| all -1 | 1 / 1 | 1 / 1 |
| single non-contractible winding string of -1 edges | 1 / 0.0833 | 0 / 0 |
| dense random (seed 20260712) | 1 / 0.5278 | 1 / 0.9722 |

The winding-string case has a logically-forced answer (a non-contractible string
topologically MUST percolate) — hard-asserted `electric_probability==1`, passed. That's
the one thing in this table I'm confident is a correct, meaningful check, not just a
reported number.

**The finding that needs your read**: on the STATIC hand-built all-(-1) config, the
electric channel DOES give POP=1 — confirming your original numeric prediction was
directionally right for a genuinely uniform all-(-1) state. But the earlier DYNAMIC QMC
confirmatory test (real simulation, `default_spin=-1`, L=32, h=0.15, 3 seeds) gave
EXACTLY 0, not 1. So: the static calibration says "all-(-1) should percolate", yet the
actual running simulation with that same nominal initial condition measures 0. That
gap — between what a hand-built static state gives and what the dynamical run with the
matching starting condition gives — points at the MEASUREMENT WIRING inside the
simulation loop (timing of when/how percolation gets measured relative to thermalization
and any state rotation), not at the raw cluster-counting math itself (which the
winding-string assertion shows is at least behaving sensibly on obvious cases). This
reads to me like it puts the rotate_imag_time/measurement-timing hypothesis back on
firmer ground than the refuted specific prediction did alone — but I'm flagging it for
your read rather than concluding that myself, since "what the simulation loop actually
measures at snapshot time" is exactly the mechanism your original diagnosis was about.

**Also**: the winding string percolates in the electric channel but NOT in the plaquette
channel (0 vs 1) — confirms the two channels measure genuinely different topological
content, consistent with electric/plaquette being dual to each other as expected, not a
bug in either.

**One caveat I want on record, not resolved myself**: this test (and the existing
pre-2026-07-08 tests it's modeled on, `percolation_strength_test_1`/`plaquette_
percolation_strength_test_1`) all use `LatSpec.basis='x'`, while the entire PT/QMC
campaign runs in `basis='z'` (PT is hardwired to z-basis only per pt_architecture.md).
I believe the percolation/cluster-counting computation itself is basis-structure-agnostic
(pure edge-cluster counting, no branch on the basis field that I can see), so this
shouldn't invalidate the calibration — but I haven't verified that by reading the actual
implementation, only inferred it from the test's simple counting-formula style. Flagging
rather than asserting.

Have NOT proceeded to items 2-4 of your ordered plan (verify beta=L in scripts; time-series
+ long thermalization cold/hot; rotate_imag_time test) — want your read on the
static-vs-dynamic gap first, since it may reprioritize what's actually worth running next.
Full test diff/commit `52de6a5` if useful. Separately (unrelated to this physics thread):
PI authorized parallelizing D2 (offline PT diagnostics analyzer) and D3 (PM adaptive
ladder port) as concurrent Codex dispatches in isolated git worktrees — proceeding with
that now since those are independent software components that don't need s* located to
build/unit-test, will report back once both land.

---

## thesis:T1 | plan:T1-P1 | type:RESULT

**2026-07-12.** D2/D3 both landed (merged, commits `6aeee94`/`d158822`) — D2's strict
round-trip count on real job 14982619 data (6) is genuinely lower than the naive
endpoint-touch count (10), confirming your H2 concern with real evidence; D3 honestly
reports PM alone does not resolve a synthetic bottleneck case, matching H5.

Ran your finalized decisive experiment (`tests/debug/percolation_decisive_experiment.cpp`,
commit `579061b`): L=32, h=0.15, lmbda=0.3, beta=32, both default_spin, thermalization
checkpoints at 100/500/2000/10000 sweeps, base-vs-rotated-slice on the SAME config.

**Density/relaxation question: RESOLVED, supports (a).** Base -1-edge count relaxes for
both starts (+1-start 38->752 of 2048; -1-start 406->825) and is converging toward each
other by 10000 sweeps — this is genuine under-thermalization, not a frozen/wrong-slice
artifact. Plaquette channel tracks this physically (POP turns on as density rises,
e.g. +1-start 10000 sweeps: base 0,0 -> rotated 1,0.638) and rotation clearly matters
there.

**But a sharper, unresolved puzzle emerged**: the ELECTRIC channel stays EXACTLY 0/0 in
ALL 8 points — both starts, every thermalization depth, base AND rotated slice — even at
10000 sweeps with ~37-40% base density where plaquette clearly percolates. Whatever
explains plaquette's behavior does not explain electric's. This looks like electric has
its own, separate problem beyond thermalization — I don't have a hypothesis for it and
am not proceeding to guess one.

Also (cheap items from your list): beta=L confirmed enforced in the existing job
scripts (checked the Linsel benchmark and G2 scan scripts). Calibration re-confirmed
basis-independent (z matches x for all 4 known configs) — the earlier caveat is closed.

Full data: `/scratch/a/A.Otaifi/paratoric_percolation_decisive_20260712/
percolation_decisive_readout.csv`. Paused on the electric-channel puzzle specifically —
everything else in this experiment reads as resolved.

## RESULT — D4 done + rotate_imag_time bug found and fixed (2026-07-12)

Acting on your Step-1 resolution (real physics, not a bug) and the PI's "proceed":

1. D4 (independent h/lmbda PT ladders) implemented, commit `e42914d`. Swap
   log-weight decoupled exactly as your architecture doc specified: kink-
   fugacity term now uses log(partner_h/local_h), integrated-edge term uses
   (partner_lmbda - local_lmbda). Self-dual case (empty ladder_lmbda) verified
   bit-for-bit identical to the old formula. Independent h!=lmbda case
   verified internally consistent (cached-vs-bruteforce, max dev 0). Clean
   rebuild, 3/3 ctest. Independent review (Martina) dispatched but blocked on
   Codex's own provider-side usage limit (unrelated to the project) — will
   retry and report separately.

2. Implementing your item 2 ("rotate once per snapshot, all equal-time obs on
   the same slice") surfaced a REAL bug, now fixed, commit `f08fd9d`:
   get_sample dispatched the observable table without one shared rotation;
   several individual observable lambdas rotated the lattice independently.
   That means observable evaluation ORDER determined which imaginary-time
   slice each observable actually read — percolation could see a different
   slice than energy/susceptibility observables within the SAME recorded
   sample. This was silent (no test caught it before now) since nothing
   previously asserted same-slice-ness across observables directly. Fixed to
   exactly one rotate_imag_time() per sample before any observable dispatch;
   new regression test confirms identical rotation index across all 4
   percolation observables over 3 samples. Clean rebuild, 3/3 ctest.

Given this was a genuine cross-observable consistency bug (not limited to
percolation), it's worth flagging: any PRE-fix run's cross-observable
correlations (e.g. energy vs percolation at the same recorded sample) may be
suspect if it relied on same-slice alignment. I don't believe any GATED
result so far depended on that alignment (the decisive experiment logged
percolation alone; G1.2/G1.3 didn't cross-correlate observables), but flagging
per the standing "report everything, let you judge" discipline.

Not yet started (queued behind this): plaquette-as-POP documentation,
>50%-density confirmatory check, longer-thermalization L=16-32 scan, Linsel
re-run. Jarvis is at critical usage this session (PI-acknowledged, ~4% at
last check) — if this session is cut, a fresh session resumes directly from
status.md / this file, per standing practice.

## RESULT — x-basis "verify before locking" check FAILS, not confirms (2026-07-12)

Ran your prescribed check (UPDATE 2026-07-12d): same self-dual point(s) in
both bases, no PT, L=8, beta=8, s={0.15,0.22,0.30,0.335,0.50}, matched seeds.
Parameter mapping used: identical tuple (h=lmbda=s, mu=J=1) in both bases —
justified since self-duality exchanges (h,mu)<->(lmbda,J), which this tuple
is invariant under.

Result does NOT confirm the prediction:

| s    | z-e POP/POS        | x-m POP/POS        |
|------|---------------------|---------------------|
| .15  | .395(14)/.109(4)   | .787(12)/.344(6)   |
| .22  | .311(15)/.086(4)   | .744(14)/.305(6)   |
| .30  | .189(12)/.050(3)   | .581(16)/.214(6)   |
| .335 | .125(11)/.032(3)   | .367(19)/.127(7)   |
| .50  | .009(3)/.0016(6)   | .004(2)/.0008(4)   |

Both z-e and x-m decrease monotonically with s — not the complementary/
mirror-image behavior your prediction implies. Disagreement between z-e and
x-m is 11-32sigma through s=0.335 (they only both approach zero together at
s=0.50). A 50k-sweep cold/hot repeat at s=0.50 (5x the sweeps of the L=8, s=0.15
run) still shows hot/cold initialization-dependence — i.e. NOT converged,
same signature as the earlier under-thermalization finding, but surviving a
sweep count that resolved that one before at this small L. This smells like
either an x-basis update/observable-wiring issue or a genuinely wrong mapping
assumption on my part (or Jones's), not confirmed new physics either way.

I have NOT attempted a code fix or re-derived the mapping myself — this is
exactly the kind of physics-interpretation-vs-bug fork I should route to you
rather than guess at, per this session's standing practice. Full data:
/scratch/a/A.Otaifi/paratoric_em_selfduality_20260712 (production CLI run,
shared hdf5_readers.py, no code edits made).

Not blocking: D4, the rotate_imag_time fix, and the single-z-sector Linsel
benchmark plan (per your note that Linsel is unchanged) all proceed
independently of this. Only the topological/MCP e-vs-m characterization
sub-step is paused pending your read.

## RESULT — decisive convergence study: rules out under-thermalization AND sector-trapping, points to a cross-basis wiring issue (2026-07-12)

Ran your prescribed decisive check: cold/hot (default_spin +1/-1) vs sweeps,
BOTH bases, fixed self-dual point s=0.22 (h=lmbda=0.22, mu=J=1), L=8, beta=8,
checkpoints at 10k/50k/100k/500k/1M sweeps.

| Sweeps | z cold      | z hot       | x cold      | x hot       |
|-------:|-------------|-------------|-------------|-------------|
| 10k    | .370/.082   | .340/.075   | .710/.352   | .820/.319   |
| 50k    | .294/.068   | .286/.075   | .786/.299   | .746/.301   |
| 100k   | .273/.083   | .285/.079   | .736/.306   | .750/.304   |
| 500k   | .283/.083   | .295/.081   | .749/.305   | .733/.301   |
| 1M     | .286/.077   | .296/.083   | .746/.306   | .742/.304   |

(prob/strength, cumulative means; error bars unavailable — autocorrelation too
strong for the built-in estimator at this chain length, noted as a limitation.)

Reading against your decision rule:
- WITHIN each basis, cold/hot converge cleanly (z gap 0.010, x gap 0.004 by
  1M sweeps, monotonically closing from 10k) — this rules out BOTH plain
  under-thermalization at s=0.22 AND sector-trapping (if sector-trapping were
  real, cold/hot would stay apart WITHIN a basis; they don't).
- ACROSS bases, z-e (~0.29) and x-m (~0.75) are FLAT and ~0.45 apart in
  probability (~0.22 in strength) from 50k sweeps onward — no closing trend
  at all through 1M sweeps. Basis-internal convergence being clean while the
  cross-basis gap stays flat means this isn't a "needs more sweeps" problem
  for either basis alone — it looks like a basis mapping/parameter-convention
  issue (is h=lmbda=s really the same physical self-dual point in both
  bases?) or an observable-wiring issue (is percolation_probability/strength
  computing the same physical quantity, correctly transformed, in both
  bases?), not physics and not sector-trapping.

I have not attempted to audit the x<->z parameter/observable mapping myself
— flagging per this session's practice of not guessing at physics-vs-bug
forks. Full data: /scratch/a/A.Otaifi/paratoric_angle_b_convergence_s022_20260712/
(checkpoint_means.csv + per-chain readout_exact.csv + HDF5). Cost was trivial
(~32 CPU-s/chain, no Slurm needed).

PAUSED again on this specific sub-step pending your read. Independently: D4,
the rotate_imag_time fix, and the single-z-sector Linsel benchmark plan all
proceed unaffected; the L={16,24,32} thermalization-convergence scan for the
Linsel benchmark (job 15038690) is 16/18 complete, readout to follow shortly.

## RESULT — confirmatory 4-channel test: POP confirms your dual-pairing fix, STRENGTH isolates a real, specific bug (2026-07-12)

Ran your prescribed confirmatory test at the equilibrated s=0.22, L=8, beta=8,
1M-sweep point (4 seeds each, conservative SE):

| channel      | probability | strength     |
|--------------|-------------|--------------|
| z-percolation (E)          | .2863(32) | .07893(93)  |
| z-plaquette_percolation (E')| .7451(32) | .54831(267) |
| x-percolation (M)          | .7420(33) | .30455(146) |
| x-plaquette_percolation (M')| .2938(44) | .14200(207) |

Comparisons per your corrected pairing:
- E <-> M' (z-percolation vs x-plaquette_percolation): probability matches,
  1.37 sigma. Strength FAILS, 27.8 sigma.
- E' <-> M (z-plaquette_percolation vs x-percolation): probability matches,
  0.66 sigma. Strength FAILS, 80.0 sigma.

Reading against your decision rule: PROBABILITY confirms — your root-cause
diagnosis (I compared the wrong same-graph channels before) is correct, and
the binary POP observable is physically sound under the corrected pairing.
STRENGTH does NOT confirm — it fails badly on BOTH correct pairs, at a scale
(28-80 sigma) far beyond anything under-thermalization or seed noise could
produce. Since POP already rules out a duality/physics problem, this isolates
a real bug specifically to `percolation_strength`'s normalization or
definition when comparing the direct graph (vertices, #vertices=L^2) against
the dual graph (plaquettes, #plaquettes=L^2) — a code-level issue, not a
physics one.

I have not audited the `percolation_strength`/`plaquette_percolation_strength`
implementations myself (lattice.cpp) or attempted a fix — flagging for your
read on whether this is worth a Clark dispatch to audit the largest-cluster/
normalization definitions now that it's this precisely isolated, or whether
you want to reason about the expected normalization relationship first. Full
data: /scratch/a/A.Otaifi/paratoric_angle_b_dualpair_s022_20260712/.

Independently unaffected: D4, the rotate_imag_time fix, the single-z-sector
Linsel benchmark plan, and the sampling-window thermalization-convergence
scan (job 15038796, running) all proceed.

## RESULT — Linsel benchmark re-run (job 15039466): INCONCLUSIVE, with a notable internal inconsistency (2026-07-12)

Ran the re-run with the fixes in place: plaquette channel (your resolved
POP), rotate_imag_time fix (f08fd9d), and validated per-L thermalization
depths (L16=100k, L24=2M, L32=100k sweeps). L={16,24,32}, h={0.15,0.25,0.30,
0.325,0.335,0.345,0.37,0.42,0.50}, both starts, real sampling windows
(1000 samples/25 sweeps).

Crossing (plaquette POP=0.5): L16 h=0.384, L24 h=0.367, L32 h=0.364.
Pairwise crossings: 16/24 at 0.336, 16/32 at 0.343, 24/32 at 0.363 — all in
the right ballpark of h_c=0.335(2) but NOT a single common crossing, and not
obviously narrowing with L the way genuine FSS should. Local-slope nu
estimates are window-dependent, 1.2-1.9 — not a credible nu=0.65.

Cold/hot (default_spin +1/-1) disagree (>2sigma, POP and/or strength) at:
L16 h={0.25,0.325}; L24 h={0.25,0.335}; L32 h={0.15,0.37}.

The one I want to flag most: **L24,h=0.335 is the EXACT point we just
validated as converged** (jobs 15038796/15038977, 2M sweeps, 0.83sigma/
0.71sigma agreement) — same physical parameters, same thermalization depth
— and it now shows 3.55sigma/3.28sigma disagreement in THIS run. Same point,
contradictory verdict across two runs.

My read (not acted on, flagging for you): this smells like underestimated
error bars from autocorrelation rather than a real physics or wiring
problem. Percolation near a critical point can have long correlation times;
both runs used fairly short sampling windows relative to thermalization
depth (100 samples/25 sweeps in the validation run, 1000 samples/25 sweeps
here) — if the true autocorrelation time is longer than 25 sweeps, naive
stderr from nominally-independent samples is too small, and ordinary
run-to-run/seed variation can look like several sigma. This would mean the
benchmark isn't actually FAILING physically — it's under-resourced
statistically, specifically worse near h_c where slowing-down is expected.
Alternative I can't rule out: something more specific to L24 or the h=0.335
point in how this run's staging differs from the validation run's (different
seeds, different total run length) — haven't audited for that.

Not deciding between these myself — this intersects statistics/tooling
(does percolation_strength's error estimator need an autocorrelation
correction before trusting near-critical points at all?) and physics
(is nu extraction out of reach with the current sampling scheme regardless).
Full data: /scratch/a/A.Otaifi/paratoric_linsel_percolation_benchmark_fixed_20260712/.

PAUSED on further Linsel benchmark or D4-validation work (Step 5 of the
master sequence needs a passing Linsel benchmark first) pending your read.

## RESULT — your naive-error-artifact hypothesis is AUDITED and REFUTED (2026-07-12)

Followed your directive precisely: did NOT rerun sampling, fixed the
estimator, switched to the Binder-cumulant method, recalibrated the pass bar.
Here's what the audit actually found, which changes the picture:

**The stored `_error` field was already autocorrelation-corrected.** It's
computed via a stationary bootstrap (src/statistics/bootstrap.hpp), not a
naive N-independent-samples stderr. This was verifiable directly rather than
assumed. So the "naive stderr shrinks 1/sqrt(N) on correlated data" mechanism
you proposed does not apply to this data as-is.

At L24,h=0.335, with the (already-correct) bootstrap error and Neff reported
explicitly: probability z=3.546 (Neff=832.6/1000 — reasonably efficient
sampling, not severely autocorrelated), strength z=3.280 (Neff=1000/1000 —
fully independent by this measure). **The disagreement survives proper
correction. It looks real, not an estimator artifact.**

Binder-cumulant crossings (your prescribed method) also do NOT agree across
L-pairs: L16/L24 gives h=0.336, L24/L32 gives h=0.361 — a real spread, not
just noise around a single value. Additionally, this benchmark only ran 2
chains per point (cold+hot); a fair R-hat-style check per this project's own
I4 gate wants at least 4 — so even the corrected diagnostics here are running
below the project's own standard for a confident verdict.

I did make ensemble_diagnostics.py autocorrelation-aware by default anyway
(Neff reporting, a --stored-error flag, synthetic regression proving the
naive-vs-corrected distinction works correctly on synthetic data) — that's a
good standing fix regardless of whether it explains THIS discrepancy, per
your instruction to make this the default going forward.

Net: your diagnosis doesn't survive audit for this specific case. Verdict
stays INCONCLUSIVE. I don't have a replacement hypothesis and am not
guessing at one — flagging back to you rather than picking between "still an
observable/wiring issue," "genuinely needs >=4 chains before any of this is
trustworthy," or "something else entirely." Full detail + numbers: commit
`187e490`, data unchanged at
/scratch/a/A.Otaifi/paratoric_linsel_percolation_benchmark_fixed_20260712/.

PAUSED on Linsel/D4-validation (Step 5) pending your read.

## RESULT — decisive L32,h=0.335 test: cold/hot agree everywhere, but BOTH groups drift together — literal rule says CONFIRMED, but flagging a nuance (2026-07-13)

Ran your prescribed test exactly: 8 independent chains (4 cold/4 hot,
distinct seeds), L=32, h=0.335, checkpoints at 2M/4M/8M sweeps, judged via
proper 4-chain R-hat (not the old 2-chain z-score) using the now-
autocorrelation-aware ensemble_diagnostics.py.

| Depth | Plaquette POP | Plaquette strength | Base density |
|------:|--------------:|--------------------:|--------------:|
| 2M    | Rhat=1.00098 PASS | Rhat=1.00170 PASS | Rhat=1.00109 PASS |
| 4M    | Rhat=1.00298 PASS | Rhat=1.00424 PASS | Rhat=1.00111 PASS |
| 8M    | Rhat=1.00008 PASS | Rhat=1.00017 PASS | Rhat=1.00032 PASS |

Cold and hot groups agree with EACH OTHER at all three depths — by the
letter of your decision rule ("groups converge at some depth =>
under-thermalization confirmed"), this reads as CONFIRMED, with 2M already
sufficient.

But there's a wrinkle I don't want to paper over: **both groups drift
TOGETHER** as depth increases — plaquette POP goes 0.699 (2M) -> 0.717 (4M)
-> 0.734 (8M), strength similarly 0.427->0.439->0.449, while density stays
flat (~0.409) throughout. Cold and hot never disagreed with each other at
any depth, but neither has obviously reached a STATIONARY value even at 8M
sweeps -- R-hat/I5 only rules out "stuck in different basins," it doesn't
by itself prove "reached equilibrium," since both starts can drift together
in lockstep while both are still relaxing.

This doesn't support the observable/wiring hypothesis (that would predict a
persistent cold-vs-hot SPLIT, not a coherent joint drift) -- so I don't
think this points away from under-thermalization. But it does raise the
question of whether 8M sweeps is actually enough at L=32 near h_c, or
whether we're still on a slow monotonic climb that needs to go deeper still
before trusting any single-depth POP/strength value for the Linsel crossing.
I'm not picking an answer here -- flagging the drift explicitly since your
decision rule's binary framing (agree vs. disagree) didn't anticipate this
"agree with each other, but jointly still moving" case.

Full data: /scratch/a/A.Otaifi/paratoric_thermalization_decisive_l32_20260712/.
Paused pending your read on whether 8M is trustworthy enough to redo the
crossing, or whether we need an even deeper check (e.g. extend one more
decade, 16M-32M, to confirm the drift has actually plateaued) before
redoing the Linsel benchmark.

## RESULT — Binder cumulant ALSO drifts: confirms critical slowing without PT, not a benchmark failure (2026-07-13)

Ran your free decisive check on the existing L32,h=0.335 data (no new runs):
Binder cumulant of plaquette_percolation_strength at each checkpoint depth.

Pooled: U(2M)=0.491(10), U(4M)=0.504(12), U(8M)=0.517(7). Cold-only:
0.492(8)->0.502(16)->0.516(10). Hot-only: 0.490(20)->0.505(19)->0.518(10).
Increments are non-decelerating (+0.0128(129), then +0.0130(122) — same
pattern as the raw moments, not settling toward a plateau.

This lands squarely in your second branch: the Binder also drifts, so this
is NOT a benchmark failure — it's direct evidence that the global
percolation/winding structure at h_c is critically slow to equilibrate
without PT, which is exactly the phenomenon motivating this whole campaign.
I'm treating this as resolved per your decision rule (not re-litigating it)
and moving to stage the PT-BASED benchmark next: D4's validation run (PT
ladder across Linsel's fixed-lmbda=0.3 line, L=20-30, confirm PT recovers
h_c=0.335(2)/nu=0.65 WITH PT on) — D4 itself is already implemented, tested,
and independently reviewed (commit e42914d), so this is ready to run. Will
report the result once staged.

## URGENT RESULT — PT-based Linsel benchmark: NO-GO, and I independently verified a real bug in the swap-partner-selection logic (2026-07-13)

The D4/Step-5 PT validation run (L={20,24,28}, 15-rank ladders, h in
[0.15,0.50] bracketing h_c=0.335, lmbda=0.3 constant, mu=J=1, z-basis)
completed after fixing an unrelated OOM (6G/15-ranks was too little,
resubmitted at 32G). The readout is NO-GO on both mechanism and physics:

- Mechanism: 0/15 strict round-trips at ALL THREE L values. Per-link
  acceptance is overwhelmingly near-zero (most links <1e-3), with isolated
  spikes at 0.16-1.0 — nowhere close to the 20-40% healthy target.
- Physics: the Binder-cumulant crossing is not stationary within the
  sampling window (first/second-half z up to 3.5 at several h) and drifts
  across L-pairs, not converging on 0.335(2).

I did not just relay the worker's read here — given how consequential this
is, I read the actual swap code myself
(src/mcmc/extended_toric_code_qmc.hpp, ~line 3976) before writing this. The
worker's flag holds up: `partner = (pt_rank%2==phase) ? pt_rank+1 : pt_rank-1`
selects the swap partner by FIXED MPI RANK NUMBER. But on every ACCEPTED
swap, `pt_ladder_index` (and the h/lmbda values themselves) MIGRATE between
ranks (`pt_ladder_index = partner_index` on accept — this is the documented
"config-stays, label migrates" scheme). I traced a concrete 4-rank example:
starting sorted (rank R holds ladder_index R for all R), after ONE accepted
swap between ranks R and R+1, rank R now holds R+1's OLD (larger) index and
vice versa — the rank axis is no longer sorted by ladder position. Every
subsequent "adjacent-RANK" swap attempt after that can end up comparing
ranks holding FAR-APART ladder indices (a large h-gap, which the physics
correctly almost never accepts) — this is a self-consistent, mechanistic
explanation for the near-total mechanism failure observed, not just a
coincidence.

Why this was invisible until now: every prior PT correctness test this
session (G1.2/G1.3, D1's swap_period fix, D4's own unit tests) used exactly
2-rank ladders. With only 2 ranks, "adjacent rank" and "adjacent ladder
index" are trivially the SAME single pair regardless of any migration — the
bug has no way to manifest. This is the first run this session to actually
exercise 3+ ranks, and it appears to have surfaced a real, previously
untested gap between the swap-WEIGHT formula (verified correct at 2 ranks,
still correct in isolation) and the swap-PARTNER-selection mechanism (never
exercised at N>2 ranks until now).

Implication I want to flag explicitly: this means the swap-weight physics
being right does NOT mean the overall N-rank ladder mechanism is right —
those are separable claims, and only the first was actually tested before
today. Production runs (L=120, presumably many more than 2 ladder rungs)
would inherit this same problem if it's real.

I am NOT proposing a fix myself yet (this would mean redesigning how a rank
finds its "current ladder-index neighbor" — likely needs an MPI_Allgather of
(rank, ladder_index) pairs before each swap phase, then routing swap
requests to whichever rank currently holds ladder_index±1, not
pt_rank±1 — a real architectural change to the core exchange loop, not a
small patch). Given how much prior PT work this potentially calls into
question, I'm stopping to report rather than unilaterally redesigning the
core swap mechanism — flagging for your (and the PI's) explicit direction
on how to proceed. Full data:
/scratch/a/A.Otaifi/paratoric_pt_linsel_benchmark_20260713/results/.

## RESULT — swap-partner fix committed, Martina PASS, but the N=4 integration test surfaced a SEPARATE issue (2026-07-13)

Fix implemented per your exact spec (commit `0289b6c`): new
`parallel_tempering_partner_rank`/`parallel_tempering_rank_of_ladder_index`
free functions, applied to both swap-block copies, swap-weight/migration
logic untouched. Martina's independent review: PASS on all 6 spec points
checked against the diff, no defect found (line-cited: functions at
qmc.hpp:283/307, both call sites at 3923/4038, everything downstream
unchanged, both new unit test cases present and correct).

Staged the N=4/N=8 integration validation (S1b/S3-S6) on the real cluster
(local MPI in the Codex sandbox fails on PMIx/socket init, unrelated to
this bug). The N=4 PT run (job 15052942) FAILED — but importantly, it
failed via OUR OWN NEW VALIDATOR working exactly as designed: "PT ladder
index 2 is reported by both rank 0 and rank 3." This is the
permutation-integrity check catching a genuine invariant violation.

I want to be precise about what this is and isn't: this is NOT evidence
against the partner-selection fix itself (that logic is Martina-reviewed
clean, and a partner-selection bug wouldn't manifest as "two ranks report
the same ladder index" — it would manifest as ranks pairing with the wrong
partner, which is a different failure mode). This looks like it could be a
SEPARATE, pre-existing synchronization issue in the swap-accept/migration
logic itself (the code that updates pt_ladder_index on accept) that the OLD
code never had a way to detect, since no permutation check existed before
today. The new validator may simply be the first thing to ever check this
invariant and it's finding a real problem that was always there,
independent of the rank-vs-position bug we just fixed.

The N=8 run (15052943) was still in progress at last check; the 4
independent PT-off reference chains (15052944, no swap logic involved)
completed cleanly, as expected.

I am NOT resubmitting blindly or guessing at the cause myself — dispatching
a focused root-cause investigation (adding fine-grained diagnostics around
the swap-accept/migrate sequence, checking for a race between the
Sendrecv-based accept broadcast and the Allgather call in the NEXT swap
attempt, and checking the boundary between the thermalization-loop and
main-loop swap-block copies) before attempting the integration tests again.
Will report what's found. Full logs:
/scratch/a/A.Otaifi/paratoric_pt_integration_20260713/.

## RESULT — root cause of the N=4 integration failure found: a SECOND, independent MPI bug (2026-07-13)

Root cause (Clark's static hypothesis, which I independently verified by
reading the code myself): `extended_toric_code_qmc.hpp:4044,4201` —
`MPI_Bcast(&accepted, 1, MPI_INT, std::min(pt_rank, partner), MPI_COMM_WORLD)`
is called PER SWAP PAIR but targets the FULL world communicator. Whenever a
phase has multiple simultaneous disjoint pairs (any N>=4), each pair's
Bcast declares a DIFFERENT root on the SAME communicator -- undefined
behavior per the MPI standard (all participants in a collective call must
agree on the root). Some ranks can receive the WRONG pair's "accepted"
value, causing asymmetric/incorrect migration -- exactly the mechanism
that produces a duplicate ladder index.

This is a SEPARATE bug from the partner-selection issue you already
confirmed fixed. It's likely PRE-EXISTING (would have affected the
ORIGINAL code too, independent of the partner-selection bug, at any
N>=4) -- meaning the G1.2/G1.3 6-rank retraction may have had two
independent causes stacked together, not one. I was not able to get an
instrumented local-MPI reproduction running (same sandbox PMIx/socket
issue as always) -- this is confirmed by careful static analysis by two
independent readers (Clark, then me) matching the exact documented MPI
collective semantics violation, not by an observed trace. Flagging that
caveat honestly.

Proposed fix (not yet applied, dispatching now): replace the
world-communicator Bcast with pair-scoped point-to-point communication
(Send/Recv between just the two ranks in the pair, reusing the existing
tagged-Sendrecv pattern already used elsewhere in this same block for
h/lmbda/index/energy exchange). Will report back once implemented,
reviewed, and integration-tested at N>=3.
