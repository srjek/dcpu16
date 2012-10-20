#include "wx/wx.h"

#include "thread.h"

#ifndef emulator_cpu_h
#define emulator_cpu_h


#include "main.h"
DECLARE_APP(emulatorApp);
wxWindow* getTopLevelWindow();

class cpu {
public:
    virtual void createCtrlWindow() =0;
    virtual void loadImage(const wxChar* imagePath) =0;
    virtual void Run() =0;
};

class cpuConfig {
protected:
    cpuConfig(): name("") { };
public:
    const char* name;
    virtual cpu* createCpu()=0;
};

#endif
