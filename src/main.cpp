// main.cpp - Role 3: benchmark driver for the sequential food-trace program
//
// One call = one experiment (one graph size + one number of reported batches):
//   1 warm-up run (printed, NOT written to the CSV)
//   then --runs timed runs, one CSV row per run.
//
// Each run times the four phases separately with std::chrono::steady_clock:
//   gen_ms   generateGraph()   (Role 1)
//   build_ms buildReverse()    (Role 2)
//   trace_ms countSuppliers()  (Role 2)  <- expected hotspot
//   agg_ms   summarize()       (Role 2)
//   total_ms = sum of the four phases (freeing memory afterwards is not timed)
//
// Usage:
//   food_trace --nodes 100000 --reported 1000 --seed 42 --runs 10
//              --member "Name" --cpu "CPU model" --out results/Name.csv
// Every option has a default, see printUsage().

#include "generator.h"
#include "timer.h"
#include "trace.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::size_t nodes = 100000;
    std::size_t reported = 1000;
    std::uint32_t seed = 42;
    double avgInDegree = 3.0;
    int runs = 10;
    std::string member = "unknown";
    std::string cpu = "unknown";
    std::string out = "results/results.csv";
};

struct RunResult {
    std::size_t edges = 0;
    double genMs = 0, buildMs = 0, traceMs = 0, aggMs = 0, totalMs = 0;
    std::uint64_t checksum = 0;
    std::size_t commonSuppliers = 0;
};

void printUsage() {
    std::cout <<
        "Usage: food_trace [options]\n"
        "  --nodes N        number of nodes             (default 100000)\n"
        "  --reported R     number of reported batches  (default 1000)\n"
        "  --seed S         random seed                 (default 42)\n"
        "  --deg D          average incoming edges      (default 3)\n"
        "  --runs K         timed runs after 1 warm-up  (default 10)\n"
        "  --member NAME    your name, written to the CSV\n"
        "  --cpu MODEL      CPU model, written to the CSV\n"
        "  --out FILE       CSV file to append to       (default results/results.csv)\n";
}

Options parseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") { printUsage(); std::exit(0); }
        if (i + 1 >= argc) throw std::invalid_argument("missing value after " + key);
        const std::string val = argv[++i];
        if      (key == "--nodes")    o.nodes = std::stoull(val);
        else if (key == "--reported") o.reported = std::stoull(val);
        else if (key == "--seed")     o.seed = static_cast<std::uint32_t>(std::stoul(val));
        else if (key == "--deg")      o.avgInDegree = std::stod(val);
        else if (key == "--runs")     o.runs = std::stoi(val);
        else if (key == "--member")   o.member = val;
        else if (key == "--cpu")      o.cpu = val;
        else if (key == "--out")      o.out = val;
        else throw std::invalid_argument("unknown option " + key);
    }
    if (o.runs < 1) throw std::invalid_argument("--runs must be at least 1");
    return o;
}

// One complete run of the program, each phase timed on its own.
// The graph and arrays are destroyed when the function returns, outside the timers.
RunResult runOnce(const Options& o) {
    RunResult r;
    Timer t;

    t.start();
    Graph g = generateGraph(o.nodes, o.avgInDegree, o.reported, o.seed);
    r.genMs = t.elapsedMs();

    t.start();
    ReverseGraph rg = buildReverse(g);
    r.buildMs = t.elapsedMs();

    t.start();
    std::vector<std::uint32_t> counts = countSuppliers(rg, g);
    r.traceMs = t.elapsedMs();

    t.start();
    TraceSummary s = summarize(counts, g.reportedBatches.size());
    r.aggMs = t.elapsedMs();

    r.totalMs = r.genMs + r.buildMs + r.traceMs + r.aggMs;
    r.edges = g.edges.size();
    r.checksum = s.checksum;
    r.commonSuppliers = s.commonSuppliers.size();
    return r;
}

// Quote a CSV field if it contains a comma, quote or newline.
std::string csvField(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
    std::string q = "\"";
    for (char c : s) { if (c == '"') q += '"'; q += c; }
    return q + "\"";
}

bool fileIsEmpty(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    return !in || in.tellg() == 0;
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

}  // namespace

int main(int argc, char** argv) {
    Options o;
    try {
        o = parseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        printUsage();
        return 1;
    }

    const bool writeHeader = fileIsEmpty(o.out);
    std::ofstream csv(o.out, std::ios::app);
    if (!csv) {
        std::cerr << "Error: cannot open " << o.out << " (does the folder exist?)\n";
        return 1;
    }
    if (writeHeader)
        csv << "member,cpu_model,nodes,edges,reported,run,gen_ms,build_ms,"
               "trace_ms,agg_ms,total_ms,checksum\n";
    csv << std::fixed << std::setprecision(3);
    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\n=== nodes=" << o.nodes << " reported=" << o.reported
              << " seed=" << o.seed << " deg=" << o.avgInDegree
              << " runs=" << o.runs << " (+1 warm-up) ===\n";

    try {
        // Warm-up: fills caches, triggers page faults and lets the CPU clock
        // ramp up. It is printed but not written to the CSV.
        const RunResult warm = runOnce(o);
        std::cout << "warm-up    total=" << warm.totalMs << " ms  edges=" << warm.edges
                  << "  checksum=" << warm.checksum
                  << "  common_suppliers=" << warm.commonSuppliers << "\n";

        std::vector<double> totals, traces;
        for (int run = 1; run <= o.runs; ++run) {
            const RunResult r = runOnce(o);

            // Same input must always give the same answer.
            if (r.checksum != warm.checksum) {
                std::cerr << "Error: checksum changed between runs (" << warm.checksum
                          << " vs " << r.checksum << "). Results are not reproducible.\n";
                return 2;
            }

            csv << csvField(o.member) << ',' << csvField(o.cpu) << ',' << o.nodes << ','
                << r.edges << ',' << o.reported << ',' << run << ',' << r.genMs << ','
                << r.buildMs << ',' << r.traceMs << ',' << r.aggMs << ',' << r.totalMs
                << ',' << r.checksum << '\n';
            csv.flush();

            std::cout << "run " << std::setw(2) << run << '/' << o.runs
                      << "  gen=" << r.genMs << "  build=" << r.buildMs
                      << "  trace=" << r.traceMs << "  agg=" << r.aggMs
                      << "  total=" << r.totalMs << " ms  checksum=" << r.checksum << "\n";
            totals.push_back(r.totalMs);
            traces.push_back(r.traceMs);
        }

        const double medTotal = median(totals);
        std::cout << "median total=" << medTotal << " ms  (min "
                  << *std::min_element(totals.begin(), totals.end()) << ", max "
                  << *std::max_element(totals.begin(), totals.end()) << ")  median trace="
                  << median(traces) << " ms (" << std::setprecision(1)
                  << 100.0 * median(traces) / medTotal << "% of total)\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (!csv) {
        std::cerr << "Error: failed while writing " << o.out << "\n";
        return 1;
    }
    return 0;
}
