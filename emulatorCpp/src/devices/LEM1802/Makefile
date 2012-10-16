OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

# implementation

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<

all:	$(PROGRAM)
    
.PHONY: clean all

clean:
	rm -f *.o
