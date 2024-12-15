# FPGA25_artifact
Instructions:

Following needs to be installed and is the version used for our design:  
-XRT version 2.14.384  
-Vitis version 2022.2  
-xilinx_u55c_gen3x16_xdma_3_202210_1 platform  
-gcc/g++ version 10+  

Provided is a shell script (./runCompile.sh) to compile the HLS code to generate the bitstream.
However, for your convenience, an executable and the FPGA bitstream is already provided. 
In the shell script, the path for VITIS_INCLUDE and XRT_INCLUDE need to be changed to point to your include path.
  
VITIS_INCLUDE="/opt/xilinx/tools/Vitis_HLS/$VER/include" -> TO WHERE YOUR VITIS INCLUDE IS  
XRT_INCLUDE="/opt/xilinx/xrt/include" -> TO WHERE YOUR XRT INCLUDE IS  

The same needs to be done with the source command.

source /opt/xilinx/xrt/setup.sh -> TO WHERE YOUR XRT SETUP SCRIPT IS  
source /opt/xilinx/tools/Vitis_HLS/$VER/settings64.sh -> TO WHERE YOUR VITIS_HLS SETUP SCRIPT IS  

To build:  
./runCompile hls && ./runCompile hw  

To run simulation for software and hardware emulation:  
./runCompile doall  

To run hardware execution after build:  
-First compile openCL with ./runCompile.sh opencl (only need to do once)  
-Enable host memory with the command: sudo PATH_TO_XBUTIL/xbutil configure --host-mem -d DEVICE_ID -s 1G ENABLE  
where PATH_TO_XBUTIL is the install path for xbutil and DEVICE_ID is the device id of the U55C FPGA. The device id command can be found with xbutil examine -d.  
-Then run the ./testcases.sh  
-A .txt file (which is in CSV format) called answers.txt will be found in src/bin  
-ColumnJ is the FPGA runtime in seconds.  

To run MiniSat or Kissat:  
-First clone repository and follow the install instructions provided by those authors.  
https://github.com/niklasso/minisat  
https://github.com/arminbiere/kissat  
-Locate the installed executables and run each individual SAT instance, for example:  
./minisat PATH_TO_DIMACS/unsat/4_4_2.dimacs  
./kisat PATH_TO_DIMACS/unsat/4_4_2.dimacs  
where PATH_TO_DIMACS is the folder SAT_test_cases in this repository.  
