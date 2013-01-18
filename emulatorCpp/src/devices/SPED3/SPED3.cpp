#include <GL/glew.h>
#ifdef __WXMAC__
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#include <iostream>
#include <vector>
#include <cstdint>

#include "SPED3.h"
#include "SPED3_shaders.h"

class RenderTimer : public wxTimer {
    wxGLCanvas* panel;
public:
    RenderTimer(wxGLCanvas* panel) : wxTimer() {
        this->panel = panel;
    }
    void Notify() {
        panel->Refresh();
        panel->Update();
    }
    void start() {
        wxTimer::Start(1000/60);
    }
    void stop() {
        wxTimer::Stop();
    }
};
 
class SPED3;

class SPED3DisplayPanel: public wxGLCanvas {
    friend class RenderTimer;
protected:
    int args[5] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0};
    
    SPED3* device;
    RenderTimer timer;
    wxGLContext* gl_context;
    bool firstTime = true;
    
    GLuint shaderProgram;
    float* vertexData;
    GLuint vertexBufferObject;
public:
    SPED3DisplayPanel(wxFrame* parent, const wxSize& size, SPED3* device);
    ~SPED3DisplayPanel();
    void paintEvent(wxPaintEvent& evt) {
        render();
    }
    void resized(wxSizeEvent& evt) {
        //wxGLCanvas::OnSize(evt);
        Refresh();
    }
    void loadShaders() {
	    std::vector<GLuint> shaderList;

	    shaderList.push_back(CreateShader(GL_VERTEX_SHADER, SPED3_VertexShader));
	    shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, SPED3_FragmentShader));

	    shaderProgram = CreateProgram(shaderList);

	    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
	}
    void initializeGL();
    void render();
    void OnShow() {
        initializeGL();
        Refresh();
        Update();
    }
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(SPED3DisplayPanel, wxGLCanvas)
    EVT_KEY_DOWN(SPED3DisplayPanel::OnKeyDown)
    EVT_KEY_UP(SPED3DisplayPanel::OnKeyUp)
    EVT_PAINT(SPED3DisplayPanel::paintEvent)
    EVT_SIZE(SPED3DisplayPanel::resized)
END_EVENT_TABLE()

class SPED3Display: public wxFrame {
protected:
    SPED3* device;
    SPED3DisplayPanel* panel;
public:
    SPED3Display(wxWindow* parent, const wxPoint& pos, const wxSize& size, SPED3* device): wxFrame(parent, -1, _("SPED-3"), pos, wxSize(-1, -1)) {
        this->device = device;
        SetClientSize(size);
        panel = new SPED3DisplayPanel(this, size, device);
        
        //wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        //sizer->Add(panel, 1, wxEXPAND);
        //SetSizer(sizer);
        //SetAutoLayout(true);
    }
    bool Show(bool show = true) {
        wxFrame::Show(show);
        panel->OnShow();
        Refresh();
        Update();
    }
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnClose(wxCloseEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(SPED3Display, wxFrame)
    EVT_KEY_DOWN(SPED3Display::OnKeyDown)
    EVT_KEY_UP(SPED3Display::OnKeyUp)
    EVT_CLOSE(SPED3Display::OnClose)
END_EVENT_TABLE()


class SPED3: public device {
    friend class SPED3Display;
    friend class SPED3DisplayPanel;
protected:
    cpu* host;
    SPED3Display* display;
    
    static const int MAX_VERTICES = 128;
    volatile unsigned short vertexAddress;
    volatile unsigned short vertexCount;
    volatile unsigned short targetRotation;
    volatile unsigned short currentRotation;
    
    static const int w = 128;
    static const int h = 96;
    static const int scale = 3;
    static const int width = w*scale;
    static const int height = h*scale;
    
    static const int STATE_NO_DATA =    0x0000;
    static const int STATE_RUNNING =    0x0001;
    static const int STATE_TURNING =    0x0002;

    static const int ERROR_NONE =       0x0000;
    static const int ERROR_BROKEN =     0xFFFF;
    
    std::vector<keyHandler*> keyHandlers;
    
public:
    void reset() {
        vertexAddress = 0;
        vertexCount = 0;
        targetRotation = 0;
        currentRotation = 0;
    }
    SPED3(cpu* host) {
        this->host = host;
        host->addHardware(this);
        
        reset();
    }
    ~SPED3() {
        if (display)
            display->Close(true);
    }
    void createWindow() {
        display = new SPED3Display(host->getWindow(), wxPoint(50, 50), wxSize(width, height), this);
        display->Show(true);
    }
    unsigned long getManufacturer() {
        return 0x1eb37e91;  //Mackapar Media
    }
    unsigned long getId() {
        return 0x42babf3c;  //Suspended Particle Exciter Display
    }
    unsigned long getVersion() {
        return 0x0003;  //Rev 3
    }
    
    int interrupt() {
        unsigned short A = host->registers[cpu::REG_A];
        if (A == 0) {           //POLL
            host->registers[cpu::REG_B] = STATE_NO_DATA; //TODO: report state
            host->registers[cpu::REG_C] = ERROR_NONE;  //Emulators can't fail!
        } else if (A == 1) {    //MAP
            vertexAddress = host->registers[cpu::REG_X];
            unsigned short B = host->registers[cpu::REG_Y];
            if (B > MAX_VERTICES)
                vertexCount = MAX_VERTICES;
            else
                vertexCount = B;
        } else if (A == 2)      //ROTATE
            targetRotation = host->registers[cpu::REG_X]%360;
        return 0;
    }
    void registerKeyHandler(keyHandler* handler) {
        keyHandlers.push_back(handler);
    }
    void OnKeyDown(wxKeyEvent& event) {
        for (int i = 0; i < keyHandlers.size(); i++)
            keyHandlers[i]->OnKeyEvent(event.GetKeyCode(), true);
    }
    void OnKeyUp(wxKeyEvent& event) {
        for (int i = 0; i < keyHandlers.size(); i++)
            keyHandlers[i]->OnKeyEvent(event.GetKeyCode(), false);
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

SPED3DisplayPanel::SPED3DisplayPanel(wxFrame* parent, const wxSize& size, SPED3* device):
                wxGLCanvas(parent, wxID_ANY, /*args,*/ wxPoint(0, 0), size, wxFULL_REPAINT_ON_RESIZE), timer(this) {
    this->device = device;
    
	//gl_context = new wxGLContext(this);
    //SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}
SPED3DisplayPanel::~SPED3DisplayPanel() {
    delete[] vertexData;
    timer.stop();
}
void SPED3DisplayPanel::initializeGL() {
    firstTime = false;
    
    SetCurrent();
    wxPaintDC(this);
    initGlew();
    
    loadShaders();
    vertexData = new float[SPED3::MAX_VERTICES*4*2];
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    timer.start();
}
void SPED3DisplayPanel::render() {
    if (firstTime) {
        initializeGL();
    }
    SetCurrent();
    wxPaintDC(this);
    
    //wxLongLong time = wxGetLocalTimeMillis();
    //bool blink = (((time/16)/20) % 2) == 0;
    volatile unsigned short* ram = device->host->ram;
    unsigned short vertexAddress = device->vertexAddress;
    unsigned short vertexCount = device->vertexCount;
    
    static const int color_offset = SPED3::MAX_VERTICES*4;
    static int lastCount = 0;
    
    if (lastCount != vertexCount) {
        std::cout << "vertexCount is now " << vertexCount << std::endl;
        lastCount = vertexCount;
    }
    
    for (int i = 0; i < vertexCount; i++) {
        unsigned short firstWord = ram[vertexAddress+i*2];
        float x = firstWord & 0xFF;
        x /= 255;
        x = (x * 2) - 1;
        float y = (firstWord >> 8) & 0xFF;
        y /= 255;
        y = (y * 2) - 1;
        
        unsigned short secondWord = ram[vertexAddress+i*2];
        float z = secondWord & 0xFF;
        z /= 255;
        z = (z * 2) - 1;
        
        int color = (secondWord >> 8) & 0x07;
        
        vertexData[i*4+0] = x;
        vertexData[i*4+1] = y;
        vertexData[i*4+2] = z;
        vertexData[i*4+3] = 1.0f;
        
        if (color & 0x3) {
            vertexData[color_offset+i*4+0] = ( (color&0x3) == 1 );
            vertexData[color_offset+i*4+1] = ( (color&0x3) == 2 );
            vertexData[color_offset+i*4+2] = ( (color&0x3) == 3 );
        } else {
            vertexData[color_offset+i*4+0] = 0.2f;
            vertexData[color_offset+i*4+1] = 0.2f;
            vertexData[color_offset+i*4+2] = 0.2f;
        }
        if (color & 0x4)
            vertexData[color_offset+i*4+3] = 1.0f;
        else
            vertexData[color_offset+i*4+3] = 1.0f;
    }
    
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glUseProgram(shaderProgram);
	
	/*
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)(color_offset));
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	
	glDrawArrays(GL_LINE_STRIP, 0, vertexCount); */
	
	float vertexData2[] = {
	     0.0f,    0.5f, 0.0f, 1.0f,
	     0.5f, -0.366f, 0.0f, 1.0f,
	    -0.5f, -0.366f, 0.0f, 1.0f,
	     1.0f,    0.0f, 0.0f, 1.0f,
	     0.0f,    1.0f, 0.0f, 1.0f,
	     0.0f,    0.0f, 1.0f, 1.0f,
    };
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData2), vertexData2, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)48);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	
	glDrawArrays(GL_LINE_STRIP, 0, vertexCount);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	
    glFlush();
    SwapBuffers();
}
 
void SPED3DisplayPanel::OnKeyDown(wxKeyEvent& event) {
//    device->OnKeyDown(event);
}
void SPED3DisplayPanel::OnKeyUp(wxKeyEvent& event) {
//    device->OnKeyUp(event);
}
void SPED3Display::OnKeyDown(wxKeyEvent& event) {
//    device->OnKeyDown(event);
}
void SPED3Display::OnKeyUp(wxKeyEvent& event) {
//    device->OnKeyUp(event);
}

void SPED3Display::OnClose(wxCloseEvent& event) {
    device->display = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}

device* SPED3Config::createDevice(cpu* host) {
    return new SPED3(host);
}
