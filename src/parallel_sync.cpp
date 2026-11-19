// Reemas Alshraim - Assignment 2: atomic and private arrays + merge.
#include "parallel_trace.h"

#include <algorithm>
#ifdef _OPENMP
#include <omp.h>
#endif

std::vector<std::uint32_t> countSuppliersAtomic(const ReverseGraph& rg,
                                               const Graph& g, int threads) {
    if (threads < 1) throw std::invalid_argument("threads must be at least 1");
    std::vector<std::uint32_t> supplierCount(g.numSuppliers, 0);
    const auto& reported = g.reportedBatches;
    const long long numReported = static_cast<long long>(reported.size());

    #pragma omp parallel num_threads(threads)
    {
        std::vector<std::uint32_t> visited(g.nodeTypes.size(), 0);
        std::vector<std::uint32_t> queue, found;
        queue.reserve(1024);
        std::uint32_t stamp = 0;

        #pragma omp for schedule(dynamic, 1)
        for (long long b = 0; b < numReported; ++b) {
            // A stamp belongs to this thread's visited array only.
            if (++stamp == 0) {
                std::fill(visited.begin(), visited.end(), 0);
                stamp = 1;
            }
            found.clear();
            traceBatch(rg, g, reported[static_cast<std::size_t>(b)],
                       visited, stamp, queue, found);
            for (std::uint32_t s : found) {
                #pragma omp atomic update
                supplierCount[s]++;
            }
        }
    }
    return supplierCount;
}

std::vector<std::uint32_t> countSuppliersReduction(const ReverseGraph& rg,
                                                  const Graph& g, int threads) {
    if (threads < 1) throw std::invalid_argument("threads must be at least 1");
    std::vector<std::uint32_t> supplierCount(g.numSuppliers, 0);
    const auto& reported = g.reportedBatches;
    const long long numReported = static_cast<long long>(reported.size());
    // Unused rows remain zero if OpenMP creates fewer threads than requested.
    std::vector<std::vector<std::uint32_t>> localCounts(
        static_cast<std::size_t>(threads),
        std::vector<std::uint32_t>(g.numSuppliers, 0));

    #pragma omp parallel num_threads(threads)
    {
        int threadId = 0;
#ifdef _OPENMP
        threadId = omp_get_thread_num();
#endif
        auto& localCount = localCounts[static_cast<std::size_t>(threadId)];
        std::vector<std::uint32_t> visited(g.nodeTypes.size(), 0);
        std::vector<std::uint32_t> queue, found;
        queue.reserve(1024);
        std::uint32_t stamp = 0;

        #pragma omp for schedule(dynamic, 1)
        for (long long b = 0; b < numReported; ++b) {
            if (++stamp == 0) {
                std::fill(visited.begin(), visited.end(), 0);
                stamp = 1;
            }
            found.clear();
            traceBatch(rg, g, reported[static_cast<std::size_t>(b)],
                       visited, stamp, queue, found);
            for (std::uint32_t s : found) localCount[s]++;
        }
    } // All threads finish before any local arrays are read below.

    // Manual reduction: combine the private counts after parallel tracing.
    for (const auto& localCount : localCounts) {
        for (std::size_t s = 0; s < supplierCount.size(); ++s)
            supplierCount[s] += localCount[s];
    }
    return supplierCount;
}
