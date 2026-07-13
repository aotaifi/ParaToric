# ParaToric Parallel-Tempering Architecture & Ladder-Health Spec

Status: 2026-07-10, pre-testing. Purpose: pin down the PT swapping paradigm and
the diagnostics required to trust the ladder BEFORE any pilot. Authored by
Feynman from a direct read of ParaToric PT code + a cross-project read of the
more mature PT/grid stack in `code_ruby_duy_pinning_lab` (PI-authorized cross-
project comparison). Nothing here is a run result; it is the architecture we
must satisfy.

Cross-references: doc/multicritical_execution_harness.md (gates G1.2/G1.3/G2),
doc/feynman_thesis_tojarvis.md (pilot envelope, worm on hold).

## 0. TL;DR of the open problems

1. **swap_period is defined in single MOVES, not sweeps — quantitatively broken
   at production L.** Default 1000 moves is ~8 sweeps at L=8 (why validation
   looked fine) but ~0.035 sweeps at L=120 (≈29 swaps per sweep) → predominantly
   LABEL TRAVEL, not genuine decorrelation. Must be redefined in sweeps and
   scaled with L². This alone can invalidate a naive L=120 pilot.
2. **No live round-trip / flow / ergodic-time diagnostics.** ParaToric records
   only the raw trajectory; coupledpinning already has strict round-trips,
   tau_ergodic, and up/down flow. Without these, "PT works" is unfalsifiable.
3. **No adaptive ladder (grid mode).** ParaToric ladder is a fixed manual input;
   coupledpinning has PM + LOGR adaptive builders with a quality objective.

## 1. Current ParaToric PT — verified structure

(from src/mcmc/extended_toric_code_qmc.hpp; line numbers 2026-07-10 commit state)

- **Tempered parameter:** s = h = lmbda, mu=J=1 fixed (self-dual line). On
  accept: runtime_h = runtime_lmbda = partner_s (qmc.hpp:3872-3874). Single-
  parameter ladder. NOTE: beta is FIXED across the ladder (unlike coupledpinning,
  which tempers beta) — so NO imaginary-time rescaling on swap is needed here.
- **Scheme:** config-stays / parameter-swaps. The lattice never moves between
  ranks; only s and the ladder index are exchanged (Sendrecv, qmc.hpp:3824-3834).
  A replica's config random-walks in ladder-index space over time.
- **Swap pattern:** odd/even nearest-neighbour, alternating each swap period
  (phase = (step/swap_period)&1, qmc.hpp:3815).
- **Swap weight (qmc.hpp:3855-3857), verified correct by hand-derivation:**
  log_r = beta*(N_local - N_partner)*log(s_p/s_l)      [kink-fugacity term]
        + (s_p - s_l)*(edge_local - edge_partner)        [diagonal integrated pot]
  N = get_non_diag_single_energy_z() (= N_kinks/beta), edge =
  cached_total_integrated_edge_energy(). Structurally identical to the mature
  coupledpinning formula logR = dbeta*dE + dN*log(beta_p/beta) (there tempering
  beta; here tempering s) — good independent cross-check of the FORM.
- **Decision:** lower rank draws u, decides, Bcasts (qmc.hpp:3859-3866). Correct.
- **On accept:** O(N) recompute of integrated_pot_energy (qmc.hpp:3875) — fine,
  swaps are rare.
- **Restrictions (verified):** basis=z only (3667), mu=J=1 only (3677), no
  PT+checkpoint (3673-3675). PT hardwired to the self-dual line (lmbda:=h).

## 2. Diagnostics inventory

| Diagnostic | ParaToric | coupledpinning (reference) |
|---|---|---|
| Per-link (edge) swap acceptance | YES (swap_*_by_link) | YES |
| Global swap acceptance | YES | YES |
| Swap-weight self-check | YES (max_swap_log_weight_deviation, cached-vs-brute) | YES (reverse-DB logR_fwd+logR_rev≈0) |
| Per-replica ladder trajectory | YES (ladder_index_by_sample) | YES |
| **Strict round-trips (visited both ends)** | NO (offline only, from trajectory) | YES (strict_roundtrip_count_local, visited_both_ranks) |
| **tau_ergodic (round-trip time, in SWEEPS)** | NO | YES (tau_ergodic, tau_ergodic_strict_) |
| **Up/down flow f(rung) (n_up/n_dn)** | NO | YES (feedback, bottleneck/nonmono penalties) |
| **Adaptive ladder (grid mode)** | NO (fixed manual input) | YES (build_grid; PM + LOGR adapt_betas) |
| **Per-pair / autocorr-aware swap period** | NO (single global swap_period) | YES (get_swap_period_for_pair, build_grid_autocorr) |

## 3. THE swap_period problem (physics of the runs)

- ParaToric swaps when `total_metropolis_step_count % swap_period == 0`
  (qmc.hpp:3912). One metropolis_step = ONE single local move (one of 7 update
  types on one site/tuple), NOT a sweep. 1 sweep = N_edges = 2*L^2 moves.
- Default swap_period = 1000 moves (types.hpp:118):
  - L=8:   2*64   = 128 moves/sweep  -> 1000 moves = 7.8 sweeps  (HEALTHY; this
    is why the G1.2/G1.3 L=8 validation and its "6/4 round-trips" looked fine)
  - L=120: 2*14400= 28800 moves/sweep -> 1000 moves = 0.035 sweeps (~29 swaps
    PER sweep) -> config barely evolves between swaps = LABEL TRAVEL, not
    decorrelation. The L=8 round-trip counts DO NOT transfer to L=120.
- **Consequence:** a naive L=120 pilot reusing swap_period=1000 would produce
  many "round-trips" with near-zero genuine decorrelation of the physical
  (percolation) observable -> a misleadingly optimistic mechanism signal and no
  real tau reduction. This must be fixed before the pilot.
- **Fix:** define swap_period in SWEEPS and scale with L²: swap_period_moves =
  k * (2*L^2), k = O(1-5) sweeps as a starting point. At L=120 that is ~30k-150k
  moves, NOT 1000. Longer term: port coupledpinning's per-pair autocorr-aware
  schedule (build_grid_autocorr / get_swap_period_for_pair).

## 4. Ladder-health invariants (must all hold before trusting a pilot)

H1 (genuine decorrelation, not label travel): swap_period >= O(1) sweep so real
   local work happens between swaps (Sec 3). Round-trips are NECESSARY but NOT
   SUFFICIENT: a label can round-trip with a frozen config. Verify with H2+H3.
H2 (strict round-trips): count only replicas that genuinely visited BOTH ladder
   ends (coupledpinning "visited_both_ranks"), and require tau_ergodic (in
   sweeps) small enough that several strict round-trips occur in the run.
H3 (reservoir dwell): per strict round-trip, the sweeps spent at the deconfined
   reservoir rungs must exceed the LOCAL decorrelation time there — else the
   config zips through the fast phase without actually decorrelating the slow
   (percolation/winding) mode. This is THE mechanism by which PT helps here
   (inject cheaply-decorrelated toric-phase configs into the near-MCP rung); if
   H3 fails, PT gives round-trips but no physics.
H4 (per-link acceptance): ~20-40% on every link, no dead link (<1e-4). Equal-
   acceptance is the pilot target; NOT necessarily optimal at a first-order MCP
   (see H5).
H5 (first-order bottleneck watch): if the MCP is (weakly) first-order, the two
   rungs straddling s* have bimodal, non-overlapping energy distributions ->
   central-link acceptance collapses and strict round-trips stall THERE even if
   the rest of the ladder is healthy. This is simultaneously a sampling failure
   AND the physics signal for scenario-1 (first-order). Diagnose via per-link
   acceptance hole + energy-histogram bimodality at the central rung; fix needs
   denser spacing there and/or multicanonical weighting, not uniform spacing.
H6 (reach the reservoir): the ladder must extend into the deconfined phase
   (POP>0 rungs). Ties to the deconfined-confirmation benchmark
   (feynman_thesis_tojarvis.md): confirm POP>0 at small s FIRST.

## 5. What to port from coupledpinning (reference, do NOT copy blindly)

Priority order:
1. **swap_period in sweeps** (+ scale with L²). Trivial, blocking, do first.
2. **Strict round-trip + tau_ergodic diagnostics** (worm.cpp:3685-3852 pattern):
   at minimum an OFFLINE analyzer over ladder_index_by_sample producing strict
   round-trip count, tau_ergodic (sweeps), and up/down flow f(rung). This makes
   H2/H3 measurable. Live counters are nice-to-have; offline is enough to gate.
3. **Grid mode (adaptive ladder) — PM / acceptance mode (PI decision D3,
   2026-07-10):** port the PM (acceptance-rate) adapt_betas from
   temp_grid.cpp, renamed to adapt over s (algorithm is axis-agnostic: it only
   reshapes spacing from per-link acceptance p_m; wire it to our s swap-weight).
   PI's empirical finding: acceptance mode refines grids better than LOGR here,
   so PM is the chosen builder (NOT LOGR). Carry the robustness guards WITH it:
   windowed/smoothed p_m, alpha-blend with previous ladder, enforce_min_spacing,
   fixed-iteration cutoff (raw PM is noise-sensitive when swaps/rank are few).
   Do NOT port LOGR unless PM later proves unstable.
KEY DIFFERENCE TO RESPECT: coupledpinning tempers BETA and therefore rescales
kink times on swap (t_new = t*beta_p/beta). ParaToric tempers s with beta FIXED
-> NO time rescaling. Do not port the rescaling. Only the ladder-spacing,
scheduling, and diagnostic logic transfer; the swap payload is different.

## 5b. PRODUCTION ladder = self-dual s-line; D4 is benchmark-only (PI-confirmed 2026-07-12)

- PRODUCTION PT runs on the SINGLE parameter s = h = lmbda (mu=J=1): a 1D ladder
  ALONG the self-dual line, crossing the MCP at once. This is the existing PT
  (self-dual-only) and stays the production path.
- D4's decoupled (h, lmbda) generalization is BENCHMARK-ONLY (run PT across
  Linsel's known fixed-lmbda Ising line for a correctness check). It does NOT
  change production. Reason to keep production 1D: a decoupled 2D (h,lmbda)
  production ladder is impractical to tune; the self-dual line is the natural 1D
  path through the MCP. After D4, production = the h=lmbda special case.
- The s-ladder brackets the MCP with BENIGN phases: deconfined/toric (low s) ->
  MCP (s*) -> confined (high s). No transition is crossed except the MCP itself.
- NUANCE (ladder placement): "benign phase" != "good reservoir for the WINDING
  mode". The winding/topological sector is EXACTLY conserved at s=0 (ideal toric)
  and gets MORE frozen the deeper into the deconfined phase; it mixes FASTEST
  near/just-below s* (strings percolate, not yet critically frozen), and freezes
  again in the confined phase (strings too dilute). So the slow mode is frozen at
  BOTH deep ends and fluctuates only near s* — UNLIKE textbook PT. Therefore:
  (i) CONCENTRATE rungs near/just-below s*, do NOT reach far into deconfined;
  (ii) VERIFY the reservoir mixes (measure tau(percolation) at the low-s rungs).
  s* IS NOT YET MEASURED (that is the "locate s*" step). UNITS TRAP: Tupitsyn/BPV
  quote the MCP in CLASSICAL gauge-Higgs couplings (lambda_bond*≈0.226) — NOT our
  quantum field. Linsel (arXiv:2401.08770) = our quantum model: self-dual MCP
  ~ s*≈0.33-0.35 (their Fig 3; precise value left as future work = our target).
  Best prior in OUR units: s*≈0.33-0.35, NOT 0.226 (classical) and NOT the earlier
  mis-derived 0.31/[0.15,0.25]/[0.22,0.30] (all thrash — superseded). So the useful
  reservoir "just below s*" is ~s in [0.28,0.34]; the dense part of the ladder sits
  ~0.30-0.34. The s=0.22 convergence point is DEEPER deconfined (valid reservoir,
  not optimal). MEASURE s* before finalizing the ladder.
- First-order (H5) still applies: a first-order MCP gives a central-rung
  acceptance bottleneck on the s-ladder; the s-line is still correct, diagnostics
  (per-link acceptance, strict round-trips) catch it.

## 6. Decisions (PI-ratified 2026-07-10)

- D1 [AGREED]: swap_period in sweeps, scaled with L². Blocking, do first.
- D2 [AGREED]: build the offline round-trip/tau_ergodic/flow analyzer as a PILOT
  PREREQUISITE (H2/H3 must be measurable or "PT works" is unfalsifiable).
- D3 [DECIDED — PM/acceptance mode]: port PM adapt_betas (acceptance-rate) to s,
  with robustness guards; NOT LOGR (PI empirical: acceptance mode refines best).
- D4 [RECOMMEND YES, scoped]: generalize PT to independent (h, lmbda) ladders so
  a fixed-lmbda (vary-h) ladder can run PT ACROSS Linsel's KNOWN 3D Ising line
  (lmbda=0.3, h_c=0.335(2), nu=0.65) at L=20-30.
  RATIONALE: the recurring lesson this session is that L=8 validations do NOT
  transfer (swap_period is the poster child — clean at L=8, broken at L=120). Our
  ONLY PT-correctness check is G1.3 (PT-on=PT-off at an L=8 EASY point). We should
  not trust PT at the UNKNOWN MCP when we've never checked PT near criticality at
  moderate L against a KNOWN answer. The self-dual line has NO known critical
  point (its transition IS the unknown MCP), so this check REQUIRES going off the
  self-dual line — hence the generalization. Cost is low: the swap weight already
  has both terms (kink-fugacity in h, integrated-edge in lmbda); generalizing =
  let h and lmbda ladder INDEPENDENTLY (pass them separately instead of s:=h=lmbda)
  and the current self-dual case becomes the special case h=lmbda. Mostly
  plumbing, no new physics. NOT for benchmarking PT "acceleration" (the Ising
  line is easy, uninformative about the hard MCP) — strictly a CORRECTNESS-at-
  scale check against ground truth.
  SEQUENCING: after (observable validated + swap_period fixed), reuse the same
  Linsel lmbda=0.3 benchmark point, before the MCP pilot. If PT+FSS recovers
  h_c=0.335 and nu=0.65 there, PT is trustworthy for the MCP; if not, we found a
  PT bug where we know the answer.

--------------------------------------------------------------------------------
## 7. CRITICAL BUG: swap-partner selection by RANK (must fix before any PT) — 2026-07-13

STATUS: confirmed in code; PT mechanism at N>2 ranks is BROKEN. Found by Jarvis
(15-rank D4 benchmark: 0/N strict round-trips, crossing links <1e-3, isolated
healthy spikes). Swap-WEIGHT formula is fine; the PARTNER SELECTION is wrong.

THE BUG (qmc.hpp swap loop): partner = (pt_rank%2==phase)?pt_rank+1:pt_rank-1 —
pairs by MPI RANK — while on accept the rank adopts partner's h/lmbda/ladder_index
(labels MIGRATE, config stays). After one accepted swap, adjacent RANKS hold
non-adjacent LADDER POSITIONS, so odd-phase swaps try to exchange the extreme-low
and extreme-high labels (huge s-gap -> rejected). The ladder SEVERS at every
odd-phase boundary: labels trapped in {0,1},{2,3},... -> 0 round-trips. Invisible
at 2 ranks (one pair, no gap forms); first exposed at N>=3.

RETRACTION: G1.2/G1.3's multi-rank "round-trips" (job 14982619, 6-rank: "6 and 4")
are SPURIOUS under this bug (6 ranks => severed => 0). PT was never validated at
N>2. Re-validate after the fix.

CORRECT ALGORITHM (port from code_ruby_duy_pinning_lab/worm.update.cpp ~L375-410,
the tested reference). Before EACH swap phase, on all ranks:
  MPI_Allgather(my ladder_index) -> ladder_index_by_rank[0..N-1]
  build rank_of_ladder_index[pos]=r ; ABORT on duplicate/invalid (permutation
    integrity — coupled-pinning MPI_Aborts here)
  i = my ladder_index          (NOT my rank)
  j = (phase==0) ? (i even? i+1:i-1) : (i odd? i+1:i-1)   // pair by POSITION
  if j not in [0,N-1]: no partner this phase (endpoint)
  partner_rank = rank_of_ladder_index[j]
  ...then Sendrecv (h,lmbda,index,energies) + log_r + migrate label, AS NOW.
Only the partner-resolution changes; keep the weight/migration. (Alternative =
Approach A: fix labels per rank, migrate the CONFIG — rejected: needs sending the
full lattice each accept; config-stays + Allgather is cheaper and is what the
reference does.)

SMOKE SUITE (ALL at N>=3 ranks; 2 ranks can NEVER expose this; equal-s ladders
also hide it since every swap then accepts — use a SPREAD ladder):
  S1 Permutation integrity: ladder positions across ranks == {0..N-1} every phase.
  S2 Partner correctness (unit): hand-built scrambled permutation => partner is the
     holder of position i+/-1, not rank i+/-1.
  S3 Full traversal (killer): spread 4-8 rung ladder; every replica visits BOTH
     ends and accumulates strict round-trips. Buggy severs; fixed passes.
  S4 Detailed balance (G1.3 at N>2): PT-on marginal per rung == PT-off single chain
     at that s, z<3.
  S5 Acceptance shape: healthy band on ALL links; the near-zero-crossing +
     isolated-spike pattern (bug fingerprint) must be gone.
  S6 RT consistency: strict round-trips from ladder_index_by_sample == independent
     recompute.
GATE: fix + S1-S6 pass + G1.2/G1.3 re-run at N>=3 BEFORE any further PT (D4, pilot).

## 7b. Smoke suite — CONCRETE specs (hand to Clark) — 2026-07-13

Each test: setup / procedure / PASS. Unit tests need no MPI; integration tests
need N>=3 ranks and a SPREAD ladder (equal-s hides the bug; N=2 cannot expose it).
Reference ladder for L=8, N=4: s = {0.22, 0.26, 0.30, 0.34} (moderate spread,
~decent per-link overlap; retune if any link <5%). Diagnostics used:
ladder_index_by_sample (per rank), swap_{attempted,accepted}_by_link (keyed by
ladder position link_index=min(local_index,partner_index) — already correct).

S0 REGRESSION (guard the fix): 2-rank G1.2 (swap-weight <1e-8) and G1.3 (PT-on==
   PT-off, z<3) still PASS unchanged. PASS: both still pass.

S1 PERMUTATION INTEGRITY.
   (a) Unit/fault-injection: call the validator on ladder_index_by_rank =
       [0,1,1,3] (dup) and [0,1,5,3] (out-of-range) => MUST throw/abort; on
       [0,1,2,3] and [2,0,3,1] => MUST pass.
   (b) Integration: N=4 spread, >=2000 swap attempts => NO false abort.
   PASS: (a) aborts iff invalid; (b) healthy run never aborts.

S2 PARTNER CORRECTNESS (pure unit; extract partner_rank(my_rank,
   ladder_index_by_rank, phase) as a testable free function).
   Cases (assert exact):
     [0,1,2,3] phase0: partner(0)=1, partner(2)=3.
     [0,1,2,3] phase1: partner(1)=2; partner(0)=NONE, partner(3)=NONE (endpoints).
     [1,0,3,2] phase1 (THE bug case): partner(0)=3, partner(3)=0;
        partner(1)=NONE, partner(2)=NONE.  (rank0 holds pos1, needs pos2's holder
        = rank3. The OLD rank+/-1 logic returns 1 here => must FAIL old, PASS new.)
     [2,3,0,1] phase0: partner(2)=3, partner(0)=1.
   PASS: all match; include an xfail/assert documenting the old logic fails
   [1,0,3,2] phase1.

S3 FULL TRAVERSAL (killer integration; N=4 AND N=8, spread ladder, ~5000 swap
   attempts). Record ladder_index_by_sample per rank.
   PASS: for EVERY rank, (max_index_visited - min_index_visited) == N-1 (full
   span) AND total strict round-trips >= N over the run.
   FAILS on buggy code: each rank's span == 1 (confined to its pair), 0 round-trips.

S4 DETAILED BALANCE / PHYSICS (integration, N=4 spread). Reorganize PT-on samples
   by ladder_index; compare each position's marginal (energy + one more obs) to an
   independent PT-OFF single chain run at that s (4 chains).
   PASS: z<3 for every position and observable.

S5 ACCEPTANCE SHAPE (from the S3 run). Per-link acceptance (by ladder position)
   for all N-1 links.
   PASS: min link acceptance > 5% (target 20-40%); NO near-zero-link-amid-healthy
   pattern (the bug fingerprint).

S6 RT-ANALYSIS CONSISTENCY. strict round-trip count from scripts/pt_diagnostics.py
   == an independent recompute on the same ladder_index_by_sample.
   PASS: exact match. (Guards against the analyzer miscount that made the old
   6-rank "round-trips" look fine.)

GATE (unchanged): S0-S6 all PASS on a CLEAN rebuild + independent review, AND
G1.2/G1.3 re-run at N>=3, BEFORE any further PT (D4, D3, pilot).
