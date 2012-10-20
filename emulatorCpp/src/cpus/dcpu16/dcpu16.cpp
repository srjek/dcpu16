#include <iostream>
#include "dcpu16.h"

class dcpu16;

enum {
    ID_Run = 1,
    ID_Step,
    ID_Stop,
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
    void OnStop(wxCommandEvent& WXUNUSED(event)) {
        Close(TRUE);
    }
    
    void update();
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(dcpu16CtrlWindow, wxFrame)
    EVT_BUTTON(ID_Run, dcpu16CtrlWindow::OnStop)
    EVT_BUTTON(ID_Step, dcpu16CtrlWindow::OnStop)
    EVT_BUTTON(ID_Stop, dcpu16CtrlWindow::OnStop)
END_EVENT_TABLE()


class dcpu16: public cpu {
    friend class dcpu16CtrlWindow;
protected:
    dcpu16CtrlWindow* ctrlWindow;
    
    unsigned short ram[0x10000];
    unsigned short registers[13];
    unsigned long long totalCycles;
public:
    dcpu16() {
        for (int i = 0; i < 0x10000; i++)
            ram[i] = 0;
        for (int i = 0; i < 13; i++)
            registers[i] = i;
        totalCycles = 0;
    }
    ~dcpu16() {
        if (ctrlWindow)
            delete ctrlWindow;
    }
    void createWindow() {
        ctrlWindow = new dcpu16CtrlWindow(wxPoint(50, 50), this);
        ctrlWindow->Show(true);
    }
    wxWindow* getWindow() {
        if (ctrlWindow)
            return ctrlWindow;
        return getTopLevelWindow();
    }
    void loadImage(const wxChar* imagePath) {
        return; //TODO:
    }
    void Run() {
    }
};

void printHex(wxChar* buffer, int bufferSize, long long number) {
    for (wxChar* c = buffer+bufferSize-1; c >= buffer; c--) {
        switch (number & 0xF) {
            case 0x0:
                *c = wxT('0');
                break;
            case 0x1:
                *c = wxT('1');
                break;
            case 0x2:
                *c = wxT('2');
                break;
            case 0x3:
                *c = wxT('3');
                break;
            case 0x4:
                *c = wxT('4');
                break;
            case 0x5:
                *c = wxT('5');
                break;
            case 0x6:
                *c = wxT('6');
                break;
            case 0x7:
                *c = wxT('7');
                break;
            case 0x8:
                *c = wxT('8');
                break;
            case 0x9:
                *c = wxT('9');
                break;
            case 0xA:
                *c = wxT('A');
                break;
            case 0xB:
                *c = wxT('B');
                break;
            case 0xC:
                *c = wxT('C');
                break;
            case 0xD:
                *c = wxT('D');
                break;
            case 0xE:
                *c = wxT('E');
                break;
            case 0xF:
                *c = wxT('F');
                break;
            default:
                //....HOW?
                break;
        }
        number >>= 4;
    }
}
void dcpu16CtrlWindow::update() {
    if (!cpu)
        return;
        
    wxChar PC_TXT[] = wxT("PC: 0000");
    wxChar SP_TXT[] = wxT("SP: 0000");
    wxChar IA_TXT[] = wxT("IA: 0000");
    printHex(PC_TXT+4, 4, cpu->registers[DCPU16_REG_PC]);
    printHex(SP_TXT+4, 4, cpu->registers[DCPU16_REG_SP]);
    printHex(IA_TXT+4, 4, cpu->registers[DCPU16_REG_IA]);
    PC->SetLabel(PC_TXT);
    SP->SetLabel(SP_TXT);
    IA->SetLabel(IA_TXT);
    
    wxChar A_TXT[] = wxT("A: 0000");
    wxChar B_TXT[] = wxT("B: 0000");
    wxChar C_TXT[] = wxT("C: 0000");
    printHex(A_TXT+3, 4, cpu->registers[DCPU16_REG_A]);
    printHex(B_TXT+3, 4, cpu->registers[DCPU16_REG_B]);
    printHex(C_TXT+3, 4, cpu->registers[DCPU16_REG_C]);
    A->SetLabel(A_TXT);
    B->SetLabel(B_TXT);
    C->SetLabel(C_TXT);
    
    wxChar X_TXT[] = wxT("X: 0000");
    wxChar Y_TXT[] = wxT("Y: 0000");
    wxChar Z_TXT[] = wxT("Z: 0000");
    printHex(X_TXT+3, 4, cpu->registers[DCPU16_REG_X]);
    printHex(Y_TXT+3, 4, cpu->registers[DCPU16_REG_Y]);
    printHex(Z_TXT+3, 4, cpu->registers[DCPU16_REG_Z]);
    X->SetLabel(X_TXT);
    Y->SetLabel(Y_TXT);
    Z->SetLabel(Z_TXT);
    
    wxChar I_TXT[] = wxT("I: 0000");
    wxChar J_TXT[] = wxT("J: 0000");
    wxChar EX_TXT[] = wxT("EX: 0000");
    printHex(I_TXT+3, 4, cpu->registers[DCPU16_REG_I]);
    printHex(J_TXT+3, 4, cpu->registers[DCPU16_REG_J]);
    printHex(EX_TXT+4, 4, cpu->registers[DCPU16_REG_EX]);
    I->SetLabel(I_TXT);
    J->SetLabel(J_TXT);
    EX->SetLabel(EX_TXT);
}

dcpu16Config::dcpu16Config() { name = "dcpu16"; }
dcpu16Config::dcpu16Config(int& argc, wxChar**& argv) {
    name = "dcpu16";
}
cpu* dcpu16Config::createCpu() {
    return new dcpu16();
}