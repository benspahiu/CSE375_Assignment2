#include <iostream>
#include "hashes.h"
#include "cuckoo_seq.h"
int main(){
  std::cout << myhash::StdHash1<std::string>{}("abcd") << std::endl;
  std::cout << myhash::StdHash2<std::string>{}("abcd") << std::endl;
  cuckoo::cuckoo_seq<std::string> set;
  std::cout << set.insert("A") << std::endl;
  return 0;
}