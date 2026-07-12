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
