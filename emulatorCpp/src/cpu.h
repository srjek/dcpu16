#include "wx/wx.h"

#include "thread.h"

#ifndef emulator_cpu_h
#define emulator_cpu_h


#include "main.h"
DECLARE_APP(emulatorApp);
wxWindow* getTopLevelWindow();

class cpuCallback {
public:
    virtual void callback() =0;
};

class cpu;
#include "device.h"
class cpu {
public:
    volatile bool running;
    virtual void createWindow() =0;
    virtual wxWindow* getWindow() =0;
    virtual void loadImage(const wxChar* imagePath) =0;
    virtual void Run() =0;
    virtual void Stop() =0;
    
    volatile unsigned long long& time;
    volatile unsigned short* ram;
    volatile unsigned short* registers;
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
