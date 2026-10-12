#!/bin/bash
# Gate N2: G1 (L=3 open, C++ gSW vs ED) with --gsw_solver nd; args OUTROOT NPAR DENSE_VARS DENSE_ROWS LEAF
# (sparse nested-dissection elimination all the way, ordering with real recursion even on this tiny system).
# usage: run_g1_nd.sh OUTROOT NPAR
OUTROOT=${1:?}; NPAR=${2:-1}; DV=${3:-4096}; DR=${4:-0}; LEAF=${5:-64}
EXE=/scratch/a/A.Otaifi/gsw_cpp_20261011/build_nd/paratoric
RC=/scratch/a/A.Otaifi/gsw_cpp_20261011/nd/run_chain_nd.sh
mkdir -p "$OUTROOT"
JOBS="$OUTROOT/jobs.txt"; : > "$JOBS"
declare -A P=( [A]="1 0.3391 0.3391 1 1" [B]="4 0.3391 0.3391 1 1" [C]="2 0.6 0.25 1.2 0.8" [D]="3 0.15 0.9 0.6 1.5" )
for pt in A B C D; do
  read -r beta h lm mu J <<< "${P[$pt]}"
  for s in 1 2 3 4 5 6 7 8; do
    seed=$((9000 + 100 * s + $(printf '%d' "'$pt")))
    echo "$EXE $OUTROOT/$pt gswonly_s$s 3 open $beta $h $lm $mu $J $seed 2000 200000 1 0 1 1 nd $DV $DR $LEAF" >> "$JOBS"
    echo "$EXE $OUTROOT/$pt gswlocal_s$s 3 open $beta $h $lm $mu $J $seed 2000 100000 24 12 0 1 nd $DV $DR $LEAF" >> "$JOBS"
    echo "$EXE $OUTROOT/$pt broken_s$s 3 open $beta $h $lm $mu $J $seed 2000 200000 1 0 1 0.5 nd $DV $DR $LEAF" >> "$JOBS"
  done
done
xargs -P "$NPAR" -L 1 "$RC" < "$JOBS"
n_ok=$(grep -l "rc 0$" "$OUTROOT"/*/*/run.log | wc -l)
echo "N2_CHAINS_DONE ok=$n_ok of $(wc -l < "$JOBS")"
