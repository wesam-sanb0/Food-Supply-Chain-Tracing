#ifndef FOOD_TRACE_GENERATOR_H
#define FOOD_TRACE_GENERATOR_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class NodeType : std::uint8_t { Supplier, Intermediate, Batch };

struct Edge {
    std::uint32_t from;  // upstream node
    std::uint32_t to;    // downstream node
};

struct Graph {
    std::vector<NodeType> nodeTypes;        // node IDs are indices, 0 .. N-1
    std::vector<std::uint32_t> nodeLayers;  // strictly increases along edges
    std::vector<Edge> edges;               // forward edge list, not reverse CSR
    std::vector<std::uint32_t> reportedBatches;
    std::size_t numSuppliers = 0;          // supplier IDs: 0 .. numSuppliers-1
    std::size_t numBatches = 0;            // batch IDs: N-numBatches .. N-1
};

// avgInDegree means target edges / ALL nodes, including suppliers.
// Tiny/dense requests are capped at the possible number of unique edges.
// nodes >= 3; finite avgInDegree >= 0; numReported <= numBatches.
// Throws std::invalid_argument or std::length_error for invalid parameters.
Graph generateGraph(std::size_t nodes, double avgInDegree,
                    std::size_t numReported, std::uint32_t seed);

// Optional inspection export; do not include file I/O in benchmark timings.
// Creates prefix_nodes.csv, prefix_edges.csv and prefix_reported.csv.
void saveGraphCsv(const Graph& graph, const std::string& prefix);

#endif
