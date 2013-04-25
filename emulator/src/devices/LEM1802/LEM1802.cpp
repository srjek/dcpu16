#include <iostream>
#include <vector>
#include <cstdint>

#include <wx/filename.h>
#include <wx/dcbuffer.h>
#include <wx/stdpaths.h>

#include "LEM1802.h"

class LEM1802_state: public deviceState {
public:
    unsigned short mapAddress;
    unsigned short tileAddress;
    unsigned short paletteAddress;
    unsigned short borderColor;
    
    LEM1802_state() {
        name = "LEM1802";
    }
};

class RenderTimer : public wxTimer {
    wxPanel* panel;
public:
    RenderTimer(wxPanel* panel) : wxTimer() {
        this->panel = panel;
    }
    void Notify() {
        panel->Refresh();
        panel->Update();
    }
    void start() {
        wxTimer::Start(1000/60);
    }
};
 
class LEM1802;

class LEM1802DisplayPanel: public wxPanel {
    friend class RenderTimer;
protected:
    LEM1802* device;
    RenderTimer timer;
    wxLongLong displayReadyAt;
    unsigned short lastMapAddress;
    wxBitmap bootBitmap;
public:
    LEM1802DisplayPanel(wxWindow* parent, const wxSize& size, LEM1802* device);
    void paintEvent(wxPaintEvent& evt) {
        wxBufferedPaintDC dc(this);
        render(dc);
    }
    void render(wxDC& dc_in);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(LEM1802DisplayPanel, wxPanel)
    EVT_KEY_DOWN(LEM1802DisplayPanel::OnKeyDown)
    EVT_KEY_UP(LEM1802DisplayPanel::OnKeyUp)
    EVT_PAINT(LEM1802DisplayPanel::paintEvent)
END_EVENT_TABLE()

class LEM1802Display: public wxFrame {
protected:
    LEM1802* device;
public:
    LEM1802Display(wxWindow* parent, const wxPoint& pos, const wxSize& size, LEM1802* device): wxFrame(parent, -1, _("LEM1802"), pos, wxSize(-1, -1)) {
        this->device = device;
        SetClientSize(size);
        new LEM1802DisplayPanel(this, size, device);
    }
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnClose(wxCloseEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(LEM1802Display, wxFrame)
    EVT_KEY_DOWN(LEM1802Display::OnKeyDown)
    EVT_KEY_UP(LEM1802Display::OnKeyUp)
    EVT_CLOSE(LEM1802Display::OnClose)
END_EVENT_TABLE()


class LEM1802: public device {
    friend class LEM1802Display;
    friend class LEM1802DisplayPanel;
protected:
    cpu* host;
    LEM1802Display* display;
    volatile unsigned short mapAddress;
    volatile unsigned short tileAddress;
    volatile unsigned short paletteAddress;
    volatile unsigned short borderColor;
    
    unsigned short fontRom[128*2];
    
    static const int w = 32;
    static const int pw = 4;
    static const int h = 12;
    static const int ph = 8;
    static const int border = 16;
    static const int scale = 3;
    static const int prescaleWidth = w*pw+border*2;
    static const int prescaleHeight = h*ph+border*2;
    static const int width = prescaleWidth*scale;
    static const int height = prescaleHeight*scale;
    
    std::vector<keyHandler*> keyHandlers;
    
public:
    void reset() {
        mapAddress = 0;
        tileAddress = 0;
        paletteAddress = 0;
        borderColor = 0;
    }
    LEM1802(cpu* host) {
        this->host = host;
        host->addHardware(this);
        
        reset();
        
        //Load font rom
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxFileName filename;
        filename.Assign(stdpath.GetExecutablePath());
        filename.AppendDir(_("devices"));
        filename.AppendDir(_("LEM1802"));
        filename.SetFullName(_("font.png"));
        if (!filename.FileExists()) {
            std::cout << "ERROR: File \"";
            std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
            std::cout << "\" does not exist" << std::endl;
            return;
        }
        if (!filename.IsFileReadable()) {
            std::cout << "ERROR: File \"";
            std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
            std::cout << "\" is not readable" << std::endl;
            return;
        }
        
        wxInitAllImageHandlers();
        wxImage fontImg(filename.GetFullPath(), wxBITMAP_TYPE_PNG);
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 32; x++) {
                int xOffset = x*4;
                int yOffset = y*8;
                for (int cx = 0; cx < 2; cx++) {
                    unsigned short charByte0 = 0;
                    unsigned short charByte1 = 0;
                    for (int cy = 0; cy < 8; cy++) {
                        if ((fontImg.GetRed(xOffset+cx*2, yOffset+cy) & 0xFF) > 128)
                            charByte0 |= (1 << cy);
                        if ((fontImg.GetRed(xOffset+cx*2+1, yOffset+cy) & 0xFF) > 128)
                            charByte1 |= (1 << cy);
                    }
                    fontRom[(y*32+x)*2+cx] = (charByte0 << 8) | charByte1;
                }
            }
        }
    }
    ~LEM1802() {
        if (display)
            display->Close(true);
    }
    void createWindow() {
        display = new LEM1802Display(host->getWindow(), wxPoint(50, 50), wxSize(width, height), this);
        display->Show(true);
    }
    unsigned long getManufacturer() {
        return 0x1c6c8b36;  //Nya Elektriska
    }
    unsigned long getId() {
        return 0x7349f615;  //LEM180X series
    }
    unsigned long getVersion() {
        return 0x1802;  //1802
    }
    
    int interrupt() {
        unsigned short A = host->registers[0];
        if (A == 0)
            mapAddress = host->registers[1];
        else if (A == 1)
            tileAddress = host->registers[1];
        else if (A == 2)
            paletteAddress = host->registers[1];
        else if (A == 3)
            borderColor = host->registers[1];
        else if (A == 4) {
            unsigned short address = host->registers[1];
            volatile unsigned short* ram = host->ram;
            for (int i = 0; i < 128*2; i++)
                ram[(address+i)&0xFFFF] = fontRom[i];
            return 256;
        } else if (A == 5) {
            unsigned short address = host->registers[1];
            volatile unsigned short* ram = host->ram;
            for (int i = 0; i < 16; i++) {
                int b = ((i >> 0) & 0x1) * 10;
                int g = ((i >> 1) & 0x1) * 10;
                int r = ((i >> 2) & 0x1) * 10;
                if (i == 6)
                    g -= 5;
                else if (i >= 8) {
                    r += 5;
                    g += 5;
                    b += 5;
                }
                ram[(address+i)&0xFFFF] = (r << 8 | g << 4 | b) & 0xFFFF;
            }
            return 16;
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
    
    deviceState* saveState() {
        LEM1802_state* result = new LEM1802_state();
        
        result->mapAddress = mapAddress;
        result->tileAddress = tileAddress;
        result->paletteAddress = paletteAddress;
        result->borderColor = borderColor;
        
        return result;
    }
    void restoreState(deviceState* state_in) {
        if (strcmp(state_in->name, "LEM1802") != 0) {
            std::cerr << "A LEM1802 was given a state for a " << state_in->name << ", unable to recover previous state. Overall system state may be inconsisent." << std::endl;
            return;
        }
        LEM1802_state* state = (LEM1802_state*) state_in;
        
        mapAddress = state->mapAddress;
        tileAddress = state->tileAddress;
        paletteAddress = state->paletteAddress;
        borderColor = state->borderColor;
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};

LEM1802DisplayPanel::LEM1802DisplayPanel(wxWindow* parent, const wxSize& size, LEM1802* device):
                wxPanel(parent, wxID_ANY, wxPoint(0, 0), size, wxWANTS_CHARS), timer(this) {
    this->device = device;
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    lastMapAddress = 0;
    timer.start();
    
    
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    wxFileName filename;
    filename.Assign(stdpath.GetExecutablePath());
    filename.AppendDir(_("devices"));
    filename.AppendDir(_("LEM1802"));
    filename.SetFullName(_("boot.png"));
    if (!filename.FileExists()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" does not exist" << std::endl;
        return;
    }
    if (!filename.IsFileReadable()) {
        std::cout << "ERROR: File \"";
        std::cout << filename.GetFullPath().mb_str(wxConvUTF8);
        std::cout << "\" is not readable" << std::endl;
        return;
    }
    
    wxInitAllImageHandlers();
    wxImage bootImage(filename.GetFullPath(), wxBITMAP_TYPE_PNG);
    bootImage.Rescale(device->w*device->pw*device->scale, device->h*device->ph*device->scale);
    bootBitmap = wxBitmap(bootImage, -1);
}
void LEM1802DisplayPanel::render(wxDC& dc) {
    wxLongLong time = wxGetLocalTimeMillis();
    bool blink = (((time/16)/20) % 2) == 0;
    volatile unsigned short* ram = device->host->ram;
    
    if ((lastMapAddress == 0) && (device->mapAddress != lastMapAddress))
        displayReadyAt = time + 1000;
    lastMapAddress = device->mapAddress;
    if ((device->mapAddress == 0) || (time < displayReadyAt)) {
        dc.SetBackground(wxBrush(wxColour(0x00, 0x00, 0xAA)));
        dc.Clear();
        dc.DrawBitmap(bootBitmap, device->border*device->scale, device->border*device->scale, false);
        return;
    }
    
    wxImage image(device->prescaleWidth, device->prescaleHeight, false);
    unsigned char* display = image.GetData();
    
    uint32_t palette[4*16];
    if (device->paletteAddress == 0) {          //Get the palette
        for (int i = 0; i < 16; i++) {
            int b = ((i >> 0) & 0x1) * 170;
            int g = ((i >> 1) & 0x1) * 170;
            int r = ((i >> 2) & 0x1) * 170;
            if (i == 6)
                g -= 85;
            else if (i >= 8) {
                r += 85;
                g += 85;
                b += 85;
            }
            palette[i] = (r << 16) | (g << 8) | b ;
        }
    } else {
        for (int i = 0; i < 16; i++) {
            unsigned short value = ram[(device->paletteAddress+i)&0xFFFF];
            int b = value & 0x000F;
            int g = (value & 0x00F0) >> 4;
            int r = (value & 0x0F00) >> 8;
            r *= 17; g *= 17; b *= 17;
            palette[i] = (r << 16) | (g << 8) | b ;
        }
    }
    int tile[128][4][8] = {0};
    volatile unsigned short* fontRom;
    unsigned int ramOffset;
    if (device->tileAddress != 0) {       //Get the tiles
        fontRom = ram;
        ramOffset = device->tileAddress;
    } else {
        fontRom = device->fontRom;
        ramOffset = 0;
    }
    for (int i = 0; i < 128; i++) {
        for (int x = 0; x < 4; x += 2) {
            for (int y = 7; y >= 0; y--) {
                tile[i][x][y] = (fontRom[ramOffset] >> 8) & (1 << y);
                tile[i][x+1][y] = fontRom[ramOffset] & (1 << y);
            }
            ramOffset = (ramOffset + 1) & 0xFFFF;
        }
    }
    
    
    //RENDER TIME! (Also, reads the map)
    int lineWidth = (4*32+32);
    uint32_t borderColor = palette[device->borderColor];
    
    for (int x = 0; x < lineWidth; x++) {
        for (int y = 0; y < 16; y++) {
            int offset = (y*lineWidth) + x;
            display[offset*3] = (borderColor >> 16) & 0xFF;
            display[offset*3+1] = (borderColor >> 8) & 0xFF;
            display[offset*3+2] = borderColor & 0xFF;
        }
        for (int y = 16+(12*8); y < 32+(12*8); y++) {
            int offset = (y*lineWidth) + x;
            display[offset*3] = (borderColor >> 16) & 0xFF;
            display[offset*3+1] = (borderColor >> 8) & 0xFF;
            display[offset*3+2] = borderColor & 0xFF;
        }
    }
    for (int y = 16; y < 16+(12*8); y++) {
        for (int x = 0; x < 16; x++) {
            int offset = (y*lineWidth) + x;
            display[offset*3] = (borderColor >> 16) & 0xFF;
            display[offset*3+1] = (borderColor >> 8) & 0xFF;
            display[offset*3+2] = borderColor & 0xFF;
        }
        for (int x = lineWidth-16; x < lineWidth; x++) {
            int offset = (y*lineWidth) + x;
            display[offset*3] = (borderColor >> 16) & 0xFF;
            display[offset*3+1] = (borderColor >> 8) & 0xFF;
            display[offset*3+2] = borderColor & 0xFF;
        }
    }
    
    for (int x = 0; x < 32; x++) {
        int xOffset = 16+(x*4);
        for (int y = 0; y < 12; y++) {
            int yOffset = 16+(y*8);
            unsigned short value = ram[device->mapAddress+(y*32+x)];
            int i = value & 0x7F;
            uint32_t fg = palette[(value >> 12) & 0x0F];
            uint32_t bg = palette[(value >> 8) & 0x0F];
            if (((value >> 7) & 0x1) == 1 && blink)
                fg = bg;
            
            //if ((value >> 7) & 0x1) == 1 and blinkTime >= 60:
            //    fg = bg
            
            int cx, cy;
            for (cy = 0; cy < 8; cy++) {            //Render the tile
                int almostOffset = (yOffset+cy)*lineWidth;
                for (cx = 0; cx < 4; cx++) {
                    int offset = almostOffset + (xOffset+cx);
                    if (tile[i][cx][cy]) {
                        display[offset*3] = (fg >> 16) & 0xFF;
                        display[offset*3+1] = (fg >> 8) & 0xFF;
                        display[offset*3+2] = fg & 0xFF;
                    } else {
                        display[offset*3] = (bg >> 16) & 0xFF;
                        display[offset*3+1] = (bg >> 8) & 0xFF;
                        display[offset*3+2] = bg & 0xFF;
                    }
                }
            }
        }
    }
    
    image.Rescale(device->width, device->height);
    wxBitmap bitmap(image, -1);
    dc.DrawBitmap(bitmap, 0, 0, false);
}
 
void LEM1802DisplayPanel::OnKeyDown(wxKeyEvent& event) {
    device->OnKeyDown(event);
}
void LEM1802DisplayPanel::OnKeyUp(wxKeyEvent& event) {
    device->OnKeyUp(event);
}
void LEM1802Display::OnKeyDown(wxKeyEvent& event) {
//    device->OnKeyDown(event);
}
void LEM1802Display::OnKeyUp(wxKeyEvent& event) {
//    device->OnKeyUp(event);
}

void LEM1802Display::OnClose(wxCloseEvent& event) {
    device->display = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}

device* LEM1802Config::createDevice(cpu* host) {
    return new LEM1802(host);
}
