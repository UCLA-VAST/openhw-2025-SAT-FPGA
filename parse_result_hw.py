#!/usr/bin/env python3
"""
Parse build/hw/result_hw.txt, sort tests by name, and dump simulation time in milliseconds.
Also compute PAR-2 with a default timeout of 360 seconds.
PAR-2 rule here: if sim_time == 0 (did not finish), use 2 * timeout; otherwise use the actual time.

Usage:
    python3 parse_result_hw.py [-i INPUT_CSV] [-o OUTPUT_CSV] [-t TIMEOUT_S]

- INPUT_CSV defaults to ./build/hw/result_hw.txt
- OUTPUT_CSV defaults to stdout
- TIMEOUT_S defaults to 360 seconds

Output columns:
        name,sim_time_ms,par2_ms
"""
from __future__ import annotations
import argparse
import csv
import io
import os
import sys
from typing import List, Tuple


def _is_float(s: str) -> bool:
    try:
        float(s)
        return True
    except Exception:
        return False


def parse_results(path: str) -> List[Tuple[str, float, bool]]:
    with open(path, newline='') as f:
        reader = csv.reader(f)
        try:
            header = next(reader)
        except StopIteration:
            return []

        # Normalize header and get indices
        header_map = {name.strip(): idx for idx, name in enumerate(header)}
        name_idx = header_map.get('CNF NAME', 0)
        time_idx = header_map.get('Time (s)', None)
    # We ignore SOLVE STATUS for PAR-2; per request, use sim_time==0 as unfinished

        results: List[Tuple[str, float, bool]] = []
        for row in reader:
            if not row:
                continue
            # Pad row length if very short
            # (some lines might be truncated due to skips; we'll fall back to last numeric field)
            name = row[name_idx].strip() if name_idx < len(row) else row[0].strip()
            if not name:
                continue

            time_s: float = 0.0
            if time_idx is not None and time_idx < len(row) and _is_float(row[time_idx]):
                time_s = float(row[time_idx])
            else:
                # Fallback: find last numeric token in row
                for cell in reversed(row):
                    if _is_float(cell):
                        time_s = float(cell)
                        break

            # Determine solved/unsolved for PAR-2: solved iff time_s > 0
            solved = time_s > 0.0

            time_ms = time_s * 1000.0
            results.append((name, time_ms, solved))

        # Sort by test name
        results.sort(key=lambda x: x[0])
        return results


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('-i', '--input', default=os.path.join('build', 'hw', 'result_hw.txt'), help='Input CSV path')
    ap.add_argument('-o', '--output', default='-', help='Output CSV path (default: stdout)')
    ap.add_argument('-t', '--timeout', type=float, default=360.0, help='Timeout in seconds for PAR-2 (default: 360s)')
    args = ap.parse_args(argv)

    if not os.path.isfile(args.input):
        print(f"Input file not found: {args.input}", file=sys.stderr)
        return 2

    rows = parse_results(args.input)

    out: io.TextIOBase
    close_out = False
    if args.output == '-' or args.output == '':
        out = sys.stdout
    else:
        out = open(args.output, 'w', newline='')
        close_out = True

    try:
        writer = csv.writer(out)
        writer.writerow(['name', 'sim_time_ms', 'par2_ms'])
        timeout_ms = float(args.timeout) * 1000.0
        for name, ms, solved in rows:
            # per-instance PAR-2 value in ms
            par2_ms = ms if solved else (2.0 * timeout_ms)
            writer.writerow([name, f"{ms:.3f}", f"{par2_ms:.3f}"])
    finally:
        if close_out:
            out.close()

    # Compute and print PAR-2 summary to stderr
    # For solved cases: contribution = time_s; for unsolved: 2 * timeout
    timeout = float(args.timeout)
    total = 0.0
    solved_cnt = 0
    for _, ms, solved in rows:
        if solved:
            total += ms
            solved_cnt += 1
        else:
            total += 2.0 * timeout * 1000.0
    n = len(rows)
    mean = (total / n) / 1000 if n > 0 else 0.0
    print(f"Summary: n={n}, timeout_s={timeout}, solved={solved_cnt}, par-2={mean:.3f}s", file=sys.stderr)

    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
