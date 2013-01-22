#include "thread.h"
#include <vector>
#include <queue>
#include <map>
#include <iostream>
#include <GL/freeglut.h>
#include "glew.h"
#undef CreateWindow
#undef CreateWindowW

#ifndef emulator_freeglut_h
#define emulator_freeglut_h

class freeglutCallback {
public:
    volatile bool waiting;
    freeglutCallback(): waiting(true) {};
    virtual void callback() = 0;
    void wait() {
        while (waiting) ;
    }
};

class freeglutWindow;
class registerWindowCallback: public freeglutCallback {
protected:
    freeglutWindow* window;
public:
    registerWindowCallback(freeglutWindow* window): freeglutCallback(), window(window) { }
    void callback();
};

class freeglut: public wxThread, public thread {
    friend class freeglutWindow;
    friend class registerWindowCallback;
    friend void ::freeglutManager_glutDisplayFunc();
    friend void ::freeglutManager_glutReshapeFunc(int, int);
    friend void ::freeglutManager_glutKeyboardFunc(unsigned char, int, int);
    friend void ::freeglutManager_glutMouseFunc(int, int, int, int);
    friend void ::freeglutManager_glutMotionFunc(int, int);
    friend void ::freeglutManager_glutPassiveMotionFunc(int, int);
    friend void ::freeglutManager_glutVisibilityFunc(int);
    friend void ::freeglutManager_glutEntryFunc(int);
    friend void ::freeglutManager_glutSpecialFunc(int, int, int);
    friend void ::freeglutManager_RenderTimer(int);
protected:
    std::map<int, freeglutWindow*> windows;
    std::queue<freeglutCallback*> callbackQueue;
    volatile bool running;
    wxMutex* MainLoopLock;
    wxMutex* QueueLock;
    
    void addToQueueAndWait(freeglutCallback* callback);
public:
    freeglut();
    ~freeglut();
    void StartMainLoop();
    void StopMainLoop();
    int CreateWindow(char* name);
    void RegisterWindow(freeglutWindow* window);
    
    wxThreadError Create();
    wxThreadError Run();
    wxThread::ExitCode Entry();
    void Stop();
    wxThread::ExitCode Wait();
};
freeglut* getFreeglutManager();
void setFreeglutManager(freeglut* manager);

class freeglutWindow {
    friend class registerWindowCallback;
protected:
    int id;
    void DefaultReshapeFunc(int width, int height);
public:
    freeglutWindow(char* name) {
        freeglut* freeglutManager = getFreeglutManager();
        id = freeglutManager->CreateWindow(name);
        freeglutManager->RegisterWindow(this);
    }
    
    virtual void OnDisplay() =0;
    virtual void OnReshape(int width, int height) {
        DefaultReshapeFunc(width, height);
    }
    virtual void OnKeyboard(unsigned char key, int x, int y) =0;
    virtual void OnMouse(int button, int state, int x, int y) =0;
    virtual void OnMotion(int x, int y) =0;
    virtual void OnPassiveMotion(int x, int y) =0;
    virtual void OnVisibility(int state) =0;
    virtual void OnEntry(int state) =0;
    virtual void OnSpecial(int key, int x, int y) =0;
    virtual void OnRender() =0;
};

#endif
