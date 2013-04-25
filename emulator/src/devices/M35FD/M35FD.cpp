#include <iostream>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include "M35FD.h"

#undef ERROR_BUSY   //I think wx has it defined? Or maybe in a windows header in wx?

class M35FD_state: public deviceState {
public:
    unsigned short state;
    unsigned short error;
    unsigned short interruptMsg;
    
    wxString* imagePath;
    bool writeProtected;
    unsigned short buffer[1440*512];
    
    M35FD_state() {
        name = "M35FD";
        imagePath = NULL;
    }
    ~M35FD_state() {
        if (imagePath != NULL)
            delete imagePath;
    }
};

class M35FD;

class M35FD_callback_state: public cpuCallbackState {
protected:
    M35FD* device;
    int cmd;
    unsigned short floppySector;
    unsigned short ramOffset;
public:
    M35FD_callback_state(M35FD* device, int cmd, unsigned short floppySector, unsigned short ramOffset):
                        device(device), cmd(cmd), floppySector(floppySector), ramOffset(ramOffset) { }
    void restoreCallback(unsigned long long time);
};

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
    cpuCallbackState* saveState() {
        return new M35FD_callback_state(device, cmd, floppySector, ramOffset);
    }
};

enum {
    ID_Eject = 1,
    ID_LoadRO,
    ID_LoadRam,
    ID_Load,
    ID_Save,
    ID_ButtonDoesNotExist,
};
class M35FD_display: public wxFrame {
protected:
    M35FD* device;
    wxStaticText* imagePath;
    
public:
    void _guiUpdatePath();
    
    M35FD_display(wxWindow* parent, const wxPoint& pos, M35FD* device): wxFrame(parent, -1, _("M35FD"), pos, wxSize(-1, -1)) {
        this->device = device;
        
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddSpacer(2);
        
        imagePath = new wxStaticText(this, -1, _(""));
        sizer->Add(imagePath, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->AddSpacer(4);
        
        sizer->Add(new wxButton(this, ID_Eject, _("Eject")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->Add(new wxButton(this, ID_LoadRO, _("Load as readonly")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->Add(new wxButton(this, ID_LoadRam, _("Load as isolated ramdisk")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->Add(new wxButton(this, ID_Load, _("Load")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->Add(new wxButton(this, ID_Save, _("Save")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        
        sizer->AddSpacer(4);
        sizer->SetSizeHints(this);
        SetSizer(sizer);
        _guiUpdatePath();
    }
    
    wxString getFileR() {
        wxString result(_(""));
        wxFileDialog* OpenDialog = new wxFileDialog(
            this, _("Select a floppy image to load"), wxEmptyString, wxEmptyString, 
            _("*.*"),
            wxFD_OPEN | wxFD_CHANGE_DIR, wxDefaultPosition);
     
        // Creates a "open file" dialog
        if (OpenDialog->ShowModal() == wxID_OK) { // if the user click "Open" instead of "Cancel"
            result = OpenDialog->GetPath();
        }
     
        // Clean up after ourselves
        OpenDialog->Destroy();
        
        return result;
        /*from tkinter import ttk, filedialog, messagebox
        tmp = os.getcwd()
        os.chdir(origRoot)
        imagePath = filedialog.askopenfilename(title="Select a floppy image to load", parent=self.window)
        os.chdir(tmp)
        if (imagePath == "") or (imagePath == ()):
            return None
        return imagePath */
    }
    wxString getFileW() {
        wxString result(_(""));
        wxFileDialog* OpenDialog = new wxFileDialog(
            this, _("Specify a file name to save the floppy image as"), wxEmptyString, wxEmptyString, 
            _("*.*"),
            wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);
     
        // Creates a "open file" dialog
        if (OpenDialog->ShowModal() == wxID_OK) { // if the user click "Open" instead of "Cancel"
            result = OpenDialog->GetPath();
        }
     
        // Clean up after ourselves
        OpenDialog->Destroy();
        
        return result;
    }
    
    void LoadFile(wxString filePath, bool readonly = false, bool ramdisk = true);
    void SaveFile(wxString filePath);
    
    void LoadReadonly() {
        LoadFile(getFileR(), true);
    }
    void LoadRamdisk() {
        LoadFile(getFileR(), false, true);
    }
    void Load() {
        LoadFile(getFileR(), false, false);
    }
    void Save() {
        SaveFile(getFileW());
    }
    
    void OnEject(wxCommandEvent& WXUNUSED(event));
    void OnLoadRO(wxCommandEvent& WXUNUSED(event)) { LoadReadonly(); }
    void OnLoadRam(wxCommandEvent& WXUNUSED(event)) { LoadRamdisk(); }
    void OnLoad(wxCommandEvent& WXUNUSED(event)) { Load(); }
    void OnSave(wxCommandEvent& WXUNUSED(event)) { Save(); }
    void OnClose(wxCloseEvent& event);
    void OnButtonDoesNotExist(wxCommandEvent& WXUNUSED(event)) {
        _guiUpdatePath();
    }
    
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(M35FD_display, wxFrame)
    EVT_BUTTON(ID_Eject, M35FD_display::OnEject)
    EVT_BUTTON(ID_LoadRO, M35FD_display::OnLoadRO)
    EVT_BUTTON(ID_LoadRam, M35FD_display::OnLoadRam)
    EVT_BUTTON(ID_Load, M35FD_display::OnLoad)
    EVT_BUTTON(ID_Save, M35FD_display::OnSave)
    EVT_CLOSE(M35FD_display::OnClose)
    EVT_BUTTON(ID_ButtonDoesNotExist, M35FD_display::OnButtonDoesNotExist)
END_EVENT_TABLE()


class M35FD: public device {
    friend class M35FD_display;
    friend class M35FD_callback;
    friend class M35FD_callback_state;
protected:
    cpu* host;
    M35FD_display* display;
    volatile unsigned short state;
    volatile unsigned short error;
    volatile unsigned short interruptMsg;
    
    volatile wxString* imagePath;
    volatile bool writeProtected;
    volatile unsigned short buffer[1440*512];
    
    static const int STATE_NO_MEDIA =   0x0000;
    static const int STATE_READY =      0x0001;
    static const int STATE_READY_WP =   0x0002;
    static const int STATE_BUSY =       0x0003;

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
    
    
    void guiUpdatePath() {
        if (display) {
            wxCommandEvent tmp = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_ButtonDoesNotExist);
            #if wxCHECK_VERSION(2, 9, 0)
            display->GetEventHandler()->AddPendingEvent(tmp);
            #else
            display->AddPendingEvent(tmp);
            #endif
        }
    }
public:
    void reset() {
        stateMutex->Lock();
        
        if (state != STATE_NO_MEDIA) {
            //callbacks would have been cancelled by the cpu, so we can/must reset
            if (writeProtected)
                changeState(STATE_READY_WP);
            else
                changeState(STATE_READY);
        }
        error = ERROR_NONE;
        interruptMsg = 0;
        
        stateMutex->Unlock();
        guiUpdatePath();
    }
    M35FD(cpu* host) {
        this->host = host;
        host->addHardware(this);
        
        state = STATE_NO_MEDIA;
        imagePath = 0;
        writeProtected = true;
        
        stateMutex = new wxMutex();
        reset();
    }
    ~M35FD() {
        if (display)
            display->Close(true);
        stateMutex->Lock();
        delete stateMutex;
        if (imagePath != 0)
            delete imagePath;
    }
    void createWindow() {
        display = new M35FD_display(host->getWindow(), wxPoint(50, 50), this);
        display->Show(true);
    }
    unsigned long getManufacturer() {
        return 0x1eb37e91;  //Mackapar Media
    }
    unsigned long getId() {
        return 0x4fd524c5;
    }
    unsigned long getVersion() {
        return 0x000b;  //???
    }
    
protected:
    void loadFile(wxString filepath, bool readonly = false, bool ramdisk = true) {
        stateMutex->Lock();
        if (state != STATE_NO_MEDIA) {
            stateMutex->Unlock();
            //raise Exception("Floppy already inserted")
        }
        
        wxFFileInputStream stream(filepath, wxT("rb"));
        int i;
        for (i = 0; i < 1440*512; i++) {
            unsigned short c1 = stream.GetC();
            if (stream.LastRead() == 0)
                break;
            unsigned short c2 = stream.GetC();
            if (stream.LastRead() == 0) {
                buffer[i] = (c1 << 8) || 0;
                i++;
                break;
            }
            buffer[i] = (c1 << 8) | c2;
        }
        for (; i < 1440*512; i++) {
            buffer[i] = 0;
        }
        writeProtected = readonly;
        if (imagePath != 0) {
            delete imagePath;
            imagePath = 0;
        }
        if (readonly) {
            changeState(STATE_READY_WP);
        } else {
            changeState(STATE_READY);
            if (!(readonly || ramdisk))
                imagePath = new wxString(filepath);
        }
        guiUpdatePath();
        stateMutex->Unlock();
    }
    void _saveFile(wxString filepath) {
        if (state == STATE_NO_MEDIA) {
            //raise Exception("No floppy inserted")
            return;
        }
        wxFFileOutputStream stream(filepath, wxT("wb"));
        for (int i = 0; i < 1440*512; i++) {
            stream.PutC((buffer[i] >> 8) & 0xFF);
            stream.PutC(buffer[i] & 0xFF);
        }
    }
    void _saveFile() {
        if (imagePath != 0)
            _saveFile(*((wxString*) imagePath));
    }
    void saveFile(wxString filepath) {
        stateMutex->Lock();
        _saveFile(filepath);
        stateMutex->Unlock();
    }
    
    void read(unsigned long floppySector, unsigned short ramOffset) {
        stateMutex->Lock();
        if (state == STATE_NO_MEDIA) {
            stateMutex->Unlock();
            return;
        }
        for (unsigned long i = 0; i < 512; i++)
            host->ram[(ramOffset+i)&0xFFFF] = buffer[floppySector*512+i];
        if (writeProtected)
            changeState(STATE_READY_WP);
        else
            changeState(STATE_READY);
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
        if (writeProtected)
            changeState(STATE_READY_WP);
        else {
            changeState(STATE_READY);
            if (imagePath != 0)
                _saveFile();
        }
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
            if (imagePath != 0) {
                delete imagePath;
                imagePath = 0;
            }
            if ((state != STATE_READY) && (state != STATE_READY_WP))
                error = ERROR_EJECT;
            changeState(STATE_NO_MEDIA);
            result = 1;
            guiUpdatePath();
        } else if (state == STATE_NO_MEDIA) {
            changeError(ERROR_NO_MEDIA);
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
    
    deviceState* saveState() {
        M35FD_state* result = new M35FD_state();
        
        result->state = state;
        result->error = error;
        result->interruptMsg = interruptMsg;
        
        result->writeProtected = writeProtected;
        result->imagePath = new wxString(*((wxString*) imagePath));
        for (int i = 0; i < 1440*512; i++)
            result->buffer[i] = buffer[i];
        
        return result;
    }
    void restoreState(deviceState* state_in) {
        if (strcmp(state_in->name, "M35FD") != 0) {
            std::cerr << "A M35FD was given a state for a " << state_in->name << ", unable to recover previous state. Overall system state may be inconsisent." << std::endl;
            return;
        }
        M35FD_state* true_state = (M35FD_state*) state_in;
        
        state = true_state->state;
        error = true_state->error;
        interruptMsg = true_state->interruptMsg;
        
        writeProtected = true_state->writeProtected;
        if (imagePath != 0)
            delete imagePath;
        imagePath = new wxString(*(true_state->imagePath));
        
        for (int i = 0; i < 1440*512; i++)
            buffer[i] = true_state->buffer[i];
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void M35FD_callback::callback() {
    device->callback(cmd, floppySector, ramOffset);
}
void M35FD_callback_state::restoreCallback(unsigned long long time) {
    device->host->scheduleCallback(time, new M35FD_callback(device, cmd, floppySector, ramOffset));
}

void M35FD_display::_guiUpdatePath() {
    if (device->state == M35FD::STATE_NO_MEDIA)
        imagePath->SetLabel(_("<None>"));
    else if (device->writeProtected)
        imagePath->SetLabel(_("<Readonly RAM Disk>"));
    else if (device->imagePath == 0)
        imagePath->SetLabel(_("<RAM Disk>"));
    else
        imagePath->SetLabel(*((wxString*) device->imagePath));
}
void M35FD_display::OnEject(wxCommandEvent& WXUNUSED(event)) {
    device->handleCommand(M35FD::CMD_EJECT, 0, 0);
}
void M35FD_display::LoadFile(wxString filePath, bool readonly, bool ramdisk) {
    if (filePath.length() == 0) return;
    if (device->state != M35FD::STATE_NO_MEDIA)
        device->handleCommand(M35FD::CMD_EJECT, 0, 0);
    
    wxFileName filename;
    filename.AssignCwd();
    filename.Assign(filePath);
    if (!filename.FileExists()) {
        if (readonly || ramdisk) {
            std::cout << "ERROR: File \"";
            std::cout << filePath.mb_str(wxConvUTF8);
            std::cout << "\" does not exist. Only writable floppies will automatically create a blank image." << std::endl;
            return;
        } else {
            std::cout << "M35FD: Creating blank file \"";
            std::cout << filePath.mb_str(wxConvUTF8);
            std::cout << "\"" << std::endl;
            device->stateMutex->Lock();
            for (int i = 0; i < 1440*512; i++)
                device->buffer[i] = 0;
            if (device->imagePath != 0)
                delete imagePath;
            device->imagePath = new wxString(filename.GetFullPath());
            device->writeProtected = false;
            //TODO: save file
            device->stateMutex->Unlock();
            return;
        }
    } else if (!filename.IsFileReadable()) {
        std::cout << "ERROR: File \"";
        std::cout << filePath.mb_str(wxConvUTF8);
        std::cout << "\" is not readable" << std::endl;
        return;
    }
    std::cout << "M35FD: Loading file \"";
    std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
    std::cout << "\"" << std::endl;
    device->loadFile(filename.GetFullPath(), readonly, ramdisk);
    //try:
    //    self.loadFile(filePath, readonly, ramdisk)
    //except Exception as e:
    //    self.showError(str(e.args[0]))
}
void M35FD_display::SaveFile(wxString filePath) {
    if (filePath.length() == 0) return;
    
    wxFileName filename;
    filename.AssignCwd();
    filename.Assign(filePath);
    std::cout << "M35FD: Saving file \"";
    std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
    std::cout << "\"" << std::endl;
    device->saveFile(filename.GetFullPath());
}
    
void M35FD_display::OnClose(wxCloseEvent& event) {
    device->display = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}

device* M35FDConfig::createDevice(cpu* host) {
    return new M35FD(host);
}
