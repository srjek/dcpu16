#include <iostream>
#include "dcpu16.h"
#include "../../strHelper.h"

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
        for (int i = 0; i < 13; i++)
            registers[i] = 0xFFFF - i;
        ram[registers[DCPU16_REG_PC]] = 0x1 | (0x0 << 5) | (0x1F << 10);
        ram[(registers[DCPU16_REG_PC]+1)&0xFFFF] = 0x8000;
        return; //TODO
    }
    void Run() {
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
                wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("")   , wxT("HCF"),
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
            
            if (op != 0) {
                length += wxStrcpy(buffer+length, values[b_code], bufferSize-length);
                buffer[length++] = wxT(',');
                buffer[length++] = wxT(' ');
            }
            length+= wxStrcpy(buffer+length, values[a_code], bufferSize-length);
        }
        return length;
    }
};

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
    cpu->disassembleCurInstruction(curInstruction_TXT+1, 1000);
    curInstruction->SetLabel(curInstruction_TXT);
}

dcpu16Config::dcpu16Config() { name = "dcpu16"; }
dcpu16Config::dcpu16Config(int& argc, wxChar**& argv) {
    name = "dcpu16";
}
cpu* dcpu16Config::createCpu() {
    return new dcpu16();
}