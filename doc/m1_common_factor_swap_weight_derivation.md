# M1 — common-factor (beta, s) swap weight, derived from source

Executor: this session. Independent of Feynman's own derivation (not consulted).

## 0. Ground truth quoted verbatim

`src/mcmc/extended_toric_code_qmc.hpp:1706-1717` (`parallel_tempering_log_weight_ratio`,
basis `z`, Boost lattice only):

```
double log_ratio = 0.;
if (local_h != partner_h) {
    ...
    log_ratio += local_lat.get_beta()
        * (local_single - partner_single)
        * std::log(partner_h / local_h);
}
if (local_lmbda != partner_lmbda) {
    log_ratio += (partner_lmbda - local_lmbda) * (local_edge - partner_edge);
}
return log_ratio;
```

with `local_single = local_lat.get_non_diag_single_energy_z()` and
`local_edge = local_lat.cached_total_integrated_edge_energy()` (and partner analogues).
This function is validated against a brute-force recomputation
(`validate_parallel_tempering_swap_weight_debug`, same file, lines 1758-1783) and that
check is exercised by `boost_pt_swap_weight_matches_bruteforce_l8`
(`tests/mcmc/test_extended_toric_code_qmc.cpp:1268`), which passed in M0. This existing
formula is therefore trusted as ground truth for the reduction check in section 3.

## 1. Sector-to-coupling pairing in the `z` basis, verified from source, not from the plan doc

`src/lattice/lattice.hpp:980-985`:

    "Returns the sum of all single spin flips in the imaginary time axis of the lattice
    divided by beta WITHOUT an overall negative sign. The result is the electric field
    energy term multiplied by h!"
    double get_non_diag_single_energy_z();

`src/lattice/lattice.hpp:996-1002`:

    "Returns the sum of all star flips in the imaginary time axis of the lattice divided
    by beta WITHOUT an overall negative sign. The result is the star energy term
    multiplied by mu!"
    double get_non_diag_tuple_energy_z();

So in the `z` basis: `h` pairs with the *single-flip* off-diagonal accessor,
`mu` pairs with the *tuple/star-flip* off-diagonal accessor. This is cross-checked
against the diagonal-update code, which shows the same pairing from the other side.
`src/mcmc/extended_toric_code_qmc.hpp:1804-1808`
(`integrated_pot_energy_diff_single_spin_flip_edge`):

```
if constexpr (Basis == 'x') {
    delta_energy_single = -h * bare_energy_single;
} else if constexpr (Basis == 'z') {
    delta_energy_single = -lmbda * bare_energy_single;
}
```

and `:1822-1834` (`integrated_pot_energy_diff_single_spin_flip_tuple`):

```
if constexpr (Basis == 'x') {
    ... delta_energy_tuple = -mu * bare_energy; // No "/ 2"!
} else if constexpr (Basis == 'z') {
    ... delta_energy_tuple = -J * bare_energy; // No "/ 2"!
}
```

Reading both together: in the `z` basis, `lmbda` and `J` are the couplings that appear
in the **diagonal** potential-energy update (`-lmbda * edge`, `-J * plaquette`), while
`h` and `mu` are absent from this diagonal code path — confirming they are the
**off-diagonal** couplings in `z` basis, exactly as the accessor doc comments say.
This is the correct pairing:

| coupling | sector (z basis)      | accessor used                                |
|----------|------------------------|-----------------------------------------------|
| `h`      | off-diagonal, single    | `get_non_diag_single_energy_z()`             |
| `mu`     | off-diagonal, tuple/star| `get_non_diag_tuple_energy_z()`              |
| `lmbda`  | diagonal, edge          | `cached_total_integrated_edge_energy()`      |
| `J`      | diagonal, plaquette     | `cached_total_integrated_plaquette_energy()` |

**Correction to the plan document.** Section 3, item 5 of
`doc/coupled_tempering_implementation_plan.md` names
`cached_total_integrated_star_energy()` (`lattice.hpp:747`) as an existing accessor
relevant to this work. That accessor's doc comment (`lattice.hpp:739-744`, "total star
energy... between the imaginary times 0 and beta") pairs with `get_diag_tuple_energy_x()`
(`lattice.hpp:1005-1011`, "star energy... WITHOUT multiplying it by mu") — i.e. it is
the **x-basis diagonal** star accessor. PT runs in `z` basis only
(`extended_toric_code_qmc.hpp:1690-1691` throws otherwise), where `mu` is
**off-diagonal**. The correct accessor for the `mu` sector here is
`get_non_diag_tuple_energy_z()`, not `cached_total_integrated_star_energy()`. This is
exactly the star/plaquette role-swap between bases the plan's own warning flagged;
recorded here as a plan correction rather than silently worked around.

## 2. General two-replica swap, all four couplings independent

Write the Boltzmann weight of a fixed worldline configuration as a product of an
off-diagonal (kink-count) factor and a diagonal (integrated-energy) factor. From the
sign convention read off section 1 (`H ⊃ -lmbda·edge_bare - J·plaquette_bare`, so
`exp(-beta H)` contributes `exp(+lmbda·(integrated edge)+J·(integrated plaquette))`),
and from the accessor doc's "sum of flips / beta = coupling × energy" identity
(so `beta × accessor = N_kink`, the raw kink count, independent of how that count
was generated):

- **Off-diagonal factor**, coupling `g ∈ {h, mu}`: for a fixed configuration with
  `N_g` kinks of that type, `W ∝ g^{N_g}`. Swapping `g` between two replicas at fixed
  configurations gives log-weight change
  `N_g,local · log(g_partner/g_local) + N_g,partner · log(g_local/g_partner)`
  `= (N_g,local - N_g,partner) · log(g_partner/g_local)`.

- **Diagonal factor**, coupling `g ∈ {lmbda, J}`: `W ∝ exp(g · E_g)` with `E_g` the
  bare integrated energy (edge or plaquette). Swapping `g` gives
  `(g_partner - g_local) · E_g,local + (g_local - g_partner) · E_g,partner`
  `= (g_partner - g_local) · (E_g,local - E_g,partner)`.

Applying this to all four couplings, with `N_h = beta·local_single` etc.:

```
log_ratio =
    beta * (local_single - partner_single) * log(partner_h / local_h)          [h,  off-diag]
  + beta * (local_tuple  - partner_tuple ) * log(partner_mu / local_mu)        [mu, off-diag]
  + (partner_lmbda - local_lmbda) * (local_edge       - partner_edge)          [lmbda, diag]
  + (partner_J     - local_J    ) * (local_plaquette  - partner_plaquette)     [J,  diag]
```

with `local_tuple = local_lat.get_non_diag_tuple_energy_z()` and
`local_plaquette = local_lat.cached_total_integrated_plaquette_energy()`
(`lattice.hpp:758`, already exists — plan section 3 item 5 is correct about this one).

## 3. Reduction check (M1 guardrail 1)

Set `mu_local = mu_partner` and `J_local = J_partner` (couplings match across rungs,
today's situation — recall `validate_parallel_tempering_swap_weight_debug` even
enforces `mu = J = 1` for both replicas, `extended_toric_code_qmc.hpp:1742-1743`).

- `mu` term: `log(partner_mu/local_mu) = log(1) = 0` → term vanishes regardless of
  `local_tuple`/`partner_tuple`.
- `J` term: `(partner_J - local_J) = 0` → term vanishes regardless of
  `local_plaquette`/`partner_plaquette`.

What remains:

```
log_ratio = beta * (local_single - partner_single) * log(partner_h / local_h)
          + (partner_lmbda - local_lmbda) * (local_edge - partner_edge)
```

This is character-for-character the existing formula quoted in section 0. **Guardrail
1 passes.**

## 4. Common-factor substitution (M1 guardrail 2)

Let `h = c·h0`, `mu = c·mu0`, `J = c·J0`, `lmbda = c·lmbda0` for each replica, with
`(h0, mu0, J0, lmbda0)` the shared base couplings and `c` the per-rung ladder value
(fixed `beta = 32` for every rung). Then for any coupling pair, `g_partner/g_local =
c_partner/c_local` (off-diagonal ratio) and `g_partner - g_local = (c_partner -
c_local)·g0` (diagonal difference). Substituting into section 2:

```
log_ratio =
    beta * (local_single - partner_single) * log(c_partner/c_local)
  + beta * (local_tuple  - partner_tuple ) * log(c_partner/c_local)
  + (c_partner - c_local) * lmbda0 * (local_edge      - partner_edge)
  + (c_partner - c_local) * J0     * (local_plaquette - partner_plaquette)

  = beta * [(local_single + local_tuple) - (partner_single + partner_tuple)]
        * log(c_partner / c_local)
  + (c_partner - c_local) * [lmbda0*(local_edge - partner_edge)
                              + J0*(local_plaquette - partner_plaquette)]
```

Identify `dN = beta*[(local_single+local_tuple) - (partner_single+partner_tuple)]`
(total kink-count difference, both off-diagonal sectors combined) and
`dE = lmbda0*(local_edge-partner_edge) + J0*(local_plaquette-partner_plaquette)`
(base-coupling-weighted diagonal energy difference, both diagonal sectors combined):

```
log_ratio = dN * log(c_partner/c_local) + (c_partner - c_local) * dE
```

This is exactly the merged two-term form the plan's second M1 guardrail asks for, and
matches the pinning lab's `pt_swap_weight.hpp` pattern structurally (not copied, per
plan section 3 item 8 — only the functional form is compared). **Guardrail 2 passes.**

## 5. Where I am not fully certain

I have not traced the Metropolis kink-insertion/deletion proposal code itself to give
an independent first-principles proof that a fixed configuration's off-diagonal weight
is exactly `g^{N_g}` (the standard continuous-time/SSE identity). My evidence for that
step is: (a) it reproduces the h-sector term that is *already* validated against a
brute-force recomputation in a passing test, (b) the accessor doc comments and the
diagonal-update code cross-confirm the h/mu and lmbda/J pairings from two independent
places in the source, and (c) the mu-sector term is built by exact structural analogy
to the h-sector term, which are symmetric in the `z`-basis code (single ↔ tuple,
h ↔ mu). I did not independently re-derive the kink-count identity from the raw move
acceptance rates. This is the part I would want M2's extended brute-force test to
stress hardest, since it is the one piece of section 2 not directly quoted from
existing, tested code.
