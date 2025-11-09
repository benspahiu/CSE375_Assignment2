#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cassert>

#include "hashes.h"  // your striped cuckoo header
//#include "cuckoo_striped.h"  // your striped cuckoo header
namespace cuckoo{
class cuckoo_striped {
public:
    explicit cuckoo_striped(size_t capacity = 4)
        : capacity(next_power_of_two(capacity)),
          size_(0),
          table1(this->capacity, std::vector<int>(0)),
          table2(this->capacity, std::vector<int>(0)),
          mtx1(this->capacity),
          mtx2(this->capacity){}

    bool add(int key){
      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx1 at add " << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx2 at add " << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      size_t b1 = h1 & (capacity - 1);
      size_t b2 = h2 & (capacity - 1);

      int i = -1, h = -1;
      bool mustResize = false;
      if(present(key, b1, b2)) return false;

      std::vector<int>& set1 = table1[b1];
      std::vector<int>& set2 = table2[b2];
      
      if(set1.size() < THRESHOLD){
        set1.push_back(key);
        size_++;
        return true;
      }
      else if(set2.size() < THRESHOLD){
        set2.push_back(key);
        size_++;
        return true;
      }
      else if(set1.size() < PROBE_SIZE){
        set1.push_back(key);
        i = 0;
        h = b1;
      }
      else if(set2.size() < PROBE_SIZE){
        set2.push_back(key);
        i = 1;
        h = b2;
      }
      else{
        mustResize = true;
      }
      lock1.unlock();
      std::cout << "[Thread " << std::this_thread::get_id() << "] unlocked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      lock2.unlock();
      std::cout << "[Thread " << std::this_thread::get_id() << "] unlocked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;
      
      if(mustResize){
        std::cout << "[Thread " << std::this_thread::get_id() << "] mustResize " << std::endl;
        
        resize();
        add(key);
      }
      else if(!relocate(i, h)){
        std::cout << "[Thread " << std::this_thread::get_id() << "] !relocate " << std::endl;
        resize();
      }
      else size_++;
      return true;
    }

    bool contains(const int key) const{
      // TODO try shared mutexes
      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      // std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);

      std::cout << "[Thread " << std::this_thread::get_id() << "] Contains trying to lock stripe mtx1 at contains" << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Contains locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      std::cout << "[Thread " << std::this_thread::get_id() << "] Contains trying to lock stripe mtx2 at contains" << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Contains locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      return present(key, h1 & (capacity - 1), h2 & (capacity - 1));
    }

    bool remove(const int key){
      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      // std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      
      std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx1 at remove" << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx2 at remove" << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      size_t b1 = h1 & (capacity - 1);
      std::vector<int>& set1 = table1[b1];
      auto it = std::find(set1.begin(), set1.end(), key);
      if(it != set1.end()){
        set1.erase(it);
        size_--;
        return true;
      }
      
      size_t b2 = h2 & (capacity - 1);
      std::vector<int>& set2 = table2[b2];
      it = std::find(set2.begin(), set2.end(), key);
      if(it != set2.end()){
        set2.erase(it);
        size_--;
        return true;
      }
      return false;
    }

    size_t size() const { 
      return size_; 
    }

    void populate(std::vector<int>& values){
      for(int key : values){
        add(key);
      }
      return;
    }

private:
    void resize(){
      if (capacity > (1 << 25)) throw std::runtime_error("Hash table too large. Check hash function");
      size_t oldCapacity = capacity;
      std::cout << "[Thread " << std::this_thread::get_id() << "] Locking all" << std::endl;
      std::vector<std::unique_lock<std::recursive_mutex>> locks;
      for(std::recursive_mutex& mtx : mtx1){
        locks.push_back(std::unique_lock<std::recursive_mutex>(mtx));
      }
      std::cout << "[Thread " << std::this_thread::get_id() << "] Done locking all" << std::endl;
      if(capacity != oldCapacity)
        return;

      std::cout << "Readding " << size_ << " els" << std::endl;
      size_ = 0; //updated by add

      std::vector<std::vector<int>> old_table1 = std::move(table1);
      std::vector<std::vector<int>> old_table2 = std::move(table2);
      capacity = oldCapacity * 2;
      table1.assign(capacity, std::vector<int>(0));
      table2.assign(capacity, std::vector<int>(0));

      for(std::vector<int>& set : old_table1){
        for(int z : set){
          std::cout << "[Thread " << std::this_thread::get_id() << "]  adding " << z << std::endl;
          add(z);
        }
      }
      for(std::vector<int>& set : old_table2){
        for(int z : set){
          std::cout << "[Thread " << std::this_thread::get_id() << "]  adding " << z << std::endl;
          add(z);
        }
      }
    }

    bool present(int key, size_t b1, size_t b2) const{
      const std::vector<int>& set1 = table1[b1];
      auto it1 = std::find(set1.begin(), set1.end(), key);
      if(it1 != set1.end()) return true;
      
      const std::vector<int>& set2 = table2[b2];
      auto it2 = std::find(set2.begin(), set2.end(), key);
      return it2 != set2.end();
    }

    bool relocate(int i, int hi){
      int hj = 0;
      int j = 1 - i;
      for(size_t round = 0; round < LIMIT; round++){
        std::cout << "[Thread " << std::this_thread::get_id() << "] Relocate round " << round << std::endl;
        std::vector<int>& iSet = ((i == 0) ? table1 : table2)[hi];
        std::cout << "[Thread " << std::this_thread::get_id() << "] Got iSet " << std::endl;
        int y = iSet[0];
        std::cout << "[Thread " << std::this_thread::get_id() << "] Got y = " << y << std::endl;
        
        size_t hj1 = hash1(y);
        size_t hj2 = hash2(y);
        
        std::cout << "[Thread " << std::this_thread::get_id() << "] Locking in relocate "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
        std::unique_lock<std::recursive_mutex> lock1(mtx1[hj1 & (mtx1.size() - 1)]);
        std::unique_lock<std::recursive_mutex> lock2(mtx2[hj2 & (mtx2.size() - 1)]);
        std::cout << "[Thread " << std::this_thread::get_id() << "] Done locking in relocate "<< std::endl;
        
        hj = (i != 0) ? (hj1 & (capacity - 1)) : (hj2 & (capacity - 1));
        std::cout << "[Thread " << std::this_thread::get_id() << "] Calculated hj = " << hj << std::endl;
        std::vector<int>& jSet = ((i != 0) ? table1 : table2)[hj];
        
        auto it = std::find(iSet.begin(), iSet.end(), y);
        if(it != iSet.end()){
          iSet.erase(it);
          if(jSet.size() < THRESHOLD){
            jSet.push_back(y);
            std::cout << "[Thread " << std::this_thread::get_id() << "] relocate unlocking "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
            return true;
          }
          else if(jSet.size() < PROBE_SIZE){
            jSet.push_back(y);
            i = 1 - i;
            hi = hj;
            j = 1 - j;
          }
          else{
            iSet.push_back(y);
            std::cout << "[Thread " << std::this_thread::get_id() << "] relocate unlocking "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
            return false;
          }
        }
        else if(iSet.size() >= THRESHOLD){
          std::cout << "[Thread " << std::this_thread::get_id() << "] relocate unlocking "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
          continue;
        }
        else{
          std::cout << "[Thread " << std::this_thread::get_id() << "] relocate unlocking "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
          return true;
        }
        std::cout << "[Thread " << std::this_thread::get_id() << "] relocate unlocking "<< (hj1 & (mtx1.size() - 1)) << " " << (hj2 & (mtx2.size() - 1)) << std::endl;
      }
      return false;
    }

    size_t next_power_of_two(size_t n){
      if (n == 0) return 1;
      // If already a power of two, return n
      if ((n & (n - 1)) == 0) return n;
      // Otherwise, round up
      size_t power = 1;
      while (power < n) power <<= 1;
      return power;
    }

    // size_t bucket1(const Key& key) const {
    //   return hash1(key) & (capacity - 1); // % capacity (since power of 2)
    // }

    // size_t bucket2(const Key& key) const {
    //   return hash2(key) & (capacity - 1); // % capacity (since power of 2)
    // }

    std::atomic<size_t> capacity; // must be power of two
    std::atomic<size_t> size_;
    std::vector<std::vector<int>> table1;
    std::vector<std::vector<int>> table2;
    
    mutable std::vector<std::recursive_mutex> mtx1;
    mutable std::vector<std::recursive_mutex> mtx2;

    myhash::StdHash1<int> hash1;
    myhash::StdHash2<int> hash2;

    static constexpr size_t MAX_RELOCATIONS = 16;
    static constexpr size_t PROBE_SIZE = 4;
    static constexpr size_t THRESHOLD = 2;
    static constexpr size_t LIMIT = 10;
};
};

// Helper to print messages with thread ID
void log(const std::string& msg) {
    std::cout << "[Thread " << std::this_thread::get_id() << "] " << msg << std::endl;
}

void simple_thread_safety_test() {
    cuckoo::cuckoo_striped table(1);

    const int NUM_THREADS = 4;
    const int NUM_KEYS = 100;

    auto insert_worker = [&](int tid) {
        log("Starting insert_worker");
        for (int i = tid * NUM_KEYS; i < (tid + 1) * NUM_KEYS; ++i) {
            if (table.add(i)) {
                if (i % 20 == 0) log("Inserted key " + std::to_string(i));
            } else {
                log("Failed to insert key " + std::to_string(i));
            }
        }
        log("Finished insert_worker");
    };

    // Launch insert threads
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(insert_worker, t);
    }
    for (auto& th : threads) th.join();
    log("All insert threads finished");

    // Verify all keys
    log("Checking all keys");
    for (int i = 0; i < NUM_THREADS * NUM_KEYS; ++i) {
        if (!table.contains(i)) {
            log("Missing key: " + std::to_string(i));
        }
        assert(table.contains(i));
    }
    log("All keys verified");

    // Remove keys
    auto remove_worker = [&](int tid) {
        log("Starting remove_worker");
        for (int i = tid * NUM_KEYS; i < (tid + 1) * NUM_KEYS; ++i) {
            if (!table.contains(i)) {
                log("WARNING: key " + std::to_string(i) + " not present before remove!");
            }
            bool removed = table.remove(i);
            if (!removed) {
                log("WARNING: failed to remove key " + std::to_string(i));
            } else {
                log("Removed key " + std::to_string(i));
            }
        }
        log("Finished remove_worker");
    };

    threads.clear();
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(remove_worker, t);
    }
    for (auto& th : threads) th.join();
    log("All remove threads finished");

    std::cout << table.size() << std::endl;

    assert(table.size() == 0);
    log("Simple cuckoo_striped thread-safety test passed!");
}

int main() {
    simple_thread_safety_test();
    return 0;
}
