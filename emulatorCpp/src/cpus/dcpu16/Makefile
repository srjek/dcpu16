include ../../Makefile.rules

dcpu16.o: dcpu16.cpp dcpu16.h ../../cpu.h ../../strHelper.h ../../orderedForwardMap.h firmware.bin

firmware.bin: firmware.dasm
	$(PYTHON) ../../../../assembler/assembler.py $< $@

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.bin
