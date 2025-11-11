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

template<typename Table>
void mixed_operations(Table& table, int numOps, double read_ratio, double insert_ratio, double remove_ratio) {
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

template<typename Table>
double benchmark_seq(Table& table,
                     const vector<int>& initVals,
                     const string& name,
                     double read_ratio,
                     double insert_ratio,
                     double remove_ratio) {
    table.populate(const_cast<vector<int>&>(initVals));
    (void) name;
    Timer t;
    mixed_operations(table, NUM_OPS, read_ratio, insert_ratio, remove_ratio);
    return t.elapsed_ms();
  }
  
  template<typename Table>
  double benchmark_concurrent(Table& table,
                              int numThreads,
                              const vector<int>& initVals,
                              const string& name,
                              double read_ratio,
                              double insert_ratio,
                              double remove_ratio) {
    table.populate(const_cast<vector<int>&>(initVals));
      
    (void) name;
    Timer t;
    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            mixed_operations(table, NUM_OPS / numThreads, read_ratio, insert_ratio, remove_ratio);
        });
    }
    for (auto& th : threads) th.join();
    return t.elapsed_ms();
}

int main() {
    size_t initial_capacity = 128;
    cout << "Generating test data..." << endl;
    auto initVals = random_ints(NUM_KEYS, MAX_KEY);

    vector<tuple<double, double, double>> workloads = {
        {0.80, 0.10, 0.10},
        {0.90, 0.05, 0.05},
        {0.96, 0.02, 0.02},
        {0.98, 0.01, 0.01},
        {1.00, 0.00, 0.00}
    };

    cout << "\n=== Cuckoo Hash Performance Test ===" << endl;
    cout << "Initial capacity: " << initial_capacity << "\n\n";

    // Table header
    cout << left << setw(15) << "Workload"
         << setw(15) << "Seq (ms)"
         << setw(15) << "Striped (ms)"
         << setw(18) << "Refinable (ms)"
         << setw(18) << "Tx (ms)" << "\n";

    cout << string(15 + 15 + 15 + 18 + 18, '-') << "\n";

    for (auto [r, i, d] : workloads) {
        cuckoo_seq<int> seq(initial_capacity);
        cuckoo_striped<int> striped(initial_capacity);
        cuckoo_refinable<int> refinable(initial_capacity);
        cuckoo_tx<int> tx(initial_capacity);

        double seq_time = benchmark_seq(seq, initVals, "cuckoo_seq", r, i, d);
        double striped_time = benchmark_concurrent(striped, 16, initVals, "cuckoo_striped", r, i, d);
        double refined_time = benchmark_concurrent(refinable, 16, initVals, "cuckoo_refinable", r, i, d);
        double tx_time = benchmark_concurrent(tx, 16, initVals, "cuckoo_tx", r, i, d);

        // Print each row
        cout << fixed << setprecision(2)
             << setw(15) << (to_string((int)(r * 100)) + "/" +
                             to_string((int)(i * 100)) + "/" +
                             to_string((int)(d * 100)))
             << setw(15) << seq_time
             << setw(15) << striped_time
             << setw(18) << refined_time
             << setw(18) << tx_time
             << "\n";
    }

    cout << string(15 + 15 + 15 + 18 + 18, '-') << "\n";
    cout << "All tests complete.\n";
    return 0;
}