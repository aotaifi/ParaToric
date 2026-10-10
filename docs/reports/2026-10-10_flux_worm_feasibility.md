# A vison worm will not cut the L-growth of the flux autocorrelation at s* (feasibility, 2026-10-10)

**Verdict.** In our sigma^z worldline representation every non-local flux move drags a membrane of flipped
sigma^z whose bare cost is set by the Higgs term: 2*lmbda*<sigma^z> = **0.161 per edge per unit imaginary time**
(derived from measured energies, section 3). With the electric (star) sector frozen during the move, a vison worm is
confined to membranes of area ~1/0.161 ~ 6 cells, independent of L. It can buy a constant factor, not a smaller z.
The one published worm with this structure (Korzec & Wolff, D=3 Z2 gauge) had z = 2.02, the same as local updates.
Odds that a worm gives >10x per CPU at L >= 32: **~10%**; that it cuts the L-growth: **~5%**. Recommendation: do not
build it. If the PI wants the question closed with a measurement, run the 1-day stage-0 test (section 4) first.
The better bet, labelled as a hypothesis: a Swendsen-Wang (Fortuin-Kasteleyn) cluster update for the gauge-Higgs
model in the classical formulation, ~30% odds, ~8 days (section 6). That is a change of formulation and the PI's call.

## 1. Precedents (what was read, and how)

python/poppler are broken on this node, so PDFs were read as extracted text (figures unseen). Workers read most of
them; I verified the load-bearing claims myself as marked.

| paper | read | what matters here |
|---|---|---|
| Korzec & Wolff arXiv:1311.5198 (D=3 Z2 gauge, random-surface rep.) | **in full, by me** | Worm-type update ("Polyakov shift": move a defect line, flip the whole temporal ladder of plaquettes, acceptance t^(L0-2k), Sec. 4.3) plus local cube moves: **z = 2.02** for the total surface (Fig. 1). With the Wolff-type cube cluster (= Wolff for the dual Ising model): z = 0.60 (surface), 1.55 (wrapping). Conclusion: the worm recipe "so far does not appear to carry over to gauge theories" (Sec. 6). Same structure as our vison worm: a moving defect dragging a membrane that costs weight. |
| Delgado Mercado, Gattringer, Schmidt arXiv:1211.3436 (surface worm, Z3/U(1) gauge-Higgs, 4D) | in full (worker) | Wins of 17x to ~6000x (Tables 3-6), all **away from continuous transitions**; loses (0.3-0.8x, data sets Z-2 and U-3) where matter-link flux is costly and closed surfaces dominate, because worms stay short (p. 20). At our MCP the Higgs links are costly: only ~2.5% occupied in the membrane representation (Somoza et al. p. 16), i.e. their failure regime. |
| Somoza, Serna, Nahum PRX 11, 041008 / arXiv:2012.15845 | methods + App. C in full (worker); quotes verified by me | Local plaquette + cube Metropolis only, **z ~ 2.48** at the self-dual point (Sec. VIII B, Eq. 36). "no efficient nonlocal Monte Carlo update ... is known for this problem" (p. 2); "a challenging open problem" (p. 15). Boundary links at the MCP form a percolating fractal, d_f = 1.77(2). |
| Biswas & Damle arXiv:1812.05326 | algorithm sections in full (worker); key quotes verified by me | **The brief's premise conflates two papers.** BD's lesson is about SSE cluster decompositions of dual TFIMs: if the decomposition rule is not consistent along imaginary time, clusters "percolate and freeze"; and whole-imaginary-axis single-site ("canonical 1D") updates are "crucial" for sigma^x (p. 8, Fig. 22). The whole-axis flips are ParaToric's own updates 2/3 (Linsel & Pollet Sec. 4.1). Neither failure mode applies to a worm that uses the exact sigma^z history; the transferable point is: keep updates 2/3. |
| Wu, Deng, Prokof'ev arXiv:1201.6409 | in full (worker) | Our five local updates; no worm, no autocorrelation data; could not resolve the region near h ~ 0.34 at L <= 20. |
| Linsel & Pollet arXiv:2510.14781 | algorithm + autocorrelation sections (worker); update text verified by me | 5 WDP updates + 2 whole-axis flips, all equal probability; no worm or MCP-specific discussion. |
| Yan arXiv:2011.08457 | algorithm (worker) | Sweeping-cluster global loop for the quantum dimer model; acceptance falls fast with L (0.3 at L=4 to 2e-4 at L=10). No speedup claimed. |
| Coyle & Halliday hep-lat/9506010 | in full (worker) | Single-cluster update for the 4D Z2 plaquette model: z = 0.32 vs 2.58 (Metropolis). Works because the model is dual to Ising. |
| Pizzimenti, Duncan, Schweinhart arXiv:2507.13503 (2026) | in full (worker); table values verified by me | Plaquette Swendsen-Wang for 4D Z2/Z3 pure gauge: gauge field resampled as a uniform random cocycle (sparse linear algebra over Z_q). tau_int(E) = 1.72-1.86 for N = 6-16, fitted z_E = 0.078(21) for Z2 (Table 1). No matter fields (future work). Open code: github.com/apizzimenti/ATEAMS. Cost per sweep grows ~V^1.6-2 (App. C.3). |
| Eldridge, Forsstrom, Schweinhart arXiv:2602.22199 | abstract only | Cellular (dependent plaquette percolation) representation of the Potts lattice Higgs model; no algorithm. |

No paper reports a worm or cluster update that beats local dynamics at a continuous transition of a gauge theory
**with matter**. Every near-critical success (z 0.3-0.7) relies on a duality to an Ising/Potts spin model, which the
self-dual gauge-Higgs point does not have (its dual is itself).

## 2. Design: a vison-string worm for this representation

**Configuration.** Edge timelines of sigma^z(e,tau); events: single-edge kinks (weight h, flip one edge), star
events (weight mu, flip the four edges of a vertex). Weight
W = h^Nk mu^Ns exp( int dtau [ J sum_p B_p + lmbda sum_e sigma^z_e ] ). Plaquettes change only at kinks.
Parity constraint per edge: n_kink(e) + n_star(v) + n_star(w) even.

**Why a single-vison move must carry a membrane.** A kink on e flips B on both plaquettes of e, so visons are
created in pairs; moving one vison end by one plaquette needs a sigma^z flip on the crossed edge, and the parity
constraint forces that flip to be paired (a strip of flipped sigma^z in time) or compensated by star events. Either
way the vison worldline is the boundary of a membrane of flipped sigma^z. At lmbda = 0 the membrane is pure gauge and
free (the worm reduces to the Prokof'ev-Svistunov worm of the dual transverse-field Ising model, which is efficient);
at lmbda > 0 it costs exp(-2 lmbda int_M sigma^z).

**Extended ensemble.** Z_ext = Z + eps sum_S rho(S)^-1 Z[S], where Z[S] = Tr[X_S(tau_s) e^{-beta H}] and
X_S = prod_{e in S} sigma^x_e is an open equal-time 't Hooft string along the dual path S from tail plaquette p_t to
head plaquette p_h (it commutes with every star, so it creates exactly two visons). In the configuration, S is a set
of "marked" kinks at the common time tau_s (weight 1, not h), and each edge of S carries an odd number of ordinary
flips, i.e. a strip of flipped sigma^z between tau_s and an ordinary kink.

**Moves** (each with its exact reverse):
- *Open*: pick an edge e (prob 1/N_e) and time tau_s (uniform in beta); add a marked kink at (e, tau_s) and an
  ordinary kink at tau' (uniform in beta). Reverse *close*: when |S| = 1, remove the marked kink and one of the
  n_e ordinary kinks on e (chosen uniformly).
- *Extend* at the head (or tail) across edge e of p_h, e not in S, chosen among the 3 such edges: add a marked kink
  at (e, tau_s) and an ordinary kink at tau' uniform in beta. Reverse *retract*: the last edge of S, removing the marked
  kink and one of the n_e + 1 ordinary kinks on e.
- Detailed balance for the extend/retract pair, with p_ext, p_ret the probabilities of choosing each move type:
  A_ext = min(1, R), A_ret = min(1, 1/R), with
  R = h * beta * rho(S)/rho(S+e) * exp(-DeltaU) * [p_ret / (n_e + 1)] / [p_ext / 3],
  where DeltaU is the change of the integrated diagonal energy from flipping sigma^z(e) on [tau_s, tau'] (cyclic):
  the J terms of the two plaquettes of e and -2 lmbda int sigma^z_e. All are single-edge integrals already computed
  by updates 0/1. Open/close are the |S| = 0 <-> 1 case of the same pair, with eps fixing the open-sector fraction.
- *Shift* of tau_s moves every marked kink and changes the strips on all of S: cost proportional to |S|; optional.
- Measurements only in the closed sector (S empty), as in any worm.

**Ergodicity.** The worm adds moves; all seven existing updates keep running on the closed sector, so ergodicity is
inherited. Per Biswas & Damle and Linsel & Pollet, the whole-axis flips (updates 2, 3) must stay in.

**Coexistence.** Simplest correct scheme: the worm is one more update type in `metropolis_step`; when chosen it runs
from open to close (no cutoff: a cutoff breaks detailed balance unless written as part of Z_ext). The existing updates
never see marked kinks. Checkpoints only in the closed sector.

**What it can and cannot do.** A closed worm cycle composes many single-edge strips coherently, so it moves a vison
pair a distance R and returns it, a move that updates 0/1 can only build through high-energy intermediate states.
But each string edge costs, on top of the vison's own energy, the membrane factor below, which nothing the worm does
can renormalise, because the star (electric) sector is frozen while the worm runs.

## 3. The obstruction, in numbers

| quantity | value | source |
|---|---|---|
| <sigma^z> per edge, L=24, s* | 0.238 | derived: (E - E_mu - E_J - E_h) / (lmbda N_e), from measured means in /scratch/a/A.Otaifi/paratoric/hladder_endpoint_20261010/analysis_runs.json (L24_h0.3391_s0: E = -1228.22, star_x 0.90499, plaquette 0.90239, E_h = -94.23); energy observable = full H (extended_toric_code_qmc.hpp:1025) |
| <sigma^z>, L=16 | 0.237 | same file, L16_h0.3391_s0 |
| <sigma^x> (self-dual partner) | 0.241 (L=24), 0.240 (L=16) | -E_h / (h N_e), same file; agrees with <sigma^z> to 2%, as self-duality requires |
| bare membrane cost 2 lmbda <sigma^z> | **0.161 per edge per unit tau** | derived from the line above |
| sigma^z flip rate per edge (stars at both ends + kinks) | ~1.9 per unit tau | derived: 2 x star_x + N_kink/(N_e beta) = 2 x 0.905 + 0.082 |

With the stars frozen, a vison pair at separation R that lives for a time T ~ R (z = 1 at the critical point) carries
a membrane of area ~R T; its typical weight is exp(-0.161 R T). The worm therefore samples a conditional problem that
is confined at R ~ T ~ 2.5 lattice units, at every L. The physical vison correlator is critical only after summing over
the electric sector: a second-cumulant estimate with the measured flip rate gives a net cost of ~0.05 per cell
(derived, crude: Gaussian, ignores correlations between neighbouring edges), so the cancellation comes from rare
electric configurations that the frozen-star worm cannot reach. This is the same reason the Polyakov-shift worm of
Korzec & Wolff kept z = 2.02: the defect drags a membrane that costs weight.

**Expected payoff.** Moves of linear size b ~ 2-3 instead of ~1 give at best ~b^z ~ 5-20x as a constant (idealised;
wasted worm effort and the per-move overhead reduce it), and no change in the L-growth (per sweep tau ~ L^2.8 between
L = 16 and 24, two sizes only).

## 4. Cost, risks, kill test

**Build cost:** 8-12 days. Unknowns: (a) how updates 0-6 react to a foreign event type in the edge timelines (est.
2 days if they must be taught to skip marked kinks); (b) open-sector bookkeeping and checkpointing (2 days);
(c) validation against L=3 exact diagonalisation (exact_toric.py, ct-heatbath worktree), with a deliberately broken
arm (wrong factor in R) that must fail (2 days); (d) tau per CPU at L = 16, 24, 32 (~300 core-h).

**Stage 0: kill test before any worm code (~1 day, seconds of CPU, no jobs).** A standalone reader for the existing
equilibrium checkpoints at s* (e.g. /scratch/a/A.Otaifi/paratoric/hladder_endpoint_20261010/runs/L24_h0.3391_s{0,1}/
ckpt_rank0.bin and the L=16 pair) computes, for time-like vison loops along dual paths of length R = 1..L/2 and
duration T = 0.25..L/2, the membrane weight -2 lmbda int_M sigma^z, for straight paths (typical cost sigma_typ) and for
the cheapest path found by a shortest-path search over the frozen sigma^z field (sigma_best, the most favourable route
a worm could find). Many placements per snapshot; the electric sector decorrelates in ~1 sample.
- **Fail (do not build):** sigma_best >= 0.03 per cell at both L = 16 and 24, with cost linear in area (R T). The worm's
  reach is then bounded at ~30 cells whatever L.
- **Pass:** sigma_best < 0.01, or cost growing slower than area.
- My prediction: sigma_typ ~ 0.16, sigma_best ~ 0.05-0.10, i.e. fail, at ~75%.

**Stage 1 (only if stage 0 passes and the PI approves the build):** worm on a branch, ED-validated, then tau_int of the
plaquette per CPU-second at L = 16, 24, 32 at s*, ordered and random starts, against the production binary.
Fail: gain at L=24 < 3, or gain(32)/gain(16) < 2 (L-growth not cut).

## 5. Odds

- **>10x per CPU at L >= 32: ~10%.** A constant factor of that size is possible in principle (section 3), but the
  measured membrane cost bounds the reach, the precedent with the same structure gave no change in z, and worm
  overhead eats into a constant.
- **Cutting the L-growth: ~5%.** It would need the frozen-electric-sector membrane cost to be far below the estimate
  (stage 0 would show it).

## 6. The better bet (hypothesis, not on the brief)

**Swendsen-Wang for the Z2 gauge-Higgs model in the classical (Euclidean) formulation** used by Somoza et al. Write
the model with Higgs spins phi on sites and gauge links U; activate each satisfied plaquette with p = 1 - e^{-2K} and
each satisfied Higgs link (phi U phi) with p = 1 - e^{-2J}; then resample (phi, U) uniformly among solutions of the
active constraints, a sparse linear system over GF(2) (the Higgs extension of Pizzimenti et al.'s plaquette SW).
- Why it might work: Edwards-Sokal couplings exist for any Z2 multi-spin model; at the self-dual point the activated
  complex and its dual are statistically equivalent, so its percolation should be critical exactly at the MCP, and
  pure-gauge versions give z = 0.08 (4D, N <= 16) and 0.6-0.73 (3D).
- Why it might fail: no duality to a spin model here, so active 4-spin constraints may percolate and freeze (Biswas &
  Damle's failure mode); sparse elimination costs ~V^1.6-2 per sweep, which at L = 96 could eat a z gain unless the
  solver exploits structure.
- Odds that z_eff <= 1 at the MCP: ~30%.
- Cost: ~8 days. Local classical code (plaquette + cube Metropolis as SSN, 1-2 days, checked against SSN's x_c =
  0.63664 crossing), FK step + GF(2) uniform sampler (reuse ATEAMS's sparse solver, 3-4 days), small-lattice exact
  enumeration check (L = 2, 1 day).
- Kill test: tau_int of S and A at L = 8, 12, 16, 24 on the self-dual MCP vs local (SSN z = 2.48). Fail if
  z_eff(12 -> 24) > 1.5 or gain at L=24 < 10x per CPU.
- Scope: this abandons the quantum (beta = L, continuous time) model for the classical one in the same universality
  class (assumed, standard; Somoza et al. work in the classical model). The goal (nu, x_A) does not need the quantum
  model; the existing quantum data would become a cross-check.

## Not checked

- The stage-0 numbers themselves (the 0.161 is the mean only; sigma_best is predicted, not measured).
- Somoza et al. App. A-B; Delgado Mercado et al.'s worm-length histogram (Fig. 10); the ATEAMS repository.
- Whether the FK representation of the gauge-Higgs model percolates exactly at the MCP (argued from self-duality only).

## 7. Refinement (added 2026-10-10, after PI question): integrate the Higgs term out with Swendsen-Wang bonds

**Idea (hypothesis).** Split sigma^z_e = s_e g_v g_w: s carries the flux (flipped only by kinks), g is the gauge
parity at each vertex (flipped only by star events). The Higgs term lmbda s g g is then an Ising coupling for g in a
background frustrated exactly at the visons (B_p = -1 <=> odd product of s around p). Give it the continuous-time
Edwards-Sokal / Swendsen-Wang decomposition (Rieger-Kawashima style):
- bond points on each edge at Poisson rate 2 lmbda on sigma^z = +1 segments (measured density 2 x 0.3391 x 0.619 =
  0.42 per edge per unit tau, derived from <sigma^z> = 0.238);
- cut points on each vertex timeline = existing star events plus Poisson(mu) extra points;
- clusters = vertex segments joined by bond points; each cluster's g flipped with probability 1/2 (an exact,
  rejection-free update of the electric sector).

**Why it removes the obstruction.** With the bonds fixed, the joint weight is W_m(s) x (bond/cut weights) x
[every bond point satisfied]. A vison move flips s on a strip; violated bond points are repaired by flipping g on one
side of a cut in the cluster graph, at no weight cost. The 0.161-per-cell membrane cost disappears exactly; the vison
worm then sees only h and J (the dual transverse-field Ising weight, where the Prokof'ev-Svistunov worm is the standard
efficient algorithm) plus one rule: a vison worldline may not link a cycle of an electric cluster (the e-m mutual
statistics made literal). Small cycles near the worldline give a perimeter-type cost, not an area law; whether large
critical clusters at the MCP block large vison loops is the open question.

**Exactness.** The bond step is the standard Edwards-Sokal coupling; the worm acts with bonds fixed and the consistency
check is a hard constraint that depends only on the bonds, so worm proposals stay symmetric. Bonds are resampled every
sweep.

**Cost.** Electric-sector SW: 3-4 days. Vison worm with the consistency check: 6-10 days. The engineering risk is the
check itself (is the set of flipped bond points a cut of the cluster graph?); done naively it costs a BFS over the
cluster, which is expensive if a cluster spans the system.

**Kill test (stage 0', ~2 days, no jobs, existing checkpoints).** Build the electric clusters on L=16/24 s* snapshots
(many bond resamplings per snapshot). For time-like vison loops of size R x T, measure the probability that the loop
is consistent (links no cluster cycle). Pass: decay no faster than exp(-c (R+T)) or a power law. Fail: decay linear
in area R T with rate >= 0.03. Also report whether a cluster spans the system at s*.

**Odds (mine):** >10x per CPU at L >= 32, ~25%; cutting the L-growth, ~20%. It stays in the quantum code. Electric-
sector SW alone buys nothing (stars already decorrelate in 15-175 samples vs 2050 for the plaquette).

## 8. Stage 0' result (2026-10-10): PASS. Swendsen-Wang bonds remove the area law of the vison membrane

Tool: tools/es_stage0/ (ckpt.js reader, validate.js, es.js, fit.js) on branch worm/es-stage0. Output:
/scratch/a/A.Otaifi/paratoric/es_stage0_20261010/L{16,24}_s{0,1}.txt (one equilibrium snapshot per run of
hladder_endpoint_20261010, 10 bond resamplings x 200 placements = 2000 loops per cell).

**Checks before the measurement (all can fail and one was made to):**
- Reader: parse ends exactly at EOF; all 1152 edges' flip lists = kinks + both endpoints' star events; stored edge and
  plaquette integrals reproduced to 1e-12; flip-count caches match. A deliberately wrong edge ordering fails
  (1104/1152 edges). Snapshot <sigma^z> 0.241, <B_p> 0.911, star events 0.906 per vertex per unit tau (run means
  0.238, 0.902-0.904, 0.905).
- Cut test agrees with a brute-force bridge test on 300/300 bonds; flags 300/300 random node-set boundaries as cuts.
- Bond density 0.4240 per edge per unit tau vs predicted 2 lmbda P(sigma^z=+1) = 0.4234.

**Result: probability that a time-like vison loop (R plaquettes x T in tau) is consistent**

| R x T | ES L=16 (s0/s1) | ES L=24 (s0/s1) | frozen stars L=24 (s0/s1) |
|---|---|---|---|
| 1 x 1 | 0.925 / 0.933 | 0.924 / 0.920 | 0.667 / 0.690 |
| 4 x 4 | 0.640 / 0.704 | 0.594 / 0.580 | 0.005 / 0.002 |
| 8 x 8 | 0.364 / 0.443 | 0.382 / 0.290 | 0 / 0 |
| 12 x 12 | n/a | 0.209 / 0.124 | 0 / 0 (predicted ~e^-57) |

Fits of log P = a - sA*RT - sP*2(R+T) (weights from binomial errors):

| arm | sA per cell | sP per unit perimeter | chi2/dof |
|---|---|---|---|
| frozen stars (control), L=16 / L=24 | 0.385(5) / 0.393(5) | 0.001(2) / 0.002(1) | 1.3 / 1.6 |
| ES, L=16 s0/s1 | 0.0087(4) / 0.0051(4) | 0.0198(5) / 0.0179(5) | 13.6 / 10.0 |
| ES, L=24 s0/s1 | 0.0053(2) / 0.0094(3) | 0.0207(3) / 0.0215(4) | 17.5 / 15.6 |

- The control arm is a clean area law (chi2/dof 1.3-1.6), so the method sees one when it is there.
- With Swendsen-Wang bonds the area coefficient drops 40-80x, to 0.005-0.009 per cell, below the pre-set kill
  line of 0.03. Model-independent bound: at 12 x 12 (area 144), -log P / area = 0.011 (s0), 0.014 (s1).
- The ES data are not a clean area-plus-perimeter form (chi2/dof 10-17); the coefficients are descriptive only.
- Electric clusters at s* are near critical: largest cluster 7-12% of segments at both L, mean degree 0.86-0.89,
  spatially winding in 0-90% of bond samples. Loops of half the system size: P = 0.36-0.44 (L=16, 8x8) vs
  0.12-0.21 (L=24, 12x12), i.e. roughly L^-2 (two sizes, two snapshots each; derived, rough).

**What this does and does not show.** It shows that, once the electric sector is put into Swendsen-Wang form, the
Higgs-term obstruction to large flux moves is gone (by the pre-set criterion) and the remaining constraint is the
linking rule, which costs order one even for loops of half the system size. It does not show that a worm built on it
decorrelates the plaquette faster: the vison's own weight (h, J) and the coupling between the two sectors through
bond resampling are untested. That is stage 1.

**Stage 1 (needs the PI: it edits production source, on branch worm/es-stage0):** update type 7 = electric SW step
(bond and cut sampling, cluster flips) + vison worm with h/J weights and the cut constraint. Validate on L=3 against
exact diagonalisation with a deliberately broken arm. Then tau_int(plaquette) per CPU at L = 16, 24, 32 vs
production. Fail: gain at L=24 < 3, or gain(32)/gain(16) < 2. Estimate 8-12 days; ~300 core-h for the scaling test.
Odds (revised up from 25%): >10x at L >= 32, ~35%.
