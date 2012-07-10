#ifdef _MSC_VER
#include "stdint.h"
#else
#include <stdint.h>
#endif
#include <stdlib.h>
#include <string.h>

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