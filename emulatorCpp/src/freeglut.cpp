#include "freeglut.h"

freeglut* freeglutManager;
freeglut* getFreeglutManager() { return freeglutManager; }
void setFreeglutManager(freeglut* manager) { freeglutManager = manager; }

freeglut::freeglut(): wxThread(wxTHREAD_JOINABLE) {
    Create();
    MainLoopLock = new wxMutex();
    QueueLock = new wxMutex();
}
freeglut::~freeglut() {
    MainLoopLock->Lock();
    QueueLock->Lock();
    delete MainLoopLock;
    delete QueueLock;
}

void freeglut::addToQueueAndWait(freeglutCallback* callback) {
    QueueLock->Lock();
    callbackQueue.push(callback);
    QueueLock->Unlock();
    callback->wait();
}

void freeglut::StartMainLoop() {
    running = true;
    if (MainLoopLock->TryLock() == wxMUTEX_NO_ERROR) {
	    unsigned int displayMode = GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
        glutInitDisplayMode (displayMode);
	    glutInitContextVersion (3, 3);
	    glutInitContextProfile(GLUT_CORE_PROFILE);
	    
        int tmpWindow = glutCreateWindow("GLEW init");
        glutShowWindow();
        initGlew();
        std::cout << "OpenGL: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "OpenGL: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
        glutHideWindow();
        glutDestroyWindow(tmpWindow);
        
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
        
        while (running) {
            glutMainLoopEvent();
            QueueLock->Lock();
            if (!callbackQueue.empty()) {
                callbackQueue.front()->callback();
                callbackQueue.pop();
            }
            QueueLock->Unlock();
        }
        
        for (std::map<int,freeglutWindow*>::iterator it=windows.begin(); it!=windows.end(); ++it) {
            glutDestroyWindow(it->first);
        }
        windows.clear();
    
        MainLoopLock->Unlock();
    }
}
void freeglut::StopMainLoop() {
    glutLeaveMainLoop();
    running = false;
}
wxThreadError freeglut::Create() {
    return wxThread::Create(100*1024*1024); //100 MB
}
wxThreadError freeglut::Run() { return wxThread::Run(); }
wxThread::ExitCode freeglut::Entry() {
    StartMainLoop();
    return 0;
}
void freeglut::Stop() {
    StopMainLoop();
}
wxThread::ExitCode freeglut::Wait() {
    if (IsRunning())
        return wxThread::Wait();
    return 0;
}
    

void freeglutManager_glutDisplayFunc() {
    freeglutManager->windows[glutGetWindow()]->OnDisplay();
}
void freeglutManager_glutReshapeFunc(int width, int height) {
    freeglutManager->windows[glutGetWindow()]->OnReshape(width, height);
}
void freeglutManager_glutKeyboardFunc(unsigned char key, int x, int y) {
    freeglutManager->windows[glutGetWindow()]->OnKeyboard(key, x, y);
}
void freeglutManager_glutMouseFunc(int button, int state, int x, int y) {
    freeglutManager->windows[glutGetWindow()]->OnMouse(button, state, x, y);
}
void freeglutManager_glutMotionFunc(int x, int y) {
    freeglutManager->windows[glutGetWindow()]->OnMotion(x, y);
}
void freeglutManager_glutPassiveMotionFunc(int x, int y) {
    freeglutManager->windows[glutGetWindow()]->OnPassiveMotion(x, y);
}
void freeglutManager_glutVisibilityFunc(int state) {
    freeglutManager->windows[glutGetWindow()]->OnVisibility(state);
}
void freeglutManager_glutEntryFunc(int state) {
    freeglutManager->windows[glutGetWindow()]->OnEntry(state);
}
void freeglutManager_glutSpecialFunc(int key, int x, int y) {
    freeglutManager->windows[glutGetWindow()]->OnSpecial(key, x, y);
}
void freeglutManager_RenderTimer(int value) {
    freeglutManager->windows[value]->OnRender();
    glutTimerFunc(35, freeglutManager_RenderTimer, value);
}
                                   
class createWindowCallback: public freeglutCallback {
public:
    char *name;
    volatile int result;
    createWindowCallback(char *name): freeglutCallback(), name(name), result(0) { }
    void callback() {
	    unsigned int displayMode = GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL;
        glutInitDisplayMode (displayMode);
	    glutInitContextVersion (3, 3);
	    glutInitContextProfile(GLUT_CORE_PROFILE);
        result = glutCreateWindow(name);
        waiting = false;
    }
};
int freeglut::CreateWindow(char *name) {
    createWindowCallback* tmp = new createWindowCallback(name);
    addToQueueAndWait(tmp);
    int result = tmp->result;
    delete tmp;
    return result;
}
void freeglut::RegisterWindow(freeglutWindow* window) {
    registerWindowCallback* tmp = new registerWindowCallback(window);
    addToQueueAndWait(tmp);
    delete tmp;
}
void registerWindowCallback::callback() {
    freeglutManager->windows[window->id] = window;
    glutSetWindow(window->id);
    
    glutDisplayFunc(freeglutManager_glutDisplayFunc);
    glutReshapeFunc(freeglutManager_glutReshapeFunc);
    glutKeyboardFunc(freeglutManager_glutKeyboardFunc);
    glutMouseFunc(freeglutManager_glutMouseFunc);
    glutMotionFunc(freeglutManager_glutMotionFunc);
    glutPassiveMotionFunc(freeglutManager_glutPassiveMotionFunc);
    glutVisibilityFunc(freeglutManager_glutVisibilityFunc);
    glutEntryFunc(freeglutManager_glutEntryFunc);
    glutSpecialFunc(freeglutManager_glutSpecialFunc);
    glutTimerFunc(2000, freeglutManager_RenderTimer, window->id);
    
    glutShowWindow();
    waiting = false;
}

void freeglutWindow::DefaultReshapeFunc(int width, int height) {
    glViewport(0,0,width,height);
}
