# FPGA25_artifact
## Instructions:

Following needs to be installed and is the version used for our design:  
-XRT version 2.14.384  
-Vitis version 2022.2  
-xilinx_u55c_gen3x16_xdma_3_202210_1 platform  
-gcc/g++ version 10+  

Provided is a shell script (./runCompile.sh) to compile the HLS code to generate the bitstream.
However, for your convenience, an executable and the FPGA bitstream is already provided. 
In the shell script, the path for VITIS_INCLUDE and XRT_INCLUDE need to be changed to point to your include path.

```sh
VITIS_INCLUDE="/opt/xilinx/tools/Vitis_HLS/$VER/include" -> TO WHERE YOUR VITIS INCLUDE IS  
XRT_INCLUDE="/opt/xilinx/xrt/include" -> TO WHERE YOUR XRT INCLUDE IS
```

The same needs to be done with the source command.

```sh
source /opt/xilinx/xrt/setup.sh -> TO WHERE YOUR XRT SETUP SCRIPT IS  
source /opt/xilinx/tools/Vitis_HLS/$VER/settings64.sh -> TO WHERE YOUR VITIS_HLS SETUP SCRIPT IS  
```
To build for real hardware:  
```sh
./runCompile hls && ./runCompile hw  
```

To run simulation for software and hardware emulation:  
```sh
./runCompile doall  
```

## To run hardware execution after hardware build:  
-Enable host memory with the command: 
```sh
sudo PATH_TO_XBUTIL/xbutil configure --host-mem -d DEVICE_ID -s 1G ENABLE  (only need to do once)
```
where PATH_TO_XBUTIL is the install path for xbutil and DEVICE_ID is the device id of the U55C FPGA. The device id command can be found with xbutil examine -d.  

### To run provided testcases:
-First compile openCL with ./runCompile.sh opencl  
-Then run the ./testcases.sh  
-A .txt file (which is in CSV format) called answers.txt will be found in src/bin  
-ColumnJ is the FPGA runtime in seconds.  
-Please note answers.txt is an appended file. Therefore, it is advised to remove it for new runs.

### To run other SAT instances:
```sh
cd src/bin
./test.real.out workload-hw.xclbin ../configuration.json <YOU_SAT_DIMACS> <SAVE_METRICS_FILE.TXT> <0_OR_1>
```

-To verify our solver, the SAT instance needed to match other solvers. Therefore, you must know beforehand if it is SAT(1) or UNSAT(0).  
-You should modify the host.cpp in src to your desire to not do this check.  
-If host.cpp is modified, remember to recompile with ./runCompile.sh opencl  

## To run MiniSat or Kissat:  
-First clone repository and follow the install instructions provided by those authors.  
https://github.com/niklasso/minisat  
https://github.com/arminbiere/kissat  
-Locate the installed executables and run each individual SAT instance, for example:  
```sh
./minisat PATH_TO_DIMACS/unsat/4_4_2.dimacs  
./kisat PATH_TO_DIMACS/unsat/4_4_2.dimacs
```
where PATH_TO_DIMACS is the folder SAT_test_cases in this repository.  

## Publication
To cite this work:

```bibtex
@inproceedings{10.1145/3706628.3708869,
author = {Lo, Michael and Chang, Mau-Chung Frank and Cong, Jason},
title = {SAT-Accel: A Modern SAT Solver on a FPGA},
year = {2025},
isbn = {9798400713965},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3706628.3708869},
doi = {10.1145/3706628.3708869},
abstract = {Boolean satisfiability (SAT) solving is the first known NP-complete problem and is widely used in many application domains. Over the years, there have been so many consistent improvements in this area such that larger instances can be solved relatively quickly. Although these improvements have found their way onto CPU implementations, there has been limited progress adopting this on hardware accelerators mainly because it is difficult to implement the dynamic data structures needed to support a modern SAT solving algorithm. In this work, we present SAT-Accel, an algorithm-hardware co-design solver that applies many of the core improvements found in modern SAT solvers. SAT-Accel uses a novel memory management system and representation that supports the dynamic data structures required by a modern SAT solving algorithm. Our design can achieve on average a 17.9x speedup against MiniSat, the previous state of the art CPU solver, and a 2.8x speedup against Kissat, the current state of the art CPU solver. Compared to the current state-of-the-art stand-alone hardware accelerator, SAT-Hard, SAT-Accel achieves on average 800.0x speedup.},
booktitle = {Proceedings of the 2025 ACM/SIGDA International Symposium on Field Programmable Gate Arrays},
pages = {234–246},
numpages = {13},
keywords = {accelerator, boolean satisfiability, fpga, high-level synthesis},
location = {Monterey, CA, USA},
series = {FPGA '25}
}
```
