
#ifndef emulator_cpu_h
#define emulator_cpu_h

class cpu {
public:
    virtual wxWindow createCtrlWindow();
    virtual void loadImage(const wxChar* imagePath);
};

class cpuConfig {
protected:
    cpuConfig(): name("") { };
public:
    const char* name;
    virtual cpu* createCpu()=0;
};

#endif
