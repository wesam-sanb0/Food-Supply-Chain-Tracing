// trace.h - Role 2: sequential upstream tracing
// Uses the shared Graph / Edge / NodeType definitions from generator.h (Role 1).
#ifndef FOOD_TRACE_TRACE_H
#define FOOD_TRACE_TRACE_H

#include "generator.h"
#include <cstdint>
#include <utility>
#include <vector>

// Reverse adjacency list in compressed (CSR) form.
// The upstream nodes ("parents") of node v are
//   parents[offsets[v]] ... parents[offsets[v + 1] - 1]
struct ReverseGraph {
    std::vector<std::uint32_t> offsets;   // size N + 1
    std::vector<std::uint32_t> parents;   // size = number of edges
};

// Phase "build": forward edge list -> reverse adjacency list.
ReverseGraph buildReverse(const Graph& g);

// Traces ONE reported batch upstream with breadth-first search (BFS) and
// appends every distinct supplier it reaches to `found`.
//   visited : scratch array of size N; a node counts as visited in this trace
//             when visited[v] == stamp, so the array never needs clearing
//   stamp   : must be different for every call (e.g. query index + 1)
//   queue   : reusable scratch space for the BFS queue
void traceBatch(const ReverseGraph& rg, const Graph& g, std::uint32_t batch,
                std::vector<std::uint32_t>& visited, std::uint32_t stamp,
                std::vector<std::uint32_t>& queue, std::vector<std::uint32_t>& found);

// Phase "trace": the loop over all reported batches (the hotspot).
// Returns supplierCount[s] = number of reported batches whose trace reaches
// supplier s (s = 0 .. numSuppliers-1).
// THIS loop is what Assignment 2 / Stage 3 parallelise with OpenMP.
std::vector<std::uint32_t> countSuppliers(const ReverseGraph& rg, const Graph& g);

// Phase "aggregate": turn the counts into the final answer.
struct TraceSummary {
    std::vector<std::uint32_t> commonSuppliers;                    // reached by ALL reported batches
    std::vector<std::pair<std::uint32_t, std::uint32_t>> topShared; // (supplier, count), largest first
    std::uint64_t checksum = 0;     // sum of all counts: must be identical on every machine
};
TraceSummary summarize(const std::vector<std::uint32_t>& supplierCount,
                       std::size_t numReported, std::size_t topK = 10);

#endif
