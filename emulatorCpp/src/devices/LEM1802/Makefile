include ../../Makefile.rules

LEM1802.o: LEM1802.cpp LEM1802.h ../../device.h ../../cpu.h
	$(CXX) $(CXXFLAGS) $(C11) -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
