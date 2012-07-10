#ifdef _MSC_VER
#include "stdint.h"
#else
#include <stdint.h>
#endif
#include <stdlib.h>

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_X 3
#define REG_Y 4
#define REG_Z 5
#define REG_I 6
#define REG_J 7
#define PC 8
#define SP 9
#define EX 10
#define IA 11
#define IAQ 12

int extOpcodeCycles[] = {
            0, 2, 0, 0, 0, 0, 0, 0,
            3, 0, 0, 2, 1, 0, 0, 0,
            1, 3, 3, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0 };
int opcodeCycles[] = {
            0, 0, 1, 1, 1, 1, 2, 2,
            2, 2, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 2, 2, 0, 0, 1, 1 };
            
typedef struct {
    uint16_t* pointer;
    uint32_t value;
} val;
val getValStruct(uint16_t* pointer, uint16_t value) {
    val result = {pointer, value};
    return result;
}

uint16_t nextWord(uint16_t* ram, uint16_t* registers, int* cycles) {
    uint16_t result = ram[registers[PC]];
    registers[PC] += 1;// (registers[PC] + 1) & 0xFFFF;
    (*cycles) += 1;
    return result;
}

val read_val(uint16_t a, int isB, uint16_t* ram, uint16_t* registers, int* cycles) {
    uint16_t pointer;
    if (a <= 0x07) {            //register
        return getValStruct(registers + a, registers[a]);
    } else if (a <= 0x0F) {     //[register]
        pointer = registers[a & 0x07];
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a <= 0x17) {     //[next word + register]
        pointer = (nextWord(ram, registers, cycles) + registers[a & 0x07]) & 0xFFFF;
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a == 0x18) {     //PUSH/POP (b/a)
        if (isB != 0) {
            pointer = (registers[SP] - 1) & 0xFFFF;
            registers[SP] = pointer;
        } else {
            pointer = registers[SP];
            registers[SP] = (pointer + 1) & 0xFFFF;
        }
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a == 0x19) {     //PEEK   ([SP])
        pointer = registers[SP];
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a == 0x1A) {     //PICK   ([SP + next word])
        pointer = (registers[SP] + nextWord(ram, registers, cycles)) & 0xFFFF;
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a == 0x1B) {     //SP
        return getValStruct(registers + SP, registers[SP]);
    } else if (a == 0x1C) {     //PC
        return getValStruct(registers + PC, registers[PC]);
    } else if (a == 0x1D) {     //EX
        return getValStruct(registers + EX, registers[EX]);
    } else if (a == 0x1E) {     //[next word]
        pointer = nextWord(ram, registers, cycles);
        return getValStruct(ram + pointer, ram[pointer]);
    } else if (a == 0x1F) {     //next word (literal)
        return getValStruct(NULL, nextWord(ram, registers, cycles));
    } else if (a <= 0x3F) {     //0xffff-0x1e (literal)
        pointer = a & 0x1F;
        pointer -= 1;
        return getValStruct(NULL, pointer);
    }
    return getValStruct(NULL, 0);
}
void write_val(val a, uint16_t value) {
    if (a.pointer != NULL)
        *(a.pointer) = value;
}

int32_t sign16(uint32_t a) {
    a &= 0x0000FFFF;
    if (a & 0x8000)
        return a - 0x10000;
    return a;
}
uint16_t overflow(uint32_t value, uint16_t* registers) {
    registers[EX] = (value >> 16) & 0xFFFF;
    return value & 0xFFFF;
}
uint16_t overflowDIV(uint32_t b, uint32_t a, uint16_t* registers) {
    if (a == 0) {
        registers[EX] = 0;
        return 0;
    }    
    registers[EX] = ((b << 16) / a) & 0xFFFF;
    return (b / a) & 0xFFFF;
}
uint16_t overflowDVI(int32_t b, int32_t a, uint16_t* registers) {
    if (a == 0) {
        registers[EX] = 0;
        return 0;
    }
    registers[EX] = ((b << 16) / a) & 0xFFFF;
    return (b / a) & 0xFFFF;
}
uint16_t underflow(uint32_t value, uint16_t* registers) {
    registers[EX] = (value >> 16) & 0xFFFF;
    return value & 0xFFFF;
}

int skip(uint16_t* ram, uint16_t* registers, int* cycles) {
    uint16_t instruction = nextWord(ram, registers, cycles);
    uint16_t op = instruction & 0x1F;
    uint16_t b_code = (instruction >> 5) & 0x1F;
    uint16_t a_code = (instruction >> 10) & 0x3F;
    if (((0x10 <= a_code) && (a_code <= 0x17)) || (a_code == 0x1A) || (a_code == 0x1E) || (a_code == 0x1F))
        registers[PC] += 1;
    if (((0x10 <= b_code) && (b_code <= 0x17)) || (b_code == 0x1A) || (b_code == 0x1E) || (b_code == 0x1F))
        registers[PC] += 1;
    return (0x10 <= op) && (op <= 0x17);
}
void conditional(int condition, uint16_t* ram, uint16_t* registers, int* cycles) {
    if (!condition)
        while (skip(ram, registers, cycles));
}

#define HWI_HWN -1
#define HWI_HWQ_BA (1 << 16)
#define HWI_HWQ_C (2 << 16)
#define HWI_HWQ_YX (3 << 16)
uint32_t callPyHWI(PyObject* pyHWI, int value) {
    PyObject* result = PyObject_CallFunction(pyHWI, "i", value);
    if (PyLong_Check(result))
        return PyLong_AsUnsignedLong(result);
    return 0;
}

//MORE TODO IN HERE
void execOp(uint16_t op, val b, val a, uint16_t* ram, uint16_t* registers, int* cycles) {
    //This function doesn't handle op==0, caller detects that state and uses an alt function
    switch (op) {
        case 0x01:      //SET
            write_val(b, a.value);
            break;
        case 0x02:      //ADD
            write_val(b, overflow(b.value + a.value, registers));
            break;
        case 0x03:      //SUB
            write_val(b, underflow(b.value - a.value, registers));
            break;
        case 0x04:      //MUL
            write_val(b, overflow(b.value * a.value, registers));
            break;
        case 0x05:      //MLI
            write_val(b, overflow(sign16(b.value) * sign16(a.value), registers));
            break;
        case 0x06:      //DIV
            write_val(b, overflowDIV(b.value, a.value, registers));
            break;
        case 0x07:      //DVI
            write_val(b, overflowDVI(sign16(b.value), sign16(a.value), registers));
            break;
        case 0x08:      //MOD       TESTING NEEDED
            if (a.value == 0)
                write_val(b, 0);
            else
                write_val(b, b.value % a.value);
            break;
        case 0x09:      //MDI       IMPLEMENTATION NEEDED
            if (a.value == 0)
                write_val(b, 0);
            else
                write_val(b, sign16(b.value) % sign16(a.value));
            break;
        case 0x0A:      //AND
            write_val(b, b.value & a.value);
            break;
        case 0x0B:      //BOR
            write_val(b, b.value | a.value);
            break;
        case 0x0C:      //XOR
            write_val(b, b.value ^ a.value);
            break;
        case 0x0D:      //SHR
            if (a.value == 0)
                registers[EX] = 0;
            else
                registers[EX] = ((b.value << 15) >> (a.value-1)) & 0xFFFF;  //Avoiding the effects of arithmitic shifts...
            write_val(b, b.value >> a.value);
            break;
        case 0x0E:      //ASR
            registers[EX] = ((sign16(b.value) << 16) >> a.value) & 0xFFFF;
            write_val(b, sign16(b.value) >> a.value);
            break;
        case 0x0F:      //SHL
            registers[EX] = ((b.value << a.value) >> 16) & 0xFFFF;
            write_val(b, b.value << a.value);
            break;
        case 0x10:      //IFB
            conditional((b.value & a.value) != 0, ram, registers, cycles);
            break;
        case 0x11:      //IFC
            conditional((b.value & a.value) == 0, ram, registers, cycles);
            break;
        case 0x12:      //IFE
            conditional(b.value == a.value, ram, registers, cycles);
            break;
        case 0x13:      //IFN
            conditional(b.value != a.value, ram, registers, cycles);
            break;
        case 0x14:      //IFG
            conditional(b.value > a.value, ram, registers, cycles);
            break;
        case 0x15:      //IFA
            conditional(sign16(b.value) > sign16(a.value), ram, registers, cycles);
            break;
        case 0x16:      //IFL
            conditional(b.value < a.value, ram, registers, cycles);
            break;
        case 0x17:      //IFU
            conditional(sign16(b.value) < sign16(a.value), ram, registers, cycles);
            break;
        case 0x18:
        case 0x19:
            break;
        case 0x1A:      //ADX
            write_val(b, overflow(b.value + a.value + registers[EX], registers));
            break;
        case 0x1B:      //SBX
            if (registers[EX] != 0)
                write_val(b, underflow(b.value - a.value + ((uint32_t) registers[EX]) - 0x10000, registers));
            else
                write_val(b, underflow(b.value - a.value, registers));
            break;
        case 0x1C:
        case 0x1D:
            break;
        case 0x1E:      //STI
            write_val(b, a.value);
            registers[REG_I] += 1;
            registers[REG_J] += 1;
            break;
        case 0x1F:      //STD
            write_val(b, a.value);
            registers[REG_I] -= 1;
            registers[REG_J] -= 1;
            break;
    }
}

//MORE TODO IN HERE
void execExtOp(uint16_t op, val a, uint16_t* ram, uint16_t* registers, int* cycles, uint16_t* intQueue, PyObject* pyInt, PyObject* pyHWI) {
    uint32_t tmp;
    switch (op) {
        case 0x01:      //JSR <a>
            //SET PUSH, PC
            execOp(1, read_val(0x18, 1, ram, registers, cycles), getValStruct(registers + PC, registers[PC]), ram, registers, cycles);
            //SET PC, <a>
            registers[PC] = a.value;
            break;
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            break;
        case 0x08:      //INT
            if (PyCallable_Check(pyInt))
                PyObject_CallFunction(pyInt, "i", a.value);
            break;
        case 0x09:      //IAG <a>
            write_val(a, registers[IA]);
            break;
        case 0x0A:      //IAS <a>
            registers[IA] = a.value;
            break;
        case 0x0B:      //RFI
            //IAQ 0 -- should disable queuing
            registers[IAQ] = 0;
            //SET A, POP
            execOp(1, getValStruct(registers + REG_A, registers[REG_A]), read_val(0x18, 0, ram, registers, cycles), ram, registers, cycles);
            //SET PC, POP
            execOp(1, getValStruct(registers + PC, registers[PC]), read_val(0x18, 0, ram, registers, cycles), ram, registers, cycles);
            break;
        case 0x0C:      //IAQ
            registers[IAQ] = a.value;
            break;
        case 0x0D:
        case 0x0E:
        case 0x0F:
            break;
        case 0x10:      //HWN
            if (PyCallable_Check(pyHWI))
                write_val(a, callPyHWI(pyHWI, HWI_HWN) & 0xFFFF);
            break;
        case 0x11:      //HWQ
            if (PyCallable_Check(pyHWI)) {
                tmp = callPyHWI(pyHWI, HWI_HWQ_BA | ((int) a.value));
                registers[REG_A] = (uint16_t) (tmp & 0xFFFF);
                registers[REG_B] = (uint16_t) ((tmp >> 16) & 0xFFFF);
                tmp = callPyHWI(pyHWI, HWI_HWQ_C | ((int) a.value));
                registers[REG_C] = (uint16_t) (tmp & 0xFFFF);
                tmp = callPyHWI(pyHWI, HWI_HWQ_YX | ((int) a.value));
                registers[REG_X] = (uint16_t) (tmp & 0xFFFF);
                registers[REG_Y] = (uint16_t) ((tmp >> 16) & 0xFFFF);
            }
            break;
        case 0x12:      //HWI
            if (PyCallable_Check(pyHWI))
                (*cycles) += callPyHWI(pyHWI, a.value);
            break;
    }
}


PyObject* decode(PyObject* pyRam, PyObject* pyRegisters) {
    int cycles = 0;
    uint16_t instruction;
    uint16_t op;
    uint16_t b_code, a_code;
    val b, a;
    
    Py_buffer ramBuf;
    Py_buffer registersBuf;
    
    uint16_t* ram;
    uint16_t* registers;
    
    char* opStr = "";
    char* aStr = "";
    char* bStr = "";
    
    if ( PyObject_GetBuffer(pyRam, &ramBuf, PyBUF_WRITABLE) == -1)
        return NULL;
    if ( PyObject_GetBuffer(pyRegisters, &registersBuf, PyBUF_WRITABLE) == -1) {
        PyBuffer_Release(&ramBuf);
        return NULL;
    }
    
    ram = (uint16_t*) ramBuf.buf;
    registers = (uint16_t*) registersBuf.buf;
    
    
    instruction = ram[registers[PC]]; //nextWord(ram, registers, &cycles);
    op = instruction & 0x1F;
    b_code = (instruction >> 5) & 0x1F;
    a_code = (instruction >> 10) & 0x3F;
    //a = read_val(a_code, 0, ram, registers, &cycles);
    //if (op != 0)
    //    b = read_val(b_code, 1, ram, registers, &cycles);
    
    switch (op) {
        case 0x00:
            switch (b_code) {
                case 0x01:
                    opStr = "JSR";
                    break;
                case 0x08:
                    opStr = "INT";
                    break;
                case 0x09:
                    opStr = "IAG";
                    break;
                case 0x0A:
                    opStr = "IAS";
                    break;
                case 0x0B:
                    opStr = "RFI";
                    break;
                case 0x0C:
                    opStr = "IAQ";
                    break;
                case 0x10:
                    opStr = "HWN";
                    break;
                case 0x11:
                    opStr = "HWQ";
                    break;
                case 0x12:
                    opStr = "HWI";
                    break;
                default:
                    opStr = "NOP";
                    break;
            }
            break;
        case 0x01:
            opStr = "SET";
            break;
        case 0x02:
            opStr = "ADD";
            break;
        case 0x03:
            opStr = "SUB";
            break;
        case 0x04:
            opStr = "MUL";
            break;
        case 0x05:
            opStr = "MLI";
            break;
        case 0x06:
            opStr = "DIV";
            break;
        case 0x07:
            opStr = "DVI";
            break;
        case 0x08:
            opStr = "MOD";
            break;
        case 0x09:
            opStr = "MDI";
            break;
        case 0x0A:
            opStr = "AND";
            break;
        case 0x0B:
            opStr = "BOR";
            break;
        case 0x0C:
            opStr = "XOR";
            break;
        case 0x0D:
            opStr = "SHR";
            break;
        case 0x0E:
            opStr = "ASR";
            break;
        case 0x0F:
            opStr = "SHL";
            break;
        case 0x10:
            opStr = "IFB";
            break;
        case 0x11:
            opStr = "IFC";
            break;
        case 0x12:
            opStr = "IFE";
            break;
        case 0x13:
            opStr = "IFN";
            break;
        case 0x14:
            opStr = "IFG";
            break;
        case 0x15:
            opStr = "IFA";
            break;
        case 0x16:
            opStr = "IFL";
            break;
        case 0x17:
            opStr = "IFU";
            break;
        case 0x1A:
            opStr = "ADX";
            break;
        case 0x1B:
            opStr = "SBX";
            break;
        case 0x1E:
            opStr = "STI";
            break;
        case 0x1F:
            opStr = "STD";
            break;
        default:
            opStr = "NOP";
            break;
    }
    
    switch (a_code) {
        case 0x00:
            aStr = "A";
            break;
        case 0x01:
            aStr = "B";
            break;
        case 0x02:
            aStr = "C";
            break;
        case 0x03:
            aStr = "X";
            break;
        case 0x04:
            aStr = "Y";
            break;
        case 0x05:
            aStr = "Z";
            break;
        case 0x06:
            aStr = "I";
            break;
        case 0x07:
            aStr = "J";
            break;
        case 0x08:
            aStr = "[A]";
            break;
        case 0x09:
            aStr = "[B]";
            break;
        case 0x0A:
            aStr = "[C]";
            break;
        case 0x0B:
            aStr = "[X]";
            break;
        case 0x0C:
            aStr = "[Y]";
            break;
        case 0x0D:
            aStr = "[Z]";
            break;
        case 0x0E:
            aStr = "[I]";
            break;
        case 0x0F:
            aStr = "[J]";
            break;
        case 0x10:
            aStr = "[A+x]";
            break;
        case 0x11:
            aStr = "[B+x]";
            break;
        case 0x12:
            aStr = "[C+x]";
            break;
        case 0x13:
            aStr = "[X+x]";
            break;
        case 0x14:
            aStr = "[Y+x]";
            break;
        case 0x15:
            aStr = "[Z+x]";
            break;
        case 0x16:
            aStr = "[I+x]";
            break;
        case 0x17:
            aStr = "[J+x]";
            break;
        case 0x18:
            aStr = "POP";
            break;
        case 0x19:
            aStr = "[SP]";
            break;
        case 0x1A:
            aStr = "[SP+x]";
            break;
        case 0x1B:
            aStr = "SP";
            break;
        case 0x1C:
            aStr = "PC";
            break;
        case 0x1D:
            aStr = "EX";
            break;
        case 0x1E:
            aStr = "[x]";
            break;
        case 0x1F:
            aStr = "x";
            break;
        default:
            if ((a_code >= 0x20) && (a_code <= 0x3F))
                aStr = "x";
            break;
    }
    switch (b_code) {
        case 0x00:
            bStr = "A";
            break;
        case 0x01:
            bStr = "B";
            break;
        case 0x02:
            bStr = "C";
            break;
        case 0x03:
            bStr = "X";
            break;
        case 0x04:
            bStr = "Y";
            break;
        case 0x05:
            bStr = "Z";
            break;
        case 0x06:
            bStr = "I";
            break;
        case 0x07:
            bStr = "J";
            break;
        case 0x08:
            bStr = "[A]";
            break;
        case 0x09:
            bStr = "[B]";
            break;
        case 0x0A:
            bStr = "[C]";
            break;
        case 0x0B:
            bStr = "[X]";
            break;
        case 0x0C:
            bStr = "[Y]";
            break;
        case 0x0D:
            bStr = "[Z]";
            break;
        case 0x0E:
            bStr = "[I]";
            break;
        case 0x0F:
            bStr = "[J]";
            break;
        case 0x10:
            bStr = "[A+x]";
            break;
        case 0x11:
            bStr = "[B+x]";
            break;
        case 0x12:
            bStr = "[C+x]";
            break;
        case 0x13:
            bStr = "[X+x]";
            break;
        case 0x14:
            bStr = "[Y+x]";
            break;
        case 0x15:
            bStr = "[Z+x]";
            break;
        case 0x16:
            bStr = "[I+x]";
            break;
        case 0x17:
            bStr = "[J+x]";
            break;
        case 0x18:
            bStr = "PUSH";
            break;
        case 0x19:
            bStr = "[SP]";
            break;
        case 0x1A:
            bStr = "[SP+x]";
            break;
        case 0x1B:
            bStr = "SP";
            break;
        case 0x1C:
            bStr = "PC";
            break;
        case 0x1D:
            bStr = "EX";
            break;
        case 0x1E:
            bStr = "[x]";
            break;
        case 0x1F:
            bStr = "x";
            break;
        default:
            if ((b_code >= 0x20) && (b_code <= 0x3F))
                bStr = "x";
            break;
    }
    
    //if (read_val(a_code, 0, ram, registers, &cycles).value != 0x261)
    //    aStr = "FAIL";
    PyBuffer_Release(&registersBuf);
    PyBuffer_Release(&ramBuf);
    
    return Py_BuildValue("sss", opStr, bStr, aStr);
}

PyObject* cycles(PyObject* pyRam, PyObject* pyRegisters, PyObject* pyIntQueue, int count, PyObject* pyInt, PyObject* pyHWI)
{
    int cycles = 0;  //REMEMBER, MICROSFT'S F "C COMPILER" is a tad out of date and needs all these varibles at top
    uint16_t instruction;
    uint16_t op;
    uint16_t b_code, a_code;
    val b, a;
    
    Py_buffer ramBuf;
    Py_buffer registersBuf;
    Py_buffer intQueueBuf;
    
    uint16_t* ram;
    uint16_t* registers;
    uint16_t* intQueue;
    
    if ( PyObject_GetBuffer(pyRam, &ramBuf, PyBUF_WRITABLE) == -1)
        return NULL;
    if ( PyObject_GetBuffer(pyRegisters, &registersBuf, PyBUF_WRITABLE) == -1) {
        PyBuffer_Release(&ramBuf);
        return NULL;
    }
    if ( PyObject_GetBuffer(pyIntQueue, &intQueueBuf, PyBUF_WRITABLE) == -1) {
        PyBuffer_Release(&ramBuf);
        PyBuffer_Release(&registersBuf);
        return NULL;
    }
    
    ram = (uint16_t*) ramBuf.buf;
    registers = (uint16_t*) registersBuf.buf;
    intQueue = (uint16_t*) intQueueBuf.buf;
    
    while (cycles < count) {
        instruction = nextWord(ram, registers, &cycles);
        op = instruction & 0x1F;
        b_code = (instruction >> 5) & 0x1F;
        a_code = (instruction >> 10) & 0x3F;
        a = read_val(a_code, 0, ram, registers, &cycles);
        if (op != 0) {
            b = read_val(b_code, 1, ram, registers, &cycles);
            cycles += opcodeCycles[op];
            execOp(op, b, a, ram, registers, &cycles);
        } else {
            cycles += extOpcodeCycles[op];
            execExtOp(b_code, a, ram, registers, &cycles, intQueue, pyInt, pyHWI);
        }
        
        //check interrupts
        if ((intQueue[0] != intQueue[1]) && (registers[IAQ] == 0)) {
            if (registers[IA] != 0) {
                //IAQ 1 -- should enable queuing
                registers[IAQ] = 1;
                //SET PUSH, PC
                execOp(1, read_val(0x18, 1, ram, registers, &cycles), getValStruct(registers + PC, registers[PC]), ram, registers, &cycles);
                //SET PUSH, A
                execOp(1, read_val(0x18, 1, ram, registers, &cycles), getValStruct(registers + REG_A, registers[REG_A]), ram, registers, &cycles);
                //SET PC, IA
                registers[PC] = registers[IA];
                //SET A, <msg>
                registers[REG_A] = intQueue[2+intQueue[0]];
            }
            if (intQueue[0] == 256)
                intQueue[0] = 0;
            else
                intQueue[0] += 1;
        }
    }
    
    PyBuffer_Release(&intQueueBuf);
    PyBuffer_Release(&registersBuf);
    PyBuffer_Release(&ramBuf);
    
    return Py_BuildValue("i", cycles);
}