#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_genericKeyboard_h
#define emulator_genericKeyboard_h

#define genericKeyboard_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("genericKeyboard"), wxT("DEVICES!") },
	
class genericKeyboardConfig: public deviceConfig {
public:
    inline bool providesKeyboard() { return false; }
    inline bool consumesKeyboard() { return true; }

    inline genericKeyboardConfig() { name = "Generic Keyboard"; }
    inline genericKeyboardConfig(int& argc, wxChar**& argv) {
        name = "Generic Keyboard";
    }
    device* createDevice(cpu* host, device* keyboardProvider);
    inline device* createDevice(cpu* host) {
        return createDevice(host, NULL);
    }
};

#endif
