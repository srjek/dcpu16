#include <iostream>
#include "dcpu16.h"

enum {
    ID_Quit = 1,
};

class dcpu16CtrlWindow: public wxFrame {
public:
    dcpu16CtrlWindow(const wxPoint& pos): wxFrame( getTopLevelWindow(), -1, _("dcpu16"), pos, wxSize(200,200) )  {
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new wxStaticText(this, -1, _("dcpu16")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
        sizer->Add(new wxButton(this, ID_Quit, _("Quit")), 0, 0, 0);
        sizer->SetSizeHints(this);
        SetSizer(sizer);
    }
    void OnQuit(wxCommandEvent& WXUNUSED(event)) {
        Close(TRUE);
    }
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(dcpu16CtrlWindow, wxFrame)
    EVT_BUTTON(ID_Quit, dcpu16CtrlWindow::OnQuit)
END_EVENT_TABLE()

class dcpu16: public cpu {
private:
    dcpu16CtrlWindow* ctrlWindow;
public:
    dcpu16() { }
    ~dcpu16() {
        if (ctrlWindow)
            delete ctrlWindow;
    }
    void createCtrlWindow() {
        ctrlWindow = new dcpu16CtrlWindow(wxPoint(50, 50));
        ctrlWindow->Show(true);
    }
    void loadImage(const wxChar* imagePath) {
        return; //TODO:
    }
    void Run() {
    }
};

dcpu16Config::dcpu16Config() { name = "dcpu16"; }
dcpu16Config::dcpu16Config(int& argc, wxChar**& argv) {
    name = "dcpu16";
}
cpu* dcpu16Config::createCpu() {
    return new dcpu16();
}