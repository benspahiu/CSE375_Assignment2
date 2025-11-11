#include <iostream>
#include <unordered_set>
#include <vector>
#include <random>
#include <cassert>
#include "cuckoo_seq.h"
#include "cuckoo_striped.h"
#include "cuckoo_refinable.h"
#include "cuckoo_tx.h"

using namespace cuckoo;

template<typename Key, typename Table>
void test_generic(Key& key, int op, cuckoo_seq<Key>& cuckooSet, 
                  cuckoo_striped<Key>& concSet, cuckoo_refinable<Key>& concSet2,
                  Table& concSet3, std::unordered_set<Key>& refSet){
  // std::cout << "Testing " << op << " with " << key << std::endl;
  if (op == 0) {
    bool res1 = refSet.insert(key).second;
    bool res2 = cuckooSet.add(key);
    bool res3 = concSet.add(key);
    bool res4 = concSet2.add(key);
    bool res5 = concSet3.add(key);
    assert(res1 == res2);
    assert(res1 == res3);
    assert(res1 == res4);
    assert(res1 == res5);
  } 
  else if (op == 1) {
    bool res1 = (refSet.erase(key) > 0);
    bool res2 = cuckooSet.remove(key);
    bool res3 = concSet.remove(key);
    bool res4 = concSet2.remove(key);
    bool res5 = concSet3.remove(key);
    assert(res1 == res2);
    assert(res1 == res3);
    assert(res1 == res4);
    assert(res1 == res5);
  } 
  else { // contains
    bool res1 = (refSet.find(key) != refSet.end());
    bool res2 = cuckooSet.contains(key);
    bool res3 = concSet.contains(key);
    bool res4 = concSet2.contains(key);
    bool res5 = concSet3.contains(key);
    assert(res1 == res2);
    assert(res1 == res3);
    assert(res1 == res4);
    assert(res1 == res5);
  }
}

template<typename Key, typename Table>
void final_check(cuckoo_seq<Key>& cuckooSet, 
                cuckoo_striped<Key>& concSet, 
                cuckoo_refinable<Key>& concSet2,
                Table& concSet3,  
                std::unordered_set<Key>& refSet){
  
  // Final state check
  assert(cuckooSet.size() == refSet.size());
  assert(concSet.size() == refSet.size());
  assert(concSet2.size() == refSet.size());
  assert(concSet3.size() == refSet.size());
  for(auto it = refSet.begin(); it != refSet.end(); it++){
    assert(cuckooSet.contains(*it));
    assert(concSet.contains(*it));
    assert(concSet2.contains(*it));
    assert(concSet3.contains(*it));
  }
}

void test_ints(){
  constexpr int N = 10000;
  std::mt19937 rng;
  std::uniform_int_distribution<int> dist(0, N / 2);

  cuckoo_seq<int> cuckooSet;
  cuckoo_striped<int> concSet;
  cuckoo_refinable<int> concSet2;
  cuckoo_tx<int> concSet3;
  std::unordered_set<int> refSet;

  for (int i = 0; i < N; ++i) {
    int key = dist(rng);
    int op = rng() % 3; // 0 = add, 1 = remove, 2 = contains
    test_generic(key, op, cuckooSet, concSet, concSet2, concSet3, refSet);
  }

  final_check(cuckooSet, concSet, concSet2, concSet3, refSet);
}

void test_strings(){
  std::mt19937 rng;
  std::uniform_int_distribution<char> charDist('a', 'z');
  cuckoo_seq<std::string> cuckooSet;
  cuckoo_striped<std::string> concSet;
  cuckoo_refinable<std::string> concSet2;
  cuckoo_striped<std::string> concSet3;
  std::unordered_set<std::string> refSet;

  for (int i = 0; i < 5000; ++i) {
    std::string key;
    int len = 1 + (rng() % 10);
    for (int j = 0; j < len; ++j)
      key += charDist(rng);

    int op = rng() % 3;

    test_generic(key, op, cuckooSet, concSet, concSet2, concSet3, refSet);
  }

  final_check(cuckooSet, concSet, concSet2, concSet3, refSet);
}

struct Point {
  int x, y;

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

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
  cuckoo_striped<Point> concSet;
  cuckoo_refinable<Point> concSet2;
  cuckoo_striped<Point> concSet3;
  std::unordered_set<Point> refSet;

  for (int i = 0; i < 5000; ++i) {
    Point p{ dist(rng), dist(rng)};
    int op = rng() % 3;

    test_generic(p, op, cuckooSet, concSet, concSet2, concSet3, refSet);
  }

  final_check(cuckooSet, concSet, concSet2, concSet3, refSet);
}

int main() {
  test_ints();
  test_strings();
  test_user_defined_class();
  std::cout << "Sequential correctness tests pass" << std::endl;
  return 0;
}