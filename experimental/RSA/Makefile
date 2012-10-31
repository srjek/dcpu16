PYTHON := python
ifneq ($(shell python --version 2>&1 | sed "s:Python \([0-9]\).*:\1:"), 3)
PYTHON := python3
endif

ASSEMBLER := $(PYTHON) ../../assembler/assembler.py
EMULATOR := ../../emulatorCpp/src/emulator

all: RSA.bin

RSA.bin: RSA.dasm
	$(ASSEMBLER) $< $@
	
run: RSA.bin
	$(EMULATOR) --M35FD --LEM1802 --image RSA.bin

clean:
	rm -rf *.bin

.PHONY: run all clean