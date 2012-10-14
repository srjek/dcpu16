#ifdef _MSC_VER
#include "stdint.h"
#else
#include <stdint.h>
#endif
#include <stdlib.h>

PyObject* render(PyObject* pyRam, PyObject* pyDisplay, int mapAddress, int tileAddress, PyObject* pyFontRom, int paletteAddress, int borderColorIndex, int blink)
{
    Py_buffer ramBuf;
    Py_buffer displayBuf;
    Py_buffer fontRomBuf;
    #ifdef _MSC_VER //FUCK YOU (Not you, this FUCKING "C COMPILER")
    uint16_t* ram;
    uint32_t* display;
    uint16_t* fontRom;
    uint32_t* palette;
    int b, g, r;
    int* tile = (int*) malloc(128*8*4);
    int ramOffset;
    int lineWidth, xOffset, yOffset;
    uint32_t borderColor;
    uint16_t value;
    uint32_t fg, bg;
    int almostOffset, offset;
    int cx, cy;
    #endif
    int i, x, y; //Loop counters
    
    if ( PyObject_GetBuffer(pyRam, &ramBuf, PyBUF_SIMPLE) == -1)
        return NULL;
    if ( PyObject_GetBuffer(pyDisplay, &displayBuf, PyBUF_WRITABLE) == -1) {
        PyBuffer_Release(&ramBuf);
        return NULL;
    }
    if ( PyObject_GetBuffer(pyFontRom, &fontRomBuf, PyBUF_SIMPLE) == -1) {
        PyBuffer_Release(&ramBuf);
        PyBuffer_Release(&displayBuf);
        return NULL;
    }
    
    #ifndef _MSC_VER    //The right way
    uint16_t* ram = (uint16_t*) ramBuf.buf;
    uint32_t* display = (uint32_t*) displayBuf.buf;
    
    uint32_t* palette = (uint32_t*) malloc(4*16);
    #else       //The horrible compiler's way (with the varibles all the way up there)
    ram = (uint16_t*) ramBuf.buf;
    display = (uint32_t*) displayBuf.buf;
    palette = malloc(4*16);
    #endif
    
    if (paletteAddress == 0) {          //Get the palette
        for (i = 0; i < 16; i++) {
        #ifndef _MSC_VER
            int b = ((i >> 0) & 0x1) * 170;
            int g = ((i >> 1) & 0x1) * 170;
            int r = ((i >> 2) & 0x1) * 170;
        #else
            b = ((i >> 0) & 0x1) * 170;
            g = ((i >> 1) & 0x1) * 170;
            r = ((i >> 2) & 0x1) * 170;
        #endif
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
        for (i = 0; i < 16; i++) {
        #ifndef _MSC_VER
            int value = ram[paletteAddress+i];
            int b = value & 0x000F;
            int g = (value & 0x00F0) >> 4;
            int r = (value & 0x0F00) >> 8;
        #else
            value = ram[paletteAddress+i];
            b = value & 0x000F;
            g = (value & 0x00F0) >> 4;
            r = (value & 0x0F00) >> 8;
        #endif
            r *= 17; g *= 17; b *= 17;
            palette[i] = (r << 16) | (g << 8) | b ;
        }
    }
    
    #ifndef _MSC_VER
    int tile[128][4][8] = {0};
    uint16_t* fontRom;
    #endif
    if (tileAddress != 0)       //Get the tiles
        fontRom = (uint16_t*) (ram + tileAddress);
    else
        fontRom = (uint16_t*) fontRomBuf.buf;
    
    #ifndef _MSC_VER
    int ramOffset = 0;
    for (i = 0; i < 128; i++) {
        for (x = 0; x < 4; x += 2) {
            for (y = 7; y >= 0; y--) {
                tile[i][x][y] = (fontRom[ramOffset] >> 8) & (1 << y);
                tile[i][x+1][y] = fontRom[ramOffset] & (1 << y);
    #else
    ramOffset = 0;
    for (i = 0; i < 128; i++) {
        for (x = 0; x < 4; x += 2) {
            for (y = 7; y >= 0; y--) {
                tile[i*(8*4) + x*8 + y] = (fontRom[ramOffset] >> 8) & (1 << y);
                tile[i*(8*4) + (x+1)*8 + y] = fontRom[ramOffset] & (1 << y);
    #endif
            }
            ramOffset++;
        }
    }
    
    //RENDER TIME! (Also, reads the map)
    #ifndef _MSC_VER
    int lineWidth = (4*32+32);
    uint32_t borderColor = palette[borderColorIndex];
    
    for (x = 0; x < lineWidth; x++) {
        for (y = 0; y < 16; y++)
            display[(y*lineWidth) + x] = borderColor;
        for (y = 16+(12*8); y < 32+(12*8); y++)
            display[(y*lineWidth) + x] = borderColor;
    }
    for (y = 16; y < 16+(12*8); y++) {
        for (x = 0; x < 16; x++)
            display[(y*lineWidth) + x] = borderColor;
        for (x = lineWidth-16; x < lineWidth; x++)
            display[(y*lineWidth) + x] = borderColor;
    }
    
    for (x = 0; x < 32; x++) {
        int xOffset = 16+(x*4);
        for (y = 0; y < 12; y++) {
            int yOffset = 16+(y*8);
            uint16_t value = ram[mapAddress+(y*32+x)];
            uint16_t i = value & 0x7F;
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
    #else
    lineWidth = (4*32+32);
    borderColor = palette[borderColorIndex];
    
    for (x = 0; x < lineWidth; x++) {
        for (y = 0; y < 16; y++)
            display[(y*lineWidth) + x] = borderColor;
        for (y = 16+(12*8); y < 32+(12*8); y++)
            display[(y*lineWidth) + x] = borderColor;
    }
    for (y = 16; y < 16+(12*8); y++) {
        for (x = 0; x < 16; x++)
            display[(y*lineWidth) + x] = borderColor;
        for (x = lineWidth-16; x < lineWidth; x++)
            display[(y*lineWidth) + x] = borderColor;
    }
    
    for (x = 0; x < 32; x++) {
        xOffset = 16+(x*4);
        for (y = 0; y < 12; y++) {
            yOffset = 16+(y*8);
            value = ram[mapAddress+(y*32+x)];
            i = value & 0x7F;
            fg = palette[(value >> 12) & 0x0F];
            bg = palette[(value >> 8) & 0x0F];
            if (((value >> 7) & 0x1) == 1 && blink)
                fg = bg;
            
            //if ((value >> 7) & 0x1) == 1 and blinkTime >= 60:
            //    fg = bg
            
            for (cy = 0; cy < 8; cy++) {                //Render the tile
                almostOffset = (yOffset+cy)*lineWidth;
                for (cx = 0; cx < 4; cx++) {
                    offset = almostOffset + (xOffset+cx);
                    if (tile[i*(8*4) + cx*8 + cy])
                        display[offset] = fg;
                    else
                        display[offset] = bg;
                }
            }
        }
    }
    #endif
    #ifdef _MSC_VER
    free(tile);
    #endif
    free(palette);
    PyBuffer_Release(&fontRomBuf);
    PyBuffer_Release(&displayBuf);
    PyBuffer_Release(&ramBuf);
    
    Py_INCREF(Py_None);
    return Py_None;
}
