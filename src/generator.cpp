#include "generator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace {
// Avoid implementation-specific uniform_int_distribution and shuffle results.
// Rejection sampling gives an unbiased integer in [0, bound).
std::uint32_t randomBelow(std::mt19937& rng, std::uint32_t bound) {
    const std::uint32_t threshold = (std::uint32_t(0) - bound) % bound;
    std::uint32_t value;
    do { value = static_cast<std::uint32_t>(rng()); }
    while (value < threshold);
    return value % bound;
}

const char* typeName(NodeType type) {
    switch (type) {
        case NodeType::Supplier: return "supplier";
        case NodeType::Intermediate: return "intermediate";
        case NodeType::Batch: return "batch";
    }
    return "unknown";
}
}

Graph generateGraph(std::size_t nodes, double avgInDegree,
                    std::size_t numReported, std::uint32_t seed) {
    if (nodes < 3 || nodes > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("nodes must be between 3 and UINT32_MAX");
    if (!std::isfinite(avgInDegree) || avgInDegree < 0)
        throw std::invalid_argument("avgInDegree must be finite and nonnegative");

    Graph g;
    g.numSuppliers = std::max<std::size_t>(1, nodes / 10);
    g.numBatches = std::max<std::size_t>(1, nodes / 5);
    if (numReported > g.numBatches)
        throw std::invalid_argument("numReported exceeds the number of batch nodes");
    const std::size_t intermediateCount = nodes - g.numSuppliers - g.numBatches;
    const std::size_t intermediateLayers = std::min<std::size_t>(8, intermediateCount);

    // Layer boundaries are half-open ranges [starts[i], starts[i+1]).
    std::vector<std::size_t> starts{0, g.numSuppliers};
    for (std::size_t i = 0; i < intermediateLayers; ++i) {
        const auto count = intermediateCount / intermediateLayers
                         + (i < intermediateCount % intermediateLayers ? 1 : 0);
        starts.push_back(starts.back() + count);
    }
    starts.push_back(nodes);
    g.nodeTypes.resize(nodes);
    g.nodeLayers.resize(nodes);
    for (std::size_t layer = 0; layer + 1 < starts.size(); ++layer) {
        const auto type = layer == 0 ? NodeType::Supplier
                        : layer + 2 == starts.size() ? NodeType::Batch
                        : NodeType::Intermediate;
        for (std::size_t v = starts[layer]; v < starts[layer + 1]; ++v) {
            g.nodeTypes[v] = type;
            g.nodeLayers[v] = static_cast<std::uint32_t>(layer);
        }
    }

    // Only adjacent layers are connected. No self-loop or cycle is possible.
    std::uint64_t capacity = 0;
    for (std::size_t layer = 1; layer + 1 < starts.size(); ++layer)
        capacity += static_cast<std::uint64_t>(starts[layer] - starts[layer - 1])
                  * (starts[layer + 1] - starts[layer]);
    const long double requested = std::floor(static_cast<long double>(avgInDegree)
                                             * nodes + 0.5L);
    const std::uint64_t edgeTarget = requested >= static_cast<long double>(capacity)
                                  ? capacity : static_cast<std::uint64_t>(requested);
    if (edgeTarget > g.edges.max_size())
        throw std::length_error("requested edge list is too large");
    g.edges.reserve(static_cast<std::size_t>(edgeTarget));
    std::mt19937 edgeRng(seed);
    std::uint64_t edgesLeft = edgeTarget;
    std::uint64_t capacityLeft = capacity;
    std::size_t nodesLeft = nodes - g.numSuppliers;

    for (std::size_t layer = 1; layer + 1 < starts.size(); ++layer) {
        const auto parentStart = starts[layer - 1];
        const auto parentCount = static_cast<std::uint32_t>(starts[layer] - parentStart);
        for (std::size_t v = starts[layer]; v < starts[layer + 1]; ++v) {
            capacityLeft -= parentCount;
            // Distribute the requested edges while respecting each node's capacity.
            const auto balanced = edgesLeft / nodesLeft + (edgesLeft % nodesLeft != 0);
            const auto minimum = edgesLeft > capacityLeft ? edgesLeft - capacityLeft : 0;
            const auto degree = static_cast<std::uint32_t>(std::max<std::uint64_t>(
                minimum, std::min<std::uint64_t>(parentCount, balanced)));

            // Floyd sampling: degree distinct parent indices in O(degree) draws.
            // Never iterate the hash set, so hash iteration order cannot affect output.
            std::unordered_set<std::uint32_t> selected;
            selected.reserve(degree);
            for (std::uint64_t j = parentCount - degree; j < parentCount; ++j) {
                auto candidate = randomBelow(edgeRng, static_cast<std::uint32_t>(j + 1));
                if (selected.count(candidate)) candidate = static_cast<std::uint32_t>(j);
                selected.insert(candidate);
                g.edges.push_back({static_cast<std::uint32_t>(parentStart + candidate),
                                   static_cast<std::uint32_t>(v)});
            }
            edgesLeft -= degree;
            --nodesLeft;
        }
    }

    // Separate stream: changing query count cannot change the underlying graph.
    // Partial Fisher-Yates also makes smaller query lists prefixes of larger ones.
    std::mt19937 batchRng(seed);
    std::vector<std::uint32_t> batches(g.numBatches);
    std::iota(batches.begin(), batches.end(),
              static_cast<std::uint32_t>(nodes - g.numBatches));
    g.reportedBatches.reserve(numReported);
    for (std::size_t i = 0; i < numReported; ++i) {
        const auto j = i + randomBelow(batchRng, static_cast<std::uint32_t>(g.numBatches - i));
        std::swap(batches[i], batches[j]);
        g.reportedBatches.push_back(batches[i]);
    }
    return g;
}

void saveGraphCsv(const Graph& g, const std::string& prefix) {
    std::ofstream nodes(prefix + "_nodes.csv"), edges(prefix + "_edges.csv"),
                  reported(prefix + "_reported.csv");
    if (!nodes || !edges || !reported)
        throw std::runtime_error("could not create CSV files; check output directory");
    nodes << "node_id,type,layer\n";
    for (std::size_t i = 0; i < g.nodeTypes.size(); ++i)
        nodes << i << ',' << typeName(g.nodeTypes[i]) << ',' << g.nodeLayers[i] << '\n';
    edges << "from,to\n";
    for (const auto& edge : g.edges) edges << edge.from << ',' << edge.to << '\n';
    reported << "batch_id\n";
    for (auto id : g.reportedBatches) reported << id << '\n';
    nodes.flush(); edges.flush(); reported.flush();
    if (!nodes || !edges || !reported) throw std::runtime_error("failed to write CSV data");
}
