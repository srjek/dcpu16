CPUS := $(shell find * -type d -print)
CLEANCPUS := $(CPUS:%=clean-%)
	
cpus: cpus.h $(CPUS)

cpus.h:
	./build-cpus_h $(CPUS) > cpus.h

print_cpu_objs:
	@echo $(CPUS) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "
	
$(CPUS):
	@cd $@; make $@.o
    
.PHONY: clean $(CLEANCPUS) cpus $(CPUS) cpus.h print_cpu_objs

clean: $(CLEANCPUS)
	rm -f cpus.h
$(CLEANCPUS):
	@cd $(@:clean-%=%); make clean
