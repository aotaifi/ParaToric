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
