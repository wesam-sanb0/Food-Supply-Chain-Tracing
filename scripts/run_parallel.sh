#!/usr/bin/env bash
# run_parallel.sh - Assignment 2 (Jory Altwijry): compile with OpenMP and run every
# version on 1, 2, 4 and 8 threads (Mac / Linux)
#
# Usage, from the repository root:
#     bash scripts/run_parallel.sh "Your Name"
#
# Output: results/parallel/<Your_Name>.csv
# Before running: laptop plugged in, other apps closed.
#
# Optional settings (environment variables):
#     RUNS=5 bash scripts/run_parallel.sh "Name"         fewer timed runs (default 10)
#     THREADS="1 2 4" bash scripts/run_parallel.sh "Name" other thread counts
#
# Mac: Apple clang has no OpenMP built in. Install it once with:
#     brew install libomp

set -euo pipefail
cd "$(dirname "$0")/.."

MEMBER="${1:-}"
if [ -z "$MEMBER" ]; then
    read -r -p "Your name in English (written to the CSV): " MEMBER
fi
[ -z "$MEMBER" ] && { echo "Error: a name is required."; exit 1; }

RUNS="${RUNS:-10}"            # timed runs per experiment (plus 1 warm-up)
THREADS="${THREADS:-1 2 4 8}"
VERSIONS="race critical atomic reduction"
SEED=42
DEG=3

# ---- compiler + OpenMP flags -------------------------------------------------
CXX="${CXX:-}"
OMP_FLAGS=()
case "$(uname -s)" in
    Darwin)
        CXX="${CXX:-clang++}"
        if ! command -v brew >/dev/null 2>&1 || ! brew --prefix libomp >/dev/null 2>&1; then
            echo "Error: OpenMP (libomp) not found. Run: brew install libomp"
            exit 1
        fi
        OMP="$(brew --prefix libomp)"
        OMP_FLAGS=(-Xpreprocessor -fopenmp -I"$OMP/include" -L"$OMP/lib" -lomp)
        ;;
    *)
        if [ -z "$CXX" ]; then
            for c in g++ clang++ c++; do
                if command -v "$c" >/dev/null 2>&1; then CXX="$c"; break; fi
            done
        fi
        OMP_FLAGS=(-fopenmp)
        ;;
esac
if [ -z "$CXX" ]; then
    echo "Error: no C++ compiler found. Linux: install g++."
    exit 1
fi

# ---- CPU model and logical CPUs ---------------------------------------------
CPU="unknown"; LOGICAL="?"
case "$(uname -s)" in
    Darwin)
        CPU="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo unknown)"
        LOGICAL="$(sysctl -n hw.logicalcpu 2>/dev/null || echo '?')" ;;
    Linux)
        CPU="$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d: -f2 | sed 's/^ *//' || true)"
        if [ -z "$CPU" ] && command -v lscpu >/dev/null 2>&1; then
            CPU="$(lscpu | grep -m1 'Model name' | cut -d: -f2 | sed 's/^ *//' || true)"
        fi
        [ -z "$CPU" ] && CPU="unknown"
        LOGICAL="$(nproc 2>/dev/null || echo '?')" ;;
esac

# ---- output file ------------------------------------------------------------
SAFE_NAME="$(printf '%s' "$MEMBER" | tr -c 'A-Za-z0-9_-' '_')"
mkdir -p results/parallel
OUT="results/parallel/${SAFE_NAME}.csv"
rm -f "$OUT"      # start fresh so a second attempt does not duplicate rows

echo "Member  : $MEMBER"
echo "CPU     : $CPU  ($LOGICAL logical CPUs)"
echo "Threads : $THREADS"
echo "Output  : $OUT"
echo "Compiling with $CXX -O2 ${OMP_FLAGS[*]} ..."
"$CXX" -std=c++17 -O2 -Isrc src/main.cpp src/generator.cpp src/trace.cpp \
       src/parallel_trace.cpp src/parallel_sync.cpp "${OMP_FLAGS[@]}" -o food_trace_omp

run() {   # nodes reported version threads
    ./food_trace_omp --nodes "$1" --reported "$2" --seed "$SEED" --deg "$DEG" --runs "$RUNS" \
                     --version "$3" --threads "$4" --member "$MEMBER" --cpu "$CPU" --out "$OUT"
}

# Three workloads: the Stage 1 reference size, the largest graph, and the
# heaviest tracing load (most reported batches).
for W in "100000 1000" "1000000 1000" "100000 10000"; do
    set -- $W
    run "$1" "$2" seq 1                       # sequential baseline for this workload
    for V in $VERSIONS; do
        for T in $THREADS; do
            run "$1" "$2" "$V" "$T"
        done
    done
done

echo
echo "Done. Results saved to $OUT"
