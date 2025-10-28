#pragma once
#include <vector>
// #include <functional>
#include <optional>
// #include <utility>
#include "hashes.h"

namespace cuckoo {

template<myhash::HashableAndEquatable Key>
class cuckoo_seq {
public:
    explicit cuckoo_seq(size_t capacity = 16)
        : table1(capacity, std::nullopt),
          table2(capacity, std::nullopt),
          capacity(capacity),
          size_(0) {}

    bool insert(const Key& key){
      // (void) key;
      std::cout << hash1(key) << std::endl;
      std::cout << hash2(key) << std::endl;
      return false;
    }
    bool contains(const Key& key) const{
      return false;
    }
    bool erase(const Key& key){
      return false;
    }
    size_t size() const { return size_; }
    void populate(std::vector<Key>& values){
      return;
    }

private:
    bool cuckoo_insert(const Key& key, size_t depth){
      return false;
    }

    std::vector<std::optional<Key>> table1;
    std::vector<std::optional<Key>> table2;
    size_t capacity;
    size_t size_;

    myhash::StdHash1<Key> hash1;
    myhash::StdHash2<Key> hash2;

    static constexpr size_t MAX_RELOCATIONS = 16;
};

} // namespace cuckoo
