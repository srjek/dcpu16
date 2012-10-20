#include "cpu.h"
#include "device.h"
#include "sysConfig.h"

#ifndef emulator_system_h
#define emulator_system_h

class compSystem: public wxThread {
protected:
    cpu *sysCpu;
    device** devices;
    int numDevices;
public:
    compSystem(cpuConfig* cpuConfig, const wxChar* imagePath, vector<deviceConfig*> deviceConfigs) {
        sysCpu = cpuConfig->createCpu();
        if (wxStrlen(imagePath) != 0)
            sysCpu->loadImage(imagePath);
        
        numDevices = deviceConfigs.size();
        devices = new device*[numDevices];
        for (int i = 0; i < numDevices; i++)
            devices[i] = deviceConfigs[i]->createDevice(sysCpu);
    }
    ~compSystem() {
        //for (int i = 0; i < numDevices; i++)
        //    delete devices[i];
        delete[] devices;
        //delete sysCpu;
    }
    
    ExitCode Entry() {
        return 0;
    }
};

#endif
