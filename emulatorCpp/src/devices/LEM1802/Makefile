OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXX = $(shell wx-config --cxx)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
LEM1802.o: LEM1802.cpp LEM1802.h ../../device.h ../../cpu.h
	$(CXX) $(WX_CXXFLAGS) -std=gnu++11 -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
