ifdef BREAKDOWN
BREAK = -DBREAKDOWN
endif

ifdef MEMORY
MEMO = -DMEM
endif

ifdef EVALUE
EVALUATE = -DEVAL
endif

ifdef SCORE
SCORES = -DSCORE
endif

ifdef SPARSE_BREAK
SP_BREAK = -DSPARSE_BREAK
endif

ifdef NLONG
INTN = -DNLONG
endif

ifdef ELONG
INTE = -DELONG
endif

ifdef CLANG
CC = clang++
else
CC = g++
endif

CPPFLAGS = -std=c++17 \
$(INTN) $(INTE) $(BREAK) $(SP_BREAK) $(MEMO) $(EVALUATE) $(SCORES) \
-I./PAM/include/ \
-I./parlaylib/include/

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

all: IM general_cascade

IM: IM.cpp union_find.hpp IM_compact.hpp $(COMMON)
	$(CC) $(CPPFLAGS) IM.cpp -o IM

general_cascade: general_cascade.cpp BFS_cascade.hpp bfs.hpp $(COMMON)
	$(CC) $(CPPFLAGS) general_cascade.cpp -o general_cascade

clean:
	rm -f IM general_cascade