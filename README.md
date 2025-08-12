# README for A Complete SAT Solver on a U55C FPGA (Team AOHW25-894)

## Project Overview

This project demonstrates accelerating a complete SAT solver on a U55C FPGA. SAT solving is an important workload due to its wide use in different domains such as artificial intelligence, circuit verification, FPGA routing, etc.

## Compilation Instruction


## Run Benchmark Instruction
-Compile openCL executables  
-Change directory into src/bin  
-Enable Host Memory  
-Run testcases.sh  

```sh
cd src/bin
sudo /opt/xilinx/xrt/bin/xbutil configure --host-mem -d <DEVICE_ID> -s 1G ENABLE
../../testcases.sh
```

-A .txt file (which is in CSV format) called answers.txt will be found inside src/bin  
-These are the column description of the csv file:
CNF NAME,SOLVE STATUS,Literals,Clauses,Time (s),Decisions,Conflcits,Restarts,Load Cycle,Load %,Decide Cycle,Decide %,Propagate Cycle,Propagate %,Learn Cycle,Learn %,Min-Btrk Cycle,Min-Btrk %,Save Cycle,Save %,Allocate Cycle,Allocate %,NOT USED,NOT USED,Delete Cycle,Delete %,Literals Checked,Average Propagation Latency,Total Cycles

### To run other SAT instances:
```sh
cd src/bin
./test.real.out workload-hw.xclbin ../configuration.json <YOUR_SAT_DIMACS> <SAVE_METRICS_FILE.TXT> <0(UNSAT)_OR_1(SAT)>
```

-To verify our solver, the SAT instance needed to match other solvers. Therefore, you must know beforehand if it is SAT(1) or UNSAT(0).  
-Note that our solver may not be able to solve the instance due to running out of on-chip memory during the solving process. 
