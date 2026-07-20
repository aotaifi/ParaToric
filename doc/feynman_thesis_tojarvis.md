# Feynman -> Jarvis Handoff: Thesis T1 and M0 Authorization

Date: 2026-07-08. Context: Feynman physics-attack pass on
doc/multicritical_universality_plan.md, reading phase (no source edits, no
submissions), findings recorded in doc/run_tracker_multicritical.md
("Open questions for M1", A1-A3, R1-R5). This file is the ratified decision
record and the authorization to start M0.

## Thesis T1 (ratified)

The decidable claim for this campaign is **first-order vs continuous** at the
multicritical point on the exact self-dual line h = lmbda (mu = J = 1), not
"which universality class." Reason (R1): 3D XY (nu = 0.6717) and the SSN
self-dual class (nu = 0.669(4)) differ by ~0.4% in nu and are exponent-
degenerate at any L our budget reaches (SSN themselves could not separate
them at L<=96 with dedicated real-space methods; our reach is L<=192 with a
coarser method). Any M4 write-up must reflect this:
- Result = clean first-order proof -> PRL/PRX, decisive.
- Result = continuous -> PRB; report nu/x_S as "consistent with XY and with
  the SSN self-dual class"; explicitly state these are NOT separated by this
  campaign. Do not claim a class selection between scenarios 2 and 3.
- The emergent-O(2) test that would actually discriminate XY from the new
  class needs operator-multiplet structure not in our observable set; out of
  scope for this campaign.

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

## MASTER PRE-PILOT SEQUENCE (Feynman, 2026-07-10 — supersedes the earlier
## "Finalized next steps" list; integrates RESOLVE-FIRST + pt_architecture.md)

Do in this order. Nothing below is the MCP pilot itself; the pilot sbatch stays
PI-gated. Full detail in doc/pt_architecture.md (D1-D4, invariants H1-H6).

0. [BLOCKING] D1 — swap_period in SWEEPS, scaled with L² (default 1000 MOVES =
   ~0.035 sweeps at L=120 = label travel). Fix before ANY L>8 PT run. No PT run
   at production L is valid until this lands.
1. Validate the OBSERVABLE vs Linsel (no PT): reproduce h_c=0.335(2), nu=0.65
   (3D Ising) at lmbda=0.3, L=10-30, beta=L, in BOTH percolation channels to fix
   which is the electric-string POP. Decides the total-zero POP: bug vs placement.
2. Confirm the DECONFINED RESERVOIR on the self-dual line: at small s (e.g.
   0.05/0.10/0.20) confirm POP>0, POS>0, AND directly measure tau(percolation)
   at a small-s rung — verify the reservoir decorrelates the RIGHT (slow) mode,
   not just fast local modes. If it doesn't, PT has nothing to borrow.
3. LOCATE s* — wide-s scan BOTH directions, no pre-committed direction; cross-
   check Linsel F-S / BPV self-dual MCP.
4. D2 — build the offline PT diagnostics analyzer (strict RT, tau_ergodic in
   sweeps, up/down flow) over ladder_index_by_sample. PILOT PREREQUISITE.
5. D4 — generalize PT to independent (h, lmbda); run PT ACROSS Linsel's KNOWN
   3D Ising line (lmbda=0.3) at L=20-30 and confirm it recovers h_c=0.335,
   nu=0.65 WITH PT. Correctness-at-scale check (NOT acceleration).
6. D3 — PM/acceptance-mode grid (ported to s, robustness guards) to build the
   ladder bracketing s*.
7. THEN stage the MCP pilot (PI-gated): GO = mechanism (edge acceptance +
   strict RT round-trips) AND physics (per-replica sector change + tau(percolation)
   >= 5x drop). (a) without (b) = NO-GO.

## STANDING RULE: PT run diagnostics pipeline (Feynman, 2026-07-10)

From now on EVERY PT run analyzed with James/Jones MUST report this fixed
readout — decorrelation is everything, and these are the knobs we fine-tune:

  - swap_period IN SWEEPS (confirm >= ~1 sweep; flag if it is label-travel).
  - per-link (EDGE) swap acceptance for every link (target 20-40%, no dead link
    < 1e-4; a hole localizes a bottleneck rung / possible first-order point).
  - STRICT round-trips (replicas that genuinely visited BOTH ends —
    "visited_both_ranks", not just endpoint touches).
  - visited-LOW and visited-HIGH counts per replica (up/down flow f(rung);
    non-monotonic f => bottleneck).
  - tau_ergodic (round-trip time, in SWEEPS).
  - tau of the PHYSICAL observable (percolation_strength) at the s* rung — the
    decorrelation that actually sets Neff; the effective tau ~ the round-trip
    time, NOT the local tau. Budget Neff in tau_ergodic units, not raw samples.

A PT run report WITHOUT these is incomplete and must not be used to judge a gate.
Round-trips are necessary but NOT sufficient (H1-H3): a label can round-trip on a
frozen config. Always pair RT with genuine-decorrelation evidence.

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

## UPDATE 2026-07-18: DECOUPLE the benchmark from PT (Jarvis right); marker test to confirm the h_c bottleneck; beta-bridge = MCP plan of record (documented, deferred)

4th consecutive PT round-trip failure at the L=10-30 Ising benchmark, same cause
each time: a severe LOCALIZED acceptance hole at h_c (L30: 0.24% at h_c; harmonic
mean 6.3% vs median 21.5%). Jarvis's harmonic-mean insight is correct — diffusion
is limited by the WORST link (series resistors), so tau_RT ~ N^2/A_harmonic and a
median-based run-length margin is ~100x too small here. This refines my earlier
"just run longer": it is a REAL, physical, localized h_c hole (specific-heat
enhancement of Var E), not only under-resourcing.

### DECISION 1 (immediate): DECOUPLE the benchmark from PT — Jarvis is right
The benchmark does NOT need PT. Linsel did L=10/20/30 with LOCAL updates; our
"critically slow without PT" evidence was at L=32 (top of range); PT mechanism is
ALREADY validated (S0-S6 at N=4/8). We were making PT work for a check that does
not need it. => CLOSE the overlay benchmark with DEEP NO-PT cold/hot single chains
at L=10/20/30 (Linsel's own method). Removes the 4-attempt PT struggle from the
benchmark. benchmark_report.md stays PROVISIONAL until the NO-PT overlay matches
trend + crossing.

### DECISION 2 (do this, PI's idea): MARKER TEST — measure, do not guess
On ONE failed PT run, instrument the per-replica ladder trajectory: where does
flow collapse, where do replicas turn around. FALSIFIABLE prediction if the
diagnosis is right: ALL replicas pile up against the single h_c link (0.24%), none
cross, flow healthy on both sides. If flow instead dribbles out everywhere =>
distributed under-resourcing (run longer). Cheap (one analysis pass), decisive
localized-critical vs distributed, and it is the evidence base for the beta-bridge
decision.

### DECISION 3 (strategic): beta-bridge = PLAN OF RECORD for the MCP; DOCUMENT, do NOT build yet
Full spec: pt_architecture.md sec 9. Summary:
 - Add beta as a 2nd ladder coordinate; route AROUND the critical point via HIGH
   TEMPERATURE = LOW beta (transition smears out, mixing free), traverse h/s there,
   descend to beta=L (the only measured rung).
 - BEATS the duality-odd (h-lmbda) bridge: duality-odd escapes to the continuous
   Ising lines (STILL critical); beta-bridge escapes to NO transition => defeats
   critical slowing of ANY order. FEASIBLE: coupled-pinning tempers beta with
   worldline rescaling (reference exists).
 - COST: 2D ladder (N^2 diffusion), rescaling + beta-swap weight = new validated
   code. Converts an INFINITE 1D barrier into a FINITE diffusion time.
 - RANKING of MCP barrier tools: beta-bridge > worm (bespoke) > duality-odd.
 - BUILD IF AND ONLY IF the self-dual PT ladder STALLS at s* with a FIRST-ORDER
   signature (central-rung hole density cannot fix + bimodal energy). The
   benchmark (continuous, mild) does NOT justify it. Do not build ahead of that
   evidence.

ORDER: (1) close benchmark NO-PT; (2) marker test; (3) resume the self-dual PT
path toward the MCP — the beta-bridge is on the shelf for when/if s* stalls.

## UPDATE 2026-07-18b: L=10 no-PT flag is RIGHT — refines (not refutes) Decision 1. Depth-ramp is the DECISIVE, high-stakes test.

Jarvis L=10 no-PT pilot: clean off-critical; h_c window [0.319,0.344] shows
non-plateaued drift (ratio still falling at 4M, z=3.71), cold/hot split (z up to
29.6), tau_int ~7,375 sweeps at h_c (300-500x off-critical). Finding is real and
well-characterized. VERDICT: it does NOT break Decision 1's logic; it sharpens it.

KEY PHYSICS: the benchmark is CONTINUOUS (3D Ising, Linsel nu=0.65) => NO
thermodynamic barrier => cold/hot MUST converge and the ratio MUST plateau at
sufficient depth. So the z=29.6 split is NOT first-order metastability. It is
either (a) ordinary critical slowing (depth-fixable, slow) or (b) the WINDING-
SECTOR ergodicity problem: cold(+1)/hot(-1) start in different topological sectors
and local updates connect sectors only slowly.

MECHANISM (Jarvis reported it but under-weighted it): global_single_spin_flip —
the SECTOR-CHANGING move — is dead (<1e-4). Expected/known, BUT its consequence is
the point: with that move dead, the winding sector mixes ONLY by slow string
accumulation, critically slow at h_c => plausibly why cold/hot have not met at 4M.

WHY 100-1000x slower than Linsel (~1e4 sweeps): EXPECTED — our local worldline
update is autocorrelation-bound (the campaign's founding premise). "Linsel didn't
need PT => it'll just work" was too strong; our dynamics need far deeper runs.
Refines Decision 1's premise, does not refute it.

DECISIVE, CHEAP, HIGH-STAKES TEST (Jarvis's proposed step — endorsed, with stakes):
Run the h_c window at L=10 to 20-40M sweeps (~1 hr). Continuous => binary outcome:
 - PLATEAUS + cold/hot CONVERGES => depth-solvable, DECISION 1 HOLDS. Calibrate the
   required depth, extrapolate to L=20/30, scale with matched depth.
 - STILL DRIFTING / cold-hot SPLIT at 40M => the winding sector is NOT ergodically
   sampled by local updates even at L=10 (cheapest size). PROFOUND: PT ALONE would
   NOT fix it (PT shuffles configs, adds NO sector-changing power unless a rung
   mixes sectors) => the WORM becomes MANDATORY, not optional. Biggest campaign
   finding so far; would reorder the whole M0 strategy.
I do not know which way it goes — that is exactly why we run it, at the cheapest L.

ORDER (revised): (1) DEPTH-RAMP at L=10 h_c window = the decisive probe (completes
"close benchmark no-PT"); (2) marker test on a PT run stays valuable but is now
secondary to (1). Beta-bridge/worm ranking unchanged; the depth-ramp outcome
decides whether WORM jumps ahead of everything.

## UPDATE 2026-07-18c: FINALIZED GO — the L=10 depth-ramp, made unambiguous. This is the only decisive open thread.

Marker test (Decision 2) is IN: Hypothesis A confirmed — the h_c link on the L=30
PT ladder is a genuine localized near-wall (0.235% acceptance, only 4/67 replicas
ever cross, 99.9% immediately return; every other link 2-3 orders more permeable).
Credit Jarvis — clean, quantitative, exactly the falsifiable prediction resolved by
data. But note what it does and does NOT settle: it confirms the barrier is
localized at h_c on the CONTINUOUS Ising line WITH PT. It does NOT tell us whether
that barrier is depth-solvable by local updates alone. That is the depth-ramp's
job, and the depth-ramp is now the single thread that gates the whole M0 ordering
(depth-solvable vs WORM-mandatory). Finalizing it here so it runs once, cleanly,
and returns a binary.

### WHY no-PT is the right vehicle (not incidental)
NO-PT means NO PT+checkpoint composition (which throws — an M2 blocker), so a single
40M-sweep chain runs in ONE walltime block with no resume machinery. At L=10, 4M
sweeps ≈ 5-7 min => 40M ≈ 50-70 min/chain; 8 chains × a handful of h-points fits a
single modest array well under any cap. There is no budget argument against running
this properly. Run it properly.

### EXACT SPEC (submit as one Slurm array; do not redesign this)
Physics (unchanged, controlled vs the pilot job 15141215): L=10, beta=10, lmbda=0.3
fixed, mu=J=1, z-basis, NO PT (pure local Metropolis, zero swap moves).

h-points — three classes, all in ONE array:
 - DECISIVE (the failing window, must heal or not): h ∈ {0.318989, 0.322, 0.326,
   0.330615, 0.335363, 0.339574, 0.343521}. These are exactly the pilot's failing
   points; 0.335363 (drift z=3.71), 0.339574 (nonstationary p=0.024), and 0.318989
   (cold/hot z=29.6) are the three anchors — the others fill the window.
 - POSITIVE CONTROLS (must STAY converged — proves the deeper machinery still
   reports "PASS" where physics says it should): h ∈ {0.305629, 0.347399}, the two
   points flanking the window that already passed at 4M. If a control DRIFTS at 40M,
   the ramp itself is suspect and the whole readout is void — this is the guard
   against fooling ourselves.
Chains: 4 cold (default_spin=+1) + 4 hot (default_spin=-1) per h, distinct seeds
(the I5 cold/hot split is THE decisive diagnostic; keep all 8).
Depth: ONE continuous run to 40M sweeps per chain, with checkpoint readouts at
2M / 4M / 8M / 16M / 24M / 32M / 40M. The checkpoint LADDER matters as much as the
endpoint — we need the SHAPE of the approach, not just the final number.

### READOUT — three quantities per (h, checkpoint), via ensemble_diagnostics.py
 1. Cold/hot I5 z on the raw ratio <P^4>/<P^2>^2 (plaquette channel) AND on base
    -1-edge density. (Two observables: the ratio is the FSS quantity; the density
    is the slow winding-structure mode from the L32 finding.)
 2. Stationarity: constant-fit p and slope-z of the ratio over the checkpoint
    ladder, AND the INCREMENT sequence Δ(2→4M), Δ(4→8M), Δ(8→16M), Δ(16→24M),
    Δ(24→32M), Δ(32→40M). Decelerating (geometrically shrinking) increments =
    approaching a plateau; roughly-constant increments = linear/algebraic drift.
 3. tau_int and Neff at each point (context only; Neff on a non-stationary series
    is not trustworthy and does not by itself gate anything — the drift call does).

### DIRECT MECHANISM PROBE (the sharpening — do this, it is nearly free)
The 18b hypothesis is specific: cold(+1) and hot(-1) sit in DIFFERENT
winding/topological sectors and local updates connect them slowly. Test the
MECHANISM, not just its symptom: track the per-chain winding indicator
(percolation_probability / winding-string percolation, the non-contractible-string
sector proxy — winding string topologically MUST percolate, calibrated 2026-07-12)
across the 40M trajectory, and count SECTOR-EXCHANGE EVENTS (a chain crossing from
cold-basin to hot-basin value and vice versa).
 - If in-window points show ZERO sector exchanges over 40M while the positive
   controls exchange freely => that is the DIRECT smoking gun for winding-sector
   trapping, independent of the ratio drift => WORM-mandatory, and we do not need
   to argue from drift shape at all.
 - If chains DO exchange sectors but the ratio still converges only slowly =>
   ordinary critical slowing => depth-solvable.
This measures the thing 18b hypothesized instead of inferring it. That is the
whole point of the campaign's "measure, don't guess" discipline.

### DECISION RULE (binary; pre-committed so the outcome is not re-litigated)
Evaluate at 40M on the DECISIVE points (controls must have stayed converged):
 - PASS = depth-solvable, DECISION 1 HOLDS: cold/hot z<3 on BOTH observables at the
   worst points AND ratio increments clearly decelerating. => Read off the depth
   needed for L=10, extrapolate the depth law to L=20/30, close the no-PT overlay
   benchmark at matched depth. Promote benchmark_report only after the L=10/20/30
   overlay matches trend+crossing (unchanged bar).
 - FAIL = winding sector NOT ergodic under local updates: cold/hot still split
   (z>>3) with non-decelerating increments AND/OR zero in-window sector exchanges
   while controls exchange. => PT ALONE cannot fix it (it shuffles configs, adds no
   sector-changing power) => WORM becomes MANDATORY, reorders M0. Biggest finding of
   the campaign. Report immediately; do NOT attempt more depth or a patch.

### PRE-AUTHORIZED TIEBREAK (avoids one needless round-trip)
If at 40M the DECISIVE points are AMBIGUOUS — increments clearly decelerating (so
not the FAIL signature) but cold/hot z not yet <3 — Jarvis may extend the SAME
chains ONCE to 80M sweeps (still <2.5 hr at L=10) before reporting. That is the same
test at more depth, not a redesign. Any OTHER ambiguity, or a control that drifted,
STOP and report. This is the only extension authorized without a fresh read.

### LOOP CONTAINMENT
One ramp (plus at most the pre-authorized 80M tiebreak), then STOP and report the
binary. No ladder work, no PT, no new h-points beyond the spec above. The marker
test is done; the beta-bridge/worm ranking is unchanged and on the shelf. Nothing
else is in scope until the depth-ramp binary is in.

## UPDATE 2026-07-20: depth-ramp PASS accepted (my sector hypothesis is DEAD). L=20 is VOID, not merely ambiguous. L=30 is a NO on the current basis — and the circularity is MY spec's fault. Measure tau, do not extrapolate a guess.

### 1. L=10 depth-ramp: PASS accepted, and my 18b hypothesis (b) is REFUTED
Cold/hot z<3 on both observables at all 7 decisive points including h_c exactly,
controls held (p=0.97/0.84), and the direct probe returned hundreds of thousands
of sector exchanges everywhere. The FAIL signature I specified is absent.
So: winding-sector ergodicity breaking is DEAD as an explanation. The z=29.6 split
at 4M was ordinary critical slowing, resolved by depth. WORM IS NOT MANDATORY, and
Decision 1 stands. That is my third refuted hypothesis this campaign and I want it
recorded as such. The reason it died cleanly in one run is the direct mechanism
probe rather than symptom-watching. Credit Jarvis for the sector-exchange
instrumentation and for the false-timeout traceback (clean record, right call).
tau_int(h_c) ~10,792 sweeps vs the pilot's ~7,375: consistent, reassuring.

### 2. L=20: by my OWN rule this is VOID, not "ambiguous"
UPDATE 18c: "If a control DRIFTS, the ramp itself is suspect and the whole readout
is void." Control h=0.305629 fails (z=4.68 at 16M; no nontrivial suffix passes the
constant test; NONE through 64M). So the L=20 curve is not usable for the overlay
until that is cleared. Jarvis flagged exactly this condition himself; the rule
applies as written. But apply it proportionately: the diagnosis is legible
(under-depth plus a checkpoint ladder too coarse to resolve the relaxation shape),
so the fix is a targeted extension, not discarding 774 task-hours.

### 3. The circularity catch is CORRECT and it is MY error
Jarvis: 64M was chosen by assuming z=2, so recovering z=2 confirms nothing. That is
right, it is the most important sentence in the report, and the flaw was in MY 18c
NEXT line ("read off the depth at L=10, extrapolate the depth law to L=20/30").
Extrapolating a depth law from ONE size REQUIRES assuming the exponent. I built the
circularity into the spec. The worker's refusal to submit L=30 on that basis was
correct judgment and saved ~3,919 task-hours. The added point that a two-point
power law has ZERO residual degrees of freedom and cannot detect curvature is also
right, and it is why the fix below needs four or more sizes, not one more.

### 4. THE NUMBER NOBODY HAS CONNECTED: the sector-exchange rate already measures a timescale, and it is STEEPER than z=2
The exchange counts are per-sample at matched 25-sweep spacing, so the RATE is a
measured dynamical quantity, independent of the depth we chose. At h_c:
  L=10: 233,758 events / 40M sweeps = 1 per 171 sweeps
  L=20:  28,421 events / 64M sweeps = 1 per 2,252 sweeps
  slowdown x13.2 for a doubling of L  =>  apparent exponent ~3.7, NOT 2.
Treat this as a STRONG HINT, not a measured z: the count is threshold-crossings of
an indicator whose distribution itself changes with L, so it is not clean tau_int.
But it points the same way as the failed stationarity tests, and both say the z=2
prior is OPTIMISTIC. Consequence, since cost ~ L^(3+z):
  z=2.0 => L=20->30 is x7.6   ; z=3.7 => x15.3
So the proposed 144M at L=30 is plausibly UNDER-resourced by ~2x. Option (b) buys
a second AMBIGUOUS readout for ~3,919 task-hours. That is the worst available
outcome and I reject it.

### 5. VERDICT ON THE THREE OPTIONS
 (b) submit L=30 on the current guess: NO. Rejected on the arithmetic above.
 (c) drop L=30 on cost alone: premature, but the instinct is right and it is
     cheaper to answer than to assume (see step 2 below).
 (a) extend L=20: YES in part, but narrowed to the two bad points, and it is NOT
     the main event. Plateau-entry depth is a noisy, threshold-dependent quantity
     that costs a full deep run per size. That was the wrong calibration route and
     it was my prescription. The right observable is tau_int, which is measurable
     from runs of ~100 tau instead of ~1000 tau.

### 6. GO — three steps, in order. Roughly 100 task-hours total, versus 3,919.
STEP 1 (FREE, existing data, no new jobs). From the L=10 (15149119) and L=20
(15159599) HDF5 already on disk: (i) extract tau_int of the raw ratio AND the base
density at h_c and across the window at BOTH sizes, with honest error bars, and
report tau_int(20)/tau_int(10); (ii) report the sector-exchange rate scaling above
as a cross-check; (iii) produce the L=10/20 raw-ratio overlay against Linsel Fig.4a
WITH the crossing and bootstrap error bars. Item (iii) is the actual benchmark
deliverable and it is free.
STEP 2 (CHEAP, the real fix for z). Short runs at h_c ONLY, L = 12, 14, 16, 4 cold
+ 4 hot, long enough to RESOLVE tau_int (target ~300 tau total: thermalize ~100 tau,
measure ~200 tau). Estimated ~1-2 h/chain, order 20-60 task-hours all in. Then fit
tau_int(L) over L = 10,12,14,16,20: five points, real degrees of freedom, curvature
detectable, honest error on z. THIS is the campaign-deciding number, worth more
than L=30 (see step 7).
STEP 3 (targeted, clears the void). Extend ONLY h=0.305629 (control) and
h=0.339574 (decisive) at L=20, same chains via checkpoint/restart if available for
this no-PT configuration (G1.1), else rerun those two points from scratch, to 128M
with a GEOMETRIC ladder starting well below 64M (8/16/32/64/96/128M). 16 tasks,
~170-340 task-hours. Clears the control failure so the L=20 curve becomes usable.

### 7. ON L=30: HOLD, and understand what it is actually competing with
L=30 buys crossing DRIFT (a third pair). Trend is already confirmed at every L, and
L=10/20 are two of Linsel's three exact sizes, so the direct overlay is already
possible. Decide L=30 only AFTER steps 1-2, against measured z and against the
crossing you can already see. Two cost levers when we get there:
 - A third size at L=24 instead of L=30 costs ~(24/30)^(3+z) ~ 1/3 as much and
   gives the same crossing-drift information. We lose exact-size overlay aesthetics
   with Fig.4a; we do not lose the science.
 - If the step-1 crossing already lands in/near Linsel's 0.335(2) with the right
   trend and honest bars, the benchmark may be closeable at two sizes. Linsel's own
   nu carried 40% error at these very sizes; the bar is "same trend, same crossing,"
   never "nail h_c."
WHY z OUTRANKS L=30: cost ~ L^(3+z), so L=20 -> L=120 is x7,776 at z=2 and
x169,503 at z=3.7. Either way the no-PT local path CANNOT reach the MCP at L=120 —
which is the campaign's founding premise, now quantified. Measured z converts that
premise into the concrete speedup any algorithm (PT, beta-bridge, worm) must
deliver, and it sets the M2 budget. That is a campaign-level number. A third
benchmark crossing is not.

### 8. STANDING RULE (new, from the L=20 miss)
Checkpoint ladders must be GEOMETRIC and the FIRST checkpoint must sit well BELOW
the expected convergence depth (~1/8 or less). At L=20 the ladder opened at 8M,
already converged at most points, so there was no relaxation SHAPE left to fit.
A checkpoint ladder that starts after convergence measures nothing. Add to the run
manual.

### LOOP CONTAINMENT
Steps 1-3 then STOP and report: measured z with error bars, the L=10/20 crossing
with honest bars, and whether the L=20 control cleared. No L=30, no L=24, no PT
work until that is in.

## UPDATE 2026-07-20b: CORRECTION to my own step-2 rationale, and the FINAL ordering. This supersedes the step order in 2026-07-20. GO.

### CORRECTION (mine, prompted by the PI): benchmark-line z is NOT the M2 budget
In 2026-07-20 I wrote that measured z "sets the M2 budget." That is too strong and
I am correcting it. Every run in that update (L=10 pilot, L=10 depth-ramp, L=20
scale-up, proposed L=30) is at **lmbda=0.3 FIXED** = the Linsel ISING line, which
is OFF the self-dual line and is a CONTINUOUS 3D Ising transition. A z fitted there
is the cost law for the CONTINUOUS case only.
If the MCP is FIRST-ORDER (half of thesis T1, still undecided), the slow mode there
is NOT power-law critical slowing but EXPONENTIAL tunneling between coexisting
phases, tau ~ exp(2*sigma*L^2). No z fitted on a continuous line extrapolates to
that. => The Ising-line z is a BEST-CASE LOWER BOUND on MCP cost. Necessary, not
sufficient. Do not quote it as the MCP budget. This is also exactly the scenario
where beta-bridge/worm becomes mandatory, so yesterday's PASS does NOT retire that
ranking.

### FOR THE RECORD: we have never run at the MCP
The numbers are deceptively close and the trap is live. Benchmark critical point =
(h,lmbda) = (0.335, 0.30). MCP = (s*,s*) with s* ~ 0.33-0.35 (Linsel PRIOR, still
UNMEASURED by us). Nearly the same h, different lmbda. Also: L=10-30 is
benchmark-scale; the MCP FSS design targets L ~ 120-192 at beta=L. The only
post-observable-fix self-dual work is the L=8 duality checks at s=0.22; the one
attempt to locate s* (job 15034719, L=120) returned the null result and predates
the fixes. Locating s* remains an unexecuted step of the master sequence.

### FINAL ORDER (supersedes 2026-07-20 sec 6). Jarvis has the PI's GO.
 STEP 1 — UNCHANGED, FREE, do first (no new jobs). From existing L=10 (15149119)
   and L=20 (15159599) HDF5: tau_int of the raw ratio AND base density at h_c and
   across the window at BOTH sizes with error bars; the sector-exchange rate
   scaling cross-check (x13.2, apparent exponent ~3.7); and the L=10/20 raw-ratio
   overlay vs Linsel Fig.4a WITH crossing and bootstrap bars. Item (iii) is the
   benchmark deliverable and it costs nothing.
 STEP 2 — PROMOTED: LOCATE s* on the SELF-DUAL line (master sequence step 3).
   Wide-s scan, h=lmbda=s, mu=J=1, SMALL L (8-16), both directions, NO
   pre-committed direction, applying the depth lessons we just paid for (cold/hot
   I5 + stationarity, geometric checkpoint ladder starting well below expected
   convergence). This is now the highest-value new compute: it is the first step
   toward the actual target and it is the prerequisite for measuring anything about
   the transition we care about.
   EARLY FIRST-ORDER TELL (cheap, take it while you are there): record the ENERGY
   HISTOGRAM at each s. A BIMODAL/double-peak distribution is strong evidence of a
   first-order MCP and is visible at modest L without the full FSS ladder.
   ASYMMETRY, state it in the report: bimodality PRESENT => strong first-order
   evidence; bimodality ABSENT => INCONCLUSIVE, not evidence of continuity (the
   double peak only develops once L exceeds the interface scale). Do not report
   absence as a continuous result.
 STEP 3 — UNCHANGED: clear the L=20 void. Extend ONLY h=0.305629 (control) and
   h=0.339574 at L=20 to 128M on a geometric ladder (8/16/32/64/96/128M), same
   chains via checkpoint/restart if available for no-PT (G1.1), else rerun those
   two points. ~16 tasks, ~170-340 task-hours. Needed for the L=20 curve to be
   usable in the overlay.
 STEP 4 — DEMOTED, OPTIONAL: the Ising-line z-fit at L=12/14/16. Run it only if
   the continuous-case cost law is wanted for its own sake, and label it
   explicitly as a best-case bound per the correction above. It is no longer a
   prerequisite for anything.
 L=30: still HOLD. L=24 remains the ~1/3-cost alternative if a third size is later
 judged necessary. Decide only after step 1 shows what the two-size crossing
 actually looks like.

### LOOP CONTAINMENT
Steps 1-3 (step 4 optional), then STOP and report: the L=10/20 crossing with honest
bars, whether the L=20 control cleared, and the s* scan with energy histograms and
the bimodality read stated under the asymmetry rule above. No L=30, no L=24, no PT
ladder work until that is in.

## UPDATE 2026-07-20c: NEW STEP 2B — MEASURE PT's SPEEDUP on the benchmark line. PI catch; I had left this assumed. Runs in PARALLEL with the s* scan.

### THE GAP (mine): we have never measured whether PT actually helps
After Decision 1 I parked PT and wrote "resume the self-dual PT path toward the
MCP." That quietly ASSUMED PT delivers a useful speedup, having never measured one.
Same failure pattern that has cost this campaign weeks: assume instead of measure.
State it precisely: S0-S6 validated the PT MECHANISM (swaps are correct). It says
NOTHING about whether PT reduces autocorrelation. Separable claims, exactly like
swap-WEIGHT vs swap-PARTNER. Every PT run this campaign either failed on transport
(4 attempts, 0 round-trips) or was an L=8 correctness smoke test. **PT's speedup
factor is unmeasured.** We are currently planning to deploy it at the MCP on faith.

### WHY IT MUST BE DONE NOW, ON THE BENCHMARK LINE
Speedup = tau_noPT / tau_PT requires a TRUSTWORTHY no-PT baseline. We have exactly
one, just paid for: L=10, tau_int(h_c) ~10,792 sweeps, Neff 1848, controls held
(job 15149119); L=20 follows once step 3 clears the void. At the MCP we will NEVER
have a no-PT reference, because no-PT there is precisely what is infeasible.
=> This measurement is POSSIBLE NOW and IMPOSSIBLE LATER. That decides the timing.
No conflict with Decision 1: that decoupled the benchmark's ANSWER from PT; this
uses the benchmark as a CALIBRATION FIXTURE for PT. Different purpose. Do not
re-litigate Decision 1 on the basis of this step.

### WHY THIS IS NOT A 5TH LADDER ATTEMPT
 - ROUND-TRIPS ARE NOT REQUIRED and are NOT a gate here. The 4 failures chased
   round-trips to CLOSE the benchmark. A speedup measurement needs only tau_int per
   rung. A rung that never exchanges simply returns tau_PT = tau_noPT (speedup 1).
   The measurement is well-defined even in the regime that previously "failed".
 - The h_c hole is now CHARACTERIZED, not mysterious (marker test: 0.235%
   acceptance at one link, every other link 2-3 orders more permeable).
 - We now know the required DEPTH from the no-PT ramp, so the run can be sized
   instead of guessed.

### THE METRIC — cost-normalized, or we will flatter PT
PT costs N times the cores. Raw tau reduction is NOT the figure of merit. Report:
 (i) tau_int and Neff PER RUNG at the PT rung h-values, vs the no-PT tau_int at the
     SAME (h, L) from job 15149119. Speedup(h) = tau_noPT(h)/tau_PT(h).
 (ii) THE DECIDING NUMBER: for EQUAL TOTAL CORE-HOURS, does PT with N rungs give
     more Neff at the h-points of interest than N INDEPENDENT no-PT chains at those
     same N h-values? The no-PT side is already measured, so this is apples-to-
     apples. Quote Neff per core-hour for both.
 (iii) Speedup AT h_c specifically, called out separately from the ladder average.
     The average will be flattered by the easy rungs; h_c is where we need it.
 (iv) PHYSICS AGREEMENT (a real G1.3, at a non-trivial point unlike the L=8
     original): does PT reproduce the same raw-ratio curve and crossing as the
     no-PT run and as Linsel? If PT shifts the crossing, we must know before
     betting the MCP on it.
 (v) Round-trips/transport as CONTEXT only. Zero round-trips does not void this.

### SPEC (small, cheap, one walltime block)
L=10, beta=10, lmbda=0.3, mu=J=1, z-basis, PT ON. Reuse the Lane-2 L=10 14-rung
ladder spanning the h-window (do NOT redesign it; rung placement is not what is
being tested). Depth: thermalize to the no-PT-calibrated plateau entry (~16M
sweeps) then produce >= 200 tau at the slowest rung (~2-3M sweeps at h_c scale);
budget ~20M sweeps total. NOTE PT+checkpoint THROWS, so it must fit ONE walltime
block; at L=10 that is ~0.5-1 h/rank, order 20-30 task-hours all in. Then repeat at
L=20 AFTER step 3 clears that baseline: two sizes give the speedup TREND, which is
what extrapolates toward the MCP.

### DECISION RULE (pre-committed)
 - Cost-normalized speedup at h_c >> 1  => PT is worth deploying at the MCP;
   proceed with the self-dual ladder, and fold the measured factor into the M2
   budget (replacing the current assumption).
 - Cost-normalized speedup at h_c ~ 1   => PT does NOT help where the barrier is,
   so it will not rescue the MCP either (where the barrier is likely worse).
   beta-bridge/worm move UP the ranking, and we do NOT spend MCP-scale compute on a
   plain PT ladder. This is the outcome that would save the most.
 - Intermediate => quote the honest factor; it is the number the M2 budget has been
   missing all along.
Either way this is decision-grade, which is why it outranks the optional step 4.

### ORDERING
Step 1 (free) first, as before. Then 2A (locate s*) and 2B (this) run in PARALLEL —
independent lines, independent jobs, both small-L, both cheap; precedent is the
earlier Lane 1 / Lane 2 split. Step 3 unchanged, and it unlocks 2B at L=20. Step 4
stays optional and is now clearly the lowest value item.

## UPDATE 2026-07-20d: FRESH-EYES REVIEW of 20b/20c. Steps 2A and 2B RATIFIED with additions. But I found a MEASUREMENT-INSTRUMENT DEFECT that changes step 1, cancels step 3 pending a free re-analysis, and makes step 4 invalid as specified. New STEP 0, free, blocks the rest.

New Feynman session. The PI held dispatch so this directive could be reviewed with
fresh eyes rather than inherited. I reviewed it against the existing L=10
(`15149119`) and L=20 (`15159599`) HDF5 on disk. Most of 20b/20c survives. One
thing does not, and it is upstream of almost every number this campaign quotes.

### 0. THE FINDING: our tau estimator silently underestimates, and it decides step 3
`integrated_autocorrelation_time` in `scripts/ensemble_diagnostics.py` (and the
matching stored HDF5 `autocorrelation_time`, which I checked reproduces it to ~1.0x)
is an initial-positive-sequence sum. IPS truncates at the first non-positive
autocorrelation. Our observable has ~95-98% of its variance in a component that
decorrelates within ONE sample, sitting on top of a small-amplitude, very long-lived
tail. Measured rho(lag), lag in samples of 25 sweeps, plaquette_percolation_strength:

  L=10 h=0.326  : r(1)=+0.051 r(100)=+0.022 r(1e4)=+0.019 r(5e4)=+0.012
  L=20 h=0.326  : r(1)=+0.018 r(100)=+0.001 r(1e4)=+0.000 r(5e4)=+0.001
  L=20 h=0.33957: r(1)=+0.016 r(100)=+0.005 r(1e4)=+0.002 r(5e4)=-0.000

IPS integrates the fast component and truncates before the tail. The blocking
curve (tau vs block length, median over the 8 chains) shows what that costs at the
two L=20 points that were declared failures:

  h=0.305629 : 44 / 46 / 45 / 42 / 44 / 39 sweeps at block 16k->8M   = FLAT PLATEAU
  h=0.339574 : 102 / 288 / 881 / 2289 / 3094 / 3300 sweeps           = STILL CLIMBING

So the two "failures" have DIFFERENT causes and neither is what we assumed:
 - h=0.305629 (the CONTROL that voided the run): the blocking curve is flat across
   three decades. tau = ~44 sweeps, genuinely. The statistics are SOUND, the error
   bars are CORRECT, and its full-ladder constant-fit p=0.0528 is ABOVE 0.05. The
   z=4.68 that voided the readout is ONE checkpoint out of five (8M/32M/48M/64M give
   z = 0.28/1.13/0.47/0.46) among ~90 comparisons in the table, and the slope-z is
   1.57, i.e. NOT SIGNIFICANT. There is no drift here. I do not believe this control
   ever failed.
 - h=0.339574 (decisive, and the real tau peak): IPS returns 827 sweeps, blocking
   returns >=3300 and has not plateaued at a block length of 8M sweeps. IPS is low
   by AT LEAST 4x. Its error bars are correspondingly too small and its
   constant-fit p=0.026 is untrustworthy IN BOTH DIRECTIONS. Note the sampling is
   not the problem: Neff = 64M/(2*3300) ~ 9,700, versus a bar of 30.

CONSEQUENCE: the L=20 void is a DATA-ANALYSIS defect, not a DATA-COLLECTION defect.
Step 3 proposes 170-340 task-hours to fix a data-collection problem I do not think
we have. And I verified job `15159599`'s sbatch sets no `checkpoint_path`, so there
is no checkpoint to restart from: step 3 is a full rerun, not an extension, and its
cost is the TOP of the quoted range.

### 1. WHY THIS ALSO BREAKS STEP 1(i) AND STEP 4
tau_int is a variance-weighted quantity: roughly (slow-mode amplitude) x (slow-mode
decay time). Across L=10 -> L=20 the AMPLITUDE falls hard (self-averaging: 4x more
plaquettes, so the global mode is a smaller share of the observable's variance)
while the decay TIME rises. Their product is not a relaxation time and need not be
monotonic in L. Measured with IPS at h=0.335363, tau falls 10,541 -> 35 sweeps from
L=10 to L=20. Read literally that says the dynamics get 300x FASTER with L, which
violates G2 and is impossible.
 - STEP 1(i) as written ("report tau_int(20)/tau_int(10)") would report a ratio far
   below 1 and hand us a false headline.
 - STEP 4 (fit z to tau_int(L) at L=12/14/16) is therefore not merely optional, it
   is INVALID AS SPECIFIED. It would fit the product of a falling amplitude and a
   rising time and return a NEGATIVE z.
This is my own error class again, one level deeper than I had it: I told you to
prefer a directly measured quantity (tau_int) over a choice-contaminated one
(plateau-entry depth). Correct instinct, wrong quantity. tau_int is contaminated
too, by the observable's variance decomposition, and that decomposition depends on L.
The quantity that governs THERMALIZATION is the slow-mode decay time; tau_int
governs ERROR BARS. They are different and I conflated them.

### 2. A SECOND, FREE, POSITIVE RESULT ALREADY IN THE L=20 DATA
Scanning tau across the window, L=20 shows a clean single peak at h=0.339574
(827 sweeps by IPS, >=3300 by blocking) with shoulders at 0.343521 and 0.335363 and
a flat floor of ~20 sweeps everywhere else. That is a textbook pseudo-critical
signal, and it is an INDEPENDENT, DYNAMICAL estimate of the finite-L critical point
from a quantity unrelated to the Binder ratio. It agrees with the PT-era crossings
(0.3408-0.3441) and sits ABOVE Linsel's extrapolated 0.335(2), which is exactly what
finite-size scaling requires. This strengthens the benchmark and costs nothing.
It also means h=0.335362847648 is NOT our finite-L critical point. We have been
quoting "tau at h_c" and the sector-exchange rate at a point that is OFF-PEAK at
both sizes. The x13.2 sector-exchange ratio must be recomputed at each size's OWN
pseudo-critical h before it is used for anything.
L=10 has no comparable peak: its tau rises monotonically toward SMALL h (88,826
sweeps at h=0.318989) with enormous chain-to-chain scatter (min 1,475, max 202,508).
That is the frozen-winding slow mode of the deconfined side (run manual E3), not
critical slowing, and it is another reason the L=20 run is the more informative of
the two, contrary to our treating L=20 as the void one.

### 3. REVISED ORDER. Step 0 is new, free, and blocks the rest.
 STEP 0 — NEW, FREE, NO NEW JOBS. Fix the estimator, then re-analyze what we have.
   (a) Add a blocking-plateau tau estimator to `scripts/ensemble_diagnostics.py`:
       scan block length over decades, require a plateau, and where no plateau is
       reached report a LOWER BOUND and flag the point tau-UNRESOLVED. Keep IPS as
       a cross-check, never as the sole source. Feed the plateau tau into the
       block-bootstrap block size.
   (b) Re-run the depth-ramp readout for BOTH `15149119` and `15159599` with it.
       Re-issue cold/hot z, stationarity p, and Neff at every (L,h).
   (c) PRE-COMMITTED PREDICTION, so this is falsifiable and I do not get to
       reinterpret it afterwards: control h=0.305629 will PASS (flat plateau, p
       already 0.053), and h=0.339574's error bars will grow >=2x with its
       stationarity p rising above 0.05. IF BOTH HOLD, the L=20 void is CLEARED FOR
       FREE and step 3 is CANCELLED. If either fails, step 3 is back on, and then it
       is justified by evidence rather than by assumption.
   (d) Every tau, Neff, and bootstrap error bar this campaign has quoted came from
       the IPS estimator, including "tau_int(h_c) ~10,792 sweeps, Neff 1848" at L=10.
       Treat them all as suspect IN THE DIRECTION OF OVERCONFIDENCE until re-issued.
       Do not re-engineer the C++ stored tau; fix the analyzer and stop trusting the
       stored value.

 STEP 1 — KEPT, FREE, but re-specified. Same three items, with: tau from the
   blocking plateau, not IPS; slow-mode amplitude and decay time reported
   SEPARATELY rather than as their product; each size's own pseudo-critical h taken
   from its tau peak, and the sector-exchange rate ratio recomputed there; the
   overlay's error bars derived from the corrected tau. Add the sec-2 tau-peak
   result to `doc/benchmark_report.md`.

 STEP 2A — LOCATE s*. RATIFIED as written. Three additions:
   (i) Run BOTH BASES and apply the I2/B5 duality check at EVERY scanned s, not only
       at s*. On the self-dual line duality must hold everywhere along it, so this is
       the sharpest available check that we are actually on the line and that the
       instrument is sound. At L=8-16 it is nearly free. The directive omitted it.
   (ii) Energy histograms: report POOLED cold+hot AND per-chain separately. With
       local updates at small L a first-order point does NOT show as a bimodal
       single-chain histogram; it shows as cold and hot chains sitting at two
       distinct, stable, depth-persistent energies. Report the cold/hot energy gap
       versus depth explicitly.
   (iii) THEREFORE: a persistent cold/hot gap at s* must NOT be auto-diagnosed as
       under-thermalization and fed more depth. Run manual E2 says cold/hot may
       NEVER converge at a first-order point. Distinguish by whether the two groups
       approach each other with depth (under-thermalized) or hold two stable
       separated values (coexistence). Getting this backwards burns MCP-scale
       compute chasing a convergence that cannot happen.
   The asymmetry rule on bimodality is correct as written; keep it verbatim.
   Expectation to set now: 2A's deliverable is s* with a bracket. At L=8-16 the
   order will most likely read INCONCLUSIVE, and that is a success, not a failure.

 STEP 2B — MEASURE PT's SPEEDUP. RATIFIED. This is the best item in 20c and the PI
   was right to force it. Three additions:
   (i) PRE-CHECK BEFORE SUBMITTING, cheap and fatal if wrong: verify whether the PT
       HDF5 observable series is indexed by MPI RANK or by LADDER POSITION. The
       measurement needs tau at FIXED h. If the series is per-rank while ladder
       labels migrate, "tau at the h_c rung" is not what is stored and the whole run
       is void. This is the same rank-versus-ladder-position bookkeeping that
       already produced one real bug in this code. Check it first.
   (ii) Use the STEP 0 estimator for both sides. A speedup built from two IPS
       numbers is a ratio of two biased quantities, and the bias is not guaranteed
       to cancel: it depends on the slow-mode amplitude, which PT changes.
   (iii) Tighten the decision-rule wording. Cost-normalized speedup ~1 licenses
       "a PLAIN SINGLE-PARAMETER h-LADDER does not help", NOT "PT does not help".
       The beta-bridge is a ladder in a DIFFERENT direction and is not refuted by
       this measurement. The 20c text mostly says this; the tracker card overstates
       it, and I am fixing the card.

 STEP 3 — DOWNGRADED from authorized to CONDITIONAL on step 0(c). Do not spend
   170-340 task-hours until the free re-analysis says the void is real. If it is
   real, note it is a full rerun (no checkpoint exists for `15159599`), so quote the
   top of the range.

 STEP 4 — INVALID AS SPECIFIED, not merely optional. Do not fit z to tau_int(L).
   If the continuous-case cost law is wanted, re-specify it on blocking-converged
   tau at each size's OWN pseudo-critical h, or on the sector-exchange rate. Either
   way it stays a best-case lower bound on MCP cost per my 20b correction.

 L=30 and L=24: unchanged, still HOLD.

### 4. WHAT I AM RATIFYING WITHOUT CHANGE
The 20b correction (benchmark-line z is a best-case bound, not the M2 budget), the
scope fact that we have never run at the MCP, the promotion of locate-s* above the
z-fit, the entire rationale of 2B, the bimodality asymmetry rule, and the loop
containment. Those are right and I am not relitigating them.

### LOOP CONTAINMENT
Step 0 first and report its outcome before anything else moves; it is free and it
decides step 3. Then step 1 (free), then 2A and 2B in parallel. Then STOP and
report: the re-issued L=10/20 validity table with honest tau, whether the void
cleared, the L=10/20 crossing with corrected bars, the s* bracket with the energy
histograms and the cold/hot gap read, and the cost-normalized PT speedup at h_c.
No L=30, no L=24, no step 3 unless step 0 says so.

## UPDATE 2026-07-20e: REFINEMENT of 20d for the actual execution order. Step 1 is in flight and lands FIRST; Step 0 becomes the immediate next action. Triage rule for Step 1's output, and a fully executable Step 0 spec.

Context change since 20d: Jarvis had already been dispatched and is finalising
STEP 1. The PI is stopping him after it. So the order 20d assumed (0 then 1) is
inverted in practice. That is fine and costs nothing, provided Step 1's output is
triaged correctly on arrival. This update supplies that triage and makes Step 0
executable end to end so it can be run without another round with me.

### 1. TRIAGE RULE for the in-flight Step 1 output. One line to remember:
**THE MEANS ARE FINE. EVERY ERROR BAR IS SUSPECT.**
The IPS defect (20d sec 0) mis-estimates the autocorrelation time. It does NOT bias
the sample means. So on arrival, split Step 1's deliverables:

 KEEP AND USE:
  - The raw-ratio curve SHAPES and the pooled MEANS at every (L,h), both sizes.
  - The crossing LOCATION from the L=10/20 overlay.
  - The sector-exchange RATE and its recomputation at each size's own tau peak.
    A counting rate needs no tau and is untouched by the defect.
  - The identification of each size's pseudo-critical h from where tau PEAKS. The
    peak LOCATION is robust even though the peak HEIGHT is underestimated, because
    IPS is biased low roughly uniformly across the window.

 QUARANTINE — do not quote, do not enter in the tracker, do not put in a figure:
  - Every error bar, bootstrap or otherwise, at both sizes.
  - Every Neff.
  - Every tau value, and above all any tau_int(20)/tau_int(10) RATIO. If Step 1
    reports that ratio it will be below 1 and will read as "the dynamics get faster
    with L". That is the artifact of 20d sec 1, not a result. It must not enter the
    record even as a provisional number.
  - The crossing's quoted UNCERTAINTY, and therefore any statement about whether we
    agree with Linsel's 0.335(2). Trend and location yes; significance no.
  - Any promotion of `doc/benchmark_report.md` sec 5 out of PROVISIONAL. The bar has
    always been "same trend, same crossing"; we can speak to both now, but not to
    consistency within errors until Step 0 re-issues the bars.

 REPORT IT ANYWAY, clearly labelled. A quarantined number is not a deleted number.
 Step 1's figure is still the benchmark deliverable; it just carries provisional
 bars with a one-line caveat until Step 0 lands.

### 2. STEP 0 — full spec. Free, no new jobs, immediate next action.
Purpose: replace the truncating tau estimator, then re-issue the validity tables for
`15149119` (L=10) and `15159599` (L=20). Three legs; (b) is the one I most want.

 (a) BLOCKING-PLATEAU tau, added to `scripts/ensemble_diagnostics.py` beside the
     existing IPS function (do not delete IPS; demote it to a cross-check).
     For block length b on a geometric ladder b = 1,2,4,...,b_max in SAMPLES:
       tau(b) = 0.5 * b * Var(block means) / Var(series)      [sample units]
       n_b    = N / b ;   relative error on tau(b) ~ sqrt(2/(n_b - 1))
     Cap b_max at the largest b with n_b >= 16 — below ~16 blocks the block-mean
     variance is too noisy to read a plateau from, and reading one there is how you
     talk yourself into a number.
     PLATEAU := tau(b) agrees within its own error across >= 3 consecutive
     doublings. Report the plateau value.
     NO PLATEAU by b_max := report tau(b_max) as a LOWER BOUND and flag the point
     **TAU-UNRESOLVED**. A tau-unresolved point is not a failed point; it is a point
     whose error bar we cannot yet honestly quote, and it must be labelled as such
     rather than silently assigned the last value on the ladder.
     Per chain, then report median across the 8 chains plus the min/max spread.
     I4 already requires per-chain tau within a factor 2; flag violations.
     Feed 2 * tau_plateau into the block-bootstrap block size.

 (b) BETWEEN-CHAIN ERROR — a tau-FREE cross-check, and the cleanest one available.
     We have 8 independent chains per point. The error on the pooled mean is then
     simply std(chain means)/sqrt(8), with NO autocorrelation model at all: each
     chain is one independent draw, so a slow mode with a decay time comparable to
     the run length is captured correctly by construction. It carries ~27% relative
     error at 8 chains, which is plenty to expose a factor-4 discrepancy.
     **Report the between-chain error side by side with the bootstrap error at every
     (L,h), and report their RATIO.** That ratio IS the direct measurement of the
     estimator defect, it requires no tau, and it settles the question without
     depending on the very machinery under audit. If the two agree, my 20d diagnosis
     is wrong at that point and I want to know immediately. This is the cheap direct
     probe for this problem and it should be run first within Step 0.
     Caveat to state in the report: between-chain error is blind to a drift COMMON
     to all chains (G1b). It cross-checks the ERROR BAR, it does not replace the
     stationarity test.

 (c) RE-ISSUE, for both jobs, at every (L,h): tau (plateau value or lower bound +
     flag), Neff, bootstrap error, between-chain error and their ratio, cold/hot z,
     and the suffix stationarity p. Same table shape as
     `DEPTH_LAW_AND_DESIGN.md` so the two are directly comparable.

### 3. PRE-COMMITTED PREDICTIONS. Unchanged from 20d, restated so they bind.
 - L=20 control h=0.305629 PASSES: flat blocking plateau ~44 sweeps, bootstrap and
   between-chain errors AGREE, full-ladder p already 0.0528 > 0.05.
 - L=20 h=0.339574: bars grow >= 2x, tau resolves to >= 3300 sweeps or comes back
   TAU-UNRESOLVED, stationarity p rises above 0.05.
 IF BOTH HOLD: the L=20 void is CLEARED FOR FREE, step 3 is CANCELLED, and the L=20
 curve becomes usable in the overlay with honest bars.
 IF EITHER FAILS: step 3 is back on, justified by evidence rather than assumption,
 and quoted as a full rerun (no `checkpoint_path` in `15159599`).
 IF THE (b) RATIOS COME BACK ~1 EVERYWHERE: I am wrong about the defect. Say so
 plainly, keep the existing bars, and the void stands as originally diagnosed. I
 would rather be corrected here for free than after step 3 has been paid for.

### 4. ORDERING FROM HERE
 Step 1 lands (in flight) -> triage per sec 1 -> STEP 0 (free) -> report.
 Then, and only then, 2A and 2B in parallel per 20d. Step 3 stays conditional on
 sec 3. Step 4 stays invalid as specified. L=30 and L=24 stay HOLD.

### LOOP CONTAINMENT
Stop after Step 0 and report: the re-issued validity table, the bootstrap-versus-
between-chain error ratios, and a one-line verdict on whether the L=20 void cleared.
Do not start 2A or 2B in the same breath — the void answer changes what the L=20
baseline is worth to 2B.

## UPDATE 2026-07-20f: I WAS WRONG ABOUT THE ERROR BARS. The tau-free cross-check refutes my own 20d claim. The L=20 VOID IS REAL, step 3 is BACK ON. What survives: tau_int is still the wrong instrument for z, and Step 1's apparent z = -7.65 must still be quarantined.

Step 1 landed while 20e was being written, so I ran the 20e leg-(b) cross-check
myself on the existing HDF5 rather than wait. It refutes me. Recording that first,
before anything else, per the pre-committed rule in 20e sec 3.

### 1. THE REFUTATION — clean, and by the probe I specified
Between-chain error on the pooled raw ratio, std(chain means)/sqrt(8), needs NO
autocorrelation model at all: 8 independent chains, 8 independent draws. Against the
quoted bootstrap error:

  L=10 h=0.322      1.1x        L=20 h=0.322      0.8x
  L=10 h=0.335363   1.1x        L=20 h=0.326      1.2x
                                L=20 h=0.335363   1.0x
                                L=20 h=0.343521   0.8x
  AND AT THE TWO POINTS I ACTUALLY DISPUTED:
    L=20 h=0.305629   between-chain 0.000306 vs bootstrap 0.000398 = 0.77x
    L=20 h=0.339574   between-chain 0.002737 vs bootstrap 0.002768 = 0.99x

Ratios ~1 everywhere, including both disputed points. **The bootstrap error bars are
CORRECT.** And the per-chain ratios at h=0.305629 are 1.3999 1.4005 1.3997 1.4003
1.4001 1.4010 1.3995 1.3981 — eight independent chains agreeing to four decimals.
That is not a point with a hidden slow mode. That is a beautifully converged point.

### 2. WHAT THIS KILLS — my 20d sec 0, and the consequences I drew from it
 - **The L=20 VOID IS REAL** and stands exactly as the retired session diagnosed it.
   My "it is an analysis artifact" claim is dead. Step 3 goes back to AUTHORIZED,
   not conditional. The retired session was right and I was wrong to dispute it.
 - The pre-committed prediction in 20d/20e sec 3 FAILED, and it failed in the
   direction that costs money: step 3's 170-340 task-hours are needed after all.
 - Where I went wrong, precisely: I read a rising blocking curve at L=20 h=0.339574
   (up to 3300 sweeps) as an unresolved slow mode. At those block lengths there were
   only 8-16 blocks, so that rise was noise. **My own 20e spec contains the warning
   that would have caught it** — "below ~16 blocks the block-mean variance is too
   noisy to read a plateau from, and reading one there is how you talk yourself into
   a number." I wrote that caution and then did the thing it warns against, one
   update earlier. The lesson is not new; the lesson is that I applied it to Jarvis's
   future work and not to my own already-drawn conclusion.
 - Standing correction to my sec-0 blanket claim: prior tau/Neff/error bars are NOT
   globally suspect. Withdraw that. The error bars were fine all along.

### 3. WHAT SURVIVES, and it is the half that matters for the physics
The two claims in 20d were separable and only one is dead:
 (A) tau_int is the WRONG INSTRUMENT for a dynamic exponent or a thermalization
     cost law.  **SURVIVES, and is now SHARPER.** tau_int is the variance-weighted
     integral. That is exactly the right quantity for an ERROR BAR, which is why the
     cross-check passes. It is the wrong quantity for a RELAXATION TIME, because it
     is (slow-mode amplitude) x (decay time) and the amplitude falls with L through
     self-averaging. Both facts are now confirmed simultaneously: the errors are
     right AND tau_int falls with L. No contradiction. tau_int does its own job
     correctly and cannot do the other job at all.
 (B) the error bars are underestimated, so the void is an artifact.  **REFUTED.**

### 4. STEP 1's OUTPUT — the quarantine still applies, for reason (A)
Step 1 reports `hc_raw_ratio_tau20_over_tau10 = 0.00498` and
**`hc_raw_ratio_apparent_z = -7.65 +/- 0.52`**. A dynamic exponent of MINUS SEVEN is
not a measurement of anything. It is precisely the artifact predicted in 20d sec 1,
and it must NOT enter the tracker, the benchmark report, or any figure, even
provisionally. Same for `hc_base_density_apparent_z = +0.195`: the base density has
tau ~19-21 sweeps at BOTH sizes, i.e. it sits at the 25-sweep sampling floor
everywhere and carries no slow mode, so its "z" is measuring the sampling interval,
not the dynamics. Neither number is evidence. Step 4 remains INVALID AS SPECIFIED.
Everything else in Step 1 is now UNQUARANTINED: with the error bars vindicated, the
means, the curve shapes, the bars themselves, and Neff are all usable.

### 5. THE CROSSING NEEDS A SECOND LOOK, AND THIS IS NOW THE OPEN QUESTION
Step 1 reports crossing h = 0.32441 +/- 0.00082. That sits BELOW Linsel's
extrapolated 0.335(2) and far below the PT-era crossings 0.3408-0.3441, with an
error bar tighter than the PT-era spread of 0.003. Finite-L crossings should sit
ABOVE the extrapolated value, and every previous run of ours did. Before this is
reported as a benchmark result, note what went into it: **both EXCLUDED_VOID points
are L=20, and one of them is h=0.339574** — which is exactly where I measured the
L=20 tau peak, i.e. the pseudo-critical point. The L=20 curve therefore jumps
1.4832 at h=0.335363 straight to 1.7027 at h=0.343521 with the steepest interval
removed. Dropping the steep segment of one of two curves will move a crossing.
 => Do not promote this crossing. Re-derive it after step 3 restores h=0.339574,
 and report the crossing WITH and WITHOUT the excluded points so the sensitivity is
 visible rather than buried. If the crossing moves back toward 0.34 when the point
 is restored, the exclusion was the cause and the 0.32441 was never real.

### 6. REVISED ORDER
 STEP 3 — **AUTHORIZED** (was conditional). Extend h=0.305629 and h=0.339574 at
   L=20 to 128M, geometric ladder 8/16/32/64/96/128M. Full rerun: job `15159599`
   set no `checkpoint_path`, so quote the TOP of the 170-340 task-hour range. This
   is now the unblocker for both the crossing (sec 5) and the L=20 PT baseline.
 STEP 0 — **CANCELLED as an audit.** Its premise is refuted; there is no error-bar
   defect to fix. Keep ONE piece of it as cheap permanent hygiene, not as a fix:
   add the between-chain error, std(chain means)/sqrt(N_chains), as a routine column
   beside the bootstrap error in `scripts/ensemble_diagnostics.py`. It is nearly
   free, it needs no tau, and it would have settled this in one command instead of
   two updates. Do NOT rebuild the tau estimator; IPS is fine for its actual job.
 STEP 4 — still INVALID as specified (reason (A), unaffected by the refutation).
 STEPS 2A / 2B — unchanged from 20d, including all the additions there. 2B's L=20
   arm still waits on step 3.
 L=30, L=24 — still HOLD.

### 7. FOR THE RECORD
That is the fourth hypothesis of mine this campaign to be refuted by a cheap direct
probe, and the first one I have managed to refute myself before it cost a run. The
probe cost one command. The two updates of reasoning that preceded it did not.
Credit where it is due: the retired session's L=20 void call was correct, and
Jarvis's original EXCLUDED_VOID handling of those two points was the right call on
the data as it stood.

### LOOP CONTAINMENT
Step 3 is the next compute. Report the crossing with and without the excluded
points, per sec 5. 2A and 2B proceed in parallel per 20d.
