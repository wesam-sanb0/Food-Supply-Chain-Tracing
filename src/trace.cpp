// trace.cpp - Role 2: sequential upstream tracing
#include "trace.h"
#include <algorithm>

ReverseGraph buildReverse(const Graph& g) {
    const std::size_t n = g.nodeTypes.size();
    ReverseGraph rg;
    rg.offsets.assign(n + 1, 0);

    // 1) count how many parents (incoming edges) each node has
    for (const Edge& e : g.edges) rg.offsets[e.to + 1]++;
    // 2) prefix sum -> where each node's parent list starts
    for (std::size_t v = 0; v < n; ++v) rg.offsets[v + 1] += rg.offsets[v];
    // 3) place every edge's upstream node into its downstream node's list
    rg.parents.resize(g.edges.size());
    std::vector<std::uint32_t> next(rg.offsets.begin(), rg.offsets.end() - 1);
    for (const Edge& e : g.edges) rg.parents[next[e.to]++] = e.from;
    return rg;
}

void traceBatch(const ReverseGraph& rg, const Graph& g, std::uint32_t batch,
                std::vector<std::uint32_t>& visited, std::uint32_t stamp,
                std::vector<std::uint32_t>& queue, std::vector<std::uint32_t>& found) {
    queue.clear();
    queue.push_back(batch);
    visited[batch] = stamp;
    for (std::size_t head = 0; head < queue.size(); ++head) {          // BFS
        const std::uint32_t v = queue[head];
        if (g.nodeTypes[v] == NodeType::Supplier) found.push_back(v);  // visited once -> counted once
        for (std::uint32_t i = rg.offsets[v]; i < rg.offsets[v + 1]; ++i) {
            const std::uint32_t u = rg.parents[i];
            if (visited[u] != stamp) {                                 // not yet seen in THIS trace
                visited[u] = stamp;
                queue.push_back(u);
            }
        }
    }
}

std::vector<std::uint32_t> countSuppliers(const ReverseGraph& rg, const Graph& g) {
    std::vector<std::uint32_t> supplierCount(g.numSuppliers, 0);   // supplier IDs are 0..numSuppliers-1
    std::vector<std::uint32_t> visited(g.nodeTypes.size(), 0), queue, found;
    queue.reserve(1024);

    const auto& reported = g.reportedBatches;
    for (std::size_t b = 0; b < reported.size(); ++b) {           // <-- independent iterations
        found.clear();
        traceBatch(rg, g, reported[b], visited, static_cast<std::uint32_t>(b + 1), queue, found);
        for (std::uint32_t s : found) supplierCount[s]++;           // <-- shared update (race later)
    }
    return supplierCount;
}

TraceSummary summarize(const std::vector<std::uint32_t>& supplierCount,
                       std::size_t numReported, std::size_t topK) {
    TraceSummary out;
    for (std::uint32_t s = 0; s < supplierCount.size(); ++s) {
        const std::uint32_t c = supplierCount[s];
        if (c == 0) continue;
        out.checksum += c;
        if (c == numReported) out.commonSuppliers.push_back(s);
        out.topShared.push_back({s, c});
    }
    // largest count first; equal counts ordered by ID so the output is deterministic
    std::sort(out.topShared.begin(), out.topShared.end(), [](const auto& a, const auto& b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    if (out.topShared.size() > topK) out.topShared.resize(topK);
    return out;
}
