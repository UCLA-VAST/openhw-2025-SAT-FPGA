# HLS kernel compile and link wrappers using v++ via custom commands

# Generate emconfig.json next to xclbin/xo
set(EMCONFIG_DIR ${CMAKE_BINARY_DIR}/bin)
set(TCL_DIR ${CMAKE_BINARY_DIR}/tcl)

# Helper to serialize lists with spaces for v++
function(join_list OUT)
  set(tmp "")
  foreach(arg IN LISTS ARGN)
    if(tmp STREQUAL "")
      set(tmp "${arg}")
    else()
      set(tmp "${tmp} ${arg}")
    endif()
  endforeach()
  set(${OUT} "${tmp}" PARENT_SCOPE)
endfunction()

# Collect per-kernel source lists defined in root CMakeLists
set(KSRC_0 ${KERNEL_SOURCES_0})
set(KSRC_1 ${KERNEL_SOURCES_1})
set(KSRC_2 ${KERNEL_SOURCES_2})
set(KSRC_3 ${KERNEL_SOURCES_3})
set(KSRC_4 ${KERNEL_SOURCES_4})
set(KSRC_5 ${KERNEL_SOURCES_5})
set(KSRC_6 ${KERNEL_SOURCES_6})

# Common v++ flags
set(COMMON_VPP_ARGS
  --temp_dir ${CMAKE_BINARY_DIR}/_x
  --log_dir ${CMAKE_BINARY_DIR}/_x
  --report_dir ${CMAKE_BINARY_DIR}/_x
  --include ${CMAKE_SOURCE_DIR}/include
  --platform ${PLATFORM}
  -R2
)

# Extra commands for hw_emu and hw builds
set(EXTRA_ARGS)
if(EMU_TYPE STREQUAL "hw_emu" OR EMU_TYPE STREQUAL "hw")
  list(APPEND EXTRA_ARGS -DFPGA_HW)
endif()

# Prepare Tcl scripts at configure time (inject build dir and vivado log path)
file(MAKE_DIRECTORY ${TCL_DIR})
set(VIVADO_LOG_PATH "${CMAKE_BINARY_DIR}/_x/link/vivado/vpl/vivado.log")
configure_file(${TCL_SCRIPTS_DIR}/constrain_blocks.tcl ${TCL_DIR}/constrain_blocks.tcl COPYONLY)
configure_file(${TCL_SCRIPTS_DIR}/phys_opt_loop.tcl ${TCL_DIR}/phys_opt_loop.tcl @ONLY)
configure_file(${TCL_SCRIPTS_DIR}/post_place_qor.tcl ${TCL_DIR}/post_place_qor.tcl @ONLY)

# Create compile commands per kernel
foreach(idx RANGE 0 6)
  list(GET KERNEL_NAMES ${idx} KNAME)
  list(GET KERNEL_CLOCKS ${idx} KCLOCK)
  set(XO ${CMAKE_BINARY_DIR}/bin/workload-${KNAME}-${EMU_TYPE}.xo)
  set(SRCS ${KSRC_${idx}})

  # g++ compile check
  set(CHECK_STAMP ${CMAKE_BINARY_DIR}/.hls_compilecheck_${KNAME}.stamp)
  add_custom_command(
    OUTPUT ${CHECK_STAMP}
    COMMAND ${CMAKE_CXX_COMPILER} -std=c++17 -w -O3 -fsyntax-only
      -I${XRT_INCLUDE_DIRS}
      -I${VitisHLS_INCLUDE_DIRS}
      -I${CMAKE_SOURCE_DIR}/include
      ${SRCS}
    COMMAND ${CMAKE_COMMAND} -E touch ${CHECK_STAMP}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${SRCS}
    COMMENT "C++ compile check for kernel ${KNAME}"
    VERBATIM)
  add_custom_command(
    OUTPUT ${XO}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND v++ -c -t ${EMU_TYPE} ${COMMON_VPP_ARGS} ${EXTRA_ARGS}
            -s --kernel ${KNAME} --hls.clock ${KCLOCK}:${KNAME}
            ${SRCS}
            -o ${XO}
  DEPENDS ${SRCS} ${CHECK_STAMP}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Compiling kernel ${KNAME} (${EMU_TYPE}) at ${KCLOCK}"
    VERBATIM)
  list(APPEND XO_LIST ${XO})
  # Create a per-kernel target so the build system can schedule these in parallel
  add_custom_target(xo_${KNAME}_${EMU_TYPE} DEPENDS ${XO})
  list(APPEND XO_TGT_LIST xo_${KNAME}_${EMU_TYPE})
endforeach()

add_custom_target(hls_compile_${EMU_TYPE} DEPENDS ${XO_LIST} ${XO_TGT_LIST})

# emconfigutil copy
add_custom_command(
  OUTPUT ${EMCONFIG_DIR}/emconfig.json
  COMMAND ${CMAKE_COMMAND} -E make_directory ${EMCONFIG_DIR}
  COMMAND emconfigutil --platform ${PLATFORM} --od ${EMCONFIG_DIR}
  DEPENDS hls_compile_${EMU_TYPE}
  COMMENT "Generating emconfig.json"
)

# Add additional Vivado properties for "hw" link
set(ADDITIONAL_VPP_LINK_ARGS)
if(EMU_TYPE STREQUAL "hw")
  list(APPEND ADDITIONAL_VPP_LINK_ARGS
    --vivado.prop "run.my_rm_synth_1.{STEPS.SYNTH_DESIGN.ARGS.MORE OPTIONS}={-directive AlternateRoutability}"
    --vivado.prop run.impl_1.STEPS.OPT_DESIGN.TCL.PRE=${TCL_DIR}/constrain_blocks.tcl
    --vivado.prop "run.impl_1.{STEPS.PLACE_DESIGN.ARGS.MORE OPTIONS}={-directive AltSpreadLogic_medium}"
    --vivado.prop "run.impl_1.{STEPS.PHYS_OPT_DESIGN.ARGS.MORE OPTIONS}={-directive Explore}"
    --vivado.prop run.impl_1.STEPS.PHYS_OPT_DESIGN.TCL.POST=${TCL_DIR}/phys_opt_loop.tcl
    --vivado.prop "run.impl_1.{STEPS.ROUTE_DESIGN.ARGS.MORE OPTIONS}={-directive AlternateCLBRouting}"
    -s
  )
endif()

# Link all kernels
set(XCLBIN ${CMAKE_BINARY_DIR}/bin/workload-${EMU_TYPE}.xclbin)
add_custom_command(
  OUTPUT ${XCLBIN}
  COMMAND v++ -l -t ${EMU_TYPE} ${COMMON_VPP_ARGS}
          --config ${CONNECTIVITY_FILE}
          --kernel_frequency ${FREQ_SC}
          ${ADDITIONAL_VPP_LINK_ARGS}
          ${XO_LIST}
          -o ${XCLBIN}
  DEPENDS ${XO_LIST} ${EMCONFIG_DIR}/emconfig.json
  COMMENT "Linking xclbin (${EMU_TYPE})"
)

add_custom_target(hls_link_${EMU_TYPE} DEPENDS ${XCLBIN})

# HLS report copy (only for EMU_TYPE=hw synth)
if(EMU_TYPE STREQUAL "hw")
  add_custom_target(hw_reports
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_SOURCE_DIR}/FPGArpt
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/FPGArpt
    # Copy reports for each kernel if they exist
    COMMAND /bin/sh -c [=[
KERNELS="${KERNEL_NAMES}"
# Convert potential semicolon-separated CMake list to space-separated for shell
KERNELS=${KERNELS//;/ }
for k in $KERNELS; do
  rpt_dir="${CMAKE_BINARY_DIR}/_x/workload-${k}-hw/${k}/${k}/solution/syn/report"
  if [ -d "$rpt_dir" ]; then
    cp "$rpt_dir"/*.rpt ${CMAKE_SOURCE_DIR}/FPGArpt/ || true
  fi
done
]=]
    DEPENDS hls_compile_${EMU_TYPE}
    COMMENT "Copying Vitis HLS reports to FPGArpt"
  )
else()
  add_custom_target(hw_reports
  COMMAND ${CMAKE_COMMAND} -E echo "Configure with -DEMU_TYPE=hw to generate HLS reports."
  )
endif()
