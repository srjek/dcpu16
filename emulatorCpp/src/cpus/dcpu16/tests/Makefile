include ../../../Makefile.rules

SOURCES := $(wildcard *.dasm)
OBJECTS := $(patsubst %.dasm,%.o,$(SOURCES))
OBJCOPY := objcopy --input binary -B $(BINARY_ARCH) --output $(ELF_ARCH)

.SUFFIXES: .bin .dasm
.dasm.bin :
	$(ASSEMBLER) $< $@
.bin.o :
	$(OBJCOPY) $< $@

dcpu16_tests.h: build-dcpu16_tests_h
	./build-dcpu16_tests_h > dcpu16_tests.h

dcpu16_tests.o: $(OBJECTS) dcpu16_tests.h
	ld -r $(OBJECTS) -o dcpu16_tests.o

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
