#include <iostream>
#include "SPC2000.h"

class SPC2000: public device {
protected:
    const static unsigned short ERROR_TOO_MANY_HASH = 0x0000;
    const static unsigned short ERROR_NOT_IN_VACUUM = 0x0001;
    const static unsigned short ERROR_NOT_ENOUGH_FUEL = 0x0002;
    const static unsigned short ERROR_INHOMOGENEOUS_GRAVITATIONAL_FIELD = 0x0003;
    const static unsigned short ERROR_TOO_MUCH_ANGULAR_MOMENTUM = 0x0004;
    const static unsigned short ERROR_DOOR_OPEN = 0x0005;
    const static unsigned short ERROR_MECHANICAL = 0x0006;
    const static unsigned short ERROR_UNKNOWN = 0xFFFF;
    
    const static unsigned short UNIT_MILLISECONDS = 0x0000;
    const static unsigned short UNIT_MINUTES = 0x0001;
    const static unsigned short UNIT_DAYS = 0x0002;
    const static unsigned short UNIT_YEARS = 0x0003;
    
    
    cpu* host;
    bool debug;
    
    unsigned long long skip;
    unsigned short unit;
    
public:
    SPC2000(cpu* host, bool debug) {
        this->host = host;
        this->debug = debug;
        
        skip = 0;
        unit = 0;
        
        host->addHardware(this);
    }
    ~SPC2000() { }
    void createWindow() { }
    unsigned long getManufacturer() {
        return 0x1c6c8b36;  //Nya Elektriska
    }
    unsigned long getId() {
        return 0x40e41d9d;  //SPC2000
    }
    unsigned long getVersion() {
        return 0x005e;  //????
    }
    
    int _interrupt(unsigned short A) {
        if (A == 0) {
            host->registers[1] = 0;
            //ERROR_NOT_IN_VACUUM might qualify as well, except I wouldn't know how a vacuum detector works in a lack of existance, so.....
            host->registers[2] = ERROR_UNKNOWN;
            //Perhaps I should use ERROR_MECHANICAL?
        } else if (A == 1) {
            unsigned short B = host->registers[1];
            skip = host->ram[B];
            for (int i = 1; i < 4; i++) {
                skip <<= 8;
                skip |= host->ram[(B+i)&0xFFFF];
            }
        } else if (A == 2) {
            int result = _interrupt(0);
            if (host->registers[1] == 1) {
                //FIRE! ..... how? this things doesn't actually exist.....
            }
        } else if (A == 3)
            unit = host->registers[1];
        return 0;
    }
    int interrupt() {
        unsigned short A = host->registers[0];
        return _interrupt(A);
    }
    
    void registerKeyHandler(keyHandler* handler) { };
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

device* SPC2000Config::createDevice(cpu* host) {
    return new SPC2000(host, false);
}
