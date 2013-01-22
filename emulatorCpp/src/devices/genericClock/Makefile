OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXX = $(shell wx-config --cxx)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)
ifeq ($(shell uname), MINGW32_NT-6.1)
WX_CXXFLAGS := $(WX_CXXFLAGS) -I/usr/include/boost-1_52
endif

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
genericClock.o: genericClock.cpp genericClock.h ../../device.h ../../cpu.h
	$(CXX) $(WX_CXXFLAGS) -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
