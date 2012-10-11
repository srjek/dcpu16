#import pygame, sys, traceback
from pygame.locals import *
#import multiprocessing
from multiprocessing import Value, Array, Manager
import ctypes
import threading
import queue
from queue import Queue

class genericKeyboard(threading.Thread):
    def __init__(self, cpu, errorQueue, monitor):
        self.register, self.ram = cpu.getInternals()
        self.cpu = cpu
        self.errorQueue = errorQueue
        self.buffer = Queue()
        self.state = Array(ctypes.c_uint16, [0]*256, lock=True)
        self.interruptMsg = Value(ctypes.c_uint16, 0, lock=True)
        self.running = Value("i", 1, lock=False)
        self.manager = Manager()
        self.eventQueue = self.manager.Queue()
        if monitor == None:
            errorQueue.put(("Keyboard: No display to attach to. Keyboard will still connect to dcpu16, but will be disabled.",))
        else:
            monitor.registerKeyHandler(self.eventQueue)
        cpu.addHardware(self)
        threading.Thread.__init__(self)
        #multiprocessing.Process.__init__(self)
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (515079825, 0x30cf7406, 4919)
    
    def interrupt(self):
        A = self.register[0]
        if A == 0:
            while True:
                try:
                    self.buffer.get_nowait()
                except queue.Empty:
                    break
        elif A == 1:
            try:
                self.register[2] = self.buffer.get_nowait()
            except queue.Empty:
                self.register[2] = 0
        elif A == 2:
            self.register[2] = self.state(A & 0xFF)
        elif A == 3:
            self.interruptMsg.value = register[1]
        return 0

    translate = {}
    def run(self):
        while self.running.value == 1:
            try:
                event = self.eventQueue.get(timeout=0.1)
                if event == None:
                    continue
                if event[1] not in genericKeyboard.translate:
                    continue
                key = genericKeyboard.translate[event[1]]
                if event[0] == KEYDOWN:
                    self.state[key] = 1
                elif event[0] == KEYUP:
                    self.state[key] = 0
                    self.buffer.put(key)
                    intMsg = self.interruptMsg.value
                    if intMsg != 0:
                        self.cpu.interrupt(intMsg)
            except queue.Empty:
                pass
    def finishUp(self):
        self.running.value = 0
        self.join()
        self.manager.shutdown()

genericKeyboard.translate[K_BACKSPACE] = 0x10
genericKeyboard.translate[K_RETURN] = 0x11
genericKeyboard.translate[K_INSERT] = 0x12
genericKeyboard.translate[K_DELETE] = 0x13
for i in range(0x20, 0x80):
    genericKeyboard.translate[i] = i
genericKeyboard.translate[K_UP] = 0x80
genericKeyboard.translate[K_DOWN] = 0x81
genericKeyboard.translate[K_LEFT] = 0x82
genericKeyboard.translate[K_RIGHT] = 0x83
genericKeyboard.translate[K_RSHIFT] = 0x84
genericKeyboard.translate[K_LSHIFT] = 0x84
genericKeyboard.translate[K_RCTRL] = 0x85
genericKeyboard.translate[K_LCTRL] = 0x85
