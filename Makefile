CC = gcc
CXX = g++
DEBUG = -g
LIBFLAGS =
CXXFLAGS = -Wall -pg -fopenmp -std=c++11 $(DEBUG)
CFLAGS = -Wall -std=c99 $(DEBUG)

#to use openMP on OSX
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	CXX = /usr/local/opt/llvm/bin/clang
	CXXFLAGS = -Wall -O2 -std=c++11 $(DEBUG) -I/usr/local/opt/llvm/include -fopenmp -lstdc++
	LIBFLAGS = -L/usr/local/opt/llvm/lib
endif

MAIN_EXECUTABLE = bin/bcs
TEST_EXECUTABLE = bin/test

all: depend $(MAIN_EXECUTABLE)

SUBDIRS = src
CPP_SRC := $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.cpp))
C_SRC := $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.c))
EXE_SRC = src/main/bcs.cpp src/test/bcs_test.cpp

#generate object names
CPP_OBJ = $(CPP_SRC:.cpp=.o)
C_OBJ = $(C_SRC:.c=.o)

.PHONY: depend
depend: .depend

.depend: $(CPP_SRC) $(C_SRC) $(EXE_SRC)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $(CPP_SRC) $(C_SRC) > ./.depend;

#compile each object
.cpp.o:
	$(CXX) -o $@ -c $(CXXFLAGS) $<

.c.o:
	$(CC) -o $@ -c $(CFLAGS) $(H5_INCLUDE) $<

#compile the main executable
$(MAIN_EXECUTABLE): src/main/bcs.o $(CPP_OBJ) $(C_OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(CPP_OBJ) $(C_OBJ) src/main/bcs.o $(LIBFLAGS)

#compile the test executable
$(TEST_EXECUTABLE): src/test/bcs_test.o $(CPP_OBJ) $(C_OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(CPP_OBJ) $(C_OBJ) src/test/bcs_test.o $(LIBFLAGS)

PASS_SUBDIRS = tests/shouldPass
FAIL_SUBDIRS = tests/shouldFail
.PHONY: test
test: $(PASS_SUBDIRS)/* $(FAIL_SUBDIRS)/* $(TEST_EXECUTABLE)

	for file in $(PASS_SUBDIRS)/*; do \
		./$(TEST_EXECUTABLE) $${file};  \
	done
	for file in $(FAIL_SUBDIRS)/*; do \
		./$(TEST_EXECUTABLE) --shouldFail $${file};  \
	done
	rm test.simulation.bcs

.PHONY: clean	
clean:
	rm -f $(MAIN_EXECUTABLE) $(TEST_EXECUTABLE) $(CPP_OBJ) $(C_OBJ) src/main/bcs.o src/test/bcs_test.o
