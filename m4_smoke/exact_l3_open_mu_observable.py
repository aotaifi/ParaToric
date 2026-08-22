#!/usr/bin/env python3
"""M7' power-check follow-up (Feynman review): exact reference for the mu-conjugate
observable "energy_mu" (= -sum_v A_v in expectation, per
extended_toric_code_qmc.hpp:706-709, z basis: `- lat.get_non_diag_tuple_energy_z()`,
whose ensemble average over a run at coupling mu equals -mu*<sum_v A_v>_thermal by the
standard kink-count identity), at rung A's couplings, both operating points.

Same connectivity as exact_l3_open.py (verified, not hand-derived). Needs full
eigenvectors (not just eigenvalues) since A_v is off-diagonal.
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


def sum_star_operator_expectation(V, E, beta):
    """<sum_v A_v>_thermal via full eigenvectors V (columns), eigenvalues E."""
    dim = V.shape[0]
    states = np.arange(dim, dtype=np.int64)
    OV = np.zeros_like(V)
    for v, edges in STARS.items():
        m = mask_of(edges)
        permuted = states ^ m
        OV += V[permuted, :]
    diag_terms = np.einsum('si,si->i', V, OV)  # <psi_i| sum_v A_v |psi_i>
    E0 = E.min()
    w = np.exp(-beta * (E - E0))
    Z = w.sum()
    return (diag_terms * w).sum() / Z


if __name__ == "__main__":
    h, mu, J, lmbda = 0.20, 1.0, 1.0, 0.25
    H = build_hamiltonian(h, mu, J, lmbda)
    print("diagonalizing rung A with eigenvectors...")
    E, V = np.linalg.eigh(H)

    for beta in [0.5, 0.25]:
        sumAv = sum_star_operator_expectation(V, E, beta)
        exact_energy_mu = -mu * sumAv  # matches energy_mu_obs's convention exactly
        print(f"beta={beta}: <sum_v A_v>_thermal = {sumAv:.6f}   exact energy_mu = {exact_energy_mu:.6f}")
