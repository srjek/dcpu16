#include "wx/wx.h"
#include "wx/glcanvas.h"

#include "thread.h"
#include "gdb.h"

#ifndef emulator_cpu_h
#define emulator_cpu_h


#ifndef NO_WXMAIN
#include "main.h"
DECLARE_APP(emulatorApp);
inline wxWindow* getTopLevelWindow() {
   return (*((wxApp*) (&wxGetApp()))).GetTopWindow();
}
#else
inline wxWindow* getTopLevelWindow() {
   return NULL;
}
#endif

class cpuCallback {
public:
    virtual void callback() =0;
};

class cpu;
#include "device.h"
class cpu {
public:
    volatile bool running;
    virtual void attachDebugger(gdb_remote* debugger) =0;
    virtual void createWindow() =0;
    virtual wxWindow* getWindow() =0;
    
    virtual void loadImage(const wxChar* imagePath) =0;
    virtual void Run() =0;
    virtual void Stop() =0;
    
    volatile unsigned long long& time;
    
    //deprecated, use functions instead.
    volatile unsigned short* ram;
    volatile unsigned short* registers;
    
    enum {
        REG_A = 0,
        REG_B,
        REG_C,
        REG_X,
        REG_Y,
        REG_Z,
        REG_I,
        REG_J,
        REG_PC,
        REG_SP,
        REG_EX,
        NUM_REGS
    };
    virtual int getNumRegisters() =0;
    virtual unsigned int getRegisterSize() =0;
    virtual unsigned long long getRegister(int id) =0;
    virtual void setRegister(int id, unsigned long long value) =0;
    virtual unsigned long long getRamSize() =0;
    virtual unsigned int getRamValueSize() =0;
    virtual unsigned long long getRam(unsigned long long offset) =0;
    virtual void setRam(unsigned long long offset, unsigned long long value) =0;
    virtual void setRam(unsigned long long offset, unsigned long long len, unsigned long long* values) =0;
    
    //These must be used if the current code is being executed in the cpu's thread, otherwise don't use
    //  (the cpu does not release the lock when calling callbacks or issuing hardware interrupts)
    virtual void setRegister_unsafe(int id, unsigned long long value) =0;
    virtual void setRam_unsafe(unsigned long long offset, unsigned long long value) =0;
    virtual void setRam_unsafe(unsigned long long offset, unsigned long long len, unsigned long long* values) =0;
    
    virtual void debug_run() =0;
    virtual void debug_step() =0;
    virtual void debug_stop() =0;
    virtual void debug_reset() =0;
    
    //Breakpoint/watchpoint functions return true if they are supported. If a set is supported, so must the clear.
    virtual bool debug_setBreakpoint(unsigned long long pos) =0;
    virtual bool debug_clearBreakpoint(unsigned long long pos) =0;
    //Read watchpoints
    virtual bool debug_setWatchpoint_r(unsigned long long pos) =0;
    virtual bool debug_clearWatchpoint_r(unsigned long long pos) =0;
    //Write watchpoints
    virtual bool debug_setWatchpoint_w(unsigned long long pos) =0;
    virtual bool debug_clearWatchpoint_w(unsigned long long pos) =0;
    //Access watchpoints
    virtual bool debug_setWatchpoint_a(unsigned long long pos) =0;
    virtual bool debug_clearWatchpoint_a(unsigned long long pos) =0;
    
    //Returns hardware num
    virtual unsigned int addHardware(device* hw) =0;
    virtual void interrupt(unsigned short msg) =0;
    //Schedules a callback to occur right before time
    //Warning: cpuCallback will be destroyed after it is called
    virtual void scheduleCallback(unsigned long long time, cpuCallback* callback) =0;
protected:
    inline cpu(volatile unsigned long long& time): time(time) { }
};

class cpuConfig {
protected:
    cpuConfig(): name("") { };
public:
    const char* name;
    virtual cpu* createCpu()=0;
};

#endif
