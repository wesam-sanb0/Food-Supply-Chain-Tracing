// parallel_trace.cpp - Assignment 2 (Lama Aseeri): OpenMP race and critical versions
//
// Shared (read-only during tracing): g, rg, g.reportedBatches
// Shared (WRITTEN by every thread):  supplierCount          <-- RACE 1
// Private per thread:                visited, queue, found, stamp
//
// Why visited / queue / found / stamp must be private (RACE 2 and RACE 3):
// the sequential countSuppliers() reuses ONE visited array, ONE queue and ONE
// found list for every batch, and uses the loop index b + 1 as the stamp.
// If threads shared them, one thread could pop or overwrite another thread's
// queue entries, mix suppliers of different batches in `found`, and write its
// stamp over another thread's stamp in `visited`, so a node would look
// "already visited" in a trace that never reached it. Giving each thread its
// own copies removes those races without any synchronisation.
#include "parallel_trace.h"

#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

bool openmpEnabled() {
#ifdef _OPENMP
    return true;
#else
    return false;
#endif
}

SyncMode parseSyncMode(const std::string& name) {
    if (name == "seq" || name == "sequential") return SyncMode::Sequential;
    if (name == "race")      return SyncMode::Race;
    if (name == "critical")  return SyncMode::Critical;
    if (name == "atomic")    return SyncMode::Atomic;
    if (name == "reduction") return SyncMode::Reduction;
    throw std::invalid_argument("unknown version '" + name +
                                "' (use seq, race, critical, atomic or reduction)");
}

const char* syncModeName(SyncMode mode) {
    switch (mode) {
        case SyncMode::Sequential: return "seq";
        case SyncMode::Race:       return "race";
        case SyncMode::Critical:   return "critical";
        case SyncMode::Atomic:     return "atomic";
        case SyncMode::Reduction:  return "reduction";
    }
    return "unknown";
}

std::vector<std::uint32_t> countSuppliersParallel(const ReverseGraph& rg, const Graph& g,
                                                  SyncMode mode, int threads) {
    if (threads < 1) throw std::invalid_argument("threads must be at least 1");
    switch (mode) {
        case SyncMode::Sequential: return countSuppliers(rg, g);
        case SyncMode::Race:       return countSuppliersRace(rg, g, threads);
        case SyncMode::Critical:   return countSuppliersCritical(rg, g, threads);
        case SyncMode::Atomic:     return countSuppliersAtomic(rg, g, threads);
        case SyncMode::Reduction:  return countSuppliersReduction(rg, g, threads);
    }
    throw std::invalid_argument("unknown version");
}

// ---------------------------------------------------------------------------
// Version 1: NO synchronisation - race-condition demonstration only.
// Two threads can read the same supplierCount[s], both add 1 and both write
// back, so one increment is lost. The checksum can then be lower than the
// sequential checksum and can change from run to run. Never report a speedup
// for this version.
// ---------------------------------------------------------------------------
std::vector<std::uint32_t> countSuppliersRace(const ReverseGraph& rg, const Graph& g, int threads) {
    std::vector<std::uint32_t> supplierCount(g.numSuppliers, 0);    // SHARED
    const auto& reported = g.reportedBatches;
    const long long numReported = static_cast<long long>(reported.size());

    #pragma omp parallel num_threads(threads)
    {
        std::vector<std::uint32_t> visited(g.nodeTypes.size(), 0);   // PRIVATE
        std::vector<std::uint32_t> queue, found;                     // PRIVATE
        queue.reserve(1024);
        std::uint32_t stamp = 0;                                      // PRIVATE

        #pragma omp for schedule(dynamic, 1)
        for (long long b = 0; b < numReported; ++b) {
            found.clear();
            traceBatch(rg, g, reported[static_cast<std::size_t>(b)], visited, ++stamp, queue, found);
            for (std::uint32_t s : found)
                supplierCount[s]++;          // <-- RACE 1: unprotected read-modify-write
        }
    }
    return supplierCount;
}

// ---------------------------------------------------------------------------
// Version 2: critical section.
// Only one thread at a time may enter the block, so no increment is lost.
// The lock is taken once per batch (around the whole update loop), not once
// per supplier, to limit how often threads wait. The BFS itself still runs in
// parallel; only the counting is serialised.
// ---------------------------------------------------------------------------
std::vector<std::uint32_t> countSuppliersCritical(const ReverseGraph& rg, const Graph& g, int threads) {
    std::vector<std::uint32_t> supplierCount(g.numSuppliers, 0);    // SHARED
    const auto& reported = g.reportedBatches;
    const long long numReported = static_cast<long long>(reported.size());

    #pragma omp parallel num_threads(threads)
    {
        std::vector<std::uint32_t> visited(g.nodeTypes.size(), 0);   // PRIVATE
        std::vector<std::uint32_t> queue, found;                     // PRIVATE
        queue.reserve(1024);
        std::uint32_t stamp = 0;                                      // PRIVATE

        #pragma omp for schedule(dynamic, 1)
        for (long long b = 0; b < numReported; ++b) {
            found.clear();
            traceBatch(rg, g, reported[static_cast<std::size_t>(b)], visited, ++stamp, queue, found);
            #pragma omp critical(supplier_count_update)
            {
                for (std::uint32_t s : found) supplierCount[s]++;    // protected
            }
        }
    }
    return supplierCount;
}
