#!/usr/bin/env python3
"""M7' pre-registration (Feynman review): find an operating point where the ladder's
two rungs have exact thermal energies separated by more than the expected QMC error
bar, BEFORE running anything. beta=8.0 at rung A's couplings put the system in its
ground state (E_thermal - E0 ~ 3e-7) -- a common-factor ladder's rungs share
eigenvectors (same H up to an overall multiple), so deep in the ground state the
dominant configurations at every rung are the same and a mis-assigned swap could not
move the measured mean. This scans beta to find where that stops being true.

Connectivity and Hamiltonian convention: see exact_l3_open.py (same STARS/PLAQUETTES,
copied verbatim from the verified dump, not hand-derived).
"""
import numpy as np

N_EDGES = 12

STARS = {
    0: [0, 1], 1: [0, 2, 3], 2: [2, 4], 3: [1, 5, 6],
    4: [5, 3, 7, 8], 5: [7, 4, 9], 6: [6, 10], 7: [10, 8, 11], 8: [11, 9],
}
PLAQUETTES = {
    0: [0, 3, 5, 1], 1: [2, 4, 7, 3], 2: [5, 8, 10, 6], 3: [7, 9, 11, 8],
}


def mask_of(edges):
    m = 0
    for e in edges:
        m |= (1 << e)
    return m


def build_hamiltonian(h, mu, J, lmbda):
    dim = 1 << N_EDGES
    H = np.zeros((dim, dim), dtype=np.float64)
    states = np.arange(dim, dtype=np.int64)
    bit = np.zeros((dim, N_EDGES), dtype=np.int8)
    for e in range(N_EDGES):
        bit[:, e] = (states >> e) & 1
    sz = 1 - 2 * bit

    diag = -lmbda * sz.sum(axis=1)
    for p, edges in PLAQUETTES.items():
        Bp = np.ones(dim, dtype=np.float64)
        for e in edges:
            Bp *= sz[:, e]
        diag += -J * Bp
    H[states, states] += diag

    for e in range(N_EDGES):
        flipped = states ^ (1 << e)
        H[states, flipped] += -h

    for v, edges in STARS.items():
        m = mask_of(edges)
        flipped = states ^ m
        H[states, flipped] += -mu

    return H


def thermal_mean_var(E, beta):
    E0 = E.min()
    w = np.exp(-beta * (E - E0))
    Z = w.sum()
    mean_E = (E * w).sum() / Z
    mean_E2 = (E * E * w).sum() / Z
    var_E = mean_E2 - mean_E * mean_E
    return mean_E, var_E, E0


if __name__ == "__main__":
    base = dict(h=0.20, mu=1.0, J=1.0, lmbda=0.25)

    print("=== diagonalizing rung A (c=1.0) ===")
    Ha = build_hamiltonian(**base)
    Ea = np.linalg.eigvalsh(Ha)

    print("=== diagonalizing rung B (c=1.3) ===")
    Hb = build_hamiltonian(*[1.3 * v for v in base.values()])
    Eb = np.linalg.eigvalsh(Hb)

    print("\nbeta scan at rung A couplings (h=0.20,mu=1.0,J=1.0,lmbda=0.25):")
    print(f"{'beta':>8} {'<H>':>14} {'E0':>14} {'<H>-E0':>12} {'Var(H)':>12}")
    for beta in [8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.1]:
        mean_E, var_E, E0 = thermal_mean_var(Ea, beta)
        print(f"{beta:8.3f} {mean_E:14.6f} {E0:14.6f} {mean_E - E0:12.6f} {var_E:12.6f}")

    # Pick beta=0.5 as the trial operating point: report items 1-3 for it and
    # for beta=0.25 as a second candidate, so Feynman can pick without a re-run.
    print("\n=== candidate operating points: both rungs, separation vs expected QMC error ===")
    for target_Neff in [100.0, 150.0]:
        print(f"\n--- assuming target Neff = {target_Neff:.0f} (achieved for the L=3 open COLD chain at beta=8) ---")
        for beta in [0.5, 0.25, 0.1]:
            mean_A, var_A, E0_A = thermal_mean_var(Ea, beta)
            mean_B, var_B, E0_B = thermal_mean_var(Eb, beta)
            sem_A = np.sqrt(var_A / target_Neff)
            sem_B = np.sqrt(var_B / target_Neff)
            combined = np.sqrt(sem_A**2 + sem_B**2)
            sep = mean_A - mean_B
            sep_sigma = sep / combined if combined > 0 else 0.0
            print(f"beta={beta:5.2f}: exact<H>_A={mean_A:10.4f} exact<H>_B={mean_B:10.4f} "
                  f"separation={sep:8.4f}  predicted_sem_A={sem_A:.4f} predicted_sem_B={sem_B:.4f} "
                  f"combined={combined:.4f}  separation_in_sigma={sep_sigma:7.3f}")
