include Makefile.rules
PROGRAM = emulator

SOURCES := $(wildcard *.cpp)
GENERATED_SOURCES := cpus/cpus.h devices/devices.h
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJECTS := $(filter-out main.o, $(filter-out test.o, $(OBJECTS)))
CPU_OBJS := $(shell cd cpus; make print_cpu_objs | tr " " "\n" | sed "s:.*:cpus/&:" | tr "\n" " ") cpus/cpus.o
DEVICE_OBJS := $(shell cd devices; make print_device_objs | tr " " "\n" | sed "s:.*:devices/&:" | tr "\n" " ") #devices/devices.o

all:	$(PROGRAM)

ifneq (,$(findstring depend,$(MAKECMDGOALS)))
MAKEDEPEND := $(SOURCES) $(wildcard *.h)
endif
makedepend: $(MAKEDEPEND) $(GENERATED_SOURCES)
	@SOURCES="$(SOURCES)" CFLAGS="$(CFLAGS)" ./gen-makedepend.bash

depend: makedepend
    
orderedForwardMap.o: orderedForwardMap.cpp
	$(CXX) $(WX_CXXFLAGS) -std=gnu++0x -c -o $@ $<

#$(PROGRAM):$(SOURCES)
#	$(CXX) $(SOURCES) $(WX_CXXFLAGS) $(WX_LIBS) -o $(PROGRAM)
$(PROGRAM): main.o $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS)
	$(CXX) -o $(PROGRAM) main.o $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS) $(LIBS) $(STATIC)
emulator-test: test.o $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS)
	$(CXX) -o emulator-test test.o $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS) $(LIBS) $(STATIC)

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
	rm -f emulator-test emulator-test.exe
	@cd cpus && $(MAKE) clean
	@cd devices && $(MAKE) clean
