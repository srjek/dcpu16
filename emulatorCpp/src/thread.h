#include "wx/wx.h"

#ifndef emulator_thread_h
#define emulator_thread_h

class thread {
    virtual wxThreadError Create() =0;
    virtual wxThreadError Run() =0;
    virtual void Stop() =0;
    virtual wxThread::ExitCode Wait() =0;
};

#endif
