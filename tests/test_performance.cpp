#include <iostream>
#include <vector>
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
constexpr int NUM_OPS = 10000000; // total per test

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

// Performs mixed operations
template<typename Table>
void mixed_operations(Table& table, int numOps) {
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> keyDist(0, MAX_KEY);
    uniform_real_distribution<double> opDist(0.0, 1.0);

    for (int i = 0; i < numOps; ++i) {
        double op = opDist(rng);
        int key = keyDist(rng);
        if (op < 0.8) {
            table.contains(key);
        } else if (op < 0.9) {
            table.add(key);
        } else {
            table.remove(key);
        }
    }
}

template<typename Table>
double benchmark_seq(Table& table, const vector<int>& initVals) {
    // cout << "Populating " << name << "..." << endl;
    table.populate(const_cast<vector<int>&>(initVals)); // populate modifies

    // cout << "Running " << name << " sequential benchmark..." << endl;
    Timer t;
    mixed_operations(table, NUM_OPS);
    double ms = t.elapsed_ms();

    // cout << name << " completed in " << ms << " ms" << endl;
    return ms;
}

template<typename Table>
double benchmark_concurrent(Table& table, int numThreads, const vector<int>& initVals) {
    // cout << "Populating " << name << "..." << endl;
    table.populate(const_cast<vector<int>&>(initVals));

    // cout << "Running " << name << " concurrent benchmark (" << numThreads << " threads)..." << endl;

    Timer t;
    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&table, numThreads]() {
            mixed_operations(table, NUM_OPS / numThreads);
        });
    }
    for (auto& th : threads)
        th.join();

    double ms = t.elapsed_ms();
    // cout << name << " completed in " << ms << " ms" << endl;
    return ms;
}

int main(int argc, char* argv[]) {
    size_t initial_capacity = 128;
    if (argc > 1) {
        try {
            initial_capacity = stoul(argv[1]);
        } catch (...) {
            cerr << "Invalid capacity argument, using default (128)." << endl;
        }
    }

    cout << "Generating test data..." << endl;
    auto initVals = random_ints(NUM_KEYS, MAX_KEY);

    // Configurable thread counts to test
    vector<int> thread_configs = {1, 2, 4, 8, 16};

    cout << "\n=== Cuckoo Hash Performance Test ===\n";
    cout << "Initial capacity: " << initial_capacity << endl;
    cout << "Total operations: " << NUM_OPS << " per configuration\n\n";

    // Column header
    cout << left << setw(20) << "Table Type"
         << setw(10) << "Threads"
         << setw(15) << "Time (ms)"
         << setw(15) << "Ops/sec"
         << endl;
    cout << string(60, '-') << endl;

    // Sequential (1-thread)
    {
        cuckoo_seq<int> seq_table(initial_capacity);
        double t = benchmark_seq(seq_table, initVals);
        cout << left << setw(20) << "cuckoo_seq"
             << setw(10) << 1
             << setw(15) << fixed << setprecision(2) << t
             << setw(15) << static_cast<long long>(NUM_OPS / (t / 1000.0))
             << endl;
    }

    // Multithreaded configurations
    for (int threads : thread_configs) {
        if (threads == 1) continue; // already did seq test

        // Striped
        {
            cuckoo_striped<int> striped(initial_capacity);
            double t = benchmark_concurrent(striped, threads, initVals);
            cout << left << setw(20) << "cuckoo_striped"
                 << setw(10) << threads
                 << setw(15) << fixed << setprecision(2) << t
                 << setw(15) << static_cast<long long>(NUM_OPS / (t / 1000.0))
                 << endl;
        }

        // Refinable
        {
            cuckoo_refinable<int> refinable(initial_capacity);
            double t = benchmark_concurrent(refinable, threads, initVals);
            cout << left << setw(20) << "cuckoo_refinable"
                 << setw(10) << threads
                 << setw(15) << fixed << setprecision(2) << t
                 << setw(15) << static_cast<long long>(NUM_OPS / (t / 1000.0))
                 << endl;
        }

        // Transactional
        {
            cuckoo_tx<int> tx(initial_capacity);
            double t = benchmark_concurrent(tx, threads, initVals);
            cout << left << setw(20) << "cuckoo_tx"
                 << setw(10) << threads
                 << setw(15) << fixed << setprecision(2) << t
                 << setw(15) << static_cast<long long>(NUM_OPS / (t / 1000.0))
                 << endl;
        }
    }

    cout << "\nBenchmark complete.\n";
    return 0;
}
