import sys
import tkinter
import time

register = [0, 0, 0, 0, 0, 0, 0, 0] #A, B, C, X, Y, Z, I, J
PC = 0
SP = 0
O = 0
cycleDebt = 0
ram = [0]*0x10000 #128 kb (0x10000 words[16bit])
console = 0

def loadIntoRam(ramOffset, filepath):
    global ram
    obj = open(filepath, 'rb')
    dat = obj.read()
    obj.close()
    for i in range(0, len(dat), 2):
        if i+1 == len(dat):
            dat[i+1] == 0
        ram[ramOffset+int(i/2)] = (dat[i] << 8) | dat[i+1]

def readWord():
    global ram
    global PC
    instruction = ram[PC]
    PC += 1
    if PC > 0xFFFF:
        PC = PC & 0xFFFF
    return instruction

VAL_VALUE = 0
VAL_DEREF = 1
VAL_REGISTER = 2
def read(a):
    global cycleDebt
    global ram
    global register
    global SP
    global PC
    global O
    if a <= 0x07:       #register
        return (VAL_REGISTER, a, register[a])
    elif a <= 0x0F:     #[register]
        pointer = register[a & 0x07]
        return (VAL_DEREF, pointer, ram[pointer])
    elif a <= 0x17:     #[next word + register]
        cycleDebt += 1
        pointer = readWord() + register[a & 0x07]
        pointer &= 0xFFFF
        return (VAL_DEREF, pointer, ram[pointer])
    elif a == 0x18:     #POP
        tmp = (VAL_DEREF, SP, ram[SP])
        SP += 1
        if SP > 0xFFFF:
            SP = 0
        return tmp
    elif a == 0x19:     #PEEK
        return (VAL_DEREF, SP, ram[SP])
    elif a == 0x1A:     #PUSH   (not sure if this should be legal, but if PUSH is an alias for [--SP] then it would work)
        SP -= 1
        if SP < 0:
            SP = 0xFFFF
        return (VAL_DEREF, SP, ram[SP])
    elif a == 0x1B:     #SP
        return (VAL_REGISTER, a, SP)
    elif a == 0x1C:     #PC
        return (VAL_REGISTER, a, PC)
    elif a == 0x1D:     #O
        return (VAL_REGISTER, a, O)
    elif a == 0x1E:     #[next word]
        cycleDebt += 1
        pointer = readWord()
        return (VAL_DEREF, pointer, ram[pointer])
    elif a == 0x1F:     #next word (literal)
        cycleDebt += 1
        pointer = PC
        return (VAL_DEREF, pointer, readWord())
    elif a <= 0x3F:     #0x00-0x1f (literal)
        return (VAL_VALUE, a & 0x1F, a & 0x1F)

def write(a, value):
    global ram
    global register
    global SP
    global PC
    global O
    if a[0] == VAL_DEREF:
        ram[a[1]] = value
    elif a[0] == VAL_REGISTER:
        if a[1] <= 0x07:        #register
            register[a[1]] = value
        elif a[1] == 0x1B:      #SP
            SP = value
        elif a[1] == 0x1C:      #PC
            PC = value
        elif a[1] == 0x1D:      #O
            O = value
    #elif a[0] == VAL_VALUE     #You're crazy, this won't work

def overflow(value):
    global cycleDebt
    global O
    cycleDebt += 1
    if value > 0xFFFF:  #ADD, MUL, SHL
        O = (value >> 16) & 0xFFFF
        return value & 0xFFFF
    elif value < 0:     #SUB
        O = 0xFFFF
        return value + 0x10000
    return value
def overflowDIV(a, b):
    global cycleDebt
    global O
    cycleDebt += 2
    if b == 0:
        O = 0
        return 0
    O = int((a << 16) / b) & 0xffff
    return int(a / b)
def MOD(a, b):
    global cycleDebt
    cycleDebt += 2
    if b == 0:
        return a
    else:
        return a % b
def overflowSHR(a, b):
    global cycleDebt
    global O
    cycleDebt += 1
    O = ((a << 16) >> b) & 0xFFFF
    return a >> b
def conditional(condition):
    global cycleDebt
    global PC
    cycleDebt += 1
    if not condition:
        cycleDebt += 1
        instruction = readWord()
        opcode = instruction & 0x000F
        a = (instruction & 0x03F0) >> 4
        b = (instruction & 0xFC00) >> 10
        if opcode != 0x0 and (0xF < a < 0x17 or a == 0x1E or a == 0x1F):
            readWord()
        if 0xF < b < 0x17 or b == 0x1E or b == 0x1F:
            readWord()
def execJSR(a):
    global cycleDebt
    global PC
    global SP
    global ram
    cycleDebt += 1
    SP -= 1
    if SP < 0:
        SP = 0xFFFF
    ram[SP] = PC
    PC = a[2]

ext_opcodes = {0x01:execJSR }                                       #JSR
def exec_ext_opcodes(a, b):
    if a not in ext_opcodes:
        return
    return ext_opcodes[a](b)
opcodes = {0x0:(exec_ext_opcodes ),                                 #Non-basic instruction
           0x1:(lambda a, b: write(a, b[2]) ),                      #SET
           0x2:(lambda a, b: write( a, overflow(a[2] + b[2]) )),    #ADD
           0x3:(lambda a, b: write( a, overflow(a[2] - b[2]) )),    #SUB
           0x4:(lambda a, b: write( a, overflow(a[2] * b[2]) )),    #MUL
           0x5:(lambda a, b: write( a, overflowDIV(a[2], b[2]) )),  #DIV
           0x6:(lambda a, b: write(a, MOD(a[2], b[2]) )),           #MOD
           0x7:(lambda a, b: write( a, overflow(a[2] << b[2]) )),   #SHL
           0x8:(lambda a, b: write( a, overflowSHR(a[2], b[2]) )),  #SHR
           0x9:(lambda a, b: write( a, a[2] & b[2] )),              #AND
           0xA:(lambda a, b: write( a, a[2] | b[2] )),              #BOR
           0xB:(lambda a, b: write( a, a[2] ^ b[2] )),              #XOR
           0xC:(lambda a, b: conditional(a[2] == b[2]) ),           #IFE
           0xD:(lambda a, b: conditional(a[2] != b[2]) ),           #IFN
           0xE:(lambda a, b: conditional(a[2] > b[2]) ),            #IFG
           0xF:(lambda a, b: conditional( (a[2] & b[2])!=0 ))}      #IFB
def execute(opcode, a, b):
    global cycleDebt
    if opcode != 0x0:
        a = read(a)
    b = read(b)
    if opcode not in opcodes:
        return
    opcodes[opcode](a, b)

def changeChar(i, char, foreground, background):
    global console
    x = i % (1*32)
    y = int(i / (1*32)) + 1
    tag = 'x'+str(x)+'y'+str(y)
    loc = str(y)+'.'+str(x)
    console.configure(state="normal")
    console.tag_configure(tag, background='black', foreground='white', font=("lucida console",16))
    console.delete(loc, str(y)+'.'+str(x+1))
    if char >= 128 or char < 32:
        char = " "
    else:
        char = bytes([char]).decode("ascii")
    console.insert(loc, char, tag)
    console.configure(state="disabled")
timestamp = time.time()*1000 + 3000
debug = False
def cycle(count=100000):
    global cycleDebt
    global timestamp

    if debug and count != 1:
        return
    for i in range(count):
        if cycleDebt > 0:
            cycleDebt -= 1
            continue
        instruction = readWord()
        opcode = instruction & 0x000F
        a = (instruction & 0x03F0) >> 4
        b = (instruction & 0xFC00) >> 10
        execute(opcode, a, b)
        #if cycleDebt == -1:
        #    break
    for i in range(0x8000, 0x8000 + 1*32*12):
        char = ram[i] & 0x00FF
        background = (ram[i] & 0x0F00) >> 8
        foreground = (ram[i] & 0xF000) >> 12
        changeChar(i - 0x8000, char, foreground, background)
    if cycleDebt != -1:
        timestamp += 1000
        clock = time.time()*1000
        diff = int(timestamp-clock)
        console.after(diff, cycle)
        print(diff)

def main(argv):
    global console
    loadIntoRam(0, argv[0])
    console = tkinter.Text(wrap="none")
    initialText="Hang on," + (" "*24 + "\n") + "     the emulator is starting...\n"
    initialText += (" "*32 + "\n") + "  Rate: 100000 cycles / 1 sec   \n" + (" "*32 + "\n")*8
    console.insert('1.0', initialText)
    for x in range(0, 32):
        for y in range(1,12+1):
            tag = 'x'+str(x)+'y'+str(y)
            console.tag_configure(tag, background='black', foreground='white', font=("lucida console",16))
            console.tag_add(tag, str(y)+'.'+str(x), str(y)+'.'+str(x+1))
    console.configure(width=52,height=16,state="disabled")
    console.pack()
    console.after(2000, cycle)
    console.mainloop()
    
    
if __name__ == "__main__":
    main(sys.argv[1:])
