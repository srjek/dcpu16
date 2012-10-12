#import pygame, sys, traceback
from pygame.locals import *
#import multiprocessing
from multiprocessing import Value, Array, Lock
import ctypes
import threading
import queue
from queue import Queue

class genericClock(threading.Thread):
    def __init__(self, cpu, errorQueue):
        self.register, self.ram = cpu.getInternals()
        self.cpu = cpu
        self.errorQueue = errorQueue

        self.timing = Value(ctypes.c_uint16, 0, lock=False)
        self.ticks = Value(ctypes.c_uint16, 0, lock=False)
        self.interruptMsg = Value(ctypes.c_uint16, 0, lock=True)
        self.uid = Value(ctypes.c_uint16, 0, lock=False)

        cpu.addHardware(self)
        threading.Thread.__init__(self)
        #multiprocessing.Process.__init__(self)
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (515079825, 0x12d0b402, 1)


    def tick(self, uid):
        if self.uid.value != uid:
            return
        cpu = self.cpu
        cpu.scheduleCallback(cpu.time[0]+self.timing.value, genericClock.tick, (self, uid))
        self.ticks.value += 1
        
        intMsg = self.interruptMsg.value
        if intMsg != 0:
            self.cpu.interrupt(intMsg)
        
    def interrupt(self):
        A = self.register[0]
        if A == 0:
            self.uid.value = (self.uid.value+1) & 0xFFFF
            B = self.register[1]
            timing = 0
            if B > 0:
                timing = int(100000.0/(60.0/B))
            self.timing.value = timing
            self.ticks.value = 0
            cpu = self.cpu
            cpu.scheduleCallback(cpu.time[0]+self.timing.value, genericClock.tick, (self, uid))
        elif A == 1:
            self.register[2] = self.ticks.value
        elif A == 2:
            self.interruptMsg.value = register[1]
        return 0
        
    def run(self):
        return
    def finishUp(self):
        self.join()
