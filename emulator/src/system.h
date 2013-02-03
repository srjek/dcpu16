#include <vector>
#include "wx/wx.h"

#include "thread.h"
#include "cpu.h"
#include "device.h"
#include "gdb.h"

#ifndef emulator_system_h
#define emulator_system_h

class compSystem: public thread, public wxThread {
protected:
    cpu *sysCpu;
    device** devices;
    int numDevices;
    gdb_remote* debugger;
public:
    compSystem(cpuConfig* cpuConfig, const wxChar* imagePath, unsigned int gdb_port, std::vector<deviceConfig*> deviceConfigs);
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
    unsigned int gdb_port;
    
    sysConfig(int& argc, wxChar**& argv);
    ~sysConfig();
    compSystem* createSystem();
};

#endif
