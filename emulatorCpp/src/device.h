
#ifndef emulator_device_h
#define emulator_device_h

class device
{
    virtual bool hasWindow();
    virtual wxWindow createWindow();
};

#endif
