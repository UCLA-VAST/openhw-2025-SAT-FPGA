# Host/OpenCL build

# Decide emulation flag and lib
set(HW_SIM_DEF -DHW_SIM)
# Library names
set(XRT_OPENCL_LIB xilinxopencl)
set(XRT_SWEMU_LIB xrt_swemu)
set(XRT_HWEMU_LIB xrt_hwemu)
set(XRT_CORE_LIB xrt_core)

set(XRT_EMU_LIB ${XRT_SWEMU_LIB})
if(EMU_TYPE STREQUAL "hw_emu")
  set(XRT_EMU_LIB ${XRT_HWEMU_LIB})
elseif(EMU_TYPE STREQUAL "hw")
  # link with xrt_core and remove HW_SIM define
  set(XRT_EMU_LIB ${XRT_CORE_LIB})
  set(HW_SIM_DEF)
endif()

set(HOST_SOURCES
  ${CMAKE_SOURCE_DIR}/host/src/host.cpp
  ${CMAKE_SOURCE_DIR}/host/src/xcl2.cpp
)

add_library(host_objs_${EMU_TYPE} OBJECT ${HOST_SOURCES})

# Ensure C++17 and warnings similar to script
target_compile_features(host_objs_${EMU_TYPE} PRIVATE cxx_std_17)
target_compile_options(host_objs_${EMU_TYPE} PRIVATE -Wall -Wno-unknown-pragmas -O3)

target_compile_definitions(host_objs_${EMU_TYPE} PRIVATE FPGA_DEVICE C_KERNEL ${HW_SIM_DEF})
 
# Include directories
#  - XRT includes (from FindXRT)
#  - Vitis HLS includes (from FindVitisHLS)
target_include_directories(host_objs_${EMU_TYPE} PRIVATE
  ${SAT_COMMON_INCLUDES}
  ${CMAKE_SOURCE_DIR}/host/include
  ${CMAKE_SOURCE_DIR}/include/rapidjson
)

add_executable(test_${EMU_TYPE} $<TARGET_OBJECTS:host_objs_${EMU_TYPE}>)

# Link dirs and libs
target_link_directories(test_${EMU_TYPE} PRIVATE ${XRT_LIBRARY_DIR})
target_link_libraries(test_${EMU_TYPE}
  ${XRT_OPENCL_LIB}
  pthread
  rt
  uuid
  stdc++
  ${XRT_EMU_LIB}
)

set_target_properties(test_${EMU_TYPE} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  OUTPUT_NAME "test.${EMU_TYPE}.out"
)

add_custom_target(host_${EMU_TYPE} DEPENDS test_${EMU_TYPE})
