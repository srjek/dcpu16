DEVICES := $(shell find * -type d -print)
CLEANDEVICES := $(DEVICES:%=clean-%)
	
devices: devices.h $(DEVICES)

devices.h:
	./build-devices_h $(CPUS) > devices.h

print_device_objs:
	@echo $(DEVICES) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "
	
$(DEVICES):
	@cd $@; make $@.o
    
.PHONY: clean $(CLEANDEVICES) devices $(DEVICES) devices.h print_device_objs

clean: $(CLEANDEVICES)
	rm -f devices.h
$(CLEANDEVICES):
	@cd $(@:clean-%=%); make clean
