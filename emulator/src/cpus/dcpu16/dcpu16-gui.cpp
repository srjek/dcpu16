#include "dcpu16-gui.h"
#include "../../strHelper.h"

enum {
    ID_Run = 1,
    ID_Step,
    ID_Stop,
    ID_Reset,
};

dcpu16CtrlWindowTimer::dcpu16CtrlWindowTimer(dcpu16CtrlWindow* panel): wxTimer() {
    this->panel = panel;
}
void dcpu16CtrlWindowTimer::Notify() {
    panel->update();
}
void dcpu16CtrlWindowTimer::start() {
    wxTimer::Start(500);
}


dcpu16CtrlWindow::dcpu16CtrlWindow(const wxPoint& pos, dcpu16* cpu):
            wxFrame( getTopLevelWindow(), -1, _("dcpu16"), pos, wxSize(200,200) ), timer(this)  {
        
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(2);
    wxFlexGridSizer* innerSizer0 = new wxFlexGridSizer(5, 2, 1, 0);
    cycles = new wxStaticText(this, -1, _("Cycles: 0"));
    innerSizer0->Add(cycles, 0, wxALIGN_LEFT | wxALIGN_CENTRE, 0);
    innerSizer0->Add(resetButton = new wxButton(this, ID_Reset, _("Reset")), 0, wxALIGN_RIGHT, 0);
    innerSizer0->AddGrowableCol(0);
    sizer->Add(innerSizer0, 0, wxEXPAND, 0);
    sizer->AddSpacer(4);
    
    wxFlexGridSizer* innerSizer = new wxFlexGridSizer(5, 3, 4, 0);
    innerSizer->Add(runButton = new wxButton(this, ID_Run, _("Run")), 0, 0, 0);
    innerSizer->Add(stepButton = new wxButton(this, ID_Step, _("Step")), 0, 0, 0);
    innerSizer->Add(stopButton = new wxButton(this, ID_Stop, _("Stop")), 0, 0, 0);
    
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
    
    timer.start();
}
void dcpu16CtrlWindow::OnRun(wxCommandEvent& WXUNUSED(event)) {
    cpu->debug_run();
}
void dcpu16CtrlWindow::OnStep(wxCommandEvent& WXUNUSED(event)) {
    cpu->debug_step();
}
void dcpu16CtrlWindow::OnStop(wxCommandEvent& WXUNUSED(event)) {
    cpu->debug_stop();
}
void dcpu16CtrlWindow::OnReset(wxCommandEvent& WXUNUSED(event)) {
    cpu->debug_reset();
}
void dcpu16CtrlWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
    cpu->ctrlWindow = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}
    
void dcpu16CtrlWindow::disableButtons() {
    runButton->Disable();
    stepButton->Disable();
    stopButton->Disable();
    resetButton->Disable();
}
void dcpu16CtrlWindow::enableButtons() {
    runButton->Enable();
    stepButton->Enable();
    stopButton->Enable();
    resetButton->Enable();
}
    
void dcpu16CtrlWindow::update() {
    if (!cpu)
        return;
    
    if (cpu->debugger_attached)
        disableButtons();
    else
        enableButtons();
        
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
void dcpu16CtrlWindow::Notify() {
    update();
}

BEGIN_EVENT_TABLE(dcpu16CtrlWindow, wxFrame)
    EVT_BUTTON(ID_Run, dcpu16CtrlWindow::OnRun)
    EVT_BUTTON(ID_Step, dcpu16CtrlWindow::OnStep)
    EVT_BUTTON(ID_Stop, dcpu16CtrlWindow::OnStop)
    EVT_BUTTON(ID_Reset, dcpu16CtrlWindow::OnReset)
    EVT_CLOSE(dcpu16CtrlWindow::OnClose)
END_EVENT_TABLE()
