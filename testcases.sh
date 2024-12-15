7#!/bin/bash

RD="\033[0;31m"
GN="\033[0;32m"
NC="\033[0m"

cd src/bin
DATA_PATH="../../SAT_test_cases"
BENCHMARKS=()

BENCHMARKS+=("$DATA_PATH/sat/dp10s10.shuffled.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/rand_net60-30-1.shuffled.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/battleship-6-9-unsat.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/logistics-rotate-07t5.shuffled-as.sat05-1137.dimacs",0)

BENCHMARKS+=("$DATA_PATH/sat/289-sat-6x30.cnf",1)
BENCHMARKS+=("$DATA_PATH/unsat/php-010-008.shuffled-as.sat05-1171.cnf",0)
BENCHMARKS+=("$DATA_PATH/unsat/marg3x3add8.shuffled-as.sat03-1449.cnf",0)
BENCHMARKS+=("$DATA_PATH/unsat/marg3x3add8ch.shuffled-as.sat03-1448.cnf",0)

BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-1.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-2.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-5.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/bmc-ibm-7.dimacs",1)

BENCHMARKS+=("$DATA_PATH/sat/ssa7552-160.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/ssa0432-003.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/ssa2670-141.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/ssa6288-047.dimacs",0)

BENCHMARKS+=("$DATA_PATH/sat/qg3-08.dimacs",1)
BENCHMARKS+=("$DATA_PATH/unsat/qg6-10.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/qg6-11.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/qg6-12.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/qg7-10.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/qg7-11.dimacs",0)
BENCHMARKS+=("$DATA_PATH/unsat/qg7-12.dimacs",0)

BENCHMARKS+=("$DATA_PATH/sat/nqueens_16.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/nqueens_32.dimacs",1)

BENCHMARKS+=("$DATA_PATH/sat/sudoku_9_3_3.dimacs",1)

BENCHMARKS+=("$DATA_PATH/sat/blocksworld.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/blocksworldc.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/4blocks.dimacs",1)

BENCHMARKS+=("$DATA_PATH/sat/logisticsa.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsb.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsc.dimacs",1)
BENCHMARKS+=("$DATA_PATH/sat/logisticsd.dimacs",1)

BENCHMARKS+=("$DATA_PATH/unsat/4_4_1.txt",0)
BENCHMARKS+=("$DATA_PATH/unsat/4_4_2.txt",0)
BENCHMARKS+=("$DATA_PATH/sat/4_4_3.txt",1)
BENCHMARKS+=("$DATA_PATH/sat/4_4_4.txt",1)
BENCHMARKS+=("$DATA_PATH/unsat/9_6_1.txt",0)
BENCHMARKS+=("$DATA_PATH/sat/9_6_2.txt",1)
BENCHMARKS+=("$DATA_PATH/sat/9_6_3.txt",1)
BENCHMARKS+=("$DATA_PATH/sat/9_6_4.txt",1)
BENCHMARKS+=("$DATA_PATH/unsat/9_8_7.txt",0)
BENCHMARKS+=("$DATA_PATH/unsat/16_8_5.txt",0)
BENCHMARKS+=("$DATA_PATH/unsat/16_8_6.txt",0)
BENCHMARKS+=("$DATA_PATH/unsat/16_8_7.txt",0)
BENCHMARKS+=("$DATA_PATH/unsat/16_16_3.txt",0)

BENCHMARKS+=("$DATA_PATH/satlib/hole7_unsat.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/hole8_unsat.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/hole9_unsat.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/uf100-010_sat.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/uuf100-02_unsat.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/uf125-01_sat.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/uuf125-05_unsat.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/uf150-08_sat.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/CBS_k3_n100_m403_b10_1_sat.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/aim-200-3_4-yes1-4.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/aim-200-1_6-no-4.cnf",0)
BENCHMARKS+=("$DATA_PATH/satlib/ii16e2_sat.cnf",1)
BENCHMARKS+=("$DATA_PATH/satlib/ii32e1_sat.cnf",1)


for GET_BENCHMARK in "${BENCHMARKS[@]}"
do
    IFS=, read path correct <<< $GET_BENCHMARK
    #yes | xbutil reset -d 0000:81:00.1
    ./test.real.out workload-hw.xclbin ../configuration.json $path answers.txt $correct

	exitCode=$?
    if [ $exitCode -ne 0 ]
	then
		if [ $exitCode -eq 3 ]
		then
			echo -e "${RD}Not enough on-chip memory, skipping ${NC}"
			yes | xbutil reset -d 0000:81:00.1
			sleep 1
		else
			echo -e "${RD}Error in the program ${NC}"
			exit 1
		fi
	else
		echo -e "${GN}Test passed ${NC}"
	fi	
done
