CXX = $(shell wx-config --cxx)
PROGRAM = emulator
# wx-config --libs
WX_LIBS = $(shell wx-config --libs)
# wx-config --cxxflags
WX_CXXFLAGS = $(shell wx-config --cxxflags)

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
CPU_OBJS := $(shell cd cpus; make print_cpu_objs | tr " " "\n" | sed "s:.*:cpus/&:" | tr "\n" " ")
DEVICE_OBJS := $(shell cd devices; make print_device_objs | tr " " "\n" | sed "s:.*:devices/&:" | tr "\n" " ")

# implementation

all:	$(PROGRAM)

.SUFFIXES:      .o .cpp

.cpp.o :
	$(CXX) $(WX_CXXFLAGS) -c  -o $@ $<
main.o: cpus.h devices.h


#$(PROGRAM):$(SOURCES)
#	$(CXX) $(SOURCES) $(WX_CXXFLAGS) $(WX_LIBS) -o $(PROGRAM)
$(PROGRAM):$(OBJECTS) devices cpus
	$(CXX) -o $(PROGRAM) $(OBJECTS) $(CPU_OBJS) $(DEVICE_OBJS) $(WX_LIBS)

cpus:
	@cd cpus && $(MAKE) cpus
cpus.h:
	@cd cpus && $(MAKE) cpus.h
devices:
	@cd devices && $(MAKE) devices
devices.h:
	@cd devices && $(MAKE) devices.h
    
.PHONY: clean all run devices cpus cpus.h devices.h
run: all
	./$(PROGRAM)

clean:
	rm -f *.o
	rm -f $(PROGRAM)
	@cd cpus && $(MAKE) clean
	@cd devices && $(MAKE) clean
