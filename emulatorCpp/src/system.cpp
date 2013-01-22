#include <iostream>
using namespace std;

#include "glew.h"
#include "system.h"
#include "strHelper.h"

#include "cpus/dcpu16/dcpu16.h" //Default cpu
#include "cpus/cpus.h"   //helper methods to find a cpu
#include "devices/devices.h"   //helper methods to find a device


compSystem::compSystem(cpuConfig* cpuConfig, const wxChar* imagePath, unsigned int gdb_port, vector<deviceConfig*> deviceConfigs): wxThread(wxTHREAD_JOINABLE) {
    sysCpu = cpuConfig->createCpu();
    if (wxStrlen(imagePath) != 0)
        sysCpu->loadImage(imagePath);
    
    numDevices = deviceConfigs.size();
    device* keyboardProvider = NULL;
    devices = new device*[numDevices];
    for (int i = 0; i < numDevices; i++) {
        if (deviceConfigs[i]->consumesKeyboard())
            devices[i] = deviceConfigs[i]->createDevice(sysCpu, keyboardProvider);
        else
            devices[i] = deviceConfigs[i]->createDevice(sysCpu);
        if (deviceConfigs[i]->providesKeyboard())
            keyboardProvider = devices[i];
    }
    
    sysCpu->createWindow();
    for (int i = 0; i < numDevices; i++)
        devices[i]->createWindow();
        
    if (gdb_port != 0) {
        debugger = new gdb_remote(sysCpu, gdb_port);
        debugger->Create();
    } else
        debugger = NULL;
}
compSystem::~compSystem() {
    
    //for (int i = 0; i < numDevices; i++)
    //    delete devices[i];
    delete[] devices;
    delete sysCpu;
}

wxThreadError compSystem::Create() {
    return wxThread::Create(10*1024*1024); //10 MB
}
wxThreadError compSystem::Run() { return wxThread::Run(); }
wxThread::ExitCode compSystem::Entry() {
    for (int i = 0; i < numDevices; i++)
        devices[i]->Run();
    if (debugger != NULL)
        debugger->Run();
    while (sysCpu->running) {
        sysCpu->Run();
        Sleep(100);
        TestDestroy();
    }
    return 0;
}
void compSystem::Stop() {
    for (int i = 0; i < numDevices; i++)
        devices[i]->Stop();
    if (debugger != NULL)
        debugger->Stop();
    sysCpu->Stop();
}
wxThread::ExitCode compSystem::Wait() {
    for (int i = 0; i < numDevices; i++)
        devices[i]->Wait();
    if (debugger != NULL)
        debugger->Wait();
    if (IsRunning())
        return wxThread::Wait();
    return 0;
}


sysConfig::sysConfig(int& argc, wxChar**& argv) {
    imagePath = wxS("");
    gdb_port = 0;
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
                cout << "Option 'image' requires a value." << endl;
                return;
            }
            imagePath = argv[0];
            argc--; argv++;
            if (canRedefineCpu) {
                canRedefineCpu = false;
                cpu = new dcpu16Config();
            }
        } else if (wxStrcmp(argument, _("--gdb")) == 0) {
            argc--; argv++;
            if (argc < 1) {
                cout << "Option 'gdb' requires a value." << endl;
                return;
            }
            gdb_port = wxFromDecimal(argv[0]);
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
sysConfig::~sysConfig() {
    delete cpu;
    for (int i = 0; i < devices.size(); i++) {
        deviceConfig* tmp = devices[i];
        delete tmp;
    }
    while (devices.size() > 0)
        devices.pop_back();
}
compSystem* sysConfig::createSystem() {
    return new compSystem(cpu, imagePath, gdb_port, devices);
}
