include ../Makefile.rules

DEVICES := $(shell find * -maxdepth 0 -type d -print)
CLEANDEVICES := $(DEVICES:%=clean-%)
MISSINGDEVICES := $(filter-out $(DEVICES), $(shell cat .lastDevices 2>/dev/null))
EXTRADEVICES := $(filter-out $(shell cat .lastDevices 2>/dev/null), $(DEVICES))
EXTRADEVICES := $(EXTRADEVICES:%=extra-%)
	
devices: $(DEVICES) devices.h

devices.h: $(MISSINGDEVICES) $(EXTRADEVICES) build-devices_h
	@echo -n $(DEVICES) > .lastDevices
	./build-devices_h > devices.h

print_device_objs:
	@echo $(DEVICES) | tr " " "\n" | sed "s:.*:&/&.o:" | tr "\n" " "

$(MISSINGDEVICES): FORCE
$(EXTRADEVICES): FORCE
$(DEVICES): FORCE
	@cd $@ && $(MAKE) $@.o

FORCE: ;
.PHONY: clean $(CLEANDEVICES) devices print_device_objs

clean: $(CLEANDEVICES)
	rm -f devices.h .lastDevices
$(CLEANDEVICES):
	@cd $(@:clean-%=%) && $(MAKE) clean
