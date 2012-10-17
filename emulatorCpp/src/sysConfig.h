#include <iostream>
#include <vector>
#include "wx/wx.h"

#include "cpu.h"
#include "device.h"

using namespace std;

#ifndef emulator_sysConfig_h
#define emulator_sysConfig_h

class cpuConfig {
public:
    const char* name;
    virtual cpu* createCpu()=0;
protected:
    cpuConfig(): name("") { };
};
#include "cpus/dcpu16/dcpu16.h" //Default cpu
#include "cpus/cpus.h"   //helper methods to find a cpu

class deviceConfig {
protected:
    deviceConfig(): name("") { };
public:
    const char* name;
    virtual device* createDevice(cpu host)=0;
};
#include "devices/devices.h"   //helper methods to find a device

class sysConfig {
public:
    cpuConfig* cpu;
    const wxChar* imagePath;
    vector<deviceConfig*> devices;
    
    sysConfig(int& argc, wxChar**& argv) {
        imagePath = _("");
        if (argc == 0) {
            cpu = new dcpu16Config();
            return;
        }
        bool canRedefineCpu = true;
        
        while (argc > 0) {
            wxChar* argument = argv[0];
            if (wxStrcmp(argument, _("--image")) == 0) {
                argc--; argv++;
                if (argc < 1) {
                    cout << "Option 'image' requires a value.";
                    return;
                }
                imagePath = argv[0];
                argc--; argv++;
                if (canRedefineCpu) {
                    canRedefineCpu = false;
                    cpu = new dcpu16Config();
                }
            } else if (findDevice(argument)) {
                devices.push_back(createDeviceConfig(argc, argv));
                if (canRedefineCpu) {
                    canRedefineCpu = false;
                    cpu = new dcpu16Config();
                }
            } else if (findCpu(argument)) {
                if (canRedefineCpu) {
                    canRedefineCpu = false;
                    cpu = createCpuConfig(argc, argv);
                } else
                    return;
            } else {
                wxString tmp(argument);
                cout << "Failed to understand argument \"";
                cout << tmp.mb_str(wxConvUTF8);
                cout << "\"" << endl;
                argc--; argv++;
            }
        }
    }
    ~sysConfig() {
        delete cpu;
        for (int i = 0; i < devices.size(); i++) {
            deviceConfig* tmp = devices[i];
            delete tmp;
        }
        while (devices.size() > 0)
            devices.pop_back();
    }
};

class emulationConfig {
public:
    vector<sysConfig*> systems;
    
    emulationConfig(int argc, wxChar** argv) {
        if (argc == 0)
            systems.push_back(new sysConfig(argc, argv));
        while (argc > 0) {
            systems.push_back(new sysConfig(argc, argv));
        }
    }
    ~emulationConfig() {
        for (int i = 0; i < systems.size(); i++) {
            sysConfig* tmp = systems[i];
            delete tmp;
        }
        while (systems.size() > 0)
            systems.pop_back();
    }
};

#endif
