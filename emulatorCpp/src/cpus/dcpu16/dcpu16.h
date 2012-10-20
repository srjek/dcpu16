#include "../../cpu.h"

#ifndef emulator_dcpu16_h
#define emulator_dcpu16_h

#define dcpu16_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("dcpu16"), wxT("CPUS!") },
	
class dcpu16Config: public cpuConfig {
public:
    dcpu16Config();
    dcpu16Config(int& argc, wxChar**& argv);
    cpu* createCpu();
};

#endif
