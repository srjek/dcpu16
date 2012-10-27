#include <iostream>
#include "M35FD.h"

#undef ERROR_BUSY   //I think wx has it defined? Or maybe in a windows header in wx?

class M35FD;

class M35FD_callback: public cpuCallback {
protected:
    M35FD* device;
    int cmd;
    unsigned short floppySector;
    unsigned short ramOffset;
public:
    M35FD_callback(M35FD* device, int cmd, unsigned short floppySector, unsigned short ramOffset):
                        device(device), cmd(cmd), floppySector(floppySector), ramOffset(ramOffset) { }
    void callback();
};

class M35FD_display: public wxFrame {
protected:
    M35FD* device;
public:
    M35FD_display(wxWindow* parent, const wxPoint& pos, M35FD* device): wxFrame(parent, -1, _("M35FD"), pos, wxSize(-1, -1)) {
        this->device = device;
    }
    void OnClose(wxCloseEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(M35FD_display, wxFrame)
    EVT_CLOSE(M35FD_display::OnClose)
END_EVENT_TABLE()


class M35FD: public device {
    friend class M35FD_display;
    friend class M35FD_callback;
protected:
    cpu* host;
    M35FD_display* display;
    volatile unsigned short state;
    volatile unsigned short error;
    volatile unsigned short interruptMsg;
    
    volatile unsigned short buffer[1440*512];
    
    static const int STATE_NO_MEDIA =   0x0000;
    static const int STATE_READY =      0x0001;
    static const int STATE_READY_WP =   0x0002;
    static const int STATE_BUSY =       0xFFFF;

    static const int ERROR_NONE =       0x0000;
    static const int ERROR_BUSY =       0x0001;
    static const int ERROR_NO_MEDIA =   0x0002;
    static const int ERROR_PROTECTED =  0x0003;
    static const int ERROR_EJECT =      0x0004;
    static const int ERROR_BAD_SECTOR = 0x0005;
    static const int ERROR_BROKEN =     0xFFFF;
    
    static const int CMD_NONE =     0;
    static const int CMD_READ =     1;
    static const int CMD_WRITE =    2;
    static const int CMD_EJECT =    3;
    
    wxMutex* stateMutex;
    
public:
    M35FD(cpu* host) {
        this->host = host;
        host->addHardware(this);
        
        state = STATE_NO_MEDIA;
        error = ERROR_NONE;
        interruptMsg = 0;
        
        stateMutex = new wxMutex();
    }
    ~M35FD() {
        if (display)
            display->Close(true);
        stateMutex->Lock();
        delete stateMutex;
    }
    void createWindow() {
        display = new M35FD_display(host->getWindow(), wxPoint(50, 50), this);
        display->Show(true);
    }
    unsigned long getManufacturer() {
        return 0x1eb37e91;  //Mackapar Media
    }
    unsigned long getId() {
        return 0x12345678;  //A fake temporary id
    }
    unsigned long getVersion() {
        return 0x000b;  //???
    }
    
protected:
    void read(unsigned long floppySector, unsigned short ramOffset) {
        stateMutex->Lock();
        if (state == STATE_NO_MEDIA) {
            stateMutex->Unlock();
            return;
        }
        for (unsigned long i = 0; i < 512; i++)
            host->ram[(ramOffset+i)&0xFFFF] = buffer[floppySector*512+i];
        //if self.imagePath == None:
            changeState(STATE_READY_WP);
        //else:
        //    changeState(STATE_READY);
        stateMutex->Unlock();
    }
    void write(unsigned long floppySector, unsigned short ramOffset) {
        stateMutex->Lock();
        if (state == STATE_NO_MEDIA) {
            stateMutex->Unlock();
            return;
        }
        for (unsigned long i = 0; i < 512; i++)
            host->ram[floppySector*512+i] = buffer[(ramOffset+i)&0xFFFF];
        //if self.imagePath == None:
            changeState(STATE_READY_WP);
        //else:
        //    changeState(STATE_READY)
        //    if self.imagePath != M35FD.RAMDISK:
        //        self._saveFile()
        stateMutex->Unlock();
    }
    
    void callback(int cmd, unsigned short floppySector, unsigned short ramOffset) {
        if (cmd == CMD_READ)
            read(floppySector, ramOffset);
        else if (cmd == CMD_WRITE)
            write(floppySector, ramOffset);
    }
    
    
    void changeState(unsigned short state) {
        this->state = state;
        if (interruptMsg != 0)
            host->interrupt(interruptMsg);
    }
    void changeError(unsigned short error) {
        this->error = error;
        if (interruptMsg != 0)
            host->interrupt(interruptMsg);
    }
    unsigned short handleCommand(int cmd, unsigned short floppySector, unsigned short ramOffset) {
        stateMutex->Lock();
        unsigned short result = 0;

        if (cmd == CMD_EJECT) {
            //self.image = None
            //self.guiUpdatePath()
            if ((state != STATE_READY) && (state != STATE_READY_WP))
                error = ERROR_EJECT;
            changeState(STATE_NO_MEDIA);
            result = 1;
        //} else if (image == None) {
        //    self.changeError(M35FD.ERROR_NO_MEDIA)
        } else if ((state != STATE_READY) and (state != STATE_READY_WP)) {
            changeError(ERROR_BUSY);
        } else if ((state == STATE_READY_WP) and (cmd == CMD_WRITE)) {
            changeError(ERROR_PROTECTED);
        } else if (floppySector >= 1440) {
            changeError(ERROR_BAD_SECTOR);
        } else {
            host->scheduleCallback(host->time+(100000/60), new M35FD_callback(this, cmd, floppySector, ramOffset));
            changeState(STATE_BUSY);
            result = 1;
        }

        stateMutex->Unlock();
        return result;
    }

public:
    int interrupt() {
        unsigned short A = host->registers[0];
        if (A == 0) {
            stateMutex->Lock();
            host->registers[1] = state;
            host->registers[2] = error;
            error = ERROR_NONE;
            stateMutex->Unlock();
        } else if (A == 1) {
            interruptMsg = host->registers[3];
        } else if (A == 2)
            host->registers[1] = handleCommand(CMD_READ, host->registers[3], host->registers[4]);
        else if (A == 3)
            host->registers[1] = handleCommand(CMD_WRITE, host->registers[3], host->registers[4]);
        return 0;
    }
    void registerKeyHandler(keyHandler* handler) { }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void M35FD_callback::callback() {
    device->callback(cmd, floppySector, ramOffset);
}

void M35FD_display::OnClose(wxCloseEvent& event) {
    device->display = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}

device* M35FDConfig::createDevice(cpu* host) {
    return new M35FD(host);
}
