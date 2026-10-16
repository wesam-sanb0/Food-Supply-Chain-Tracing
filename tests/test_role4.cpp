
#include "trace.h"
#include <cassert>
#include <iostream>

int main() {
    // Create a small graph with 5 nodes
    Graph g;

    g.nodeTypes = {
        NodeType::Supplier,      // 0
        NodeType::Supplier,      // 1
        NodeType::Intermediate,  // 2
        NodeType::Batch,         // 3
        NodeType::Batch          // 4
    };

    g.numSuppliers = 2;
    g.numBatches = 2;

    // Supplier 0 supplies both batches through node 2.
    // Supplier 1 supplies only Batch 4.
    g.edges = {
        {0, 2},
        {2, 3},
        {2, 4},
        {1, 4}
    };

    g.reportedBatches = {3, 4};

    // Run Role 2's algorithm
    ReverseGraph rg = buildReverse(g);
    auto counts = countSuppliers(rg, g);
    auto summary = summarize(counts, g.reportedBatches.size());

    // Verify expected results
    assert(counts[0] == 2);
    assert(counts[1] == 1);
    assert(summary.checksum == 3);
    assert(summary.commonSuppliers.size() == 1);
    assert(summary.commonSuppliers[0] == 0);

    std::cout << "Role 4: Shared Supplier Count Test ... PASS\n";
    std::cout << "Supplier 0 count: " << counts[0] << "\n";
    std::cout << "Supplier 1 count: " << counts[1] << "\n";
    std::cout << "Checksum: " << summary.checksum << "\n";
    std::cout << "ROLE 4 TEST PASSED\n";

    return 0;
}
