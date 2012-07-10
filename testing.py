from emulator3 import dcpu16
from assembler2 import reader, writer
import copy
import io,sys

class flushfile:
    def __init__(self, f):
        self.f = f
    def write(self, x):
        self.f.write(x)
        self.f.flush()
    def flush(self):
        self.f.flush()

class dcpu16State:
    def __init__(self, state, ram):
        self.cycles = state[0]
        self.registers = state[1:9]
        self.A = state[1]
        self.B = state[2]
        self.C = state[3]
        self.X = state[4]
        self.Y = state[5]
        self.Z = state[6]
        self.I = state[7]
        self.J = state[8]
        self.PC = state[9]
        self.SP = state[10]
        self.EX = state[11]
        self.IA = state[12]
        self.instruction = state[13:]
        self.ram = ram

def test(code, instructions):
    parser = reader()
    for c in code:
        parser.readLine(c)
    assembler = writer(parser.instructions, parser.labels)
    comp1 = dcpu16()
    comp1.loadDatIntoRam(0, assembler.genObj())

    if type(instructions) != type((0,)) and type(instructions) != type([0]):
        for x in range(instructions):
            comp1.cycle(1)
            comp1.cycle(comp1.cycles)
        return dcpu16State(comp1.getState(), comp1.ram)
        
    states = []
    for c in instructions:
        for x in range(c):
            comp1.cycle(1)
            comp1.cycle(comp1.cycles)
        states.append(dcpu16State(comp1.getState(), copy.copy(comp1.ram)))
    return states

def main():
    from random import randrange
    from termcolor import colored
    FAILED = colored("FAILED", 'red')
    SUCCESS = colored("SUCCESS", 'green')
    NOT_TESTED = colored("NOT TESTED", 'yellow')
    
    print("Testing operation SET: ", end='')
    for x in range(10):
        r = randrange(33, pow(2,16)-1)
        r2 = randrange(-1, 32) & 0xFFFF
        state = test(("SET A, " + repr(r), "SET A, " + repr(r2)), (1,1))
        if state[0].A != r:
            print(FAILED)
            print("\tCould not set register A to literal " + repr(r) + ". Register was", state.A, "instead")
            return
        if state[1].A != r2:
            print(FAILED)
            print("\tCould not set register A to short literal " + repr(r2) + ". Register was", state.A, "instead")
            return
    print(SUCCESS)
    
    print("Testing various operands using SET...")
    print("\tLiterals and short literals: " + SUCCESS + " (included in test of the SET operation)")
    print("\tTesting registers: ", end='')
    for foo in ((0, "A"), (1, "B"), (2, "C"), (3, "X"), (4, "Y"), (5, "Z"), (6, "I"), (7, "J")):
       for x in range(10):
            r = randrange(0, pow(2,16))
            t = (0, "A")
            if foo[1] == "A":
                t = (1, "B")
            state = test(("SET " + foo[1] + ", " + repr(r), "SET " + t[1] + ", " + foo[1]), 2)
            if state.registers[foo[0]] != r:
                print(FAILED)
                print("\t\tCould not set register", foo[1], "to " + repr(r) + ". Register was", state.registers[foo[0]], "instead")
                return
            if state.registers[t[0]] != r:
                print(FAILED)
                print("\t\tCould not read register " + foo[1] + ". Register was read as", state.registers[t[0]], "instead of", r)
                return
    print(SUCCESS)
    print("\tTesting special register PC: ", end='')
    for x in range(10):
        r = randrange(4, pow(2,16)-4)
        state = test(("SET PC, " + repr(r), ".align " + repr(r), "SET A, 0x10c", "SET B, PC", "SET PC, 0xFFFF", ".align 0xFFFF", "SET A, 1"), (1,1,1,2))
        failed = []
        if state[0].PC != r:
            failed.append("\t\tCould not set register PC to " + repr(r) + ". Register was " + repr(state[0].PC) + " instead")
        if state[1].A != 0x10c:
            failed.append("\t\tEmulator failed to follow the PC register after a \"SET PC, " + repr(r) + "\"")
        if state[3].A != 1:
            failed.append("\t\tEmulator failed to follow the PC register after a \"SET PC, 0xFFFF\"")
        if state[3].PC != 0:
            failed.append("\t\tEmulator failed to wrap the PC register back to 0 after executing an instruction at 0xFFFF")
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            return
        if state[2].B != r+3:
            print(FAILED)
            print("\t\tCould not read register PC. Register was read as", state[2].B, "instead of", r+3)
            return
    print(SUCCESS)
    print("\tTesting special register SP, PUSH, PEEK, POP, PICK: ", end='')
    for x in range(10):
        r = randrange(20, pow(2,16)-1)
        r2 = randrange(r, pow(2,16))-r
        state = test(("SET SP, " + repr(r), "SET A, SP", "SET PUSH, 0x10c", "SET A, SP", "SET A, PEEK", "SET A, POP", "SET A, [SP-1]", "SET B, [SP-" + repr(r) + "]", "SET [SP+" + repr(r2) + "], 0xBEEF"), (2,2,1,1,2,1))
        failed = []
        #Write to SP, read SP
        if state[0].SP != r:
            failed.append("\t\tCould not set register SP to " + repr(r) + ". Register was " + repr(state[0].SP) + " instead")
        if state[0].A != state[0].SP:
            failed.append("\t\tCould not read register SP. Register was read as " + repr(state[0].A) + " instead of " + repr(state[0].SP))
        #Push to stack, read SP
        if state[1].ram[state[1].SP] != 0x10c:
            failed.append("\t\tStack operation \"SET PUSH, 0x10c\" failed, last item on stack was " + hex(state[1].ram[state[1].SP]))
        if state[1].SP != state[0].SP-1:
            failed.append("\t\tStack operation \"SET PUSH, 0x10c\" failed, stack pointer was " + repr(state[1].SP) + " instead of " + repr(state[0].SP-1))
        if state[1].A != state[1].SP:
            failed.append("\t\tCould not read register SP. Register was read as " + repr(state[1].A) + " instead of " + repr(state[1].SP))
        #Peek from stack
        if state[2].SP != state[1].SP:
            failed.append("\t\tStack operation \"SET A, PEEK\" failed, stack pointer changed to " + repr(state[1].SP) + " from " + repr(state[0].SP))
        if state[2].A != state[1].ram[state[1].SP]:
            failed.append("\t\tCould not peek. Last item on stack was read as " + hex(state[2].A) + " instead of " + hex(state[1].ram[state[1].SP]))
        #Pop from stack
        if state[3].SP != state[2].SP+1:
            failed.append("\t\tStack operation \"SET A, POP\" failed, stack pointer was " + repr(state[3].SP) + " instead of " + repr(state[2].SP+1))
        if state[3].ram[state[2].SP] != state[2].ram[state[2].SP]:
            failed.append("\t\tStack operation \"SET A, POP\" failed, pop-ed item on stack was changed in ram from " + hex(state[2].ram[state[2].SP]) + " to " + hex(state[3].ram[state[2].SP]))
        if state[3].A != state[2].ram[state[2].SP]:
            failed.append("\t\tCould not pop. Last item on stack was pop-ed as " + hex(state[3].A) + " instead of " + hex(state[2].ram[state[2].SP]))
        #Read stack offset (TWICE!)
        if state[4].SP != state[3].SP:
            failed.append("\t\tStack operation \"SET A, [SP-1]; SET B, [SP-" + repr(r) + "]\" failed, stack pointer changed to " + repr(state[4].SP) + " from " + repr(state[3].SP))
        if state[4].ram[state[3].SP-1] != state[3].ram[state[3].SP-1]:
            failed.append("\t\tStack operation \"SET A, [SP-1]\" failed, Pick-ed item on stack was changed in ram from " + hex(state[3].ram[state[3].SP-1]) + " to " + hex(state[4].ram[state[3].SP-1]))
        if state[4].ram[(state[3].SP-r)&0xFFFF] != state[3].ram[(state[3].SP-r)&0xFFFF]:
            failed.append("\t\tStack operation \"SET B, [SP-" + repr(r) + "]\" failed, Pick-ed item on stack was changed in ram from " + hex(state[3].ram[(state[3].SP-r)&0xFFFF]) + " to " + hex(state[4].ram[(state[3].SP-r)&0xFFFF]))
        if state[4].A != state[3].ram[state[3].SP-1]:
            failed.append("\t\tCould not pick. Offset -1 was pick-ed as " + hex(state[4].A) + " instead of " + hex(state[3].ram[state[3].SP-1]))
        if state[4].B != state[3].ram[(state[3].SP-r)&0xFFFF]:
            failed.append("\t\tCould not pick. Offset -" + repr(r) + " was pick-ed as " + hex(state[4].B) + " instead of " + hex(state[3].ram[(state[3].SP-r)&0xFFFF]))
        #Write to stack offset
        if state[5].ram[r+r2] != 0xBEEF:
            failed.append("\t\tCould not write to stack offset. [SP+"+repr(r2)+"] ("+repr(r)+"+"+repr(r2)+") was written as " + hex(state[5].ram[r+r2]) + " instead of 0xbeef")
        
        state = test(("SET PUSH, 0x10c", "SET A, [SP+1]", "SET [SP+"+repr(r)+"], 0xBEEF", "SET A, [SP+"+repr(r)+"]", "SET A, POP"), (1,1,1,1,1))
        #Push to stack when SP is 0
        if state[0].ram[state[0].SP] != 0x10c:
            failed.append("\t\tStack operation \"SET PUSH, 0x10c\" failed, last item on stack was " + hex(state[1].ram[state[1].SP]))
        if state[0].SP != 0xFFFF:
            failed.append("\t\tStack operation \"SET PUSH, 0x10c\" failed, stack pointer was " + repr(state[1].SP) + " instead of " + repr(0xFFFF))
        #Pick from stack when SP is 0xFFFF
        if state[1].A != state[0].ram[0]:
            failed.append("\t\tCould not pick. Offset +1 was pick-ed as " + hex(state[1].A) + " instead of " + hex(state[0].ram[0]))
        #Write to stack offset when SP is 0xFFFF
        if state[2].ram[r-1] != 0xBEEF:
            failed.append("\t\tCould not write to stack offset. Offset +"+repr(r)+" was written as " + hex(state[2].ram[r-1]) + " instead of 0xbeef")
        #Pick from stack offset when SP is 0xFFFF
        if state[3].A != state[2].ram[r-1]:
            failed.append("\t\tCould not pick. Offset +"+repr(r)+" was pick-ed as " + hex(state[3].A) + " instead of " + hex(state[2].ram[r-1]))
        #Pop from stack when SP is 0xFFFF
        if state[4].SP != (state[3].SP+1)&0xFFFF:
            failed.append("\t\tStack operation \"SET A, POP\" failed, stack pointer was " + repr(state[4].SP) + " instead of " + repr((state[3].SP+1)&0xFFFF))
        if state[4].ram[state[3].SP] != state[3].ram[state[3].SP]:
            failed.append("\t\tStack operation \"SET A, POP\" failed, pop-ed item on stack was changed in ram from " + hex(state[3].ram[state[3].SP]) + " to " + hex(state[4].ram[state[3].SP]))
        if state[4].A != state[3].ram[state[3].SP]:
            failed.append("\t\tCould not pop. Last item on stack was pop-ed as " + hex(state[4].A) + " instead of " + hex(state[3].ram[state[3].SP]))


        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            return
    print(SUCCESS)
    print("\tTesting special register EX: ", end='')
    for x in range(10):
        r = randrange(10, pow(2,16))
        state = test(("SET EX, " + repr(r), "SET A, EX"), (1,1))
        failed = []
        if state[0].EX != r:
            failed.append("\t\tCould not set special register EX to " + repr(r) + ". Register was " + repr(state[0].EX) + " instead")
        if state[1].A != state[0].EX:
            failed.append("\t\tCould not read special register EX. Register was read as " + repr(state[1].A) + " instead of " + repr(state[1].EX))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            return
    print(SUCCESS)
    print("\tTesting dereferencing: ", end='')
    for x in range(10):
        r = randrange(10, pow(2,16))
        r2 = randrange(0, pow(2,16))
        t = (0, "A")
        if foo[1] == "A":
            t = (1, "B")
        state = test(("SET [" + repr(r) + "], " + repr(r2), "SET A, [" + repr(r) + "]"), (1,1))
        failed = []
        if state[0].ram[r] != r2:
            failed.append("\t\tCould not set [" + repr(r) + "] to " + repr(r2) + ". Ram was " + repr(state[0].ram[r]) + " instead")
        if state[1].ram[r] != state[0].ram[r]:
            failed.append("\t\t\"SET A, [" + repr(r) + "]\" failed. Ram changed from " + repr(state[0].ram[r]) + " to " + repr(state[1].ram[r]))
        if state[1].A != state[0].ram[r]:
            failed.append("\t\tCould not read [" + repr(r) + "]. Ram was read as " + repr(state[1].A) + " instead of " + repr(state[0].ram[r]))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            return
    print(SUCCESS)
    print("\tTesting register dereferencing: ", end='')
    for foo in ((0, "A"), (1, "B"), (2, "C"), (3, "X"), (4, "Y"), (5, "Z"), (6, "I"), (7, "J")):
        for x in range(10):
            r = randrange(10, pow(2,16))
            r2 = randrange(0, pow(2,16))
            t = (0, "A")
            if foo[1] == "A":
                t = (1, "B")
            state = test(("SET " + foo[1] + ", " + repr(r), "SET [" + foo[1] + "], " + repr(r2), "SET " + t[1] + ", [" + foo[1] + "]"), (2,1))
            failed = []
            if state[0].ram[r] != r2:
                failed.append("\t\tCould not set [" + foo[1] + "] ([" + repr(r) + "]) to " + repr(r2) + ". Ram was " + repr(state[0].ram[r]) + " instead")
            if state[1].ram[r] != state[0].ram[r]:
                failed.append("\t\t\"SET " + t[1] + ", [" + foo[1] + "]\" ([" + repr(r) + "]) failed. Ram changed from " + repr(state[0].ram[r]) + " to " + repr(state[1].ram[r]))
            if state[1].registers[t[0]] != state[0].ram[r]:
                failed.append("\t\tCould not read [" + foo[1] + "] ([" + repr(r) + "]). Ram was read as " + repr(state[1].registers[t[0]]) + " instead of " + repr(state[0].ram[r]))
            if len(failed) > 0:
                print(FAILED)
                for msg in failed: print(msg)
                return
    print(SUCCESS)
    print("\tTesting register+offset dereferencing: ", end='')
    for foo in ((0, "A"), (1, "B"), (2, "C"), (3, "X"), (4, "Y"), (5, "Z"), (6, "I"), (7, "J")):
        for x in range(10):
            r = randrange(5, pow(2,15))
            r2 = randrange(5, pow(2,15))
            r3, r4 = 0, 0
            while (r3+r4) < (0xFFFF+20):
                if r3 == pow(2,16): r3 -= 1
                if r4 == pow(2,16): r4 -= 1
                r3 = randrange(r3, pow(2,16))
                r4 = randrange(r4, pow(2,16))
            v = randrange(0, pow(2,16))
            t = (0, "A")
            if foo[1] == "A":
                t = (1, "B")
            state = test(("SET "+foo[1]+", "+repr(r), "SET ["+foo[1]+"+"+repr(r2)+"], "+repr(v), "SET "+t[1]+", ["+foo[1]+"+"+repr(r2)+"]", "SET "+foo[1]+", "+repr(r3), "SET ["+foo[1]+"+"+repr(r4)+"], "+repr(v), "SET "+t[1]+",["+foo[1]+"+"+repr(r4)+"]"), (2,1,2,1))
            failed = []
            #When offset doesn't wrap around
            if state[0].ram[r+r2] != v:
                failed.append("\t\tCould not set [" + foo[1] + "+" + repr(r2) + "]\" ([" + repr(r) + "+" + repr(r2) + "]) to " + repr(v) + ". Ram was " + repr(state[0].ram[r+r2]) + " instead")
            if state[1].ram[r+r2] != state[0].ram[r+r2]:
                failed.append("\t\t\"SET " + t[1] + ", [" + foo[1] + "+" + repr(r2) + "]\" ([" + repr(r) + "+" + repr(r2) + "]) failed. Ram changed from " + repr(state[0].ram[r+r2]) + " to " + repr(state[1].ram[r+r2]))
            if state[1].registers[t[0]] != state[0].ram[r+r2]:
                failed.append("\t\tCould not read [" + foo[1] + "+" + repr(r2) + "] ([" + repr(r) + "+" + repr(r2) + "]). Ram was read as " + repr(state[1].registers[t[0]]) + " instead of " + repr(state[0].ram[r+r2]))
            #When offset wraps around
            if state[2].ram[(r3+r4)&0xFFFF] != v:
                failed.append("\t\tCould not set [" + foo[1] + "+" + repr(r4) + "]\" ([" + repr(r3) + "+" + repr(r4) + "]) to " + repr(v) + ". Ram was " + repr(state[2].ram[(r3+r4)&0xFFFF]) + " instead")
            if state[3].ram[(r3+r4)&0xFFFF] != state[2].ram[(r3+r4)&0xFFFF]:
                failed.append("\t\t\"SET " + t[1] + ", [" + foo[1] + "+" + repr(r4) + "]\" ([" + repr(r3) + "+" + repr(r4) + "]) failed. Ram changed from " + repr(state[2].ram[(r3+r4)&0xFFFF]) + " to " + repr(state[3].ram[(r3+r4)&0xFFFF]))
            if state[3].registers[t[0]] != state[2].ram[(r3+r4)&0xFFFF]:
                failed.append("\t\tCould not read [" + foo[1] + "+" + repr(r4) + "] ([" + repr(r3) + "+" + repr(r4) + "]). Ram was read as " + repr(state[3].registers[t[0]]) + " instead of " + repr(state[2].ram[(r3+r4)&0xFFFF]))
            if len(failed) > 0:
                print(FAILED)
                for msg in failed: print(msg)
                return
    print(SUCCESS)
    print("Testing operation ADD: ", end=''); failed = False
    for x in range(20):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if x < 10:
            while r+r2 > 0xFFFF:
                tmp = r; r = r2
                if tmp == 0: tmp = 1
                r2 = randrange(0, tmp)
        else:
            while r+r2 < 0x10000:
                tmp = r; r = r2
                if tmp == pow(2,16): tmp -= 1
                r2 = randrange(tmp, pow(2,16))
        state = test(("SET A, "+repr(r), "ADD A, "+repr(r2)), 2)
        failed = []
        if state.A != (r+r2)&0xFFFF:
            failed.append("\tCould not add "+repr(r)+" and "+repr(r2)+". Result was " + repr(state.A) + " instead of " + repr((r+r2)&0xFFFF))
        if state.EX != ((r+r2)>>16)&0xFFFF:
            failed.append("\tCould not add "+repr(r)+" and "+repr(r2)+". EX was " + repr(state.EX) + " instead of " + repr(((r+r2)>>16)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation SUB: ", end=''); failed = False
    for x in range(20):
        r = randrange(0, pow(2,16)-1)
        r2 = randrange(0, pow(2,16))
        if x < 10:
            if (r-r2) < 0:
                tmp = r; r = r2
                r2 = tmp
        else:
            if (r-r2) >= 0:
                tmp = r; r = r2
                r2 = tmp+1
        state = test(("SET A, "+repr(r), "SUB A, "+repr(r2)), 2)
        failed = []
        if state.A != (r-r2)&0xFFFF:
            failed.append("\tCould not subtract "+repr(r2)+" from "+repr(r)+". Result was " + repr(state.A) + " instead of " + repr((r-r2)&0xFFFF))
        if state.EX != ((r-r2)&0xFFFF0000)>>16:
            failed.append("\tCould not subtract "+repr(r2)+" from "+repr(r)+". EX was " + repr(state.EX) + " instead of " + repr(((r-r2)&0xFFFF0000)>>16))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation MUL: ", end=''); failed = False
    for x in range(20):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if x < 10:
            while r*r2 > 0xFFFF:
                tmp = r; r = r2
                if tmp == 0: tmp = 1
                r2 = randrange(0, tmp)
        else:
            while r*r2 < 0x10000:
                tmp = r; r = r2
                if tmp == pow(2,16): tmp -= 1
                r2 = randrange(tmp, pow(2,16))
        state = test(("SET A, "+repr(r), "MUL A, "+repr(r2)), 2)
        failed = []
        if state.A != (r*r2)&0xFFFF:
            failed.append("\tCould not multiply "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex((r*r2)&0xFFFF))
        if state.EX != ((r*r2)>>16)&0xFFFF:
            failed.append("\tCould not multiply "+hex(r)+" and "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(((r*r2)>>16)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation MLI: ", end=''); failed = False
    for x in range(20):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(-pow(2,15), pow(2,15))
        if x < 10:
            while not (-pow(2,15) <= (r*r2) < pow(2,15)):
                tmp = r; r = r2
                if tmp <= 0: tmp = (-tmp)+1
                r2 = randrange(-tmp, tmp)
        else:
            while -pow(2,15) <= (r*r2) < pow(2,15):
                tmp = r; r = r2
                r2 = randrange(-pow(2,15), pow(2,15))
        state = test(("SET A, "+repr(r), "MLI A, "+repr(r2)), 2)
        failed = []
        if state.A != (r*r2)&0xFFFF:
            failed.append("\tCould not multiply "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex((r*r2)&0xFFFF))
        if state.EX != ((r*r2)>>16)&0xFFFF:
            failed.append("\tCould not multiply "+hex(r)+" and "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(((r*r2)>>16)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation DIV: ", end=''); failed = False
    for x in range(11):
        r = randrange(0, pow(2,16))
        r2 = randrange(1, pow(2,16))
        result = int(r/r2)&0xFFFF
        extra = int((r<<16)/r2)&0xFFFF
        if x == 10:
            r2, result, extra = 0, 0, 0
        state = test(("SET A, "+repr(r), "DIV A, "+repr(r2)), 2)
        failed = []
        if state.A != result:
            failed.append("\tCould not divide "+hex(r)+" by "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(result))
        if state.EX != extra:
            failed.append("\tCould not divide "+hex(r)+" by "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(extra))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation DVI: ", end=''); failed = False
    for x in range(11):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(-pow(2,15), pow(2,15))
        result = int(r/r2)&0xFFFF
        extra = int((r<<16)/r2)&0xFFFF
        if x == 10:
            r2, result, extra = 0, 0, 0
        state = test(("SET A, "+repr(r), "DVI A, "+repr(r2)), 2)
        failed = []
        if state.A != result:
            failed.append("\tCould not divide "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(result))
        if state.EX != extra:
            failed.append("\tCould not divide "+hex(r)+" and "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(extra))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation MOD: ", end=''); failed = False
    for x in range(11):
        r = randrange(0, pow(2,16))
        r2 = randrange(1, pow(2,16))
        result = (r%r2)&0xFFFF
        if x == 10:
            r2, result = 0, 0
        state = test(("SET A, "+repr(r), "MOD A, "+repr(r2)), 2)
        failed = []
        if state.A != result:
            failed.append("\tCould not calculate "+hex(r)+" mod "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(result))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation MDI: ", end=''); failed = False
    for x in range(11):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(-pow(2,15), pow(2,15))
        result = (r-r2*int(r/r2))&0xFFFF
        if x == 10:
            r2, result = 0, 0
        state = test(("SET A, "+repr(r), "MDI A, "+repr(r2)), 2)
        failed = []
        if state.A != result:
            failed.append("\tCould not get the remainder of "+hex(r)+" divided by "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(result))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation AND: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        state = test(("SET A, "+repr(r), "AND A, "+repr(r2)), 2)
        failed = []
        if state.A != r&r2:
            failed.append("\tCould not binary AND "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(r&r2))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation BOR: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        state = test(("SET A, "+repr(r), "BOR A, "+repr(r2)), 2)
        failed = []
        if state.A != r|r2:
            failed.append("\tCould not binary OR "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(r|r2))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation XOR: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        state = test(("SET A, "+repr(r), "XOR A, "+repr(r2)), 2)
        failed = []
        if state.A != r^r2:
            failed.append("\tCould not binary XOR "+hex(r)+" and "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(r^r2))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation SHR: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, 17)
        state = test(("SET A, "+repr(r), "SHR A, "+repr(r2)), 2)
        failed = []
        if state.A != (r>>r2):
            failed.append("\tCould not logical shift "+hex(r)+" right by "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex(r>>r2))
        if state.EX != ((r<<16)>>r2)&0xFFFF:
            failed.append("\tCould not logical shift "+hex(r)+" right by "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(((r<<16)>>r2)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation ASR: ", end=''); failed = False
    for x in range(10):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(0, 17)
        state = test(("SET A, "+repr(r), "ASR A, "+repr(r2)), 2)
        failed = []
        if state.A != (r>>r2)&0xFFFF:
            failed.append("\tCould not arithmetic shift "+hex(r)+" right by "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex((r>>r2)&0xFFFF))
        if state.EX != (((r&0xFFFF)<<16)>>r2)&0xFFFF:
            failed.append("\tCould not arithmetic shift "+hex(r)+" right by "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex((((r&0xFFFF)<<16)>>r2)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation SHL: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, 17)
        state = test(("SET A, "+repr(r), "SHL A, "+repr(r2)), 2)
        failed = []
        if state.A != (r<<r2)&0xFFFF:
            failed.append("\tCould not logical shift "+hex(r)+" left by "+hex(r2)+". Result was " + hex(state.A) + " instead of " + hex((r<<r2)&0xFFFF))
        if state.EX != ((r<<r2)>>16)&0xFFFF:
            failed.append("\tCould not logical shift "+hex(r)+" left by "+hex(r2)+". EX was " + hex(state.EX) + " instead of " + hex(((r<<r2)>>16)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFB: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if randrange(0,1) != 0:
            r2 ^= (r&r2)
        state = test(("IFB "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if ((r&r2) != 0) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if ((r&r2) == 0) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFC: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if randrange(0,1) != 0:
            r2 ^= (r&r2)
        state = test(("IFC "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if ((r&r2) == 0) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if ((r&r2) != 0) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFE: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if randrange(0,1) != 0:
            r2 = r
        state = test(("IFE "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r == r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r != r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFN: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        if randrange(0,1) != 0:
            r2 = r
        state = test(("IFN "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r != r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r == r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFG: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        state = test(("IFG "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r > r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r <= r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFA: ", end=''); failed = False
    for x in range(10):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(-pow(2,15), pow(2,15))
        state = test(("IFA "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r > r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r <= r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFL: ", end=''); failed = False
    for x in range(10):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        state = test(("IFL "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r < r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r >= r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IFU: ", end=''); failed = False
    for x in range(10):
        r = randrange(-pow(2,15), pow(2,15))
        r2 = randrange(-pow(2,15), pow(2,15))
        state = test(("IFU "+repr(r)+", "+repr(r2),"SET B, 1", "SET A, 1"), 3)
        failed = []
        if state.A != 1:
            failed.append("\tDid not reach expected point in program flow. PC was " + hex(state.PC))
        if (r < r2) and (state.B != 1):
            failed.append("\tProgram flow unexpectedly skipped. PC was " + hex(state.PC))
        if (r >= r2) and (state.B != 0):
            failed.append("\tProgram flow did not skip as expected. PC was " + hex(state.PC))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation ADX: ", end=''); failed = False
    for x in range(20):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        r3 = randrange(0, pow(2,16))
        if x < 10:
            while r+r2 > 0xFFFF:
                tmp = r; r = r2; r2 = r3
                if tmp == 0: tmp = 1
                r3 = randrange(0, tmp)
        else:
            while r+r2 < 0x10000:
                tmp = r; r = r2; r2 = r3
                if tmp == pow(2,16): tmp -= 1
                r3 = randrange(tmp, pow(2,16))
        state = test(("SET A, "+repr(r), "SET EX, "+repr(r3), "ADX A, "+repr(r2)), 3)
        failed = []
        if state.A != (r+r2+r3)&0xFFFF:
            failed.append("\tCould not add "+hex(r)+", "+hex(r2)+", and "+hex(r3)+". Result was " + hex(state.A) + " instead of " + hex((r+r2+r3)&0xFFFF))
        if state.EX != ((r+r2+r3)>>16)&0xFFFF:
            failed.append("\tCould not add "+hex(r)+", "+hex(r2)+", and "+hex(r3)+". EX was " + hex(state.EX) + " instead of " + hex(((r+r2+r3)>>16)&0xFFFF))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation SBX: ", end=''); failed = False
    for x in range(20):
        r = randrange(0, pow(2,16))
        r2 = randrange(0, pow(2,16))
        r3 = randrange(0, pow(2,16)-1)
        if x < 10:
            while ((r-r2)-r3) < 0:
                r = randrange(max(r-1, 0), pow(2,16))
                if ((r-r2)-r3) < 0: break
                r2 = randrange(0, r2+1)
                if ((r-r2)-r3) < 0: break
                r3 = randrange(0, r3+1)
        else:
            while ((r-r2)-r3) < 0:
                r3 = randrange(max(r3-1, 0), pow(2,16)-1)
                if ((r-r2)-r3) < 0: break
                r2 = randrange(max(r2-1, 0), pow(2,16))
                if ((r-r2)-r3) < 0: break
                r = randrange(0, r+1)
        state = test(("SET A, "+repr(r), "SET EX, -"+repr(r3), "SBX A, "+repr(r2)), (2,1))
        failed = []
        if state[1].A != (r-r2-r3)&0xFFFF:
            failed.append("\tCould not subtract "+hex(r2)+" and "+hex(r3)+" from "+hex(r)+". Result was " + hex(state[1].A) + " instead of " + hex((r-r2-r3)&0xFFFF))
        if state[1].EX != ((r-r2-r3)&0xFFFF0000)>>16:
            failed.append("\tCould not subtract "+hex(r2)+" and "+hex(r3)+" from "+hex(r)+". EX was " + hex(state[1].EX) + " instead of " + hex(((r-r2-r3)&0xFFFF0000)>>16))
        if len(failed) > 0:
            print(FAILED)
            failed.append("\tEX was set to " + hex(state[0].EX)+". Something like ("+hex(r)+"-"+hex(r2)+"+"+hex(state[0].EX)+")")
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation STI: ", end=''); failed = False
    for x in range(10):
        pos1 = randrange(10, pow(2,16))
        pos2 = randrange(10, pow(2,16))
        if pos1 == pos2: pos2 += 1
        value = randrange(0, pow(2,16))
        
        state = test(("SET I, "+repr(pos1), "SET J, "+repr(pos2),"STI [J], [I]", ".align "+repr(pos1), "DAT "+hex(value)), 3)
        failed = []
        if state.I != pos1+1:
            failed.append("\t\"STI [J], [I] failed. Register I was " + hex(state.I) + " instead of " + hex(pos1+1))
        if state.J != pos2+1:
            failed.append("\t\"STI [J], [I] failed. Register J was " + hex(state.J) + " instead of " + hex(pos2+1))
        if state.ram[pos2] != value:
            failed.append("\tCould not set [" + hex(pos2) + "] to [" + hex(pos1) + "] ("+hex(value)+"). Ram was " + hex(state.ram[pos2]) + " instead")
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation STD: ", end=''); failed = False
    for x in range(10):
        pos1 = randrange(10, pow(2,16))
        pos2 = randrange(10, pow(2,16))
        if pos1 == pos2: pos2 += 1
        value = randrange(0, pow(2,16))
        
        state = test(("SET I, "+repr(pos1), "SET J, "+repr(pos2),"STD [J], [I]", ".align "+repr(pos1), "DAT "+hex(value)), 3)
        failed = []
        if state.I != pos1-1:
            failed.append("\t\"STI [J], [I] failed. Register I was " + hex(state.I) + " instead of " + hex(pos1-1))
        if state.J != pos2-1:
            failed.append("\t\"STI [J], [I] failed. Register J was " + hex(state.J) + " instead of " + hex(pos2-1))
        if state.ram[pos2] != value:
            failed.append("\tCould not set [" + hex(pos2) + "] to [" + hex(pos1) + "] ("+hex(value)+"). Ram was " + hex(state.ram[pos2]) + " instead")
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation JSR: ", end=''); failed = False
    for x in range(10):
        r = randrange(10, pow(2,16))        
        state = test(("JSR " + repr(r), "SET A, 0xbeef", ".align " + repr(r), "SET A, 0x10c", "SET PC, POP"), (1,1,1,1))
        
        failed = []
        if state[0].PC != r:
            failed.append("\tCould not set register PC to " + hex(r) + ". Register was " + hex(state[0].PC) + " instead")
        if state[0].ram[state[0].SP] != 2:
            failed.append("\tOperation \"JSR "+hex(r)+"\" failed, last item on stack was " + hex(state[0].ram[state[0].SP]))
        if state[0].SP != 0xFFFF:
            failed.append("\tOperation \"JSR "+hex(r)+"\" failed, stack pointer was " + hex(state[0].SP) + " instead of 0xffff")
        if state[1].A != 0x10c:
            failed.append("\tEmulator failed to follow the PC register after a \"JSR " + hex(r) + "\"")
        if state[2].PC != 2:
            failed.append("\tEmulator failed to pop from the stack into the PC register")
        if state[3].A != 0xbeef:
            failed.append("\tEmulator failed to follow the PC register after a \"SET PC, POP\"")
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Testing operation IAS, IAG: ", end=''); failed = False
    for x in range(10):
        r = randrange(10, pow(2,16))
        state = test(("IAS " + repr(r), "IAG A"), (1,1))
        failed = []
        if state[0].IA != r:
            failed.append("\tCould not set special register IA to " + repr(r) + ". Register was " + repr(state[0].IA) + " instead")
        if state[1].A != state[0].IA:
            failed.append("\tCould not read special register IA. Register was read as " + repr(state[1].A) + " instead of " + repr(state[1].IA))
        if len(failed) > 0:
            print(FAILED)
            for msg in failed: print(msg)
            failed = True
            break
    if not failed:
        print(SUCCESS)
    print("Operation IAQ: "+NOT_TESTED)
    print("Operation INT: "+NOT_TESTED)
    print("Operation RFI: "+NOT_TESTED)
    print("Operation HWN: "+NOT_TESTED)
    print("Operation HWQ: "+NOT_TESTED)
    print("Operation HWI: "+NOT_TESTED)
    print("Interrupts: "+NOT_TESTED)
    print("Interrupt queuing: "+NOT_TESTED)
    print("Hardware plugin API: "+NOT_TESTED)


if __name__ == '__main__':
    try:
        sys.stdout.errors
        sys.stdout = flushfile(sys.stdout)
        sys.stderr = flushfile(sys.stderr)
    except AttributeError:
        import colorama
        colorama.init(strip=True, convert=False)
    main()
