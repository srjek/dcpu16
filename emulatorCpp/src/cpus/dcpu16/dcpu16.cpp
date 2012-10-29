#include <iostream>
#include <cstdio>

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include "dcpu16.h"
#include "../../strHelper.h"
#include "../../orderedForwardMap.h"
#include "../../device.h"

class dcpu16;

enum {
    ID_Run = 1,
    ID_Step,
    ID_Stop,
    ID_ButtonDoesNotExist,
};

class dcpu16CtrlWindow: public wxFrame {
protected:
    dcpu16* cpu;
    
    wxStaticText* cycles;
    wxStaticText* PC;
    wxStaticText* SP;
    wxStaticText* IA;
    
    wxStaticText* A;
    wxStaticText* B;
    wxStaticText* C;
    
    wxStaticText* X;
    wxStaticText* Y;
    wxStaticText* Z;
    
    wxStaticText* I;
    wxStaticText* J;
    wxStaticText* EX;
    
    wxStaticText* curInstruction;
public:
    dcpu16CtrlWindow(const wxPoint& pos, dcpu16* cpu): wxFrame( getTopLevelWindow(), -1, _("dcpu16"), pos, wxSize(200,200) )  {
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddSpacer(2);
        cycles = new wxStaticText(this, -1, _("Cycles: 0"));
        sizer->Add(cycles, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->AddSpacer(4);
        
        wxFlexGridSizer* innerSizer = new wxFlexGridSizer(5, 3, 4, 0);
        innerSizer->Add(new wxButton(this, ID_Run, _("Run")), 0, 0, 0);
        innerSizer->Add(new wxButton(this, ID_Step, _("Step")), 0, 0, 0);
        innerSizer->Add(new wxButton(this, ID_Stop, _("Stop")), 0, 0, 0);
        
        PC = new wxStaticText(this, -1, _("PC: 0000"));
        SP = new wxStaticText(this, -1, _("SP: 0000"));
        IA = new wxStaticText(this, -1, _("IA: 0000"));
        innerSizer->Add(PC, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(SP, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(IA, 0, wxALIGN_LEFT, 0);
        
        A = new wxStaticText(this, -1, _("A: 0000"));
        B = new wxStaticText(this, -1, _("B: 0000"));
        C = new wxStaticText(this, -1, _("C: 0000"));
        innerSizer->Add(A, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(B, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(C, 0, wxALIGN_LEFT, 0);
        
        X = new wxStaticText(this, -1, _("X: 0000"));
        Y = new wxStaticText(this, -1, _("Y: 0000"));
        Z = new wxStaticText(this, -1, _("Z: 0000"));
        innerSizer->Add(X, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(Y, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(Z, 0, wxALIGN_LEFT, 0);
        
        I = new wxStaticText(this, -1, _("I: 0000"));
        J = new wxStaticText(this, -1, _("J: 0000"));
        EX = new wxStaticText(this, -1, _("EX: 0000"));
        innerSizer->Add(I, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(J, 0, wxALIGN_LEFT, 0);
        innerSizer->Add(EX, 0, wxALIGN_LEFT, 0);
        
        sizer->Add(innerSizer, 0, 0, 0);
        sizer->AddSpacer(8);
        
        curInstruction = new wxStaticText(this, -1, _("NOP"));
        sizer->Add(curInstruction, 0, wxALIGN_LEFT, 0);
        
        sizer->AddSpacer(4);
        sizer->SetSizeHints(this);
        SetSizer(sizer);
        
        this->cpu = cpu;
        update();
    }
    void OnRun(wxCommandEvent& WXUNUSED(event));
    void OnStep(wxCommandEvent& WXUNUSED(event));
    void OnStop(wxCommandEvent& WXUNUSED(event));
    void OnClose(wxCloseEvent& event);
    
    void update();
    void OnButtonDoesNotExist(wxCommandEvent& WXUNUSED(event)) {
        update();
    }
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(dcpu16CtrlWindow, wxFrame)
    EVT_BUTTON(ID_Run, dcpu16CtrlWindow::OnRun)
    EVT_BUTTON(ID_Step, dcpu16CtrlWindow::OnStep)
    EVT_BUTTON(ID_Stop, dcpu16CtrlWindow::OnStop)
    EVT_CLOSE(dcpu16CtrlWindow::OnClose)
    EVT_BUTTON(ID_ButtonDoesNotExist, dcpu16CtrlWindow::OnButtonDoesNotExist)
END_EVENT_TABLE()


class dcpu16_rom;
class dcpu16_rom_callback: public cpuCallback {
protected:
    dcpu16_rom* device;
public:
    dcpu16_rom_callback(dcpu16_rom* device):
                        device(device) { }
    void callback();
};

class dcpu16_rom: public device {
protected:
    dcpu16* host;
    volatile bool enabled;
    unsigned short rom[512];
public:
    dcpu16_rom(dcpu16* host);
    ~dcpu16_rom() { }
    void createWindow() { }
    unsigned long getManufacturer() {
        return 0;  //????
    }
    unsigned long getId() {
        return 0;  //????
    }
    unsigned long getVersion() {
        return 0;  //????
    }
    
    void enable() {
        enabled = true;
    }
    void disable() {
        enabled = false;
    }
    void startup();
    int interrupt();
    
    void registerKeyHandler(keyHandler* handler) { };
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

void dcpu16_rom_callback::callback() {
    device->startup();
}


class dcpu16: public cpu {
    friend class dcpu16_rom;
    friend class dcpu16CtrlWindow;
protected:
    static const int extOpcodeCycles[];
    static const int opcodeCycles[];
    class val {
    public:
        volatile unsigned short* pointer;
        unsigned short value;
        val(volatile unsigned short* pointer) {
            this->pointer = pointer;
            this->value = *pointer;
        }
        val(volatile unsigned short* pointer, unsigned short value) {
            this->pointer = pointer;
            this->value = value;
        }
    };
    
    dcpu16CtrlWindow* ctrlWindow;
    volatile int cmdState;
    int lastCmdState;
    
    //volatile unsigned short ram[0x10000];
    //volatile unsigned short registers[13];
    int cycles; //This is our debt/credit counter
    volatile unsigned long long totalCycles; //This is how many cycles total we have done

    device* hardware[0x10000];
    unsigned long hwLength;
    dcpu16_rom* romDevice;
    
    volatile unsigned short intQueue[256];
    volatile int intQueueStart;
    volatile int intQueueEnd;
    
    wxMutex* callbackMutex;
    ordered_forward_map<unsigned long long, cpuCallback*> callbackSchedule;
    unsigned long long nextCallback;
    
    volatile bool onFire;
    bool debug;
    
public:
    dcpu16(bool debug): cpu(totalCycles) {
        ram = new unsigned short[0x10000];
        registers = new unsigned short[13];
        for (int i = 0; i < 0x10000; i++)
            ram[i] = 0;
        for (int i = 0; i < 13; i++)
            registers[i] = 0;
            
        totalCycles = 0;
        hwLength = 0;
        intQueueStart = 0;
        intQueueStart = 0;
        onFire = false;
        
        nextCallback = 0;
        callbackMutex = new wxMutex();
        
        running = true;
        cmdState = 1;
        this->debug = debug;
        
        romDevice = new dcpu16_rom(this);
    }
    ~dcpu16() {
        if (ctrlWindow)
            ctrlWindow->Close(true);
        delete romDevice;
        callbackMutex->Lock();
        delete callbackMutex;
        delete[] ram;
        delete[] registers;
    }
    void createWindow() {
        ctrlWindow = new dcpu16CtrlWindow(wxPoint(50, 50), this);
        ctrlWindow->Show(true);
        cmdState = 0;
    }
    wxWindow* getWindow() {
        if (ctrlWindow)
            return ctrlWindow;
        return getTopLevelWindow();
    }
    
protected:
    unsigned short nextWord() {
        unsigned short result = ram[registers[DCPU16_REG_PC]];
        registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
        cycles += 1;
        return result;
    }
    val read_val(int a, bool isB) {
        unsigned short pointer;
        if (a <= 0x07) {            //register
            return val(registers + a);
        } else if (a <= 0x0F) {     //[register]
            pointer = registers[a & 0x07];
            return val(ram + pointer);
        } else if (a <= 0x17) {     //[next word + register]
            pointer = (nextWord() + registers[a & 0x07]) & 0xFFFF;
            return val(ram + pointer);
        } else if (a == 0x18) {     //PUSH/POP (b/a)
            if (isB) {
                pointer = (registers[DCPU16_REG_SP] - 1) & 0xFFFF;
                registers[DCPU16_REG_SP] = pointer;
            } else {
                pointer = registers[DCPU16_REG_SP];
                registers[DCPU16_REG_SP] = (pointer + 1) & 0xFFFF;
            }
            return val(ram + pointer);
        } else if (a == 0x19) {     //PEEK   ([SP])
            pointer = registers[DCPU16_REG_SP];
            return val(ram + pointer);
        } else if (a == 0x1A) {     //PICK   ([SP + next word])
            pointer = (registers[DCPU16_REG_SP] + nextWord()) & 0xFFFF;
            return val(ram + pointer);
        } else if (a == 0x1B) {     //SP
            return val(registers + DCPU16_REG_SP);
        } else if (a == 0x1C) {     //PC
            return val(registers + DCPU16_REG_PC);
        } else if (a == 0x1D) {     //EX
            return val(registers + DCPU16_REG_EX);
        } else if (a == 0x1E) {     //[next word]
            pointer = nextWord();
            return val(ram + pointer);
        } else if (a == 0x1F) {     //next word (literal)
            return val(NULL, nextWord());
        } else if (a <= 0x3F) {     //0xffff-0x1e (literal)
            pointer = a & 0x1F;
            pointer -= 1;
            return val(NULL, pointer);
        }
        return val(NULL, 0);
    }
    void write_val(val a, unsigned short value) {
        if (a.pointer != NULL)
            *(a.pointer) = value;
    }
    
    long sign16(unsigned long a) {
        a &= 0x0000FFFF;
        if (a & 0x8000)
            return a - 0x10000;
        return a;
    }
    unsigned short overflow(unsigned long value) {
        registers[DCPU16_REG_EX] = (value >> 16) & 0xFFFF;
        return value & 0xFFFF;
    }
    unsigned short overflowDIV(unsigned long b, unsigned long a) {
        if (a == 0) {
            registers[DCPU16_REG_EX] = 0;
            return 0;
        }    
        registers[DCPU16_REG_EX] = ((b << 16) / a) & 0xFFFF;
        return (b / a) & 0xFFFF;
    }
    unsigned short overflowDVI(long b, long a) {
        if (a == 0) {
            registers[DCPU16_REG_EX] = 0;
            return 0;
        }
        registers[DCPU16_REG_EX] = ((b << 16) / a) & 0xFFFF;
        return (b / a) & 0xFFFF;
    }
    unsigned short underflow(unsigned long value) {
        registers[DCPU16_REG_EX] = (value >> 16) & 0xFFFF;
        return value & 0xFFFF;
    }

    int skip() {
        unsigned short instruction = nextWord();
        int op = instruction & 0x1F;
        int b_code = (instruction >> 5) & 0x1F;
        int a_code = (instruction >> 10) & 0x3F;
        if (((0x10 <= a_code) && (a_code <= 0x17)) || (a_code == 0x1A) || (a_code == 0x1E) || (a_code == 0x1F))
            registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
        if (((0x10 <= b_code) && (b_code <= 0x17)) || (b_code == 0x1A) || (b_code == 0x1E) || (b_code == 0x1F))
            registers[DCPU16_REG_PC] = (registers[DCPU16_REG_PC] + 1) & 0xFFFF;
        return (0x10 <= op) && (op <= 0x17);
    }
    void conditional(int condition) {
        if (!condition)
            while (skip());
    }
    
    //MORE TODO IN HERE
    void execOp(int op, val b, val a) {
        //This function doesn't handle op==0, caller detects that state and uses an alt function
        switch (op) {
            case 0x01:      //SET
                write_val(b, a.value);
                break;
            case 0x02:      //ADD
                write_val(b, overflow(((unsigned long) b.value) + a.value));
                break;
            case 0x03:      //SUB
                write_val(b, underflow(((unsigned long) b.value) - a.value));
                break;
            case 0x04:      //MUL
                write_val(b, overflow(((unsigned long) b.value) * a.value));
                break;
            case 0x05:      //MLI
                write_val(b, overflow(sign16(b.value) * sign16(a.value)));
                break;
            case 0x06:      //DIV
                write_val(b, overflowDIV(b.value, a.value));
                break;
            case 0x07:      //DVI
                write_val(b, overflowDVI(sign16(b.value), sign16(a.value)));
                break;
            case 0x08:      //MOD       TESTING NEEDED
                if (a.value == 0)
                    write_val(b, 0);
                else
                    write_val(b, b.value % a.value);
                break;
            case 0x09:      //MDI       IMPLEMENTATION NEEDED
                if (a.value == 0)
                    write_val(b, 0);
                else
                    write_val(b, sign16(b.value) % sign16(a.value));
                break;
            case 0x0A:      //AND
                write_val(b, b.value & a.value);
                break;
            case 0x0B:      //BOR
                write_val(b, b.value | a.value);
                break;
            case 0x0C:      //XOR
                write_val(b, b.value ^ a.value);
                break;
            case 0x0D:      //SHR
                if (a.value == 0)
                    registers[DCPU16_REG_EX] = 0;
                else
                    registers[DCPU16_REG_EX] = ((((unsigned long) b.value) << 16) >> a.value) & 0xFFFF;  //Avoiding the effects of arithmitic shifts...
                write_val(b, b.value >> a.value);
                break;
            case 0x0E:      //ASR
                registers[DCPU16_REG_EX] = ((sign16(b.value) << 16) >> a.value) & 0xFFFF;
                write_val(b, sign16(b.value) >> a.value);
                break;
            case 0x0F:      //SHL
                registers[DCPU16_REG_EX] = ((((unsigned long) b.value) << a.value) >> 16) & 0xFFFF;
                write_val(b, b.value << a.value);
                break;
            case 0x10:      //IFB
                conditional((b.value & a.value) != 0);
                break;
            case 0x11:      //IFC
                conditional((b.value & a.value) == 0);
                break;
            case 0x12:      //IFE
                conditional(b.value == a.value);
                break;
            case 0x13:      //IFN
                conditional(b.value != a.value);
                break;
            case 0x14:      //IFG
                conditional(b.value > a.value);
                break;
            case 0x15:      //IFA
                conditional(sign16(b.value) > sign16(a.value));
                break;
            case 0x16:      //IFL
                conditional(b.value < a.value);
                break;
            case 0x17:      //IFU
                conditional(sign16(b.value) < sign16(a.value));
                break;
            case 0x18:
            case 0x19:
                break;
            case 0x1A:      //ADX
                write_val(b, overflow((((unsigned long) b.value) + a.value) + registers[DCPU16_REG_EX]));
                break;
            case 0x1B:      //SBX
                if (registers[DCPU16_REG_EX] != 0)
                    write_val(b, underflow((((unsigned long) b.value) - a.value) + ((unsigned long) registers[DCPU16_REG_EX]) - 0x10000));
                else
                    write_val(b, underflow(((unsigned long) b.value) - a.value));
                break;
            case 0x1C:
            case 0x1D:
                break;
            case 0x1E:      //STI
                write_val(b, a.value);
                registers[DCPU16_REG_I] += 1;
                registers[DCPU16_REG_J] += 1;
                break;
            case 0x1F:      //STD
                write_val(b, a.value);
                registers[DCPU16_REG_I] -= 1;
                registers[DCPU16_REG_J] -= 1;
                break;
        }
    }
    void execExtOp(int op, val a) {
        unsigned int num;
        unsigned long tmp;
        switch (op) {
            case 0x01:      //JSR <a>
                //SET PUSH, PC
                execOp(1, read_val(0x18, true), val(registers + DCPU16_REG_PC));
                //SET PC, <a>
                registers[DCPU16_REG_PC] = a.value;
                break;
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
                break;
            case 0x07:      //HCF
                onFire = true;
                break;
            case 0x08:      //INT
                interrupt(a.value);
                break;
            case 0x09:      //IAG <a>
                write_val(a, registers[DCPU16_REG_IA]);
                break;
            case 0x0A:      //IAS <a>
                registers[DCPU16_REG_IA] = a.value;
                break;
            case 0x0B:      //RFI
                //IAQ 0 -- should disable queuing
                registers[DCPU16_REG_IAQ] = 0;
                //SET A, POP
                execOp(1, val(registers + DCPU16_REG_A), read_val(0x18, false));
                //SET PC, POP
                execOp(1, val(registers + DCPU16_REG_PC), read_val(0x18, false));
                break;
            case 0x0C:      //IAQ
                registers[DCPU16_REG_IAQ] = a.value;
                break;
            case 0x0D:
            case 0x0E:
            case 0x0F:
                break;
            case 0x10:      //HWN
                write_val(a, hwLength);
                break;
            case 0x11:      //HWQ
                num = a.value;
                if (num < hwLength) {
                    tmp = hardware[num]->getId();
                    registers[DCPU16_REG_A] = (unsigned short) (tmp & 0xFFFF);
                    registers[DCPU16_REG_B] = (unsigned short) ((tmp >> 16) & 0xFFFF);
                    tmp = hardware[num]->getVersion();
                    registers[DCPU16_REG_C] = (unsigned short) (tmp & 0xFFFF);
                    tmp = hardware[num]->getManufacturer();
                    registers[DCPU16_REG_X] = (unsigned short) (tmp & 0xFFFF);
                    registers[DCPU16_REG_Y] = (unsigned short) ((tmp >> 16) & 0xFFFF);
                }
                break;
            case 0x12:      //HWI
                num = a.value;
                if (num < hwLength)
                    cycles += hardware[num]->interrupt();
                break;
        }
    }
 
    void handleCallbacks() {
        while ( (nextCallback != 0) && (nextCallback <= (totalCycles+1)) ) {
            callbackMutex->Lock();
            cpuCallback* callback = callbackSchedule.front().value;
            //Because we release the lock, we got to pop before hand
            // (otherwise we would reacquire the lock and there will be no clean way to remove the callback)
            callbackSchedule.pop_front();
            if (callbackSchedule.empty())
                nextCallback = 0;
            else
                nextCallback = callbackSchedule.front().key;
            //Release before call, because call might attempt to schedule another another callback
            callbackMutex->Unlock();
            
            if (debug)
                std::cout << "dcpu16: Calling callback from " << time << ". Next callback queued is at " << nextCallback << std::endl;
            callback->callback();
            delete callback;
        }
    }
public:   
    void cycle(int count) {
        cycles -= count;
        while (cycles < 0) {
            int initialCycles = cycles;
            handleCallbacks();
            
            unsigned short instruction = nextWord();
            int op = instruction & 0x1F;
            int b_code = (instruction >> 5) & 0x1F;
            int a_code = (instruction >> 10) & 0x3F;
            val a = read_val(a_code, false);
            if (op != 0) {
                val b = read_val(b_code, true);
                cycles += opcodeCycles[op];
                execOp(op, b, a);
            } else {
                cycles += extOpcodeCycles[b_code];
                execExtOp(b_code, a);
            }
            
            if ((intQueueStart != intQueueEnd) && (registers[DCPU16_REG_IAQ] == 0)) {
                if (registers[DCPU16_REG_IA] != 0) {
                    //IAQ 1 -- should enable queuing
                    registers[DCPU16_REG_IAQ] = 1;
                    //SET PUSH, PC
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_PC));
                    //SET PUSH, A
                    execOp(1, read_val(0x18, 1), val(registers + DCPU16_REG_A));
                    //SET PC, IA
                    registers[DCPU16_REG_PC] = registers[DCPU16_REG_IA];
                    //SET A, <msg>
                    registers[DCPU16_REG_A] = intQueue[intQueueStart];
                }
                if (intQueueStart >= 256)
                    intQueueStart = 0;
                else
                    intQueueStart++;
            }
            
            totalCycles += (cycles - initialCycles);
        }
    }
    
    
    unsigned int addHardware(device* hw) {
        hardware[hwLength] = hw;
        return hwLength++;
    }
    
    void interrupt(unsigned short msg) {
        if (debug)
            std::cout << "dcpu16: Interrupted with msg " << msg << std::endl;
        intQueue[intQueueEnd] = msg;
        int tmp = intQueueStart;
        if (intQueueEnd+1 >= 256)
            intQueueEnd = 0;
        else
            intQueueEnd++;
        
        if (intQueueEnd == tmp)
            onFire = true; //raise Exception("DCPU16 is on fire. Behavior undefined")
    }
    
    void scheduleCallback(unsigned long long time, cpuCallback* callback) {
        callbackMutex->Lock();
        if (time == 0)
            time++;
        callbackSchedule.insert(time, callback);
        if ((nextCallback == 0) || (time < nextCallback))
            nextCallback = time;
        if (debug)
            std::cout << "dcpu16: Callback scheduled for " << time << ". Next callback queued is at " << nextCallback << std::endl;
        callbackMutex->Unlock();
    }
    
    void loadImage(const wxChar* imagePath) {
        /*for (int i = 0; i < 13; i++)
            registers[i] = 0xFFFF - i;
        //ram[registers[DCPU16_REG_PC]] = 0x01 | (0x00 << 5) | (0x1F << 10);
        //ram[(registers[DCPU16_REG_PC]+1)&0xFFFF] = 0x8000;
        //ram[(registers[DCPU16_REG_PC]+2)&0xFFFF] = 0x8000;
        ram[registers[DCPU16_REG_PC]] = 0x02 | (0x00 << 5) | (0x22 << 10); */
        
        wxString wxImagePath(imagePath);
        wxFileName filename;
        filename.AssignCwd();
        filename.Assign(wxImagePath);
        if (!filename.FileExists()) {
            std::cout << "ERROR: File \"";
            std::cout << wxImagePath.mb_str(wxConvUTF8);
            std::cout << "\" does not exist" << std::endl;
            return;
        }
        if (!filename.IsFileReadable()) {
            std::cout << "ERROR: File \"";
            std::cout << wxImagePath.mb_str(wxConvUTF8);
            std::cout << "\" is not readable" << std::endl;
            return;
        }
        
        wxFFileInputStream stream(filename.GetFullPath(), wxT("rb"));
        for (int i = 0; i < 0x10000; i++) {
            unsigned short c1 = stream.GetC();
            if (stream.LastRead() == 0)
                break;
            unsigned short c2 = stream.GetC();
            if (stream.LastRead() == 0) {
                ram[i] = (c1 << 8) || 0;
                break;
            }
            ram[i] = (c1 << 8) | c2;
        }
        romDevice->disable();
        
        //ram[0] = 0x01 | (0x1C << 5) | (0x1F << 10); //0x7F81
        //ram[1] = 0x031d;
    }
    void Run() {
        if (running) {
            if (((cmdState == 2) || (cmdState == 3)) && cmdState != lastCmdState)
                cycle(1);
            else if (cmdState == 1)
                cycle(10000);
            lastCmdState = cmdState;
            if (ctrlWindow) {
                wxCommandEvent tmp = wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_ButtonDoesNotExist);
                ctrlWindow->AddPendingEvent(tmp);
            }
        }
    }
    void Stop() {
        running = false;
    }
    
    int disassembleCurInstruction(wxChar* buffer, int bufferSize) {
        if (bufferSize < 26)
            return 0;
        wxChar empty[] = wxT("");
        wxChar opcodes[][4] = {
                wxT("")   , wxT("SET"), wxT("ADD"), wxT("SUB"), wxT("MUL"), wxT("MLI"), wxT("DIV"), wxT("DVI"),
                wxT("MOD"), wxT("MDI"), wxT("AND"), wxT("BOR"), wxT("XOR"), wxT("SHR"), wxT("ASR"), wxT("SHL"),
                wxT("IFB"), wxT("IFC"), wxT("IFE"), wxT("IFN"), wxT("IFG"), wxT("IFA"), wxT("IFL"), wxT("IFU"),
                wxT("")   , wxT("")   , wxT("ADX"), wxT("SBX"), wxT("")   , wxT("")   , wxT("STI"), wxT("STD")};
        wxChar ext_opcodes[][4] = {
                wxT("")   , wxT("JSR"), wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("HCF"),
                wxT("INT"), wxT("IAG"), wxT("IAS"), wxT("RFI"), wxT("IAQ"), wxT("")   , wxT("")   , wxT("")   ,
                wxT("HWN"), wxT("HWQ"), wxT("HWI"), wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   ,
                wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   };
        wxChar values[][10] = {
                wxT("A")    , wxT("B")    , wxT("C")    , wxT("X")    , wxT("Y")    , wxT("Z")    , wxT("I")    , wxT("J")    ,
                wxT("[A]")  , wxT("[B]")  , wxT("[C]")  , wxT("[X]")  , wxT("[Y]")  , wxT("[Z]")  , wxT("[I]")  , wxT("[J]")  ,
                wxT("[A+@]"), wxT("[B+@]"), wxT("[C+@]"), wxT("[X+@]"), wxT("[Y+@]"), wxT("[Z+@]"), wxT("[I+@]"), wxT("[J+@]"),
                wxT("POP")  , wxT("PEEK") , wxT("[SP+@]"), wxT("SP")  , wxT("PC")   , wxT("EX")   , wxT("[@]")  , wxT("@")    ,
                wxT("0xFFFF"), wxT("0x0000"), wxT("0x0001"), wxT("0x0002"), wxT("0x0003"), wxT("0x0004"), wxT("0x0005"), wxT("0x0006"),
                wxT("0x0007"), wxT("0x0008"), wxT("0x0009"), wxT("0x000A"), wxT("0x000B"), wxT("0x000C"), wxT("0x000D"), wxT("0x000E"),
                wxT("0x000F"), wxT("0x0010"), wxT("0x0011"), wxT("0x0012"), wxT("0x0013"), wxT("0x0014"), wxT("0x0015"), wxT("0x0016"),
                wxT("0x0017"), wxT("0x0018"), wxT("0x0019"), wxT("0x001A"), wxT("0x001B"), wxT("0x001C"), wxT("0x001D"), wxT("0x001E")};
        unsigned short instruction = ram[registers[DCPU16_REG_PC]];
        int op = instruction & 0x1F;
        int b_code = (instruction >> 5) & 0x1F;
        int a_code = (instruction >> 10) & 0x3F;
        
        wxChar* opStr = opcodes[op];
        if (op == 0)
            opStr = ext_opcodes[b_code];
        
        int length = 0;
        if (opStr[0] == 0) {
            length += wxStrcpy(buffer, wxT("DAT 0x"), bufferSize);
            printHex(buffer+length, 4, instruction);
            length += 4;
        } else {
            length += wxStrcpy(buffer, opStr, bufferSize);
            buffer[length++] = wxT(' ');
            unsigned short PC = (registers[DCPU16_REG_PC]+1)&0xFFFF;
            
            if (((a_code >= 0x10) && (a_code < 0x18)) || (a_code == 0x1A) || (a_code == 0x1E) || (a_code == 0x1F)) {
                PC = (PC+1)&0xFFFF;
            }
            if (op != 0) {
                length += wxStrcpy(buffer+length, values[b_code], bufferSize-length);
                if (buffer[length-1] == wxT('@')) {
                    buffer[length-1] = wxT('0');
                    buffer[length++] = wxT('x');
                    printHex(buffer+length, 4, ram[PC]);
                    PC = (PC+1)&0xFFFF;
                    length += 4;
                } else if (buffer[length-2] == wxT('@')) {
                    buffer[length-2] = wxT('0');
                    buffer[length-1] = wxT('x');
                    printHex(buffer+length, 4, ram[PC]);
                    length += 4;
                    PC = (PC+1)&0xFFFF;
                    buffer[length++] = wxT(']');
                }
                buffer[length++] = wxT(',');
                buffer[length++] = wxT(' ');
            }
            length+= wxStrcpy(buffer+length, values[a_code], bufferSize-length);
            if (buffer[length-1] == wxT('@')) {
                buffer[length-1] = wxT('0');
                buffer[length++] = wxT('x');
                printHex(buffer+length, 4, ram[(registers[DCPU16_REG_PC]+1)&0xFFFF]);
                length += 4;
            } else if (buffer[length-2] == wxT('@')) {
                buffer[length-2] = wxT('0');
                buffer[length-1] = wxT('x');
                printHex(buffer+length, 4, ram[(registers[DCPU16_REG_PC]+1)&0xFFFF]);
                length += 4;
                buffer[length++] = wxT(']');
            }
        }
        return length;
    }
};

const int dcpu16::extOpcodeCycles[] = {
                0, 2, 0, 0, 0, 0, 0, 0,
                3, 0, 0, 2, 1, 0, 0, 0,
                1, 3, 3, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0 };
const int dcpu16::opcodeCycles[] = {
                0, 0, 1, 1, 1, 1, 2, 2,
                2, 2, 0, 0, 0, 0, 0, 0,
                1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 2, 2, 0, 0, 1, 1 };


dcpu16_rom::dcpu16_rom(dcpu16* host) {
    this->host = host;
    
    enabled = true;
    for (int i = 0; i < 512; i++)
        rom[i] = 0;
    
    host->addHardware(this);
    //Callbacks are processed before any cycles are run!
    host->scheduleCallback(0, new dcpu16_rom_callback(this));
    
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    
    wxFileName filename;
    filename.Assign(stdpath.GetExecutablePath());
    filename.Assign(_("firmware.bin"));
    if (!filename.FileExists()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" does not exist" << std::endl;
        return;
    }
    if (!filename.IsFileReadable()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" is not readable" << std::endl;
        return;
    }
    
    wxFFileInputStream romFile(filename.GetFullPath(), wxT("rb"));
    if (romFile.GetLength() <= 0) {
        std::cout << "ERROR: Failed to open \"firmware.bin\"";
    } else {
        int i;
        for (i = 0; i < 512; i++) {
            unsigned short word;
            int c1 = romFile.GetC();
            if (romFile.LastRead() == 0)
                break;
            int c2 = romFile.GetC();
            if (romFile.LastRead() == 0) {
                word = c1 << 8;
                rom[i++] = word;
                break;
            }
            word = (c1 << 8) | c2;
            rom[i] = word;
        }
        for (; i < 512; i++)
            rom[i] = 0;
    }
    
}
void dcpu16_rom::startup() {
    if (enabled)
        host->cycles += interrupt(); //We have to manage the interrupt ourselves...
}
int dcpu16_rom::interrupt() {
    unsigned short A = host->registers[0];  //Specs don't say anything about using this, so ignore
    
    int offset = host->registers[1]; //Start copying into memory at position B
    for (int i = 0; i < 512; i++) {
        host->ram[(offset + i)&0xFFFF] = rom[i];
    }
    return 512; //Specs don't say anything about this either, so I gave it 512 cycles, as the rom has 512 words
}
    
void dcpu16CtrlWindow::OnRun(wxCommandEvent& WXUNUSED(event)) {
    cpu->cmdState = 1;
}
void dcpu16CtrlWindow::OnStep(wxCommandEvent& WXUNUSED(event)) {
    if (cpu->cmdState == 3)
        cpu->cmdState = 2;
    else
        cpu->cmdState = 3;
}
void dcpu16CtrlWindow::OnStop(wxCommandEvent& WXUNUSED(event)) {
    cpu->cmdState = 0;
}
void dcpu16CtrlWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
    cpu->ctrlWindow = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}
void dcpu16CtrlWindow::update() {
    if (!cpu)
        return;
        
    wxChar CYCLES_TXT[110] = wxT("Cycles: ");
    int nCharsPrinted = printDecimal(CYCLES_TXT+8, 100, cpu->totalCycles);
    CYCLES_TXT[8+nCharsPrinted] = wxT('\0');
    cycles->SetLabel(CYCLES_TXT);
    
    wxChar PC_TXT[] = wxT(" PC: 0000");
    wxChar SP_TXT[] = wxT("SP: 0000");
    wxChar IA_TXT[] = wxT("IA: 0000");
    printHex(PC_TXT+5, 4, cpu->registers[DCPU16_REG_PC]);
    printHex(SP_TXT+4, 4, cpu->registers[DCPU16_REG_SP]);
    printHex(IA_TXT+4, 4, cpu->registers[DCPU16_REG_IA]);
    PC->SetLabel(PC_TXT);
    SP->SetLabel(SP_TXT);
    IA->SetLabel(IA_TXT);
    
    wxChar A_TXT[] = wxT(" A: 0000");
    wxChar B_TXT[] = wxT("B: 0000");
    wxChar C_TXT[] = wxT("C: 0000");
    printHex(A_TXT+4, 4, cpu->registers[DCPU16_REG_A]);
    printHex(B_TXT+3, 4, cpu->registers[DCPU16_REG_B]);
    printHex(C_TXT+3, 4, cpu->registers[DCPU16_REG_C]);
    A->SetLabel(A_TXT);
    B->SetLabel(B_TXT);
    C->SetLabel(C_TXT);
    
    wxChar X_TXT[] = wxT(" X: 0000");
    wxChar Y_TXT[] = wxT("Y: 0000");
    wxChar Z_TXT[] = wxT("Z: 0000");
    printHex(X_TXT+4, 4, cpu->registers[DCPU16_REG_X]);
    printHex(Y_TXT+3, 4, cpu->registers[DCPU16_REG_Y]);
    printHex(Z_TXT+3, 4, cpu->registers[DCPU16_REG_Z]);
    X->SetLabel(X_TXT);
    Y->SetLabel(Y_TXT);
    Z->SetLabel(Z_TXT);
    
    wxChar I_TXT[] = wxT(" I: 0000");
    wxChar J_TXT[] = wxT("J: 0000");
    wxChar EX_TXT[] = wxT("EX: 0000");
    printHex(I_TXT+4, 4, cpu->registers[DCPU16_REG_I]);
    printHex(J_TXT+3, 4, cpu->registers[DCPU16_REG_J]);
    printHex(EX_TXT+4, 4, cpu->registers[DCPU16_REG_EX]);
    I->SetLabel(I_TXT);
    J->SetLabel(J_TXT);
    EX->SetLabel(EX_TXT);
    
    
    wxChar curInstruction_TXT[1001] = wxT(" ");
    int length = cpu->disassembleCurInstruction(curInstruction_TXT+1, 1000);
    curInstruction_TXT[length+1] = 0;
    curInstruction->SetLabel(curInstruction_TXT);
}

dcpu16Config::dcpu16Config() { name = "dcpu16"; }
dcpu16Config::dcpu16Config(int& argc, wxChar**& argv) {
    name = "dcpu16";
}
cpu* dcpu16Config::createCpu() {
    return new dcpu16(false);
}
