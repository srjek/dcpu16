#include "wx/wx.h"
#include "../../cpu.h"

#ifndef emulator_dcpu16_h
#define emulator_dcpu16_h

#define DCPU16_REG_A 0
#define DCPU16_REG_B 1
#define DCPU16_REG_C 2
#define DCPU16_REG_X 3
#define DCPU16_REG_Y 4
#define DCPU16_REG_Z 5
#define DCPU16_REG_I 6
#define DCPU16_REG_J 7
#define DCPU16_REG_PC 8
#define DCPU16_REG_SP 9
#define DCPU16_REG_EX 10
#define DCPU16_REG_IA 11
#define DCPU16_REG_IAQ 12

#define dcpu16_CMDLINE_HELP \
	{ wxCMD_LINE_SWITCH, NULL, wxT("dcpu16"), wxT("CPUS!") },
	
class dcpu16Config: public cpuConfig {
public:
    dcpu16Config();
    dcpu16Config(int& argc, wxChar**& argv);
    cpu* createCpu();
};

#endif
