#ifndef emulator_main_h
#define emulator_main_h

#ifndef emulator_emulation_h
class emulation;
class emulationConfig;
#endif

class emulatorApp: public wxApp {
public:
    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    virtual int OnRun();
    virtual int OnExit();
    emulation* environment;
private:
    emulationConfig* config;
};
#endif