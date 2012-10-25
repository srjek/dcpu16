OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXX = $(shell wx-config --cxx)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
genericKeyboard.o: genericKeyboard.cpp genericKeyboard.h ../../device.h ../../cpu.h
	$(CXX) $(WX_CXXFLAGS) -std=gnu++0x -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
