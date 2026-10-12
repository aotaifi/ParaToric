#!/bin/bash
# usage: time_one.sh OUTDIR NAME L SOLVER NTH NS DV DR LEAF   (s*, beta=L, periodic, gsw-only)
EXE=/scratch/a/A.Otaifi/gsw_cpp_20261011/build_nd/paratoric
RC=/scratch/a/A.Otaifi/gsw_cpp_20261011/nd/run_chain_nd.sh
OUT=$1; NAME=$2; L=$3; SOLVER=$4; NTH=$5; NS=$6; DV=$7; DR=$8; LEAF=$9
hostname > $OUT/$NAME.node
/usr/bin/time -v $RC $EXE $OUT $NAME $L periodic $L 0.3391 0.3391 1 1 $((800+L)) $NTH $NS 1 0 1 1 $SOLVER $DV $DR $LEAF 2> $OUT/$NAME.time
grep -o "gSW summary.*" $OUT/$NAME/run.log | cut -c1-200
grep -o "gSW solver.*" $OUT/$NAME/run.log | cut -c1-400
tail -1 $OUT/$NAME/run.log
grep "Maximum resident" $OUT/$NAME.time
