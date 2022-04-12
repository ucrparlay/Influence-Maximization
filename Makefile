ifdef BREAKDOWN
BREAK = -DBREAKDOWN
endif

ifdef NLONG
INTN = -DNLONG
endif

ifdef ELONG
INTE = -DELONG
endif

ifdef CLANG
#CC = clang++
CC = /tmp/llvm/bin/clang++
else
CC = g++
endif

CPPFLAGS = -std=c++17 -Wall -Wextra -Werror $(INTN) $(INTE) $(BREAK)

ifdef CILKPLUS
CPPFLAGS += -DPARLAY_CILKPLUS -DCILK -fcilkplus
else ifdef OPENCILK
CPPFLAGS += -DPARLAY_OPENCILK -DCILK -fopencilk
else ifdef SERIAL
CPPFLAGS += -DPARLAY_SEQUENTIAL
else
CPPFLAGS += -pthread
endif

ifdef DEBUG
CPPFLAGS += -DDEBUG -O0 -g
else
CPPFLAGS += -O3 -mcx16 -march=native
endif

ifdef STDALLOC
CPPFLAGS += -DPARLAY_USE_STD_ALLOC
endif

COMMON = graph.hpp parseCommandLine.hpp utilities.h get_time.hpp

all: graph IM  connect bfs scc reach 

	
graph: graph.cpp $(COMMON)
	$(CC) $(CPPFLAGS) graph.cpp -o graph

IM: IM.cpp IM.hpp $(COMMON)
	$(CC) $(CPPFLAGS) IM.cpp -o IM

connect: connect.h connect.cpp ldd.hpp 
	$(CC) $(CPPFLAGS) connect.cpp -o connect

bfs: bfs.cpp bfs.hpp $(COMMON)
	$(CC) $(CPPFLAGS) bfs.cpp -o bfs

scc: scc.hpp scc.cpp resizable_table.h hash_bag.h $(COMMON) reach.hpp
	$(CC) $(CPPFLAGS) scc.cpp -o scc

reach: reach.cpp reach.hpp hash_bag.h $(COMMON)
	$(CC) $(CPPFLAGS) reach.cpp -o reach

clean:
	rm -f graph IM