#import pygame, sys, traceback
from pygame.locals import *
#import multiprocessing
from multiprocessing import Value, Array, Lock
import ctypes
import threading
import queue
from queue import Queue

class M35FD(threading.Thread):
    STATE_NO_MEDIA =        0x0000
    STATE_READY =           0x0001
    STATE_READY_WP =        0x0002
    STATE_BUSY =            0xFFFF

    ERROR_NONE =            0x0000
    ERROR_BUSY =            0x0001
    ERROR_NO_MEDIA =        0x0002
    ERROR_PROTECTED =       0x0003
    ERROR_EJECT =           0x0004
    ERROR_BAD_SECTOR =      0x0005
    ERROR_BROKEN =          0xFFFF

    CMD_NONE =  0
    CMD_READ =  1
    CMD_WRITE = 2
    CMD_EJECT = 3

    def __init__(self, cpu, errorQueue, tkinterQueue, imagePath=None):
        self.register, self.ram = cpu.getInternals()
        self.cpu = cpu
        self.errorQueue = errorQueue

        self.running = Value("i", 0, lock=False)
        self.state = Value(ctypes.c_uint16, M35FD.STATE_NO_MEDIA, lock=False)
        self.error = Value(ctypes.c_uint16, M35FD.ERROR_NONE, lock=True)
        self.interruptMsg = Value(ctypes.c_uint16, 0, lock=True)

        self.cmdQueue = Queue()

        self.image = None
        self.imageLock = Lock()
        if imagePath != None:
            self.loadFileReadonly(imagePath)

        cpu.addHardware(self)
        threading.Thread.__init__(self)
        #multiprocessing.Process.__init__(self)
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (0, 0x12345678, 0)
    

    def loadDat(self, dat, readonly=False):
        self.imageLock.acquire()
        if self.state.value != M35FD.STATE_NO_MEDIA:
            self.imageLock.release()
            raise Exception("Floppy already inserted")
        image = []
        for i in range(0, len(dat), 2):
            if i+1 == len(dat):
                dat[i+1] == 0
            image.append( (dat[i] << 8) | dat[i+1] )
        if len(image) < 1440*512:
            image.extend((0,)*(1440*512-len(image)))
        self.image = image
        if readonly:
            self.changeState(M35FD.STATE_READY_WP)
        else:
            self.changeState(M35FD.STATE_READY)
        self.imageLock.release()
    def loadFileReadonly(self, filepath):
        if self.state.value != M35FD.STATE_NO_MEDIA:
            raise Exception("Floppy already inserted")
        obj = open(filepath, 'rb')
        dat = obj.read()
        obj.close()
        self.loadDat(dat, readonly=True)


    def interrupt(self):
        A = self.register[0]
        if A == 0:
            self.register[1] = self.state.value
            self.register[2] = self.error.value
            self.error.value = M35FD.ERROR_NONE
        elif A == 1:
            self.interruptMsg.Value = self.register[3]
        elif A == 2:
            responseQueue = Queue()
            self.cmdQueue.put((M35FD.CMD_READ, self.register[3], self.register[4], responseQueue))
            try:
                response = responseQueue.get(timeout=5)
            except queue.Empty:
                raise Exception("M35FD thread is not responding")
            self.register[1] = response
        elif A == 3:
            responseQueue = Queue()
            self.cmdQueue.put((M35FD.CMD_WRITE, self.register[3], self.register[4], responseQueue))
            try:
                response = responseQueue.get(timeout=5)
            except queue.Empty:
                raise Exception("M35FD thread is not responding")
            self.register[1] = response
        return 0

    def read(self, floppySector, ramOffset):
        ram = self.ram
        image = self.image
        self.imageLock.acquire()
        if self.image == None:
            self.imageLock.release()
            return
        for i in range(512):
            ram[ramOffset+i] = image[floppySector*512+i]
        self.changeState(M35FD.STATE_READY_WP)
        self.imageLock.release()
    def write(self, floppySector, ramOffset):
        ram = self.ram
        image = self.image
        self.imageLock.acquire()
        if self.image == None:
            self.imageLock.release()
            return
        for i in range(512):
            image[floppySector*512+i] = ram[(ramOffset+i)&0xFFFF]
        self.changeState(M35FD.STATE_READY_WP)
        self.imageLock.release()

    def changeError(self, error):
        self.error.value = error
        intMsg = self.interruptMsg.value
        if intMsg != 0:
            self.cpu.interrupt(intMsg)
    def changeState(self, state):
        self.state.value = state
        intMsg = self.interruptMsg.value
        if intMsg != 0:
            self.cpu.interrupt(intMsg)
    translate = {}
    def run(self):
        running = self.running
        running.value = 1
        cpu = self.cpu

        while self.running.value == 1:
            try:
                cmdStruct = self.cmdQueue.get(timeout=0.1)
                if cmdStruct == None:
                    continue
                (cmd, floppySector, ramOffset, responseQueue) = cmdStruct
                self.imageLock.acquire()

                if cmd == M35FD.CMD_EJECT:
                    self.image = None
                    if (self.state.value != M35FD.STATE_READY) and (self.state.value != M35FD.STATE_READY_WP):
                        self.error.value = M35FD.ERROR_EJECT
                    self.changeState(M35FD.STATE_NO_MEDIA)
                    responseQueue.put(1)

                elif self.image == None:
                    self.changeError(M35FD.ERROR_NO_MEDIA)
                    responseQueue.put(0)

                elif (self.state.value != M35FD.STATE_READY) and (self.state.value != M35FD.STATE_READY_WP):
                    self.changeError(M35FD.ERROR_BUSY)
                    responseQueue.put(0)

                elif (self.state.value == M35FD.STATE_READY_WP) and (cmd == M35FD.CMD_WRITE):
                    self.changeError(M35FD.ERROR_PROTECTED)
                    responseQueue.put(0)

                elif floppySector >= 1440:
                    self.changeError(M35FD.ERROR_BAD_SECTOR)
                    responseQueue.put(0)

                else:
                    if cmd == M35FD.CMD_READ:
                        self.cpu.scheduleCallback(cpu.time[0]+int(100000/60), M35FD.read, (self, floppySector, ramOffset)) 
                    elif cmd == M35FD.CMD_WRITE:
                        self.cpu.scheduleCallback(cpu.time[0]+int(100000/60), M35FD.read, (self, floppySector, ramOffset)) 
                    self.changeState(M35FD.STATE_BUSY)
                    responseQueue.put(1)

                self.imageLock.release()
            except queue.Empty:
                pass
            
    def finishUp(self):
        self.running.value = 0
        self.join()
