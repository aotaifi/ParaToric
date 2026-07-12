# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-12 (rewrite this card in place; durable results -> plan/report).

## Status
- Phase: M0 gates G0-G1.3 PASS but "M0 complete" still premature. The sector-rate
  scan v2 (`15034719`) came back with a NULL result (percolation_probability/
  percolation_strength exactly 0.0 across the entire tested s in [0.10,0.50]) —
  reported to Feynman, who traced it to two issues and issued a MASTER PRE-PILOT
  SEQUENCE (see doc/feynman_thesis_tojarvis.md "MASTER PRE-PILOT SEQUENCE" +
  doc/pt_architecture.md), superseding the earlier "finalized next steps". PI
  authorized executing the full sequence 2026-07-12.
- **Separately-discovered blocking bug (D1)**: `swap_period` is defined in raw
  Metropolis moves, not sweeps. Default 1000 moves = ~7.8 sweeps at L=8 (healthy —
  why G1.2/G1.3's L=8 round-trip validation looked fine) but ~0.035 sweeps at
  L=120 (~29 swaps per sweep) — replicas barely evolve between swaps, so
  round-trips there would be label-travel, not real decorrelation. No PT run at
  L>8 is valid until this is fixed. The L=8 G1.2/G1.3 PASS itself is not wrong,
  just does not transfer to L=120 as previously assumed.
- Sequence (full detail in status.md): D1 swap_period-in-sweeps fix (BLOCKING,
  do first) -> validate percolation observable vs Linsel arXiv:2401.08770
  benchmark (no PT, small L, off self-dual line) -> confirm deconfined reservoir
  at small s on the self-dual line -> locate s* (wide-s scan, both directions,
  no pre-committed direction) -> D2 offline PT diagnostics analyzer (strict
  round-trips, tau_ergodic in sweeps, up/down flow) -> D4 generalize PT to
  independent (h,lmbda), validate against Linsel's known Ising line WITH PT ->
  D3 adaptive PM ladder builder (ported from code_ruby_duy_pinning_lab,
  PI-authorized cross-project reference) -> THEN stage the G2 pilot (PI-gated,
  harness hard stop regardless of outcome).
- BLOCKER for M2 (not the pilot): PT+checkpoint composition must be implemented
  before any production ladder — one L=192 point at I4 is 46-91 days/chain,
  impossible without resume.
- Standing rule (all future PT run reports): must include swap_period in
  sweeps, per-link edge acceptance (flag any dead link <1e-4), strict
  round-trips (visited_both_ranks), up/down flow per replica, tau_ergodic in
  sweeps, tau(percolation_strength) at the s* rung. Incomplete reports don't
  gate decisions.
- Next action: D1 (swap_period fix) — dispatched to Clark, Martina review,
  clean-rebuild verification per this session's standing process rule.

## Gate log (append one line per gate decision)
| date | gate | evidence | result | decision |
|---|---|---|---|---|
| 2026-07-07 | pre | code review (chat) + performance_report.md | — | plan approved by user |
| 2026-07-08 | G0.1 baseline reproduction | James dispatch, L=40 beta=80 Boost, seed 24601, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_1_baseline/...` | PASS | acceptance 0.246364 vs baseline 0.24658606 (0.09% rel diff); energy -3326.88+/-5.556 vs baseline -3314.6337+/-3.337 (z=1.89, <3) |
| 2026-07-08 | G0.3 existing tests | James dispatch, `ctest --test-dir build-current-clang-portable` | PASS | 3/3 tests passed (test_lattice, test_flat_square_lattice, test_extended_toric_code_qmc) |
| 2026-07-08 | G0.2 fast-math A/B | James dispatch + Martina verification, L=40 beta=40, seeds 24601-24604, ON vs OFF, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_2_fastmath_ab/...` | PASS (confirmed) | combined z=0 for all observables (energy, anyon_count, sigma_x, star_x, plaquette_z) between ON/OFF. Verified NOT a build no-op: CMakeCache.txt/compile_commands.json confirm -ffast-math present only in ON build, binaries differ (md5 bcf205f.. vs 57f01ae.., cmp differs). Fast-math genuinely has no measurable effect on these observables at this scale -> production can keep FAST_MATH ON |
| 2026-07-08 | cross-project scan | Jones dispatch on /project/theorie/a/A.Otaifi/paratoric_tampered (separate repo, gitlab A.Otaifi/paratoric_tampered) | informational | real MPI PT code exists there (src/mcmc/parallel_tempering.hpp, odd/even MPI_Sendrecv swaps, z-basis kink-count + integrated-energy swap weights) but: (a) does not meet current G1.1-G1.3 harness standards (partial validation only, some OOM/failures), (b) checkpoint/restart only exists in a separate dirty worktree with PT-resume explicitly blocked, (c) current ParaToric is 48 commits past the shared base with templated Boost/flat_square structure the tampered code doesn't have. Verdict: port PT DESIGN (swap formula, MPI pattern) as reference, do not merge files; implement fresh against current structures, checkpoint first per plan order |
| 2026-07-08 | G1.1 checkpoint/restart | Clark implementation + Martina review, L=8 z-basis and x-basis self-dual check, checkpoint@sample10 resume-to-20, commit e461686 | PASS | bit-faithful restart, h5diff exact match on energy/string_number/anyon_count/production_acceptance in both bases. Martina-confirmed: RNG state is full-engine not seed-only, event-vector sort invariant enforced (throws, not silently repaired) on load, checkpoint writes only at sample boundaries (no mid-update tear risk), ctest 3/3 no regression. Known limitation (non-blocking): --snapshots spool state not checkpointed; campaign's planned production runs do not use live snapshots. Automated regression test is weaker (in-memory z-basis only) than the manual h5diff validation performed — acceptable for gate purposes, flagged for future hardening |
| 2026-07-08 | G1.2 swap-weight math (partial) | Clark implementation + 2 rounds Martina review, L=8, two seed/ladder configs, commit b29ff1c | **RETRACTED, see correction below** | cached-vs-brute-force swap weight max abs deviation 1.8e-14 / 1.6e-14 (bar <1e-8), across two distinct configs. Ladder correctly restricted to self-dual line s=h=lmbda (mu=J=1 runtime-enforced), swap hot path confirmed O(1) from cache (no event-vector walk). Round 1 review found 2 real bugs (susceptibility normalization used static config h/lmbda instead of per-replica runtime value -> would have silently misnormalized susceptibility observables under PT; MPI_Init with no MPI_Finalize/return-code checks) + 2 minor (pt_ladder CLI had no validation; test coverage was 1 trajectory) — all 4 fixed and re-confirmed by a second Martina pass. REMAINING: real multi-rank mpirun validation (swap acceptance rate, ladder round-trips, PT-on vs PT-off G1.3 comparison) not yet done — mpirun fails to launch in the Codex dev sandbox (opal_ifinit socket() failure, confirmed a sandbox/network restriction not a code bug by 2 independent reviews); OpenMPI 4.1.1 is installed on the actual cluster at /usr/lib64/openmpi (not on default PATH) — needs a real Slurm srun job to complete |
| 2026-07-08 | **CORRECTION**: G1.2/G0.3 false-pass discovered | Jones clean-rebuild bisection (separate scratch build dirs per commit, not incremental), `/scratch/a/A.Otaifi/paratoric_clean_lattice_20260708_jones` | **FAIL — real regression** | `ctest -R test_lattice` clean-built at commit `7394885` (baseline): PASS. Clean-built at `e461686` (checkpoint only): PASS. Clean-built at `b29ff1c` (+ PT): **FAILS** — `integrated_edge_energy_diff_combination_test_1` gets exactly 0 instead of expected 4 / 6.32 after `insert_single_spin_flip`/`insert_tuple_flip`. Every "ctest 3/3 passed" reported by Clark/Martina AFTER b29ff1c in this session's G1.2 round was a FALSE PASS from a stale/incrementally-rebuilt build-current-clang-portable directory that did not pick up the regression. **This invalidates the G1.2 PASS logged above** — the swap-weight cached-vs-bruteforce validation used the same (apparently broken) cached-energy machinery, so that 1e-14 agreement needs re-verification with a clean build before it can be trusted, not assumed self-consistent. Root cause not yet found — Martina's earlier (pre-this-discovery) hypothesis was that `insert_single_spin_flip`/`insert_tuple_flip` don't route through whatever `set_/add_potential_*` path actually updates the cache, but that needs confirming against what specifically changed in b29ff1c, not assumed. NEXT: Clark to find and fix the actual root cause in b29ff1c's lattice.cpp/.hpp changes, re-validate G1.1 (checkpoint still needs re-checking too, out of caution) and G1.2 swap-weight math from a clean build, get Martina to review with an explicit clean-rebuild requirement this time (not incremental). Process gate added: no gate is logged PASS in this tracker on an incremental ctest run again — clean rebuild only, going forward. |
| 2026-07-08 | G1.2 swap-weight math (re-instated) | Clark fix + Martina independent re-verification (both from separate CLEAN scratch rebuilds), commit `12ebdb8` | PASS | Root cause: b29ff1c repointed public `total_integrated_edge/star/plaquette_energy()` to return the cache instead of recomputing; `insert_single_spin_flip`/`insert_tuple_flip` don't update that cache (only the production move path does), so any caller using the raw insert path (incl. the pre-existing test) got a stale/zero value. Fix: those 3 public accessors call `recompute_total_integrated_*()` again (pre-PT behavior); PT's swap hot path is unaffected, it already calls `cached_total_integrated_edge_energy()` explicitly. TWO independent clean (`rm -rf`+from-scratch) rebuilds both give full `ctest 3/3 passed, 0 failed` (Clark's and Martina's, separate scratch dirs). Swap-weight cached-vs-bruteforce re-measured independently twice post-fix: Clark 6.2e-14/8.2e-14, Martina 4.5e-14/0 — all far under the 1e-8 bar. Known non-blocking item: `total_integrated_edge_energy()` is still called from the `--snapshots`-only `sigma_x_tau_avg_obs` path (extended_toric_code_qmc.hpp), now O(events) instead of O(1) again — this campaign's planned production runs don't use live snapshots, so not on the critical path; flagged for future hardening, not fixed now. |
| 2026-07-08 | G1.2 real multi-rank + G1.3 PT-on/off | James submitted Slurm job `14982619` (spack OpenMPI 5.0.0/GCC 13.2.0, `srun --mpi=pmix`, L=8, 6-rank ladder s={0.22..0.32}, PT-off companion at s=0.26), Jones read back HDF5 results, `/scratch/a/A.Otaifi/paratoric_pt_mpi_validation_20260708/` | **PASS** | Job completed cleanly (exit 0, 1m24s, no invariant-check failures in logs). G1.2: swap-weight cached-vs-bruteforce re-confirmed on real hardware (max 3.6e-13). Real multi-rank transport works — rank1 completes 6 full ladder round-trips, rank2 completes 4 (harness bar: >=3; the harness's actual STOP condition is round-trips, not the acceptance-rate window, which is described as a tuning target). Per-link swap acceptance (5 links): 14.5%, 6.5%, 16.2%, 26.9%, 55.4% — uneven vs the 20-40% target, needs retuning before L=120 (already anticipated in the plan: "actual retuning happens later at the L=120 pilot stage", not a gate failure at this L=8 mechanism-validation stage). G1.3: PT-on (s=0.26 replica) vs PT-off companion (s=0.26), combined-z: energy 0.758, sigma_x 0.792, star_x 0.944 — all well under the z<3 bar. Benign autocorrelation warnings (tau > 0.1*N) expected for a short L=8 mechanism-check run, not a converged physics measurement. **G1.2 and G1.3 both PASS — PT is now genuinely, independently validated on real cluster hardware.** MPI toolchain going forward for any MPI-linked build: spack OpenMPI 5.0.0/GCC 13.2.0 (paths recorded in commit history), NOT system /usr/lib64/openmpi. Separately: an unreviewed, uncommitted stray edit appeared in the live repo during this dispatch (Boost.Log includes replaced by a `#define BOOST_LOG_TRIVIAL(level) std::clog` shim in src/cli/paratoric.cpp and extended_toric_code_qmc.hpp) — Martina-reviewed, verdict: not needed for the default clang++-20 production build (which still compiles/passes 3/3 without it), reverted from the live repo; the real fix (if ever needed) belongs at the CMake level for the MPI build variant specifically, not a live source shim. |
| 2026-07-10 | master pre-pilot sequence + standing PT diagnostics rule | Feynman, folded into doc/feynman_thesis_tojarvis.md ("MASTER PRE-PILOT SEQUENCE" + "STANDING RULE") | DIRECTIVE | Ordered pre-pilot path (supersedes earlier next-steps list): 0) D1 swap_period-in-sweeps [blocking] 1) validate observable vs Linsel h_c=0.335/nu=0.65 no-PT 2) confirm deconfined reservoir + measure tau(percolation) at small-s 3) locate s* both directions 4) D2 offline RT/tau_ergodic/flow analyzer 5) D4 generalize (h,lmbda), PT across Linsel Ising line as correctness check 6) D3 PM grid to bracket s* 7) MCP pilot (PI-gated). STANDING RULE for ALL PT runs (James/Jones): report swap_period-in-sweeps, per-link edge acceptance, STRICT round-trips (visited_both_ranks), visited-low/visited-high flow, tau_ergodic (sweeps), and tau(percolation) at s* — decorrelation is everything; round-trips necessary NOT sufficient; a report without these can't judge a gate. |
| 2026-07-10 | PT architecture + ladder-health spec | Feynman, from ParaToric PT code read + cross-project read of code_ruby_duy_pinning_lab PT/grid (PI-authorized) — see doc/pt_architecture.md | SPEC | KEY FINDING: swap_period is in single MOVES not sweeps — default 1000 = ~8 sweeps at L=8 (why validation looked fine) but ~0.035 sweeps at L=120 (~29 swaps/sweep) = LABEL TRAVEL not decorrelation; must be redefined in sweeps and scaled with L² BEFORE any L=120 pilot. ParaToric PT lacks strict round-trips, tau_ergodic (sweeps), up/down flow, and adaptive grid — all of which coupledpinning already has (build_grid PM/LOGR, strict_roundtrip visited_both_ranks, tau_ergodic). Port order: (1) swap_period-in-sweeps [blocking], (2) offline round-trip/tau_ergodic/flow analyzer [pilot prereq], (3) LOGR grid for M2. KEY DIFFERENCE: coupledpinning tempers beta (rescales kink times); ParaToric tempers s with beta fixed (NO rescaling) — port spacing/scheduling/diagnostics only, not the swap payload. Ladder-health invariants H1-H6 in the doc. |
| 2026-07-10 | percolation=0 diagnosis + Linsel benchmark (CORRECTED convention) | Feynman, re Jarvis finding percolation_probability total 0 — see doc/feynman_thesis_tojarvis.md "RESOLVE FIRST" | GUIDANCE | CONVENTION (Linsel arXiv:2401.08770 Fig.3, extended toric code mu=J=1): POP>0 <=> DECONFINED, POP=0 <=> CONFINED (earlier row had this INVERTED — corrected). percolation_probability (binary winding, locate s*) != percolation_strength (largest cluster/#links = order param P, beta/nu). All-zero POP: s* location NOT established (RETRACTED an earlier "s*<0.22 scan down" guess). Linsel Fig.4a (at T=1/L=beta/L, our aspect ratio!): h_c=0.335(2) at lmbda=0.3, nu=0.65 3D Ising, and the self-dual tip sits near s~0.33 — PLAUSIBLY INSIDE [0.22,0.36], which makes total-zero POP look more like an OBSERVABLE/BASIS BUG than a phase-placement issue. RESOLVE ORDER: (1) benchmark observable vs Linsel Fig.4a (reproduce h_c=0.335, nu=0.65 3D Ising, L=10-30, beta=L, BOTH percolation channels to fix basis) — a deconfined-side point giving 0 = real bug. (2) locate s* by wide-s scan BOTH directions (no pre-committed direction). (3) re-place ladder to bracket s*. Do BEFORE sector scan/spacing/pilot. NOTE PT is self-dual-only (code hardwires runtime_lmbda=runtime_h, qmc.hpp:3716) so it cannot ladder the fixed-lmbda Fig.4a benchmark without a generalization. |
| 2026-07-10 | G2 ladder-design + sector-mixing guidance | Feynman answer to jarvis_report_tofeynman.md (energy_lmbda frozen) — see doc/feynman_thesis_tojarvis.md "Answer to Jarvis's G2 ladder-design question" | GUIDANCE | (1) Snapshot energy_lmbda is the frozen net/cut sector, NOT the swap-weight input (which uses cached_total_integrated_edge_energy, thermalizes fine). Design ladder from DIRECT per-link acceptance of a short trial PT run (io.cpp swap_*_by_link), not snapshot variance. (2) The frozen net-sector = the slow topological/winding mode = the G2 GO metric (percolation_strength). Move-set fact: we have star/plaquette (tuple) moves incl. a global-over-[0,beta] tuple flip, but those are CONTRACTIBLE (don't change winding); no non-contractible loop move (R5, out of scope) and no worm, so the sector changes only by slow accumulation. CORRECTED (PI challenge): that accumulation is slow but NON-zero (tau~1e4 means it flips ~once/1e4), so PT is a FAIR bet (textbook cure for coexistence-limited slowing), not a long shot — do not pre-judge NO-GO. Wide-s sector-rate scan reframed: place ladder endpoints to bracket the fast-mixing window, not "is there hope". Pilot GO needs BOTH swap round-trips AND per-replica sector-change + tau(percolation) 5x drop. Fallback if marginal = WORM (polynomial-cost winding change), user decision — NOT the R5 winding-loop move. |
| 2026-07-10 | G2 wide-s sector-rate scan submission | James/Codex, Slurm array `15012336`, launcher `scripts/submit_g2_sector_rate_scan_20260710.sbatch`, output root `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_20260710` | SUBMITTED | L=120 beta=120 z-basis single-chain scan at s={0.10,0.16,0.22,0.28,0.34,0.40,0.46,0.50}; N_thermalization=1000000000, N_samples=10000, N_between_samples=5000000, N_resamples=200; mu=J=1, h=lmbda=s; array 0-7, 1 CPU/task, mem-per-cpu=1G, walltime=20:00:00, partition=cluster. Slurm DNS/connectivity succeeded from danger-full-access Codex (`squeue -u "$USER"` returned jobs), and one post-submit confirmation showed `15012336_[0-7]` pending with reason `(Priority)`. |
| 2026-07-11 | G2 wide-s sector-rate scan OUTCOME + methodology gap found | Jarvis direct squeue/sacct check + Jones HDF5 readout + Martina code-trace of the accept-path/diagnostics | **INCOMPLETE, needs redesign** | Only 2/8 tasks completed: s=0.10 (15h29m, wall_seconds=55768) and s=0.16 (18h03m, wall_seconds=65012) — real per-attempt cost INCREASES with s (+16.6% wall time for a modest s step, not anticipated when sizing the scan uniformly). Tasks s=0.22..0.50 all hit the 20h walltime and TIMED OUT with ZERO output (no partial/incremental HDF5 writes exist in this code path). More important: Martina's code trace found the scan's premise is unachievable with what was measured — `global_single_spin_flip` (accepted_by_update index 2, exactly 0/7.14e9 in both completed runs) is an ORDINARY single-edge global flip, not a dedicated cut-changing move (an earlier same-session assumption was wrong); the true winding/cut-relevant behavior is a rare wrap-around sub-case buried inside indices 1/3/5's ordinary (11-98%) accept behavior, and **no diagnostic separates that sub-case from the ordinary one** (extended_toric_code_qmc.hpp:1363 `record_update_acceptance()` only increments the aggregate per-index counter; io.cpp:243 only writes that aggregate). This run also used `--full_time_series 0`, so the one thing that could reveal sector changes directly (per-sample anyon_count/percolation_probability series) was never recorded (io.cpp:203 only emits `series` when `full_time_series` is true). Net: ~34 core-hours + ~1 day wall-clock produced move-acceptance statistics but NOT a winding-sector-mixing-rate-vs-s signal. REDESIGN before resubmitting: `--full_time_series 1` + analyze anyon_count/percolation_probability series for sector changes (or add a dedicated wrap-accept counter, more invasive); re-size N_samples/N_between_samples per s given the observed cost increase, not one uniform size across [0.10,0.50]. Flagged to PI/Feynman before another ~20h cycle. |
| 2026-07-11 | G2 wide-s sector-rate scan v2 submission | Codex/James, Slurm array `15034719`, launcher `scripts/submit_g2_sector_rate_scan_v2_20260711.sbatch`, output root `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711` | SUBMITTED | Redesign uses 64 independent single-chain array tasks: 8 s-values `{0.10,0.16,0.22,0.28,0.34,0.40,0.46,0.50}` x 8 chains, task mapping `s_index=SLURM_ARRAY_TASK_ID/8`, `chain_index=SLURM_ARRAY_TASK_ID%8`, seed `7111200 + 100*s_index + chain_index`. L=120 beta=120 z-basis, mu=J=1, h=lmbda=s, `N_thermalization=1000000000`, `N_samples=1500`, `N_between_samples=5000000`, `N_resamples=200`, Boost backend. Corrected signal capture: `--full_time_series 1` and `--observables energy anyon_count percolation_probability percolation_strength`. Resources: 1 node/task, 1 CPU/task, `--mem-per-cpu=1G`, `--time=08:00:00`. Pre-submit `squeue -u "$USER"` succeeded; post-submit `squeue -j 15034719` showed `15034719_[0-63]` pending. |
| 2026-07-12 | G2 wide-s sector-rate scan v2 OUTCOME | Jarvis squeue/sacct monitoring + Jones HDF5 readout (per-sample series analysis), `/scratch/a/A.Otaifi/paratoric_g2_sector_rate_scan_v2_20260711/` | **NULL RESULT — needs Feynman/PI read before proceeding** | 57/64 tasks completed by 05:43 CEST (7/8 s-values with full 8/8 chains: s=0.10-0.46; s=0.50 partial, 2/8 chains, 6 still running with ~3h headroom left on their 8h cap, no timeouts observed this time — the parallel-chain redesign worked as intended for wall-clock efficiency). Signal readout: `percolation_probability`/`percolation_strength` per-sample series are **EXACTLY 0.0 for every completed chain at every s-value tested** (0.10 through 0.46, plus the 2 completed s=0.50 chains) — i.e. zero percolation events anywhere in the entire tested s-range, 84000+ samples analyzed. `anyon_count` series is NOT plateau-like (91-97% of adjacent samples differ, longest constant run 3-4 samples) — confirms it's tracking ordinary local anyon-number fluctuations, not discrete topological/winding-sector jumps, so it cannot serve as the sector-change indicator either. **Net: no winding/cut-sector-change signal was detected ANYWHERE in s in [0.10,0.50] with this measurement.** Two live interpretations, NOT yet distinguished (needs physics judgment, not an engineering call): (a) the system is confined (non-percolating) throughout this entire tested range and the actual multicritical/deconfinement region lies outside [0.10,0.50] on the self-dual line — the scan never got near the transition at all; or (b) a measurement issue (e.g. percolation_probability/strength genuinely needs many more samples than 1500/chain to see even one percolating cluster at this L, or a convention/threshold issue in how it's computed/reported). Flagging to PI/Feynman before deciding next steps (widen/re-center the s-range, investigate the percolation observable's sample-size requirements, or something else) — NOT proceeding to step 2 (ladder tuning) on this data. |
| 2026-07-09 | pilot decisions + budget audit | Feynman review of M0 (commits e461686..4a70c6c) + budget audit; recorded in doc/feynman_thesis_tojarvis.md | RATIFIED | Swap-weight formula hand-verified correct incl. the beta kink-count factor (G1.2 only proved cache==recompute, not physics; G1.3 z<1 + this derivation cover correctness). PI accepted for pilot: PT z-basis-only, PT+checkpoint unimplemented (pilot=one block), beta=120 vs b100 baseline. Prereq before submit: retune L=120 ladder (L=8 ladder ~zero acceptance at L=120), cap N_rep<=30 (1 node, 4 chains). Pilot walltime-capped ~27k samples/chain, ~4.6k core-h, ratio-diagnostic (I4 100*tau bar applies at M2 not pilot). M2 BLOCKER surfaced: PT+checkpoint mandatory before production (L=192 point at I4 = 46-91 d/chain, no resume possible). Ensemble DRIVER still absent (only the analysis script exists) — needed to launch the >=4 pilot chains. |
| 2026-07-08 | G1.2/G0.3 regression fix | Clark clean rebuild from empty `build-current-clang-portable`, clang 20 portable MPI build; `ctest --test-dir build-current-clang-portable --output-on-failure`; non-MPI debug validation driver linked against clean-built `libparatoric_core.a` | PASS (single-process only) | Root cause: b29ff1c changed `total_integrated_{edge,star,plaquette}_energy()` to return caches, while low-level insert helpers mutate event vectors only and production moves update caches separately via `add_potential_*`; direct pre-existing tests therefore saw stale zero totals. Fix: public total accessors recompute again; explicit `cached_total_*()` remains PT hot path. Clean ctest: 3/3 passed in 0.42 s. Fresh cached-vs-bruteforce PT swap-weight deviations: 6.21725e-14 and 8.19345e-14 (max 8.19345e-14 < 1e-8); differs from old 1.8e-14/1.6e-14 but still gate-clean. CLI PT validation remains blocked in sandbox by MPI_Init/OpenMPI socket/PMIx failure, so real multi-rank G1.2/G1.3 still requires Slurm. |

## Baseline anchors (do not re-derive)
- L200 b400 Boost production: 80k samples / 11.8 d; tau(percolation_strength)
  ~2200; Neff ~18 (~1.5 Neff/day). t_sample ~ 12.7 s.
- L120 b100 multicritical region: 50k samples / 3.1 d; tau ~9000-10000 near
  mu~2.2-2.25, h~0.08-0.09; Neff 2-3 (~0.8 Neff/day).
- L1000 b400 Boost hour-scale: 2.12e9 attempts / ~1.4 h, MaxRSS 3.2 GB —
  memory is not the blocker.
- G2 GO bar: PT pilot tau_worst drop >= 5x vs L120 baseline.

## Open questions for M1 — answered (Feynman physics pass, 2026-07-08)

### A1. Self-dual line in code variables (RESOLVED)
- Hamiltonian (from src energy channels): H = -mu*sum_s A_s(σ^x) - J*sum_p B_p(σ^z)
  - h*sum_e σ^x_e - lmbda*sum_e σ^z_e. CLI: --muc/--Jc/--hc/--lmbdac, all
  independent.
- em-(Wegner) self-duality exchanges stars<->plaquettes (mu<->J) and
  σ^x<->σ^z (h<->lmbda). This is EXACTLY what the code's Basis='x' vs 'z'
  templating encodes (two representations of one Z). It is an EXACT operator
  duality (Vidal-Thomale-Schmidt-Dusuel PRB 80 081104), not a single-point
  coincidence.
- => Exact self-dual line is **h = lmbda at mu = J** (take mu=J=1). M1 scan is
  1-parameter s := h = lmbda (mu=J=1); the MCP is the single s* where the
  deconfined phase terminates. PT should ladder ALONG this line (vary s), not
  in (h,mu), so every replica stays exactly self-dual and I2 stays meaningful.
- Numerical guard = invariant I2 (x-basis star obs == z-basis plaquette dual).
  If I2 fails on h=lmbda, the mapping/impl is wrong, not the physics.

### A2. BPV PRB 105,165138 geometry match (RESOLVED — geometry matches)
- BPV (unitary gauge, Eq.6): H=-J*Σσ - kappa*ΣΠ; duality Eq.7
  (J',k')=(-½ln tanh k, -½ln tanh J); self-dual line Eq.9 J+½ln tanh k=0;
  MCP bicritical at (kappa*≈0.7525, J*≈0.2258); first-order segment on the
  self-dual line for 0.688≲kappa≲0.753; two duality-related Ising lines bound
  the topological phase and meet at the MCP. u1=D(J,k) is duality-ODD (=the
  first-order/self-dual direction), u2 duality-EVEN, crossover phi=y1/y2>1,
  latent heat Δ~|u2|^θ.
- Our (h=lmbda) line is the image of BPV's self-dual line under the
  continuous-time quantum->classical map; deconfined phase bounded by two
  em-dual Ising lines meeting at one MCP. GEOMETRY MATCHES. The duality-odd
  direction (off the self-dual line, h-lmbda) is BPV's u1; the tuning along
  the line is u2.
- CAVEAT (anisotropy/z): BPV is ISOTROPIC 3D classical; we are anisotropic
  (2+1)D with beta=L, which ASSUMES z=1. Binder values and crossing locations
  are aspect-ratio dependent, so our U* will NOT numerically equal BPV/XY U*
  unless anisotropy is matched or optimized. Do not compare raw crossing
  numbers to isotropic references without an aspect-ratio correction. If z!=1
  the beta=L ladder is miscalibrated (see risk R3).

### A3. Somoza-Serna-Nahum (PRX 11,041008; arXiv 2012.15845) — what we can /
        cannot reproduce (RESOLVED)
- SSN find the self-dual transition CONTINUOUS (L<=96). Key numbers:
  duality-odd op x_A=1.224, duality-even op x_S=1.506 (~3/2), ν=1/(3-x_S)=
  0.669(4). MC-dynamics z≃2.48 (membrane stochastic dynamics — NON-physical,
  scheme-specific). Fourth exponent = fractal dim d_f of anyon-worldline
  percolation in spacetime.
- CAN reproduce with existing observables:
  * BPV energy cumulants C_nm and SSN cumulant-RG (Var(S)=L^{-2x_S}f) — build
    from per-snapshot energy_h/energy_lmbda/energy_mu/energy_J time series:
    duality-odd A ~ (E_h - E_lmbda), duality-even S ~ (E_h + E_lmbda) on the
    self-dual line. This is our route to x_S and ν.
  * Percolation-based transition location + first-order test — our
    percolation_probability/_strength/largest_cluster (Linsel et al. basis)
    are order-parameter-like and fine for crossings/histograms.
    CORRECTED 2026-07-08 (Fable re-check): these are EQUAL-TIME-SLICE spatial
    clusters (lattice.cpp largest_cluster: union-find over g[e].spin at the
    current slice). SSN's fourth exponent d_f is for anyon-WORLDLINE clusters
    in 2+1D SPACETIME — a 2D slice does not carry the 3D d_f. Quantitative
    reproduction of SSN's d_f needs spacetime cluster code (Q2-adjacent,
    beyond M0). Do not claim the d_f deliverable without it.
- CANNOT reproduce without new code / not applicable:
  * SSN x_A, x_S from real-space 2-pt/3-pt correlators G_A(r),G_S(r) and the
    conformal 3-pt test (their Fig.17): we only have q=0 integrated
    susceptibilities, giving at best a susceptibility-FSS x estimate, not the
    real-space conformal check. GAP if we want to match their decisive test.
  * SSN's MC-dynamics z≃2.48: tied to their local-Metropolis membrane
    dynamics; our continuous-time worldline + PT dynamics is different and
    this z is not a physical exponent — NOT reproducible/meaningful.
  * Emergent-O(2) test (the ACTUAL XY-vs-new-class discriminator): needs
    operator-multiplet/degeneracy structure (currents), not single exponents.
    Our observable set does not expose it. => we cannot separate scenario 2
    from 3 even in principle with the planned observables (see R1).

## Plan risk flags (Feynman physics pass, 2026-07-08)

- R1 (UNDERPOWERED — scenario 2 vs 3): XY and the SSN "new self-dual class"
  are exponent-degenerate at our reach. ν_XY=0.67169(7) vs SSN ν=0.669(4):
  gap ~0.003 (0.4%); x_S(XY)=3-1/ν_XY≈1.511 vs SSN 1.506. Excluding XY via ν
  at 3σ needs σ_ν<=0.001; an L<=192, 6-size, Neff~30 campaign realistically
  gives σ_ν~0.01-0.02 (~10x too coarse). SSN themselves could not exclude XY
  at L<=96 with dedicated methods. => Reframe M3/M4: the DECIDABLE question is
  first-order vs continuous, NOT XY vs new class. Do not promise a
  universality-class verdict on exponent values alone.
- R2 (Binder as XY discriminant is weak here): local operators have large
  scaling dim (SSN) so standard Binder is "ineffective"; our percolation_strength
  Binder tests PERCOLATION universality (d_f), a different fixed-point quantity,
  and is aspect-ratio dependent (R? A2 caveat). Binder crossings separate
  first-order (min diverges) from continuous, but not XY from new class at any
  reachable precision. Keep Binder for the first-order test; drop it from the
  XY-exclusion claim.
- R3 (z=1 assumption unverified): beta=L presupposes z=1. Cheap early check:
  anisotropic FSS / match spatial vs imaginary-time correlation lengths at one
  point before committing the ladder. Our (2+1)D setup can MEASURE z (SSN's
  isotropic model cannot) — a potential value-add, but needs a temporal
  correlator not currently output.
- R4 (winding-loop move — energy-bookkeeping trap, NOT a DB flaw): the move is
  a symmetric involution, so detailed balance holds automatically IF (i) the
  loop is chosen from a configuration-INDEPENDENT distribution and (ii) the
  acceptance uses the correct ΔE. The plan phrase "compose cached O(1)
  global-single-flip diffs along a straight loop" is DANGEROUS: single-edge
  full-period flips each change 2 stars; adjacent loop edges SHARE a star, so
  for a closed straight non-contractible loop every on-line star is flipped
  twice => ALL tuple (mu, in x-basis) contributions CANCEL and ΔE reduces to
  the pure field term ΔE = 2h*Σ_edges ∫_0^β σ^x dτ. Naively summing cached
  single-flip diffs double-counts a spurious star term. Fix: derive ΔE
  analytically with tuple sector = 0; keep G1.4(a) L=8 brute-force check as the
  gate. Also confirm the move actually changes the winding/topological sector
  (is_winding_percolating) or it is not doing its job.
- R5 (winding move DOA at production scale — SHARPENED 2026-07-08): ΔE =
  2h*Σ_e∫σˣdτ has mean ~ 2h*L*β*⟨σˣ⟩; at MCP (h≈0.34, ⟨σˣ⟩=O(0.3-0.5)),
  L=β=120 gives ΔE=O(10³) => acceptance ~ e^{-10³}. Not marginal — dead.
  Usable only at gate sizes (L=8). CONSEQUENCE: the plan's kill-criterion
  rescue "PT+winding move" is illusory; PT is the only real lever. If G2 is
  MARGINAL, the retuning option is the PT ladder, not this move. R4 nuance:
  sequentially composed single-flip diffs WITH cache updates telescope
  correctly; the trap is summing pre-flip cached diffs without applying.

## Sharpened open questions (carry into M0/M1)
- Q1: Decide the M4 claim scope NOW: target = "continuous vs first-order" (in
  reach) + report exponents "consistent with XY/SSN, cannot distinguish". Get
  user sign-off before budgeting L>=128.
- Q2: Will we add real-space A/S correlators (to attempt x_A,x_S like SSN) or
  stay with cumulant-FSS + percolation d_f only? Former is the only path to a
  genuine exponent statement but is new-code scope beyond M0.
- Q3: Confirm z=1 (R3) at one self-dual point before the FSS ladder.
