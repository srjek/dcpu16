#ifndef emulator_main_h
#define emulator_main_h

#ifndef emulator_emulation_h
class emulation;
class emulationConfig;
#endif

class emulatorApp: public wxApp {
    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    virtual int OnRun();
    virtual int OnExit();
private:
    emulationConfig* config;
    emulation* environment;
};
#endif