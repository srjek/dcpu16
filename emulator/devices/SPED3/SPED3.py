import pygame
from pygame.locals import *
from OpenGL.GL import *
import sys, traceback
import multiprocessing
from multiprocessing import Value, Array, Lock
import ctypes
#import threading
import queue
from queue import Queue

class SPED3(multiprocessing.Process):
    needGui = False
    needMonitor = False
    isMonitor = False
    
    STATE_NO_DATA = 0x0000
    STATE_RUNNING = 0x0001
    STATE_TURNING = 0x0002
    
    ERROR_NONE =    0x0000
    ERROR_BROKEN =  0xFFFF
    
    def __init__(self, cpu, errorQueue):
        self.register, self.ram = cpu.getInternals()
        self.errorQueue = errorQueue
        self.displayReadyAt = 0
        
        self.state = Value(ctypes.c_uint16, SPED3.STATE_NO_DATA, lock=False)
        self.error = Value(ctypes.c_uint16, SPED3.ERROR_NONE, lock=False)
        self.stateLock = Lock()
        self.vertexAddress = Value("i", 0, lock=False)
        self.numVertex = Value("i", 0, lock=False)
        self.rotation = Value("i", 0, lock=False)
        
        self.running = Value("i", 1, lock=False)
        
        cpu.addHardware(self)
        multiprocessing.Process.__init__(self)
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (0x1eb37e91, 0x42babf3c, 0x003)
    
    def interrupt(self):
        A = self.register[0]
        if A == 0:
            self.stateLock.acquire()
            self.register[1] = self.state.value
            self.register[2] = self.error.value
            self.error.value = SPED3.ERROR_NONE
            self.stateLock.release()
        elif A == 1:
            self.vertexAddress.value = self.register[3]
            self.numVertex.value = self.register[4]
        elif A == 2:
            self.rotation.value = self.register[3] % 360
        return 0

    def finishUp(self):
        self.running.value = 0
        self.join()
    def run(self):
        return
        try:
            import sys, dummyFile
            sys.stderr = dummyFile.dummyFile()
            sys.stdout = dummyFile.dummyFile()
            
            pygame.init()
            clock = pygame.time.Clock()

            display = pygame.display.set_mode((800, 800), pygame.OPENGL | pygame.DOUBLEBUF)
            pygame.display.set_caption("SPED-3")
            glClearColor(0.0, 0.0, 1.0, 1.0)
            
            blinkTime = 0
            ram = self.ram
            while self.running.value == 1:
                vertexAddress = self.vertexAddress.value
                numVertex = self.numVertex.value
                rotation = self.rotation.value
                
                ticks = pygame.time.get_ticks()
                
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)
                glBegin(GL_TRIANGLES)
                glVertex3f(-0.5,0.0,0.0)
                glVertex3f(0.5,0.0,0.0)
                glVertex3f(0.5,0.5,0.0)
                glEnd()
                pygame.display.flip()
                
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
                            handler.put((event.type, event.key))
                #extDisplay.blit(pygame.transform.scale2x(pygame.transform.scale2x(display)), (0, 0))
                #pygame.transform.scale(display, extSize, extDisplay)
                #pygame.display.update()

                blinkTime += 1
                if blinkTime >= 240:
                    blinkTime = 0
                clock.tick(120)
                #if blinkTime == 120:
                #    self.errorQueue.put((repr(clock.get_fps()),))
            pygame.quit()
        except Exception as e:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            self.errorQueue.put(traceback.format_exc().splitlines())
        self.errorQueue.put(sys.stdout.buffer.split("\n"))
        self.errorQueue.put(sys.stderr.buffer.split("\n"))
    
