# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-08 (rewrite this card in place; durable results -> plan/report).

## Status
- Phase: M0 in progress. Thesis T1 ratified, T1-P1 status GO — see
  doc/feynman_thesis_tojarvis.md (2026-07-08). G0 gates PASS. Checkpoint/restart
  (G1.1) PASS (commit e461686). PT along self-dual line implemented (commit
  b29ff1c); single-process swap-weight math validated, real multi-rank mpirun
  validation BLOCKED in the Codex dev sandbox (network/socket restriction) —
  needs a real Slurm srun job on the cluster to finish G1.2/G1.3.
- Next action: read Slurm validation job 14981055 after it finishes:
  `/scratch/a/A.Otaifi/paratoric_pt_mpi_validation_20260708`. It stages a
  portable MPI build in scratch, launches 6-rank L=8 PT with
  `srun --mpi=pmix`, and runs a PT-off companion chain for G1.3. Submission was
  observed RUNNING once on 2026-07-08; concise-alert registration was attempted
  but blocked by transient Slurm controller DNS failure during notifier sbatch.
  Then ensemble driver, then G2 pilot (L=120 ladder). Winding-loop move is OUT
  of M0 scope (R5).

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
