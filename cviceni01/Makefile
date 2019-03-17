CC        = gcc
CFLAGS    = -Wall -pedantic -ansi 
LIBS      = -lpthread  
CXX       = g++
CXXFLAGS  = -Wall -pedantic

TARGETS   = sum_01 

all: $(TARGETS)

sum_01: sum_01.cpp
	$(CXX) -std=c++11 $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	\rm -f $(TARGETS) *~ core

