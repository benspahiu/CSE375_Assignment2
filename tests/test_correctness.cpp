#include <iostream>
#include <unordered_set>
#include <vector>
#include <random>
#include <cassert>
#include "cuckoo_seq.h"
#include "cuckoo_conc.h"

using namespace cuckoo;

template<typename Key>
void test_generic(Key& key, int op, cuckoo_seq<Key>& cuckooSet, 
                  cuckoo_conc<Key>& concSet, std::unordered_set<Key>& refSet){
  if (op == 0) {
    bool res1 = cuckooSet.add(key);
    bool res2 = concSet.add(key);
    bool res3 = refSet.insert(key).second;
    assert(res1 == res3);
    assert(res2 == res3);
  } 
  else if (op == 1) {
    bool res1 = cuckooSet.remove(key);
    bool res2 = concSet.remove(key);
    bool res3 = (refSet.erase(key) > 0);
    assert(res1 == res3);
    assert(res2 == res3);
  } 
  else { // contains
    bool res1 = cuckooSet.contains(key);
    bool res2 = concSet.contains(key);
    bool res3 = (refSet.find(key) != refSet.end());
    assert(res1 == res3);
    assert(res2 == res3);
  }
}

template<typename Key>
void final_check(cuckoo_seq<Key>& cuckooSet, 
                cuckoo_conc<Key>& concSet, 
                std::unordered_set<Key>& refSet){
  
  // Final state check
  assert(cuckooSet.size() == refSet.size());
  assert(concSet.size() == refSet.size());
  for(auto it = refSet.begin(); it != refSet.end(); it++){
    assert(cuckooSet.contains(*it));
    assert(concSet.contains(*it));
  }
}

void test_ints(){
  constexpr int N = 10000;
  std::mt19937 rng;
  std::uniform_int_distribution<int> dist(0, N / 2);

  cuckoo_seq<int> cuckooSet;
  cuckoo_conc<int> concSet;
  std::unordered_set<int> refSet;

  for (int i = 0; i < N; ++i) {
    int key = dist(rng);
    int op = rng() % 3; // 0 = add, 1 = remove, 2 = contains

    test_generic(key, op, cuckooSet, concSet, refSet);
  }

  final_check(cuckooSet, concSet, refSet);
}

void test_strings(){
  std::mt19937 rng;
  std::uniform_int_distribution<char> charDist('a', 'z');
  cuckoo_seq<std::string> cuckooSet;
  cuckoo_conc<std::string> concSet;
  std::unordered_set<std::string> refSet;

  for (int i = 0; i < 5000; ++i) {
    std::string key;
    int len = 1 + (rng() % 10);
    for (int j = 0; j < len; ++j)
      key += charDist(rng);

    int op = rng() % 3;

    test_generic(key, op, cuckooSet, concSet, refSet);
  }

  final_check(cuckooSet, concSet, refSet);
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
  std::mt19937 rng;
  std::uniform_int_distribution<int> dist(0, 50);
  cuckoo_seq<Point> cuckooSet;
  cuckoo_conc<Point> concSet;
  std::unordered_set<Point> refSet;

  for (int i = 0; i < 5000; ++i) {
    Point p{ dist(rng), dist(rng)};
    int op = rng() % 3;

    test_generic(p, op, cuckooSet, concSet, refSet);
  }

  final_check(cuckooSet, concSet, refSet);
}

int main() {
  test_ints();
  test_strings();
  test_user_defined_class();
  std::cout << "Sequential correctness tests pass" << std::endl;
  return 0;
}