include ../../Makefile.rules

SPED3.o: SPED3.cpp SPED3.h SPED3_shaders.h ../../freeglut.h ../../device.h ../../cpu.h
	$(CXX) $(CXXFLAGS) $(C11) -c  -o $@ $<
    
.PHONY: clean

clean:
	rm -f *.o
