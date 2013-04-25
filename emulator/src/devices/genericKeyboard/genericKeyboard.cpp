#include <iostream>
#include "genericKeyboard.h"

class genericKeyboard_state: public deviceState {
public:
    unsigned short interruptMsg;
    
    genericKeyboard_state() {
        name = "genericKeyboard";
    }
};

class genericKeyboard: public device, public keyHandler {
protected:
    cpu* host;
    device* keyboardProvider;
    volatile bool state[256];
    volatile unsigned short interruptMsg;
    
    volatile unsigned short buffer[256];
    volatile int bufferStart;
    volatile int bufferEnd;
public:
    void reset() {
        for (int i = 0; i < 256; i++)
            state[i] = false;
        interruptMsg = 0;
        
        bufferStart = 0;
        bufferEnd = 0;
    }
    genericKeyboard(cpu* host, device* keyboardProvider=NULL) {
        this->host = host;
        this->keyboardProvider = keyboardProvider;
        host->addHardware(this);
        
        reset();
        if (keyboardProvider == NULL)
            std::cout << "Warning: Keyboard has no display to attach to. Keyboard will still connect to dcpu16, but will be disabled." << std::endl;
        else
            keyboardProvider->registerKeyHandler(this);
    }
    ~genericKeyboard() { }
    void createWindow() { }
    unsigned long getManufacturer() {
        return 515079825;  //????
    }
    unsigned long getId() {
        return 0x30cf7406;  //Generic Keyboard
    }
    unsigned long getVersion() {
        return 4919;  //????
    }
    
    int interrupt() {
        unsigned short A = host->registers[0];
        if (A == 0)
            bufferStart = bufferEnd;
        else if (A == 1) {
            if (bufferStart == bufferEnd)
                host->registers[2] = 0;
            else {
                host->registers[2] = buffer[bufferStart];
                bufferStart = (bufferStart + 1) & 0xFF;
            }
        } else if (A == 2)
            if (state[host->registers[1] && 0xFF])
                host->registers[2] = 1;
            else
                host->registers[2] = 0;
        else if (A == 3)
            interruptMsg = host->registers[1];
        return 0;
    }
    void registerKeyHandler(keyHandler* handler) { };
    void handleKey(unsigned short keyCode, bool isDown) {
        if (state[keyCode] == isDown)
            return;
        state[keyCode] = isDown;
        if (interruptMsg != 0)
            host->interrupt(interruptMsg);
        if (!isDown) {
            buffer[bufferEnd] = keyCode;
            bufferEnd = (bufferEnd + 1) & 0xFF;
        }
    }
    void OnKeyEvent(int keyCode, bool isDown) {
        int actualKeyCode = -1;
        if ((keyCode >= 'A') && (keyCode <= 'Z'))
            keyCode += 'a' - 'A';
        if ((keyCode >= 0x20) && (keyCode <= 0x7f)) {
            actualKeyCode = keyCode;
            if (keyCode == WXK_DELETE) {  //Weird, but the delete key has 2 codes in the spec as time of writing
                handleKey(0x13, isDown);
            }
        } else {
            switch (keyCode) {
                case WXK_BACK:
                    actualKeyCode = 0x10;
                    break;
                case WXK_TAB:
                    actualKeyCode = -1; //No tab at time of coding :(
                    break;
                case WXK_RETURN:
                    actualKeyCode = 0x11;
                    break;
                case WXK_INSERT:
                    actualKeyCode = 0x12;
                    break;
                case WXK_DELETE:
                    actualKeyCode = 0x13;
                    break;
                case WXK_UP:
                    actualKeyCode = 0x80;
                    break;
                case WXK_DOWN:
                    actualKeyCode = 0x81;
                    break;
                case WXK_LEFT:
                    actualKeyCode = 0x82;
                    break;
                case WXK_RIGHT:
                    actualKeyCode = 0x83;
                    break;
                case WXK_SHIFT:
                    actualKeyCode = 0x90;
                    break;
                case WXK_CONTROL:
                    actualKeyCode = 0x91;
                    break;
            }
        }
        if (actualKeyCode != -1)
            handleKey(actualKeyCode, isDown);
    }
    
    deviceState* saveState() {
        genericKeyboard_state* result = new genericKeyboard_state();
        
        result->interruptMsg = interruptMsg;
        
        return result;
    }
    void restoreState(deviceState* state_in) {
        if (strcmp(state_in->name, "genericKeyboard") != 0) {
            std::cerr << "A genericKeyboard was given a state for a " << state_in->name << ", unable to recover previous state. Overall system state may be inconsisent." << std::endl;
            return;
        }
        genericKeyboard_state* state = (genericKeyboard_state*) state_in;
        
        interruptMsg = state->interruptMsg;
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

device* genericKeyboardConfig::createDevice(cpu* host, device* keyboardProvider) {
    return new genericKeyboard(host, keyboardProvider);
}
