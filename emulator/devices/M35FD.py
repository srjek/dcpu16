#import pygame, sys, traceback
from pygame.locals import *
#import multiprocessing
from multiprocessing import Value, Array, Lock
import ctypes
import threading
import queue
from queue import Queue

class M35FD(threading.Thread):
    needGui = True
    needMonitor = False
    isMonitor = False
    
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
    
    RAMDISK = 0
    
    def __init__(self, cpu, errorQueue, gui, imagePath=None):
        self.register, self.ram = cpu.getInternals()
        self.cpu = cpu
        self.gui = None
        self.errorQueue = errorQueue

        self.state = Value(ctypes.c_uint16, M35FD.STATE_NO_MEDIA, lock=False)
        self.error = Value(ctypes.c_uint16, M35FD.ERROR_NONE, lock=False)
        self.interruptMsg = Value(ctypes.c_uint16, 0, lock=True)

        self.image = None
        self.imagePath = None
        self.imageLock = Lock()
        if imagePath != None:
            self.loadFile(imagePath, readonly=True)

        cpu.addHardware(self)
        threading.Thread.__init__(self)
        
        self.gui = gui
        gui.callback(M35FD.__initGui__, (self,))
    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (0x1eb37e91, 0x12345678, 3)  #TODO: device spec doesn't hace a legit id yet (it reuses the LEMs)
    def __initGui__(self):
        import tkinter
        from tkinter import ttk, filedialog, messagebox
        window = tkinter.Toplevel()
        window.title("M35FD")
        
        mainframe = ttk.Frame(window, padding="3 3 12 12")
        mainframe.grid(column=0, row=1, sticky=(tkinter.N, tkinter.W, tkinter.E, tkinter.S))
        mainframe.columnconfigure(0, weight=1)
        mainframe.rowconfigure(0, weight=1)
        
        imagePath = tkinter.StringVar()
        ttk.Label(window, textvariable=imagePath).grid(column=0, row=0)
        self.guiImagePath = imagePath
        self._guiUpdatePath()
        
        ttk.Button(mainframe, text="Eject", command=self.eject).grid(column=0, row=0, sticky=(tkinter.W, tkinter.E))
        ttk.Button(mainframe, text="Load as readonly", command=self.guiLoadReadonly).grid(column=0, row=1, sticky=(tkinter.W, tkinter.E))
        ttk.Button(mainframe, text="Load as isolated ramdisk", command=self.guiLoadRamdisk).grid(column=0, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Button(mainframe, text="Load", command=self.guiLoad).grid(column=0, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Button(mainframe, text="Save", command=self.guiSave).grid(column=0, row=4, sticky=(tkinter.W, tkinter.E))
        
        self.window = window
    
    def _guiUpdatePath(self):
        if self.image == None:
            self.guiImagePath.set("<None>")
        elif self.imagePath == None:
            self.guiImagePath.set("<Readonly RAM Disk>")
        elif self.imagePath == M35FD.RAMDISK:
            self.guiImagePath.set("<RAM Disk>")
        else:
            self.guiImagePath.set(str(self.imagePath))
    def guiUpdatePath(self):
        if self.gui != None:
            self.gui.callback(M35FD._guiUpdatePath, (self,))
    def eject(self):
        self.handleCommand(M35FD.CMD_EJECT, 0, 0)
    def getFileR(self):
        imagePath = filedialog.askopenfilename(title="Select a floppy image to load", parent=self.window)
        if (imagePath == "") or (imagePath == ()):
            return None
        return imagePath
    def getFileW(self):
        return filedialog.asksaveasfilename(title="Specify a file name to save as", parent=self.window)
        if (imagePath == "") or (imagePath == ()):
            return None
        return imagePath
    def showError(self, msg):
        messagebox.showerror("Error", msg)
    def guiLoadFile(self, filePath, readonly=False, ramdisk=True):
        if filePath == None: return
        try:
            self.loadFile(filePath, readonly, ramdisk)
        except Exception as e:
            self.showError(str(e.args[0]))
    def guiSaveFile(self, filePath):
        if filePath == None: return
        try:
            self.saveFile(filePath)
        except Exception as e:
            self.showError(str(e.args[0]))
    def guiLoadReadonly(self):
        self.guiLoadFile(self.getFileR(), readonly=True)
    def guiLoadRamdisk(self):
        self.guiLoadFile(self.getFileR(), readonly=False, ramdisk=True)
    def guiLoad(self):
        self.guiLoadFile(self.getFileR(), readonly=False, ramdisk=False)
    def guiSave(self):
        self.guiSaveFile(self.getFileW())

    def _loadDat(self, dat, readonly=False):
        if self.state.value != M35FD.STATE_NO_MEDIA:
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
            self.imagePath = None
            self.changeState(M35FD.STATE_READY_WP)
        else:
            self.imagePath = M35FD.RAMDISK
            self.changeState(M35FD.STATE_READY)
    def loadDat(self, dat, readonly=False):
        self.imageLock.acquire()
        try:
            self._loadDat(dat, readonly)
        except:
            self.imageLock.release()
            raise
        self.guiUpdatePath()
        self.imageLock.release()
    def loadFile(self, filepath, readonly=False, ramdisk=True):
        self.imageLock.acquire()
        if self.state.value != M35FD.STATE_NO_MEDIA:
            self.imageLock.release()
            raise Exception("Floppy already inserted")
        image = open(filepath, 'rb')
        dat = image.read()
        image.close()
        try:
            self._loadDat(dat, readonly)
        except:
            self.imageLock.release()
            raise
        if (not readonly) and (not ramdisk):
            self.imagePath = filepath
        self.guiUpdatePath()
        self.imageLock.release()
    def _getDat(self):
        dat = []
        for obj in self.image:
            dat.append((obj >> 8) & 0xFF)
            dat.append(obj & 0xFF)
        return bytes(dat)
    def getDat(self):
        self.imageLock.acquire()
        self._genDat()
        self.imageLock.release()
    def _saveFile(self, filepath=None):
        if self.state.value == M35FD.STATE_NO_MEDIA:
            raise Exception("No floppy inserted")
        image = open(filepath, 'wb')
        image.write(self._getDat())
        image.close()
    def saveFile(self, filepath=None):
        self.imageLock.acquire()
        try:
            self._saveFile(filepath)
        except:
            self.imageLock.release()
            raise
        self.imageLock.release()


    def interrupt(self):
        A = self.register[0]
        if A == 0:
            self.imageLock.acquire()
            self.register[1] = self.state.value
            self.register[2] = self.error.value
            self.error.value = M35FD.ERROR_NONE
            self.imageLock.release()
        elif A == 1:
            self.interruptMsg.Value = self.register[3]
        elif A == 2:
            self.register[1] = self.handleCommand(M35FD.CMD_READ, self.register[3], self.register[4])
        elif A == 3:
            self.register[1] = self.handleCommand(M35FD.CMD_WRITE, self.register[3], self.register[4])
        return 0

    def read(self, floppySector, ramOffset):
        ram = self.ram
        self.imageLock.acquire()
        image = self.image
        if image == None:
            self.imageLock.release()
            return
        for i in range(512):
            ram[ramOffset+i] = image[floppySector*512+i]
        if self.imagePath == None:
            self.changeState(M35FD.STATE_READY_WP)
        else:
            self.changeState(M35FD.STATE_READY)
        self.imageLock.release()
    def write(self, floppySector, ramOffset):
        ram = self.ram
        self.imageLock.acquire()
        image = self.image
        if image == None:
            self.imageLock.release()
            return
        for i in range(512):
            image[floppySector*512+i] = ram[(ramOffset+i)&0xFFFF]
        if self.imagePath == None:
            self.changeState(M35FD.STATE_READY_WP)
        else:
            self.changeState(M35FD.STATE_READY)
            if self.imagePath != M35FD.RAMDISK:
                self._saveFile()
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

    def handleCommand(self, cmd, floppySector, ramOffset):
        self.imageLock.acquire()
        state = self.state.value
        result = 0

        if cmd == M35FD.CMD_EJECT:
            self.image = None
            self.guiUpdatePath()
            if (self.state.value != M35FD.STATE_READY) and (self.state.value != M35FD.STATE_READY_WP):
                self.error.value = M35FD.ERROR_EJECT
            self.changeState(M35FD.STATE_NO_MEDIA)
            result = 1

        elif self.image == None:
            self.changeError(M35FD.ERROR_NO_MEDIA)
        elif (state != M35FD.STATE_READY) and (state != M35FD.STATE_READY_WP):
            self.changeError(M35FD.ERROR_BUSY)
        elif (state == M35FD.STATE_READY_WP) and (cmd == M35FD.CMD_WRITE):
            self.changeError(M35FD.ERROR_PROTECTED)
        elif floppySector >= 1440:
            self.changeError(M35FD.ERROR_BAD_SECTOR)
        else:
            cpu = self.cpu
            if cmd == M35FD.CMD_READ:
                cpu.scheduleCallback(cpu.time[0]+int(100000/60), M35FD.read, (self, floppySector, ramOffset)) 
            elif cmd == M35FD.CMD_WRITE:
                cpu.scheduleCallback(cpu.time[0]+int(100000/60), M35FD.read, (self, floppySector, ramOffset)) 
            self.changeState(M35FD.STATE_BUSY)
            result = 1

        self.imageLock.release()
        return result
        
    def run(self):
        return
    def finishUp(self):
        self.join()
