#pragma once
#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include "hashes.h"

namespace cuckoo {

template<myhash::HashableAndEquatable Key>
class cuckoo_striped {
public:
    explicit cuckoo_striped(size_t capacity = 16)
        : capacity(next_power_of_two(capacity)),
          size_(0),
          table1(this->capacity, std::vector<Key>(0)),
          table2(this->capacity, std::vector<Key>(0)),
          mtx1(this->capacity),
          mtx2(this->capacity){}

    template<std::convertible_to<Key> K>
    bool add(K&& keyParam){
      Key key = std::forward<K>(keyParam);

      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx1 at add " << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx2 at add " << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      size_t b1 = h1 & (capacity - 1);
      size_t b2 = h2 & (capacity - 1);

      int i = -1, h = -1;
      bool mustResize = false;
      if(present(key, b1, b2)) return false;

      std::vector<Key>& set1 = table1[b1];
      std::vector<Key>& set2 = table2[b2];
      
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
      // std::cout << "[Thread " << std::this_thread::get_id() << "] unlocked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      lock2.unlock();
      // std::cout << "[Thread " << std::this_thread::get_id() << "] unlocked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      if(mustResize){
        resize();
        add(key);
      }
      else if(!relocate(i, h)){
        resize();
      }
      else size_++;
      return true;
    }

    bool contains(const Key& key) const{
      // TODO try shared mutexes
      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      // std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);

      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx1 at contains" << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx2 at contains" << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      return present(key, h1 & (capacity - 1), h2 & (capacity - 1));
    }

    bool remove(const Key& key){
      size_t h1 = hash1(key);
      size_t h2 = hash2(key);

      // std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx1 at remove" << (h1 & (mtx1.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock1(mtx1[h1 & (mtx1.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx1 " << (h1 & (mtx1.size() - 1)) << std::endl;
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Trying to lock stripe mtx2 at remove" << (h2 & (mtx2.size() - 1)) << std::endl;
      std::unique_lock<std::recursive_mutex> lock2(mtx2[h2 & (mtx2.size() - 1)]);
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locked stripe mtx2 " << (h2 & (mtx2.size() - 1)) << std::endl;

      size_t b1 = h1 & (capacity - 1);
      std::vector<Key>& set1 = table1[b1];
      auto it = std::find(set1.begin(), set1.end(), key);
      if(it != set1.end()){
        set1.erase(it);
        size_--;
        return true;
      }
      
      size_t b2 = h2 & (capacity - 1);
      std::vector<Key>& set2 = table2[b2];
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

    void populate(std::vector<Key>& values){
      for(Key& key : values){
        add(key);
      }
      return;
    }

private:
    void resize(){
      if (capacity > (1 << 25)) throw std::runtime_error("Hash table too large. Check hash function");
      size_t oldCapacity = capacity;
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Locking all" << std::endl;
      std::vector<std::unique_lock<std::recursive_mutex>> locks;
      for(std::recursive_mutex& mtx : mtx1){
        locks.push_back(std::unique_lock<std::recursive_mutex>(mtx));
      }
      // std::cout << "[Thread " << std::this_thread::get_id() << "] Done locking all" << std::endl;
      if(capacity != oldCapacity)
        return;

      // std::cout << "Readding " << size_ << " els" << std::endl;
      size_ = 0; //updated by add

      std::vector<std::vector<Key>> old_table1 = std::move(table1);
      std::vector<std::vector<Key>> old_table2 = std::move(table2);
      capacity = oldCapacity * 2;
      table1.assign(capacity, std::vector<Key>(0));
      table2.assign(capacity, std::vector<Key>(0));

      for(std::vector<Key>& set : old_table1){
        for(Key& z : set){
          // std::cout << "[Thread " << std::this_thread::get_id() << "] Locking all" << std::endl;
          add(z);
        }
      }
      for(std::vector<Key>& set : old_table2){
        for(Key& z : set){
          add(z);
        }
      }
    }

    bool present(Key key, size_t b1, size_t b2) const{
      const std::vector<Key>& set1 = table1[b1];
      auto it1 = std::find(set1.begin(), set1.end(), key);
      if(it1 != set1.end()) return true;
      
      const std::vector<Key>& set2 = table2[b2];
      auto it2 = std::find(set2.begin(), set2.end(), key);
      return it2 != set2.end();
    }

    bool relocate(int i, int hi){
      int hj = 0;
      int j = 1 - i;
      for(size_t round = 0; round < LIMIT; round++){
        std::vector<Key>& iSet = ((i == 0) ? table1 : table2)[hi];
        Key y = iSet[0];
        
        size_t hj1 = hash1(y);
        size_t hj2 = hash2(y);

        // std::cout << "[Thread " << std::this_thread::get_id() << "] Locking in relocate "<< std::endl;
        std::unique_lock<std::recursive_mutex> lock1(mtx1[hj1 & (mtx1.size() - 1)]);
        std::unique_lock<std::recursive_mutex> lock2(mtx2[hj2 & (mtx2.size() - 1)]);
        // std::cout << "[Thread " << std::this_thread::get_id() << "] Done locking in relocate "<< std::endl;

        hj = (j == 0) ? (hj1 & (capacity - 1)) : (hj2 & (capacity - 1));
        std::vector<Key>& jSet = ((j == 0) ? table1 : table2)[hj];
        
        auto it = std::find(iSet.begin(), iSet.end(), y);
        if(it != iSet.end()){
          iSet.erase(it);
          if(jSet.size() < THRESHOLD){
            jSet.push_back(y);
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
            return false;
          }
        }
        else if(iSet.size() >= THRESHOLD){
          continue;
        }
        else{
          return true;
        }
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
    std::vector<std::vector<Key>> table1;
    std::vector<std::vector<Key>> table2;
    
    mutable std::vector<std::recursive_mutex> mtx1;
    mutable std::vector<std::recursive_mutex> mtx2;

    myhash::StdHash1<Key> hash1;
    myhash::StdHash2<Key> hash2;

    static constexpr size_t MAX_RELOCATIONS = 16;
    static constexpr size_t PROBE_SIZE = 4;
    static constexpr size_t THRESHOLD = 2;
    static constexpr size_t LIMIT = 10;
};

} // namespace cuckoo
