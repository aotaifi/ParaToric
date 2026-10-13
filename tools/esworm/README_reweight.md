# Reweighting gSW series along the self-dual line

`reweight.js` reweights per-sample series measured at s0 (h = lmbda = s0, mu = J = 1, beta = L) to another s1 on the same line.

The exact log weight ratio of a configuration is

    log W(s1)/W(s0) = n_h ln(s1/s0) + (s1 - s0) int sum_e sigma^z_e dtau
                    = -beta E_h ln(s1/s0) - beta E_lmbda (s1 - s0)/s0.

It uses the per-sample estimators E_h = energy_h = -n_h/beta and E_lmbda = energy_lmbda = -lmbda int sum sigma^z / beta.
This is single-histogram reweighting, with no multi-histogram combination yet.

## Usage
1. Dump each chain's series from its checkpoint, which `interim.js` does as a side effect:
   `node interim.js 0.1 <point>/runs`. It writes `runs/c*/ckpt_series.txt`, columns 0 energy, 1 plaquette_z_tau_avg,
   2 star_x, 3 delta_tau_avg, 4 energy_h, 5 energy_lmbda, 6 sigma_x, 7 sigma_z, 8 star_parity, 9 wilson_x, 10 wilson_y,
   11 winding_x, 12 winding_y.
2. `node reweight.js <point>/runs <beta = L> <s0> <s1> [discard = 0.1]`
   Per chain, it computes reweighted kappa3(S) (S = plaquette + star), Binder kappa4/kappa2^2(delta) and the effective
   sample fraction. It combines them over chains with chain-scatter errors.

## Validation (2026-10-13)
- L=32, from 0.3406 to 0.3399 / 0.3413: kappa3 -5.27(25) / -6.81(51) e-8, against direct runs -5.30(11) / -7.05(18) e-8.
  Binder within 1.3 sigma.
- L=24, from 0.3391 to 0.3376 / 0.3406: within 1.4 sigma.
- The effective sample fraction is 0.08-0.16 at |ds| = 0.0007 (L=32) and falls roughly as exp(-(ds * sigma_E)^2).
  Reweight only within windows whose ESS fraction is >~ 0.1.

## Multi-histogram (WHAM): `mhist.js`
`node mhist.js <beta> <target s> <runs dir> <s> [<runs dir> <s> ...] [--exclude-target] [--direct] [--discard=0.1] [--thin=1]`
combines several points at one L (Ferrenberg-Swendsen, Newton solve of the dual, log-sum-exp, |df| < 1e-10) and predicts <S>,
kappa2(S), kappa3(S), kappa2(delta), Binder(delta) at any s. Errors are a delete-one-chain-index jackknife with a full WHAM
re-solve per group. It assumes equal integrated autocorrelation times at all points. `--exclude-target --direct` is the
leave-one-out test (prediction against the plain chain-scatter cumulants of the excluded point, with z). `--break-dz=d` and
`--break-sign` perturb the weight of input 0 and exist only to test that the checks can fail.

**Validity limit (2026-10-13): the series cannot be used for reweighting in s as they stand.** Z = -beta E_lmbda / s0 is
beta * (sum sigma^z at one time slice), an integer times beta (E_lmbda / s0 = -432.000 in the first L=32 sample), not the
time integral. It is unbiased for <Z> but noisy per configuration: sd(Z) = 2.0e3 against about 6e2 implied by the measured
d<Z>/ds (Var Z = d<Z>/ds - Cov(Z, n_h)/s). Test on L=32, s = 0.3406: Cov(Z, n_h/s + Z) = 3.99e6 against the finite difference d<Z>/ds = 1.9e5 from the
neighbouring points (20 times off), while the same identity for n_h passes (8.29e4 against 8.25e4). The exact weight needs the
tau-integral of sum sigma^z (a tau-averaged sigma^z observable). `reweight.js` has the same problem. `mhist.js` itself is
correct on synthetic data with a known answer (see the commit message).
