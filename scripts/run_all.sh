#!/usr/bin/env bash
# run_all.sh - Role 3: compile with -O2 and run every standard experiment (Mac / Linux)
#
# Usage, from the repository root:
#     bash scripts/run_all.sh "Your Name"
#
# Output: results/<Your_Name>.csv
# Before running: laptop plugged in, other apps closed.

set -euo pipefail
cd "$(dirname "$0")/.."

MEMBER="${1:-}"
if [ -z "$MEMBER" ]; then
    read -r -p "Your name in English (written to the CSV): " MEMBER
fi
[ -z "$MEMBER" ] && { echo "Error: a name is required."; exit 1; }

RUNS="${RUNS:-10}"     # timed runs per experiment (plus 1 warm-up)
SEED=42
DEG=3

# ---- compiler -------------------------------------------------------------
CXX="${CXX:-}"
if [ -z "$CXX" ]; then
    for c in clang++ g++ c++; do
        if command -v "$c" >/dev/null 2>&1; then CXX="$c"; break; fi
    done
fi
if [ -z "$CXX" ]; then
    echo "Error: no C++ compiler found. Mac: run 'xcode-select --install'. Linux: install g++."
    exit 1
fi

# ---- CPU model --------------------------------------------------------------
CPU="unknown"
case "$(uname -s)" in
    Darwin) CPU="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo unknown)" ;;
    Linux)
        CPU="$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d: -f2 | sed 's/^ *//' || true)"
        if [ -z "$CPU" ] && command -v lscpu >/dev/null 2>&1; then
            CPU="$(lscpu | grep -m1 'Model name' | cut -d: -f2 | sed 's/^ *//' || true)"
        fi
        [ -z "$CPU" ] && CPU="unknown" ;;
esac

# ---- output file ------------------------------------------------------------
SAFE_NAME="$(printf '%s' "$MEMBER" | tr -c 'A-Za-z0-9_-' '_')"
mkdir -p results
OUT="results/${SAFE_NAME}.csv"
rm -f "$OUT"      # start fresh so a second attempt does not duplicate rows

echo "Member : $MEMBER"
echo "CPU    : $CPU"
echo "Output : $OUT"
echo "Compiling with $CXX -O2 ..."
"$CXX" -std=c++17 -O2 -Isrc src/main.cpp src/generator.cpp src/trace.cpp -o food_trace

run() {
    ./food_trace --nodes "$1" --reported "$2" --seed "$SEED" --deg "$DEG" --runs "$RUNS" \
                 --member "$MEMBER" --cpu "$CPU" --out "$OUT"
}

# Default sizes, 1,000 reported batches
run 10000   1000
run 100000  1000
run 500000  1000
run 1000000 1000
# Extra test on the 100K graph: vary the number of reported batches
run 100000  100
run 100000  10000

echo
echo "Done. Results saved to $OUT"
