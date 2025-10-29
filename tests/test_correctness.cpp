#include <iostream>
#include <unordered_set>
#include <vector>
#include <random>
#include <cassert>
#include "cuckoo_seq.h"

using namespace cuckoo;

void test_ints(){
  constexpr int N = 10000;
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, N / 2);

  cuckoo_seq<int> cuckooSet;
  std::unordered_set<int> refSet;

  for (int i = 0; i < N; ++i) {
    int key = dist(rng);
    int op = rng() % 3; // 0 = add, 1 = remove, 2 = contains

    if (op == 0) {
      bool res1 = cuckooSet.add(key);
      bool res2 = refSet.insert(key).second;
      assert(res1 == res2);
    } 
    else if (op == 1) {
      bool res1 = cuckooSet.remove(key);
      bool res2 = (refSet.erase(key) > 0);
      assert(res1 == res2);
    } 
    else { // contains
      bool res1 = cuckooSet.contains(key);
      bool res2 = (refSet.find(key) != refSet.end());
      assert(res1 == res2);
    }
  }

  // Final state check
  assert(cuckooSet.size() == refSet.size());
  for(auto it = refSet.begin(); it != refSet.end(); it++){
    assert(cuckooSet.contains(*it));
  }
}

void test_strings(){
  std::mt19937 rng2(67890);
  std::uniform_int_distribution<char> charDist('a', 'z');
  cuckoo_seq<std::string> cuckooSet;
  std::unordered_set<std::string> refSet;

  for (int i = 0; i < 5000; ++i) {
    std::string key;
    int len = 1 + (rng2() % 10);
    for (int j = 0; j < len; ++j)
      key += charDist(rng2);

    int op = rng2() % 3;

    if (op == 0) {
      bool res1 = cuckooSet.add(key);
      bool res2 = refSet.insert(key).second;
      assert(res1 == res2);
    } else if (op == 1) {
      bool res1 = cuckooSet.remove(key);
      bool res2 = (refSet.erase(key) > 0);
      assert(res1 == res2);
    } else {
      bool res1 = cuckooSet.contains(key);
      bool res2 = (refSet.find(key) != refSet.end());
      assert(res1 == res2);
    }
  }

  // Final state check
  assert(cuckooSet.size() == refSet.size());
  for(auto it = refSet.begin(); it != refSet.end(); it++){
    assert(cuckooSet.contains(*it));
  }
}

struct Point {
  int x, y;

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

namespace std {
template<>
struct hash<Point> {
  size_t operator()(const Point& p) const {
    return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 16);
  }
};
}

void test_user_defined_class(){
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, 50);
  cuckoo_seq<Point> cuckooSet;
  std::unordered_set<Point> refSet;

  for (int i = 0; i < 5000; ++i) {
    Point p{ dist(rng), dist(rng)};
    int op = rng() % 3;

    if (op == 0) {
      bool res1 = cuckooSet.add(p);
      bool res2 = refSet.insert(p).second;
      assert(res1 == res2);
    } else if (op == 1) {
      bool res1 = cuckooSet.remove(p);
      bool res2 = (refSet.erase(p) > 0);
      assert(res1 == res2);
    } else {
      bool res1 = cuckooSet.contains(p);
      bool res2 = (refSet.find(p) != refSet.end());
      assert(res1 == res2);
    }
  }

  // Final state check
  assert(cuckooSet.size() == refSet.size());
  for(auto it = refSet.begin(); it != refSet.end(); it++){
    assert(cuckooSet.contains(*it));
  }
}

int main() {
  test_ints();
  test_strings();
  test_user_defined_class();
  return 0;
}