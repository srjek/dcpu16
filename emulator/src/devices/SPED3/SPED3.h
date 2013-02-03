#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_SPED3_h
#define emulator_SPED3_h

#define SPED3_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT_2("SPED3"), wxT_2("DEVICES!") },
	
class SPED3Config: public deviceConfig {
public:
    inline bool providesKeyboard() { return true; }
    inline bool consumesKeyboard() { return false; }

    inline SPED3Config() { name = "SPED-3"; }
    inline SPED3Config(int& argc, wxChar**& argv) {
        name = "SPED-3";
    }
    device* createDevice(cpu* host);
    inline device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
