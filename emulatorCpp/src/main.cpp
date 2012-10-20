#include <wx/cmdline.h>
#include "wx/wx.h"

#include "sysConfig.h"
#include "cpus/cpus.h"
#include "devices/devices.h"
#include "system.h"

class emulatorApp: public wxApp
{
    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    virtual int OnExit();
private:
    emulationConfig* config;
    compSystem** systems;
    int nSystems;
};
static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
     { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("displays help on the command line parameters"),
          wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, NULL, wxT("image"), wxT("Disables a cpu's boot sequence, instead loading the specified image into ram directly.")},
     CPUS_CMDLINE_HELP
     DEVICES_CMDLINE_HELP
     { wxCMD_LINE_PARAM, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
     { wxCMD_LINE_NONE }
};

class masterWindow: public wxFrame
{
public:

    masterWindow(const wxPoint& pos);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

enum
{
    ID_Quit = 1,
    ID_About,
};

BEGIN_EVENT_TABLE(masterWindow, wxFrame)
    EVT_BUTTON(ID_Quit, masterWindow::OnQuit)
END_EVENT_TABLE()

IMPLEMENT_APP(emulatorApp)

 
void emulatorApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (g_cmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (wxT("-"));
}
bool emulatorApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    bool silent_mode = parser.Found(wxT("s"));
 
    // to get at your unnamed parameters use
    /*wxArrayString files;
    for (int i = 0; i < parser.GetParamCount(); i++)
    {
            files.Add(parser.GetParam(i));
    }*/
 
    // and other command line parameters
 
    // then do what you need with them.
 
    return true;
}
bool emulatorApp::OnInit() {
    if (!wxApp::OnInit())
        return false;
    
    config = new emulationConfig(argc-1, argv+1);
    for (int i = 0; i < config->systemConfigs.size(); i++) {
        sysConfig* system = config->systemConfigs[i];
        std::cout << "System " << i << ":" << std::endl;
        std::cout << "\tCPU: " << system->cpu->name << std::endl;
        for (int j = 0; j < system->devices.size(); j++) {
            deviceConfig* device = system->devices[j];
            std::cout << "\tDevice " << j << ": " << device->name << std::endl;
        }
    }
    
    masterWindow *master = new masterWindow( wxPoint(50, 50));
    master->Show(true);
    SetTopWindow(master);
    
    config->createSystems(systems, nSystems);
    return true;
} 
int emulatorApp::OnExit() {
    for (int i = 0; i < nSystems; i++) {
            delete systems[i];
    }
    delete[] systems;
    free(config);
    return wxApp::OnExit();
} 

masterWindow::masterWindow(const wxPoint& pos)
: wxFrame( NULL, -1, _(""), pos, wxSize(200,200) )
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, -1, _("0x10c emulator")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
    sizer->Add(new wxButton(this, ID_Quit, _("Quit")), 0, 0, 0);
    sizer->SetSizeHints(this);
    SetSizer(sizer);

    /*wxMenu *menuFile = new wxMenu;

    menuFile->Append( ID_About, _("&About...") );
    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, _("E&xit") );

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("&File") );

    SetMenuBar( menuBar );

    CreateStatusBar();
    SetStatusText( _("Welcome to wxWidgets!") );*/
}

void masterWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}

/*
void masterWindow::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox( _("This is a wxWidgets Hello world sample"),
                  _("About Hello World"),
                  wxOK | wxICON_INFORMATION, this);
}*/
