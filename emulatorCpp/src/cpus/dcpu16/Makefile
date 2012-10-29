OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXX = $(shell wx-config --cxx)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

PYTHON := python
ifneq ($(shell python --version 2>&1 | sed "s:Python \([0-9]\).*:\1:"), 3)
PYTHON := python3
endif

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
dcpu16.o: dcpu16.cpp dcpu16.h ../../cpu.h ../../strHelper.h ../../orderedForwardMap.h firmware.bin
	$(CXX) $(WX_CXXFLAGS) -c  -o $@ $<

firmware.bin: firmware.dasm
	$(PYTHON) ../../../../assembler/assembler.py $< $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
