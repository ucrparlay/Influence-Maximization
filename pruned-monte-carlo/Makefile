CXX = g++
CXXFLAGS = -g -pthread -Wall -Wextra -O3 -std=c++17

all: benchmark

benchmark: samples/benchmark.cpp src/pmc.cpp src/pmc.hpp ../graph.hpp ../get_time.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f benchmark
