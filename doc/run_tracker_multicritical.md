# Multicritical Campaign Tracker — current-state card

Updated: 2026-07-07 (rewrite this card in place; durable results -> plan/report).

## Status
- Phase: M0 authorized. Thesis T1 ratified, T1-P1 status GO — see
  doc/feynman_thesis_tojarvis.md (2026-07-08). No campaign code changes, no
  campaign runs yet.
- Next action: M0 — checkpoint/restart, then PT along the self-dual line
  (h=lmbda, mu=J=1), then ensembles, then G2 pilot (L=120 ladder). Winding-
  loop move is OUT of M0 scope (R5).

## Gate log (append one line per gate decision)
| date | gate | evidence | result | decision |
|---|---|---|---|---|
| 2026-07-07 | pre | code review (chat) + performance_report.md | — | plan approved by user |
| 2026-07-08 | G0.1 baseline reproduction | James dispatch, L=40 beta=80 Boost, seed 24601, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_1_baseline/...` | PASS | acceptance 0.246364 vs baseline 0.24658606 (0.09% rel diff); energy -3326.88+/-5.556 vs baseline -3314.6337+/-3.337 (z=1.89, <3) |
| 2026-07-08 | G0.3 existing tests | James dispatch, `ctest --test-dir build-current-clang-portable` | PASS | 3/3 tests passed (test_lattice, test_flat_square_lattice, test_extended_toric_code_qmc) |
| 2026-07-08 | G0.2 fast-math A/B | James dispatch + Martina verification, L=40 beta=40, seeds 24601-24604, ON vs OFF, `/scratch/a/A.Otaifi/paratoric_g0_20260708/g0_2_fastmath_ab/...` | PASS (confirmed) | combined z=0 for all observables (energy, anyon_count, sigma_x, star_x, plaquette_z) between ON/OFF. Verified NOT a build no-op: CMakeCache.txt/compile_commands.json confirm -ffast-math present only in ON build, binaries differ (md5 bcf205f.. vs 57f01ae.., cmp differs). Fast-math genuinely has no measurable effect on these observables at this scale -> production can keep FAST_MATH ON |
| 2026-07-08 | cross-project scan | Jones dispatch on /project/theorie/a/A.Otaifi/paratoric_tampered (separate repo, gitlab A.Otaifi/paratoric_tampered) | informational | real MPI PT code exists there (src/mcmc/parallel_tempering.hpp, odd/even MPI_Sendrecv swaps, z-basis kink-count + integrated-energy swap weights) but: (a) does not meet current G1.1-G1.3 harness standards (partial validation only, some OOM/failures), (b) checkpoint/restart only exists in a separate dirty worktree with PT-resume explicitly blocked, (c) current ParaToric is 48 commits past the shared base with templated Boost/flat_square structure the tampered code doesn't have. Verdict: port PT DESIGN (swap formula, MPI pattern) as reference, do not merge files; implement fresh against current structures, checkpoint first per plan order |

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
