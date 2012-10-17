#include "../../cpu.h"
#include "../../sysConfig.h"
	
#ifndef emulator_dcpu16_h
#define emulator_dcpu16_h

#define dcpu16_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("dcpu16"), wxT("CPUS!") },
	
class dcpu16Config: public cpuConfig {
public:
    dcpu16Config() { name = "dcpu16"; }
    dcpu16Config(int& argc, wxChar**& argv) {
        name = "dcpu16";
    }
    cpu* createCpu() {
        //TODO
        return NULL;
    }
};

#endif
