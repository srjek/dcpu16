#include "wx/wx.h"

#include "thread.h"

#ifndef emulator_device_h
#define emulator_device_h

class keyHandler {
public:
    virtual void OnKeyEvent(int keyCode, bool isDown) =0;
};

class deviceState {
protected:
    deviceState(): name("") { };
public:
    const char* name;
};

class device;
#include "cpu.h"
class device: public thread {
public:
    virtual void createWindow() =0;
    virtual unsigned long getId() =0;
    virtual unsigned long getVersion() =0;
    virtual unsigned long getManufacturer() =0;
    
    virtual void reset() =0;
    virtual int interrupt() =0;
    virtual void registerKeyHandler(keyHandler* handler) =0;
//    virtual int attachKeyboard() =0;

    virtual deviceState* saveState() =0;
    virtual void restoreState(deviceState* state) =0;
};

class deviceConfig {
protected:
    deviceConfig(): name("") { };
public:
    const char* name;
    virtual bool providesKeyboard()=0;
    virtual bool consumesKeyboard()=0;
    virtual device* createDevice(cpu* host)=0;
    virtual device* createDevice(cpu* host, device* keyboardProvider)=0;
};

#endif
