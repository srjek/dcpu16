#include "wx/wx.h"
#include "dcpu16.h"
#include "../../orderedForwardMap.h"

#ifndef emulator_dcpu16_cpu_h
#define emulator_dcpu16_cpu_h

class dcpu16;
#include "dcpu16-gui.h"

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

class dcpu16_rom;
class dcpu16: public cpu {
    friend class dcpu16_rom;
    friend class dcpu16CtrlWindow;
protected:
    static const int extOpcodeCycles[];
    static const int opcodeCycles[];
    class val {
    public:
        volatile unsigned short* pointer;
        unsigned short value;
        inline val(volatile unsigned short* pointer) {
            this->pointer = pointer;
            this->value = *pointer;
        }
        inline val(volatile unsigned short* pointer, unsigned short value) {
            this->pointer = pointer;
            this->value = value;
        }
    };
    
    dcpu16CtrlWindow* ctrlWindow;
    volatile int cmdState;
    int lastCmdState;
    
    wxMutex* stateMutex;
    //volatile unsigned short ram[0x10000];
    //volatile unsigned short registers[DCPU16_NUM_REGS];
    volatile unsigned char* ram_debug;
    bool debugger_attached;
    gdb_remote* debugger;
    bool watchpoint_hit;
    #define DCPU16_BREAKPOINT_HW (1<<0)
    #define DCPU16_WATCHPOINT_HW_R (1<<1)
    #define DCPU16_WATCHPOINT_HW_W (1<<2)
    #define DCPU16_WATCHPOINT_HW_A (1<<3)
    
    int cycles; //This is our debt/credit counter
    volatile unsigned long long totalCycles; //This is how many cycles total we have done

    device* hardware[0x10000];
    unsigned long hwLength;
    dcpu16_rom* romDevice;
    
    volatile unsigned short intQueue[256];
    volatile int intQueueStart;
    volatile int intQueueEnd;
    
    wxMutex* callbackMutex;
    ordered_forward_map<unsigned long long, cpuCallback*> callbackSchedule;
    unsigned long long nextCallback;
    
    volatile bool onFire;
    bool debug;
    wxString* wxImagePath;
    
    void LoadImage();
public:
    void reset();
    dcpu16(bool debug);
    ~dcpu16();
    void attachDebugger(gdb_remote* debugger);
    void createWindow();
    wxWindow* getWindow();
    
protected:
    unsigned short nextWord();
    val read_val(int a, bool isB);
    void write_val(val a, unsigned short value);
    
    long sign16(unsigned long a);
    unsigned short overflow(unsigned long value);
    unsigned short overflowDIV(unsigned long b, unsigned long a);
    unsigned short overflowDVI(long b, long a);
    unsigned short underflow(unsigned long value);

    int skip();
    void conditional(int condition);
    
    void execOp(int op, val b, val a);
    void execExtOp(int op, val a);
 
    void handleCallbacks();
public:
    void cycle(int count);
    
    
    inline void setRegister_unsafe(int id, unsigned long long value) {
        if (id < DCPU16_NUM_REGS)
            registers[id] = value;
    }
    inline void setRam_unsafe(unsigned long long offset, unsigned long long value) {
        ram[offset&0xFFFF] = value;
    }
    inline void setRam_unsafe(unsigned long long offset, unsigned long long len, unsigned long long* values) {
        for (unsigned long long i = 0; i < len; i++) {
            ram[(offset+i)&0xFFFF] = values[i];
        }
    }
    
    int getNumRegisters();
    unsigned int getRegisterSize();
    
    unsigned long long getRegister(int id);
    void setRegister(int id, unsigned long long value);
    
    unsigned long long getRamSize();
    unsigned int getRamValueSize();
    
    unsigned long long getRam(unsigned long long offset);
    void setRam(unsigned long long offset, unsigned long long value);
    void setRam(unsigned long long offset, unsigned long long len, unsigned long long* values);
    
    void debug_run();
    void debug_step();
    void debug_stop();
    void debug_reset();
    bool debug_setBreakpoint(unsigned long long pos);
    bool debug_clearBreakpoint(unsigned long long pos);
    bool debug_setWatchpoint_r(unsigned long long pos);
    bool debug_clearWatchpoint_r(unsigned long long pos);
    bool debug_setWatchpoint_w(unsigned long long pos);
    bool debug_clearWatchpoint_w(unsigned long long pos);
    bool debug_setWatchpoint_a(unsigned long long pos);
    bool debug_clearWatchpoint_a(unsigned long long pos);
    
    
    unsigned int addHardware(device* hw);
    
    void interrupt(unsigned short msg);
    
    void scheduleCallback(unsigned long long time, cpuCallback* callback);
    
    systemState* saveSystemState();
    void restoreSystemState(systemState* state);
    
    void loadImage(size_t len, char* image);
    void loadImage(size_t len, unsigned short* image);
    void loadImage(wxString wxImagePath);
    void loadImage(const wxChar* imagePath);
    void Run();
    void Stop();
    
    int disassembleCurInstruction(wxChar* buffer, int bufferSize);
};

#endif
