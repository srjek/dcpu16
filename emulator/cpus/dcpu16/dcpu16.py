import pygame, sys, os, traceback
from pygame.locals import *
import multiprocessing
from multiprocessing import Value, Array, Queue, Lock
import ctypes
import threading
import queue

dcpu16_Root = ""
if __name__ == '__main__':
    dcpu16_Root = os.path.dirname(os.path.realpath(sys.argv[0]))
else:
    dcpu16_Root = os.path.dirname(os.path.realpath(__file__))
dcpu16_Root = os.path.abspath(dcpu16_Root)
#os.chdir(dcpu16_Root)

import dummyFile
#sys.stderr.errors = 'unknown' #sys.stderr = dummyFile.dummyFile()
#sys.stdout.errors = 'unknown' #sys.stdout = dummyFile.dummyFile()

class dcpu16:
    def __init__(self):
        self.register = Array(ctypes.c_uint16, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], lock=False) #A, B, C, X, Y, Z, I, J, PC, SP, EX, IA, IAQ
        self.ram = Array(ctypes.c_uint16, [0]*0x10000, lock=False) #128 kb (0x10000 words[16bit])
        self.cycles = 0 #The extra cycles
        self.totalCycles = 0
        self.hardware = []
        self.intQueue = Array(ctypes.c_uint16, [0]*259, lock=False)
        self.intLock = Lock()
        self.callQueue = {}  #Holds callbacks that are requested by hardware (only safe for threads to call?)
        self.waitingCallback = Array(ctypes.c_ulonglong, [0]*1, lock=False)
        self.callbackLock = Lock()
        self.time = Array(ctypes.c_ulonglong, [0]*1, lock=False)
        from PyInline import C
        import PyInline, sys
        try:
            sys.stdout.errors
        except AttributeError:
            sys.stderr.errors = 'unknown'
            sys.stdout.errors = 'unknown'
        self.m = PyInline.build(code="""
              PyObject* decode(PyObject* pyRam, PyObject* pyRegisters);
              PyObject* cycles(PyObject* ram, PyObject* registers, PyObject* intQueue, int count, PyObject* interruptCallback,
                                         PyObject* hardwareCallback, PyObject* isCallback, PyObject* callbackCallback,  PyObject* time);
              #include "../../dcpu16.c"
              """,
              language="C")#, forceBuild=True)

    def addHardware(self, hardware):
        self.hardware.append(hardware)
        return len(self.hardware)-1

    ### Warning, thread safe, not process-safe
    def scheduleCallback(self, time, callback_func, args):
        self.callbackLock.acquire()
        if time in self.callQueue:
            self.callQueue[time].append((callback_func, args))
        else:
            self.callQueue[time] = [(callback_func, args)]
        if (self.waitingCallback[0] > 0) and (time < self.waitingCallback[0]):
            self.waitingCallback[0] = time
        self.callbackLock.release()
        
    def interrupt(self, msg):
        intQueue = self.intQueue
        intLock = self.intLock
        
        intLock.acquire()
        pos = intQueue[1]
        intQueue[2+pos] = msg
        pos += 1
        if pos > 256:
            pos = 0
        intQueue[1] = pos
        intLock.release()
        
        if intQueue[0] == intQueue[1]:
            raise Exception("DCPU16 is on fire. Behavior undefined")
    def callbackCallback(self):
        callbackLock = self.callbackLock
        times = []
        times.extend(self.callQueue.keys())
        callbackLock.acquire()
        self.waitingCallback[0] = 0
        callbackLock.release()
        while len(times) > 0 and times[0] <= self.time[0]:
            callbackLock.acquire()
            callbacks = self.callQueue.pop(times[0])
            callbackLock.release()
            for (callback, args) in callbacks:
                callback(*args)
            times.pop(0)
        callbackLock.acquire()
        if (len(times) != 0) and (times[0] < self.waitingCallback[0]):
            self.waitingCallback[0] = times[0]
        callbackLock.release()
    def HWIcallback(self, request):
        hardware = self.hardware
        if request == -1:       #HWN
            return len(hardware)
        hwid = request & 0xFFFF
        request = (request >> 16) & 0x3
        if hwid >= len(hardware):
            return 0
        if request == 0:        #HWI
            return hardware[hwid].interrupt()
        info = hardware[hwid].queryInfo()
        if request == 1:        #HWQ_BA
            return info[1] & 0xFFFFFFFF  #Hardware ID
        if request == 2:        #HWQ_C
            return info[2] & 0xFFFF      #Hardware Version
        if request == 3:        #HWQ_YX
            return info[0] & 0xFFFFFFFF  #Manufacturer
        return 0
    def getInternals(self):
        return (self.register, self.ram)

    def cycle(self, count):
        self.callbackCallback()
        actualCount = self.m.cycles(self.ram, self.register, self.intQueue, count - self.cycles, self.interrupt,
                                                 self.HWIcallback, self.waitingCallback, self.callbackCallback, self.time)
        self.cycles += actualCount - count
        self.totalCycles += count
        
    def disassemble(words):
        opcodes = {         "SET":0x01, "ADD":0x02, "SUB":0x03, "MUL":0x04, "MLI":0x05, "DIV":0x06, "DVI":0x07,
                "MOD":0x08, "MDI":0x09, "AND":0x0A, "BOR":0x0B, "XOR":0x0C, "SHR":0x0D, "ASR":0x0E, "SHL":0x0F,
                "IFB":0x10, "IFC":0x11, "IFE":0x12, "IFN":0x13, "IFG":0x14, "IFA":0x15, "IFL":0x16, "IFU":0x17,
                                        "ADX":0x1A, "SBX":0x1B,                         "STI":0x1E, "STD":0x1F,
                "DAT":0x10000, "JMP":0x10001}
        ext_opcodes = {
                            "JSR":0x01,                                                             "HCF":0x07,
                "INT":0x08, "IAG":0x09, "IAS":0x0A, "RFI":0x0B, "IAQ":0x0C,
                "HWN":0x10, "HWQ":0x11, "HWI":0x12 }
        opcodes = {v:k for k, v in opcodes.items()}
        ext_opcodes = {v:k for k, v in ext_opcodes.items()}
        
        result = None
        op = words[0] & 0x1f
        b = (words[0] >> 5) & 0x1f
        a = (words[0] >> 10) & 0x3f

        if op == 0x00:
            if b in ext_opcodes:
                result = ext_opcodes[b] + " "
        else:
            if op in opcodes:
                result = opcodes[op] + " "
        if result == None:
            tmp = hex(words[0])[2:]
            return "DAT 0x" + "0"*(4-len(tmp)) + tmp

        values = {0x18:"POP", 0x19:"PEEK", 0x1A:"[SP + @]", 0x1B:"SP", 0x1C:"PC", 0x1D:"EX", 0x1E:"[@]", 0x1F:"@"}
        i = 0
        for val in ("A", "B", "C", "X", "Y", "Z", "I", "J"):
            values[i] = val
            values[i | 0x08] = "[" + val + "]"
            values[i | 0x10] = "[" + val + " + @]"
            i += 1
        for i in range(-1, 0x1E):
            tmp = hex(i)[2:]
            if i < 0:
                tmp = hex(i)[3:]
            values[(i+1)|0x20] = "0x" + "0"*(4-len(tmp)) + tmp
            if i < 0:
                values[(i+1)|0x20] = "-" + values[(i+1)|0x20]

        PC = 1
        a = values[a]
        if "@" in a:
            tmp = hex(words[PC])[2:]
            a = a.replace("@", "0x" + "0"*(4-len(tmp)) + tmp)
            PC += 1

        if op != 0x00:
            values[0x18] = "PUSH"
            b = values[b]
            if "@" in b:
                tmp = hex(words[PC])[2:]
                b = b.replace("@", "0x" + "0"*(4-len(tmp)) + tmp)
                PC += 1

        if op == 0x00:
            result += a
        else:
            result += b + ", " + a
        return result
    def getState(self):
        PC = self.register[8]
        SP = self.register[9]
        EX = self.register[10]
        IA = self.register[11]
        state = [self.totalCycles]
        state.extend(self.register[0:8])
        state.append(PC)
        state.append(SP)
        state.append(EX)
        state.append(IA)
        state.append(self.ram[PC])
        state.append(self.ram[PC+1])
        state.append(self.ram[PC+2])
        return tuple(state)

    def loadDatIntoRam(self, ramOffset, dat):
        for i in range(0, len(dat), 2):
            if i+1 == len(dat):
                dat[i+1] == 0
            self.ram[ramOffset+int(i/2)] = (dat[i] << 8) | dat[i+1]
            
    def loadFileIntoRam(self, ramOffset, filepath):
        obj = open(filepath, 'rb')
        dat = obj.read()
        obj.close()
        self.loadDatIntoRam(ramOffset, dat)

class dcpu16Rom(threading.Thread):
    def __init__(self, cpu, errorQueue, firmwareFile):
        self.errorQueue = errorQueue
        self.loadFile(firmwareFile)
        self.enable()
        
        self.register, self.ram = cpu.getInternals()
        self.cpu = cpu
        hwid = cpu.addHardware(self)

        #Callbacks are processed before any cycles are run! (and after too....)
        cpu.scheduleCallback(0, dcpu16Rom.startup, (self,))

        threading.Thread.__init__(self)

    def loadDat(self, dat):
        firmware = []
        for i in range(0, len(dat), 2):
            if i+1 == len(dat):
                dat[i+1] == 0
            firmware.append( (dat[i] << 8) | dat[i+1] )
        self.firmware = tuple(firmware[:512])
    def loadFile(self, filepath):
        obj = open(filepath, 'rb')
        dat = obj.read()
        obj.close()
        self.loadDat(dat)

    def enable(self):
        self.enabled = True
    def disable(self):
        self.enabled = False
    def startup(self):
        if self.enabled:
            self.cpu.cycles += self.interrupt() #We have to manage the interrupt ourselves...

    def queryInfo(self):
             #Manufacturer,    ID     , Version)
        return (0, 0, 0)
    def interrupt(self):
        A = self.register[0] #Specs don't say anything about using this, so ignore

        i = self.register[1] #Start copying into memory at position B
        ram = self.ram
        for word in self.firmware:
            ram[i] = word
            i += 1
        return 512 #Specs don't say anything about this either, so I gave it 512 cycles, as the rom has 512 words

    def run(self):
        pass
    def finishUp(self):
        self.join()

class cpuControl(threading.Thread):
    def __init__(self, ctrlQueue, stateQueue):
        self.ctrl = ctrlQueue
        self.state = stateQueue
        self.totalCycles = 0
        self.quiting = True
        self.callbackQueue = queue.Queue()
        threading.Thread.__init__(self)

    def callback(self, function, args):
        self.callbackQueue.put((function, args))
    #def start(self):
    #    self.gui.callback(cpuControl.run, (self,))
    def run(self):
        import tkinter
        from tkinter import ttk
        self.quitting = False
        root = tkinter.Tk() #tkinter.Toplevel()
        root.title("dcpu16")
        mainframe = ttk.Frame(root, padding="3 3 12 12")
        mainframe.grid(column=0, row=1, sticky=(tkinter.N, tkinter.W, tkinter.E, tkinter.S))
        mainframe.columnconfigure(0, weight=1)
        mainframe.rowconfigure(0, weight=1)

        cycles = tkinter.StringVar()
        ttk.Label(root, textvariable=cycles).grid(column=0, row=0)
        cycles.set("Cycles: " + str(self.totalCycles))
        PC = tkinter.StringVar()
        SP = tkinter.StringVar()
        IA = tkinter.StringVar()
        A = tkinter.StringVar()
        B = tkinter.StringVar()
        C = tkinter.StringVar()
        X = tkinter.StringVar()
        Y = tkinter.StringVar()
        Z = tkinter.StringVar()
        I = tkinter.StringVar()
        J = tkinter.StringVar()
        EX = tkinter.StringVar()
        ttk.Label(mainframe, textvariable=PC).grid(column=0, row=1, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=SP).grid(column=1, row=1, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=IA).grid(column=2, row=1, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=A).grid(column=0, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=B).grid(column=1, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=C).grid(column=2, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=X).grid(column=0, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=Y).grid(column=1, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=Z).grid(column=2, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=I).grid(column=0, row=4, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=J).grid(column=1, row=4, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=EX).grid(column=2, row=4, sticky=(tkinter.W, tkinter.E))
        disassembly = tkinter.StringVar()
        ttk.Label(root, textvariable=disassembly).grid(column=0, row=2, sticky=(tkinter.W, tkinter.E))

        def bHex(a):
            result = hex(a)[2:]
            result = "0"*(4-len(result)) + result
            return result
        def cycle():
            if self.quitting:
                return
            self.ctrl.put(0)
            state = None
            while True:
                try:
                    (function, args) = self.callbackQueue.get_nowait()
                    function(*args)
                except queue.Empty:
                    break
            while True:
                try:
                    state = self.state.get_nowait()
                except queue.Empty:
                    if state != None:
                        if state == tuple():
                            root.destroy()
                            return
                        cycles.set("Cycles: " + str(state[0]))
                        PC.set("PC: " + bHex(state[9]))
                        SP.set("SP: " + bHex(state[10]))
                        IA.set("IA: " + bHex(state[12]))
                        A.set("A: " + bHex(state[1]))
                        B.set("B: " + bHex(state[2]))
                        C.set("C: " + bHex(state[3]))
                        X.set("X: " + bHex(state[4]))
                        Y.set("Y: " + bHex(state[5]))
                        Z.set("Z: " + bHex(state[6]))
                        I.set("I: " + bHex(state[7]))
                        J.set("J: " + bHex(state[8]))
                        EX.set("EX: " + bHex(state[11]))
                        disassembly.set(dcpu16.disassemble(state[13:]))
                    root.after(500, cycle)
                    return
        def run():
            self.ctrl.put(-1)
        def stop():
            self.ctrl.put(-2)
        def step():
            self.ctrl.put(-3)
            

        ttk.Button(mainframe, text="Run", command=run).grid(column=0, row=0, sticky=tkinter.W)
        ttk.Button(mainframe, text="Step", command=step).grid(column=1, row=0, sticky=tkinter.W)
        ttk.Button(mainframe, text="Stop", command=stop).grid(column=2, row=0, sticky=tkinter.W)
        root.after(1000, cycle)
        root.mainloop()
        self.quitting = True
        self.ctrl.put(-0x10c)
        #self.gui.registerQuit(cpuControl.quit, (self,))
    def quit(self):
        self.quitting = True
        self.ctrl.put(-0x10c)

class main(multiprocessing.Process): #(threading.Thread):
    def __init__(self, sysRunning, errorQueue, args, imagePath, devicePath, deviceConfig):
        if len(args) > 0:
            print("dcpu16 doesn't except args, ignoring." ,file=sys.stderr)
        self.imagePath = imagePath
        self.devicePath = devicePath
        self.deviceConfig = deviceConfig
        self.running = Value("i", 1, lock=False)
        self.sysRunning = sysRunning
        self.errorQueue = errorQueue
        #threading.Thread.__init__(self)
        multiprocessing.Process.__init__(self)
    def finishUp(self):
        self.running.value = 0
        self.join(10)
        self.terminate()
    def run(self):
        import time
        sys.stderr = dummyFile.queueFile(self.errorQueue)
        sys.stdout = dummyFile.dummyFile()
        try:
            os.chdir(dcpu16_Root)
            #pygame.init()	#don't uncomment, unneccesary for pygame.clock, and we need to keep the rest of pygame isolated to one process
            clock = pygame.time.Clock()
            error = Queue(50)
                
            ctrl = Queue(20)
            state = Queue(20)
            gui = cpuControl(ctrl, state)
            
            comp = dcpu16()
            firmwarePath = os.path.join(dcpu16_Root, "firmware.bin")
            devices = [ dcpu16Rom(comp, error, firmwarePath) ]
            if self.imagePath != None:
                devices[0].disable()
                print(devices[0].enabled)
                comp.loadFileIntoRam(0, self.imagePath)
                    
            devicePath = self.devicePath
            lastMonitor = None
            import imp
            for device in self.deviceConfig:
                name = device[0]
                args = device[1:]
                path = devicePath[name]
                if path.endswith(".py"):
                    path = os.path.dirname(path)
                
                module = None
                try:
                    tmp = imp.find_module(name, [path,])
                    module = imp.load_module(name, *tmp)
                except ImportError:
                    print("Failed to load device "+repr(name),file=sys.stderr)
                    if tmp[0] != None: tmp[0].close()
                    continue
                if tmp[0] != None: tmp[0].close()
                device_class = []
                exec("device_class.append(module."+name+")")
                device_class = device_class[0]
                initArgs = [comp, error]
                if device_class.needGui:
                    initArgs.append(gui)
                if device_class.needMonitor:
                    initArgs.append(lastMonitor)
                devices.append(device_class(*initArgs))
                if device_class.isMonitor:
                    lastMonitor = devices[-1]
            for device in devices:
                device.start()

            gui.start()
            updateState = True
            running = False
            i = 0

            from math import floor
            startTime = floor(time.time()*1000)
            executing = self.running
            executing.value = 1
            sysRunning = self.sysRunning
            while (sysRunning.value == 1) and (executing.value == 1):
                if running:
                    endTime = floor(time.time()*1000)
                    cyclesPassed = int((endTime - startTime) * 100)
                    startTime = endTime
                    comp.cycle(cyclesPassed)
                if updateState:
                    state.put(comp.getState())
                    updateState = False
                try:
                    cycles = ctrl.get_nowait()
                    updateState = True
                    if cycles > 0:
                        comp.cycle(cycles)
                    if cycles == -1:
                        running = True
                        startTime = floor(time.time()*1000)
                    if cycles == -2:
                        running = False
                    if cycles == -3:
                        comp.cycle(1)
                        comp.cycle(comp1.cycles)
                    if cycles == -0x10c:
                        executing.value = 0
                except queue.Empty:
                    dummy = 42

                try:
                    e = error.get_nowait()
                    for line in e:
                        print(line, file=sys.stderr)
                except queue.Empty:
                    dummy = 42
                
                clock.tick(60)
            self.running.value = 0
            pygame.quit()
            state.put(tuple(), True)
            
            
            for device in devices:
                device.finishUp()
            
            while True:
                try:
                    e = error.get_nowait()
                    for line in e:
                        print(line, file=sys.stderr)
                except queue.Empty:
                    break
        except Exception as e:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            self.errorQueue.put(traceback.format_exc().splitlines())
        self.errorQueue.put(sys.stdout.buffer.split("\n"))
        self.errorQueue.put(sys.stderr.buffer.split("\n"))
        
