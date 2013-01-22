#include <wx/cmdline.h>
#include "wx/wx.h"

#include "freeglut.h"
#include "emulation.h"
#include "cpus/cpus.h"
#include "devices/devices.h"

#include <wx/cmdline.h>
#include "wx/wx.h"
#include "main.h"

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
     { wxCMD_LINE_SWITCH, wxT_2("h"), wxT_2("help"), wxT_2("displays help on the command line parameters"),
          wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, NULL, wxT_2("image"), wxT_2("Disables a cpu's boot sequence, instead loading the specified image into ram directly.")},
     { wxCMD_LINE_SWITCH, NULL, wxT_2("debug"), wxT_2("May enable debug options on the last specified cpu or device")},
     { wxCMD_LINE_SWITCH, NULL, wxT_2("gdb"), wxT_2("Last cpu will host a gdb remote server.")},
     CPUS_CMDLINE_HELP
     DEVICES_CMDLINE_HELP
     { wxCMD_LINE_PARAM, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
     { wxCMD_LINE_NONE }
};

class masterWindow: public wxFrame {
    emulatorApp* app;
public:
    masterWindow(emulatorApp* app, const wxPoint& pos);

    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};

enum {
    ID_Quit = 1,
};

BEGIN_EVENT_TABLE(masterWindow, wxFrame)
    EVT_BUTTON(ID_Quit, masterWindow::OnQuit)
    EVT_CLOSE(masterWindow::OnClose)
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
    
    int orig_argc = argc;
    char** new_argv = (char**) malloc(argc*sizeof(char*));
    for (int i = 0; i < argc; i++) {
        wxString tmp(argv[i]);
        const char* tmp_c = tmp.mb_str(wxConvLibc);
        new_argv[i] = (char*) malloc((strlen(tmp_c)+2)*sizeof(char));
        strcpy(new_argv[i], tmp_c);
    }
    glutInit(&argc, new_argv);  //Supposed to be the unaltered int* argc/char** argv, but wxwidgets insists on wxChar everywhere
    for (int i = 0; i < orig_argc; i++)
        free(new_argv[i]);
    free(new_argv);
    
    config = new emulationConfig(argc-1, ((wxChar**)argv+1));   //in a specific MinGW/library configuration, argv+1 is ambigous. argv should be wxChar**, who knew?
    config->print();
    
    masterWindow *master = new masterWindow(this, wxPoint(50, 50));
    master->Show(true);
    SetTopWindow(master);
    SetExitOnFrameDelete(true);
    
    setFreeglutManager(new freeglut());
    getFreeglutManager()->Run();
    environment = config->createEmulation();
    
    return true;
}
int emulatorApp::OnRun() {
    initGlew();
        
    environment->Run();
    return wxApp::OnRun();
}
int emulatorApp::OnExit() {
    getFreeglutManager()->Stop();
    if (environment) {
        environment->Stop();
        environment->Wait();
        delete environment;
    }
    delete config;
    return wxApp::OnExit();
}

masterWindow::masterWindow(emulatorApp* app, const wxPoint& pos)
: wxFrame( NULL, -1, _("0x10c emulator"), pos, wxSize(200,200) )
{
    this->app = app;
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(2);
    sizer->Add(new wxStaticText(this, -1, _("0x10c emulator")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
    sizer->AddSpacer(4);
    sizer->Add(new wxButton(this, ID_Quit, _("Quit")), 0, wxALIGN_CENTER_HORIZONTAL, 0);
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

void masterWindow::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(TRUE);
}

void masterWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
    if (app->environment) {
        app->environment->Stop();
        app->environment->Wait();
    }
    Destroy();
}
