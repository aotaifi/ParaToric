#!/bin/bash
# usage: run_test.sh OUTFILE NSYS NSAMP SEED [STYLE [FILTER]]   compiles and runs the N1 test
G=/project/theorie/a/A.Otaifi/paratoric_worktrees/gsw-nd
O=/scratch/a/A.Otaifi/gsw_cpp_20261011/nd
OUT=$1; shift
EXE=$O/test_gf2_exact_$$
/usr/bin/clang++ -std=c++20 -O2 -Wall -Wextra -I$G/src $G/tests/gsw_nd/test_gf2_exact.cpp -o $EXE || exit 1
hostname > $OUT
$EXE "$@" >> $OUT
echo "exit $?" >> $OUT
rm -f $EXE
