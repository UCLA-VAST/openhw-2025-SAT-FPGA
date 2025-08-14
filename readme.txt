Team Number: aohw25_894
Project Name: SAT Acceleration
Link to Youtube Video: https://www.youtube.com/watch?v=hmSU6J7Zo6w
Link to Project Repository: https://github.com/UCLA-VAST/openhw-2025-SAT-FPGA

University Name: University of California Los Angeles
Participants: Michael Lo, Jake Ke
Email: milo168@g.ucla.edu jakeke@g.ucla.edu
Supervisors: Jason Cong, Tony Nowatzki
Email: cong@cs.ucla.edu tjn@cs.ucla.edu

Board Used: Alveo u55c
Software Version:
-xilinx_u55c_gen3x16_xdma_3_202210_1
-Vitis HLS 2022.2
-XRT 2.14.384

Brief Description of Project:
The Boolean satisfiability (SAT) problem is a fundamental problem in computer science and is the first problem shown to be NP-complete. 
SAT solvers have been widely used in many applications such as circuit verification, FPGA routing, program synthesis, quantum computing compilation, and are now being used in the emerging field of Explainable AI.
This project is a complete on-chip only SAT solver implementing First Unique Implication Point (1-UIP) clause learning, (Variable State Independent Decaying Sum) VSIDS decision heuristic, recursive clause minimization, Literal Block Distance deletion scoring, phase saving, Luby restart.

Description of archive:
SAT_test_cases: set of benchmarks used for this project categorized into SAT or UNSAT
src: source files used to implement our SAT solver
src/obj: object files when compiling c++
src/bin: location of OpenCL executable, bitstream xclbin, and csv of the results
tcl_scripts: scripting used during the bitstream generation process
testcases.sh: script to launch benchmark. Run the script from src/bin
testcases_sim.sh: script to test benchmarks during simulation. This file is to be used by the Makefile

Instructions to build and test project:
