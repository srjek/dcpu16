#include "wx/wx.h"

#include "thread.h"

#ifndef emulator_device_h
#define emulator_device_h

class device;
#include "cpu.h"
class device: public thread {
public:
    virtual void createWindow() =0;
    virtual unsigned long getId() =0;
    virtual unsigned long getVersion() =0;
    virtual unsigned long getManufacturer() =0;
};

class deviceConfig {
protected:
    deviceConfig(): name("") { };
public:
    const char* name;
    virtual device* createDevice(cpu* host)=0;
};

#endif
