CPUS := $(shell find * -type d -print)
CLEANCPUS := $(CPUS:%=clean-%)
MISSINGCPUS := $(filter-out $(CPUS), $(shell cat .lastCpus 2>/dev/null))
EXTRACPUS := $(filter-out $(shell cat .lastCpus 2>/dev/null), $(CPUS))
EXTRACPUS := $(EXTRACPUS:%=extra-%)

cpus: $(CPUS)

cpus.h: $(MISSINGCPUS) $(EXTRACPUS)
	@echo -n $(CPUS) > .lastCpus
	./build-cpus_h > cpus.h

print_cpu_objs:
	@echo $(CPUS) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "
	
$(MISSINGCPUS): FORCE
$(EXTRACPUS): FORCE
$(CPUS): FORCE
	@cd $@ && $(MAKE) $@.o

FORCE: ;
.PHONY: clean $(CLEANCPUS) cpus print_cpu_objs

clean: $(CLEANCPUS)
	rm -f cpus.h .lastCpus
$(CLEANCPUS):
	@cd $(@:clean-%=%) && $(MAKE) clean
