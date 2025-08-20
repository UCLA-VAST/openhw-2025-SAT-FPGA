# Locate AMD Xilinx XRT root, include, and lib
# Exposes: XRT_FOUND, XRT_ROOT_DIR, XRT_INCLUDE_DIRS, XRT_LIBRARY_DIR

# Try to discover from xbutil on PATH first
find_program(_XRT_XBUTIL NAMES xbutil)
if(_XRT_XBUTIL)
  get_filename_component(_xbutil_bin "${_XRT_XBUTIL}" DIRECTORY)
  get_filename_component(XRT_ROOT_DIR "${_xbutil_bin}" DIRECTORY)
endif()

# Fall back to environment variable
if(NOT XRT_ROOT_DIR AND DEFINED ENV{XILINX_XRT})
  set(XRT_ROOT_DIR "$ENV{XILINX_XRT}")
endif()

# Derive include and lib dirs and validate existence
unset(XRT_INCLUDE_DIRS)
unset(XRT_LIBRARY_DIR)
if(XRT_ROOT_DIR)
  set(_cand_inc "${XRT_ROOT_DIR}/include")
  if(EXISTS "${_cand_inc}")
    set(XRT_INCLUDE_DIRS "${_cand_inc}")
  endif()

  # Prefer lib, fall back to lib64
  set(_cand_lib "${XRT_ROOT_DIR}/lib")
  if(EXISTS "${_cand_lib}")
    set(XRT_LIBRARY_DIR "${_cand_lib}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XRT
  REQUIRED_VARS XRT_ROOT_DIR XRT_INCLUDE_DIRS XRT_LIBRARY_DIR
)
