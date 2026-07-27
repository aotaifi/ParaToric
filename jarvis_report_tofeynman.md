# Jarvis → Feynman report channel (ParaToric)

Jarvis appends here; Feynman reads, never writes. Entries tagged
`thesis:<id> | plan:<plan_id> | type:PROPOSED PLAN | RESULT | REVISION-RESPONSE`.

---

NEWEST ENTRY IS AT THE BOTTOM. Pre-2026-07-18 sections moved to
`doc/archive/jarvis_report_tofeynman_archive_pre_20260718.md` on 2026-07-23
to fix a chronological-ordering defect Feynman caught (UPDATE 2026-07-20p
sec 6) -- a fresh Feynman session bootstrapping on "read the last section"
would otherwise have found superseded 2026-07-19/07-20 material sitting
below newer 2026-07-21 entries and mistaken it for current.

---

## RESULT — L=10 NO-PT pilot: NOT VALIDATED. Genuine h_c-localized critical slowing found even at the smallest, cheapest L — this may threaten Decision 1's premise (2026-07-18)

Your GO on decoupling the benchmark from PT is executed: submitted a deep
NO-PT (pure local Metropolis, zero swap moves) pilot at L=10 per Linsel's
own method -- 14 h-values in [0.28,0.40] (denser near h_c=0.335), 4
cold + 4 hot chains each, checkpoints at 200k/500k/1M/2M/4M sweeps,
2M-sweep production window. Job **15141215** (Slurm array, 112 tasks),
all 112/112 COMPLETED cleanly. Full Part B validity checklist run via
ensemble_diagnostics.py.

**VERDICT: NOT VALIDATED. Do not scale to L=20/30 yet.**

The failures are not scattered noise -- they cluster tightly in
h=[0.318989, 0.343521], bracketing h_c=0.335 exactly, while every h
outside that window (0.28-0.305629, 0.347399-0.40) passes cleanly (cold/hot
z<3, plateaued ratio, Neff in the tens of thousands). Inside the window,
three distinct symptoms appear, worst right at h_c=0.335363 itself:
- **Non-plateaued drift** (G1b): at h=0.335363 the raw ratio <P^4>/<P^2>^2
  is STILL monotonically decreasing across the 200k->4M checkpoint ladder
  (1.665->1.582->1.568->1.555->1.446, slope z=3.71) -- 4M sweeps is not
  enough depth even at L=10. h=0.339574 shows the same nonstationarity
  (constant-fit p=0.024).
- **Cold/hot disagreement** (I5/B3a): z as high as 29.6 at h=0.318989 and
  5.7-7.3 at several neighbors -- chains are in different basins even at
  4M sweeps depth, not just slowly-decorrelating within one basin.
- **tau_int blowup** (B4): tau_int reaches 295 (in 25-sweep sample units,
  so ~7,375 sweeps) right at h_c and h=0.330615, vs ~1 sample-unit
  off-critical -- roughly 300-500x worse. Neff drops to 135-172 at the
  worst points, nominally still >=30 but built on a non-stationary series
  (the drift above), so not trustworthy as reported.
One global, UNRELATED tripwire also fired (global_single_spin_flip
acceptance <1e-4 in all 112/112 chains) -- this is NOT new: Martina's
2026-07-11 code trace (run_tracker_multicritical.md:264) already
established this move is an ordinary single-edge global flip expected to
be near-never-accepted; not investigating further, not a real finding.

**Why this matters for the decision we just made:** the strategic bet in
Decision 1 was that L=10-20 (unlike the L=32 evidence) would be small
enough for local-update thermalization to work in reasonable depth,
because Linsel's own paper didn't need PT there. This pilot shows a real,
qualitatively PT-shaped failure mode (basin disagreement + non-plateaued
drift, tightly localized at h_c) already at L=10, the cheapest and most
forgiving size in the set. It does NOT necessarily falsify the decouple
decision -- L=10 is cheap enough that "just go deeper" may still work
fine (I have not yet tried beyond 4M sweeps at the h_c cluster, and 4M
sweeps only took ~5-7 minutes wall-clock here, so a 10-20x extension is
trivial computationally) -- but it means "Linsel didn't use PT so this
should just work" is not automatically true even at their smallest size,
and I want your read before assuming a depth extension will resolve it
cleanly rather than reveal the same h_c bottleneck in a different guise.

**Candidate next step (not deciding myself):** extend depth specifically
for h in [0.318989, 0.343521] at L=10 (e.g. ramp to 20M-40M sweeps,
cheap at this L), re-run Part B, and see whether it actually plateaus
and cold/hot converges, or whether it's still drifting -- that result
would be decisive evidence either way before committing to the same
strategy at L=20/30 (where the same critical-slowing window will be much
more expensive to push through by brute depth alone).

Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_pilot_20260718/
(job 15141215).

## RESULT — Decision 2 (marker test): Hypothesis A CONFIRMED, with a quantitative refinement — h_c is a genuine localized barrier, not distributed under-resourcing (2026-07-18)

Ran the marker test on the L=30 Lane-2 PT run (job 15140246, 67-rung ladder,
min acceptance 0.24% at h=[0.339,0.344]) per your Decision 2. Reconstructed
per-replica ladder-position trajectories, turnaround histogram, and
per-link crossing counts (scripts/pt_diagnostics.py --production-motion).

**VERDICT: Hypothesis A (localized barrier at h_c) is supported; Hypothesis
B (distributed under-resourcing) is rejected** -- but the literal "zero
crossings, all replicas pile up" prediction was too strong and is
corrected by the data:
- h_c link: 588 accepted / 249,412 rejected swap attempts (acceptance
  0.235% +/- 0.010%), 1,176 replica-crossing events total, only 4/67
  replicas ever crossed it at all.
- Of the replicas that DO cross, they almost always bounce straight back:
  conditional return fraction 99.92% from below, 99.83% from above -- so
  crossing is possible but the barrier is functionally a near-wall, not
  distributed friction.
- EVERY other (non-h_c) link had >=11,400 crossing events, median
  ~107,683 -- 2-3 orders of magnitude more permeable than h_c specifically.
- Turnarounds cluster hard at the two rungs bracketing h_c (rung 36,
  h=0.339250: 54,571 turns; rung 37, h=0.343500: 6,955 turns) versus
  broad, low-count turnaround activity elsewhere (ordinary random-walk
  reversals, not a second blockage).
This is decisive: the ladder elsewhere mixes fine (crossings in the tens
of thousands to ~100k per link); the h_c link specifically is ~2-3 orders
of magnitude more resistant than everywhere else, and what little leakage
occurs through it almost immediately reverses. Not a whack-a-mole
artifact, not spread-out slow diffusion -- a real, physical, localized
barrier right at h_c, consistent with a specific-heat-like Var(E)
enhancement.

**On the beta-bridge (Decision 3):** per your own decision rule
(pt_architecture.md sec 9.4: build IFF the self-dual PT stalls at s*
with a first-order signature), this benchmark result alone does NOT
authorize building it -- this is the continuous, mild Ising-line
benchmark, not the self-dual MCP. Treating this as confirmatory evidence
that the localized-barrier diagnosis method works and is trustworthy,
not as a green light to build. Will re-apply the same marker test at s*
if/when the self-dual PT stalls there, per your ORDER item (3).

Full data used: /scratch/a/A.Otaifi/ (job 15140246 output directory,
per Lane 2's job history in status.md); marker-test summary at
/tmp/job15140246_marker_summary.tsv (worker's scratch output, not yet
moved into the project's tracked output tree).

## RESULT — the L=10 depth-ramp binary: PASS. Decision 1 HOLDS. No worm needed. (2026-07-18)

Job 15149119 (72 tasks, the EXACT SPEC from your UPDATE 2026-07-18c) ran
to full completion cleanly -- all 72/72 Slurm tasks COMPLETED (exit 0:0,
54min-1h48m elapsed vs a 3h cap), and I independently verified via the
HDF5 output (not just Slurm bookkeeping) that each chain's recorded
series has exactly 1,592,000 samples, matching a full 40M-sweep run --
not truncated. (Aside, unrelated: I initially got a false "they all timed
out" report from the PI, traced it to unrelated TIMEOUT jobs in a
different project, code_ruby_duy_pinning_lab, confirmed via WorkDir in
sacct -- not this run. Flagging only so the record is clean; no action
needed on your end.)

**VERDICT: PASS. Decision 1 holds -- depth-solvable ordinary critical
slowing, NOT winding-sector ergodicity breaking. The worm algorithm does
NOT become mandatory. No 80M tiebreak needed.**

Controls first (voiding check): both h=0.305629247317 and h=0.347398758564
stayed converged and stationary through 40M (z40 <=1.03 on both
observables, constant-fit p=0.97 and 0.84 respectively) -- readout is
valid, not void.

Decisive points, all 7, at the 40M checkpoint:
- Cold/hot z-scores (ratio, density): h=0.318989: (0.017, 0.311);
  h=0.322: (0.035, 0.238); h=0.326: (0.354, 0.924); h=0.330615: (0.243,
  0.667); h=0.335363 (h_c itself): (0.041, 0.767); h=0.339574: (0.080,
  0.102); h=0.343521: (1.517, 1.474). ALL comfortably below the z<3 bar --
  cold and hot are statistically indistinguishable at every decisive
  point by 40M, including exactly at h_c.
- Stationarity: increments shrink into the late checkpoints at every
  point (e.g. h=0.335363's last three increments: +0.0158, -0.0738,
  +0.1525 in early noise band vs the initial larger -0.073 -- broadly
  plateauing; h=0.318989 is the shakiest, constant-fit p=0.054, borderline,
  but its LATE-window (16-40M) slope-z drops to 0.609 from an overall
  3.24, i.e. the drift is concentrated in the earlier, not final, depth --
  consistent with approaching a plateau, not runaway drift).
- Sector-exchange events: every single chain, decisive AND control alike,
  shows heavy exchange activity -- hundreds of thousands of events over
  the 40M-sweep trajectory (e.g. at h_c itself: cold chains
  [235084,237732,243965,226170], hot chains [217013,226760,236170,247168]
  exchanges). This is the direct kill of the "sector-trapping" mechanism
  hypothesis from your 18b update: the FAIL signature (zero in-window
  exchanges while controls exchange freely) is simply absent. Chains ARE
  crossing winding sectors freely at every h-value tested, including h_c.

**Reading**: the z=29.6 cold/hot split I found in the original 4M-sweep
pilot was ordinary (if severe) critical slowing, not sector trapping --
resolved by depth alone, exactly as your "refines, does not refute"
read predicted. tau_int at h_c (10,792 sweeps median) is close to the
pilot's earlier tau_int estimate (~7,375 sweeps from the 4M run) --
consistent, not coincidentally different, reassuring that both runs are
measuring the same physical autocorrelation time, just the depth-ramp
finally has enough production window (1.59M samples vs 80k) to resolve
it (Neff 1848 vs 136 at h_c).

NEXT (per your own spec's NEXT line, not yet executed -- want your/PI's
confirm before committing the larger spend): calibrate the required
depth from this L=10 16-40M plateau shape, extrapolate a depth law to
L=20/30 (more expensive, and L=20 previously showed comparable or worse
critical slowing under PT), and submit the no-PT L=20/30 overlay at
matched depth to finally close the benchmark per Decision 1's original
plan. Loop containment note: this was "the single decisive open thread"
gating everything else per your ORDER -- it is now resolved, so Step 2/3
(self-dual-line reservoir check, locate s*) are also unblocked again if
you'd rather sequence those before the more expensive L=20/30 scale-up.

Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/
(job 15149119).

## RESULT — L=20 scale-up: AMBIGUOUS readout, L=30 correctly NOT submitted, real cost decision needed (2026-07-19)

Per the PI's go-ahead to scale to L=20/30, I calibrated a depth law from the
L=10 depth-ramp (plateau entry ~16M sweeps) and submitted L=20 NO-PT
(job 15159599, 72 tasks: 9 h-points x 4cold+4hot, 64M sweeps/chain,
checkpoints 8/16/32/48/64M). Completed fully -- verified 72/72 COMPLETED
and HDF5 series length exactly 2,552,000/chain (matches a full 64M-sweep
run, not truncated).

**VERDICT: AMBIGUOUS, not a clean PASS.** At the final 64M checkpoint,
cold/hot z<3 everywhere and every chain shows healthy sector-exchange
activity -- no catastrophic sector-trapping signature, same reassurance
as L=10. BUT the calibration itself is untrustworthy:
- Most h-points already looked "converged" (z<3 and stationary) at the
  VERY FIRST checkpoint (8M) -- too coarse a checkpoint ladder to see a
  real relaxation shape at those points, so they give no genuine
  plateau-entry depth to calibrate against.
- TWO points never achieve a STABLE joint (z<3 AND stationary) state
  through the full 64M: positive control h=0.305629247317, and decisive
  point h=0.339573841268. Both drift in and out of the z<3 threshold
  across checkpoints rather than settling -- one of these is a control,
  which is exactly the condition that should void a clean readout.
- The naive z-fit from L=10(16M)->L=20(64M) reproduces z=2 exactly, but
  this is CIRCULAR: 64M was CHOSEN by extrapolating the z=2 prior in the
  first place. It confirms nothing independent about the true exponent.

**L=30 was NOT submitted.** The worker was explicitly pre-authorized to
submit it in this dispatch once a fitted exponent was in hand, and
correctly declined -- the calibration behind it is unreliable, so
committing to L=30 on it would be extrapolating a guess through another
guess. I'm treating that refusal as correct judgment, not a failure to
execute.

**The real number driving this decision**: L=30 at the currently-implied
depth would cost median 52.9h / mean 54.4h / worst 69.9h PER CHAIN,
~3,919 total task-hours across 72 tasks -- a genuinely large resource
commitment (cluster-days, not cluster-hours) that should not rest on an
unverified exponent from a circular fit.

**Options, not deciding myself:**
(a) Extend L=20's SAME chains further past 64M (targeting specifically
    the 2 non-converged points) to get a real, non-circular plateau-entry
    depth for L=20, then fit z honestly from two genuine data points, then
    design L=30. Slower to get to L=30, but the L=30 spend would be
    justified by real data instead of a guess propagated twice.
(b) Accept more risk: submit L=30 anyway at the current best-guess depth
    (informed by the z=2 prior, since nothing has actively falsified it --
    the ambiguity is about CONFIDENCE in the calibration, not evidence
    against z=2 specifically), accepting that a wrong guess could waste
    a meaningful fraction of the ~3,919 task-hours if the depth turns out
    insufficient or badly oversized.
(c) Reconsider whether L=30 is worth this cost at all for what it adds to
    the benchmark -- L=10 and L=20 alone may already be enough to show
    trend + rough crossing convergence toward Linsel's h_c=0.335(2); L=30
    was Linsel's largest size and presumably the most informative for the
    crossing, but that's a judgment call about diminishing returns vs.
    cost that I'd rather have your/the PI's read on than assume.

Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/
(job 15159599).

## RESULT — STEP 0: estimator fixed and reviewed (4 rounds), your two falsifiable predictions tested against real L=10/L=20 data. VERDICT: STILL REAL per your own decision rule, with a nuance worth reading before accepting step 3 unqualified. (2026-07-20)

Your fresh-eyes catch was right and the diagnosis matched what I'd already
flagged as suspect: IPS was truncating on the fast component and biasing
tau low. Built the blocking-plateau + between-chain-error estimator per
your STEP 0 spec, through the project's Clark->(Martina+Grace) review
gate. This needed 4 rounds, not 3 -- rounds 1-2 found real defects
(wrong std convention, a plateau criterion literally gameable by
error-bar widening at large b with a concrete counterexample from Grace,
an aggregation bug that would have hidden unresolved chains behind a
median, a NaN-producing stationarity calculation, a bootstrap ratio
computed against the wrong quantity, raw h5py.File instead of the
existing reader). Round 3 was a split verdict (Martina REJECT on one
item, Grace APPROVE) -- PI authorized a 4th round with Clark's reasoning
raised and a model change; round 4 added the missing constant-fit-p gate
explicitly and Martina approved. Code: scripts/ensemble_diagnostics.py,
new `blocking-validity` subcommand.

**Ran it on the real data and tested your two pre-committed predictions:**

PREDICTION 1 (control h=0.305629247317 passes with tau~44 sweeps, errors
agree, p~0.0528): **FAIL as stated on the specific numbers** -- tau
resolved cleanly at 21.6 sweeps (not ~44), between/bootstrap ratio 0.70
(not "near 1"). BUT the point is otherwise clearly healthy: cold/hot
z=0.33, suffix p=0.33, Neff~1.48M. The control is fine; your specific
numeric guesses just weren't quite right.

PREDICTION 2 (h=0.339574: bars grow >=2x, tau>=3300 or TAU-UNRESOLVED, p
above 0.05): **FAIL on the error-growth clause, PASS on the other two.**
Tau came back TAU-UNRESOLVED (lower bound >=2157 sweeps -- close to your
3300 estimate, same qualitative outcome) and suffix p=0.20 (above 0.05,
as predicted). But the between-chain error did NOT grow >=2x -- it came
out essentially the SAME as the old error (ratio 0.935), not larger.

**Per your own decision rule this reads STILL REAL, step 3 stays
justified** -- across all 18 (L,h) points the between/bootstrap ratio
spans 0.49-1.02, never showing the predicted systematic inflation, so
the specific "error bars were secretly 2x+ too small" mechanism you
proposed is not what's happening.

**One caveat I want you to see before treating that as the final word:**
the driver's raw printed ratio for the MEAN-strength observable is
MATHEMATICALLY DEGENERATE -- a delete-one jackknife SE of a simple mean
is an exact algebraic identity of the ddof=1 sample SE, so comparing it
to a ddof=0 version returns sqrt(7/8)=0.935 for ANY data, always, by
construction, not because anything converged or didn't. This makes the
tool's default output uninformative for a linear observable. I had the
worker separately compute the RAW-RATIO-specific numbers (the nonlinear
<P^4>/<P^2>^2 quantity your predictions actually concern, whose bootstrap
error genuinely depends on tau via block sizing, unlike the mean's) --
those are the numbers quoted above and in the saved table. This is not a
new code defect (no source was changed for this run), just a scope note:
the driver needs the caller to specifically request the raw-ratio
observable rather than defaulting to strength/mean, or a future user will
get the same mechanically-fixed 0.935 and mistake it for a real
diagnostic.

**A second nuance**: h=0.339574's z (0.42) and suffix p (0.20) both look
fine -- it's specifically TAU ITSELF that remains unresolved at 64M
sweeps, not obviously the mean or crossing value. If what step 3 is
meant to buy is "a trustworthy tau/error-bar at this point" rather than
"a trustworthy mean," that's a narrower (and probably cheaper) thing to
fix than a full 128M rerun -- worth a look before committing to the full
170-340 task-hour range.

**Secondary tau-peak check (your sec-2 claim)**: NOT cleanly established
either way. L=20's largest unresolved lower bound IS at h=0.339574
(>=2157 sweeps) with 0.335363 next (>=1537) -- consistent with your
claimed peak location -- but 0.326 and 0.330615 are ALSO unresolved
(>=954, >=792), so a clean "isolated peak over a flat floor" can't be
confirmed; those neighbors could be part of the same broad slow region.
L=10 DOES show the claimed monotonic tau rise toward small h (lower
bounds climbing from >=1683 to >=32790 sweeps, h=0.347 down to h=0.306),
consistent with the frozen-winding-mode read, but every L=10 point is
tau-unresolved too, so this is suggestive, not resolved.

Full validity table: doc/figures/linsel_blocking_validity_l10_l20_20260720.md.
STOPPING HERE per loop containment -- no 2A, no 2B, no step 3 submission
yet. Awaiting your read on the nuances above before treating "STILL REAL"
as license to spend the full step-3 budget as originally scoped.

## RESULT — STEP 1 (free tau_int + L=10/20 overlay) done; benchmark crossing in real tension with Linsel; a suspected tau_int ARTIFACT flagged before trusting the z~3.7 cross-check. Stopping here per PI instruction (2026-07-20)

Added your STANDING RULE as G6 in paratoric_run_manual.md (geometric
checkpoint ladders, first checkpoint well below expected convergence
depth) before starting STEP 1, per your instruction.

STEP 1 executed: extracted tau_int (bootstrap/jackknife errors) of the
raw ratio and base sigma_z density from the existing L=10 (15149119) and
L=20 (15159599) HDF5, across the full h-window, with the two L=20 VOID
points (h=0.305629, h=0.339574) excluded from the crossing calculation
and clearly marked EXCLUDED_VOID in the output table (not silently
treated as trustworthy). Produced the actual benchmark overlay plot:
doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_20260720.png,
updated benchmark_report.md sec 4/5/6 accordingly, status kept
PROVISIONAL (not promoted -- correctly, given what follows).

**Two findings, reported with different confidence levels:**

1. **Crossing tension (solid finding)**: the L=10/L=20 pairwise crossing
   is h=0.3244098 +/- 0.0000817, vs Linsel's published h_c=0.335(2). That's
   roughly (0.335-0.3244)/sqrt(0.002^2+0.0000817^2) ~ 5 sigma away --
   a real, not noise-level, tension using two of Linsel's own exact
   sizes. This does NOT close the benchmark and I'm not spinning it as
   close; L=10's raw-ratio curve is also nonmonotonic near the formal
   crossing, per the worker's own caveat, so the crossing estimate
   itself may not yet be fully stable.

2. **tau_int(L=20)/tau_int(L=10) at h_c = 0.00498 +/- 0.00179 (i.e. L=20's
   measured tau is ~200x SMALLER than L=10's at h_c)** -- I want to flag
   this as SUSPECT, not report it as a confirmed cross-check of your
   sector-exchange-rate z~3.7 estimate, for a concrete reason: the
   suppression is NOT localized to h_c, it's systematic across almost
   the ENTIRE L=20 window. Every L=20 h-point except right at/near h_c
   sits at tau~18-27 sweeps, nearly flat regardless of h, while L=10
   spans 846-71,086 sweeps across the same window and peaks near h_c as
   expected. A real system should show tau GROWING with L and peaking at
   criticality -- L=20 coming out almost h-INDEPENDENT and uniformly tiny
   looks like a measurement/estimator artifact (possibly interacting with
   the same G6 checkpoint-ladder coarseness that produced the VOID
   points), not real physics. I checked the obvious explanation (a
   between_sweeps/sample-period mismatch between the two runs) and ruled
   it out -- both use between_sweeps=25 -- but have not gone further than
   that given your "stop after step 1" instruction just landed. Treat the
   density tau ratio (1.145 +/- 0.013, a much more modest and
   physically plausible ~1x-ish change) as the more trustworthy of the
   two, and treat the ratio-observable tau_int comparison as UNVERIFIED
   until someone checks whether the estimator or the L=20 series itself
   is behaving correctly.

**Net read**: the sector-exchange-rate hint (apparent z~3.7) and this
tau_int measurement do NOT actually cross-check cleanly right now --
they'd need to, for the same reason, and the tau_int side looks
suspect on physical grounds. I'd treat "z is probably steeper than 2"
as still just Your STEP-1-motivating hint, not doubly confirmed, until
the tau_int anomaly is either explained or the STEP 2 L=12/14/16 runs
give a clean answer directly.

STOPPING HERE per direct instruction ("stop after step 1") -- STEP 2
(L=12/14/16 short h_c-only runs) and STEP 3 (targeted L=20 void-clearing
extension) are designed but NOT dispatched. Awaiting your/PI's read,
including whether the tau_int anomaly should be diagnosed before STEP 2
proceeds (a bad estimator would also corrupt STEP 2's tau_int(L) fit).

Full data: /scratch/a/A.Otaifi/paratoric_linsel_no_pt_l10_depth_ramp_20260718/,
/scratch/a/A.Otaifi/paratoric_linsel_no_pt_l20_depth_ramp_20260719/;
diagnostics table: doc/figures/linsel_no_pt_l10_l20_diagnostics_20260720.tsv.
## RESULT — doc fixes done; free crossing re-derivation done: your 20g prediction FAILED, but so did the "real tension" alternative (2026-07-21)

Per your 20h loop containment, did the doc fixes and the free crossing re-derivation
before touching 2A/2B.

**Doc fixes** (F3/F7 in paratoric_run_manual.md, H1/H2 + new Correction-scope/
Retrospective block in pt_architecture.md, gate G1.2 in
multicritical_execution_harness.md): Clark wrote the diffs, Martina reviewed and
APPROVED (Grace timed out on the bridge -- known flakiness per JARVIS.md, proceeded
on Martina's clean approval alone given this was a low-risk wording-only change).
All three files now say "neither necessary nor sufficient," carry the continuous-
vs-first-order distinction and the thesis-T1-undecided caveat, and pt_architecture.md
has your retrospective note that the 4 NO-GO calls used the wrong metric without
walking back the marker test's real 0.235% barrier.

**Crossing re-derivation** (your 20g sec 4, pre-committed on the outcome): restored
both formerly-void L=20 points (h=0.305629, h=0.339574) and recomputed the L=10/L=20
raw-ratio crossing.
**Your prediction FAILED**: the linear crossing did NOT move up toward 0.34 -- it
stayed at 0.3244097798, identical with and without the two restored points, because
the crossing is bracketed by h=0.322/0.326, nowhere near the restored points. Direct
numbers: WITH all nine = 0.3244097798 +/- 0.0008659348 (stat); WITHOUT the two =
0.3244097798 +/- 0.0008649342 (stat) -- literally the same crossing, restoring the
points changed nothing about ITS location (their steep segment is elsewhere on the
curve, my prior "removing a steep segment moves the crossing" intuition was wrong
for THIS crossing specifically, even though it's a fine general concern).
BUT the honest interpolation-systematic scan (18 choices: linear/PCHIP/natural-cubic/
polynomial families x 2 fit windows) gives a full spread of 0.0112154963
(0.3237-0.3349) -- 13x the naive bootstrap-only bar, and this envelope DOES reach
your 0.335(2) band. So neither of your two pre-committed outcomes landed cleanly:
not "moved up and confirms no anomaly," but also not "stayed put with a defensible
systematic that proves a real tension" -- the honest systematic itself is wide enough
to swallow the tension. Read plainly: this benchmark currently supports "consistent
with Linsel within honest systematics," not a discovery, not a clean confirmation.
The dominant limitation is L=10's nonmonotonic curve making interpolation choice
underdetermined, not statistics.
**Also found and fixed**: the previously-quoted crossing error "+/-0.0000817" was a
decimal-place TRANSCRIPTION ERROR in narrative reporting (mine, propagated across
several status.md entries) -- the source TSV always said +/-0.0008170099. Neither
number was ever the real total uncertainty; flagging so it's not silently recopied
again.
benchmark_report.md sec 4-6 updated with the full table, both crossings, the
systematic scan, and the transcription-error note. **Kept PROVISIONAL**, as you
specified -- this pass restores data and removes false precision, it does not
promote the benchmark on its own.
New figure: doc/figures/linsel_raw_ratio_crossing_no_pt_l10_l20_restored_20260721.png

Proceeding to 2A (locate s*) and 2B (PT speedup) in parallel per your loop
containment. Will report both together per 20g/20h.

## RESULT — 2A: order INCONCLUSIVE (as your spec anticipated), s* provisionally bracketed [0.325,0.350] via a secondary method only. 2B: INCONCLUSIVE for a real reason -- baseline/rung mismatch, not a null physics result. Stopping per loop containment. (2026-07-21)

Both ran to completion (2A: job 15177203, 528/528 tasks; 2B: job 15178251, 14
ranks, 1h27m). Process note first: my original dispatches for both bundled
"submit + wait for completion + analyze" into one call, which exceeded the
30-min MCP idle timeout and orphaned the background codex processes (they kept
running server-side, doing real work, but I never received their reports and
their temp output was lost). Re-dispatched clean readout-only calls once I
confirmed both Slurm jobs were actually done -- noting this as a process lesson
(should split "submit" from "readout" into separate dispatches from the start,
per this project's own dispatch-sizing guidance).

### 2A (locate s*): ORDER INCONCLUSIVE -- exactly the acceptable outcome you set
528-task wide scan (L=8,12,16; s=0.200-0.450; both bases), preceded by a
16-task pilot (job 15177060) that validated the method first.
- **Duality (I2/B5)**: 200/201 gate-eligible comparisons PASS. One FLAG:
  L=8,s=0.275 at the 200k-sweep checkpoint (z=3.86), but it resolves by 400k
  (z=0.034) -- reads as an ordinary early-checkpoint thermalization blip, not a
  systematic mapping bug. No evidence the instrument is broken anywhere on the
  line.
- **Energy gap / bimodality**: 14 trajectories classified SHRINKING_TO_ZERO
  (ordinary under-thermalization), ZERO classified STABLE-SEPARATED
  (coexistence). ALL 528 pooled/per-chain histograms are unimodal
  (NO_CANDIDATE for bimodality) -- per your own asymmetry rule this is reported
  as INCONCLUSIVE, explicitly NOT as evidence the MCP is continuous. At
  L=8-16 this is exactly the outcome you told us to expect and treat as
  success, not failure.
- **s* bracket**: the PRIMARY (fixed-checkpoint-window) method found no common
  three-size crossing usable for a bracket. A SECONDARY (interval-window)
  method gives a PROVISIONAL bracket s* in [0.325, 0.350] -- consistent with
  Linsel's prior (~0.33-0.35), but resting on the weaker of the two methods.
  13 point-pairs came back NOT_GATED. Recommend denser sampling specifically
  in [0.325,0.350] before promoting this bracket to anything firmer.

### 2B (PT speedup): INCONCLUSIVE, but NOT a null result on PT -- a real design gap
The pre-check (2B.0) passed cleanly: fixed-h reconstruction from the rank-local
+ ladder-index series IS valid (verified against the C++ output schema), so
the measurement was well-founded methodologically. But the actual result
cannot answer the question as designed:
- **Baseline mismatch**: the Lane-2 PT ladder has 14 rungs; the no-PT L=10
  baseline (job 15149119) scanned only 9 h-values on a DIFFERENT grid. Only
  2 of 14 rungs (h=0.31375, h=0.335) land close enough to a no-PT point to
  compare AT ALL. 12/14 rungs have no baseline to measure speedup against.
- **Both matchable points are tau-unresolved** (blocking-plateau lower bounds,
  not converged values) on BOTH the PT and no-PT side -- so even the 2
  comparable rungs give a ratio of two lower bounds, which is not a valid
  bound on the true speedup. The "quotients" computed (144x and 64x at those
  two points) are explicitly flagged by the worker as NOT speedup bounds --
  do not read them as evidence PT is 60-140x faster, that would be wrong.
- **The equal-core-hour Neff/core-hour comparison** (the deciding number) came
  back with the same indeterminacy: PT <=1112.7 vs no-PT <=149.7 Neff/core-hour
  matched-subset (quotient ~7.4), and at h_c specifically PT <=591.0 vs no-PT
  <=106.4 (quotient ~5.6) -- but both sides are lower/upper bounds built on
  unresolved tau, so these numbers are NOT decision-grade. Do not fold either
  quotient into the M2 budget.
- **A genuine new flag, not explained away**: PHYSICS AGREEMENT shows the
  PT-run's finite-L crossing (PT-L10 vs no-PT-L20) lands at 0.336613, OUTSIDE
  the no-PT-only crossing envelope (0.3237-0.3349) by 0.0017 -- small in
  absolute terms but a real, measured shift, not noise. This needs
  investigation before PT is trusted at the MCP, independent of whether PT
  turns out to help with speed.
- **Excursion statistics** (the round-trip replacement): 388,711 excursions
  closed, but mean range is only 1.33 RUNGS, median 1, max 13; only 21.4%
  reach >=2 rungs, 6.6% reach >=3. Replicas mostly wiggle one rung and come
  back -- this is a genuinely limited-transport picture, consistent with
  (not contradicting) the marker test's localized-barrier finding, even
  though round-trip count is no longer the gate.
- **Harmonic vs median acceptance**: harmonic mean 0.0988 vs median 0.237 --
  the harmonic/median gap persists exactly as Lane 1 found; weak links still
  dominate transport.
- Swap-period curve (2B.3(i)): only swap_period=2 ran (1/5/10 correctly
  skipped, would have ~4x'd cost) -- this is a single data point, explicitly
  not a curve; the swap-period optimum remains unlocated.

**DECISION RULE: NOT TRIGGERED.** Per your own wording limits, neither
"PT worth deploying" nor "a plain h-ladder does not help" is licensed by this
data -- both taus are censored (lower bounds), so there is no honest factor
to quote yet, intermediate or otherwise. Recording this as UNRESOLVED, not as
a negative result on PT.

### RECOMMENDATION (not deciding myself)
2A: denser sampling in [0.325,0.350] to firm up the s* bracket -- cheap,
same method, clear next step.
2B: a redesigned retry needs (a) a no-PT baseline whose h-grid actually
overlaps the PT ladder's 14 rungs (or vice versa -- redesign one to match the
other), (b) enough production depth to actually resolve the blocking-plateau
tau at the rungs that matter (not just budget for ~200 tau against an assumed
tau, which is what left both matchable points unresolved), and (c) a look at
why the crossing shifted 0.0017 outside the no-PT envelope before spending
more on the speedup question. This was ~22-28 task-hours that produced a real
methodological lesson (fixed-h reconstruction works; rung/baseline matching
must be verified BEFORE submitting, not after) but not the decision-grade
number it targeted.

Per your loop containment, stopping here. No L=30, no L=24, no PT ladder
work until your read on this lands. Full data: 2A at
/scratch/a/A.Otaifi/paratoric_selfdual_step2a_20260721/ (job 15177203); 2B at
/scratch/a/A.Otaifi/paratoric_pt_speedup_l10_20260721/ (job 15178251).

## RESULT — PHASE 1 (per your 20m ratified sequence): 1A done, crossing genuinely disappears in the clean window. 1B/2A revealed a code defect AND a real open question I'm not resolving myself. 1C fixed+reviewed. STOPPING here per your loop containment. (2026-07-21)

Executing your 20m PHASE 1 exactly (1A/1B/1C/1D), free, no new jobs.

### 1A: clean-window crossing -- DONE. The crossing does not just fail to move, it DISAPPEARS.
Re-derived using only h where both L=10/L=20 pass the ratified per-chain spread
gate (your 20j numbers). Result at every threshold tried:
- gate <0.10 (6 points, h=0.326-0.347399): NO crossing -- L10 minus L20 is
  negative at all 6 points, no sign change. 8/18 interpolation choices produce
  higher-order overshoot roots (0.32861-0.33549, width 0.0069) but these are
  NOT bracketing roots -- quoting one would be model extrapolation between
  non-crossing data, not a measurement. 10/18 have no root at all.
- gate <0.05 (3 points): 0/18 choices have a root. Envelope undefined.
- gate <0.03 (1 point): not estimable from a single point.
PRE-COMMITMENT HONORED: did not tune the threshold toward 0.335; reporting the
curve as it came out. Also caught and resolved a window-convention mismatch
between the ratified spread audit (400k-sample discard) and the crossing
curve (632k-sample stationary-suffix discard) -- recomputing on the same
632k suffix changes the <0.05 field set slightly but the no-crossing verdict
is unchanged either way.
**Conclusion: the old 0.3244098 "consistent within honest systematics"
statement is fully superseded. We currently have NO usable L=10/L=20 crossing
from clean data at all** -- not a wide one, not a contested one, none.
benchmark_report.md sec 4-6 updated accordingly, stays PROVISIONAL, states why
in one line as you specified. Figure
doc/figures/l10_chain_disagreement_and_pt_repair_20260721.png is in place
beside the crossing figure (PT-overlay panel deferred, L10/L20 per-chain-dot
panel is populated).

### 1B: per-chain audit -- L10/L20 already fully covered by your own 20i/20j
numbers, no new work needed there. The NEW piece was the 2A scan (job
15177203), and it surfaced a real process failure plus a real open question.

**Process failure (caught by review, now fixed):** my first 2A chain-audit
pass reported "clean at s=0.200-0.300 (30/30 pass), contaminated at s>=0.325"
using the `selfdual-step2a --window-mode fixed` mode. Per this project's own
review discipline, I sent the ~1967-line script addition that produced this
to Martina before trusting it (it was written directly by an analysis
dispatch, not through Clark -- a process gap on my part, fixed going
forward). Martina REJECTED: `--window-mode fixed` was a no-op (both fixed and
interval windows were always computed regardless of the flag), and the
`ratio_spread` number was computed but never actually thresholded into the
PASS/FLAG gate -- only the tau-factor>2 criterion was real. Clark fixed both
(a genuine `--spread-gate`, default 0.05, now combines with tau-factor<=2.0
into a real `chain_audit_gate` column; a legacy-TSV SHA-256 regression
assertion now backs the "byte-identical" claim instead of asserting it).
Martina re-reviewed and APPROVED the fix.

**The open question this surfaced, NOT resolving myself:** re-running the
corrected code against the SAME job-15177203 data changed the picture
substantially. My original "clean 30/30" claim had actually been read off the
`final_interval` (32,000-sample) window's tau-factor column, not the
requested `final_fixed` (400-sample) window. Under the properly-wired
`final_fixed` window WITH the now-real spread gate: **29 of the 30
previously-"clean" (L,s,basis) rows at s=0.200-0.300 now FAIL** (26 fail on
spread alone, 3 fail on both gates) -- only (L=16, s=0.225, z-basis) still
passes cleanly. The s>=0.325 region remains fully contaminated (36/36 fail)
either way, unchanged.
**I am explicitly NOT deciding what this means, and flagging why**: this
could be (a) genuine widespread per-chain basin contamination across nearly
the WHOLE 2A scan, far worse than the original report suggested, or (b) a
statistical-power artifact of the 400-sample fixed window being too short for
the spread metric to be trustworthy at all -- structurally the SAME class of
mistake as your own 20d/20e/20f "reading noise as an unresolved trend from
too few blocks" episode on the L=20 tau estimator. I do not have a principled
way to tell these apart from here without your read (or a targeted check,
e.g. whether the interval-window's much larger sample count gives a
materially different, presumably more trustworthy, spread estimate at the
SAME points, which the original interval-window pass suggests it does).
This is squarely a repeat-shaped risk given how much this campaign has
already paid for exactly this confusion once -- flagging loudly rather than
either (i) reporting the alarming fixed-window numbers as the truth, or
(ii) quietly keeping my original, now-known-incomplete "clean at s<=0.300"
claim.

### 1C: standing code changes -- DONE, reviewed, APPROVED
Per-chain spread and tau-factor are now permanent, gated columns in
scripts/ensemble_diagnostics.py (`--spread-gate`, default 0.05, combined with
the existing tau-factor<=2.0 gate into `chain_audit_gate`). Legacy-TSV
byte-identity is now a real self-test assertion (SHA-256), not an asserted
claim. Martina APPROVED after one fix round (REJECT -> Clark fix -> APPROVE).
Grace was unavailable both rounds (one MCP timeout, one NVIDIA HTTP 504) --
proceeded on Martina alone given the fixes were narrow and concrete.
NOT yet done: adding the between-chain-error column (20f) and making the
run_manual.md "report per-chain table, never only group summary" doc
addition -- flagging as leftover Phase 1C scope if you want it done before
Phase 2/3, or foldable into whichever phase runs next.

### 1D: DONE as part of 1A's dispatch (see above).

### STOPPING per your loop containment
Per 20m: "Do not run Phases 2 and 3 before Phase 1 lands." Phase 1 has
landed, but with one real open question (the 2A window-mode/statistical-power
ambiguity above) that directly gates Phase 3's decision (per-chain-gating+PT
at L=8-16, vs. the now-moot "denser [0.325,0.350] sampling" option -- that
bracket is contaminated either way the ambiguity resolves, so Phase 3's
"clean, approved as originally proposed" branch is now closed regardless;
what's undecided is how BROAD the L=8-16 contamination actually is). Also
flagging for Phase 2: PT-at-L=10 is now motivated by TWO separate findings
(20i's ergodicity-repair argument, and 1A's "crossing does not exist without
it" finding) -- both point the same way, toward Phase 2 being higher-value
than before, not lower.
Awaiting your read on the 2A ambiguity before Phase 2/3 compute.

## RESULT — your 20p answer executed: 4(d)/4(a)/4(b)/4(c) all done. s* bracket confirmed with a trustworthy instrument; two new gates wired, reviewed, approved. Housekeeping fixed. Ready for Phase 2. (2026-07-23)

Your 20p answer resolved the 2A ambiguity cleanly: s<=0.300 was genuinely clean
(confirmed noise, my original report was right), s>=0.350 was neither
hypothesis -- the raw-ratio instrument itself dies there (>50% exact zeros),
and the vanishing IS the transition. Executed the free items in your stated
order.

**Housekeeping (sec 6) -- done first.** jarvis_report_tofeynman.md was out of
chronological order (a 2026-07-19/07-20 block had landed below newer
2026-07-21 entries). Archived everything before 2026-07-18 to
doc/archive/jarvis_report_tofeynman_archive_pre_20260718.md (matching your own
archive convention/naming), restored strict chronological order for the rest,
added a "NEWEST ENTRY AT THE BOTTOM" marker at the top. Verified programmatically
that the reordering lost or duplicated zero lines before writing anything.

**4(d): s* bracket from the correct instrument -- done, free.** Re-derived
using percolation_probability (bounded, no vanishing-moment division) and
largest_cluster as a sanity check, on the existing 2A data. Finite-L
crossings of percolation_probability across the three L pairs: z-basis
0.330968/0.331043/0.331141, x-basis 0.338041/0.331170/0.328490 -- tightly
clustered around **s* ~ 0.332**. Largest-cluster's steepest-drop location is
a broader but consistent sanity check ([0.325,0.375] range). **Bracket
[0.325,0.350] is numerically unchanged from the broken-ratio estimate, but is
now instrumentally defensible** rather than an artifact -- confirmed no
divide-by-zero behavior in either bounded observable at s>=0.350, validating
these as the right instruments there. Central ~0.332 reported as a location
guide, explicitly not a precision claim (endpoint/window sensitivity remains).

**4(a): 2A reading restored -- superseded by something better than a binary
restore.** Rather than just reverting to "clean/contaminated," the two new
gates below give a proper three-way breakdown of all 66 (L,s,basis) points:
**26 PASS, 10 genuine FAIL, 30 OBSERVABLE-DEGENERATE** (dying-instrument
regime). This replaces both my original binary claim and the false 29/30
alarm with an honest partition.

**4(b)+4(c): two new gates wired into scripts/ensemble_diagnostics.py, code-
reviewed.** (b) A spread-window-scaling check: classifies per-chain spread as
NOISE (tracks 1/sqrt(window) scaling within a factor of ~2 across a >=100x
window-size change) vs REAL (persists/grows), with a SPREAD-UNRESOLVED status
when there isn't enough window-size data to decide (none needed it this
round). (c) An OBSERVABLE-DEGENERATE tripwire: any point with >50% exact
zeros in percolation_strength is flagged and its ratio-based numbers are
fully suppressed (NA, not a fake-looking number) at both summary AND
per-chain granularity -- this was itself a 2-round review catch (Martina
first REJECTED because degenerate per-chain rows still said "OK" with real-
looking numbers; Clark fixed it to NA; re-reviewed and APPROVED). One
boundary case surfaced and handled correctly rather than forced: L=8, s=0.350,
x-basis has zero_fraction=0.4952, just under the 0.50 threshold, so it
correctly stays a genuine FAIL rather than being swept into
OBSERVABLE-DEGENERATE -- worth knowing that point specifically may be a real
(non-instrumental) issue, not yet investigated further.
Legacy-TSV byte-identity (SHA-256) self-test still PASSES throughout both
review rounds -- no regression to any previously-reported number.
Grace was unavailable for these reviews (timeout / HTTP 504 both encountered
earlier this session); proceeded on Martina alone given each fix was narrow
and concretely verified against the specific defect raised.

**Standing debt, not done this round:** per-chain-error column (20f) and the
run_manual.md "report per-chain table, never only group summary" doc addition
from Phase 1C remain outstanding. Also outstanding per your 20n sec 4 item 3:
the invariant-provenance audit (checking the rest of the harness/run-manual
for other unvalidated-but-cited rules like the old F3/F7/G1.2 and the
now-fixed intermittency tripwire) has not been started.

Ready for Phase 2 (PT-at-L=10, redesigned ladder) on your go -- ordering per
your loop containment is satisfied (4(d)->4(a)->4(b)+4(c)->housekeeping, all
free items, reported before new compute).

## RESULT: Phase 2 (PT-at-L=10, redesigned ladder) executed end to end. GATE = FAIL. Init memory does NOT drive to zero at the low-h end; crossing and speedup stay UNRESOLVED. This is the outcome your own plan named as a live possibility, not a process failure (2026-07-24)

PI said "go" on Phase 2. Redesigned 14-rung ladder (drop <~0.28, respend
0.29-0.42, N=14 fixed) submitted per the ratified spec, ran to completion
across all 4 swap-period variants (sp1/sp2/sp5/sp10, job `15215269`, 8h13m,
exit 0:0), and was read out against the GATE you defined: **per-rung
agreement + init-memory driven to zero -- NOT round-trips.**

**GATE VERDICT: FAIL, MIXED by rung.** All 40 tau-RESOLVED high-h
(rung>=4, h>=0.360) cold/hot-origin comparisons PASS at z<3 -- the fast end
of the ladder is clean, converged, no ambiguity. All 16 low-h (rung 0-3,
h=0.2925-0.347632) cases across all four swap-periods are tau-UNRESOLVED, and
12 of those 16 retain >=3 sigma init memory between replicas that originated
from a cold (init+1) vs hot (init-1) starting rank -- real, signed,
non-vanishing gaps (e.g. sp1 rung0: cold=1.3463 vs hot=1.3669, gap
z=5.42; sp1 rung2: z=7.24). No monotonic drive toward zero across
sp1->sp2->sp5->sp10 -- e.g. low-h fail counts are 4/4, 3/4, 3/4, 2/4, better
but not resolved even at the least-frequent swap attempts tried. Per your own
plan's decision rule, stated in advance: **"IF PT CANNOT drive the low-h init
memory to zero, report that plainly -- it is a real limit on PT and we need
it before the MCP, not after."** This is that outcome.

**Crossing (ii) and speedup (iii): both UNRESOLVED, not just "small."** Only
2 of the 14 redesigned rungs land on the no-PT baseline's (job 15149119)
h-grid at all -- the ladder redesign fixed the acceptance-rate problem
(median swap acceptance jumped from ~0.10 harmonic on the old Lane-2 ladder
to ~0.48 harmonic here) but did NOT fix the h-grid mismatch that already
made the first 2B attempt inconclusive. Both matched points are
`INDETERMINATE_BOTH_TAU_LOWER_BOUNDS` (tau unresolved on PT AND no-PT sides),
so no honest speedup number exists -- per your own established censoring
discipline, none is reported. One tempting but rejected read: sp2 and sp5
each resolve a crossing (0.336282, 0.336321 -- essentially identical,
tantalizingly close to Linsel's 0.335(2)), but sp1 finds zero brackets and
sp10 finds five (ambiguous). Jones's independent read, which I accept: this
agreement is NOT independent confirmation -- both numbers are built from the
SAME 2 indeterminate-tau points, and the two swap-periods that disagree
(sp1, sp10) are just as valid a priori. Treating sp2/sp5's numeric closeness
as physics would be exactly the kind of unsupported strengthening your own
"specs name evidence not verdicts" standard exists to prevent.

**What this does and does not mean, per your own decision rule:** this
rejects the TESTED combination -- this specific redesigned ladder at this
specific depth (40M thermalization + 8M production per variant) -- not PT as
a method, and not the beta-bridge (a different-axis ladder, unaffected
either way). The likely fix is not swap-period tuning (all four tested values
show the same qualitative failure) but MORE DEPTH at the low-h end
specifically, and/or a ladder redesigned to actually land on the no-PT
baseline's h-grid so a real speedup comparison becomes possible at all.

**Process note, in case it matters for how you read the numbers:** a Codex
provider-side outage (quota exhaustion, then a revoked auth token, on the
original account) hit mid-readout. Rather than block, I ran the two
already-reviewed, already-registered analysis scripts
(`pt_diagnostics.py`, `ensemble_diagnostics.py pt-speedup`) directly myself
via Bash, using `--resamples 50` instead of the default 1000 to fit this
environment's ~10-minute practical execution ceiling (confirmed by direct
testing that longer runs were being silently killed and falsely reported as
"completed" -- a real bug in this session's background-task tracking, logged
separately). Once Codex came back, Jones reviewed this manual work
independently and flagged one caveat worth carrying forward: block errors use
twice the largest measured tau lower bound, which is a conservative choice --
the true low-h significances could be even larger than what's reported here,
not smaller. The reduced-resamples choice itself was checked and does not
change any RESOLVED/UNRESOLVED call or crossing bracket.

Full per-rung tables: `/tmp/phase2_init_memory_job15215269.tsv` (56 rows,
cold/hot ratio + signed gap + z per rung per swap-period),
`/tmp/phase2_init_memory_job15215269_summary.tsv` (per-swap-period gate
summary), `/tmp/pt_speedup_sp{1,2,5,10}_table.tsv` (per-rung tau/Neff/
speedup-status), `/tmp/phase2_partA_crossing_roots.tsv` (crossing roots per
swap-period). Full run data: job `15215269` at
`/scratch/a/A.Otaifi/paratoric_pt_l10_phase2_redesigned_ladder_20260723/`.
`status.md` has the full operational history including an OOM'd first attempt
(job `15213796`, fixed by correcting a `--mem` sizing mistake, resubmitted
clean as `15215269`).

**Not deciding for you:** whether to spend more depth at the low-h end of
this same ladder, redesign the ladder again to hit the no-PT h-grid, or park
Phase 2 and reassess priorities -- that's a real fork and I'd rather you see
the actual per-rung z-scores than a pre-chewed recommendation.

## RESULT: M2 (Phase 2B-redo) executed per your 20w CONSOLIDATED GO. Three-leg GATE = PASS. The benchmark crossing is computable for the first time -- but it has no error bar yet, and the PT tau at the matched h-values shows a pattern that looks like a reconstruction artifact, not real physics. Reporting both the good news and the reason I am not calling this closed (2026-07-25)

Executed M1 (both free items), M4(2)+(3)+(4) (standing debt), and M2 (the one new
run) per 20w's ordering. This entry covers M2; M1/M4 are in `status.md` in full.

### M2 setup
Re-spanned ladder: 9 rungs at the EXACT no-PT baseline h-values (job `15149119`'s
grid), 1 transition rung, 4 reservoir rungs above 0.36 (reused from `15215269`'s
traversal-preserving end). N=14 unchanged. ONE swap period, 100 sweeps (per your
20v criterion: tau(energy) sets the scale, not a scan). Same depth as `15215269`
(40M thermalization + 8M production/rung). New mandatory pre-submission window-
coverage check (your 20w request) built, self-tested PASS on the new ladder
(9/14 inside [0.305,0.350]) and correctly FAILS as a negative control on the OLD
`15215269` ladder (3/14 inside, exit 1, explicit ERROR) -- the two-line check
that would have caught your own 20k error now exists and works. Job `15232180`
ran clean: 1h27m, exit 0:0, well within the 96G budget.

### THE GATE: PASS, all three legs, reported together (not traversal alone)
1. **Traversal**: complete. 14/14 replicas visited both endpoints, 130 strict
   round trips, matches `15215269`'s traversal quality -- the re-span did NOT
   degrade transport, which itself answers your 20w pre-committed alternative
   ("traversal degrades in the h_c region => beta-bridge evidence") in the
   NEGATIVE: transport survives moving 9 rungs into the benchmark window.
2. **tau_config**: finite. Aggregate tau(energy): IPS 8325+/-893 sweeps,
   blocking (lower, since individual-replica estimates are themselves
   TAU-UNRESOLVED) 6113+/-324 sweeps. sp=100 sits 61-83x below tau(energy) and
   ~10x above the pure-repetition floor -- your safety criterion holds, as a
   BY-PRODUCT check, no re-scan performed.
3. **Replica-level cold/hot z**: max z=2.88, at rung 10 (h=0.369418, a
   RESERVOIR rung, OUTSIDE the benchmark window). Inside the window that
   actually matters (rungs 0-8, the 9 matched h-values), max z=1.68 -- the
   physically important region passes comfortably; the borderline point sits
   outside it. Full 14-row table: replica_gate_raw_ratio.tsv (path in
   `status.md`).

### THE CROSSING NOW EXISTS -- with two live caveats, not a clean win yet
`pt-speedup`'s crossing derivation against clean L=20 (now meaningful, since 9/14
rungs land inside the comparison window instead of 2/14): **h* = 0.336519,
bracket [0.335363, 0.339574]**, close to Linsel's 0.335(2). This is the FIRST
TIME this campaign has had a computable L=10-vs-L=20 crossing since Phase 1
found none in the clean no-PT window. But:
 (a) **uncertainty = NOT_ESTIMATED** -- the crossing derivation did not propagate
     an error bar. "Close to Linsel" is a point-estimate observation, not a
     confirmed statistical agreement, until that's fixed.
 (b) **PT tau at the 9 matched h-values is suspiciously constant**: 50-52
     sweeps at every TAU-RESOLVED matched point (h=0.318989 -> 51.2,
     h=0.322 -> 50.1, h=0.335363 -> 50.9, h=0.339574 -> 51.9, h=0.358408[not
     matched] -> 50.2), regardless of h. Real relaxation should vary with h
     the way it does on the no-PT side (which spans 1184-13732 sweeps across
     the same range). A tau pinned near half the swap period smells like the
     concatenation-of-disjoint-visits artifact you flagged in 20u sec 4/20v
     sec 3 -- the reconstruction may be measuring the dwell-segment length,
     not the physical relaxation time. I have NOT resolved which it is; flagging
     it rather than either trusting or dismissing the number.
Consequence: the four LOWER_BOUND "speedup" values this run produced (254x,
274x, 128x, 70x at the 4 TAU-RESOLVED matched points) are built on that same
suspect PT tau divided into a no-PT tau that is itself unresolved/lower-bound.
Per your own decision rule and Jones's explicit recommendation, which I accept:
**do NOT fold this into M2 as a demonstrated speedup lever.** Cost-normalized
Neff/core-hour bounds near the crossing are only 1.5-6.6x, and are themselves
biased optimistic by the same artifact. Plain-h-ladder benefit remains
UNPROVEN -- not disproven, not proven, and specifically not "PT doesn't help"
or "the beta-bridge is needed instead."

### A SEPARATE FLAG: PT and the (known-unreliable) no-PT L=10 raw ratios disagree at several matched points
raw_ratio_z between PT and no-PT L=10 at the SAME h: 2.99 (0.305629), 0.26
(0.318989), 1.35 (0.322), 3.99 (0.326), 2.36 (0.330615), 2.18 (0.335363), 2.08
(0.339574), 4.91 (0.343521), 1.68 (0.347399) -- six of nine points exceed z=2,
two exceed z=3. This could read as a genuine PT-vs-no-PT physics disagreement,
but the no-PT L=10 ensemble is the SAME one Phase 1 found non-ergodic (I4
factor-2 violated by 3-4 orders of magnitude at low h) -- its quoted errors are
plausibly too small for the same block-vs-replica reason M4(2) just fixed for
PT. I have not applied a replica-level correction to the no-PT side here; that
may be the right next diagnostic before trusting or discounting this tension.

### M1 + M4 headline (full detail in status.md)
M1(a) benchmark extrapolation: h_c(L->inf) = 0.323577 +/- 0.004243 from the 3
finite-L PT crossings -- LIES BELOW Linsel's 0.335(2) band, ~2.4 sigma tension,
reported plainly per your no-tuning constraint. M1(b) energy-Binder s*
cross-check: NEGATIVE, no crossing found, no independent confirmation of
s*=0.333+/-0.004. M4(2): Clark wired `pt-replica-gate` into
`ensemble_diagnostics.py`, reproduced your 20t hand-derived numbers to 3
decimals, Martina APPROVE (Grace hit a tooling failure, not a substantive
finding -- recorded, not treated as an objection). M4(3)/(4): doc rules added
(G7 in the run manual; the background-task false-completion bug documented as
a harness issue with a sentinel-workaround, since it cannot be patched from
inside this project). M4(1) invariant-provenance audit is still unscoped --
next in queue, not started.

### WHAT I THINK THIS MEANS, STATED AS UNCERTAINTY NOT A VERDICT
The gate result is real and good: PT is validated as an ergodicity-repair tool
at L=10 (traversal + tau_config + replica-level agreement, all three, in the
region that matters). The crossing existing at all is a genuine first. But I
would not describe the benchmark as closed: no error bar on the crossing, a
PT-tau pattern that looks more like an artifact than physics, and unresolved
tension against a reference ensemble already known to be compromised. The
honest next question is whether the ~50-sweep PT tau is real or a
reconstruction artifact -- that's a diagnostic question, not a re-run, and
I have not attempted it yet.

Full evidence: `/tmp/phase2b_redo_jones_15232180/*.tsv` (traversal, replica
gate, matched-h speedup, Neff/core-hour, replica-following taus, crossing
summary). Run data: job `15232180` at
`/scratch/a/A.Otaifi/paratoric_pt_l10_phase2b_redo_20260725/`.

### LOOP CONTAINMENT (per your own 20w: "stop and report before M3")
Stopping here. M3 (denser s* grid) and M4(1) (invariant-provenance audit) both
remain queued, neither started. Awaiting your read on the crossing-artifact
question and the no-PT-side tension before I'd want to spend more compute in
this direction myself.

## ACKNOWLEDGED: 20x-20aa read in full. Artifact confirmed with mechanism, benchmark closed, real speedup = 1.19x/1.21x. Proceeding to M4(1) then M3 (2026-07-25)

Read 20x through 20aa before doing anything else. The tau-artifact suspicion I
flagged was real, and your mechanism (swap_period=sample_period=100 sweeps =>
PT tau of ~50 was literally 0.51 samples, the exact tau_int white-noise floor;
mean dwell 1.15-1.89 samples per visit, so concatenation destroyed the
correlation it measured) is more precise than my flag — I said "smells like an
artifact," you showed exactly why. `status.md` is corrected in place: the
254x/274x/128x/70x numbers are struck, replaced with the real reconstruction
(median cost-normalized speedup 1.19x, 1.21x at h_c, PT tau now decreasing
monotonically with h as it should). I accept the 20c decision rule firing on
that basis, and I accept your synthesis that this is a SPEED verdict only —
the gate PASS (traversal + tau_config + replica-level z<3) is a separate,
already-settled ERGODICITY-REPAIR result that the speed number doesn't
overwrite. Benchmark closure (0.3359 +/- 0.0049, 0.2 sigma) and the exponent
correction are noted; M1(a)'s superseded number was your own 20s spec, not my
execution error, and I read it that way, not as a self-defense.

Proceeding per your 20aa sec 8 ordering: M4(1) invariant-provenance audit
first (scoping it now, it was left open-ended — "audit the rest" against the
three known cases: run-manual F3/F7, harness G1.2, the intermittency
tripwire), then M3 with the strengthened design requirement from 20x sec 6
(more sizes, wider L range, sized to fit an exponent — not the original 20w
M3 spec). Will report both.

## RESULT: M4(1) invariant-provenance audit complete. 84 rules found across the three docs, only 5 properly validated AND wired. The worst gap: the actual L=120 PT GO/NO-GO decision rule (harness G2, manual F6) has no code enforcement at all — pure prose for the single most expensive decision in this campaign. Audit only, NOT remediated (2026-07-25)

James (read-only) went through every numbered/lettered rule in
`multicritical_execution_harness.md`, `paratoric_run_manual.md`, and
`pt_architecture.md` and cross-checked each against `scripts/*.py` for actual
enforcement. Full 84-row table is in `status.md`'s pointer to
`ai-workflow/agent_bridge/logs/msg_codex_1784989171.txt` (too long to paste
here in full; summarizing the shape and the worst finding).

**Breakdown**: 5 VALIDATED+WIRED (a citation AND a real code gate — e.g. run-
manual G1/G4/G7, `pt_architecture.md` D2/Law-9.1). 14 VALIDATED-NOT-WIRED —
your exact intermittency-tripwire disease, real evidence behind the rule but
no code checks it (e.g. harness G0.1's L=40/beta=80 1%-reproduction baseline;
manual G6's geometric-checkpoint-ladder incident; `pt_architecture.md` D4's
"must recover the known Ising line" -- which is, not incidentally, exactly
what we just did by hand for the benchmark closure and could have been an
automated check). 15 WIRED-NOT-VALIDATED — your F3/F7 disease, a real
threshold in code with no citation for why that number -- e.g. harness I2/I4/
I5b/I6, manual G3, `pt_architecture.md` H4a/D3/Law-8.2/8.4. **44 NEITHER** --
pure assertion, no citation, no code (over half the total). 6 already-known-
corrected cases (F3/F7/H1/H2/tripwire).

**The single worst gap, by cost-if-wrong**: harness G2 and manual F6 --
the actual PT-at-L=120 decision rule (>=5x speedup => GO, 2-5x => marginal,
<2x => NO-GO, plus a budget guard) -- is NEITHER validated nor wired.
`ensemble_diagnostics.py` computes a speedup number but nothing in code ever
applies the GO/NO-GO bands, checks L=120, checks for a winding-sector change,
or enforces the budget guard. This is the highest-stakes decision this
campaign will make about PT, and right now it depends entirely on a human
reading the number correctly and remembering the bands -- the exact
precondition that produced the round-trip mistake, the block-error mistake,
and the p=1-exponent mistake. Given M2 just delivered a working PT-speedup
methodology (the real-time fixed-h reconstruction), wiring an actual G2/F6
gate command is now both urgent and cheap -- it would reuse code that already
exists rather than write anything new.

Two smaller but concrete findings worth fixing regardless of remediation
priority: (a) harness G1.3 / `pt_architecture.md` S4 ("PT-on marginal equals
PT-off, z<3") computes the z but never applies the threshold -- a
correctness regression check that structurally cannot fail today no matter
what the number is; (b) manual C7's own pitfall-catalog text still says
"judge by strict round-trips," directly contradicting the already-corrected
F7 rule two entries below it in the same file -- a live in-document
inconsistency, not just a missing gate.

**I did not remediate anything** -- 84 rules is a large remediation surface
with sharply diminishing returns per rule, and prioritizing which of the 79
non-clean rules are worth wiring first is a scope decision I'd rather you
made than default into myself, especially with the benchmark just closed and
M2 just settled. The worker's own risk-ordered list (by cost-if-wrong) is in
`status.md` in full.

### LOOP CONTAINMENT
M4(1) reported. M3 (denser s* grid, strengthened design per your 20x sec 6)
is next and not yet started. Stopping to await your read on remediation
priority for the audit, and on whether M3 proceeds now or after any G2/F6
wiring.

## RESULT: 20ab acted on. Two-stage G2/F6 gate implemented, reviewed APPROVE. Your hand-table discrepancy resolved: Clark's code is right, your 6/9 was a looser informal read of the real 8/9 gate. M3 unblocked (2026-07-25)

Read 20ab in full before doing anything else. Your withdrawal of the 20aa
"1.21x, decision rule fires" verdict is accepted, and the reasoning is tight —
dividing PT tau by a no-PT reference that fails its OWN spread/I4 gate at h_c
isn't a small-number result, it's an undefined one. I should have caught that
myself before reporting 1.19x/1.21x as a real measurement; I didn't check the
no-PT side against its own ergodicity gate before dividing by it. Noted for
my own process, not just yours.

Dispatched Clark on your three-item priority (G2/F6 two-stage rewrite+wire,
G1.3/S4 threshold fix, D4 Ising-line regression). Clark's self-test against
real M2 data (job `15232180`) found the REAL spread/I4 gate flags 8 of 9
matched points as `CAPABILITY_REQUIRED`, not the 6 in your 20ab sec 2 table
(h=0.339574 and h=0.343521 also fail it, not just the 6 you listed). Clark
declined to weaken the gate to match your table and flagged the discrepancy
instead of picking a side. I sent both to Martina for independent
adjudication rather than resolve it myself: **Martina confirmed Clark's code
is a faithful reuse of the pre-existing, already-established spread/I4
machinery** (`per_chain_raw_ratio_spread`, `spread_window_scaling_status`) —
your hand-table was a looser, informal raw-spread read, not the actual
codified stage-1 criterion. So the real gate is stricter than your quoted
table: only h=0.335363 (h_c itself, by coincidence) passes stage 1 cleanly
enough for stage 2 to run at all among the 9 matched points -- meaning even
your 3 "valid-baseline" fast-end points (0.339574/0.343521/0.347399) may not
actually clear the real gate. I have not re-derived the stage-2 numbers under
this corrected gate; flagging rather than asserting, since this changes which
points the 0.51-1.19x range in your 20ab sec 2 table can be trusted at.

D4 self-tested clean: h_c=0.335668+/-0.00483, z=0.128 vs the closed benchmark
0.3359+/-0.0049 -- same method, not a coincidental pass (Martina verified the
extrapolation metric matches). G1.3/S4's z<3 check now actually asserts
(previously computed-but-decorative). Legacy-TSV regression PASS, registry
updated. Grace failed on a genuine NVIDIA HTTP 504 this round (zero
inspection calls) -- proceeding on Martina's review alone, consistent with
her documented unreliability this session.

**M3 is now unblocked** per your own 20ab sec 6 ("M3 waits until item 1 is
done"). Starting the strengthened design (20x sec 6: more sizes, wider L
range, sized to fit an exponent) next, and will fold your corrected stage-1
gate into how any new s* data gets judged rather than reusing the old,
looser reading.

### LOOP CONTAINMENT
Proceeding to M3. Items 4 (C7 wording) and 5/6 (labelling passes on the
NEITHER/WIRED-NOT-VALIDATED rules) from your remediation list remain queued,
not yet started -- free, can run alongside M3.

## RESULT: M3 L16 pilot NO-GO. x-basis PT is architecturally impossible (not transient) -- and I made a real mistake trying to route around it, which an independent readout caught. Correcting the record, not proceeding to the array (2026-07-25)

Designed M3 (5 sizes L={8,12,16,20,24}, up from your existing 3, satisfying
your 20x sec 6 requirement for real exponent-fit redundancy -- 10 nominal
crossing pairs but correctly flagged as correlated, 4 independent
adjacent-pairs, leave-one-size-out stability required). PI chose "design and
submit now"; I followed the design's own staging recommendation and
submitted only the L=16 both-basis pilot, not the full array.

**x-basis PT failed instantly, all 16 ranks, same error every rank**:
`"Parallel tempering requires basis=z and lattice_backend=boost."` I checked
the source (`extended_toric_code_qmc.hpp:3771-3772`): this is a COMPILE-TIME
`if constexpr` check -- PT is only template-instantiated for the z-basis.
This is permanent, not a config issue or transient failure; it would need
real C++ work to lift, not a resubmit.

**My mistake, and how it got caught**: I initially reasoned that since the
z-basis run already records both its direct and plaquette channels, and
those map to x-basis's plaquette and direct channels under this project's
established duality convention, the z-basis-only run could stand in for the
cancelled x-basis arm. **That's wrong, and Jones caught it on the actual
readout**: z-basis's direct and plaquette channels are COMPLEMENTARY
observables WITHIN the same basis, not a dual PAIR with each other -- the
real duality relation is cross-basis (z-direct<->x-plaquette,
z-plaquette<->x-direct). Comparing the two z-basis channels against each
other gave 9.6-75.7 sigma "disagreement" that means nothing -- I had
proposed an invalid test and would have reported it as a pass if Jones
hadn't checked it against `run_manual.md:129`'s own complementary-channels
note. Owning this plainly: I conflated "both channels present in one run"
with "duality checked," and only independent review caught it.

**Consequence: duality is genuinely UNTESTED for this design**, not
satisfied by a workaround, since x-basis PT can't run at all. This is now an
open design question, not something I've resolved: build no-PT x-basis
controls for duality purposes only (accepting whatever ergodicity risk that
carries near s*, possibly severe given the whole reason PT is needed here)?
Waive duality checking for this campaign? Something else? I'd rather you
weigh in than have me pick.

**Other pilot findings, per the design's own 5-leg stop condition**:
traversal PASS (46 strict round trips, full 16/16 coverage) and replica-level
agreement PASS (z=0.83-2.47, all six measurement s-values) both hold clean.
Production-motion acceptance analysis errored on this swap_period/sample_period
combination (tool assumption violated, not investigated further). Capability
gate could only be checked at the 2 s-values with existing no-PT reference
data (0.325, 0.350 -- both correctly CAPABILITY_REQUIRED, confirming the
original PT rationale); the 4 new s-values have no baseline to check against
at all. Two stationarity flags (s=0.325 direct, s=0.340 plaquette) not yet
investigated.

Design revised: dropped the 4 x-basis rows from `design.tsv` (10 jobs -> 5,
roughly halving projected cost to ~19-28 task-hours). **GATE: NO-GO on
releasing L=8/12/20/24** -- per the design's own stop condition, this pilot
does not yet clear it. Full evidence and the specific list of what's missing
are in `status.md`.

### LOOP CONTAINMENT
Not releasing the L=8/12/20/24 array. Awaiting your read on the duality
question in particular -- it's the one open item I don't think I should
resolve by assumption a second time.

