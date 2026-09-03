# Two ways to read the wrong number off a PT ladder

Both are silent. Neither raises an error. Both produce plausible values.
Recorded 2026-09-03 after they were found within hours of each other.

## 1. Observables at intermediate rungs carry a factor of the common scaling

`BETA` is a single scalar, `lat_spec.beta`, identical on every rung. The ladder never
touches it: a "temperature ladder" is built by scaling all four couplings by a common
factor `c_k`, so rung k carries `(mu, J, h, lmbda) = c_k * (1, 1, s, s)`.

**The sampling is exact.** The Jacobian of the imaginary-time rescaling is `c^N` for N
kink insertions, and it cancels exactly against the `c^N` from the N powers of the
rescaled off-diagonal couplings. Diagonal couplings scale identically because the
tau-integral absorbs the stretch. Two independent derivations agree, and it closes the
gap this project's own `m1_common_factor_swap_weight_derivation.md` section 5 left open.

**But the observables divide by `BETA`, not by `c_k * BETA`.** The kink-count accessors
return `count / BETA` (`lattice.cpp` `recompute_non_diag_single_energy_z`,
`recompute_non_diag_tuple_energy_z`) and `delta_tau_avg` divides its plaquette term by
`BETA` (`extended_toric_code_qmc.hpp:1148`). On rung k the system physically IS the
`(g, c_k * beta)` system. The normalisation is self-consistent per rung but is NOT the
physical-point normalisation.

**Consequence.** Any MAGNITUDE quoted from an intermediate rung -- `sd(delta)`, a Binder
cumulant, a peak-to-dip ratio, an energy -- carries a `c_k`-dependent error. Ratios and
sign fractions are immune, and so is anything counted in samples.

**Nothing on record is affected**, checked rather than assumed: every production ladder
ends at `mu = J = 1.0000000000` with `h = lmbda = s`, and every quoted number is read at
that rung. The only intermediate-rung values ever filed are a hot-rung `tau_sign = 2.5`
and `frac(delta<0) = 0.4907`, both invariant.

**If you ever plot a magnitude against ladder index, divide by `c_k` first.** `c_k` is
recoverable from the stored per-rung `h` as `h_k / s`.

## 2. `world_size - 1` is the physical rung only for an ASCENDING common-factor ladder

`coldrung.py:19` hardcodes `cold = world_size - 1`, and the engine's own PROGRESS line
does the same. That is correct for every ladder this project has run so far, because the
common factor ascends to 1 at the last rung.

**It is wrong for a tilt ladder**, where the physical point `mu = J = 1` sits at the
CENTRE and the last rung is the maximum tilt. Measured on the first `(mu - J)` run,
2026-09-03: rung 18 gave `frac(delta<0) = 0.9968`, rung 9 gave `0.1507`. Reading the
default index returns a confident number from a maximally tilted rung.

Every script downstream of `coldrung.cold_series` inherits this: `aeff.py`, `star2.py`,
`analyse_coldrung_batch.py`, and both plot scripts.

**Before reading any rung, assert which one carries `mu = J = 1`.**

## 3. Related, and worse: `tau_int` on a fixed-rung series is not a relaxation time

Consecutive samples at a fixed rung come from DIFFERENT replicas, so the series can
decorrelate quickly while the underlying physics does not. Measured on the tilt run:
fixed-rung `tau_delta` at the centre was 3.0, while following a single replica's own
trajectory gave 13759 -- a factor of 4600.

To get a physical relaxation time, follow one replica (one rank file) through its own
trajectory across rungs. Whether this makes an ERROR BAR wrong depends on how many
distinct replicas visit the rung; it is dangerous where few do.
