#include "cpu.h"

#ifndef emulator_device_h
#define emulator_device_h

class device {
public:
    virtual bool hasWindow();
    virtual wxWindow createWindow();
};

class deviceConfig {
protected:
    deviceConfig(): name("") { };
public:
    const char* name;
    virtual device* createDevice(cpu* host)=0;
};

#endif
