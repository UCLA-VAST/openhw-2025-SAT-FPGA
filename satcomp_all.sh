#!/usr/bin/env bash

# Run all SAT Competition tests (hardware only) using satcomp_sim_results.txt mapping
# Usage: ./satcomp_all.sh <config.json> <results.csv>

set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <config.json> <results.csv>"
  exit 1
fi

CONFIG="$1"
RESULT_FILE="$2"

RD="\033[0;31m"
GN="\033[0;32m"
YL="\033[0;33m"
NC="\033[0m"

# Default locations (can override via env)
SATCOMP_DIR=${SATCOMP_DIR:-"$(cd "$(dirname "$0")" && pwd)/../satcomp_sim"}
RESULTS_MAP=${SATCOMP_RESULTS_FILE:-"$(cd "$(dirname "$0")" && pwd)/../satcomp_sim_results.txt"}

# Normalize paths
SATCOMP_DIR=$(readlink -f "$SATCOMP_DIR" 2>/dev/null || echo "$SATCOMP_DIR")
RESULTS_MAP=$(readlink -f "$RESULTS_MAP" 2>/dev/null || echo "$RESULTS_MAP")

# Safety: ensure inputs exist and are readable
if [ -z "${SATCOMP_DIR:-}" ]; then
  echo -e "${RD}SATCOMP_DIR is empty or unset${NC}"
  exit 1
fi
if [ -z "${RESULTS_MAP:-}" ]; then
  echo -e "${RD}RESULTS_MAP is empty or unset${NC}"
  exit 1
fi

# Hardware binary checks
if [ ! -f "./test.hw.out" ] || [ ! -f "./workload-hw.xclbin" ]; then
  echo -e "${RD}Missing hardware files. Expected ./test.hw.out and ./workload-hw.xclbin in $(pwd)${NC}"
  exit 1
fi

if [ ! -d "$SATCOMP_DIR" ]; then
  echo -e "${RD}SAT competition directory not found: $SATCOMP_DIR${NC}"
  exit 1
fi

if [ ! -f "$RESULTS_MAP" ]; then
  echo -e "${RD}Results mapping file not found: $RESULTS_MAP${NC}"
  exit 1
fi

# Readability/traversal checks
if [ ! -r "$RESULTS_MAP" ]; then
  echo -e "${RD}Results mapping file is not readable: $RESULTS_MAP${NC}"
  exit 1
fi
if [ ! -r "$SATCOMP_DIR" ] || [ ! -x "$SATCOMP_DIR" ]; then
  echo -e "${RD}SATCOMP_DIR is not readable/traversable: $SATCOMP_DIR${NC}"
  exit 1
fi

# Ensure results header exists (after path checks)
CSV_HEADER="CNF NAME,SOLVE STATUS,Literals,Clauses,Time (s),Decisions,Conflcits,Restarts,Load Cycle,Load %,Decide Cycle,Decide %,Propagate Cycle,Propagate %,Learn Cycle,Learn %,Min-Btrk Cycle,Min-Btrk %,Save Cycle,Save %,Allocate Cycle,Allocate %,NOT USED,NOT USED,Delete Cycle,Delete %,Literals Checked,Average Propagation Latency,Total Cycles"
if [ ! -s "$RESULT_FILE" ]; then
  echo "$CSV_HEADER" > "$RESULT_FILE"
fi

echo -e "Using SATCOMP_DIR=$SATCOMP_DIR"
echo -e "Using RESULTS_MAP=$RESULTS_MAP"

# Iterate over results mapping and execute
tail -n +2 "$RESULTS_MAP" | while IFS=, read -r name outcome; do
  # trim whitespace
  name="${name## }"; name="${name%% }"
  outcome="${outcome## }"; outcome="${outcome%% }"
  [ -z "$name" ] && continue

  case "$outcome" in
    SAT|sat|Sat) correct=1 ;;
    UNSAT|unsat|Unsat) correct=0 ;;
    *) echo -e "${YL}Unknown outcome '$outcome' for '$name', skipping${NC}"; continue ;;
  esac

  path="$SATCOMP_DIR/$name"
  if [ ! -f "$path" ]; then
    echo -e "${YL}Missing test file: $path — skipping${NC}"
    continue
  fi

  echo -e "Running: $name (expected: $outcome)"
  set +e
  time ./test.hw.out workload-hw.xclbin "$CONFIG" "$path" "$RESULT_FILE" "$correct"
  exitCode=$?
  set -e

  if [ $exitCode -ne 0 ]; then
    if [ $exitCode -eq 3 ]; then
      echo -e "${YL}Skipped due to resource/memory constraints${NC}"
      sleep 1
    else
      echo -e "${RD}Error (exit $exitCode) while running $name${NC}"
      exit 1
    fi
  else
    echo -e "${GN}Test completed${NC}"
  fi
done

echo -e "${GN}All satcomp tests processed (hardware mode).${NC}"
