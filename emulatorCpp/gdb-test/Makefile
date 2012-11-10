PROGRAM = gdb-test
LIBS = -lboost_system -lpthread


# wx-config --libs
WX_LIBS = $(shell wx-config --libs) -lboost_system
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

STATIC :=
ifeq ($(shell uname), MINGW32_NT-6.1)
STATIC := --static
endif

all:	$(PROGRAM) gdb-remote-test

ifneq (,$(findstring depend,$(MAKECMDGOALS)))
MAKEDEPEND := $(SOURCES)
endif
makedepend: $(MAKEDEPEND) $(GENERATED_SOURCES)
	@SOURCES="$(SOURCES)" CFLAGS="$(CFLAGS)" ./gen-makedepend.bash

depend: makedepend
    
.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) -c  -o $@ $<
gdb-remote-test.o: gdb-remote-test.cpp
	$(CXX) $(WX_CXXFLAGS) -g -c  -o $@ $<

$(PROGRAM): main.o
	$(CXX) -o $(PROGRAM) main.o $(LIBS) $(STATIC)
gdb-remote-test: gdb-remote-test.o
	$(CXX) -g -o gdb-remote-test gdb-remote-test.o $(WX_LIBS) $(STATIC)

include makedepend

FORCE: ;
.PHONY: clean all run depend
run: all
	./$(PROGRAM)

clean:
	rm -f makedepend
	rm -f makedepend.bak
	rm -f *.o
	rm -f $(PROGRAM) $(PROGRAM).exe
	rm -f gdb-remote-test gdb-remote-test.exe
