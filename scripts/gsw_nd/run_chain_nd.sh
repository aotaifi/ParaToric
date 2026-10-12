#!/bin/bash
# One paratoric etc_sample chain with the gSW update and a chosen gSW solver (copy of scripts/gsw/run_chain.sh plus solver args).
# usage: run_chain_nd.sh EXE OUTDIR NAME L BC BETA H LMBDA MU J SEED NTHERM NSAMPLES NBS GSW_EVERY GSW_ONLY JFACTOR SOLVER [ND_DENSE_VARS ND_DENSE_ROWS ND_LEAF]
set -u
EXE=$1; OUT=$2; NAME=$3; L=$4; BC=$5; BETA=$6; H=$7; LM=$8; MU=$9; J=${10}; SEED=${11}
NTH=${12}; NS=${13}; NBS=${14}; GE=${15}; GO=${16}; JF=${17}; SOLVER=${18}
DV=${19:-4096}; DR=${20:-0}; LEAF=${21:-64}
OBS="energy plaquette_z_tau_avg star_x delta_tau_avg energy_h energy_lmbda sigma_x sigma_z"
mkdir -p "$OUT/$NAME"
start=$(date +%s.%N)
"$EXE" --simulation etc_sample --basis z --lattice_type square --system_size "$L" --boundaries "$BC" --beta "$BETA" \
  --mu_constant "$MU" --J_constant "$J" --h_constant "$H" --lmbda_constant "$LM" --h_constant_therm "$H" --lmbda_constant_therm "$LM" \
  --N_samples "$NS" --N_thermalization "$NTH" --N_between_samples "$NBS" --N_resamples 10 --custom_therm 0 --seed "$SEED" --default_spin 1 \
  --observables $OBS --output_directory "$OUT" --folder_name "$NAME" --snapshots 0 --full_time_series 1 --process_index 0 \
  --gsw_every "$GE" --gsw_only "$GO" --gsw_j_bond_factor "$JF" \
  --gsw_solver "$SOLVER" --gsw_nd_dense_vars "$DV" --gsw_nd_dense_rows "$DR" --gsw_nd_leaf "$LEAF" > "$OUT/$NAME/run.log" 2>&1
rc=$?
end=$(date +%s.%N)
echo "wall_seconds $(echo "$end - $start" | bc -l) rc $rc" >> "$OUT/$NAME/run.log"
exit $rc
