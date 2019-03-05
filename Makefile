CC = gcc
CXX = g++
DEBUG = -g
LIBFLAGS =
CXXFLAGS = -Wall -O2 -fopenmp -std=c++11 $(DEBUG)
CFLAGS = -Wall -std=c99 -O2 $(DEBUG)

#to use openMP on OSX
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	CXX = /usr/local/opt/llvm/bin/clang
	CXXFLAGS = -Wall -O2 -std=c++11 $(DEBUG) -I/usr/local/opt/llvm/include -fopenmp -lstdc++
	LIBFLAGS = -L/usr/local/opt/llvm/lib
endif

MAIN_EXECUTABLE = bin/bcs

all: depend $(MAIN_EXECUTABLE)

SUBDIRS = src
CPP_SRC := $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.cpp))
C_SRC := $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.c))
EXE_SRC = src/bcs.cpp

#generate object names
CPP_OBJ = $(CPP_SRC:.cpp=.o)
C_OBJ = $(C_SRC:.c=.o)

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
$(MAIN_EXECUTABLE): src/bcs.o $(CPP_OBJ) $(C_OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(CPP_OBJ) $(C_OBJ) $(LIBFLAGS)

clean:
	rm -f $(MAIN_EXECUTABLE) $(CPP_OBJ) $(C_OBJ) src/bcs.o
