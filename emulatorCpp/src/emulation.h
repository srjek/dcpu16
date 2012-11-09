#include <vector>
#include "wx/wx.h"

#ifndef emulator_emulation_h
#define emulator_emulation_h

#ifndef emulator_system_h
class compSystem;
class sysConfig;
#endif

class emulation {
    compSystem** systems;
    int nSystems;
public:
    emulation(std::vector<sysConfig*> systemConfigs);
    ~emulation();
    
    void Run();
    void Stop();
    void Wait();
};

class emulationConfig {
    std::vector<sysConfig*> systemConfigs;
    
public:
    emulationConfig(int argc, wxChar** argv);
    ~emulationConfig();
    emulation* createEmulation();
    
    void print();
};

#endif
