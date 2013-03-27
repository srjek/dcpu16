PROGRAM = gdb-test
LIBS = -lboost_system -lpthread


# wx-config --libs
WX_LIBS = $(shell wx-config --libs) -lboost_system
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

CXXFLAGS :=
STATIC :=
ifeq ($(shell uname), MINGW32_NT-6.1)
CXXFLAGS := -I/usr/include/boost-1_52 -D_WIN32_WINNT=0x0501
LIBS := -L/usr/lib -lboost_system-mgw47-mt-1_52 -lws2_32
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
	$(CXX) $(CXXFLAGS) -c  -o $@ $<
gdb-remote-test.o: gdb-remote-test.cpp
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) -g -c  -o $@ $<

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
