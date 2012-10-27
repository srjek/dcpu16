#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_genericClock_h
#define emulator_genericClock_h

#define genericClock_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("genericClock"), wxT("DEVICES!") },
	
class genericClockConfig: public deviceConfig {
public:
    bool providesKeyboard() { return false; }
    bool consumesKeyboard() { return false; }

    genericClockConfig() { name = "Generic Clock"; }
    genericClockConfig(int& argc, wxChar**& argv) {
        name = "Generic Clock";
    }
    device* createDevice(cpu* host);
    device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
