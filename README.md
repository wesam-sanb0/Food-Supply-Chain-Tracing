# Food Supply Chain Tracing

**Parallel Computing Project — Sequential BFS Tracing and Benchmarking**

## Project Overview

This project traces food supply chains using Breadth-First Search (BFS). It identifies suppliers connected to reported food batches and counts how many reported batches are associated with each supplier.

The current implementation provides a sequential baseline for correctness testing and performance benchmarking before parallelization.

## Project Structure

```text
Food-Supply-Chain-Tracing/
├── src/
│   ├── generator.cpp
│   ├── generator.h
│   ├── trace.cpp
│   ├── trace.h
│   ├── main.cpp
│   └── timer.h
├── scripts/
│   ├── run_all.bat
│   └── run_all.sh
├── tests/
│   ├── test_trace.cpp
│   ├── test_role4.cpp
│   ├── phase_check.cpp
│   ├── role4_test_results.txt
│   └── role4_additional_test_results.txt
├── results/
└── README.md
```

## Requirements

- C++17-compatible compiler (such as GCC or Clang)
- Windows, macOS, or Linux
- A terminal or command-line environment

## How to Run

Run the following commands from the repository root directory.

### Windows

```bat
scripts\run_all.bat "Your Name"
```

### macOS / Linux

```bash
bash scripts/run_all.sh "Your Name"
```

Replace `"Your Name"` with your name in English.

The script compiles the program with `-O2` optimization and runs six benchmark configurations. Each configuration includes one warm-up and ten timed runs.

Results are saved as a CSV file in the `results/` directory.

For consistent benchmarking, connect your laptop to power and close unnecessary applications.

## Benchmark Configurations

All experiments use seed `42` and average in-degree `3`.

| Nodes | Reported Batches | Expected Checksum |
|---|---|---|
| 10,000 | 1,000 | 964516 |
| 100,000 | 1,000 | 9120414 |
| 500,000 | 1,000 | 29257446 |
| 1,000,000 | 1,000 | 38315152 |
| 100,000 | 100 | 912107 |
| 100,000 | 10,000 | 91201863 |

Checksums must remain identical across runs and machines using the same configuration.

## Correctness Testing

### Run the Main Test Suite

Compile:

```bash
g++ -std=c++17 -O2 -Isrc tests/test_trace.cpp src/trace.cpp src/generator.cpp -o test_trace
```

Run on macOS/Linux:

```bash
./test_trace
```

On Windows, compile with `-o test_trace.exe` and run:

```powershell
.\test_trace.exe
```

The test suite includes eight correctness checks covering duplicate paths, disconnected batches, supplier identification, traversal direction, reference comparisons, and deterministic generation.

Expected output:

```text
ALL TESTS PASSED
```

### Run the Additional Role 4 Test

Compile:

```bash
g++ -std=c++17 -O2 -Isrc tests/test_role4.cpp src/trace.cpp src/generator.cpp -o test_role4
```

Run on macOS/Linux:

```bash
./test_role4
```

On Windows, compile with `-o test_role4.exe` and run:

```powershell
.\test_role4.exe
```

This test verifies supplier batch counts, common supplier identification, and checksum calculation.

Expected output:

```text
ROLE 4 TEST PASSED
```

## Testing Results

The integrated sequential implementation was tested on Windows.

- All eight main correctness tests passed.
- The additional Role 4 test passed.
- All six benchmark configurations completed successfully.
- The checksums matched the expected values across all timed runs.

Test outputs are saved in:

- `tests/role4_test_results.txt`
- `tests/role4_additional_test_results.txt`

## Benchmark Output

The benchmark produces CSV files containing:

`member, cpu_model, nodes, edges, reported, run, gen_ms, build_ms, trace_ms, agg_ms, total_ms, checksum`

The warm-up run is not saved to the CSV file. `total_ms` represents the sum of generation, reverse-graph building, tracing, and aggregation times.

## Project Status

The sequential baseline has passed integration and correctness testing on Windows and is ready for the next project phase.
