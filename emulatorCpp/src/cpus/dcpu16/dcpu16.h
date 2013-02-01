#include "wx/wx.h"
#include "../../cpu.h"

#include "dcpu16-test.h"

#ifndef emulator_dcpu16_h
#define emulator_dcpu16_h

#define dcpu16_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT_2("dcpu16"), wxT_2("CPUS!") },
	
class dcpu16Config: public cpuConfig {
    bool debug;
public:
    dcpu16Config();
    dcpu16Config(int& argc, wxChar**& argv);
    cpu* createCpu();
};

#endif
