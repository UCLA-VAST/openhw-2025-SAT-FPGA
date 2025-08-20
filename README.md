# A Complete SAT Solver on a U55C FPGA (Team AOHW25-894)
This project demonstrates accelerating a complete SAT solver on a U55C FPGA. SAT solving is an important workload due to its wide use in different domains such as artificial intelligence, circuit verification, FPGA routing, etc.

## Prerequisites
- XRT 2.14.384
- Vitis 2022.2 (HLS toolchain available)
- Platform: `xilinx_u55c_gen3x16_xdma_3_202210_1`
- gcc/g++ 11.4.0
- cmake 3.26.3
- ubuntu 22.04.2

Ensure XRT and Vitis HLS are on PATH (`source env.sh`). Or manually:
```bash
source /opt/xilinx/xrt/setup.sh
source /opt/xilinx/tools/Vitis_HLS/2022.2/settings64.sh
```

## Repository layout
- `host/` – host sources
- `hls/` – HLS kernel sources
- `include/` – shared headers
- `config/` – HLS kernel configuration and connectivity files
- `SAT_test_cases/` – example SAT instances (DIMACS format)

## Quick start (Makefile shortcuts)
Choose an emulation type: `sw_emu | hw_emu | hw`. Use `EMU_TYPE` to select the mode. The following commands runs the tests found in [SAT_test_cases](SAT_test_cases) for each emulation type.

Software emulation end-to-end (if run terminates, try `ulimit -s 131072`):
```bash
make EMU_TYPE=sw_emu run -j
```

Hardware emulation end-to-end:
```bash
make EMU_TYPE=hw_emu run -j
```

Hardware bitstream end-to-end:
```bash
make EMU_TYPE=hw run -j
```

## Makefile targets
- `configure`  – configure CMake in `build/<EMU_TYPE>`
- `compilecl`  – build the host binary for `<EMU_TYPE>`
- `compilekernel` – compile all kernels to `.xo` for `<EMU_TYPE>` (parallel)
- `linkkernel` – link all `.xo` into `workload-<EMU_TYPE>.xclbin`
- `run`        – build and run the testcase suite for `<EMU_TYPE>`
- `hw_reports` – copy HLS synth reports into `FPGArpt/` (only used for `EMU_TYPE=hw`)
- `clean`      – remove only `build/<EMU_TYPE>`
- `clean-all`  – remove the entire `build/`

Tip: pass `-j` for parallel builds. Example: `make EMU_TYPE=hw_emu compilekernel -j`.

## Direct CMake usage
You can work directly with CMake/Make targets if you prefer:
```bash
cmake -S . -B build/<EMU_TYPE> -DEMU_TYPE=<EMU_TYPE>
cmake --build build/<EMU_TYPE> --target compilecl --parallel
cmake --build build/<EMU_TYPE> --target compilekernel --parallel
cmake --build build/<EMU_TYPE> --target linkkernel --parallel
cmake --build build/<EMU_TYPE> --target run --parallel
```

Generated artifacts (per EMU):
- `build/<EMU_TYPE>/bin/test.<EMU_TYPE>.out`: host binary
- `build/<EMU_TYPE>/bin/workload-<EMU_TYPE>.xclbin`: HLS kernel XCLBIN
- `build/<EMU_TYPE>/result_<EMU_TYPE>.txt`: run results

## Running testcases and custom inputs
The `run` target executes the appropriate testcase script based on `EMU_TYPE`:
- `sw_emu` and `hw_emu`: runs `testcases_sim.sh`
- `hw`: runs `testcases.sh` on the board

You can also run the host manually from `build/<EMU_TYPE>/bin` on your own SAT instances:
```bash
# Emulation
XCL_EMULATION_MODE=<EMU_TYPE> ./test.<EMU_TYPE>.out workload-<EMU_TYPE>.xclbin ../../config/configuration.json <input DIMACS> <output results.txt> <0|1>

# Hardware
./test.hw.out workload-hw.xclbin ../../config/configuration.json <input DIMACS> <output results.txt> <0|1>
```
The last argument is the expected SAT result (1 for SAT, 0 for UNSAT).

## Notes
- Our solver may not be able to solve the instance due to running out of on-chip memory during the solving process. The maximum supported instance size is **32,768 variables** and **131,072 clauses**.
- design frequency target (`FREQ_SC`) is 235 MHz defined in [CMakeLists.txt](CMakeLists.txt)

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
