#include "wx/wx.h"
#include "../../cpu.h"

#ifndef emulator_dcpu16_h
#define emulator_dcpu16_h

#define DCPU16_REG_A cpu::REG_A
#define DCPU16_REG_B cpu::REG_B
#define DCPU16_REG_C cpu::REG_C
#define DCPU16_REG_X cpu::REG_X
#define DCPU16_REG_Y cpu::REG_Y
#define DCPU16_REG_Z cpu::REG_Z
#define DCPU16_REG_I cpu::REG_I
#define DCPU16_REG_J cpu::REG_J
#define DCPU16_REG_PC cpu::REG_PC
#define DCPU16_REG_SP cpu::REG_SP
#define DCPU16_REG_EX cpu::REG_EX
enum {
    DCPU16_REG_IA = cpu::NUM_REGS,
    DCPU16_REG_IAQ,
    DCPU16_NUM_REGS
};

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
