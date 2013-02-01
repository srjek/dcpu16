#include "wx/wx.h"
#include "dcpu16-cpu.h"

#ifndef emulator_dcpu16_gui_h
#define emulator_dcpu16_gui_h

class dcpu16CtrlWindow;

class dcpu16CtrlWindowTimer : public wxTimer {
    dcpu16CtrlWindow* panel;
public:
    dcpu16CtrlWindowTimer(dcpu16CtrlWindow* panel);
    void Notify();
    void start();
};

class dcpu16CtrlWindow: public wxFrame {
friend class CtrlWindowTimer;
protected:
    dcpu16* cpu;
    dcpu16CtrlWindowTimer timer;
    
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
    
    wxButton* runButton;
    wxButton* stepButton;
    wxButton* stopButton;
    wxButton* resetButton;
    
public:
    dcpu16CtrlWindow(const wxPoint& pos, dcpu16* cpu);
    void OnRun(wxCommandEvent& WXUNUSED(event));
    void OnStep(wxCommandEvent& WXUNUSED(event));
    void OnStop(wxCommandEvent& WXUNUSED(event));
    void OnReset(wxCommandEvent& WXUNUSED(event));
    void OnClose(wxCloseEvent& event);
    
    void disableButtons();
    void enableButtons();
    
    void update();
    void Notify();
    DECLARE_EVENT_TABLE()
};

#endif
