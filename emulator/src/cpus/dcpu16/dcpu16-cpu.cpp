#include <iostream>
#include <cstdio>
#include <cstring>

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include "dcpu16-cpu.h"
#include "dcpu16-gui.h"
#include "../../strHelper.h"
#include "../../device.h"
#include "../../test.h"

class dcpu16_rom_callback: public cpuCallback {
protected:
    dcpu16_rom* device;
public:
    dcpu16_rom_callback(dcpu16_rom* device):
                        device(device) { }
    void callback();
};

class dcpu16_rom: public device {
protected:
    dcpu16* host;
    volatile bool enabled;
    unsigned short rom[512];
public:
    void reset();
    dcpu16_rom(dcpu16* host);
    ~dcpu16_rom() { }
    void createWindow() { }
    unsigned long getManufacturer() {
        return 0;  //????
    }
    unsigned long getId() {
        return 0;  //????
    }
    unsigned long getVersion() {
        return 0;  //????
    }
    
    void enable() {
        enabled = true;
    }
    void disable() {
        enabled = false;
    }
    void startup();
    int interrupt();
    
    void registerKeyHandler(keyHandler* handler) { };
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void dcpu16_rom_callback::callback() {
    device->startup();
}


//TODO: Save image elsewhere to auto reload on reset, like the other loadImage functions
void dcpu16::loadImage(size_t len, unsigned short* image) {
    memcpy((void*) ram, image, len*sizeof(unsigned short));
    romDevice->disable();
}
void dcpu16::loadImage(size_t len, char* image) {
    if (len > 0x10000*2) len = 0x10000*2;
    for (int i = 0; i < ((len >> 1) + 1); i++) {
        if (i*2 >= len)
            break;
        unsigned short c1 = image[i*2];
        if (i*2+1 >= len) {
            ram[i] = (c1 << 8) | 0;
            break;
        }
        unsigned short c2 = ((unsigned short) image[i*2+1]) & 0xFF;
        ram[i] = (c1 << 8) | c2;
    }
    romDevice->disable();
}
void dcpu16::LoadImage() {
    if (wxImagePath == 0)
        return;
    wxFileName filename;
    filename.AssignCwd();
    filename.Assign(*wxImagePath);
    if (!filename.FileExists()) {
        std::cout << "ERROR: File \"";
        std::cout << wxImagePath->mb_str(wxConvUTF8);
        std::cout << "\" does not exist" << std::endl;
        return;
    }
    if (!filename.IsFileReadable()) {
        std::cout << "ERROR: File \"";
        std::cout << wxImagePath->mb_str(wxConvUTF8);
        std::cout << "\" is not readable" << std::endl;
        return;
    }
    
    wxFFileInputStream stream(filename.GetFullPath(), wxT("rb"));
    for (int i = 0; i < 0x10000; i++) {
        unsigned short c1 = stream.GetC();
        if (stream.LastRead() == 0)
            break;
        unsigned short c2 = stream.GetC();
        if (stream.LastRead() == 0) {
            ram[i] = (c1 << 8) | 0;
            break;
        }
        ram[i] = (c1 << 8) | c2;
    }
    romDevice->disable();
    //ram[0] = 0x01 | (0x1C << 5) | (0x1F << 10); //0x7F81
    //ram[1] = 0x031d;
}
void dcpu16::reset() {
    stateMutex->Lock();
    
    for (int i = 0; i < 0x10000; i++)
        ram[i] = 0;
    for (int i = 0; i < DCPU16_NUM_REGS; i++)
        registers[i] = 0;
    for (int i = 0; i < 0x1000; i++)
        ram_debug[i] = 0;
    
    cycles = 0;
    totalCycles = 0;
    intQueueStart = 0;
    intQueueEnd = 0;
    onFire = false;
    
    callbackMutex->Lock();
    nextCallback = 0;
    callbackSchedule.clear();
    callbackMutex->Unlock();
    
    for (int i = 0; i < hwLength; i++)
        hardware[i]->reset();
    if (wxImagePath != 0)
        LoadImage();
        
    stateMutex->Unlock();
}
dcpu16::dcpu16(bool debug): cpu(totalCycles) {
    ram = new unsigned short[0x10000];
    registers = new unsigned short[DCPU16_NUM_REGS];
    
    ram_debug = new unsigned char[0x10000];;
    debugger_attached = false;
    
    hwLength = 0;
    callbackMutex = new wxMutex();
    wxImagePath = 0;
    
    running = true;
    cmdState = 1;
    this->debug = debug;
    
    ctrlWindow = NULL;
    
    stateMutex = new wxMutex();
    
    reset();
    romDevice = new dcpu16_rom(this);
}
dcpu16::~dcpu16() {
    if (ctrlWindow)
        ctrlWindow->Close(true);
    if (this->wxImagePath != 0)
        delete wxImagePath;
    delete romDevice;
    callbackMutex->Lock();
    delete callbackMutex;
    stateMutex->Lock();
    delete stateMutex;
    delete[] ram;
    delete[] registers;
    delete[] ram_debug;
}
void dcpu16::attachDebugger(gdb_remote* debugger) {\
    debugger_attached = true;
    this->debugger = debugger;
    cmdState = 0;
}
void dcpu16::createWindow() {
    if (!ctrlWindow) {
        ctrlWindow = new dcpu16CtrlWindow(wxPoint(50, 50), this);
        ctrlWindow->Show(true);
        cmdState = 0;
    }
}
wxWindow* dcpu16::getWindow() {
    if (ctrlWindow)
        return ctrlWindow;
    return getTopLevelWindow();
}

unsigned short dcpu16::nextWord() {
    unsigned short result = ram[registers[DCPU16_REG_PC]];
    registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
    cycles += 1;
    return result;
}
dcpu16::val dcpu16::read_val(int a, bool isB) {
    unsigned short pointer;
    if (a <= 0x07) {            //register
        return val(registers + a);
    } else if (a <= 0x0F) {     //[register]
        pointer = registers[a & 0x07];
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a <= 0x17) {     //[next word + register]
        pointer = (nextWord() + registers[a & 0x07]) & 0xFFFF;
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a == 0x18) {     //PUSH/POP (b/a)
        if (isB) {
            pointer = (registers[DCPU16_REG_SP] - 1) & 0xFFFF;
            registers[DCPU16_REG_SP] = pointer;
        } else {
            pointer = registers[DCPU16_REG_SP];
            registers[DCPU16_REG_SP] = (pointer + 1) & 0xFFFF;
        }
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a == 0x19) {     //PEEK   ([SP])
        pointer = registers[DCPU16_REG_SP];
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a == 0x1A) {     //PICK   ([SP + next word])
        pointer = (registers[DCPU16_REG_SP] + nextWord()) & 0xFFFF;
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a == 0x1B) {     //SP
        return val(registers + DCPU16_REG_SP);
    } else if (a == 0x1C) {     //PC
        return val(registers + DCPU16_REG_PC);
    } else if (a == 0x1D) {     //EX
        return val(registers + DCPU16_REG_EX);
    } else if (a == 0x1E) {     //[next word]
        pointer = nextWord();
        if ((ram_debug[pointer] & (DCPU16_WATCHPOINT_HW_R | DCPU16_WATCHPOINT_HW_A)) != 0)
            watchpoint_hit = true;
        return val(ram + pointer);
    } else if (a == 0x1F) {     //next word (literal)
        return val(NULL, nextWord());
    } else if (a <= 0x3F) {     //0xffff-0x1e (literal)
        pointer = a & 0x1F;
        pointer -= 1;
        return val(NULL, pointer);
    }
    return val(NULL, 0);
}
void dcpu16::write_val(val a, unsigned short value) {
    if (a.pointer != NULL) {
        *(a.pointer) = value;
        ptrdiff_t tmp = a.pointer - ram;
        if ((tmp > 0) && (tmp <= 0xFFFF)) {
            if ((ram_debug[tmp] & (DCPU16_WATCHPOINT_HW_W | DCPU16_WATCHPOINT_HW_A)) != 0) {
                watchpoint_hit = true;
            }
        }
    }
}
    
long dcpu16::sign16(unsigned long a) {
    a &= 0x0000FFFF;
    if (a & 0x8000)
        return a - 0x10000;
    return a;
}
unsigned short dcpu16::overflow(unsigned long value) {
    registers[DCPU16_REG_EX] = (value >> 16) & 0xFFFF;
    return value & 0xFFFF;
}
unsigned short dcpu16::overflowDIV(unsigned long b, unsigned long a) {
    if (a == 0) {
        registers[DCPU16_REG_EX] = 0;
        return 0;
    }    
    registers[DCPU16_REG_EX] = ((b << 16) / a) & 0xFFFF;
    return (b / a) & 0xFFFF;
}
unsigned short dcpu16::overflowDVI(long b, long a) {
    if (a == 0) {
        registers[DCPU16_REG_EX] = 0;
        return 0;
    }
    registers[DCPU16_REG_EX] = ((b << 16) / a) & 0xFFFF;
    return (b / a) & 0xFFFF;
}
unsigned short dcpu16::underflow(unsigned long value) {
    registers[DCPU16_REG_EX] = (value >> 16) & 0xFFFF;
    return value & 0xFFFF;
}

int dcpu16::skip() {
    unsigned short instruction = nextWord();
    int op = instruction & 0x1F;
    int b_code = (instruction >> 5) & 0x1F;
    int a_code = (instruction >> 10) & 0x3F;
    if (((0x10 <= a_code) && (a_code <= 0x17)) || (a_code == 0x1A) || (a_code == 0x1E) || (a_code == 0x1F))
        registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
    if (((0x10 <= b_code) && (b_code <= 0x17)) || (b_code == 0x1A) || (b_code == 0x1E) || (b_code == 0x1F))
        registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
    return (0x10 <= op) && (op <= 0x17);
}
void dcpu16::conditional(int condition) {
    if (!condition)
        while (skip());
}

void dcpu16::execOp(int op, val b, val a) {
    //This function doesn't handle op==0, caller detects that state and uses an alt function
    switch (op) {
        case 0x01:      //SET
            write_val(b, a.value);
            break;
        case 0x02:      //ADD
            write_val(b, overflow(((unsigned long) b.value) + a.value));
            break;
        case 0x03:      //SUB
            write_val(b, underflow(((unsigned long) b.value) - a.value));
            break;
        case 0x04:      //MUL
            write_val(b, overflow(((unsigned long) b.value) * a.value));
            break;
        case 0x05:      //MLI
            write_val(b, overflow(sign16(b.value) * sign16(a.value)));
            break;
        case 0x06:      //DIV
            write_val(b, overflowDIV(b.value, a.value));
            break;
        case 0x07:      //DVI
            write_val(b, overflowDVI(sign16(b.value), sign16(a.value)));
            break;
        case 0x08:      //MOD
            if (a.value == 0)
                write_val(b, 0);
            else
                write_val(b, b.value % a.value);
            break;
        case 0x09:      //MDI
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
                registers[DCPU16_REG_EX] = 0;
            else
                registers[DCPU16_REG_EX] = ((((unsigned long) b.value) << 16) >> a.value) & 0xFFFF;  //Avoiding the effects of arithmitic shifts...
            write_val(b, b.value >> a.value);
            break;
        case 0x0E:      //ASR
            registers[DCPU16_REG_EX] = ((sign16(b.value) << 16) >> a.value) & 0xFFFF;
            write_val(b, sign16(b.value) >> a.value);
            break;
        case 0x0F:      //SHL
            registers[DCPU16_REG_EX] = ((((unsigned long) b.value) << a.value) >> 16) & 0xFFFF;
            write_val(b, b.value << a.value);
            break;
        case 0x10:      //IFB
            conditional((b.value & a.value) != 0);
            break;
        case 0x11:      //IFC
            conditional((b.value & a.value) == 0);
            break;
        case 0x12:      //IFE
            conditional(b.value == a.value);
            break;
        case 0x13:      //IFN
            conditional(b.value != a.value);
            break;
        case 0x14:      //IFG
            conditional(b.value > a.value);
            break;
        case 0x15:      //IFA
            conditional(sign16(b.value) > sign16(a.value));
            break;
        case 0x16:      //IFL
            conditional(b.value < a.value);
            break;
        case 0x17:      //IFU
            conditional(sign16(b.value) < sign16(a.value));
            break;
        case 0x18:
        case 0x19:
            break;
        case 0x1A:      //ADX
            write_val(b, overflow((((unsigned long) b.value) + a.value) + registers[DCPU16_REG_EX]));
            break;
        case 0x1B:      //SBX
            if (registers[DCPU16_REG_EX] != 0)
                write_val(b, underflow( (((unsigned long) b.value) - a.value) - (0x10000 - ((unsigned long) registers[DCPU16_REG_EX])) ));
            else
                write_val(b, underflow(((unsigned long) b.value) - a.value));
            break;
        case 0x1C:
        case 0x1D:
            break;
        case 0x1E:      //STI
            write_val(b, a.value);
            registers[DCPU16_REG_I] += 1;
            registers[DCPU16_REG_J] += 1;
            break;
        case 0x1F:      //STD
            write_val(b, a.value);
            registers[DCPU16_REG_I] -= 1;
            registers[DCPU16_REG_J] -= 1;
            break;
    }
}
void dcpu16::execExtOp(int op, val a) {
    unsigned int num;
    unsigned long tmp;
    switch (op) {
        case 0x01:      //JSR <a>
            //SET PUSH, PC
            execOp(1, read_val(0x18, true), val(registers + DCPU16_REG_PC));
            //SET PC, <a>
            registers[DCPU16_REG_PC] = a.value;
            break;
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
            break;
        case 0x07:      //HCF
            onFire = true;
            break;
        case 0x08:      //INT
            interrupt(a.value);
            break;
        case 0x09:      //IAG <a>
            write_val(a, registers[DCPU16_REG_IA]);
            break;
        case 0x0A:      //IAS <a>
            registers[DCPU16_REG_IA] = a.value;
            break;
        case 0x0B:      //RFI
            //IAQ 0 -- should disable queuing
            registers[DCPU16_REG_IAQ] = 0;
            //SET A, POP
            execOp(1, val(registers + DCPU16_REG_A), read_val(0x18, false));
            //SET PC, POP
            execOp(1, val(registers + DCPU16_REG_PC), read_val(0x18, false));
            break;
        case 0x0C:      //IAQ
            registers[DCPU16_REG_IAQ] = a.value;
            break;
        case 0x0D:
        case 0x0E:
        case 0x0F:
            break;
        case 0x10:      //HWN
            write_val(a, hwLength);
            break;
        case 0x11:      //HWQ
            num = a.value;
            if (num < hwLength) {
                tmp = hardware[num]->getId();
                registers[DCPU16_REG_A] = (unsigned short) (tmp & 0xFFFF);
                registers[DCPU16_REG_B] = (unsigned short) ((tmp >> 16) & 0xFFFF);
                tmp = hardware[num]->getVersion();
                registers[DCPU16_REG_C] = (unsigned short) (tmp & 0xFFFF);
                tmp = hardware[num]->getManufacturer();
                registers[DCPU16_REG_X] = (unsigned short) (tmp & 0xFFFF);
                registers[DCPU16_REG_Y] = (unsigned short) ((tmp >> 16) & 0xFFFF);
            }
            break;
        case 0x12:      //HWI
            num = a.value;
            if (debug)
                std::cout << "dcpu16: Interrupt hardware #" << num << std::endl;
            if (num < hwLength)
                cycles += hardware[num]->interrupt();
            break;
    }
}
 
void dcpu16::handleCallbacks() {
    while ( (nextCallback != 0) && (nextCallback <= (totalCycles+1)) ) {
        callbackMutex->Lock();
        cpuCallback* callback = callbackSchedule.front().value;
        //Because we release the lock, we got to pop before hand
        // (otherwise we would reacquire the lock and there will be no clean way to remove the callback)
        callbackSchedule.pop_front();
        if (callbackSchedule.empty())
            nextCallback = 0;
        else
            nextCallback = callbackSchedule.front().key;
        //Release before call, because call might attempt to schedule another another callback
        callbackMutex->Unlock();
        
        if (debug)
            std::cout << "dcpu16: Calling callback from " << time << ". Next callback queued is at " << nextCallback << std::endl;
        callback->callback();
        delete callback;
    }
}

void dcpu16::cycle(int count) {
    stateMutex->Lock();
    
    cycles -= count;
    int initialCycles = cycles;
    watchpoint_hit = false;
    if (debugger_attached) {
        while (cycles < 0) {
            handleCallbacks();
            
            if (((ram_debug[registers[DCPU16_REG_PC]] & DCPU16_BREAKPOINT_HW) != 0) || watchpoint_hit) {
                debug_stop();
                initialCycles -= cycles;    //We do this to keep cycle counts accurate despite clearing the cycles counter.
                cycles = 0; //We usually don't stop mid loop, so we keep the emulator stopped by clearing the counter.
                debugger->onCpuStop();
                break;
            }
            
            unsigned short instruction = nextWord();
            int op = instruction & 0x1F;
            int b_code = (instruction >> 5) & 0x1F;
            int a_code = (instruction >> 10) & 0x3F;
            val a = read_val(a_code, false);
            if (op != 0) {
                val b = read_val(b_code, true);
                cycles += opcodeCycles[op];
                execOp(op, b, a);
            } else {
                cycles += extOpcodeCycles[b_code];
                execExtOp(b_code, a);
            }
            
            if ((intQueueStart != intQueueEnd) && (registers[DCPU16_REG_IAQ] == 0)) {
                if (registers[DCPU16_REG_IA] != 0) {
                    //IAQ 1 -- should enable queuing
                    registers[DCPU16_REG_IAQ] = 1;
                    //SET PUSH, PC
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_PC));
                    //SET PUSH, A
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_A));
                    //SET PC, IA
                    registers[DCPU16_REG_PC] = registers[DCPU16_REG_IA];
                    //SET A, <msg>
                    registers[DCPU16_REG_A] = intQueue[intQueueStart];
                }
                if (intQueueStart >= 256)
                    intQueueStart = 0;
                else
                    intQueueStart++;
            }
            
        }
    } else {
        while (cycles < 0) {
            handleCallbacks();
            
            unsigned short instruction = nextWord();
            int op = instruction & 0x1F;
            int b_code = (instruction >> 5) & 0x1F;
            int a_code = (instruction >> 10) & 0x3F;
            val a = read_val(a_code, false);
            if (op != 0) {
                val b = read_val(b_code, true);
                cycles += opcodeCycles[op];
                execOp(op, b, a);
            } else {
                cycles += extOpcodeCycles[b_code];
                execExtOp(b_code, a);
            }
            
            if ((intQueueStart != intQueueEnd) && (registers[DCPU16_REG_IAQ] == 0)) {
                if (registers[DCPU16_REG_IA] != 0) {
                    //IAQ 1 -- should enable queuing
                    registers[DCPU16_REG_IAQ] = 1;
                    //SET PUSH, PC
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_PC));
                    //SET PUSH, A
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_A));
                    //SET PC, IA
                    registers[DCPU16_REG_PC] = registers[DCPU16_REG_IA];
                    //SET A, <msg>
                    registers[DCPU16_REG_A] = intQueue[intQueueStart];
                }
                if (intQueueStart >= 256)
                    intQueueStart = 0;
                else
                    intQueueStart++;
            }
            
        }
    }
    
    totalCycles += (cycles - initialCycles);
    stateMutex->Unlock();
}
    
int dcpu16::getNumRegisters() {
    return DCPU16_NUM_REGS;
}
unsigned int dcpu16::getRegisterSize() {
    return 2;
}
unsigned long long dcpu16::getRegister(int id) {
    if (id < DCPU16_NUM_REGS)
        return registers[id];
    return 0;
}
void dcpu16::setRegister(int id, unsigned long long value) {
    if (id < DCPU16_NUM_REGS) {
        stateMutex->Lock();
        registers[id] = value;
        stateMutex->Unlock();
    }
}
unsigned long long dcpu16::getRamSize() {
    return 0x10000;
}
unsigned int dcpu16::getRamValueSize() {
    return 2;
}
unsigned long long dcpu16::getRam(unsigned long long offset) {
    return ram[offset&0xFFFF];
}
void dcpu16::setRam(unsigned long long offset, unsigned long long value) {
    stateMutex->Lock();
    setRam_unsafe(offset, value);
    stateMutex->Unlock();
}
void dcpu16::setRam(unsigned long long offset, unsigned long long len, unsigned long long* values) {
    stateMutex->Lock();
    setRam_unsafe(offset, len, values);
    stateMutex->Unlock();
}
    
void dcpu16::debug_run() {
    cmdState = 1;
}
void dcpu16::debug_step() {
    if (cmdState == 3)
        cmdState = 2;
    else
        cmdState = 3;
}
void dcpu16::debug_stop() {
    cmdState = 0;
    if (debugger != NULL)
        debugger->onCpuStop();
}
void dcpu16::debug_reset() {
    reset();
}
bool dcpu16::debug_setBreakpoint(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] |= DCPU16_BREAKPOINT_HW;
    return true;
}
bool dcpu16::debug_clearBreakpoint(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] &= ~DCPU16_BREAKPOINT_HW;
    return true;
}
bool dcpu16::debug_setWatchpoint_r(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] |= DCPU16_WATCHPOINT_HW_R;
    return true;
}
bool dcpu16::debug_clearWatchpoint_r(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] &= ~DCPU16_WATCHPOINT_HW_R;
    return true;
}
bool dcpu16::debug_setWatchpoint_w(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] |= DCPU16_WATCHPOINT_HW_W;
    return true;
}
bool dcpu16::debug_clearWatchpoint_w(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] &= ~DCPU16_WATCHPOINT_HW_W;
    return true;
}
bool dcpu16::debug_setWatchpoint_a(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] |= DCPU16_WATCHPOINT_HW_A;
    return true;
}
bool dcpu16::debug_clearWatchpoint_a(unsigned long long pos) {
    if (debugger_attached && pos <= 0xFFFF)
        ram_debug[pos] &= ~DCPU16_WATCHPOINT_HW_A;
    return true;
}
   
unsigned int dcpu16::addHardware(device* hw) {
    hardware[hwLength] = hw;
    return hwLength++;
}

void dcpu16::interrupt(unsigned short msg) {
    if (debug)
        std::cout << "dcpu16: Interrupted with msg " << msg << std::endl;
    intQueue[intQueueEnd] = msg;
    int tmp = intQueueStart;
    if (intQueueEnd+1 >= 256)
        intQueueEnd = 0;
    else
        intQueueEnd++;
    
    if (intQueueEnd == tmp)
        onFire = true; //raise Exception("DCPU16 is on fire. Behavior undefined")
}

void dcpu16::scheduleCallback(unsigned long long time, cpuCallback* callback) {
    callbackMutex->Lock();
    if (time == 0)
        time++;
    callbackSchedule.insert(time, callback);
    if ((nextCallback == 0) || (time < nextCallback))
        nextCallback = time;
    if (debug)
        std::cout << "dcpu16: Callback scheduled for " << time << ". Next callback queued is at " << nextCallback << std::endl;
    callbackMutex->Unlock();
}

void dcpu16::loadImage(wxString wxImagePath) {
    if (this->wxImagePath != 0)
        delete this->wxImagePath;
    this->wxImagePath = new wxString(wxImagePath);
    LoadImage();
}
void dcpu16::loadImage(const wxChar* imagePath) {
    loadImage(wxString(imagePath));
}
void dcpu16::Run() {
    if (running) {
        if (((cmdState == 2) || (cmdState == 3)) && cmdState != lastCmdState) {
            cycle(1);       //step
            cycle(cycles);
            if (debugger != NULL and cmdState != 0) //If we haven't hit a breakpoint, inform the debugger we stopped
                debugger->onCpuStop();
        } else if (cmdState == 1)
            cycle(10000);   //run
        else if (cmdState >= 4) {   //reset
            if (cmdState == 5)
                cmdState = 1;
            else
                cmdState = 0;
            reset();
        }
        //stop
        lastCmdState = cmdState;
    }
}
void dcpu16::Stop() {
    running = false;
}

int dcpu16::disassembleCurInstruction(wxChar* buffer, int bufferSize) {
    if (bufferSize < 26)
        return 0;
    wxChar empty[] = wxT("");
    wxChar opcodes[][4] = {
            wxT("")   , wxT("SET"), wxT("ADD"), wxT("SUB"), wxT("MUL"), wxT("MLI"), wxT("DIV"), wxT("DVI"),
            wxT("MOD"), wxT("MDI"), wxT("AND"), wxT("BOR"), wxT("XOR"), wxT("SHR"), wxT("ASR"), wxT("SHL"),
            wxT("IFB"), wxT("IFC"), wxT("IFE"), wxT("IFN"), wxT("IFG"), wxT("IFA"), wxT("IFL"), wxT("IFU"),
            wxT("")   , wxT("")   , wxT("ADX"), wxT("SBX"), wxT("")   , wxT("")   , wxT("STI"), wxT("STD")};
    wxChar ext_opcodes[][4] = {
            wxT("")   , wxT("JSR"), wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("HCF"),
            wxT("INT"), wxT("IAG"), wxT("IAS"), wxT("RFI"), wxT("IAQ"), wxT("")   , wxT("")   , wxT("")   ,
            wxT("HWN"), wxT("HWQ"), wxT("HWI"), wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   ,
            wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   };
    wxChar values[][10] = {
            wxT("A")    , wxT("B")    , wxT("C")    , wxT("X")    , wxT("Y")    , wxT("Z")    , wxT("I")    , wxT("J")    ,
            wxT("[A]")  , wxT("[B]")  , wxT("[C]")  , wxT("[X]")  , wxT("[Y]")  , wxT("[Z]")  , wxT("[I]")  , wxT("[J]")  ,
            wxT("[A+@]"), wxT("[B+@]"), wxT("[C+@]"), wxT("[X+@]"), wxT("[Y+@]"), wxT("[Z+@]"), wxT("[I+@]"), wxT("[J+@]"),
            wxT("POP")  , wxT("PEEK") , wxT("[SP+@]"), wxT("SP")  , wxT("PC")   , wxT("EX")   , wxT("[@]")  , wxT("@")    ,
            wxT("0xFFFF"), wxT("0x0000"), wxT("0x0001"), wxT("0x0002"), wxT("0x0003"), wxT("0x0004"), wxT("0x0005"), wxT("0x0006"),
            wxT("0x0007"), wxT("0x0008"), wxT("0x0009"), wxT("0x000A"), wxT("0x000B"), wxT("0x000C"), wxT("0x000D"), wxT("0x000E"),
            wxT("0x000F"), wxT("0x0010"), wxT("0x0011"), wxT("0x0012"), wxT("0x0013"), wxT("0x0014"), wxT("0x0015"), wxT("0x0016"),
            wxT("0x0017"), wxT("0x0018"), wxT("0x0019"), wxT("0x001A"), wxT("0x001B"), wxT("0x001C"), wxT("0x001D"), wxT("0x001E")};
    unsigned short instruction = ram[registers[DCPU16_REG_PC]];
    int op = instruction & 0x1F;
    int b_code = (instruction >> 5) & 0x1F;
    int a_code = (instruction >> 10) & 0x3F;
    
    wxChar* opStr = opcodes[op];
    if (op == 0)
        opStr = ext_opcodes[b_code];
    
    int length = 0;
    if (opStr[0] == 0) {
        length += wxStrcpy(buffer, wxT("DAT 0x"), bufferSize);
        printHex(buffer+length, 4, instruction);
        length += 4;
    } else {
        length += wxStrcpy(buffer, opStr, bufferSize);
        buffer[length++] = wxT(' ');
        unsigned short PC = (registers[DCPU16_REG_PC]+1)&0xFFFF;
        
        if (((a_code >= 0x10) && (a_code < 0x18)) || (a_code == 0x1A) || (a_code == 0x1E) || (a_code == 0x1F)) {
            PC = (PC+1)&0xFFFF;
        }
        if (op != 0) {
            wxChar* bStr = values[b_code];
            if (b_code == 0x18)
                bStr = wxT("PUSH");
            length += wxStrcpy(buffer+length, bStr, bufferSize-length);
            if (buffer[length-1] == wxT('@')) {
                buffer[length-1] = wxT('0');
                buffer[length++] = wxT('x');
                printHex(buffer+length, 4, ram[PC]);
                PC = (PC+1)&0xFFFF;
                length += 4;
            } else if (buffer[length-2] == wxT('@')) {
                buffer[length-2] = wxT('0');
                buffer[length-1] = wxT('x');
                printHex(buffer+length, 4, ram[PC]);
                length += 4;
                PC = (PC+1)&0xFFFF;
                buffer[length++] = wxT(']');
            }
            buffer[length++] = wxT(',');
            buffer[length++] = wxT(' ');
        }
        length+= wxStrcpy(buffer+length, values[a_code], bufferSize-length);
        if (buffer[length-1] == wxT('@')) {
            buffer[length-1] = wxT('0');
            buffer[length++] = wxT('x');
            printHex(buffer+length, 4, ram[(registers[DCPU16_REG_PC]+1)&0xFFFF]);
            length += 4;
        } else if (buffer[length-2] == wxT('@')) {
            buffer[length-2] = wxT('0');
            buffer[length-1] = wxT('x');
            printHex(buffer+length, 4, ram[(registers[DCPU16_REG_PC]+1)&0xFFFF]);
            length += 4;
            buffer[length++] = wxT(']');
        }
    }
    return length;
}

const int dcpu16::extOpcodeCycles[] = {
                0, 2, 0, 0, 0, 0, 0, 0,
                3, 0, 0, 2, 1, 0, 0, 0,
                1, 3, 3, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0 };
const int dcpu16::opcodeCycles[] = {
                0, 0, 1, 1, 1, 1, 2, 2,
                2, 2, 0, 0, 0, 0, 0, 0,
                1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 2, 2, 0, 0, 1, 1 };


void dcpu16_rom::reset() {
    enabled = true;
    //Callbacks are processed before any cycles are run!
    host->scheduleCallback(0, new dcpu16_rom_callback(this));
}
dcpu16_rom::dcpu16_rom(dcpu16* host) {
    this->host = host;
    host->addHardware(this);
    
    for (int i = 0; i < 512; i++)
        rom[i] = 0;
    reset();
    
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    
    wxFileName filename;
    filename.Assign(stdpath.GetExecutablePath());
    filename.AppendDir(_("cpus"));
    filename.AppendDir(_("dcpu16"));
    filename.SetFullName(_("firmware.bin"));
    if (!filename.FileExists()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" does not exist" << std::endl;
        return;
    }
    if (!filename.IsFileReadable()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" is not readable" << std::endl;
        return;
    }
    
    wxFFileInputStream romFile(filename.GetFullPath(), wxT("rb"));
    if (romFile.GetLength() <= 0) {
        std::cout << "ERROR: Failed to open \"firmware.bin\"";
    } else {
        int i;
        for (i = 0; i < 512; i++) {
            unsigned short word;
            int c1 = romFile.GetC();
            if (romFile.LastRead() == 0)
                break;
            int c2 = romFile.GetC();
            if (romFile.LastRead() == 0) {
                word = c1 << 8;
                rom[i++] = word;
                break;
            }
            word = (c1 << 8) | c2;
            rom[i] = word;
        }
        for (; i < 512; i++)
            rom[i] = 0;
    }
}
void dcpu16_rom::startup() {
    if (enabled)
        host->cycles += interrupt(); //We have to manage the interrupt ourselves...
}
int dcpu16_rom::interrupt() {
    unsigned short A = host->registers[0];  //Specs don't say anything about using this, so ignore
    
    int offset = host->registers[1]; //Start copying into memory at position B
    for (int i = 0; i < 512; i++) {
        host->ram[(offset + i)&0xFFFF] = rom[i];
    }
    return 512; //Specs don't say anything about this either, so I gave it 512 cycles, as the rom has 512 words
}

dcpu16Config::dcpu16Config() { 
    debug = false;
    name = "dcpu16";
}
dcpu16Config::dcpu16Config(int& argc, wxChar**& argv) {
    name = "dcpu16";
    debug = false;
    if (argc > 0) {
        if (wxStrcmp(argv[0], _("--debug")) == 0) {
            debug = true;
            argv++; argc--;
        }
    }
}
cpu* dcpu16Config::createCpu() {
    return new dcpu16(debug);
}
