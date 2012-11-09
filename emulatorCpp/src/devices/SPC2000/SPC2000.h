#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_SPC2000_h
#define emulator_SPC2000_h

#define SPC2000_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("SPC2000"), wxT("DEVICES!") },
	
class SPC2000Config: public deviceConfig {
public:
    inline bool providesKeyboard() { return false; }
    inline bool consumesKeyboard() { return false; }

    inline SPC2000Config() { name = "SPC2000"; }
    inline SPC2000Config(int& argc, wxChar**& argv) {
        name = "SPC2000";
    }
    device* createDevice(cpu* host);
    inline device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
