# A Complete SAT Solver on a U55C FPGA (Team AOHW25-894)
This project demonstrates accelerating a complete SAT solver on a U55C FPGA. SAT solving is an important workload due to its wide use in different domains such as artificial intelligence, circuit verification, FPGA routing, etc.

<img width="450" height="450" alt="image" src="https://github.com/user-attachments/assets/9d2e3c92-0af9-4a4f-836f-ad0f02973dc9" />
<img width="280" height="400" alt="routed_SAT_solver" src="https://github.com/user-attachments/assets/bbda18e1-0c36-432b-a4b5-6822ee0269ed" />

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

-Enable host memory first. PATH_TO_XBUTIL is the install path for xbutil and DEVICE_ID is the device id of the U55C FPGA. The device id command can be found with xbutil examine -d.

-Change the DEVICE_ID in testcases.sh. Replace 0000:81:00.1 with your FPGA device id.

```bash
sudo PATH_TO_XBUTIL/xbutil configure --host-mem -d DEVICE_ID -s 1G ENABLE  (only need to do once)
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
