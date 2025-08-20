cmake_minimum_required(VERSION 3.16)
project(SAT_Accel LANGUAGES C CXX)

# Options
set(EMU_TYPE "sw_emu" CACHE STRING "Emulation type: sw_emu | hw_emu | hw")
set(PLATFORM "xilinx_u55c_gen3x16_xdma_3_202210_1" CACHE STRING "Target AMD Xilinx platform")
set(FREQ_SC 235 CACHE STRING "Kernel link frequency (MHz)")
set(CONNECTIVITY_FILE "${CMAKE_SOURCE_DIR}/config/k2k.cfg" CACHE FILEPATH "Vitis connectivity config")
set(CONFIG_FILE "${CMAKE_SOURCE_DIR}/config/configuration.json" CACHE FILEPATH "Host configuration file")
set(DATA_PATH "${CMAKE_SOURCE_DIR}/SAT_test_cases" CACHE PATH "Path to SAT test cases")
set(TCL_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/tcl" CACHE PATH "Vivado Tcl scripts directory")

# Output directory per emu type
if(NOT CMAKE_BINARY_DIR MATCHES "/build/")
  message(STATUS "Recommend building in build/<emu_type> directory. Current: ${CMAKE_BINARY_DIR}")
endif()

# Find dependencies (XRT & Vitis HLS includes)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(XRT REQUIRED)
find_package(VitisHLS REQUIRED)

# Common include dirs
set(SAT_COMMON_INCLUDES
  ${XRT_INCLUDE_DIRS}
  ${VitisHLS_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/rapidjson
)

# Central kernel definitions (names, source lists, per-kernel clocks)
set(KERNEL_NAMES
  solver
  clause_store_handler
  location_handler
  restartCalculator
  timer
  pqHandler
  message
)

set(KERNEL_SOURCES_0
  ${CMAKE_SOURCE_DIR}/hls/src/backtrack.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/color.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/copy_in.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/decide.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/discover.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/learn.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/minimize.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/manage.cpp
  ${CMAKE_SOURCE_DIR}/hls/src/solver.cpp
)
set(KERNEL_SOURCES_1 ${CMAKE_SOURCE_DIR}/hls/src/clause_store_handler.cpp)
set(KERNEL_SOURCES_2 ${CMAKE_SOURCE_DIR}/hls/src/location_handler.cpp)
set(KERNEL_SOURCES_3 ${CMAKE_SOURCE_DIR}/hls/src/restart.cpp)
set(KERNEL_SOURCES_4 ${CMAKE_SOURCE_DIR}/hls/src/timer.cpp)
set(KERNEL_SOURCES_5 ${CMAKE_SOURCE_DIR}/hls/src/priority_queue_functions.cpp ${CMAKE_SOURCE_DIR}/hls/src/pq_handler.cpp)
set(KERNEL_SOURCES_6 ${CMAKE_SOURCE_DIR}/hls/src/message.cpp)

# Per-kernel clock in Hz as in original script
set(KERNEL_CLOCKS
  250000000
  250000000
  250000000
  200000000
  250000000
  200000000
  200000000
)

# Subdirs
add_subdirectory(host)
add_subdirectory(hls)

# Targets
add_custom_target(compilecl DEPENDS host_${EMU_TYPE})
add_custom_target(compilekernel DEPENDS hls_compile_${EMU_TYPE})
add_custom_target(linkkernel DEPENDS hls_link_${EMU_TYPE})

# Run target: executes testcases_sim.sh with proper cwd
if(EMU_TYPE STREQUAL "hw")
  add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env DATA_PATH=${DATA_PATH}
            ${CMAKE_SOURCE_DIR}/testcases.sh ${CONFIG_FILE} ${CMAKE_BINARY_DIR}/result_hw.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEPENDS host_${EMU_TYPE} hls_link_${EMU_TYPE}
  )
else()
  add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env EMU_TYPE=${EMU_TYPE} XCL_EMULATION_MODE=${EMU_TYPE} DATA_PATH=${DATA_PATH}
            ${CMAKE_SOURCE_DIR}/testcases_sim.sh ${EMU_TYPE} ${CONFIG_FILE} ${CMAKE_BINARY_DIR}/result_${EMU_TYPE}.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEPENDS host_${EMU_TYPE} hls_link_${EMU_TYPE}
  )
endif()


