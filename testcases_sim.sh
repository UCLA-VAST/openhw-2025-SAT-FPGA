#!/bin/bash

RD="\033[0;31m"
GN="\033[0;32m"
NC="\033[0m"

if [ -z "${DATA_PATH}" ]; then
	echo -e "${RD}DATA_PATH is not set. Pass via environment or CMake.${NC}"
	exit 1
fi
BENCHMARKS=()

BENCHMARKS+=("$DATA_PATH/unsat/4_4_2.txt",0)
BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-2.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-5.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/qg6-10.dimacs",0)
BENCHMARKS+=("$DATA_PATH/sat/sudoku_4_2_2.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/sudoku_6_2_3.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/aalto_2.dimacs",0)
BENCHMARKS+=("$DATA_PATH/sat/aalto.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/nqueens_3.dimacs",0)
BENCHMARKS+=("$DATA_PATH/sat/nqueens_4.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/nqueens_8.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/sudoku_9_3_3.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/quinn.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/uf20.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/aim50.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/nqueens_16.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsa.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsc.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsd.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/nqueens_32.dimacs",1)

for GET_BENCHMARK in "${BENCHMARKS[@]}"
do
    IFS=, read path correct <<< $GET_BENCHMARK
    time XCL_EMULATION_MODE=$1 ./test.$1.out workload-$1.xclbin $2 $path $3 $correct

	exitCode=$?
    if [ $exitCode -ne 0 ]
	then
		if [ $exitCode -eq 3 ]
		then
			echo -e "${RD}Not enough on-chip memory, skipping ${NC}"
			sleep 1
		else
			echo -e "${RD}Error in the program ${NC}"
			exit 1
		fi
	else
		echo -e "${GN}Test passed ${NC}"
	fi	
done
