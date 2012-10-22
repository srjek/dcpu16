#include <iostream>
#include "LEM1802.h"

class LEM1802: public device {
protected:
    cpu* host;
public:
    LEM1802(cpu* host) {
        this->host = host;
        host->addHardware(this);
    }
    
    virtual void createWindow() { }
    virtual unsigned long getManufacturer() {
        return 0x1c6c8b36;  //Nya Elektriska
    }
    virtual unsigned long getId() {
        return 0x7349f615;  //LEM180X series
    }
    virtual unsigned long getVersion() {
        return 0x1802;  //1802
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

device* LEM1802Config::createDevice(cpu* host) {
    return new LEM1802(host);
}