#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_genericKeyboard_h
#define emulator_genericKeyboard_h

#define genericKeyboard_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("genericKeyboard"), wxT("DEVICES!") },
	
class genericKeyboardConfig: public deviceConfig {
public:
    bool providesKeyboard() { return false; }
    bool consumesKeyboard() { return true; }

    genericKeyboardConfig() { name = "Generic Keyboard"; }
    genericKeyboardConfig(int& argc, wxChar**& argv) {
        name = "Generic Keyboard";
    }
    device* createDevice(cpu* host, device* keyboardProvider);
    device* createDevice(cpu* host) {
        return createDevice(host, NULL);
    }
};

#endif
