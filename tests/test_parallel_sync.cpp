#include "parallel_trace.h"
#include <iostream>
#include <stdexcept>

void check(const Graph& g, const char* label) {
    const auto rg = buildReverse(g);
    const auto expected = countSuppliers(rg, g);
    for (int threads : {1, 2, 4, 8}) {
        for (int repeat = 0; repeat < 3; ++repeat) {
            if (countSuppliersAtomic(rg, g, threads) != expected ||
                countSuppliersReduction(rg, g, threads) != expected)
                throw std::runtime_error("supplier counts differ");
        }
    }
    std::cout << label << ": PASS (full vectors, 1/2/4/8 threads, 3 repeats)\n";
}
int main() {
    Graph g;
    g.numSuppliers = 2;
    g.nodeTypes = {NodeType::Supplier, NodeType::Supplier,
                   NodeType::Intermediate, NodeType::Intermediate,
                   NodeType::Batch, NodeType::Batch, NodeType::Batch};
    g.edges = {{0,2},{0,3},{1,3},{2,4},{3,4},{3,5}};
    g.reportedBatches = {4,5,6};
    if (countSuppliers(buildReverse(g), g) != std::vector<std::uint32_t>{2,2})
        throw std::runtime_error("tiny graph reference wrong");
    check(g, "Duplicate paths and disconnected batch");
    g.reportedBatches.clear(); check(g, "No reported batches");
    check(Graph{}, "Empty graph");
    for (int n : {2000,10000,100000})
        check(generateGraph(n,3,n < 10000 ? 100 : 1000,42), "Generated graph");
    check(generateGraph(10000,0,100,7), "No edges");
    check(generateGraph(10000,3,1,99), "One batch, extra threads");
    for (auto fn : {countSuppliersAtomic, countSuppliersReduction}) {
        bool threw = false;
        try { fn(buildReverse(g),g,0); } catch (const std::invalid_argument&) { threw=true; }
        if (!threw) throw std::runtime_error("invalid thread count accepted");
    }
    std::cout << "ALL PARALLEL SYNC TESTS PASSED\n";
}
