include ../../Makefile.rules

dcpu16-cpu.o: dcpu16-cpu.cpp dcpu16-cpu.h dcpu16-gui.h dcpu16.h ../../cpu.h ../../strHelper.h ../../orderedForwardMap.h

dcpu16-test.o: dcpu16-test.cpp dcpu16-test.h dcpu16-cpu.h dcpu16.h tests/dcpu16_tests.o ../../cpu.h ../../test.h

dcpu16-gui.o: dcpu16-gui.cpp dcpu16-gui.h

dcpu16.o: dcpu16-cpu.o firmware.bin dcpu16-gui.o dcpu16-test.o tests/dcpu16_tests.o
	ld -r dcpu16-cpu.o dcpu16-gui.o dcpu16-test.o tests/dcpu16_tests.o -o dcpu16.o

tests/dcpu16_tests.o: FORCE
	@cd tests && $(MAKE) dcpu16_tests.o

firmware.bin: firmware.dasm
	$(ASSEMBLER) $< $@

FORCE: ;
.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
