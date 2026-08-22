#!/usr/bin/env python3
"""M7' step 1: exact diagonalization reference for L=3 square open boundaries.

Connectivity below is copied VERBATIM from the verified output of
m4_smoke/dump_l3_open_connectivity.cpp (12 edges, 9 stars, 4 plaquettes, all
distinct, checked before this file was written), not hand-derived.

Hamiltonian, z basis (doc/m1_common_factor_swap_weight_derivation.md section 1,
cross-checked against extended_toric_code_qmc.hpp's diagonal-update code):
  H = - h * sum_e sigma^x_e - mu * sum_v A_v - lmbda * sum_e sigma^z_e - J * sum_p B_p
  A_v = prod_{e in star(v)} sigma^x_e   (off-diagonal in z basis)
  B_p = prod_{e in plaquette(p)} sigma^z_e   (diagonal in z basis)
"""
import numpy as np

N_EDGES = 12

STARS = {
    0: [0, 1],
    1: [0, 2, 3],
    2: [2, 4],
    3: [1, 5, 6],
    4: [5, 3, 7, 8],
    5: [7, 4, 9],
    6: [6, 10],
    7: [10, 8, 11],
    8: [11, 9],
}

PLAQUETTES = {
    0: [0, 3, 5, 1],
    1: [2, 4, 7, 3],
    2: [5, 8, 10, 6],
    3: [7, 9, 11, 8],
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

    # sigma^z_e eigenvalue on state s: +1 if bit e is 0, -1 if bit e is 1.
    bit = np.zeros((dim, N_EDGES), dtype=np.int8)
    for e in range(N_EDGES):
        bit[:, e] = (states >> e) & 1
    sz = 1 - 2 * bit  # shape (dim, N_EDGES), values +-1

    # Diagonal: -lmbda * sum_e sigma^z_e - J * sum_p B_p
    diag = -lmbda * sz.sum(axis=1)
    for p, edges in PLAQUETTES.items():
        Bp = np.ones(dim, dtype=np.float64)
        for e in edges:
            Bp *= sz[:, e]
        diag += -J * Bp
    H[states, states] += diag

    # Off-diagonal: -h * sum_e sigma^x_e (single bit flip)
    for e in range(N_EDGES):
        flipped = states ^ (1 << e)
        H[states, flipped] += -h

    # Off-diagonal: -mu * sum_v A_v (simultaneous flip of all edges in star(v))
    for v, edges in STARS.items():
        m = mask_of(edges)
        flipped = states ^ m
        H[states, flipped] += -mu

    return H


def thermal_mean_energy(H, beta):
    E = np.linalg.eigvalsh(H)
    E0 = E.min()
    w = np.exp(-beta * (E - E0))
    Z = w.sum()
    mean_E = (E * w).sum() / Z
    return mean_E, E0, E.max()


if __name__ == "__main__":
    h, mu, J, lmbda = 0.20, 1.0, 1.0, 0.25
    beta = 8.0
    H = build_hamiltonian(h, mu, J, lmbda)
    assert np.allclose(H, H.T), "Hamiltonian is not symmetric"
    mean_E, E0, Emax = thermal_mean_energy(H, beta)
    print(f"h={h} mu={mu} J={J} lmbda={lmbda} beta={beta}")
    print(f"dim={H.shape[0]}")
    print(f"E_min={E0:.10f} E_max={Emax:.10f}")
    print(f"exact <H>_thermal = {mean_E:.10f}")
