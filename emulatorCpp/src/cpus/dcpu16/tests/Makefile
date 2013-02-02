include ../../../Makefile.rules

SOURCES := $(wildcard *.dasm)
OBJECTS := $(patsubst %.dasm,%.o,$(SOURCES))

MISSING_SOURCES := $(filter-out $(SOURCES), $(shell cat .lastTests 2>/dev/null))
EXTRA_SOURCES := $(filter-out $(shell cat .lastTests 2>/dev/null), $(SOURCES))

OBJCOPY := objcopy --input binary -B $(BINARY_ARCH) --output $(ELF_ARCH)
ifeq ($(shell uname), MINGW32_NT-6.1)
OBJCOPY := objcopy --input-target binary -B $(BINARY_ARCH) --output-target $(ELF_ARCH)
endif

.SUFFIXES: .bin .dasm
.dasm.bin :
	$(ASSEMBLER) $< $@
.bin.o :
	$(OBJCOPY) $< $@

dcpu16_tests.h: $(MISSING_SOURCES) $(EXTRA_SOURCES) build-dcpu16_tests_h
	./build-dcpu16_tests_h > dcpu16_tests.h
	@echo -n $(SOURCES) > .lastTests

dcpu16_tests.o: $(OBJECTS) dcpu16_tests.h
	ld -r $(OBJECTS) -o dcpu16_tests.o

test: FORCE
	@echo $(SOURCES)
	@echo $(MISSING_SOURCES)
	@echo $(EXTRA_SOURCES)

$(MISSING_SOURCES): FORCE ;

FORCE: ;
.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
	rm -f dcpu16_tests.h
	rm -f .lastTests
