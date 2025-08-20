#!/usr/bin/env bash

# Ensure this script is sourced, not executed
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  echo "Please source this script to set environment variables:"
  echo "  source ./env.sh"
  exit 1
fi

# Save current cmake path before sourcing toolchains
# AMD Xilinx toolchains use a much older cmake
CMAKE_PATH=$(which cmake 2>/dev/null)

# Source toolchains (optional). Users can override by exporting XILINX_XRT and VITIS_HLS.
# Example (adjust to your install):
#   source /opt/xilinx/xrt/setup.sh
#   source /opt/xilinx/tools/Vitis_HLS/2022.2/settings64.sh

# Prefer user-provided locations, then fall back to common defaults
# XRT
if [ -n "${XILINX_XRT}" ] && [ -f "${XILINX_XRT}/setup.sh" ]; then
  . "${XILINX_XRT}/setup.sh"
elif [ -f /opt/xilinx/xrt/setup.sh ]; then
  . /opt/xilinx/xrt/setup.sh
fi

# Vitis HLS
if [ -n "${VITIS_HLS}" ] && [ -f "${VITIS_HLS}/settings64.sh" ]; then
  . "${VITIS_HLS}/settings64.sh"
elif [ -d /opt/xilinx/tools/Vitis_HLS ]; then
  VER=${VER:-2022.2}
  if [ -f "/opt/xilinx/tools/Vitis_HLS/${VER}/settings64.sh" ]; then
    . "/opt/xilinx/tools/Vitis_HLS/${VER}/settings64.sh"
  fi
fi

# Restore system cmake path to highest priority
if [ -n "$CMAKE_PATH" ]; then
  CMAKE_DIR=$(dirname "$CMAKE_PATH")
  export PATH="$CMAKE_DIR:$PATH"
fi

echo "Environment configured:"
command -v v++ >/dev/null 2>&1 && echo "- v++: $(command -v v++)" || echo "- v++: not found in PATH"
command -v xbutil >/dev/null 2>&1 && echo "- xbutil: $(command -v xbutil)" || true
