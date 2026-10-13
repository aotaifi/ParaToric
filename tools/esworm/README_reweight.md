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
