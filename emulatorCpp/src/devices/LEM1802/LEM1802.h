#include "../../device.h"
#include "../../cpu.h"
#include "../../sysConfig.h"

#ifndef emulator_LEM1802_h
#define emulator_LEM1802_h

#define LEM1802_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("LEM1802"), wxT("DEVICES!") },
	
class LEM1802Config: public deviceConfig {
public:
    LEM1802Config() { name = "LEM1802"; }
    LEM1802Config(int& argc, wxChar**& argv) {
        name = "LEM1802";
    }
    device* createDevice(cpu host) {
        //TODO
        return NULL;
    }
};

#endif
