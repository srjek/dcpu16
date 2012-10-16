CPUS := $(shell find * -type d -print)
CLEANCPUS := $(CPUS:%=clean-%)
	
cpus: $(CPUS)

cpus.h:
	./build-cpus_h > cpus.h

print_cpu_objs:
	@echo $(CPUS) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "
	
$(CPUS):
	@cd $@ && $(MAKE) $@.o
    
.PHONY: clean $(CLEANCPUS) cpus $(CPUS) cpus.h print_cpu_objs

clean: $(CLEANCPUS)
	rm -f cpus.h
$(CLEANCPUS):
	@cd $(@:clean-%=%) && $(MAKE) clean
