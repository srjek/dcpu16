#include <iostream>
#include "genericClock.h"

class genericClock_state: public deviceState {
public:
    unsigned short interruptMsg;
    unsigned long long uid;
    unsigned long long timing;
    unsigned short ticks;
    
    genericClock_state() {
        name = "genericClock";
    }
};

class genericClock;

class genericClockCallbackState: public cpuCallbackState {
protected:
    genericClock* clock;
    unsigned long long uid;
public:
    genericClockCallbackState(genericClock* clock, unsigned long long uid):
                        clock(clock), uid(uid) { }
    void restoreCallback(unsigned long long time);
};
class genericClockCallback: public cpuCallback {
protected:
    genericClock* clock;
    unsigned long long uid;
public:
    genericClockCallback(genericClock* clock, unsigned long long uid):
                        clock(clock), uid(uid) { }
    void callback();
    cpuCallbackState* saveState() {
        return new genericClockCallbackState(clock, uid);
    }
};

class genericClock: public device {
    friend class genericClockCallbackState;
protected:
    cpu* host;
    volatile unsigned short interruptMsg;
    volatile unsigned long long uid;
    volatile unsigned long long timing;
    volatile unsigned short ticks;
    
    bool debug;
    
public:
    void reset() {
        interruptMsg = 0;
        uid = 0;
        timing = 0;
        ticks = 0;
    }
    genericClock(cpu* host, bool debug) {
        this->host = host;
        this->debug = debug;
        host->addHardware(this);
        reset();
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
    
    deviceState* saveState() {
        genericClock_state* result = new genericClock_state();
        
        result->interruptMsg = interruptMsg;
        result->uid = uid;
        result->timing = timing;
        result->ticks = ticks;
        
        return result;
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void genericClockCallback::callback() {
    clock->onTick(uid);
}
void genericClockCallbackState::restoreCallback(unsigned long long time) {
    clock->host->scheduleCallback(time, new genericClockCallback(clock, uid));
}
device* genericClockConfig::createDevice(cpu* host) {
    return new genericClock(host, debug);
}
