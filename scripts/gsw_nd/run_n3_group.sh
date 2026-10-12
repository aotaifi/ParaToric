#!/bin/bash
# Gate N3 group: 4 chains (series, 1 core) of one arm. 2000 therm + 20000 samples, 1 gSW step each, s*, beta=L, periodic, gsw-only
# usage: run_n3_group.sh OUTROOT L ARM    ARM = dense | nd0 (dense_vars=0, sparse all the way) | nd4096 (production: dense_vars=4096, leaf 64)
OUTROOT=${1:?}; L=${2:?}; ARM=${3:?}
EXE=/scratch/a/A.Otaifi/gsw_cpp_20261011/build_nd/paratoric
RC=/scratch/a/A.Otaifi/gsw_cpp_20261011/nd/run_chain_nd.sh
case $ARM in
  dense) SOLV="dense"; K=1;;
  nd0) SOLV="nd 0 0 64"; K=2;;
  nd4096) SOLV="nd 4096 0 64"; K=3;;
esac
hostname
for s in 1 2 3 4; do
  $RC $EXE $OUTROOT/L$L ${ARM}_s$s $L periodic $L 0.3391 0.3391 1 1 $((31000 + 100*L + 10*s + K)) 2000 20000 1 0 1 1 $SOLV
  echo "${ARM} L=$L s=$s rc $?"
done
echo "N3_GROUP_DONE L=$L $ARM"
