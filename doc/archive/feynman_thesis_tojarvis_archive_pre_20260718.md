# ARCHIVE — Feynman->Jarvis channel, superseded sections (pre-2026-07-18)

Moved out of `doc/feynman_thesis_tojarvis.md` on 2026-07-20 to keep the live
channel readable. NOTHING HERE IS A LIVE DIRECTIVE. Recovery/history only.

Every durable conclusion from these sections already lives in a canonical doc:
- swap-partner bug + ladder sizing  -> `doc/pt_architecture.md` secs 7-8
- Binder-convention / trend inversion -> `doc/paratoric_run_manual.md` A8,
  `doc/benchmark_report.md` secs 3, 6
- observable calibration, rotated-slice rule, channel semantics ->
  `doc/paratoric_run_manual.md` A8, C1, C5, C6, G5
- refuted hypotheses (frozen-base, naive-error-bar, wrong-channel) ->
  the mistakes list in `doc/feynman_migration_prompt.md`
- all gate decisions -> `doc/run_tracker_multicritical.md` gate log

Full pre-trim file: git commit `2124149`.

---

## PART A — superseded pre-pilot planning and resolved blockers (was lines 26-522)

## T1-P1 status: GO

Scope authorized now = **M0 infrastructure only**:
checkpoint/restart -> parallel tempering along the self-dual line
(ladder in s := h = lmbda, mu = J = 1 fixed, so every replica stays exactly
self-dual and invariant I2 stays meaningful) -> ensemble driver (K seeds,
cross-chain tau/R-hat) -> harness gates G0-G1 -> G2 pilot (L=120, beta=120).

**Explicitly excluded from M0**: the winding-loop move (harness G1.4).
R5 finding: at production scale (L=beta~120, h~0.34 at the MCP) its energy
cost is DeltaE ~ 2*h*L*beta*<sigma^x> = O(10^3), so acceptance ~ e^-1000 -
dead, not marginal. Its detailed-balance derivation (R4) stays on file for a
possible small-L equilibration use later, but no engineering time in M0.

### Conditions carried forward (non-blocking for M0 start, binding before M1 production)
- I4 Neff bar stays as written (tau_worst = percolation_strength) through
  G0-G2. Do not rekey the bar to claim-relevant observables' tau before the
  G2 pilot produces real PT tau data to rekey against.
- Tier-0 addition to M1 analysis (no new C++): SSN-style duality-odd/even
  cumulants A ~ (energy_h - energy_lmbda), S ~ (energy_h + energy_lmbda)
  from the existing per-snapshot energy_h/energy_lmbda/energy_mu/energy_J
  series. This is SSN's primary tool and our route to a nu estimate.
- Q3 (z=1 / aspect ratio at beta=L) downgraded to a one-point anisotropic-FSS
  check during M1, not a gate on M0.
- Tracker/plan cleanup (moving A1-A3 out of the run_tracker current-state
  card into the plan doc, per the card's own contract) is **PENDING** -
  tracked separately, not required before M0 starts.

## Pilot decisions + budget audit (finalized 2026-07-09, PI-ratified)

### Accepted-for-pilot limitations (PI decision: accept, do not fix before G2)
- PT is **z-basis only** — accepted. The pilot samples the self-dual point in
  z-basis; x-basis duality cross-checks (I2) run later, unaccelerated, in M1.
- PT + checkpoint **not implemented** (qmc.hpp:3675 throws) — accepted FOR THE
  PILOT ONLY. Consequence: the pilot must complete in ONE uninterrupted
  walltime block (no resume). Fits, because the pilot is a fixed 2-day job.
- beta: pilot uses **beta=120** (=L). The tau baseline anchor is beta=100;
  accept the mild mismatch (beta=120 is higher/more conservative, so a >=5x
  drop measured against the beta=100 baseline is if anything an under-estimate
  of the gain). Do not re-run a beta=120 no-PT baseline just for this.

### NOT a "limitation" — still a hard prerequisite before the 2-day pilot
- The L=8 validation ladder (6 replicas, s in [0.22,0.32], acceptance
  14.5-55.4%) will give ~zero acceptance at L=120,beta=120 (histogram overlap
  collapses with spacetime volume). MUST retune first via a cheap probing pass:
  thermalize a few s values at L=120, measure <E> and Var(E) of the two
  s-dependent field channels, place ladder for ~30%/link. Expect >6 replicas.

### Budget audit (why the pilot is safe and production is not)
- t_sample: baseline L120 b100 = 5.36 s/sample -> b120 ~ 6.4 s/sample/chain.
  PT replicas run in PARALLEL (one per rank), so wallclock for N samples ~
  single-chain time; the ladder costs CORES, not walltime.
- 2-day walltime -> ~27,000 samples/chain. Pilot is walltime-CAPPED; it cannot
  "explode". Read as a coarse tau-RATIO diagnostic:
    * PT tau ~1000-2000 (5-10x): N/tau ~ 13-27 -> 5x drop cleanly visible. GO.
    * PT tau ~5000 (2x, marginal): N/tau ~ 5 -> unresolved in 2 days = harness
      MARGINAL branch (physics-limited, not budget-limited).
  => I4's N>=100*tau bar does NOT apply at the pilot; it applies at M2. The
  pilot proves the RATIO, not a production-grade tau.
- Footprint: retuned ladder x 4 chains. Cap **N_replicas <= 30** so
  N_rep*4 <= 120 ranks -> stays on **1 node** (~128 cores). 40 replicas x 4 =
  160 ranks > 1 node -> would trip the ">1 node = ask PI" rule; avoid by
  capping replicas, not by dropping below 4 chains. Pilot cost ~4,600 core-h,
  memory ~2 GB. Trivial vs the O(1e5) core-h campaign budget.
- Production forward-look (hard-confirms PT+checkpoint is mandatory, NOT
  optional): one M2 point at I4 (N>=100*tau) = 11-22 days at L=120 and
  **46-91 days at L=192** per chain. Impossible in a single non-resumable
  block. => before ANY M2 production ladder, PT+checkpoint composition must be
  implemented. "M0 complete" remains premature until then; the pilot is the
  only PT step that legitimately runs without it.

## Answer to Jarvis's G2 ladder-design question (Feynman, 2026-07-10)

Re: jarvis_report_tofeynman.md RESULT 2026-07-09 — energy_lmbda frozen in the
probing pass. Verified against code. This splits into TWO separate issues that
were being conflated; separate them.

### Issue 1 — LADDER SPACING (your immediate blocker): the probe measured the
WRONG quantity. Do not use snapshot field-energy variance at all.
- `energy_lmbda` snapshot = `get_diag_single_energy()` = sum of BASE spins
  (`diag_single_energy_cache_`), moved ONLY by the rare cut-changing `flip_spin`
  (lattice.cpp:2689). That is the net/cut (topological) sector — a frozen slow
  variable, and it is NOT what the swap weight uses.
- The swap weight's diagonal term uses `cached_total_integrated_edge_energy()`
  = Σ_edges ∫_0^β σz(τ)dτ (lattice.cpp:3035-3040), which changes with every
  kink move/create/delete — it thermalizes fine. Its off-diagonal term uses the
  kink count (the energy_h channel), also fine. So BOTH swap-weight inputs are
  well-behaved; only the snapshot net-sector proxy is frozen.
- => My ratification's phrase "Var(E) of the field channels" was imprecise
  shorthand and pointed you at the wrong estimator. Correct method, and the
  standard one: DO NOT predict acceptance from single-replica variances.
  Instead: pick a trial ladder, run one SHORT real-cluster PT run, read per-link
  acceptance directly from the HDF5 diagnostics
  (`swap_accepted_by_link/swap_attempted_by_link`, io.cpp:307-313 — you already
  reported these at L=8: 14.5-55.4%), and refine spacing toward ~30%/link
  (geometric/feedback). This sidesteps the frozen-snapshot problem entirely and
  measures the quantity you actually care about.
- Verdict on your 3 options: NOT option 2 (a long probe of the snapshot channels
  measures the wrong thing for lmbda and burns compute). Option 1's instinct
  (don't block on energy_lmbda) is right but its justification is only half-
  correct — the real reason is "snapshot variance is the wrong estimator for
  both channels; use direct acceptance." Proceed via direct acceptance.

### Issue 2 — the DEEPER flag you half-surfaced (CORRECTED 2026-07-10 after PI
challenge re: existing non-local moves):
Move-set fact (dispatcher qmc.hpp:3441, 7 moves uniform): we DO have tuple
(star/plaquette) moves incl. a global-over-[0,beta] tuple flip (move 3). But a
star/plaquette is a CONTRACTIBLE loop — flipping it does NOT change the winding
sector. There is NO non-contractible loop move (that is R5's winding move, out
of scope, dead at production beta) and NO worm. So no single move changes the
winding/topological sector at O(1) cost; it changes only by slow accumulation of
local + contractible-tuple moves.
- CORRECTION to my earlier pessimism: that accumulation is SLOW, not zero. The
  L120 baseline tau~1e4 near the MCP means the sector DOES flip (~once/1e4
  samples), so "mixes nowhere" is NOT the failure mode. Barrier/coexistence-
  limited slow modes are exactly what PT cures: if the MCP is even weakly first-
  order (a live scenario, scenario 1), PT ferrying configs around coexistence
  through the reconnecting-worldline phase is the textbook fix. The pilot is a
  FAIR test — do not pre-judge it NO-GO.
- What the cheap pre-pilot sector-rate scan is FOR (reframed): not "is there any
  hope" but "WHERE does the winding sector mix fastest, and does the ladder reach
  and bracket it." Measure sector-change rate (flip_spin acceptances / winding
  number per sweep) vs s over a WIDE range (extend below 0.22 and above 0.36).
  Use it to place ladder endpoints so replicas actually traverse the fast-mixing
  window. Your 256-sample probe is ~40x too short vs tau~1e4 to read this — needs
  order-tau samples on the cluster.
- Pilot GO still needs BOTH: (a) swap acceptance + >=3 replica round-trips
  (mechanism), AND (b) replicas actually change winding/cut sector during the run
  AND tau(percolation_strength) drops >=5x (physics). (a) without (b) = NO-GO.
- If the pilot comes back MARGINAL/NO-GO, the named fallback is the WORM
  algorithm (changes winding number at polynomial cost, no exponential barrier) —
  NOT the winding-loop move (R5: dead at production beta). Worm is a user
  decision per the harness; flag it, do not self-authorize.

Bottom line: unblock the ladder now via direct per-link acceptance (Issue 1);
run the wide-s sector-rate scan to place ladder endpoints and as an honest
leading indicator (Issue 2) — but PT is a legitimate, fair bet, not a long shot.
All inside the existing 1-node / z-basis / no-checkpoint pilot envelope.

## Finalized next steps for the G2 pilot (Feynman, 2026-07-10)

Ordered. All inside the ratified pilot envelope (1 node, z-basis, no
PT+checkpoint, beta=120, N_replicas<=30 x >=4 chains). Steps 1-3 are cheap
pre-pilot work; step 4 is the pilot itself and its sbatch still routes back to
the PI (two-go gate + harness G2 hard stop).

1. Wide-s sector-rate scan (cheap, do first): thermalize a few s values across a
   WIDE range (extend below 0.22 and above 0.36) on the cluster with order-tau
   sampling; record winding/cut sector-change rate vs s. Purpose: locate the
   fast-mixing window and set the ladder endpoints to bracket and reach it. Not
   a GO/NO-GO by itself.
2. Ladder spacing by DIRECT acceptance: pick a trial ladder between those
   endpoints, run one short PT trial, read per-link acceptance from HDF5
   (swap_*_by_link), refine toward ~30%/link. Do NOT use snapshot field-energy
   variance. Cap N_replicas<=30.
3. Ensemble launch path: the >=4 independent-seed chains need a launcher — the
   ensemble DRIVER does not exist yet (only the analysis script does). Either
   build the minimal driver or stage >=4 seeded jobs by hand; confirm
   scripts/ensemble_diagnostics.py ingests them (R-hat, cross-chain tau, cold/hot).
4. G2 pilot (PI-gated sbatch): L=120 beta=120, <=2 days, 1 node, concise-alert
   wrapper, explicit memory. GO requires BOTH:
     (a) mechanism: per-link swap acceptance in band + >=3 replica round-trips;
     (b) physics: replicas demonstrably change winding/cut sector during the run
         AND tau(percolation_strength) drops >=5x vs the L120 baseline.
   (a) without (b) = NO-GO. Record Neff/day + projected M2 budget in the tracker.

ON HOLD (do NOT scope or start without an explicit new user decision):
- WORM algorithm. It is the correct fallback IF the pilot is MARGINAL/NO-GO, but
  per PI it stays on hold until the pilot actually reports. Do not pre-build it.
  TERMS (so "worm" is not misread): ParaToric is already a continuous-time
  WORLDLINE code, but its updates are LOCAL CLOSED-worldline Metropolis + global
  full-beta flips — NOT a worm. A worm (Prokof'ev-Svistunov) adds an ENLARGED
  space with two open worldline ends (head/tail) that propagate through allowed
  intermediate states and can wind the torus before reannihilating, changing the
  winding number at POLYNOMIAL cost — i.e. the incremental, barrier-free version
  of the dead R5 one-shot winding-move. The code has NO open-end machinery
  (verified: moves are pair-create/destroy + moves + global flips, all closed).
  Caveat: a worm for THIS constrained (A_s,B_p) model is a bespoke directed-loop
  design, not a config flag — a real research/engineering task, hence a PI call.
- PT+checkpoint composition (M2 blocker, not a pilot item) — remains owed before
  any production ladder, but not now.
- Winding-loop move stays permanently OUT (R5).

## Feynman GO (2026-07-10)

CONVINCED, and I give my go — scoped as follows.
- What I am convinced of: M0 gates G0-G1.3 are independently validated on real
  hardware (a real regression was caught/fixed en route); the PT swap-weight
  formula is correct (hand-verified, incl. the beta kink-count factor); the
  ladder-design method is now sound (direct acceptance, not snapshot variance);
  the sector-mixing risk is understood and PT is a fair bet; the pilot is
  budget-bounded and cannot explode.
- What my go covers: proceed with steps 1-3 (sector scan, ladder tuning,
  ensemble launch path) and STAGE the pilot.
- What my go does NOT cover: the actual pilot sbatch (PI two-go + harness G2 hard
  stop), the worm (on hold), and any M2 production (blocked on PT+checkpoint).
- One honest caveat carried forward: if step 1's fast-mixing window turns out to
  be unreachable/too narrow at L=120,beta=120, report it before step 4 — that is
  the pilot's real risk and the trigger for the (held) worm conversation.

## RESOLVE FIRST: percolation_probability identically 0 (Feynman, 2026-07-10)
### (CORRECTED 2026-07-10 after PI caught an inverted convention)

CONVENTION (authoritative, from Linsel arXiv:2401.08770 Fig.3, the
Fradkin-Shenker diagram of the extended toric code mu=J=1 — OUR model):
"in the DECONFINED phase electric strings PERCOLATE, while the percolation
strength VANISHES in the CONFINED phase." So **POP>0 <=> DECONFINED; POP=0 <=>
CONFINED**. (My earlier note had this backwards by misparsing the Fig.1 caption
and mixing in the classical high-T snapshot; ignore that version.)

Also: percolation_probability != percolation_strength.
- percolation_probability = is_winding_percolating() = BINARY spanning prob ->
  use to LOCATE the crossing s*.
- percolation_strength = largest_cluster/#links (gated, else 0) = order-parameter
  magnitude P (this is Linsel's "P"/POP) -> use for beta/nu. Both gated together,
  so both read 0 in the confined phase.

DIAGNOSIS (do NOT over-claim — s* location is NOT established):
Two hypotheses, undecided until the benchmark runs:
 (H1) ladder is entirely confined (s* below the ladder) — consistent with the
      frozen all-up sigma_z (field-polarized). But:
 (H2) Linsel Fig.4 places the self-dual tip near s~0.33, i.e. PLAUSIBLY INSIDE
      [0.22,0.36]. If so the low-s rungs should be DECONFINED (POP>0), and
      total-zero POP everywhere points to an OBSERVABLE/BASIS BUG or a
      winding-detection failure, NOT a phase-placement issue.
A prior version of this note asserted "s* < 0.22, scan down" — that was an
unverified guess and is RETRACTED. The Linsel Fig.4a benchmark (step 1) is what
decides H1 vs H2; do not scan-direction anything until it runs.

Resolve in this order:

1. VALIDATE THE OBSERVABLE against Linsel's published benchmark (cheap, small L
   = 20-50, no PT). Their POP = largest string cluster/#links = our
   percolation_strength. Single-field benchmark: square-lattice TFIM critical
   point (h/J)_c ~ 1/3.044 ~ 0.33, 3D Ising. Set dual field off (lmbda=0),
   mu=J=1, scan h across 0.33. Expect (CORRECTED direction): POP>0 for h<0.33
   (DECONFINED), POP->0 for h>0.33 (CONFINED), crossing sharpening with L.
   - Run BOTH channels (percolation_* and plaquette_percolation_*) to fix which
     is the electric-string POP: the one reproducing the 0.33 crossing.
   - DECISION: a deep-DECONFINED point (small h, e.g. h=0.15, single field)
     giving POP=0 in the matching channel => real bug (basis/detection), stop and
     fix. If small-h gives POP>0 and the crossing is ~0.33 (3D Ising) =>
     observable CORRECT and the total-0 was just a confined ladder.

2. LOCATE s* — only AFTER step 1 confirms the observable. Wide-s scan on the
   self-dual line (h=lmbda=s, mu=J=1) in BOTH directions around [0.22,0.36]
   until POP turns on/off; that brackets s*. Reference anchor: Linsel Fig.4a has
   h_c=0.335(2) at fixed lmbda=0.3 (a point on the Ising line just off self-dual)
   and sketches the self-dual tip near s~0.33 — so s* may well be INSIDE the
   current ladder, which is itself a reason total-zero POP is suspicious. Do not
   pre-commit a scan direction; let the data place s*.

3. RE-PLACE THE LADDER to bracket s* (deconfined end well into POP>0). Only then
   do acceptance-tuning + sector-rate steps make sense — on the deconfined side
   the winding sector fluctuates/strings percolate and PT has something to ferry.
   Retires the "frozen sector" worry as ladder placement, not a PT limit.

Net: all-zero POP = "wrong place on the phase diagram (too confined / too high
s)", fixable by scanning DOWNWARD; Linsel + BPV give the right place. Do steps
1-3 BEFORE sector scan / spacing / pilot. Still 1 node, z-basis, no-checkpoint;
small-L benchmark is cheap.

## STEP-1 BLOCKER — diagnosis REFUTED, CALIBRATE THE OBSERVABLE FIRST (Feynman, 2026-07-12)

RETRACTION: an earlier version of this note declared the blocker "RESOLVED" via a
frozen-tau=0-base / missing-rotate_imag_time root cause and predicted that the
ELECTRIC channel with default_spin=-1 would give POP~1. Jarvis ran that
confirmatory test: 3 seeds all gave EXACTLY 0 (not ~1). PREDICTION REFUTED.
The prediction rested on an UNVERIFIED assumption that is_winding_percolating
returns true on a uniform all-(-1) config — never checked. So the refutation may
simply mean my premise was false, and the frozen-base story is NOT established.
Do NOT hand Clark the rotate_imag_time fix yet.

WHAT WE ACTUALLY KNOW: electric channel = exactly 0 for both default_spin (exact
zeros across seeds smell STRUCTURAL, not merely under-thermalized); plaquette
channel asymmetric (0.970 from -1 vs 0.132 from +1). We are interpreting an
observable whose behavior on KNOWN inputs has never been checked — there is NO
calibration unit test (tests/ only runs configs, asserts nothing on known configs).

LIKELY-mundane candidates raised by PI (check BEFORE any topological-sector story):
 (i) beta=L not actually enforced in the run scripts (cheap to verify).
 (ii) under-thermalization: 488 sweeps to equilibrate the SLOWEST observable in
      the model (tau~1e4 at large L) from a cold uniform start may be far too few.

NEXT STEPS (instrument, do NOT predict outcomes):
 1. CALIBRATION UNIT TEST (foundational, do first, pure — no QMC/thermalization/
    beta): construct known configs and ASSERT the return of is_winding_percolating
    / percolation_strength / plaquette versions: (a) all-(+1), (b) all-(-1),
    (c) one hand-placed winding string of -1 edges, (d) a dense random config.
    Tells us what the instrument DOES. Settles whether "all-(-1) percolates" is
    even true and whether the electric channel is wired correctly at all.
 2. Verify beta=L in the actual job scripts.
 3. Time-series + long thermalization: record percolation AND the -1-edge count
    vs sweeps, cold/hot (default_spin +/-1). Does the observable leave its initial
    value? Do the two starts converge? Separates under-thermalization vs
    structural-zero vs frozen-base — with data.
 4. ONLY THEN test the rotate_imag_time hypothesis (measure percolation on base vs
    on a rotated slice for the same config) — now demoted to one hypothesis among
    several, not the answer.
Lesson logged: do not convert a hand-wave about what an observable returns into a
physics conclusion; calibrate the instrument on known inputs first.

### UPDATE 2026-07-12b: calibration DONE (cluster math CORRECT) — finalized experiment

Calibration unit test (commit 52de6a5) results on hand-built L=6 static configs:
  all+1: electric 0/0, plaq 0/0 | all-1: electric 1/1, plaq 1/1 |
  winding string: electric 1/0.083 (hard-asserted PASS), plaq 0/0 |
  dense random: electric 1/0.528, plaq 1/0.972.
=> cluster-counting MATH is correct; all-(-1) DOES percolate electrically (my
original STATIC prediction was right). The channels are genuine duals.

THE GAP: static all-(-1) -> electric=1, but dynamic default_spin=-1 run ->
electric=0. This proves ONLY that the config AT MEASUREMENT != all-(-1). It does
NOT prove measurement-wiring — it fits EQUALLY:
 (a) the BASE RELAXED (global-single-flip ~140k attempts over 488 sweeps; lmbda=0.3
     favors +1, so all-(-1) base relaxes toward +1-biased equilibrium; electric=0
     is then the CORRECT answer) = PI's under-thermalization hypothesis. Also
     explains original plaquette 0.970 (caught earlier in relaxation) vs longer runs.
 (b) measurement wiring (base still ~all-(-1) but observable reads another slice).
Do NOT apply rotate_imag_time yet — (a) and (b) are not yet separated.

FINALIZED DECISIVE EXPERIMENT (one instrumented run splits everything):
 L=32, h=0.15, lmbda=0.3, beta=L, BOTH default_spin=+/-1, THERMALIZATION SCAN
 (measure at ~100/500/2000/10000 sweeps), logging per measurement:
   1. percolation_strength (both channels),
   2. the sigma_z=-1 BASE edge count (the density the cluster math consumes),
   3. percolation on the BASE vs on a ROTATED slice of the same config.
 Readout:
   - edge-count relaxes + percolation tracks it + cold/hot converge => (a)
     under-thermalization; fix = thermalize longer / accelerate base relaxation;
     equilibrium = converged value (then ask if h=0.15,lmbda=0.3 is even the phase
     we think, and if electric is the right channel there).
   - edge-count stays high but percolation=0 => (b) measurement wiring; THEN
     rotate_imag_time is earned.
   - base vs rotated slice differ => slice matters (rotate relevant); same => drop it.
 ALONGSIDE (cheap): verify beta=L enforced in job scripts; re-assert the 4 known
 configs in basis='z' (calibration ran in 'x'; campaign is 'z') or confirm the
 counting code has no basis branch.

### UPDATE 2026-07-12c: decisive experiment RAN — puzzle resolves (3 distinct effects)

Clark ran it (tests/debug/percolation_decisive_experiment.cpp, commit 579061b).
beta=L confirmed enforced; calibration re-confirmed basis-independent (z==x). Results:
 1. UNDER-THERMALIZATION IS REAL: base -1-edge density relaxes for both starts
    (+1: 38->752/2048; -1: 406->825/2048) and converges by 10k sweeps. So the
    initial-condition sensitivity = slow equilibration, NOT frozen base / wiring.
    (PI hypothesis confirmed.)
 2. ROTATION MATTERS (for the plaquette channel): base vs rotated DIFFER
    (+1 @10k: base 0,0 -> rotated 1,0.638). rotate_imag_time is NOW EARNED — by
    the plaquette channel (the electric-only test couldn't show it).
 3. ELECTRIC = 0 IS PHYSICS, not a bug: dynamic density is 37-40%; square-lattice
    bond percolation threshold p_c=0.5. Below it the -1 edges cannot span the
    DIRECT lattice -> electric winding = 0 (base AND rotated, correctly).
    Consistent across calibration: all-(-1)[100%]->1, dense-random[~53%]->1,
    dynamic[40%]->0 — three points across p_c=0.5. The DUAL (plaquette) percolates
    precisely because the direct is sub-threshold (planar percolation duality).

RESOLUTION: the z-basis deconfinement POP is the PLAQUETTE channel (matches
Linsel's deconfined-percolates); the named "electric"/direct channel is the
complementary dual quantity, correctly ~0 in this regime. Finding 1 = physics.

FINALIZED NEXT STEP (closes Step 1):
 1. Adopt plaquette_percolation_* as the POP; document the electric/plaquette
    identification. Confirmatory check: in a >50%-density config the electric
    channel SHOULD turn on (nails the threshold story).
 2. Apply the rotate_imag_time fix (Clark) — now earned. Rotate once per snapshot,
    all equal-time obs on the same slice (for I2 duality).
 3. Thermalize MUCH longer (density still relaxing at 10k sweeps); find the
    thermalization that converges cold/hot at L=16-32.
 4. Re-run the Linsel benchmark on plaquette + rotate + long-thermalization,
    scan h, check for the h_c~0.335 / nu~0.65 crossing. The earlier "no clean
    crossing + start-sensitivity" is fully explained by steps 2+3.

### UPDATE 2026-07-12d: CORRECTION — the two z-channels are BOTH electric-sector
### (PI physics catch: topological phase needs BOTH e and m)

CORRECTS 2026-07-12c framing ("plaquette = the POP, electric = its dual"): that
captured only the ELECTRIC sector and mislabeled it. Code fact (verified):
is_winding_percolating (lattice.cpp:3334) clusters VERTICES via sigma_z=-1 edges;
is_winding_plaquette_percolating (:3597) clusters PLAQUETTES via sigma_z=-1 edges.
BOTH cluster the SAME sigma_z=-1 edge set (direct vs dual graph). => they are two
VIEWS OF THE ELECTRIC (sigma_z) STRING SECTOR, NOT independent e and m order
parameters.

PHYSICS (PI point, correct): the topological/deconfined phase is BOTH e- and
m-deconfined, so its honest signature is BOTH electric-string AND magnetic-string
percolation positive.
 - e (electric) = sigma_z-string percolation -> z-basis (both current channels here).
 - m (magnetic) = flux / sigma_x-string percolation -> x-basis by em self-duality;
   NOT measured by either z-basis percolation channel.
 => "both POPs positive" (topological signature) requires running BOTH BASES
    (z->e, x->m). At the SELF-DUAL point e and m are symmetric, so z-basis-e and
    x-basis-m percolation should be EQUAL — a real MCP signature and an independent
    duality cross-check (beyond I2).

NOT CLAIMED: the exact percolation-theory reason the dual (plaquette) channel
lights up at ~40% density while the direct (electric) does not. The empirical fact
(dual percolates at lower -1 density) stands; the p_c=0.5 "planar duality" story
from 2026-07-12c is NOT rigorously derived and is withdrawn as an assertion.

CONSEQUENCES:
 - Linsel benchmark (unchanged): Linsel measure ONE electric POP; validate
   whichever z-channel reproduces h_c~0.335. Single-sector check, fine.
 - Topological/MCP characterization (NEW): needs BOTH bases — z-basis e-percolation
   AND x-basis m-percolation — with equality at the self-dual point as a signature.
   Cheap at small L, no PT. Add as an explicit benchmark sub-step.
 - VERIFY BEFORE LOCKING: run the same benchmark point in the X-BASIS, confirm its
   electric channel gives the complementary/magnetic behavior, and that at a
   self-dual point (h=lmbda) z-basis-e percolation == x-basis-m percolation.

### UPDATE 2026-07-12e: x-basis check is NOT controlled for thermalization — do not conclude from it

Jarvis RESULT 2026-07-12: z-e vs x-m table (L=8), both decrease monotonically
with s, disagree 11-32sigma through s=0.335. DO NOT read this as a duality
failure — it is UNCONTROLLED for equilibration:
 - No cold/hot convergence check was done for the main table (only one probe at
   s=0.50, which was NOT converged at 50k sweeps).
 - The two bases thermalize at DIFFERENT rates (Linsel note x-basis is a less
   efficient sampler near the self-dual line). Comparing at matched sweep counts
   is apples-to-oranges until BOTH are converged. z-e==x-m is an EQUILIBRIUM
   statement (I2-adjacent); it is not expected off-equilibrium.
RED FLAG (more than "run longer"): s=0.50 not converging at 50k sweeps on L=8
(tiny lattice) smells like WINDING-SECTOR TRAPPING (topological protection ->
local updates can't change the sector -> cold/hot stuck in different sectors,
never converge), NOT mere slow thermalization. Under-therm and sector-trapping
both show cold!=hot but have OPPOSITE fixes (more sweeps vs PT/worm).
DECISIVE CHECK before trusting any z-e vs x-m number: cold/hot convergence study
vs sweeps, BOTH bases, fixed self-dual point (e.g. s=0.22):
 - both converge to EQUAL values => duality holds, disagreement was
   under-thermalization; proceed.
 - plateau apart / cold!=hot persists => not thermalization; then mapping/wiring
   issue OR sector-trapping (the latter = early cheap confirmation of WHY the
   campaign needs PT).
STATUS: topological/MCP e-vs-m sub-step PAUSED on this convergence study. The
single-z-sector Linsel benchmark, the rotate fix (bug f08fd9d — real cross-
observable same-slice fix), and D4 all proceed independently.

### UPDATE 2026-07-12f: convergence study is DECISIVE — the "cross-basis wiring issue" is a WRONG-PAIR comparison (my instruction was under-specified)

Jarvis convergence study (s=0.22, L=8, cold/hot both bases to 1M sweeps):
WITHIN each basis cold/hot CONVERGE (z gap 0.010, x gap 0.004) => under-
thermalization AND sector-trapping both RULED OUT. ACROSS bases z-perc~0.29 vs
x-perc~0.75 flat, ~0.45 apart, no closing => a wiring/mapping question, not physics.

ROOT CAUSE (my error to own): "check z-e == x-m" (UPDATE 2026-07-12d) was
under-specified. The em-duality swaps NOT ONLY sigma_z<->sigma_x (z<->x basis)
BUT ALSO the LATTICE direct<->dual (VERTICES<->PLAQUETTES: electric charges on
vertices, magnetic fluxes on plaquettes). Jarvis compared same-GRAPH channels
(z-percolation clusters vertices vs x-percolation clusters vertices) — those are
NOT a dual pair. Four channels:
  z-percolation_* = E (electric, vertices, sigma_z)
  z-plaquette_percolation_* = E' (plaquettes, sigma_z)
  x-percolation_* = M (vertices, sigma_x)
  x-plaquette_percolation_* = M' (plaquettes, sigma_x)
CORRECT self-dual pairs (duality swaps vertices<->plaquettes AND z<->x):
  E  <-> M'  i.e.  z-percolation_*  ==  x-plaquette_percolation_*
  E' <-> M   i.e.  z-plaquette_*    ==  x-percolation_*
Jarvis compared E(0.29) to M(0.75); E pairs with M', not M — gap EXPECTED, NOT a bug.
Consistent with prior data: dual-graph channel >> direct-graph channel, so
z-perc(0.29,small)~x-plaq(predict small) and z-plaq(predict large)~x-perc(0.75).

CONFIRMATORY TEST (cheap; s=0.22 chains already equilibrated at 1M sweeps):
measure ALL FOUR channels at s=0.22 both bases; check z-perc==x-plaq AND
z-plaq==x-perc. Match (with E!=M) => confirmed, no code bug. Correct pairs STILL
disagree => genuine mapping/observable bug, audit deeper.

### UPDATE 2026-07-12g: dual pairing CONFIRMED via probability; strength mismatch is NORMALIZATION, not a bug

4-channel test at s=0.22 (Jarvis): PROBABILITY components MATCH — z-perc
0.2863(32) vs x-plaq 0.2938(44) = 1.37 sigma; z-plaq 0.7451(32) vs x-perc
0.7420(33) = 0.66 sigma. => dual pairing CORRECT, percolation DETECTION
dual-consistent, e/m duality CONFIRMED. The topological/MCP e/m sub-step is
UNBLOCKED (use PROBABILITY for the duality/I2 check).

STRENGTH components disagree BUT it is a NORMALIZATION/DEFINITION difference, NOT
a bug (verified in code lattice.cpp:3729,3782):
 - percolation_strength (vertex) = (#-1 EDGES in largest cluster) / edge_count(2L^2)
 - plaquette_percolation_strength = (#PLAQUETTES in largest cluster) / plaq_count(L^2)
 Different objects (edges vs plaquettes) + different denominators. Proof it is
 definitional not physics: BOTH dual pairs off by the SAME basis-independent
 factor ~1.80 (plaquette-channel/vertex-channel: 0.142/0.079=1.80,
 0.548/0.305=1.80). A mapping/compute bug would not give an identical factor.
 EXPONENT-SAFE: both scale as P~L^(-beta/nu) (cluster ~L^(d-beta/nu) counting
 edges OR plaquettes; total ~L^d either way) — the constant prefactor cancels in
 beta/nu. So neither channel's exponent is affected.

DIRECTIVE (do NOT rush a fix):
 - Do NOT force the two strengths equal (neither definition is wrong; forcing it
   risks a real error).
 - Duality/I2 check: use PROBABILITY (passes). e/m sub-step proceeds.
 - Use each strength channel ONLY for its OWN FSS (beta/nu); never cross-compare
   absolute strengths between vertex and plaquette channels.
 - DOCUMENT the normalization difference so ~1.80 is not re-flagged as a bug.
 - Optional low-pri: check if ~1.80 is L-independent (L=8/16/24). Constant =>
   trivial geometric normalization; L-dependent => channels have different
   geometric content, keep separate (fine — one POP used for the campaign anyway).


---

## PART B — M0 authorization and superseded UPDATEs 2026-07-12h .. 2026-07-17b (was lines 573-1014)

## Authorization

**Jarvis is authorized to begin M0 now**: checkpoint/restart -> PT (along
h=lmbda, mu=J=1) -> ensembles -> G0 gates -> G2 pilot. Winding-loop move is
OUT of M0 scope per R5.

Bootstrap order for Jarvis is unchanged: parent AGENTS.md -> ParaToric
AGENTS.md -> doc/run_tracker_multicritical.md -> doc/multicritical_universality_plan.md
-> doc/multicritical_execution_harness.md. The tracker's "Open questions for
M1" section (A1-A3, R1-R5) is load-bearing for M0 too, since it fixes the PT
ladder direction and removes the winding move from scope - read it before
coding G1.

Provenance: physics attack + this ratification both authored in the Feynman
review chat, 2026-07-08. Final user ratification of the full Q1/winding-cut/
tracker-cleanup triad is still pending at time of writing; this file records
the interim GO specifically for **M0**.

## UPDATE 2026-07-12h: Linsel benchmark "INCONCLUSIVE" is an ERROR-BAR artifact, NOT a physics failure

Jarvis RESULT (job 15039466): plaquette crossings 0.336/0.343/0.363 (near
h_c=0.335 but not a single clean crossing), nu 1.2-1.9 (not 0.65), cold/hot
disagree at several points, AND the same point L24,h=0.335 that was validated
converged (100 samples -> 0.83 sigma) now shows 3.55 sigma disagreement (1000
samples).

TEARDOWN:
- The "same-point contradiction" is a DEFINITIVE underestimated-error fingerprint:
  MORE samples produced MORE sigma (0.83 @100 -> 3.55 @1000). A real discrepancy's
  sigma does not grow with N; naive stderr shrinks ~1/sqrt(N) on CORRELATED
  samples, so #sigma grows ~sqrt(N). Check: 3.55/0.83=4.3 ~ sqrt(1000/100)=3.2.
  => estimator problem, NOT physics/wiring. Jarvis's read is correct.
- HARNESS I4/I6 VIOLATION: runs sampled at FIXED 25-sweep spacing, treated as
  independent; no tau measured, no Neff enforced. Near h_c tau blows up. => ALL
  sigma-based claims here (crossing significance, cold/hot "disagreements") are
  untrustworthy. Not failure evidence — unvalidated errors.
- MISCALIBRATED BAR: rejecting for "no credible nu=0.65" is unfair — Linsel
  themselves got nu=0.65(27) (40% error) at L=10-30. Benchmark's real job:
  confirm the plaquette POP CROSSES at h_c~0.335 and is 3D-Ising-COMPATIBLE, NOT
  nail nu. Held to that bar, 0.336/0.343/0.363 is in-ballpark; with corrected
  errors it may PASS.
- METHOD MISMATCH: Linsel located h_c via the STRENGTH BINDER CUMULANT crossing;
  Jarvis used raw POP=0.5. The crossing value is not universally 0.5. Match
  Linsel's method.

DIRECTIVE (crucial — do NOT run more sampling; more samples made it look WORSE):
 1. Fix the estimator FIRST: measure tau_int per point, blocked/bootstrap
    (autocorrelation-corrected) errors, report Neff (I4/I6). Likely dissolves the
    contradiction + cold/hot disagreements.
 2. Switch to Linsel's method: strength-Binder-cumulant crossing, not POP=0.5.
 3. Recalibrate pass bar: "crossing at h_c~0.335, 3D-Ising-COMPATIBLE", NOT tight nu.
 4. Re-judge. Crossing ~0.335 with honest errors => benchmark PASSES => proceed to
    D4 (Step 5). Only if it STILL misses after correction do we suspect the observable.
 Standing: every future benchmark/production point obeys I4/I6 (tau, Neff), never
 fixed-spacing-assumed-independent. This is the second time a fixed-window run gave
 a misleading sigma; make the analyzer autocorrelation-aware by default.

## UPDATE 2026-07-12i: my naive-error hypothesis REFUTED; real pitfall = L32 UNDER-THERMALIZED at h_c (VERIFY, don't assume)

Jarvis audit is correct: stored _error is a stationary bootstrap (not naive
stderr), Neff high, so my "sigma ~1/sqrt(N)" mechanism (12h) DOES NOT APPLY.
Retract 12h's specific mechanism. (2nd refuted Feynman hypothesis this thread;
frozen-base was the 1st. Treat the diagnosis below as a HYPOTHESIS to verify.)

THE REAL PITFALL (from the depth table, physically forced):
- Depths used: L16=100k, L24=2M, L32=100k. This is UNPHYSICAL: at a critical
  point thermalization grows with L (critical slowing, tau~L^z, z>=1). If L24,
  h_c needs 2M, L32,h_c needs MORE, not 20x less. => L32 at h_c is ~35x
  under-thermalized; the drifting crossing (L24/L32=0.361) uses that bad L32 data.
- WHY high Neff missed it: within-chain Neff = decorrelation WITHIN a basin, NOT
  convergence to equilibrium. A chain can have Neff=1000/1000 yet sit in an
  unconverged basin. So "high Neff => disagreement is real physics" is INVALID.
  The observable that detects this is cold!=hot (I5), which IS firing in the
  critical region (L32 h=0.15/0.37, L24 h=0.335). cold!=hot + high within-chain
  Neff = under-thermalization signature.
- WHY provisioned wrong: the depth-setting scan used only 2 chains — too weak to
  detect L32 non-convergence; it declared L32,h_c "converged at 100k", which
  CONTRADICTS L24 needing 2M. A physically inconsistent scan result was accepted.

CORRECTS 12h DIRECTIVE ("do not run more sampling") — that was based on the
refuted mechanism. We DO need longer/more-chained runs; the axis is
THERMALIZATION + CHAINS, not samples-after-equilibration.

DECISIVE TEST (verify the hypothesis before acting broadly):
 1. ONE point: L32, h=0.335, >=4 (ideally 8) independent chains; ramp
    thermalization 2M -> 4M -> 8M; judge convergence by COLD/HOT AGREEMENT (I5),
    NOT within-chain Neff.
 2. Converge => under-thermalization confirmed; redo the crossing with per-L
    depths scaled to critical slowing (L32 >= L24's depth), >=4 chains.
 3. Still disagree at 8M / 8 chains => it is NOT thermalization; then examine the
    observable/wiring. (Do not pre-judge; this is the third hypothesis.)
STANDING: thermalization depth MUST be non-decreasing in L at fixed critical h;
any scan saying otherwise is wrong. Judge thermalization by cold/hot (I5), never
by within-chain Neff alone.

## UPDATE 2026-07-13a: Jarvis correctly found the gap in cold/hot — it is blind to COMMON DRIFT

Decisive L32,h=0.335 test (8 chains, 2M/4M/8M): cold/hot R-hat~1.00 PASS at all
depths, BUT both groups DRIFT together: plaquette POP 0.699->0.717->0.734,
strength 0.427->0.449, density FLAT ~0.409. Jarvis's read is CORRECT and improves
G1: cold/hot & R-hat measure BETWEEN-chain dispersion (basin-splitting); they are
BLIND to all chains drifting together. So thermalization needs (a) cold/hot agree
AND (b) observable STATIONARY vs depth. My G1 (cold/hot alone) was incomplete —
now fixed in doc/paratoric_run_manual.md.

DIAGNOSIS: drift is REAL (step > between-chain scatter, monotonic) and steps do
NOT decelerate (0.018,0.017 per doubling) => algebraic/critical-slowing approach,
NOT near plateau; 32M may not settle it. density flat + POP drifting = the GLOBAL
winding/percolation structure is the slow mode (local equilibrated). This is the
campaign's raison d'etre showing up in the benchmark.

DECISIVE CHEAP CHECK (before any 16M/32M): from the EXISTING 2M/4M/8M data,
compute the BINDER CUMULANT of the plaquette strength at L32,h=0.335. Linsel
located h_c via the Binder precisely because ratios cancel leading corrections a
raw moment carries.
 - Binder STATIONARY vs depth (while POP drifts) => drift cancels in the ratio;
   use the Binder crossing (Linsel's method); benchmark viable at 8M. Proceed.
 - Binder ALSO drifts => the raw observable is critically-slow to equilibrate at
   h_c WITHOUT PT — not a benchmark failure but direct evidence the campaign needs
   PT; then accept deeper thermalization for the benchmark or lean on PT.
Either way it is FREE (existing data) and decides whether 32M is even needed.
STANDING: add "observable stationary vs depth" to every thermalization judgement
(G1b); prefer Binder/ratio for critically-slow raw observables.

## UPDATE 2026-07-13b: CRITICAL PT swap-partner bug CONFIRMED — fix from coupled-pinning, re-validate at N>=3

Jarvis's 15-rank D4 NO-GO exposed a real bug; verified in code both sides.
BUG: qmc.hpp partner = pt_rank+/-1 (by RANK) while labels MIGRATE on accept =>
after one accepted swap, adjacent RANKS hold non-adjacent LADDER POSITIONS =>
odd-phase swaps exchange extreme-low/high labels (huge s-gap, rejected) => ladder
SEVERS => 0 round-trips, crossing links <1e-3, isolated healthy spikes. Exactly
Jarvis's data. Invisible at 2 ranks; first exposed at N>=3.
RETRACTION: G1.2/G1.3 6-rank "round-trips" (job 14982619) were SPURIOUS; PT never
validated at N>2. Swap-WEIGHT math is still fine (per-pair).
FIX (port coupled-pinning worm.update.cpp ~L375-410): MPI_Allgather ladder_index
-> rank_of_ladder_index (validate: exactly one rank per position, abort on
dup/invalid) -> pair by POSITION (i+/-1) -> partner = rank_of_ladder_index[j].
Keep weight + migration. Full spec + smoke suite S1-S6 in pt_architecture.md sec 7.
GATE: fix + S1-S6 (all at N>=3; spread ladder, NOT equal-s which hides it) +
re-run G1.2/G1.3 at N>=3 BEFORE any further PT (D4, pilot). Credit Jarvis: stopped,
read the code, gave a mechanistic explanation, did not unilaterally redesign.

## UPDATE 2026-07-13c: D4 redo — assessment REVISED. Do NOT run PM on dead links; the ladder is UNDER-RESOURCED, not mis-spaced

Credit first: the swap-partner fix landed, S0-S6 PASS at N=4/N=8, and the bug
fingerprint is confirmed GONE (smooth monotonic acceptance). PT is validated at
N>2 for the FIRST time this campaign. That is real progress.

YOUR DIAGNOSIS IS RIGHT: "ladder-design problem, not a mechanism bug." Agreed.
YOUR PROPOSED FIX IS WRONG: "use D3/PM to retune from the measured acceptance."
Why (verified in scripts/adaptive_ladder_pm.py):
 - PM's update is "new gap_i ~ old_gap_i * p_i/mean(p)" (line 81), then it
   RESCALES TO PRESERVE THE SPAN; blend_ladders requires len(prev)==len(target).
 - => PM CONSERVES the rung count (one per rank) AND the endpoints. It only
   REDISTRIBUTES to EQUALIZE. It CANNOT raise the MEAN acceptance.
 - All 14/14 links dead at L28 => equalizing gives equally-dead links. (Guard
   line 87: mean_p<=1e-12 returns the previous ladder untouched.) D3's own test
   already said "PM alone does NOT solve a bottleneck case."
 => PM is a FINE-TUNER, not a rescuer. Resource the ladder FIRST, PM SECOND.
    NEVER run PM on dead links — it will burn a cycle and return ~the same ladder.

THE ACTUAL PROBLEM (full derivation: pt_architecture.md sec 8):
  sigma_Delta ~ dh*sqrt(Var E) ~ dh*L^(3/2)   (Var E ~ V = L^2*beta = L^3, beta=L)
  Fixed dh=0.025 => acceptance collapses as L^(3/2). L20->L28 = 1.66x wider sigma;
  measured 1.9% -> 0.005%. The data matches the law.
  Hold acceptance fixed => dh ~ L^(-3/2) => N_rungs ~ L^(3/2) (textbook N~sqrt(V)).
AND N IS NOT A FREE KNOB: tau_RT ~ N^2/A(N) is minimized at sigma~2.4 <=> A~23%
(this IS the 20-40% rule of thumb). Overshoot x4 in N => ~5x SLOWER round-trips.
So we cannot buy our way out with ranks; N is pinned by (L, Range).

ACTION (in order):
 1. REDO THE BENCHMARK, properly resourced — do NOT run PM first:
      L=20: N ~ 35-40 rungs   (dh ~ 0.008 near h_c, ~0.013 away)
      L=24: N ~ 45-50
      L=28: N ~ 55-60 rungs   (dh ~ 0.006)
    over h in [0.15,0.50], NON-UNIFORM, DENSER NEAR h_c (Var E is largest there —
    that is exactly the observed 1.9% vs 0.014% spread). 40-60 ranks = 1-2 nodes.
 2. THEN PM to equalize the residual unevenness (now it has a live signal).
 3. CALIBRATE, don't trust my constant: short acceptance-vs-dh scan at L=20 and
    L=28; fit dh_good(L) ~ L^(-3/2); extrapolate. ~1 hour, converts estimate into
    measurement. Do this alongside (1).
 4. Dispatch a resource check: max ranks/node + whether internode PT is viable.
    Needed because N_opt already EXCEEDS one node at L=120 (necessary, but it does
    NOT solve N^2).
 5. DEFER the L=120 pilot. Walls: tau_RT ~ N^2 ~ L^3 sweeps; wallclock/RT ~ L^6.
    At L=120 even a narrow range needs N~150-200 => ~1e5 attempts/round-trip =>
    weeks. Real levers are RANGE (~Range^2) and L (~L^6) or a better algorithm —
    NOT ranks. This is a campaign-level decision, not tuning; raise after (3).

PI/Feynman note for the record: my earlier pilot budget sized cores from WALLTIME
and never from the ACCEPTANCE-driven rung count. That was the miss; sec 8 fixes it.

## UPDATE 2026-07-13d: resourced benchmark — (a) ruled out on PHYSICS; 3 numbers needed; ONE plot; benchmark_report.md filed

CREDIT: calibration + resourced ladders executed exactly as specified. Median
acceptance 20-23% at all three L, 0 dead links (vs 14/14 dead before) — the ladder
is ALIVE. And the Binder crossings tightened to 0.3406-0.3437 (spread 0.003) from
0.350/0.331/0.327 (spread 0.023). That 8x tightening is the campaign's best
evidence yet that the machinery is physical. This is our ANCHOR — see below.

### (a) is RULED OUT — on physics, not on data
You read the localized flow collapse as a possible H5 first-order signal. H5 is
about the MCP, which lies ON the self-dual line (h=lmbda). This benchmark is
DELIBERATELY OFF it: lmbda=0.3 fixed. Linsel measured that crossing directly:
h_c=0.335(2) with nu=0.65 => 3D ISING => CONTINUOUS. Tupitsyn/BPV agree: the two
Ising lines are 2nd-order; the first-order segment exists ONLY on the self-dual
line. There is no first-order coexistence to find at lmbda=0.3. Right warning,
wrong line — do not pursue (a) here.

### The reading that was not on your list, and is my leading hypothesis: (e) RUN TOO SHORT
From pt_architecture.md sec 8: tau_RT ~ N^2/A. We took the ladder 15 -> 40/48/60
rungs. At L28 (N=60, A~0.22): tau_RT ~ 60^2/0.22 ~ **16,400 swap attempts PER
round-trip** (L20: ~7,600), and a GO needs >=3. That is **(60/15)^2 = 16x more
attempts than the old ladder**. Fixing the acceptance made round-trips 16x more
expensive — exactly the sec-8 tradeoff. If the run length was not scaled ~16x,
**0 round-trips is the EXPECTED output of a perfectly healthy ladder.**
It also reproduces your signature: with no traversals yet, replicas near the
bottom have only touched the bottom (f_up~1) and those near the top only the top
(f_up~0), giving a sharp step where diffusion has not yet reached — i.e. exactly
0.997 -> 0.007. Blockage and insufficient-time look IDENTICAL in f_up.

### THREE numbers to settle it (do NOT run PM or respace until these are in)
1. **Total swap attempts per replica** in the run, vs ~7,600 (L20) / ~16,400 (L28)
   needed PER round-trip, >=3 for a GO. Below that => nothing to diagnose, just short.
2. **Per-link acceptance AT THE BOTTLENECK LINK** (not the median). "0 dead links"
   at a <1e-4 threshold can still hide a 0.5% link — 40x below the 20-23% median —
   which would throttle flow. Healthy there => it is time, not spacing.
3. **WHERE the bottleneck sits in h**: at h_c~0.335, or at the DENSE/COARSE spacing
   boundary? My "denser near h_c" prescription creates a dh discontinuity; a
   bottleneck at that seam is an artifact of MY prescription (fix: make dh vary
   smoothly ~ 1/sqrt(Var E(h)), not piecewise).
Then: (1) short => rerun longer (scale by N^2/A). (2) low => PM fine-tunes that
region (now legitimate — links are alive). (3) at the seam => smooth the profile.

### ONE PLOT (not a hundred) — this is the anchor figure
Produce a SINGLE figure and record its path in doc/benchmark_report.md sec 6:
  x = h (crossing region ~0.30-0.38); y = Binder cumulant U_P of plaquette
  percolation strength; three curves L=20/24/28 with autocorrelation-corrected
  error bars; shaded vertical band at Linsel h_c=0.335(2); annotate our pairwise
  crossings (0.3406-0.3437).
  Caption: "ParaToric reproduces the Linsel et al. (PRB 110, L241101) Ising-line
  benchmark: Binder crossings for L=20/24/28 at lmbda=0.3, beta=L."
No gallery, no scans — ONE figure that a peer can read in ten seconds.

### doc/benchmark_report.md — FILED (Feynman). Keep it current.
It records: the paper (Linsel arXiv:2401.08770 Fig.4a), the CONCRETE expected
numbers (h_c=0.335(2), nu=0.65(27), 3D Ising, lmbda=0.3, beta=L, mu=J=1), our
setup, the intermediate results -> the two fixes (swap-partner bug; ladder
resourcing) -> the current result, and the plot path.
YOUR ACTIONS on it: fill sec 6 PLOT PATH once the figure exists; update sec 5 when
round-trips land and the crossing moves. Honest status stays: PROVISIONAL — a
crossing from a NON-MIXING ladder (0 round-trips) is encouraging, not conclusive.
This file is the regression test for the whole observable+PT stack and the thing
we hand a peer to show we are doing physics, not chasing an artifact.

## UPDATE 2026-07-13e: benchmark MISMATCH is the priority (wrong L set); bridge idea parked; plaquette-hardcoding claim REFUTED

CREDIT: anchor plot committed; PM coordinate-drift caught by James, independently
confirmed by Martina, mitigated with --max-iterations 1. That is exactly the right
handling of a narrow technical defect — good.

### (A) HIGH PRIORITY — the benchmark MISMATCH. It is not yet a benchmark.
Our crossings 0.3406-0.3437 vs Linsel 0.335(2): 3-4 sigma outside their band.
Diagnosis:
 - NOT a convention error, most likely: ratio 0.3406/0.335 = 1.017 (no clean
   factor of 2 or sqrt2), AND the crossings drift MONOTONICALLY toward 0.335 as L
   grows: 0.34373 (20/24) -> 0.34155 (20/28) -> 0.34059 (24/28). Finite-size
   corrections converging in the RIGHT direction.
 - THE REAL MISMATCH: **we are not using Linsel's sizes.** They ran L = 10, 20, 30.
   We ran L = 20, 24, 28. Different L => different curves => the two figures CANNOT
   BE OVERLAID => it is not a plot-on-plot benchmark yet.
 - ALSO: a finite-L crossing is NOT h_c. Linsel's 0.335(2) is an EXTRAPOLATED
   value. Comparing our raw finite-L crossings to it is apples-to-oranges.
ACTION (in order):
 1. Get round-trips first (the 3 numbers from 13d) — 0 RT can bias each rung.
 2. **Re-run at L = 10, 20, 30 — Linsel's EXACT sizes.** L=10/30 are cheap.
 3. Overlay our U_P(h) curves with their Fig. 4a. The bar: a peer looks at both
    plots and says "same trend, same crossing" — no minus-sign archaeology.
Only then may benchmark_report.md sec 5 be promoted off PROVISIONAL.

### (B) PARKED (good idea, wrong moment): the (beta,Gamma)-style bridge
PI raised the Newman-Moore trick (use a 2nd coordinate to route around a barrier).
Verdict: principle transfers, implementation does not.
 - beta CANNOT be the bridge here: **beta = L is locked by the FSS design**, not a
   free knob.
 - The natural bridge coordinate is the DUALITY-ODD direction (h - lmbda): the
   first-order segment lives ONLY on the self-dual line; the flanking Ising lines
   are CONTINUOUS. A replica blocked at the MCP can detour off-diagonal, cross
   where it is 2nd-order, and return — routing AROUND the barrier.
 - It stays a **1D ladder** — a CURVED path in (h,lmbda), NOT a 2D grid. (This
   answers the earlier "decoupled PT is hard to tune" objection.)
 - **D4 (independent h,lmbda) is the enabler** — its value is higher than the
   "benchmark-only" framing I gave it.
STATUS: do NOT build now. Zero evidence of an MCP bottleneck (the current
bottleneck is at lmbda=0.3, a CONTINUOUS Ising line, where H5 cannot apply).
Contingency for if/when the self-dual ladder stalls at s*.

### (C) REFUTED — "the tuple/plaquette in the update is hardcoded"
Verified all four tuple updates in extended_toric_code_qmc.hpp:
  global_tuple_flip (2609), double_tuple_flip (2666), single_tuple_flip_move
  (2868), spin_tuple_combination (3185).
EVERY one samples the tuple RANDOMLY: get_random_plaquette_index() (z-basis) /
get_random_vertex() (x-basis). **No hardcoded plaquette in the update path.** The
concern appears to come from loophex's membrane update — different code, out of
scope. No audit needed.
ADJACENT, REAL, LOW PRIORITY (different from the claim): the
`staggered_imaginary_times` OBSERVABLES (lattice.cpp:3239/3262) sample a SINGLE
random plaquette/star per measurement — a 1-sample estimator, very noisy. Not a
bug; unused by this campaign. Park it.

## UPDATE 2026-07-13f: STOP — TREND INVERSION is the top priority. Likely WRONG CHANNEL. (supersedes 13e ordering)

PI caught this and it outranks everything numerical, including the 3-sigma
question and the round-trip work: **our U_P(h) DECREASES with h; Linsel's
INCREASES.** A trend inversion is a DEFINITION/OBSERVABLE problem, not a
statistics problem. Do this BEFORE any further mixing/round-trip work — a mixing
fix on the wrong observable buys nothing.

### Why the matching crossing is NOT reassurance
A crossing is INVARIANT under any MONOTONIC map: if U_L(h) cross at h*, then
f(U_L(h)) cross at the SAME h* for any monotonic f — INCLUDING a DECREASING f.
=> "crossings agree (0.34 vs 0.335) but the trend is inverted" is EXACTLY the
fingerprint of plotting a monotonically-DECREASING FUNCTION of Linsel's
observable. The number agreeing is expected even if the channel is wrong. Do not
take it as evidence.

### It is NOT a Binder-formula mismatch (verified)
Ours: `e4_raw/(e2_raw*e2_raw)` = <P^4>/<P^2>^2 (src/statistics/bootstrap.hpp:164,179).
Linsel Fig.4a y-axis: 10^0 -> 10^2 on a LOG scale = an unbounded ratio >= 1, i.e.
the SAME functional form. Same formula => same trend expected. The inversion is in
WHAT WE FEED IT, not how we compute it.

### LEADING HYPOTHESIS: we are plotting the DUAL/COMPLEMENTARY channel
The two z-basis channels percolate on OPPOSITE sides of the transition
(complementary). Complementary channel => P decreasing where theirs increases =>
U_P inverted => crossing still lands in the same place. That fits every observation.
FEYNMAN'S DEBT, stated plainly: I made the "plaquette = the deconfinement POP" call
on the CONTAMINATED, under-thermalized dataset, and flagged it myself as
provisional ("identify which channel is the electric POP from the POST-FIX
benchmark, not the current contaminated data" — UPDATE 13d). That step was never
done. This inversion is that shortcut coming due. Treat the plaquette
identification as UNVALIDATED.

### THE FREE DECISIVE CHECK (existing clean benchmark data; do FIRST)
Do NOT plot U_P yet. Plot **raw POP vs h for BOTH channels**
(percolation_probability AND plaquette_percolation_probability) from the clean
resourced-benchmark data, and compare to Linsel's stated behaviour:
DECONFINED (small h) => electric strings PERCOLATE. So the CORRECT channel must show
  - POP high (~1) at SMALL h, falling to ~0 at LARGE h; and consequently
  - U_P ~ 1 at small h -> LARGE at big h (INCREASING) — matching their Fig.4a.
Whichever of our two channels does that IS Linsel's electric channel.
If it is the OTHER one => the plaquette identification is WRONG and every
downstream statement keyed to it must be revisited (incl. "plaquette = the z-basis
deconfinement POP" in the run manual A8 and benchmark_report sec 3).

### ORDER OF WORK (revised)
 1. POP-vs-h, BOTH channels, existing data => fix the channel identification
    EMPIRICALLY. (free, one plot)
 2. Redo the anchor plot with the RIGHT channel AND Linsel's L = 10/20/30.
 3. Compare TREND first, crossing second. Only then may benchmark_report.md sec 5
    be promoted off PROVISIONAL.
 4. THEN resume the round-trip / 3-numbers work (13d).
BAR (unchanged, PI): a peer looks at our figure and Linsel Fig.4a and says "same
trend, same crossing" — no minus-sign archaeology. Until the trend matches, it is
NOT a benchmark.

## UPDATE 2026-07-17: 0 round-trips is likely a RUN-LENGTH problem, not a ladder-design problem. STOP redesigning.

PI insight (correct): healthy per-link acceptance does NOT give round-trips —
DIFFUSION NEEDS TIME. tau_RT ~ N^2/A (diffusive RW on N rungs). Round-trips need
run >= a few x tau_RT; below that, **0 round-trips is the EXPECTED output of a
HEALTHY ladder**. Credit: stopping after 3 attempts per loop-containment was right.

### The number that decides it (likely never checked)
L28: N=92, A~0.2 => tau_RT ~ 92^2/0.2 ~ 42,000 swap attempts ONE WAY ~ 85,000 per
round-trip ~ 250,000 for 3. => **total swap attempts in the run vs ~250k** is THE
diagnostic. If the run did fewer, the 3-attempt ladder chase diagnosed a non-problem.

### The trap the 3 attempts fell into: adding rungs makes it WORSE
tau_RT ~ N^2. Each "fix" GREW N (sec-8 optimum ~40/48/60 -> actual 62/74/92) and
over-refined h_c to 72-76% acceptance (far past the A~23% OPTIMUM that MINIMIZES
tau_RT). So the design fought itself: better acceptance, ~2.4x WORSE diffusion
(92^2/60^2) at fixed run length. Whack-a-mole on weak links spends rungs, and rungs
are QUADRATICALLY expensive. "13-20% acceptance but zero flow there" = too-short-
window artifact, NOT a localized block (13-20% accept => swaps ARE crossing; with
run < tau_RT NO region shows completed flow).

### FIX (opposite of a 4th redesign)
 1. Measure total swap attempts vs 3*tau_RT (tau_RT ~ N^2/A from measured accept).
    Short => LADDER IS FINE, RUN LONGER. Stop redesigning.
 2. Cap N back at the A~23% optimum (~40/48/60); ONE smooth density profile
    ~ local sqrt(Var E) across the WHOLE ladder (Jarvis path (a) but sized DOWN,
    not up). Revert the h_c over-correction — a few links at 20-23% is OPTIMAL.
 3. Verify submitted == intended (the 62-vs-66 / one-link-vs-shoulders gap is real;
    diff the design against the job before every submit).
 4. Run for > 3*tau_RT. At N~60, L=28 that is ~100-180k sweeps ~ a few hours. Cheap.
On Jarvis's paths: (a) is right IF sized down; (b) is a FALSE ALARM at these L —
tau_RT is a few hours at L=20-28, NOT prohibitive; it only bites at L=120 (the
separately-deferred pilot question). Do not conflate.

### FLAG THAT OUTRANKS ALL OF THIS: is the 13f CHANNEL CHECK done?
13f made the TREND-INVERSION channel check (POP-vs-h, BOTH channels) the top
priority BEFORE round-trip work — the observable may be the WRONG channel. These 3
ladder attempts look like round-trip work on a possibly-wrong observable. If the
both-channels POP-vs-h check has NOT run, it STILL COMES FIRST. A round-trip on the
wrong channel proves nothing. Binder crossings (0.3330/0.3376/0.3410) remain
doubly-untrustworthy: possibly-wrong channel AND 0 round-trips.

## UPDATE 2026-07-17b: 13f RESOLVED — it was the BINDER CONVENTION, not the wrong channel. Channel CONFIRMED. Green-light two parallel lanes.

Jarvis re-plotted the raw ratio <P^4>/<P^2>^2 (log axis) instead of the bounded
Binder form; trend now INCREASES with h, matching Linsel Fig.4a. Crossings
unchanged (monotonic transform). This is CORRECT on independent grounds, not
fitting-to-answer:
 - Linsel's y-axis is LOG 10^0-10^2. The Binder CUMULANT U = 1 - <P^4>/(3<P^2>^2)
   is <= 2/3 and can go negative => CANNOT be log-plotted 1..100. So their axis can
   ONLY be the unbounded RATIO <P^4>/<P^2>^2 — exactly what ParaToric stores
   (bootstrap.hpp:164). The earlier "decreasing U" was a spurious 1-ratio/3 in the
   plot script. Plotting the raw ratio is RIGHT.
 - CHANNEL ALSO CONFIRMED (favorably): ratio INCREASING with h means P is robust
   (percolating, ratio->1) at small h and intermittent (ratio->large) at large h
   = deconfined->confined. The COMPLEMENTARY (wrong) channel would give a
   DECREASING ratio. So the plaquette channel IS the deconfinement POP.
 => My 13f "we plotted the dual channel" hypothesis was WRONG; it was the Binder
    CONVENTION. Credit Jarvis. run_manual A8 correctly marked resolved.

CONFIRMATORY (free, non-blocking): plot raw POP vs h for the plaquette channel —
must be high(~1) at small h -> low at large h. Transform-free channel confirmation;
lets benchmark_report cite physics, not just a matching curve.

GREEN-LIGHT — TWO PARALLEL LANES (independent):
 LANE 1 (round-trips): per UPDATE 2026-07-17 — STOP redesigning the ladder; cap N
   at the A~23% optimum (~40/48/60, revert the h_c over-correction to 72-76%),
   ONE smooth density profile ~ sqrt(Var E); MEASURE total swap attempts vs
   3*tau_RT (tau_RT~N^2/A); if short => RUN LONGER (~few hrs at L28), do not patch
   links. Verify submitted==intended before every submit.
 LANE 2 (overlay benchmark): rerun at Linsel's EXACT sizes L = 10, 20, 30 (cheap;
   independent of Lane 1). Then overlay our raw-ratio U_P(h) with their Fig.4a —
   bar: peer sees SAME TREND + SAME CROSSING.
Promote benchmark_report.md sec 5 off PROVISIONAL only after BOTH: round-trips > 0
AND the L=10/20/30 overlay matches trend + crossing.

