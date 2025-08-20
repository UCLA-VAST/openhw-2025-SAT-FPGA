# SAT-Accel (FPGA25 Artifact)

Modern SAT solver accelerated on AMD/Xilinx FPGA. This repo uses a clean CMake + Make flow with per-EMU build dirs under `build/<emu>/bin`.

## Prerequisites
- XRT 2.14.384
- Vitis 2022.2 (HLS toolchain available)
- Platform: `xilinx_u55c_gen3x16_xdma_3_202210_1`
- gcc/g++ 10+

Environment setup options
- Option A: source `env.sh` at the repo root, which sets up the Xilinx toolchains (XRT/Vitis HLS) so CMake can find headers and libraries.
- Option B: ensure XRT and Vitis HLS are on PATH yourself (xbutil available; Vitis HLS settings sourced).

Example:
```bash
source /opt/xilinx/xrt/setup.sh
source /opt/xilinx/tools/Vitis_HLS/2022.2/settings64.sh
```

## Repository layout
- `host/` – host application sources and headers
- `hls/src/` – HLS kernel sources
- `include/` – shared headers (includes `include/rapidjson`)
- `config/` – configuration files (`configuration.json`, `k2k.cfg`)
- `SAT_test_cases/` – input DIMACS instances
- `FPGArpt/` – HLS reports (generated for `EMU_TYPE=hw`)

## Quick start (Makefile shortcuts)
Choose an emulation type: `sw_emu | hw_emu | hw`. Use EMU_TYPE to select the mode.

Software emulation end-to-end:
```bash
make EMU_TYPE=sw_emu run -j
```

Hardware emulation end-to-end:
```bash
make EMU_TYPE=hw_emu run -j
```

Hardware bitstream link (no run):
```bash
make EMU_TYPE=hw linkkernel -j
```

## Makefile targets
- `configure`  – configure CMake in `build/<EMU_TYPE>`
- `compilecl`  – build the host app for `<EMU_TYPE>`
- `compilekernel` – compile all kernels to `.xo` for `<EMU_TYPE>` (parallel)
- `linkkernel` – link `.xo` into `workload-<EMU_TYPE>.xclbin`
- `run`        – build and run the testcase suite for `<EMU_TYPE>`
- `hw_reports` – copy HLS synth reports into `FPGArpt/` (only meaningful for `EMU_TYPE=hw`)
- `clean`      – remove only `build/<EMU_TYPE>`
- `clean-all`  – remove the entire `build/`

Tip: pass `-j` for parallel builds. Example: `make EMU_TYPE=hw_emu compilekernel -j`.

## Advanced: direct CMake usage
You can work directly with CMake/Make targets if you prefer:
```bash
cmake -S . -B build/sw_emu -DEMU_TYPE=sw_emu
cmake --build build/sw_emu --target compilecl --parallel
cmake --build build/sw_emu --target compilekernel --parallel
cmake --build build/sw_emu --target linkkernel --parallel
cmake --build build/sw_emu --target run --parallel
```

Generated artifacts (per EMU):
- `build/<emu>/bin/test.<emu>.out`
- `build/<emu>/bin/workload-<emu>.xclbin`
- `build/<emu>/bin/emconfig.json`

## Running testcases and custom inputs
The `run` target executes the appropriate testcase script based on `EMU_TYPE`:
- `sw_emu` and `hw_emu`: runs `testcases_sim.sh` and writes `result_<emu>.txt` into `build/<emu>/bin`.
- `hw`: runs `testcases.sh` on the board.

You can also run the host manually from `build/<emu>/bin`:
```bash
# Emulation
XCL_EMULATION_MODE=<emu> ./test.<emu>.out workload-<emu>.xclbin ../../config/configuration.json <DIMACS> <RESULTS.txt> <0|1>

# Hardware
./test.hw.out workload-hw.xclbin ../../config/configuration.json <DIMACS> answers.txt <0|1>
```
The last argument is the expected SAT result (1 for SAT, 0 for UNSAT).

## Configuration knobs
Override these CMake options as needed:
- `-DEMU_TYPE=sw_emu|hw_emu|hw`
- `-DPLATFORM=xilinx_u55c_gen3x16_xdma_3_202210_1`
- `-DFREQ_SC=235` (MHz for link)
- `-DCONNECTIVITY_FILE=<path>` (defaults to `config/k2k.cfg`)
- `-DCONFIG_FILE=<path>` (defaults to `config/configuration.json`)
- `-DDATA_PATH=<path>` (defaults to repo `SAT_test_cases`)

## HLS reports
When configured with `-DEMU_TYPE=hw`, collect HLS synth reports:
```bash
make EMU_TYPE=hw hw_reports
```
Reports are copied into `FPGArpt/` at the repo root.

## Notes
- CMake finds XRT and Vitis HLS via `cmake/FindXRT.cmake` and `cmake/FindVitisHLS.cmake`.
- All outputs live under `build/<emu>/bin`; temp/reports under `build/<emu>/_x`.
- No absolute paths are required; data is relative to the repo.

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
