# Feynman -> Jarvis Handoff: Thesis T1 and M0 Authorization

Date: 2026-07-08. Context: Feynman physics-attack pass on
doc/multicritical_universality_plan.md, reading phase (no source edits, no
submissions), findings recorded in doc/run_tracker_multicritical.md
("Open questions for M1", A1-A3, R1-R5). This file is the ratified decision
record and the authorization to start M0.

## Thesis T1 (ratified)

The decidable claim for this campaign is **first-order vs continuous** at the
multicritical point on the exact self-dual line h = lmbda (mu = J = 1), not
"which universality class." Reason (R1): 3D XY (nu = 0.6717) and the SSN
self-dual class (nu = 0.669(4)) differ by ~0.4% in nu and are exponent-
degenerate at any L our budget reaches (SSN themselves could not separate
them at L<=96 with dedicated real-space methods; our reach is L<=192 with a
coarser method). Any M4 write-up must reflect this:
- Result = clean first-order proof -> PRL/PRX, decisive.
- Result = continuous -> PRB; report nu/x_S as "consistent with XY and with
  the SSN self-dual class"; explicitly state these are NOT separated by this
  campaign. Do not claim a class selection between scenarios 2 and 3.
- The emergent-O(2) test that would actually discriminate XY from the new
  class needs operator-multiplet structure not in our observable set; out of
  scope for this campaign.

## T1-P1 status: GO

Scope authorized now = **M0 infrastructure only**:
checkpoint/restart -> parallel tempering along the self-dual line
(ladder in s := h = lmbda, mu = J = 1 fixed, so every replica stays exactly
self-dual and invariant I2 stays meaningful) -> ensemble driver (K seeds,
cross-chain tau/R-hat) -> harness gates G0-G1 -> G2 pilot (L=120, beta=120).

**Explicitly excluded from M0**: the winding-loop move (harness G1.4).
R5 finding: at production scale (L=beta~120, h~0.34 at the MCP) its energy
cost is DeltaE ~ 2*h*L*beta*<sigma^x> = O(10^3), so acceptance ~ e^-1000 -
dead, not marginal. Its detailed-balance derivation (R4) stays on file for a
possible small-L equilibration use later, but no engineering time in M0.

### Conditions carried forward (non-blocking for M0 start, binding before M1 production)
- I4 Neff bar stays as written (tau_worst = percolation_strength) through
  G0-G2. Do not rekey the bar to claim-relevant observables' tau before the
  G2 pilot produces real PT tau data to rekey against.
- Tier-0 addition to M1 analysis (no new C++): SSN-style duality-odd/even
  cumulants A ~ (energy_h - energy_lmbda), S ~ (energy_h + energy_lmbda)
  from the existing per-snapshot energy_h/energy_lmbda/energy_mu/energy_J
  series. This is SSN's primary tool and our route to a nu estimate.
- Q3 (z=1 / aspect ratio at beta=L) downgraded to a one-point anisotropic-FSS
  check during M1, not a gate on M0.
- Tracker/plan cleanup (moving A1-A3 out of the run_tracker current-state
  card into the plan doc, per the card's own contract) is **PENDING** -
  tracked separately, not required before M0 starts.

## Authorization

**Jarvis is authorized to begin M0 now**: checkpoint/restart -> PT (along
h=lmbda, mu=J=1) -> ensembles -> G0 gates -> G2 pilot. Winding-loop move is
OUT of M0 scope per R5.

Bootstrap order for Jarvis is unchanged: parent AGENTS.md -> ParaToric
AGENTS.md -> doc/run_tracker_multicritical.md -> doc/multicritical_universality_plan.md
-> doc/multicritical_execution_harness.md. The tracker's "Open questions for
M1" section (A1-A3, R1-R5) is load-bearing for M0 too, since it fixes the PT
ladder direction and removes the winding move from scope - read it before
coding G1.

Provenance: physics attack + this ratification both authored in the Feynman
review chat, 2026-07-08. Final user ratification of the full Q1/winding-cut/
tracker-cleanup triad is still pending at time of writing; this file records
the interim GO specifically for **M0**.
