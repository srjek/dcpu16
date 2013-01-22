OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXX = $(shell wx-config --cxx)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)
ifeq ($(shell uname), MINGW32_NT-6.1)
WX_CXXFLAGS := $(WX_CXXFLAGS) -I/usr/local/include -I/usr/include -I/usr/include/boost-1_52 -DFREEGLUT_STATIC -DGLEW_STATIC
endif

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
SPED3.o: SPED3.cpp SPED3.h SPED3_shaders.h ../../freeglut.h ../../device.h ../../cpu.h
	$(CXX) $(WX_CXXFLAGS) -std=gnu++0x -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
