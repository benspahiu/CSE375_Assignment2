#include <iostream>
#include <vector>
#include <tuple>
#include <random>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include "cuckoo_seq.h"
#include "cuckoo_striped.h"
#include "cuckoo_refinable.h"
#include "cuckoo_tx.h"

using namespace std;
using namespace cuckoo;

constexpr int NUM_KEYS = 500000;
constexpr int MAX_KEY = 1000000;
constexpr int NUM_OPS = 10000000;
constexpr int NUM_THREADS = 16;

// Helper to generate random integers
vector<int> random_ints(int count, int maxValue) {
    vector<int> data;
    data.reserve(count);
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> dist(0, maxValue);
    for (int i = 0; i < count; ++i)
        data.push_back(dist(rng));
    return data;
}

struct Timer {
    chrono::high_resolution_clock::time_point start;
    Timer() { reset(); }
    void reset() { start = chrono::high_resolution_clock::now(); }
    double elapsed_ms() const {
        auto end = chrono::high_resolution_clock::now();
        return chrono::duration<double, milli>(end - start).count();
    }
};

// Performs mixed operations with given ratios
template<typename Table>
void mixed_operations(Table& table, int numOps,
                      double read_ratio, double insert_ratio, double remove_ratio) {
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> keyDist(0, MAX_KEY);
    uniform_real_distribution<double> opDist(0.0, 1.0);

    (void) remove_ratio;

    for (int i = 0; i < numOps; ++i) {
        double op = opDist(rng);
        int key = keyDist(rng);
        if (op < read_ratio) {
            table.contains(key);
        } else if (op < read_ratio + insert_ratio) {
            table.add(key);
        } else {
            table.remove(key);
        }
    }
}

// Sequential benchmark
template<typename Table>
double benchmark_seq(Table& table,
                     const vector<int>& initVals,
                     double r, double i, double d) {
    table.populate(const_cast<vector<int>&>(initVals));
    Timer t;
    mixed_operations(table, NUM_OPS, r, i, d);
    return t.elapsed_ms();
}

// Concurrent benchmark
template<typename Table>
double benchmark_concurrent(Table& table,
                            const vector<int>& initVals,
                            double r, double i, double d) {
    table.populate(const_cast<vector<int>&>(initVals));
    Timer t;
    vector<thread> threads;
    for (int iTh = 0; iTh < NUM_THREADS; ++iTh) {
        threads.emplace_back([&table, r, i, d]() {
            mixed_operations(table, NUM_OPS / NUM_THREADS, r, i, d);
        });
    }
    for (auto& th : threads) th.join();
    return t.elapsed_ms();
}

int main() {
    size_t initial_capacity = 1024;
    cout << "Generating test data..." << endl;
    auto initVals = random_ints(NUM_KEYS, MAX_KEY);

    constexpr double READ_RATIO = 0.80;
    constexpr double INSERT_RATIO = 0.10;
    constexpr double REMOVE_RATIO = 0.10;

    vector<size_t> options = {8, 16, 32};

    cout << "\n=== Full Configuration Sweep ===" << endl;
    cout << "Workload: R/I/D = 80/10/10, Threads = " << NUM_THREADS << endl << endl;

    cout << left << setw(25) << "Config (R/P/T/L)"
         << setw(12) << "Seq (ms)"
         << setw(14) << "Striped (ms)"
         << setw(16) << "Refinable (ms)"
         << setw(14) << "Tx (ms)"
         << endl;
    cout << string(25 + 12 + 14 + 16 + 14, '-') << endl;

    for (size_t reloc : options) {
        for (size_t probe : options) {
            for (size_t thresh : options) {
                for (size_t limit : options) {
                    // Configure
                    cuckoo_seq<int>::configure(reloc);
                    cuckoo_striped<int>::configure(reloc, probe, thresh, limit);
                    cuckoo_refinable<int>::configure(reloc, probe, thresh, limit);
                    cuckoo_tx<int>::configure(reloc);

                    // Create tables
                    cuckoo_seq<int> seq(initial_capacity);
                    cuckoo_striped<int> striped(initial_capacity);
                    cuckoo_refinable<int> refinable(initial_capacity);
                    cuckoo_tx<int> tx(initial_capacity);

                    // Run benchmarks
                    double seq_t = benchmark_seq(seq, initVals, READ_RATIO, INSERT_RATIO, REMOVE_RATIO);
                    double striped_t = benchmark_concurrent(striped, initVals, READ_RATIO, INSERT_RATIO, REMOVE_RATIO);
                    double refinable_t = benchmark_concurrent(refinable, initVals, READ_RATIO, INSERT_RATIO, REMOVE_RATIO);
                    double tx_t = benchmark_concurrent(tx, initVals, READ_RATIO, INSERT_RATIO, REMOVE_RATIO);

                    cout << fixed << setprecision(2)
                         << setw(25)
                         << ("R=" + to_string(reloc) +
                             ",P=" + to_string(probe) +
                             ",T=" + to_string(thresh) +
                             ",L=" + to_string(limit))
                         << setw(12) << seq_t
                         << setw(14) << striped_t
                         << setw(16) << refinable_t
                         << setw(14) << tx_t
                         << endl;
                }
            }
        }
    }

    cout << string(25 + 12 + 14 + 16 + 14, '-') << endl;
    cout << "Total configurations tested: " << (options.size() * options.size() * options.size() * options.size()) << endl;
    cout << "All tests complete.\n";
}