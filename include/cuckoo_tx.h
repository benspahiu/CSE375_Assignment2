#pragma once
#include <mutex>
#include <vector>
#include <optional>
#include "hashes.h"

namespace cuckoo {

template<myhash::HashableAndEquatable Key>
class cuckoo_tx {
public:
    explicit cuckoo_tx(size_t cap = 16)
        : capacity(next_power_of_two(cap)),
          size_(0){

      table1 = new std::optional<Key>[capacity];
      table2 = new std::optional<Key>[capacity];
    }

    ~cuckoo_tx() {
      delete[] table1;
      delete[] table2;
    }

    static void configure(size_t max_relocations) 
    {
      MAX_RELOCATIONS = max_relocations;
    }

    template<std::convertible_to<Key> K>
    bool add(K&& keyParam){
      Key key = std::forward<K>(keyParam);
      
      if(contains(key)){
        return false;
      }

      for (size_t i = 0; i < MAX_RELOCATIONS; ++i) {
        bool tm_success = false;
        bool done = false;
        size_t b1 = bucket1(key);
        while(!tm_success){
          if(resizing){
            std::this_thread::yield();
            continue; // resizing, try again later
          }
          __transaction_atomic{
            // auto& slot = table1[b1];
            if (!table1[b1]) {
              table1[b1] = key;
              ++size_;
              done = true;
              // return true;
            } else {
              std::swap(*(table1[b1]), key);
            }
            if(resizing){
              __transaction_cancel;
            }
            tm_success = true;
          }
        }
        if(done) return true;
        // auto& slot1 = table1[bucket1(key)];
        // if (!slot1) {
        //   slot1.emplace(std::move(key));
        //   ++size_;
        //   return true;
        // } else {
        //   std::swap(*slot1, key);
        // }

        tm_success = false;
        done = false;
        size_t b2 = bucket2(key);
        while(!tm_success){
          if(resizing){
            std::this_thread::yield();
            continue; // resizing, try again later
          }
          __transaction_atomic{
            // auto& slot = table1[b1];
            if (!table2[b2]) {
              table2[b2] = key;
              ++size_;
              done = true;
              // return true;
            } else {
              std::swap(*(table2[b2]), key);
            }
            if(resizing){
              __transaction_cancel;
            }
            tm_success = true;
          }
        }
        if(done) return true;

        // auto& slot2 = table2[bucket2(key)];
        // if (!slot2) {
        //   slot2.emplace(std::move(key));
        //   ++size_;
        //   return true;
        // } else {
        //   std::swap(*slot2, key);
        // }
      }
      resize();
      return add(std::move(key));
    }

    bool contains(const Key& key) const{
      bool tm_success = false;
      std::optional<Key> v1, v2;
      while(!tm_success){
        if(resizing){
          std::this_thread::yield();
          continue;
        }
        size_t b1 = bucket1(key);
        size_t b2 = bucket2(key);
        __transaction_atomic{
          if(!resizing){
            v1 = table1[b1];
            v2 = table2[b2];
            tm_success = true;
          }
        }
      }
      return (v1 && *v1 == key) || (v2 && *v2 == key);
    }

    bool remove(const Key& key){
      size_t h1 = bucket1(key);
      size_t h2 = bucket2(key);
      bool removed = false;
      bool tm_success = false;
      while(!tm_success){
        if(resizing){
          std::this_thread::yield();
          continue; // resizing, try again later
        }
        __transaction_atomic {
          if(resizing)
            __transaction_cancel;
          if(table1[h1] == key){
            table1[h1] = std::nullopt;
            removed = true;
            --size_;
          }
          else if(table2[h2] == key){
            table2[h2] = std::nullopt;
            removed = true;
            --size_;
          }
          tm_success = true;
        }
      }
      return removed;
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
      __transaction_atomic {
        resizing = true;
      }
      std::unique_lock<std::recursive_mutex> resize_lock(resize_mtx);
      if(!resizing) return;
      resize_lvl++;
      capacity *= 2;
      
      std::optional<Key>* old_table1 = new std::optional<Key>[capacity];
      std::optional<Key>* old_table2 = new std::optional<Key>[capacity];

      std::swap(old_table1, table1);
      std::swap(old_table2, table2);
      
      // table1.assign(capacity, std::nullopt);
      // table2.assign(capacity, std::nullopt);
      
      size_ = 0; //updated by add
      
      for (int i = capacity / 2 - 1; i >= 0; i--){
        std::optional<Key>& slot = old_table1[i];
        if (slot) priv_add(std::move(*slot));
      }
      for (int i = capacity / 2 - 1; i >= 0; i--){
        std::optional<Key>& slot = old_table2[i];
        if (slot) priv_add(std::move(*slot));
      }
      resize_lvl--;
      if(resize_lvl == 0)
        resizing = false;
    }

    template<std::convertible_to<Key> K>
    bool priv_add(K&& keyParam){
      Key key = std::forward<K>(keyParam);

      for (size_t i = 0; i < MAX_RELOCATIONS; ++i) {
        auto& slot1 = table1[bucket1(key)];
        if (!slot1) {
          slot1.emplace(std::move(key));
          ++size_;
          return true;
        } else {
          std::swap(*slot1, key);
        }

        auto& slot2 = table2[bucket2(key)];
        if (!slot2) {
          slot2.emplace(std::move(key));
          ++size_;
          return true;
        } else {
          std::swap(*slot2, key);
        }
      }
      resize();
      return priv_add(std::move(key));
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

    size_t bucket1(const Key& key) const {
      return hash1(key) & (capacity - 1); // % capacity (since power of 2)
    }

    size_t bucket2(const Key& key) const {
      return hash2(key) & (capacity - 1); // % capacity (since power of 2)
    }

    size_t capacity; // must be power of two
    size_t size_;
    std::optional<Key>* table1;
    std::optional<Key>* table2;
    std::recursive_mutex resize_mtx;
    bool resizing = false;
    int resize_lvl = 0;

    myhash::StdHash1<std::optional<Key>> hash1;
    myhash::StdHash2<std::optional<Key>> hash2;

    inline static size_t MAX_RELOCATIONS = 16;
};

} // namespace cuckoo
