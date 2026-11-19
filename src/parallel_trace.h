// parallel_trace.h - Assignment 2: OpenMP versions of countSuppliers()
//
// Every version distributes the reported batches over threads with
//   #pragma omp for schedule(dynamic, 1)
// and gives every thread its OWN visited / queue / found / stamp, so the BFS
// itself is race-free. The versions differ ONLY in how the shared
// supplierCount array is updated:
//
//   Race      plain supplierCount[s]++            (race demo, results may be wrong)
//   Critical  #pragma omp critical                (Lama Aseeri)
//   Atomic    #pragma omp atomic                  (Reemas Alshraim)
//   Reduction per-thread count arrays + merge     (Reemas Alshraim)
//
// All correct versions must return exactly the same vector as the
// sequential countSuppliers() in trace.cpp.
#ifndef FOOD_TRACE_PARALLEL_TRACE_H
#define FOOD_TRACE_PARALLEL_TRACE_H

#include "trace.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Thrown by a version that is not written yet; the driver skips it.
struct NotImplemented : std::logic_error {
    using std::logic_error::logic_error;
};

enum class SyncMode { Sequential, Race, Critical, Atomic, Reduction };

// "seq", "race", "critical", "atomic", "reduction" <-> SyncMode
SyncMode parseSyncMode(const std::string& name);   // throws std::invalid_argument
const char* syncModeName(SyncMode mode);

// One entry point for the benchmark driver.
// Sequential ignores `threads` and calls the original countSuppliers().
std::vector<std::uint32_t> countSuppliersParallel(const ReverseGraph& rg, const Graph& g,
                                                  SyncMode mode, int threads);

// Individual versions (parallel_trace.cpp: Race, Critical; parallel_sync.cpp: Atomic, Reduction)
std::vector<std::uint32_t> countSuppliersRace(const ReverseGraph& rg, const Graph& g, int threads);
std::vector<std::uint32_t> countSuppliersCritical(const ReverseGraph& rg, const Graph& g, int threads);
std::vector<std::uint32_t> countSuppliersAtomic(const ReverseGraph& rg, const Graph& g, int threads);
std::vector<std::uint32_t> countSuppliersReduction(const ReverseGraph& rg, const Graph& g, int threads);

// True when the program was compiled with OpenMP (-fopenmp).
bool openmpEnabled();

#endif
