#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cassert>

// #include "hashes.h"  // your striped cuckoo header
// #include "cuckoo_refinable.h"  // your striped cuckoo header
#include "cuckoo_striped.h"  // your striped cuckoo header
#include "cuckoo_tx.h"  // your striped cuckoo header

// Helper to print messages with thread ID
void log(const std::string& msg) {
    std::cout << "[Thread " << std::this_thread::get_id() << "] " << msg << std::endl;
}

void simple_thread_safety_test() {
    cuckoo::cuckoo_striped<int>::configure(16, 4, 2, 10);
    cuckoo::cuckoo_striped<int> table(64);

    cuckoo::cuckoo_tx<int> table2(64);

    std::cout << table2.add(2) << std::endl;
    
    int k = 0;
    __transaction_atomic {
      try{  
        k = 1;
        int* a = new int[5];
        delete[] a;
        // delete[] a;
      }
      catch(...){
        __transaction_cancel;
      }
    }
    std::cout << k << std::endl;


    const int NUM_THREADS = 16;
    const int NUM_KEYS = 1000;

    auto insert_worker = [&](int tid) {
        // log("Starting insert_worker");
        for (int i = tid * NUM_KEYS; i < (tid + 1) * NUM_KEYS; ++i) {
            if(!table.add(i)){
              log("Failed to add " + std::to_string(i));
            }
            // if (table.add(i)) {
            //     if (i % 20 == 0) // log("Inserted key " + std::to_string(i));
            // } else {
            //     // log("Failed to insert key " + std::to_string(i));
            // }
        }
        // log("Finished insert_worker");
    };

    // Launch insert threads
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(insert_worker, t);
    }
    for (auto& th : threads) th.join();
    // log("All insert threads finished");

    // Verify all keys
    // log("Checking all keys");
    for (int i = 0; i < NUM_THREADS * NUM_KEYS; ++i) {
        if (!table.contains(i)) {
            log("Missing key: " + std::to_string(i));
        }
        assert(table.contains(i));
    }
    // log("All keys verified");

    // Remove keys
    auto remove_worker = [&](int tid) {
        // log("Starting remove_worker");
        for (int i = tid * NUM_KEYS; i < (tid + 1) * NUM_KEYS; ++i) {
            if (!table.contains(i)) {
                // log("WARNING: key " + std::to_string(i) + " not present before remove!");
            }
            bool removed = table.remove(i);
            if (!removed) {
                log("WARNING: failed to remove key " + std::to_string(i));
            } else {
                // log("Removed key " + std::to_string(i));
            }
        }
        // log("Finished remove_worker");
    };

    threads.clear();
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(remove_worker, t);
    }
    for (auto& th : threads) th.join();
    // log("All remove threads finished");

    std::cout << table.size() << std::endl;
    // log("Simple cuckoo_striped thread-safety test passed!");
}

int main() {
    simple_thread_safety_test();
    return 0;
}
