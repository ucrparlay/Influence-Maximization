CXX = g++
CXXFLAGS = -pthread -g -Wall -Wextra -O3 -std=c++17

all: pmc

pmc: samples/benchmark.cpp src/pmc.cpp src/pmc.hpp ../graph.hpp ../get_time.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f pmc
