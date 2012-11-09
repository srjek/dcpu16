CXX = $(shell wx-config --cxx)
PROGRAM = emulator
# wx-config --libs
WX_LIBS = $(shell wx-config --libs)
LIBS = -lboost_system
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

SOURCES := $(wildcard *.cpp)
GENERATED_SOURCES := cpus/cpus.h devices/devices.h
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
CPU_OBJS := $(shell cd cpus; make print_cpu_objs | tr " " "\n" | sed "s:.*:cpus/&:" | tr "\n" " ")
DEVICE_OBJS := $(shell cd devices; make print_device_objs | tr " " "\n" | sed "s:.*:devices/&:" | tr "\n" " ")

STATIC :=
ifeq ($(shell uname), MINGW32_NT-6.1)
STATIC := --static
endif

all:	$(PROGRAM)

ifneq (,$(findstring depend,$(MAKECMDGOALS)))
MAKEDEPEND := $(SOURCES)
endif
makedepend: $(MAKEDEPEND) $(GENERATED_SOURCES)
	@SOURCES="$(SOURCES)" CFLAGS="$(CFLAGS)" ./gen-makedepend.bash

depend: makedepend
    
.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) $(WX_CXXFLAGS) -c  -o $@ $<
orderedForwardMap.o: orderedForwardMap.cpp
	$(CXX) $(WX_CXXFLAGS) -std=gnu++0x -c  -o $@ $<

#$(PROGRAM):$(SOURCES)
#	$(CXX) $(SOURCES) $(WX_CXXFLAGS) $(WX_LIBS) -o $(PROGRAM)
$(PROGRAM): $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS)
	$(CXX) -o $(PROGRAM) $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS) $(WX_LIBS) $(LIBS) $(STATIC)

include makedepend

#Do nothing
$(CPU_OBJS): cpus FORCE
	@echo -n ""
$(DEVICE_OBJS): devices FORCE
	@echo -n ""

cpus: FORCE
	@cd cpus && $(MAKE) cpus
cpus/cpus.h: FORCE
	@cd cpus && $(MAKE) cpus.h
devices: FORCE
	@cd devices && $(MAKE) devices
devices/devices.h: FORCE
	@cd devices && $(MAKE) devices.h

FORCE: ;
.PHONY: clean all run depend
run: all
	./$(PROGRAM)

clean:
	rm -f makedepend
	rm -f makedepend.bak
	rm -f *.o
	rm -f $(PROGRAM) $(PROGRAM).exe
	@cd cpus && $(MAKE) clean
	@cd devices && $(MAKE) clean
