#include "wx/wx.h"
#include "dcpu16.h"

#ifndef emulator_dcpu16_cpu_h
#define emulator_dcpu16_cpu_h

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

#endif
