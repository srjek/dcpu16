#include <vector>
using namespace std;

#include "wx/wx.h"

#include "cpu.h"
#include "device.h"
#include "thread.h"

#include "cpus/dcpu16/dcpu16.h" //Default cpu
#include "cpus/cpus.h"   //helper methods to find a cpu
#include "devices/devices.h"   //helper methods to find a device

#ifndef emulator_system_h
#define emulator_system_h

class compSystem: public thread, public wxThread {
protected:
    cpu *sysCpu;
    device** devices;
    int numDevices;
public:
    compSystem(cpuConfig* cpuConfig, const wxChar* imagePath, vector<deviceConfig*> deviceConfigs) {
        sysCpu = cpuConfig->createCpu();
        if (wxStrlen(imagePath) != 0)
            sysCpu->loadImage(imagePath);
        
        numDevices = deviceConfigs.size();
        devices = new device*[numDevices];
        for (int i = 0; i < numDevices; i++)
            devices[i] = deviceConfigs[i]->createDevice(sysCpu);
        
        sysCpu->createCtrlWindow();
    }
    ~compSystem() {
        
        //for (int i = 0; i < numDevices; i++)
        //    delete devices[i];
        delete[] devices;
        delete sysCpu;
    }
    
    wxThreadError Create() {
        return wxThread::Create(1024*1024); //1 MB
    }
    wxThreadError Run() { return wxThread::Run(); }
    ExitCode Entry() {
        sysCpu->Run();
        return 0;
    }
    void Stop() {
    }
    wxThread::ExitCode Wait() { return wxThread::Wait(); }
};

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
    compSystem* createSystem() {
        return new compSystem(cpu, imagePath, devices);
    }
};

#endif
