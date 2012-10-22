#include <iostream>
#include <cstdint>
#include <wx/dcbuffer.h>
#include "LEM1802.h"

class LEM1802;

class LEM1802DisplayPanel: public wxPanel {
protected:
    LEM1802* device;
public:
    LEM1802DisplayPanel(wxWindow* parent, const wxSize& size, LEM1802* device): wxPanel(parent, wxID_ANY, wxPoint(0, 0), size) {
        this->device = device;
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    }
    void paintEvent(wxPaintEvent& evt) {
        wxBufferedPaintDC dc(this);
        render(dc);
    }
    void render(wxDC& dc_in);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(LEM1802DisplayPanel, wxPanel)
    EVT_PAINT(LEM1802DisplayPanel::paintEvent)
END_EVENT_TABLE()
class LEM1802Display: public wxFrame {
protected:
    LEM1802* device;
public:
    LEM1802Display(wxWindow* parent, const wxPoint& pos, const wxSize& size, LEM1802* device): wxFrame(parent, -1, _("LEM1802"), pos, size) {
        this->device = device;
        new LEM1802DisplayPanel(this, size, device);
    }
    void OnClose(wxCloseEvent& event);
    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(LEM1802Display, wxFrame)
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
    
    static const int w = 32;
    static const int pw = 4;
    static const int h = 12;
    static const int ph = 8;
    static const int border = 16;
    static const int scale = 3;
    static const int width = (w*pw+border*2)*scale;
    static const int height = (h*ph+border*2)*scale;
public:
    LEM1802(cpu* host) {
        this->host = host;
        host->addHardware(this);
        
        mapAddress = 0;
        tileAddress = 0;
        paletteAddress = 0;
        borderColor = 0;
    }
    ~LEM1802() {
        if (display)
            display->Close(true);
    }
    virtual void createWindow() {
        display = new LEM1802Display(host->getWindow(), wxPoint(50, 50), wxSize(width, height), this);
        display->Show(true);
    }
    virtual unsigned long getManufacturer() {
        return 0x1c6c8b36;  //Nya Elektriska
    }
    virtual unsigned long getId() {
        return 0x7349f615;  //LEM180X series
    }
    virtual unsigned long getVersion() {
        return 0x1802;  //1802
    }
    
    wxThreadError Create() { return wxTHREAD_NO_ERROR; }
    wxThreadError Run() { return wxTHREAD_NO_ERROR; }
    void Stop() { }
    wxThread::ExitCode Wait() { return 0; }
};


void LEM1802DisplayPanel::render(wxDC& dc) {
    int blink = 0;
    unsigned short* ram = device->host->ram;
    wxImage image(device->width, device->height, false);
    
    uint32_t* display = (uint32_t*) image.GetData();
    
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
    unsigned short* fontRom;
    int ramOffset;
    if (device->tileAddress != 0) {       //Get the tiles
        fontRom = ram;
        ramOffset = device->tileAddress;
    } else {
        fontRom = ram;//fontRomBuf.buf;
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
        for (int y = 0; y < 16; y++)
            display[(y*lineWidth) + x] = borderColor;
        for (int y = 16+(12*8); y < 32+(12*8); y++)
            display[(y*lineWidth) + x] = borderColor;
    }
    for (int y = 16; y < 16+(12*8); y++) {
        for (int x = 0; x < 16; x++)
            display[(y*lineWidth) + x] = borderColor;
        for (int x = lineWidth-16; x < lineWidth; x++)
            display[(y*lineWidth) + x] = borderColor;
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
                    if (tile[i][cx][cy])
                        display[offset] = fg;
                    else
                        display[offset] = bg;
                }
            }
        }
    }
    
    wxBitmap bitmap(image, -1);
    dc.DrawBitmap(bitmap, 0, 0, false);
}
    
void LEM1802Display::OnClose(wxCloseEvent& event) {
    device->display = 0;    //wxwidgets will just drop the window out of memory itself, thus causing a crash unless if there's some warning
    Destroy();
}

device* LEM1802Config::createDevice(cpu* host) {
    return new LEM1802(host);
}