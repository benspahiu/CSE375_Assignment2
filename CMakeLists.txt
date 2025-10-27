cmake_minimum_required(VERSION 3.16)
project(CuckooHashSetProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Libraries (each implementation separately, optional)
add_library(cuckoo_seq src/cuckoo_hashset_seq.cpp)
add_library(cuckoo_concurrent src/cuckoo_hashset_concurrent.cpp)
add_library(cuckoo_striped_concurrent src/cuckoo_hashset_striped_concurrent.cpp)
add_library(cuckoo_tx src/cuckoo_hashset_tx.cpp)

target_include_directories(cuckoo_seq PUBLIC include)
target_include_directories(cuckoo_concurrent PUBLIC include)
target_include_directories(cuckoo_striped_concurrent PUBLIC include)
target_include_directories(cuckoo_tx PUBLIC include)

# --- Main executable for quick testing
add_executable(cuckoo_main src/main.cpp)
target_link_libraries(cuckoo_main PRIVATE cuckoo_seq cuckoo_concurrent cuckoo_striped_concurrent cuckoo_tx)

# --- Tests
add_executable(test_correctness tests/test_correctness.cpp)
target_link_libraries(test_correctness PRIVATE cuckoo_seq cuckoo_concurrent cuckoo_striped_concurrent cuckoo_tx)

add_executable(test_performance tests/test_performance.cpp)
target_link_libraries(test_performance PRIVATE cuckoo_seq cuckoo_concurrent cuckoo_striped_concurrent cuckoo_tx)