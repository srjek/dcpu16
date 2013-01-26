include ../Makefile.rules

CPUS := $(shell find * -type d -print)
CLEANCPUS := $(CPUS:%=clean-%)
MISSINGCPUS := $(filter-out $(CPUS), $(shell cat .lastCpus 2>/dev/null))
EXTRACPUS := $(filter-out $(shell cat .lastCpus 2>/dev/null), $(CPUS))
EXTRACPUS := $(EXTRACPUS:%=extra-%)

cpus: cpus.o $(CPUS)

cpus.h: $(MISSINGCPUS) $(EXTRACPUS) build-cpus_h
	@echo -n $(CPUS) > .lastCpus
	./build-cpus_h > cpus.h
	
cpus.cpp: $(MISSINGCPUS) $(EXTRACPUS) build-cpus_cpp
	./build-cpus_cpp > cpus.cpp

cpus.o: cpus.cpp cpus.h
	@echo -n $(CPUS) > .lastCpus
	$(CXX) $(CXXFLAGS) -c -o $@ $<

print_cpu_objs:
	@echo $(CPUS) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "
	
$(MISSINGCPUS): FORCE
$(EXTRACPUS): FORCE
$(CPUS): FORCE
	@cd $@ && $(MAKE) $@.o

FORCE: ;
.PHONY: clean $(CLEANCPUS) cpus print_cpu_objs update_cpus

clean: $(CLEANCPUS)
	rm -f cpus.h .lastCpus
$(CLEANCPUS):
	@cd $(@:clean-%=%) && $(MAKE) clean
