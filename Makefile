
# files

EXE = sawa

OBJS	= init.o main.o type.o random.o evaluator.o position.o move.o generator.o\
          sort.o bitboard.o hand.o check_info.o piece.o book.o\
          search.o  see.o futility.o trans.o root.o usi.o mate_one.o\
          mate_three.o bonanza_method_main.o bonanza_method_one.o bonanza_method_two.o\
          good_move.o test.o thread.o
# rules

all: $(EXE) .depend

clean:
	$(RM) *.o .depend gmon.out

run :
	make -j
	./$(EXE)

tee :
	make -j
	./$(EXE) | tee zzz.log

# general

CXX      = g++
#CXX	=clang++
CXXFLAGS += -pipe -Wall -Wextra -std=c++11 -msse2 -mfpmath=sse -fno-exceptions -fno-rtti
#-pg


# mode

CXXFLAGS	+=-DNDEBUG
#CXXFLAGS	+=-DLEARN
CXXFLAGS	+=-DTLP

CXXFLAGS  += -O3 -march=native -flto 
LDFLAGS  = -lm
MYLIBS	= -lboost_system -lboost_thread -lrt -lpthread


# C++

# optimisation

# dependencies

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(MYLIBS)

.depend:
	$(CXX) -MM -std=c++11 -msse4.1 -mfpmath=sse $(OBJS:.o=.cpp) > $@

include .depend

