#include <vector>
#include "wx/wx.h"

#include "thread.h"
#include "cpu.h"
#include "device.h"

#ifndef emulator_system_h
#define emulator_system_h

class compSystem: public thread, public wxThread {
protected:
    cpu *sysCpu;
    device** devices;
    int numDevices;
public:
    compSystem(cpuConfig* cpuConfig, const wxChar* imagePath, std::vector<deviceConfig*> deviceConfigs);
    ~compSystem();
    
    wxThreadError Create();
    wxThreadError Run();
    ExitCode Entry();
    void Stop();
    wxThread::ExitCode Wait();
};

class sysConfig {
public:
    cpuConfig* cpu;
    const wxChar* imagePath;
    std::vector<deviceConfig*> devices;
    
    sysConfig(int& argc, wxChar**& argv);
    ~sysConfig();
    compSystem* createSystem();
};

#endif
