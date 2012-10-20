
#ifndef emulator_cpu_h
#define emulator_cpu_h

class cpu {
public:
    virtual wxWindow createCtrlWindow();
    virtual void loadImage(const wxChar* imagePath);
};

#endif
