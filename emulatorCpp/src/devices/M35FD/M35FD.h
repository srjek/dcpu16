#include "../../device.h"
#include "../../cpu.h"
#include "../../sysConfig.h"

#ifndef emulator_M35FD_h
#define emulator_M35FD_h

#define M35FD_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("M35FD"), wxT("DEVICES!") },
	
class M35FDConfig: public deviceConfig {
public:
    M35FDConfig() { name = "M35FD"; }
    M35FDConfig(int& argc, wxChar**& argv) {
        name = "M35FD";
    }
    device* createDevice(cpu host) {
        //TODO
        return NULL;
    }
};

#endif
