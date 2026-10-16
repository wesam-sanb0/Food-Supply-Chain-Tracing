// test_trace.cpp - Role 2 correctness tests
//   Part A: tiny graphs with answers worked out by hand
//   Part B: a real generated graph checked against an independent method
#include "../src/trace.h"
#include <cassert>
#include <iostream>
#include <queue>

using NT = NodeType;

static Graph make(std::vector<NT> types, std::vector<Edge> edges, std::vector<std::uint32_t> reported) {
    Graph g;
    g.nodeTypes = types;
    g.nodeLayers.assign(types.size(), 0);
    g.edges = edges;
    g.reportedBatches = reported;
    for (NT t : types) { if (t == NT::Supplier) g.numSuppliers++; if (t == NT::Batch) g.numBatches++; }
    return g;
}

// Independent method: search FORWARD from each supplier and count how many
// reported batches it reaches. Must give exactly the same counts.
static std::vector<std::uint32_t> referenceCounts(const Graph& g) {
    const std::size_t n = g.nodeTypes.size();
    std::vector<std::vector<std::uint32_t>> children(n);
    for (const Edge& e : g.edges) children[e.from].push_back(e.to);
    std::vector<char> isReported(n, 0);
    for (auto b : g.reportedBatches) isReported[b] = 1;
    std::vector<std::uint32_t> counts(g.numSuppliers, 0);
    for (std::uint32_t s = 0; s < g.numSuppliers; ++s) {
        std::vector<char> seen(n, 0);
        std::queue<std::uint32_t> q; q.push(s); seen[s] = 1;
        while (!q.empty()) {
            auto v = q.front(); q.pop();
            if (isReported[v]) counts[s]++;
            for (auto w : children[v]) if (!seen[w]) { seen[w] = 1; q.push(w); }
        }
    }
    return counts;
}

int main() {
    // ---------------- Part A: hand-checked graphs ----------------
    // 1) S0,S1 suppliers; I2<-S0; I3<-S0,S1; I4<-I2,I3; B5<-I4; B6<-I3
    //    B5 reaches S0 by two paths -> S0 must still be counted once for B5
    {
        Graph g = make({NT::Supplier, NT::Supplier, NT::Intermediate, NT::Intermediate, NT::Intermediate, NT::Batch, NT::Batch},
                       {{0,2},{0,3},{1,3},{2,4},{3,4},{4,5},{3,6}}, {5, 6});
        auto cnt = countSuppliers(buildReverse(g), g);
        assert(cnt[0] == 2 && cnt[1] == 2);
        auto sum = summarize(cnt, g.reportedBatches.size());
        assert((sum.commonSuppliers == std::vector<std::uint32_t>{0, 1}));
        assert(sum.checksum == 4);
        std::cout << "A1 duplicate paths + common suppliers ... OK\n";
    }
    // 2) a batch with no upstream links
    {
        Graph g = make({NT::Supplier, NT::Intermediate, NT::Batch}, {{0,1}}, {2});
        auto cnt = countSuppliers(buildReverse(g), g);
        assert(cnt[0] == 0 && summarize(cnt, 1).checksum == 0);
        std::cout << "A2 disconnected batch ..................... OK\n";
    }
    // 3) two batches with different suppliers -> no common supplier
    {
        Graph g = make({NT::Supplier, NT::Supplier, NT::Intermediate, NT::Intermediate, NT::Batch, NT::Batch},
                       {{0,2},{1,3},{2,4},{3,5}}, {4, 5});
        auto cnt = countSuppliers(buildReverse(g), g);
        auto sum = summarize(cnt, 2);
        assert(cnt[0] == 1 && cnt[1] == 1 && sum.commonSuppliers.empty());
        std::cout << "A3 no common supplier ..................... OK\n";
    }
    // 4) direction: only UPSTREAM nodes count (S1 feeds B4, not the reported B3)
    {
        Graph g = make({NT::Supplier, NT::Supplier, NT::Intermediate, NT::Batch, NT::Batch},
                       {{0,2},{2,3},{2,4},{1,4}}, {3});
        auto cnt = countSuppliers(buildReverse(g), g);
        assert(cnt[0] == 1 && cnt[1] == 0);
        std::cout << "A4 tracing direction ...................... OK\n";
    }

    // ---------------- Part B: real generator vs independent method ----------------
    for (std::size_t n : {2000, 10000, 30000}) {
        Graph g = generateGraph(n, 3.0, 200, 42);
        auto cnt = countSuppliers(buildReverse(g), g);
        auto ref = referenceCounts(g);
        assert(cnt == ref);
        std::cout << "B  generated graph n=" << n << " matches forward-search reference ... OK (checksum "
                  << summarize(cnt, g.reportedBatches.size()).checksum << ")\n";
    }
    // determinism: same settings -> same checksum
    {
        Graph g1 = generateGraph(50000, 3.0, 500, 42), g2 = generateGraph(50000, 3.0, 500, 42);
        auto c1 = summarize(countSuppliers(buildReverse(g1), g1), 500).checksum;
        auto c2 = summarize(countSuppliers(buildReverse(g2), g2), 500).checksum;
        assert(c1 == c2);
        std::cout << "B  same seed -> same checksum (" << c1 << ") ...... OK\n";
    }
    std::cout << "ALL TESTS PASSED\n";
}
