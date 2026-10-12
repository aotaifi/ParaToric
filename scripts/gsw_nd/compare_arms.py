#!/usr/bin/env python3
"""Compare the observable means of chain groups (arms) against a reference arm, z from the chain-to-chain scatter.

usage: compare_arms.py REF_LABEL ARMDIRGLOB [ARMDIRGLOB ...]   (arm label = basename prefix before _s<seed>)
Each chain dir holds obs.h5. For every observable: mean over chains, standard error from the scatter (ddof=1),
and z = (mean_arm - mean_ref) / sqrt(se_arm^2 + se_ref^2).
"""
import glob
import re
import sys

import h5py
import numpy as np

names = ["plaquette_z_tau_avg", "star_x", "energy", "delta_tau_avg", "energy_h", "energy_lmbda"]
ref_label = sys.argv[1]
arms = {}
for pat in sys.argv[2:]:
    for d in sorted(glob.glob(pat)):
        lab = re.sub(r"_s\d+$", "", d.rstrip("/").split("/")[-1])
        arms.setdefault(lab, []).append(d)
stats = {}
for lab, dirs in arms.items():
    stats[lab] = {}
    for n in names:
        m = []
        for d in dirs:
            with h5py.File(d + "/obs.h5", "r") as f:
                m.append(float(np.asarray(f["simulation/results"][n]["series"][()]).real.mean()))
        m = np.array(m)
        stats[lab][n] = (m.mean(), m.std(ddof=1) / np.sqrt(len(m)), len(m))
print(f"# reference arm {ref_label}")
zmax = {}
for lab in arms:
    for n in names:
        mu, se, k = stats[lab][n]
        if lab == ref_label:
            print(f"{lab:10s} {n:22s} {mu:13.8f} +- {se:9.2e}  (chains {k})")
        else:
            mr, sr, _ = stats[ref_label][n]
            z = (mu - mr) / np.hypot(se, sr)
            zmax[lab] = max(zmax.get(lab, 0), abs(z))
            print(f"{lab:10s} {n:22s} {mu:13.8f} +- {se:9.2e}  (chains {k})  z vs {ref_label} = {z:+.2f}")
for lab, z in zmax.items():
    print(f"VERDICT {lab} vs {ref_label}: max|z| = {z:.2f}")
