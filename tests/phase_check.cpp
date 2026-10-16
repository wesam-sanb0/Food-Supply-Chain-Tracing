// phase_check.cpp - quick per-phase timing (NOT the official benchmark; that is Role 3's main.cpp)
// usage: ./phase_check nodes avgInDegree reported seed
#include "../src/trace.h"
#include <chrono>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::size_t n   = argc > 1 ? std::stoul(argv[1]) : 100000;
    double deg      = argc > 2 ? std::stod(argv[2])  : 3.0;
    std::size_t rep = argc > 3 ? std::stoul(argv[3]) : 1000;
    std::uint32_t seed = argc > 4 ? std::stoul(argv[4]) : 42;
    using clk = std::chrono::steady_clock;
    auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };

    auto t0 = clk::now(); Graph g = generateGraph(n, deg, rep, seed);
    auto t1 = clk::now(); ReverseGraph rg = buildReverse(g);
    auto t2 = clk::now(); auto counts = countSuppliers(rg, g);
    auto t3 = clk::now(); auto sum = summarize(counts, g.reportedBatches.size());
    auto t4 = clk::now();
    double total = ms(t0, t4);
    std::cout << "nodes=" << n << " edges=" << g.edges.size() << " reported=" << rep << "\n"
              << "  generate  " << ms(t0, t1) << " ms\n  build     " << ms(t1, t2) << " ms\n"
              << "  trace     " << ms(t2, t3) << " ms  (" << 100 * ms(t2, t3) / total << "% of total)\n"
              << "  aggregate " << ms(t3, t4) << " ms\n"
              << "  checksum=" << sum.checksum << "  common_suppliers=" << sum.commonSuppliers.size()
              << "  avg_suppliers_per_trace=" << double(sum.checksum) / rep << " of " << g.numSuppliers << "\n";
}
