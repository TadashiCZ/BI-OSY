CC        = gcc
CFLAGS    = -Wall -pedantic -ansi 
LIBS      = -lpthread  
CXX       = g++
CXXFLAGS  = -Wall -pedantic

TARGETS   = hello_pt_01 join_pt_01 param_pt_01 param_pt_01 param_pt_02 \
            hello_01 hello_02 detach_01 param_01 param_02

all: $(TARGETS)

hello_pt_01: hello_pt_01.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

join_pt_01: join_pt_01.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

param_pt_01: param_pt_01.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

param_pt_01_1: param_pt_01_1.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

param_pt_02: param_pt_02.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

hello_01: hello_01.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

hello_02: hello_02.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

detach_01: detach_01.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

param_01: param_01.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

param_02: param_02.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

sum_01: sum_01.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

sum_02: sum_02.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	\rm -f $(TARGETS) *~ core

