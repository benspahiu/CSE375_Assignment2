#include <iostream>
// #include <utility>
// #include <vector>
#include <optional>
#include "hashes.h"
#include "cuckoo_seq.h"
int main(){
  cuckoo::cuckoo_seq<std::string> set;
  std::cout << set.add("A") << std::endl;
  std::cout << set.add("A") << std::endl;
  std::cout << set.contains("A") << std::endl;
  std::cout << set.remove("B") << std::endl;
  std::cout << set.remove("A") << std::endl;
  for(char c = 'a'; c <= 'z'; c++){
    set.add(std::string(c, 1));
  }
  std::cout << set.size() << std::endl;

  // std::vector<std::string> vec;
  // vec.push_back("abc");
  // vec.push_back("bcd");
  // vec.push_back("cde");
  // std::string str = "xyz";
  // std::swap(vec[2], str);
  // std::cout << vec[2] << std::endl;
  // std::cout << str << std::endl;

  // std::cout << myhash::StdHash1<std::optional<std::string>>{}("ABC") << std::endl;
  // std::cout << myhash::StdHash1<std::optional<std::string>>{}(std::optional<std::string>("ABC")) << std::endl;
  // std::cout << myhash::StdHash1<std::optional<std::string>>{}(std::nullopt) << std::endl;
  return 0;
}