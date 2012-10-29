#include <iostream>
#include "genericClock.h"

class genericClock;
class genericClockCallback: public cpuCallback {
protected:
    genericClock* clock;
    unsigned long long uid;
public:
    genericClockCallback(genericClock* clock, unsigned long long uid):
                        clock(clock), uid(uid) { }
    void callback();
};

class genericClock: public device {
protected:
    cpu* host;
    volatile unsigned short interruptMsg;
    volatile unsigned long long uid;
    volatile unsigned long long timing;
    volatile unsigned short ticks;
    
    bool debug;
    
public:
    genericClock(cpu* host, bool debug) {
        this->host = host;
        interruptMsg = 0;
        uid = 0;
        timing = 0;
        ticks = 0;
        
        this->debug = debug;
        host->addHardware(this);
    }
    ~genericClock() { }
    void createWindow() { }
    unsigned long getManufacturer() {
        return 515079825;  //????
    }
    unsigned long getId() {
        return 0x12d0b402;  //Generic Keyboard
    }
    unsigned long getVersion() {
        return 1;  //????
    }
    
    int interrupt() {
        unsigned short A = host->registers[0];
        if (A == 0) {
            uid++;
            unsigned short B = host->registers[1];
            timing = 0;
            if (B > 0) {
                timing = 100000;
                timing *= B;
                timing /= 60;
            }
            ticks = 0;
            if (debug)
                std::cout << "clock: Scheduling tick for " << host->time+timing << ". Current uid is " << uid << std::endl;
            host->scheduleCallback(host->time+timing, new genericClockCallback(this, uid));
        } else if (A == 1) {
            host->registers[1] = ticks;
        } else if (A == 2)
            interruptMsg = host->registers[1];
        return 0;
    }
    void registerKeyHandler(keyHandler* handler) { };
    
    void onTick(unsigned long long uid) {
        if (debug)
            std::cout << "clock: Ticked with uid " << uid << ". Current uid is " << this->uid << std::endl;
        if (this->uid != uid)
            return;
        ticks++;
        host->scheduleCallback(host->time+timing, new genericClockCallback(this, uid));
        
        if (interruptMsg != 0)
            host->interrupt(interruptMsg);
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void genericClockCallback::callback() {
    clock->onTick(uid);
}
device* genericClockConfig::createDevice(cpu* host) {
    return new genericClock(host, debug);
}
