#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>

#include <GL/glew.h>
#include "SPED3.h"
#include "SPED3_shaders.h"
#include "../../glew.h"
#include "../../freeglut.h"

#define PI 3.14159265359
#define TAU 2*PI

class SPED3;

class SPED3_freeglutWindow: public freeglutWindow {
protected:
    SPED3* device;
    bool firstTime = true;
    wxLongLong lastTime = 0;
    
    GLuint shaderProgram;
    float* vertexData;
    GLuint vertexBufferObject;
    GLuint vao;
public:
    SPED3_freeglutWindow(SPED3* device);
    ~SPED3_freeglutWindow();
    
    void loadShaders() {
	    std::vector<GLuint> shaderList;

	    shaderList.push_back(CreateShader(GL_VERTEX_SHADER, SPED3_VertexShader));
	    shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, SPED3_FragmentShader));

	    shaderProgram = CreateProgram(shaderList);

	    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
	}
    void initializeGL();
    
    void OnDisplay();
    void OnKeyboard(unsigned char key, int x, int y) {}
    void OnMouse(int button, int state, int x, int y) {}
    void OnMotion(int x, int y) {}
    void OnPassiveMotion(int x, int y) {}
    void OnVisibility(int state) {}
    void OnEntry(int state) {}
    void OnSpecial(int key, int x, int y) {}
    void OnRender() {
        OnDisplay();
    }
};


class SPED3: public device {
    friend class SPED3_freeglutWindow;
protected:
    cpu* host;
    
    static const int MAX_VERTICES = 128;
    volatile unsigned short vertexAddress;
    volatile unsigned short vertexCount;
    volatile float targetRotation;
    volatile float currentRotation;
    
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
    ~SPED3() { }
    void createWindow() {
        new SPED3_freeglutWindow(this);
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
            unsigned short Y = host->registers[cpu::REG_Y];
            if (Y > MAX_VERTICES)
                vertexCount = MAX_VERTICES;
            else
                vertexCount = Y;
        } else if (A == 2) {    //ROTATE
            float rotation = host->registers[cpu::REG_X]%360;
            targetRotation = (rotation * TAU) / 360;
        }
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

SPED3_freeglutWindow::SPED3_freeglutWindow(SPED3* device): freeglutWindow("SPED-3") {
    this->device = device;
    vertexData = new float[SPED3::MAX_VERTICES*4*2];
}
SPED3_freeglutWindow::~SPED3_freeglutWindow() {
    delete[] vertexData;
}
void SPED3_freeglutWindow::initializeGL() {
    if (firstTime) {
        firstTime = false;
        
        loadShaders();
        
	    glGenVertexArrays(1, &vao);
	    glBindVertexArray(vao);
	    
        glGenBuffers(1, &vertexBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*SPED3::MAX_VERTICES*2, vertexData, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glBindVertexArray(0);
    }
}
void SPED3_freeglutWindow::OnDisplay() {
    if (firstTime) {
        initializeGL();
    }
    
    wxLongLong time = wxGetLocalTimeMillis();
    volatile unsigned short* ram = device->host->ram;
    unsigned short vertexAddress = device->vertexAddress;
    unsigned short vertexCount = device->vertexCount;
    
    float targetRotation = device->targetRotation;
    float currentRotation = device->currentRotation;
    
    float diffRotation = targetRotation - currentRotation;
    while (diffRotation > PI)
        diffRotation -= TAU;
    while (diffRotation < -PI)
        diffRotation += TAU;
    float maxChange = (time-lastTime).ToDouble();
    maxChange *= (50.0f*TAU/360.0f)/1000;
    if (diffRotation > maxChange)
        diffRotation = maxChange;
    else if (diffRotation < -maxChange)
        diffRotation = -maxChange;
    currentRotation += diffRotation;
    device->currentRotation = currentRotation;
    lastTime = time;
    
    static const int color_offset = SPED3::MAX_VERTICES*4;
    static int lastCount = 0;
    static unsigned short lastAddress = 0;
    static unsigned short lastValue = 0;
    
    if (lastCount != vertexCount || lastAddress != vertexAddress || lastValue != ram[vertexAddress]) {
        std::cout << vertexAddress << ", " << vertexCount << " [" << ram[vertexAddress] << "]" << std::endl;
        //std::cout << "vertexCount is now " << vertexCount << std::endl;
        lastCount = vertexCount;
        lastAddress = vertexAddress;
        lastValue = ram[vertexAddress];
        
        float x = lastValue & 0xFF;
        std::cout << x << std::endl;
        x /= 255.0f;
        std::cout << x << std::endl;
        x = (x * 2.0f) - 1.0f;
        std::cout << x << std::endl;
    }
    
    for (int i = 0; i < vertexCount; i++) {
        unsigned short firstWord = ram[vertexAddress+i*2];
        //std::cout << firstWord << std::endl;
        float x = firstWord & 0xFF;
        x /= 255;
        x = (x * 1.8f) - 0.9f;
        float y = (firstWord >> 8) & 0xFF;
        y /= 255.0f;
        y = (y * 1.8f) - 0.9f;
        
        unsigned short secondWord = ram[vertexAddress+i*2+1];
        float z = secondWord & 0xFF;
        z /= 255.0f;
        z = (z * 1.8f) - 0.9f;
        
        int color = (secondWord >> 8) & 0x07;
        
        float cosScale = cos(currentRotation);
        float sinScale = sin(currentRotation);
        
        vertexData[i*4+0] = x*cosScale + y*sinScale; //final X coord
        vertexData[i*4+1] = z;
        vertexData[i*4+2] = x*sinScale + y*cosScale; //final Y coord
        vertexData[i*4+3] = 1.0f;
        
        if (color & 0x3) {
            if ((color&0x3) == 1)
                vertexData[color_offset+i*4+0] = 1.0f;
            else
                vertexData[color_offset+i*4+0] = 0.0f;
            if ((color&0x3) == 2)
                vertexData[color_offset+i*4+1] = 1.0f;
            else
                vertexData[color_offset+i*4+1] = 0.0f;
            if ((color&0x3) == 3)
                vertexData[color_offset+i*4+2] = 1.0f;
            else
                vertexData[color_offset+i*4+2] = 0.0f;
        } else {
            vertexData[color_offset+i*4+0] = 0.2f;
            vertexData[color_offset+i*4+1] = 0.2f;
            vertexData[color_offset+i*4+2] = 0.2f;
        }
        if (color & 0x4)
            vertexData[color_offset+i*4+3] = 0.8f;
        else
            vertexData[color_offset+i*4+3] = 0.4f;
    }
    
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*4*SPED3::MAX_VERTICES*2, vertexData);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*SPED3::MAX_VERTICES*2, vertexData, GL_STATIC_DRAW);
	
	glUseProgram(shaderProgram);
	glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)(color_offset*sizeof(float)));
	glDrawArrays(GL_LINE_STRIP, 0, vertexCount);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
	glDisable(GL_BLEND);
	glUseProgram(0);
	
	glutSwapBuffers();
}

device* SPED3Config::createDevice(cpu* host) {
    return new SPED3(host);
}
