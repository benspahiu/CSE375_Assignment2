#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <atomic>
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
double benchmark_seq(Table& table, const vector<int>& initVals, const string& name) {
    cout << "Populating " << name << "..." << endl;
    //table.populate(const_cast<vector<int>&>(initVals)); // populate modifies
    cout << "Running " << name << " sequential benchmark..." << endl;

    Timer t;
    mixed_operations(table, NUM_OPS);
    double ms = t.elapsed_ms();

    cout << name << " completed in " << ms << " ms" << endl;
    return ms;
}

template<typename Table>
double benchmark_concurrent(Table& table, const vector<int>& initVals, const string& name) {
    cout << "Populating " << name << "..." << endl;
    //table.populate(const_cast<vector<int>&>(initVals));

    cout << "Running " << name << " concurrent benchmark (" << NUM_THREADS << " threads)..." << endl;
    Timer t;
    vector<thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&table, &initVals]() {
            mixed_operations(table, NUM_OPS / NUM_THREADS);
        });
    }
    for (auto& th : threads) th.join();
    double ms = t.elapsed_ms();

    cout << name << " completed in " << ms << " ms" << endl;
    return ms;
}

int main(int argc, char* argv[]) {
    size_t initial_capacity = 128; // default
    if (argc > 1) {
        try {
            initial_capacity = stoul(argv[1]);
        } catch (...) {
            cerr << "Invalid capacity argument, using default (128)." << endl;
        }
    }

    cout << "Generating test data..." << endl;
    auto initVals = random_ints(NUM_KEYS, MAX_KEY);

    cuckoo_seq<int> seq_table(initial_capacity);
    cuckoo_striped<int> striped_table(initial_capacity);
    cuckoo_refinable<int> refined_table(initial_capacity);
    cuckoo_tx<int> tx_table(initial_capacity);
 
    cout << "\n=== Cuckoo Hash Performance Test ===\n";
    cout << "Initial capacity: " << initial_capacity << endl;

    double seq_time = benchmark_seq(seq_table, initVals, "cuckoo_seq");
    double striped_time = benchmark_concurrent(striped_table, initVals, "cuckoo_striped");
    double refined_time = benchmark_concurrent(refined_table, initVals, "cuckoo_refinable");
    double tx_time = benchmark_concurrent(tx_table, initVals, "cuckoo_tx");

    cout << "\n=== Results (" << NUM_OPS << " ops) ===" << endl;
    cout << "Sequential: " << seq_time << " ms" << endl;
    cout << "Striped (" << NUM_THREADS << " threads): " << striped_time << " ms" << endl;
    cout << "Refinable (" << NUM_THREADS << " threads): " << refined_time << " ms" << endl;
    cout << "Transactional (" << NUM_THREADS << " threads): " << tx_time << " ms" << endl;

    return 0;
}