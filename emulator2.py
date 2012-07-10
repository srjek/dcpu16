import pygame, sys
from pygame.locals import *
import multiprocessing
from multiprocessing import Value, Array, Queue
import ctypes
import threading
import queue
import tkinter
from tkinter import ttk
from LEM1802 import LEM1802

class dcpu16:
    def __init__(self):
        self.register = Array("i", [0, 0, 0, 0, 0, 0, 0, 0], lock=False) #A, B, C, X, Y, Z, I, J
        self.PC = 0
        self.SP = 0
        self.EX = 0
        self.IA = 0
        self.cycles = 0
        self.ram = Array(ctypes.c_uint16, [0]*0x10000, lock=False) #128 kb (0x10000 words[16bit])
        self.interrupts = []
        self.intQueueing = False
        self.totalCycles = 0
        self.hardware = []
        self.intQueue = Queue(256)  #For internal message passing between threads
        self.callQueue = Queue(256)  #Holds callbacks that are requested by hardware
    def nextWord(self):
        result = self.ram[self.PC]
        self.PC = (self.PC + 1) & 0xFFFF
        self.cycles += 1
        return result

    def addHardware(self, hardware):
        self.hardware.append(hardware)
    def requestCallback(self, callback):
        self.callQueue.put(callback)
    def interrupt(self, msg):
        self.intQueue.put(msg)
    def getInternals(self):
        return (self.register, self.ram)
    
    VAL_VALUE = 0
    VAL_DEREF = 1
    VAL_REGISTER = 2
    def read(self, a, isB):
        if a <= 0x07:       #register
            return (dcpu16.VAL_REGISTER, a, self.register[a])
        elif a <= 0x0F:     #[register]
            pointer = self.register[a & 0x07]
            return (dcpu16.VAL_DEREF, pointer, self.ram[pointer])
        elif a <= 0x17:     #[next word + register]
            pointer = (self.nextWord() + self.register[a & 0x07]) & 0xFFFF
            return (dcpu16.VAL_DEREF, pointer, self.ram[pointer])
        elif a == 0x18:     #PUSH/POP (b/a)
            if isB:
                self.SP = (self.SP - 1) & 0xFFFF
                return (dcpu16.VAL_DEREF, self.SP, self.ram[self.SP])
            tmp = (dcpu16.VAL_DEREF, self.SP, self.ram[self.SP])
            self.SP = (self.SP + 1) & 0xFFFF
            return tmp
        elif a == 0x19:     #PEEK   ([SP])
            return (dcpu16.VAL_DEREF, self.SP, self.ram[self.SP])
        elif a == 0x1A:     #PICK   ([SP + next word])
            pointer = (self.SP + self.nextWord()) & 0xFFFF
            return (dcpu16.VAL_DEREF, pointer, self.ram[pointer])
        elif a == 0x1B:     #SP
            return (dcpu16.VAL_REGISTER, a, self.SP)
        elif a == 0x1C:     #PC
            return (dcpu16.VAL_REGISTER, a, self.PC)
        elif a == 0x1D:     #EX
            return (dcpu16.VAL_REGISTER, a, self.EX)
        elif a == 0x1E:     #[next word]
            pointer = self.nextWord()
            return (dcpu16.VAL_DEREF, pointer, self.ram[pointer])
        elif a == 0x1F:     #next word (literal)
            value = self.nextWord()
            return (dcpu16.VAL_VALUE, value, value)
        elif a <= 0x3F:     #0xffff-0x1e (literal)
            value = (a & 0x1F) - 1
            if value == -1:
                value = 0xFFFF
            return (dcpu16.VAL_VALUE, value, value)

    def write(self, a, value):
        if a[0] == dcpu16.VAL_DEREF:
            self.ram[a[1]] = value
        elif a[0] == dcpu16.VAL_REGISTER:
            if a[1] <= 0x07:        #register
                self.register[a[1]] = value
            elif a[1] == 0x1B:      #SP
                self.SP = value
            elif a[1] == 0x1C:      #PC
                self.PC = value
            elif a[1] == 0x1D:      #EX
                self.EX = value

    def overflow(self, value):  #ADD, MUL, SHL, ADX
        self.EX = (value >> 16) & 0xFFFF
        return value & 0xFFFF
    def underflow(self, value):
        if value < 0:     #SUB, SBX
            self.EX = 0xFFFF
        else:
            self.EX = 0x0000
        return value & 0xFFFF
    def nop(self, b, a):
        return
    def sign(self, a):
        if (a & 0x8000):
            return a - 0x10000
        return a
    def unsign(self, a):
        return a & 0xFFFF

    #-------EXTENDED OPCODES--------
    def execJSR(self, a):
        #self.opcodes[0x01](self, self.read(0x18, True), self.read(0x1C, False)) #SET PUSH, PC
        #self.opcodes[0x01](self, self.read(0x1C, True), a)                      #SET PC, a
        #return
        self.SP = (self.SP - 1) & 0xFFFF
        self.ram[self.SP] = self.PC
        self.PC = a[2]

    def INT(self, a):
        self.interrupts.append(a[2])
        if len(self.interrupts) > 256:
            raise Exception("DCPU16 is on fire. Behavior undefined")
    def RFI(self, a):
        self.intQueueing = False
        self.PC = ram[self.SP]
        self.SP = (self.SP + 1) & 0xFFFF
        self.register[0] = ram[self.SP]
        self.SP = (self.SP + 1) & 0xFFFF
        #self.opcodes[0x00](self, 0x0C, (0, 0, 0))                                           #IAQ 0
        #self.opcodes[0x01](self, self.read(0x1C, True), self.read(0x18, False))             #SET PC, POP
        #self.opcodes[0x01](self, self.read(0x00, True), self.read(0x18, False))             #SET A, POP
    def IAG(self, a): self.registers[0] = self.IA
    def IAS(self, a): self.IA = a[2]
    def IAQ(self, a): self.intQueueing = (a[2] != 0)
    def HWQ(self, a):
        dev = hardware[a[2]]
        info = dev.queryInfo()
        register[3] = info[0] & 0xFFFF              #Manufacturer
        register[4] = (info[0] >> 16) & 0xFFFF
        register[0] = info[1] & 0xFFFF              #Hardware ID
        register[1] = (info[1] >> 16) & 0xFFFF
        register[2] = info[2] & 0xFFFF              #Hardware Version
    def HWI(self, a):
        if a[2] < len(self.hardware):
            self.cycles += self.hardware[a[2]].interrupt()
    ext_opcodes = {0x01:execJSR,                                                #JSR
                   0x08:INT,                                                    #INT
                   0x09:IAG,                                                    #IAG
                   0x0A:IAS,                                                    #IAS
                   0x0B:RFI,                                                    #RFI
                   0x0C:IAQ,                                                    #IAQ
                   0x10:(lambda self, a: self.write(a, len(self.hardware))),    #HWN
                   0x11:HWQ,                                                    #HWQ
                   0x12:HWI                                                     #HWI
                   }
    def exec_ext_opcodes(self, b, a):
        if b not in dcpu16.ext_opcodes:
            return
        return dcpu16.ext_opcodes[b](self, a)
    #--------BASIC OPCODES----------
    def overflowDIV(self, b, a):
        if a == 0:
            self.EX = 0
            return 0
        self.EX = int((b << 16) / a) & 0xffff
        return int(b / a) & 0xFFFF
    def MOD(self, b, a):
        if a == 0:
            return 0
        else:
            return b % a
    def overflowSHR(self, b, a):
        self.EX = ((b << 16) >> a) & 0xFFFF
        return (b >> a) & 0xFFFF
    def skip(self):
        instruction = self.nextWord()
        op = instruction & 0x1F
        b = (instruction >> 5) & 0x1F
        a = (instruction >> 10) & 0x3F
        if (0x10 <= a <= 0x17) or a == 0x1A or a == 0x1E or a == 0x1F:
            self.PC += 1
        if (0x10 <= b <= 0x17) or b == 0x1A or b == 0x1E or b == 0x1F:
            self.PC += 1
        self.PC &= 0xFFFF
        return (0x10 <= op <= 0x17)
    def conditional(self, condition):     #TODO
        if not condition:
            while self.skip():
                dummy = 42

    def STI(self, b, a):
        self.opcodes[0x01](b, a) #SET <b>, <a>
        self.register[6] = (self.register[6] + 1) & 0xFFFF
        self.register[7] = (self.register[7] + 1) & 0xFFFF
    def STD(self, b, a):
        self.opcodes[0x01](b, a) #SET <b>, <a>
        self.register[6] -= 1
        if self.register[6] < 0: self.register[6] += 0x10000
        self.register[7] -= 1
        if self.register[7] < 0: self.register[7] += 0x10000
    
    opcodes = {0x00:(lambda self, b, a: self.exec_ext_opcodes(b, a)),                      #Non-basic instruction
           0x01:(lambda self, b, a: self.write( b, a[2]) ),                                                 #SET
           0x02:(lambda self, b, a: self.write( b, self.overflow(b[2] + a[2]) )),                           #ADD
           0x03:(lambda self, b, a: self.write( b, self.underflow(b[2] - a[2]) )),                          #SUB
           0x04:(lambda self, b, a: self.write( b, self.overflow(b[2] * a[2]) )),                           #MUL
           0x05:(lambda self, b, a: self.write( b, self.overflow(self.sign(b[2]) * self.sign(a[2])) )),     #MLI
           0x06:(lambda self, b, a: self.write( b, self.overflowDIV(b[2], a[2]) )),                         #DIV
           0x07:(lambda self, b, a: self.write( b, self.overflowDIV(self.sign(b[2]), self.sign(a[2])) )),   #DVI
           0x08:(lambda self, b, a: self.write( b, self.MOD(b[2], a[2]) )),                                 #MOD
           0x09:nop,                                                                                         #MDI        (Actually, remainder)
           0x0A:(lambda self, b, a: self.write( b, b[2] & a[2] )),                                          #AND
           0x0B:(lambda self, b, a: self.write( b, b[2] | a[2] )),                                          #BOR
           0x0C:(lambda self, b, a: self.write( b, b[2] ^ a[2] )),                                          #XOR
           0x0D:(lambda self, b, a: self.write( b, self.overflowSHR(b[2], a[2]) )),                         #SHR
           0x0E:(lambda self, b, a: self.write( b, self.overflowSHR(self.sign(b[2]), a[2]) )),              #ASR
           0x0F:(lambda self, b, a: self.write( b, self.overflow(b[2] << a[2]) )),                          #SHL
           0x10:(lambda self, b, a: self.conditional( (b[2] & a[2])!=0 )),                                  #IFB
           0x11:(lambda self, b, a: self.conditional( (b[2] & a[2])==0 )),                                  #IFC
           0x12:(lambda self, b, a: self.conditional(b[2] == a[2]) ),                                       #IFE
           0x13:(lambda self, b, a: self.conditional(b[2] != a[2]) ),                                       #IFN
           0x14:(lambda self, b, a: self.conditional(b[2] > a[2]) ),                                        #IFG
           0x15:(lambda self, b, a: self.conditional(self.sign(b[2]) > self.sign(a[2])) ),                  #IFA
           0x16:(lambda self, b, a: self.conditional(b[2] < a[2]) ),                                        #IFL
           0x17:(lambda self, b, a: self.conditional(self.sign(b[2]) < self.sign(a[2])) ),                  #IFU
           0x18:nop,
           0x19:nop,
           0x1A:(lambda self, b, a: self.write( b, self.overflow(b[2] + a[2] + self.EX) )),                 #ADX
           0x1B:(lambda self, b, a: self.write( b, self.underflow(b[2] - a[2] + self.EX) )),                #SBX
           0x1C:nop,
           0x1D:nop,
           0x1E:STI,                                                                                        #STI
           0x1F:STD,                                                                                        #STD
            }
    extOpcodeCycles = {
            0x00:0, 0x01:2, 0x02:0, 0x03:0, 0x04:0, 0x05:0, 0x06:0, 0x07:0,
            0x08:3, 0x09:0, 0x0A:0, 0x0B:2, 0x0C:1, 0x0D:0, 0x0E:0, 0x0F:0,
            0x10:1, 0x11:3, 0x12:3, 0x13:0, 0x14:0, 0x15:0, 0x16:0, 0x17:0,
            0x18:0, 0x19:0, 0x1A:0, 0x1B:0, 0x1C:0, 0x1D:0, 0x1E:0, 0x1F:0 }
    opcodeCycles = {
                    0x01:0, 0x02:1, 0x03:1, 0x04:1, 0x05:1, 0x06:2, 0x07:2,
            0x08:2, 0x09:2, 0x0A:0, 0x0B:0, 0x0C:0, 0x0D:0, 0x0E:0, 0x0F:0,
            0x10:1, 0x11:1, 0x12:1, 0x13:1, 0x14:1, 0x15:1, 0x16:1, 0x17:1,
            0x18:0, 0x19:0, 0x1A:2, 0x1B:2, 0x1C:0, 0x1D:0, 0x1E:1, 0x1F:1 }
    
    def execOp(self):
        instruction = self.nextWord()
        op = instruction & 0x1F
        b = (instruction >> 5) & 0x1F
        a = (instruction >> 10) & 0x3F
        
        a = self.read(a, False)
        if op != 0:
            b = self.read(b, True)
            self.cycles += dcpu16.opcodeCycles[op]
        else:
            self.cycles += dcpu16.extOpcodeCycles[b]
        dcpu16.opcodes[op](self, b, a)
                
        if len(self.interrupts) > 0 and not self.intQueueing:
            if self.IA != 0:
                self.interrupts.pop(0)
            else:
                self.opcodes[0x00](self, 0x0C, (0, 0, 1))                                           #IAQ 1
                self.opcodes[0x01](self, self.read(0x18, True), self.read(0x1C, False))             #SET PUSH, PC
                self.opcodes[0x01](self, self.read(0x18, True), self.read(0x00, False))             #SET PUSH, A
                self.opcodes[0x01](self, self.read(0x1C, True), (0, 0, self.IA))                    #SET PC, IA
                self.opcodes[0x01](self, self.read(0x00, True), (0, 0, self.interrupts.pop(0)))     #SET A, <msg>
            
    def cycle(self, count):
        execOp = self.execOp
        while self.cycles < count:
            execOp()
        self.cycles -= count
        self.totalCycles += count
        
        while True:
            try:
                callback = self.callQueue.get_nowait()
                callback()
            except queue.Empty:
                break
        while True:
            try:
                msg = self.intQueue.get_nowait()
                self.opcodes[0x00](self, 0x08, (0, 0, msg))         #INT <msg>
            except queue.Empty:
                break
        
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
        for i in range(-1, 0x1F):
            tmp = hex(i)[2:]
            values[(i+1)|0x20] = "0x" + "0"*(4-len(tmp)) + tmp

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
        state = [self.totalCycles]
        state.extend(self.register)
        state.append(self.PC)
        state.append(self.SP)
        #state.append(self.EX)
        state.append(self.ram[0x8000])
        state.append(self.IA)
        state.append(self.ram[self.PC])
        state.append(self.ram[self.PC+1])
        state.append(self.ram[self.PC+2])
        return tuple(state)
    
    def loadIntoRam(self, ramOffset, filepath):
        obj = open(filepath, 'rb')
        dat = obj.read()
        obj.close()
        for i in range(0, len(dat), 2):
            if i+1 == len(dat):
                dat[i+1] == 0
            self.ram[ramOffset+int(i/2)] = (dat[i] << 8) | dat[i+1]

class cpuControl(threading.Thread):
    def __init__(self, ctrlQueue, stateQueue):
        self.ctrl = ctrlQueue
        self.state = stateQueue
        self.totalCycles = 0
        threading.Thread.__init__(self)

    def run(self):
        root = tkinter.Tk()
        mainframe = ttk.Frame(root, padding="3 3 12 12")
        mainframe.grid(column=0, row=0, sticky=(tkinter.N, tkinter.W, tkinter.E, tkinter.S))
        mainframe.columnconfigure(0, weight=1)
        mainframe.rowconfigure(0, weight=1)

        cycles = tkinter.StringVar()
        ttk.Label(mainframe, textvariable=cycles).grid(column=1, row=0, sticky=(tkinter.W, tkinter.E))
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
        ttk.Label(mainframe, textvariable=PC).grid(column=0, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=SP).grid(column=1, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=IA).grid(column=2, row=2, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=A).grid(column=0, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=B).grid(column=1, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=C).grid(column=2, row=3, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=X).grid(column=0, row=4, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=Y).grid(column=1, row=4, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=Z).grid(column=2, row=4, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=I).grid(column=0, row=5, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=J).grid(column=1, row=5, sticky=(tkinter.W, tkinter.E))
        ttk.Label(mainframe, textvariable=EX).grid(column=2, row=5, sticky=(tkinter.W, tkinter.E))
        disassembly = tkinter.StringVar()
        ttk.Label(root, textvariable=disassembly).grid(column=0, row=1, sticky=(tkinter.W, tkinter.E))
        
        def bHex(a):
            result = hex(a)[2:]
            result = "0"*(4-len(result)) + result
            return result
        def cycle():
            self.ctrl.put(0)
            state = None
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
            

        ttk.Button(mainframe, text="Run", command=run).grid(column=0, row=1, sticky=tkinter.W)
        ttk.Button(mainframe, text="Step", command=step).grid(column=1, row=1, sticky=tkinter.W)
        ttk.Button(mainframe, text="Stop", command=stop).grid(column=2, row=1, sticky=tkinter.W)
        root.after(1000, cycle)
        root.mainloop()
        self.ctrl.put(-0x10c)
    
def main():
    pygame.init()
    clock = pygame.time.Clock()
    error = Queue(50)

    comp1 = dcpu16()
    comp1.ram[0] = 0x01 | (0x1E << 5) | (0x25 << 10)
    comp1.ram[1] = 0x8000
    comp1.ram[2] = 0x02 | (0x1C << 5) | (0x20 << 10)
    comp1.loadIntoRam(0, "notchTest.bin")
    
    monitor = LEM1802(comp1, error)

    ctrl = Queue(20)
    state = Queue(20)
    ctrlWindow = cpuControl(ctrl, state)
    ctrlWindow.start()
    monitor.start()
    updateState = True
    executing = True
    running = False
    i = 0
    while executing:
        if running:
            comp1.cycle(2500)   #100000 / 40
        if updateState:
            state.put(comp1.getState())
            updateState = False
        try:
            cycles = ctrl.get_nowait()
            updateState = True
            if cycles > 0:
                comp1.cycle(cycles)
            if cycles == -1:
                running = True
            if cycles == -2:
                running = False
            if cycles == -3:
                comp1.cycle(1)
                comp1.cycle(comp1.cycles)
            if cycles == -0x10c:
                pygame.event.post(pygame.event.Event(QUIT))
        except queue.Empty:
            dummy = 42

        try:
            e = error.get_nowait()
            for line in e:
                print(line, file=sys.stderr)
            print(repr(clock.get_fps()))
        except queue.Empty:
            dummy = 42
        
        for event in pygame.event.get():
            if event.type == QUIT:
                executing = False
            elif event.type == KEYDOWN:
                if event.key == K_ESCAPE:
                    pygame.event.post(pygame.event.Event(QUIT))
        clock.tick(40)
    pygame.quit()
    state.put(tuple(), True)
    monitor.finishUp()
    ctrlWindow.join()
    monitor.join()

if __name__ == '__main__':
    main()
