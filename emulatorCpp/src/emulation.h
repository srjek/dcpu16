#include <iostream>
#include <vector>
using namespace std;

#include "wx/wx.h"

#include "device.h"
#include "system.h"


#ifndef emulator_sysConfig_h
#define emulator_sysConfig_h

class emulation {
    compSystem** systems;
    int nSystems;
    public:
        emulation(vector<sysConfig*> systemConfigs) {
            nSystems = systemConfigs.size();
            systems = new compSystem*[nSystems];
            for (int i = 0; i < nSystems; i++)
                systems[i] = systemConfigs[i]->createSystem();
        }
        ~emulation() {
            for (int i = 0; i < nSystems; i++)
                    delete systems[i];
            delete[] systems;
        }
};

class emulationConfig {
public:
    vector<sysConfig*> systemConfigs;
    
    emulationConfig(int argc, wxChar** argv) {
        if (argc == 0)
            systemConfigs.push_back(new sysConfig(argc, argv));
        while (argc > 0) {
            systemConfigs.push_back(new sysConfig(argc, argv));
        }
    }
    ~emulationConfig() {
        for (int i = 0; i < systemConfigs.size(); i++) {
            sysConfig* tmp = systemConfigs[i];
            delete tmp;
        }
        while (systemConfigs.size() > 0)
            systemConfigs.pop_back();
    }
    emulation* createEmulation() {
        return new emulation(systemConfigs);
    }
    
    void print() {
        for (int i = 0; i < systemConfigs.size(); i++) {
            sysConfig* system = systemConfigs[i];
            std::cout << "System " << i << ":" << std::endl;
            std::cout << "\tCPU: " << system->cpu->name << std::endl;
            for (int j = 0; j < system->devices.size(); j++) {
                deviceConfig* device = system->devices[j];
                std::cout << "\tDevice " << j << ": " << device->name << std::endl;
            }
        }
    }
};

#endif
