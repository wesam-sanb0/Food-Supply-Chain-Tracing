# Food Supply Chain Tracing

**Parallel Computing Project — Sequential BFS Tracing , Benchmarking and OpenMP Parallelization**

## Project Overview

This project traces food supply chains using Breadth-First Search (BFS). It identifies suppliers connected to reported food batches and counts how many reported batches are associated with each supplier.

The project consists of two stages:

- **Assignment 1:** Sequential BFS implementation, correctness testing, and benchmarking.
- **Assignment 2:** OpenMP parallelization, race-condition demonstration, synchronization techniques, and performance benchmarking.

The sequential implementation serves as the reference for verifying the correctness of the parallel versions.

## Project Structure

```text
Food-Supply-Chain-Tracing/
├── src/
│   ├── generator.cpp
│   ├── generator.h
│   ├── trace.cpp
│   ├── trace.h
│   ├── parallel_trace.cpp
│   ├── parallel_trace.h
│   ├── parallel_sync.cpp
│   ├── main.cpp
│   └── timer.h
├── scripts/
│   ├── run_all.bat
│   ├── run_all.sh
│   ├── run_parallel.bat
│   └── run_parallel.sh
├── tests/
│   ├── test_trace.cpp
│   ├── test_role4.cpp
│   ├── test_parallel_sync.cpp
│   ├── phase_check.cpp
│   ├── role4_test_results.txt
│   ├── role4_additional_test_results.txt
│   └── evidence/
├── results/
│   ├── Wesam.csv
│   ├── Wesam_specs.txt
│   ├── role4_smoke.csv
│   └── parallel/
│       └── Wesam.csv
└── README.md
```

The structure above shows the main project files. Temporary binaries, incoming submission copies, and diagnostic files are excluded.

## Requirements

- C++17-compatible compiler
- Windows, macOS, or Linux
- Terminal or command-line environment
- OpenMP-compatible compiler for Assignment 2
- GCC with `-fopenmp` for the provided parallel build scripts

---

# Assignment 1: Sequential BFS

## Implementation

Assignment 1 implements a sequential BFS-based supplier tracing algorithm.

The program generates a food supply chain graph, identifies suppliers connected to reported batches, and calculates supplier occurrence counts.

## How to Run

Run the commands from the repository root directory.

### Windows

```bat
scripts\run_all.bat "Your Name"
```

### macOS / Linux

```bash
bash scripts/run_all.sh "Your Name"
```

Replace `"Your Name"` with your name in English.

The benchmark uses `-O2` optimization and runs six configurations, each with one warm-up and ten timed runs.

Results are saved in the `results/` directory.

For consistent benchmarking, connect the laptop to power and close unnecessary applications.

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

### Main Test Suite

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

### Additional Role 4 Test

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

## Assignment 1 Testing Results

The integrated sequential implementation was tested on Windows.

- All eight main correctness tests passed.
- The additional Role 4 test passed.
- All six benchmark configurations completed successfully.
- Checksums matched the expected values across timed runs.

Test outputs:

- `tests/role4_test_results.txt`
- `tests/role4_additional_test_results.txt`

## Sequential Benchmark Output

The sequential benchmark CSV contains:

```text
member,cpu_model,nodes,edges,reported,run,gen_ms,build_ms,trace_ms,agg_ms,total_ms,checksum
```

The warm-up run is not saved.

`total_ms` represents the sum of generation, reverse-graph building, tracing, and aggregation times.

---

# Assignment 2: OpenMP Parallelization

## Overview

Assignment 2 extends the sequential BFS implementation using OpenMP to investigate data races, synchronization methods, correctness, and parallel performance.

The sequential implementation from Assignment 1 is retained as the correctness reference.

## Parallel Implementations

Four parallel versions are included.

### 1. Race Version

The Race version intentionally updates shared supplier counters without synchronization.

This demonstrates how multiple threads can access and modify the same shared variable, producing incorrect or inconsistent results.

This version is used for demonstrating data races and is not considered a correct implementation.

### 2. Critical Version

The Critical version uses an OpenMP critical section to protect updates to shared supplier counters.

It prevents simultaneous execution of the protected update section, but synchronization can introduce contention.

### 3. Atomic Version

The Atomic version uses OpenMP atomic operations for shared supplier counter updates.

Atomic operations protect individual updates while avoiding the broader critical section used by the Critical version.

### 4. Reduction Version

The Reduction version uses private per-thread supplier count arrays.

Each thread accumulates counts locally, and the private results are merged after parallel execution.

This reduces contention on shared counters.

## Thread Safety

Each thread maintains private traversal state, including:

- Visited markers
- Queue
- Found markers
- Stamps

The graph is shared between threads for read-only access.

The synchronized implementations are checked against the sequential reference.

## Running Assignment 2

Run the following commands from the repository root.

### Windows

```bat
scripts\run_parallel.bat "Your Name"
```

### Linux / macOS

```bash
bash scripts/run_parallel.sh "Your Name"
```

On macOS, OpenMP may require a compatible compiler and installation of `libomp`.

The scripts compile the parallel implementation and execute benchmark configurations.

Results are saved under:

```text
results/parallel/
```

## Parallel Benchmark Configurations

The benchmark compares:

- Sequential reference
- Race
- Critical
- Atomic
- Reduction

The parallel versions are evaluated using:

```text
1, 2, 4, and 8 threads
```

The benchmark uses the same graph-generation settings as the sequential baseline to support comparison.

## Parallel Benchmark Output

The parallel CSV contains:

```text
member,cpu_model,nodes,edges,reported,version,threads,run,gen_ms,build_ms,trace_ms,agg_ms,total_ms,checksum,correct
```

Important fields:

- `version`: Implementation being evaluated.
- `threads`: Number of OpenMP threads.
- `trace_ms`: Tracing execution time.
- `agg_ms`: Aggregation time.
- `total_ms`: Total measured execution time.
- `checksum`: Sum used for result comparison.
- `correct`: Indicates whether the complete supplier count vector matches the sequential reference.

Local benchmark files:

```text
results/parallel/Wesam.csv
results/role4_smoke.csv
```

## Assignment 2 Correctness Testing

### Parallel Synchronization Test

The dedicated test file is:

```text
tests/test_parallel_sync.cpp
```

Compile on Windows using GCC with OpenMP support:

```powershell
g++ -std=c++17 -O2 -fopenmp -Isrc tests/test_parallel_sync.cpp src/generator.cpp src/trace.cpp src/parallel_trace.cpp src/parallel_sync.cpp -o test_parallel_sync.exe
```

Run:

```powershell
.\test_parallel_sync.exe
```

The test compares the complete `supplierCount` vector against the sequential implementation rather than relying only on checksum equality.

The following implementations were tested:

- Critical
- Atomic
- Reduction

Tests covered 1, 2, 4, and 8 threads, with repeated executions.

### Correctness Results

The completed parallel synchronization test passed:

```text
8/8 TESTS PASSED
```

Additional local smoke testing completed:

```text
16/16 TESTS PASSED
```

These smoke tests covered the sequential reference and the three synchronized implementations across the selected thread counts.

The Race version was tested separately to demonstrate incorrect results under concurrent execution.

For the tested configurations, the synchronized versions matched the sequential supplier counts.

## Race Condition Demonstration

The Race implementation intentionally omits synchronization.

Concurrent execution with multiple threads produced incorrect supplier counts in the completed experiments.

This demonstrates why shared counter updates require appropriate synchronization.

The Race implementation is included for analysis and comparison, not as a valid final algorithm.

## ThreadSanitizer Investigation

ThreadSanitizer was attempted under Windows Subsystem for Linux (WSL) to investigate possible data races.

The investigation encountered runtime issues, including:

```text
ThreadSanitizer: unexpected memory mapping
```

One test also produced a possible race report, but the diagnostic results could not be conclusively validated in the available environment.

Therefore, the ThreadSanitizer investigation is classified as **inconclusive**.

It does not establish that the synchronized implementations are free from all data races.

Functional correctness was evaluated separately through full supplier count vector comparisons.

## Performance Analysis

The benchmark results support comparison of:

- Execution time across implementations
- Execution time versus thread count
- Speedup relative to sequential execution
- Parallel efficiency
- Synchronization overhead
- Local machine versus Azure VM performance

Speedup is calculated as:

```text
Speedup = Sequential Time / Parallel Time
```

Parallel efficiency is calculated as:

```text
Efficiency = Speedup / Number of Threads
```

The final performance interpretation and Azure comparison are completed as separate team tasks using the collected benchmark data.

## Assignment 2 Testing Environment

Local tests were executed on:

```text
CPU: 13th Gen Intel Core i9-13900H
Logical CPUs: 20
Operating System: Windows
Compiler: GCC with OpenMP support
```

Parallel thread counts:

```text
1, 2, 4, 8
```

## Assignment 2 Status

The following work has been completed:

- OpenMP parallel implementations integrated.
- Race, Critical, Atomic, and Reduction versions compiled and executed.
- Full supplier count correctness comparisons completed.
- Parallel synchronization tests passed.
- Local benchmark results generated.
- ThreadSanitizer investigation attempted and documented.

Azure benchmarking, performance charts, and final report analysis are handled through the team's remaining assigned tasks.

---

## Repository Notes

Assignment 1 sequential source files and benchmark results are preserved.

Assignment 2 introduces parallel source files, additional scripts, correctness tests, and benchmark results without replacing the original sequential baseline.

Generated executables, temporary diagnostic files, and duplicate incoming submission folders are not required in the Git repository.
