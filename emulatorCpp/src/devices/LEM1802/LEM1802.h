#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_LEM1802_h
#define emulator_LEM1802_h

#define LEM1802_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("LEM1802"), wxT("DEVICES!") },
	
class LEM1802Config: public deviceConfig {
public:
    inline bool providesKeyboard() { return true; }
    inline bool consumesKeyboard() { return false; }

    inline LEM1802Config() { name = "LEM1802"; }
    inline LEM1802Config(int& argc, wxChar**& argv) {
        name = "LEM1802";
    }
    device* createDevice(cpu* host);
    inline device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
