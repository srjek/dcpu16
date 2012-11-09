#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_M35FD_h
#define emulator_M35FD_h

#define M35FD_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("M35FD"), wxT("DEVICES!") },
	
class M35FDConfig: public deviceConfig {
public:
    inline bool providesKeyboard() { return false; }
    inline bool consumesKeyboard() { return false; }

    inline M35FDConfig() { name = "M35FD"; }
    inline M35FDConfig(int& argc, wxChar**& argv) {
        name = "M35FD";
    }
    device* createDevice(cpu* host);
    inline device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
