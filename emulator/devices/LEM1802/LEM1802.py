import pygame, sys, os, traceback
from pygame.locals import *
import multiprocessing
from multiprocessing import Value, Array
import ctypes
#import threading
import queue
from queue import Queue
LEM1802_Root = ""
if __name__ == '__main__':
    LEM1802_Root = os.path.dirname(os.path.realpath(sys.argv[0]))
else:
    LEM1802_Root = os.path.dirname(os.path.realpath(__file__))
LEM1802_Root = os.path.abspath(LEM1802_Root)

class LEM1802(multiprocessing.Process):
    needGui = False
    needMonitor = False
    isMonitor = True
    
    def __init__(self, cpu, errorQueue):
        self.register, self.ram = cpu.getInternals()
        self.errorQueue = errorQueue
        self.displayReadyAt = 0
        self.mapAddress = Value("i", 0, lock=False)
        self.tileAddress = Value("i", 0, lock=False)
        self.paletteAddress = Value("i", 0, lock=False)
        self.borderColor = Value("i", 0, lock=False)
        self.running = Value("i", 1, lock=False)
        self.keyHandlers = []
        cpu.addHardware(self)
        multiprocessing.Process.__init__(self)
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (0x1c6c8b36, 0x7349f615, 0x1802)
    def start(self):
        tmp = os.getcwd()
        os.chdir(LEM1802_Root)
        multiprocessing.Process.start(self)
        os.chdir(tmp)
        
    def interrupt(self):
        A = self.register[0]
        if A == 0:
            self.mapAddress.value = self.register[1]
        elif A == 1:
            self.tileAddress.value = self.register[1]
        elif A == 2:
            self.paletteAddress.value = self.register[1]
        elif A == 3:
            self.borderColor.value = self.register[1]
        elif A == 4:
            fontRom = LEM1802.getDefaultFont()
            address = self.register[1]
            ram = self.ram
            for i in range(128*2):
                ram[address+i] = fontRom[i]
            return 256
        elif A == 5:
            address = self.register[1]
            i = 0
            for b, g, r, a in LEM1802.getDefaultPalette():
                r = int(r/17)
                g = int(g/17)
                b = int(b/17)
                self.ram[address+i] = (r << 8 | g << 4 | b) & 0xFFFF
                i += 1
            return 16
        return 0

    def getDefaultFont():
        import pygame
        fontImg = pygame.image.load(os.path.join(LEM1802_Root, "font.png"))
        fontRom = []
        for y in range(4):
            for x in range(32):
                charImg = pygame.Surface((4,8))
                charImg.blit(fontImg, (0, 0), (x*4, y*8, x*4+4, y*8+8))
                charMap = pygame.PixelArray(charImg)
                for cx in range(2):
                    charByte0 = 0
                    for cy in range(8):
                        if charImg.unmap_rgb(charMap[cx*2][cy]).r & 0xFF > 128:
                            charByte0 |= (1 << cy)
                    charByte1 = 0
                    for cy in range(8):
                        if charImg.unmap_rgb(charMap[cx*2+1][cy]).r & 0xFF > 128:
                            charByte1 |= (1 << cy)
                    fontRom.append((charByte0 << 8) | charByte1)
                del charMap
        return tuple(fontRom)
    def getDefaultPalette():
        defaultPalette = []
        for i in range(16):
            b = ((i >> 0) & 0x1) * 170
            g = ((i >> 1) & 0x1) * 170
            r = ((i >> 2) & 0x1) * 170
            if i == 6:
                g -= 85
            elif i >= 8:
                r += 85
                g += 85
                b += 85
            defaultPalette.append(bytes((b, g, r, 0)))
            #defaultPalette.append(r << 16 | g << 8 | b)
        return defaultPalette

    def registerKeyHandler(self, handler):
        self.keyHandlers.append(handler)
    def finishUp(self):
        self.running.value = 0
    def run(self):
        try:
            from PyInline import C
            import PyInline, sys, dummyFile
            sys.stderr = dummyFile.queueFile(self.errorQueue) #sys.stderr.errors = 'unknown'
            sys.stdout = dummyFile.dummyFile() #sys.stdout.errors = 'unknown'
            m = PyInline.build(code="""
                  PyObject* render(PyObject* ram, PyObject* display, int mapAddress, int tileAddress, PyObject* fontRom, int paletteAddress, int borderColor, int blink);
                  #include "../../renderer.c"
                  """,
                  language="C")
            
            pygame.init()
            clock = pygame.time.Clock()
    
            w = 32
            pw = 4
            h = 12
            ph = 8
            scale = 3
            extSize = ((w*pw+16*2)*scale, (h*ph+16*2)*scale)
            extDisplay = pygame.display.set_mode(extSize)
            pygame.display.set_caption("LEM1802")
            display = pygame.Surface((w*pw+16*2, h*ph+16*2))
            
            bootImg = pygame.image.load("boot.png")
            fontRom = bytes(Array(ctypes.c_uint16, LEM1802.getDefaultFont(), lock=False))
            #defaultPalette = LEM1802.getDefaultPalette()
            
            lastMapAddress = 0
            blink = 0
            #blinkTime = 0
            ram = self.ram
            while self.running.value == 1:
                mapAddress = self.mapAddress.value
                tileAddress = self.tileAddress.value
                paletteAddress = self.paletteAddress.value
                borderColor = self.borderColor.value
                
                ticks = pygame.time.get_ticks()
                if mapAddress != lastMapAddress and lastMapAddress == 0:
                    self.displayReadyAt = ticks + 1000
                lastMapAddress = mapAddress

                if mapAddress == 0 or ticks < self.displayReadyAt:
                    display.fill((0, 0, 0xAA))
                    display.blit(bootImg, (16, 16))
                else:
                    buffer = display.get_buffer()
                    m.render(self.ram, buffer, mapAddress, tileAddress, fontRom, paletteAddress, borderColor, blink) #blinkTime >= 120)
                    del buffer
                
                for event in pygame.event.get():
                    if event.type == QUIT:
                        self.running.value = 0
                    elif event.type == KEYDOWN:
                        for handler in self.keyHandlers:
                            handler.put((event.type, event.key))
                        if event.key == K_ESCAPE:
                            pygame.event.post(pygame.event.Event(QUIT))
                    elif event.type == KEYUP:
                        for handler in self.keyHandlers:
                            try:
                                handler.put_nowait((event.type, event.key))
                            except Queue.Full:
                                errorQueue.put("A key handler queue was full")
                #extDisplay.blit(pygame.transform.scale2x(pygame.transform.scale2x(display)), (0, 0))
                pygame.transform.scale(display, extSize, extDisplay)
                pygame.display.update()

                time = int(pygame.time.get_ticks()/16)
                blink = (int(time/20) % 2 == 0)
                #blinkTime += 1
                #if blinkTime >= 240:
                #    blinkTime = 0
                clock.tick(120)
                #if blinkTime == 120:
                #    self.errorQueue.put((repr(clock.get_fps()),))
            pygame.quit()
        except Exception as e:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            self.errorQueue.put(traceback.format_exc().splitlines())
        self.errorQueue.put(sys.stdout.buffer.split("\n"))
        self.errorQueue.put(sys.stderr.buffer.split("\n"))
    
