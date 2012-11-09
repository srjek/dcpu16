#include <iostream>
using namespace std;

#include "device.h"
#include "system.h"
#include "emulation.h"

emulation::emulation(vector<sysConfig*> systemConfigs) {
    nSystems = systemConfigs.size();
    systems = new compSystem*[nSystems];
    for (int i = 0; i < nSystems; i++)
        systems[i] = systemConfigs[i]->createSystem();
}
emulation::~emulation() {
    for (int i = 0; i < nSystems; i++)
            delete systems[i];
    delete[] systems;
}

void emulation::Run() {
    for (int i = 0; i < nSystems; i++) {
        systems[i]->Create();
        systems[i]->Run();
    }
}
void emulation::Stop() {
    for (int i = 0; i < nSystems; i++)
        systems[i]->Stop();
}
void emulation::Wait() {
    for (int i = 0; i < nSystems; i++)
        systems[i]->Wait();
}


emulationConfig::emulationConfig(int argc, wxChar** argv) {
    if (argc == 0)
        systemConfigs.push_back(new sysConfig(argc, argv));
    while (argc > 0) {
        systemConfigs.push_back(new sysConfig(argc, argv));
    }
}
emulationConfig::~emulationConfig() {
    for (int i = 0; i < systemConfigs.size(); i++) {
        sysConfig* tmp = systemConfigs[i];
        delete tmp;
    }
    while (systemConfigs.size() > 0)
        systemConfigs.pop_back();
}
emulation* emulationConfig::createEmulation() {
    return new emulation(systemConfigs);
}

void emulationConfig::print() {
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

