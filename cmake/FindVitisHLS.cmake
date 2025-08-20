# Locate Vitis/Vivado HLS
# Exposes:
#  VitisHLS_FOUND
#  VitisHLS_ROOT_DIR
#  VitisHLS_BINARY (vivado_hls or vitis_hls)
#  VitisHLS_INCLUDE_DIRS

# Find HLS binary first, then derive root
find_path(_HLS_BIN_DIR
  NAMES vivado_hls vitis_hls
  PATHS ${VIVADO_HLS_ROOT_DIR} ENV XILINX_VIVADO_HLS ENV XILINX_HLS
  PATH_SUFFIXES bin
)

if(NOT _HLS_BIN_DIR)
  # As a minimal fallback, try common install roots
  find_path(_HLS_BIN_DIR
    NAMES vivado_hls vitis_hls
    PATHS /opt/xilinx/tools/Vitis_HLS /opt/xilinx/Vitis_HLS /tools/Xilinx/Vitis_HLS
    PATH_SUFFIXES bin
  )
endif()

if(_HLS_BIN_DIR)
  get_filename_component(VitisHLS_ROOT_DIR ${_HLS_BIN_DIR} DIRECTORY)
  if(EXISTS ${_HLS_BIN_DIR}/vivado_hls)
    set(VitisHLS_BINARY ${_HLS_BIN_DIR}/vivado_hls)
  elseif(EXISTS ${_HLS_BIN_DIR}/vitis_hls)
    set(VitisHLS_BINARY ${_HLS_BIN_DIR}/vitis_hls)
  endif()
endif()

# Locate include directory under root (existence check only)
if(VitisHLS_ROOT_DIR)
  if(EXISTS ${VitisHLS_ROOT_DIR}/include)
    set(VitisHLS_INCLUDE_DIRS ${VitisHLS_ROOT_DIR}/include)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VitisHLS REQUIRED_VARS VitisHLS_ROOT_DIR VitisHLS_INCLUDE_DIRS)
