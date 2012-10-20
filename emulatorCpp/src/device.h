
#ifndef emulator_device_h
#define emulator_device_h

class device {
public:
    virtual bool hasWindow();
    virtual wxWindow createWindow();
};

#endif
