#include "wx/wx.h"
#include "../../device.h"
#include "../../cpu.h"

#ifndef emulator_genericClock_h
#define emulator_genericClock_h

#define genericClock_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT_2("genericClock"), wxT_2("DEVICES!") },
	
class genericClockConfig: public deviceConfig {
    bool debug;
public:
    inline bool providesKeyboard() { return false; }
    inline bool consumesKeyboard() { return false; }

    inline genericClockConfig() { 
        debug = false;
        name = "Generic Clock"; 
    }
    inline genericClockConfig(int& argc, wxChar**& argv) {
        name = "Generic Clock";
        debug = false;
        if (argc > 0) {
            if (wxStrcmp(argv[0], _("--debug")) == 0) {
                debug = true;
                argv++; argc--;
            }
        }
    }
    device* createDevice(cpu* host);
    inline device* createDevice(cpu* host, device* keyboardProvider) {
        return createDevice(host);
    }
};

#endif
