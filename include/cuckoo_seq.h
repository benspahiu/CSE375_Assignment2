#pragma once
#include <vector>
#include <optional>
#include "hashes.h"

namespace cuckoo {

template<myhash::HashableAndEquatable Key>
class cuckoo_seq {
public:
    explicit cuckoo_seq(size_t capacity = 16)
        : capacity(next_power_of_two(capacity)),
          size_(0),
          table1(this->capacity, std::nullopt),
          table2(this->capacity, std::nullopt){}

    template<std::convertible_to<Key> K>
    bool add(K&& keyParam){
      Key key = std::forward<K>(keyParam);
      
      if(contains(key)){
        return false;
      }

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
      return add(std::move(key));
    }

    bool contains(const Key& key) const{
      return table1[bucket1(key)] == key 
          || table2[bucket2(key)] == key;
    }

    bool remove(const Key& key){
      size_t h1 = bucket1(key);
      size_t h2 = bucket2(key);
      if(table1[h1] == key)
        table1[h1] = std::nullopt;
      else if(table2[h2] == key)
        table2[h2] = std::nullopt;
      else return false;
      --size_;
      return true;
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
      capacity *= 2;
      
      std::vector<std::optional<Key>> old_table1 = std::move(table1);
      std::vector<std::optional<Key>> old_table2 = std::move(table2);
      
      table1.assign(capacity, std::nullopt);
      table2.assign(capacity, std::nullopt);
      
      size_ = 0; //updated by add
      
      for (auto& slot : old_table1) {
        if (slot) add(std::move(*slot));
      }
      for (auto& slot : old_table2) {
        if (slot) add(std::move(*slot));
      }
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
    std::vector<std::optional<Key>> table1;
    std::vector<std::optional<Key>> table2;

    myhash::StdHash1<std::optional<Key>> hash1;
    myhash::StdHash2<std::optional<Key>> hash2;

    static constexpr size_t MAX_RELOCATIONS = 16;
};

} // namespace cuckoo
