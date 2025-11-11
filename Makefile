# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++20 -g -O3 -fgnu-tm -Wall -Wextra -Wpedantic
LDFLAGS := -pthread

# Directories
INCLUDE_DIR := include
SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

# Targets
TARGET_MAIN := $(BUILD_DIR)/cuckoo_main
TARGET_TEST_CORRECTNESS := $(BUILD_DIR)/test_correctness
TARGET_TEST_PERFORMANCE := $(BUILD_DIR)/test_performance

# Source files
SRC_MAIN := $(SRC_DIR)/main.cpp
SRC_TEST_CORRECTNESS := $(TEST_DIR)/test_correctness.cpp
SRC_TEST_PERFORMANCE := $(TEST_DIR)/test_performance.cpp

# Default target
all: $(TARGET_MAIN) $(TARGET_TEST_CORRECTNESS) $(TARGET_TEST_PERFORMANCE)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Executables
$(TARGET_MAIN): $(SRC_MAIN) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $< -o $@ $(LDFLAGS)

$(TARGET_TEST_CORRECTNESS): $(SRC_TEST_CORRECTNESS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $< -o $@ $(LDFLAGS)

$(TARGET_TEST_PERFORMANCE): $(SRC_TEST_PERFORMANCE) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $< -o $@ $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean