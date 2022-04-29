CXX = g++
CXXFLAGS = -g -Wall -Wextra -O3 -std=c++17

all: benchmark

benchmark: samples/benchmark.cpp src/pmc.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f benchmark
