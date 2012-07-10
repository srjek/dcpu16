import sys
import copy
import string

lineNum = -1
ferr = sys.stderr
def printError(error):
    print("Line " + str(lineNum) + ": " + error, file=ferr)
    
def removeComment(line):
    findStart = 0
    start = line.find("\"", findStart)
    while (start > 0 and line[start-1] == "\\"):
        start = line.find("\"", findStart)
        findStart += 1
    comment = line.find(";")
    if start != -1 and not (comment != -1 and comment < start):
        findStart = start+1
        while line.find("\"", findStart) > len(line) and line[line.find("\"", findStart)-1] == "\\": findStart += 1
        end = line.find("\"", findStart)
        if (end == -1):
            return line
        return line[0:end+1] + removeComment(line[end+1:])
    elif comment != -1:
        return line[0:comment]
    return line

def getLabel(line):
    if not line.startswith(":"):
        return ""
    label_end = line.find(" ")
    if label_end == -1: label_end = len(line)
    label = line[1:label_end].strip()
    whitespace = False
    for c in label:
        whitespace = whitespace or c.isspace()
    if whitespace:
        printError("Label \"" + label + "\" can not contain whitespace")
        return ""
    return label
def removeLabel(line):
    label = getLabel(line)
    if (label == ""):
        return line
    return line[1 + len(label)+1:].strip()

operators = ["JSR", "SET", "ADD", "SUB", "MUL", "DIV", "MOD", "SHL",
             "SHR", "AND", "BOR", "XOR", "IFE", "IFN", "IFG", "IFB", "DAT"]
def getOp(line):
    op = line.split(" ")[0]
    if op == "": return ""
    if not (op.upper() in operators):
        printError("Invalid opcode \"" + op + "\"")
        return getOp("")
    return op.upper()
def removeOp(line):
    op = getOp(line)
    if (op == ""): return line
    i = len(op)
    return line[i:].strip()

ARG_INT = 0
ARG_DAT = 1
ARG_REGISTER = 2
ARG_CPU = 3
ARG_LABEL = 4
ARG_DEREF = 5
ARG_OFFSET = 6
ARG_LABEL_OFFSET = 7
ARG_INVALID = 8
def isHex(string):
    for c in string:
        if not (c.isdecimal or c.upper() in ["A", "B", "C", "D", "E", "F"]):
            return False
    return True
def getArg(argStr):
    try:
        if argStr.isdecimal():
            return (ARG_INT, int(argStr))
        elif argStr.lower().startswith("0x") and isHex(argStr[2:]):
            return (ARG_INT, int(argStr, 0))
        elif argStr.lower().startswith("0b") and argStr[2:].isdecimal():
            return (ARG_INT, int(argStr, 0))
        elif argStr.lower().endswith("b") and argStr[:-1].isdecimal():
            return (ARG_INT, int(argStr[:-1], 2))
        elif argStr.lower().startswith("0o") and argStr[2:].isdecimal():
            return (ARG_INT, int(argStr, 0))
    except Exception: x = 10
    if argStr.startswith('"') and argStr.endswith('"'):
        pre_data = argStr[1:-1].encode("UTF").decode("unicode_escape").encode("ascii")
        data = b''
        for i in range(0, len(pre_data)):
            data += pre_data[i:i+1]
        return (ARG_DAT, data)
    elif argStr.lower() in ["a", "b", "c", "x", "y", "z", "i", "j"]:
        return (ARG_REGISTER, argStr.lower())
    elif argStr.upper() in ["POP", "PEEK", "PUSH", "SP", "PC", "O"]:
        return (ARG_CPU, argStr.upper())
    elif argStr.startswith('[') and argStr.endswith(']'):
        internalArg = getArg(argStr[1:-1])
        if internalArg[0] == ARG_DAT:
            printError("Data can't be dereferenced")
            return (ARG_INVALID, None)
        return (ARG_DEREF, internalArg)
    elif argStr.endswith("++"):
        if getArg(argStr[0:-2]) != (ARG_CPU, "PC"):
            printError("CPU can only increment PC while dereferencing it")
            return (ARG_INVALID, None)
        return (ARG_OFFSET, ((ARG_CPU, "PC"), (ARG_INT, 1)))
    elif "+" in argStr or "-" in argStr:
        location = argStr.find("+")
        if "-" in argStr:
            location = argStr.find("-")
        parts = ( (ARG_INT, 0), getArg(argStr[location+1:].strip()) )
        if argStr[0:location].strip() != "":
            parts = ( getArg(argStr[0:location].strip()), parts[1] )
        if "-" in argStr:
            if parts[1][0] != ARG_INT:
                printError("Can't handle subtracting labels")
                return (ARG_INVALID, None)
            parts = ( parts[0], (ARG_INT, 0x10000-parts[1][1]) )
        if parts[0][0] not in [ARG_INT, ARG_LABEL]:
            parts = (parts[1], parts[0])
        if parts[0][0] not in [ARG_INT, ARG_LABEL]:
            printError("One value must be a integer literal or label in \"" + argStr + "\"")
            return (ARG_INVALID, None)
        if parts[1][0] == ARG_INT:
            if parts[0][0] == ARG_LABEL:
                return (ARG_LABEL_OFFSET, parts)
            return (ARG_INT, parts[0][1] + parts[1][1])
        elif parts[1][0] == ARG_LABEL:
            return (ARG_LABEL_OFFSET, (parts[1], parts[0]))
        elif parts[1][0] != ARG_REGISTER:
            printError("One value must be a register, a second label, or a second integer literal in \"" + argStr + "\"")
            return (ARG_INVALID, None)
        return (ARG_OFFSET, parts)
    else:
        whitespace = False
        for c in argStr:
            whitespace = whitespace or c.isspace()
        if whitespace:
            printError("Bad operand \"" + argStr + "\"")
            return (ARG_INVALID, None)
        return (ARG_LABEL, argStr)
def splitArgs(line, result):
    findStart = 0
    start = line.find("\"", findStart)
    while (start > 0 and line[start-1] == "\\"):
        start = line.find("\"", findStart)
        findStart += 1
    comment = line.find(",")
    if start != -1 and not (comment != -1 and comment < start):
        findStart = start+1
        while line.find("\"", findStart) > len(line) and line[line.find("\"", findStart)-1] == "\\": findStart += 1
        end = line.find("\"", findStart)
        if (end == -1):
            result.append(line)
            return result
        tmp = splitArgs(line[end+1:], [])
        result.append(line[0:end+1] + tmp[0])
        result.extend(tmp[1:])
        return result
    elif comment != -1:
        result.append(line[0:comment])
        return splitArgs(line[comment+1:], result)
    result.append(line)
    return result
def getArgs(line):
    args = []
    for arg in splitArgs(line, []):
        args.append( getArg(arg.strip()) )
    return args

opcodes = {"JSR":0x0, "SET":0x1, "ADD":0x2, "SUB":0x3, "MUL":0x4, "DIV":0x5, "MOD":0x6, "SHL":0x7,
           "SHR":0x8, "AND":0x9, "BOR":0xA, "XOR":0xB, "IFE":0xC, "IFN":0xD, "IFG":0xE, "IFB":0xF, "DAT":0x10000}
ext_opcode = {"JSR":0x01}
register_values = {"a":0x00, "b":0x01, "c":0x02, "x":0x03, "y":0x04, "z":0x05, "i":0x06, "j":0x07}
cpu_values = {"POP":0x18, "PEEK":0x19, "PUSH":0x1A, "SP":0x1B, "PC":0x1C, "O":0x1D}
def assemble_stage1(op, args, address):
    badResult = (0, 0, None, None, None, lineNum)
    if op == "":    #Check for empty line
        return badResult
    for a in args:  #Check for an invalid argument
        if a[0] == ARG_INVALID:
            return badResult
        elif a[0] == ARG_INT:
            if a[1] < 0:
                printError("Can't handle negative literal")
                return badResult
    
    opcode = opcodes[op]
    args_len = 2
    if opcode == 0x0:
        opcode = ext_opcode[op] << 4
        args_len = 1
    operands = []   #holds a list of operands [6bit value] (max of 2)
    nextWord = []   #holds a list of unresolved arguments [(ARG_TYPE, arg)]

    if op == "DAT" and len(args) != 0:
        for a in args:
            if a[0] in [ARG_REGISTER, ARG_CPU, ARG_OFFSET] or (a[0] == ARG_DEREF and a[1][0] in [ARG_REGISTER, ARG_CPU, ARG_OFFSET]):
                printError("CPU registers can not be used as data, their contents are unknown.")
                return badResult
            elif a[0] == ARG_DEREF and a[1][0] == ARG_DEREF:
                tmp = a[1]
                while tmp[0] == ARG_DEREF:
                    if tmp[1][0] in [ARG_REGISTER, ARG_CPU, ARG_OFFSET]:
                        printError("CPU registers can not be used as data, their contents are unknown.")
                        return badResult
                    tmp = tmp[1]
            if a[0] == ARG_INT:
                if a[1] < 0:
                    printError("Can't handle negative literal")
                    return badResult
                num = a[1]
                dat = []
                while num > 0xFFFF:
                    dat.append(num & 0xFFFF)
                    num = num >> 16
                dat.append(num & 0xFFFF)
                for i in range(len(dat)-1, -1, -1):
                    nextWord.append( (ARG_INT, dat[i]) )
            elif a[0] == ARG_LABEL:
                nextWord.append(a)
            elif a[0] == ARG_DAT:
                for val in a[1]:
                    nextWord.append( (ARG_INT, val) )
            elif a[0] == ARG_LABEL_OFFSET:
                nextWord.append(a)
            elif a[0] == ARG_DEREF:
                nextWord.append(a)
        size = len(nextWord)
        return [address, size, opcode, operands, nextWord, lineNum]
    
    if len(args) != args_len:
        if op == "DAT": args_len = "1 or more"
        printError(op + " expects " + str(args_len) + " operands, " + str(len(args)) + " operands were provided instead")

    for a in args:
        if a[0] == ARG_INT:
            if a[1] <= 0x1f:
                operands.append(a[1] | 0x20)
            elif a[1] > 0xFFFF:
                printError("Integer literal '" + a[1] + "' is too large to fit in a word (16 bits)")
                return badResult
            else:
                operands.append(0x1F)
                nextWord.append(a)
        elif a[0] == ARG_REGISTER:
            operands.append(register_values[ a[1] ])
        elif a[0] == ARG_CPU:
            operands.append(cpu_values[ a[1] ])
        elif a[0] == ARG_LABEL:
            operands.append(0x1F)
            nextWord.append(a)
        elif a[0] == ARG_OFFSET:
            printError("Can not access a offset directly. (You can dereference an offset. ex: \"[0x8000+i]\"")
            return badResult
        elif a[0] == ARG_LABEL_OFFSET:
            operands.append(0x1F)
            nextWord.append(a)
        elif a[0] == ARG_DEREF:
            if a[1][0] == ARG_LABEL:
                operands.append(0x1E)
                nextWord.append(a[1])
            elif a[1][0] == ARG_REGISTER:
                operands.append(0x08 | register_values[ a[1][1] ])
            elif a[1] == (ARG_OFFSET, ((ARG_CPU, "PC"), (ARG_INT, 1))):
                operands.append(0x1F)
                nextWord = tuple(nextWord)
            elif a[1][0] == ARG_OFFSET:
                parts = a[1][1]
                operands.append(0x10 | register_values[ parts[1][1] ])
                nextWord.append(parts[0])
            elif a[1][0] == ARG_INT:
                operands.append(0x1E)
                nextWord.append(a[1])
            elif a[1][0] == ARG_LABEL_OFFSET:
                operands.append(0x1E)
                nextWord.append(a[1])
            elif a[1][0] == ARG_DEREF:
                if a[1][1] != (ARG_OFFSET, ((ARG_CPU, "PC"), (ARG_INT, 1))):
                    printError("Outside of [[PC++]], CPU has no intructions that dereference twice")
                    return badResult
                operands.append(0x1E)
                nextWord = tuple(nextWord)
            else:
                printError("Can not dereference " + str(a[1]) + " directly")
                return badResult
        elif a[0] == ARG_OFFSET:
            printError("Can not dereference an offset directly")
            return badResult
    size = 1 + len(nextWord)
    return [address, size, opcode, operands, list(nextWord), lineNum]

def splitWord(i):
    return [(i & 0xFF00) >> 8, i & 0x00FF]
CircularDetection = []
def getObj(value, labels, addressLookup):
    if value[0] == ARG_INT:
        return value[1]
    elif value[0] == ARG_LABEL:
        if value[1] not in labels:
            printError("Label \"" + value[1] + "\" not found")
            return None
        address = labels[value[1]] 
        return address
    elif value[0] == ARG_LABEL_OFFSET:
        parts = value[1]
        result = getObj(parts[0], labels, addressLookup) + getObj(parts[1], labels, addressLookup)
        if (result > 0xFFFF):
            printError("Sum of 2 constants is larger then a word (16 bits) can hold, continuing anyways")
            result = result & 0xFFFF
        return result
    elif value[0] == ARG_DEREF:
        pointer = getObj(value[1], labels, addressLookup)
        if pointer in CircularDetection:
            printError("Can't dereference a dereference of self. (Think circular dependencies!)")
            return None
        CircularDetection.append(pointer)
        obj = [0, 0]
        if pointer in addressLookup:
            instruction = addressLookup[pointer]
            objOffset = (pointer - instruction[0]) * 2
            obj = assemble_stage2(instruction, labels, addressLookup)[objOffset:]
        else:
            printError("Attempted to dereference a location outside the code. (Assumed that value at location is 0)")
        CircularDetection.pop(-1)
        return (obj[0] << 8) | obj[1]
        
def assemble_stage2(instruction, labels, addressLookup):
    if instruction[1] == 0: #check for a "no instruction"
        return []
    obj = []    #object code byte by byte
    opcode = instruction[2]
    operands = instruction[3]
    nextWord = instruction[4]

    instruct = 0
    if opcode == 0x10000:
        instruct = None               #DAT, there is no opcode
    elif opcode > 0x0F:
        instruct = opcode | (operands[0] << 10)    #non-basic instruction (aaaaaaoooooo0000) (the opcode var is already shifted 4 to the left)
    else:
        instruct = opcode | (operands[0] << 4) | (operands[1] << 10)    #basic instruction (bbbbbbaaaaaaoooo) (the opcode var is already shifted 4 to the left)
    if instruct != None:
        obj.extend(splitWord(instruct))

    blank = False
    for value in nextWord:
        tmp = getObj(value, labels, addressLookup)
        if (tmp == None):
            blank = True
            tmp = 0
        obj.extend(splitWord(tmp))
    if blank:
        for i in range(len(obj)): obj[i] = 0
    return obj

def replaceArgs(line, args):
    result = ""
    
    tmp = line.split()
    for i in range(len(tmp)): tmp[i] = tmp[i].strip()
    while "" in tmp: tmp.remove("")
    for t in tmp:
        tmp2 = t.split(",")
        for i in range(len(tmp2)):
            tmp2[i] = tmp2[i].strip()
            if tmp2[i] != "" and tmp2[i] in args:
                tmp2[i] = args[ tmp2[i] ]
        result += ",".join(tmp2)
        result += " "
    return result[:-1]
def preprocess(file):
    global lineNum
    lineNum = 0
    line = file.readline().strip()

    macros = {}
    pre_lines = []
    macroName = None
    macroLines = []
    while line != "":
        line = removeComment(line).strip()
        if line == "":
            line = file.readline()
            continue
        lineNum += 1
        if line.startswith("%macro") and line[6].isspace():
            tmp = line[7:].strip().split()
            for i in range(len(tmp)): tmp[i] = tmp[i].strip()
            while "" in tmp: tmp.remove("")
            if macroName != None:
                printError("Macro \"" + tmp[0] + "\" can't be defined because macro \"" + macroName[0] + "\" is being defined")
                line = file.readline()
                continue
            macroName = [tmp[0], len(tmp)-1, tmp[1:]]
            macroLines = []
            line = file.readline()
            continue
        elif line.startswith("%endmacro"):
            if macroName == None:
                printError("%endmacro encountered when no macro was being defined")
                line = file.readline()
                continue
            macros[tuple(macroName[0:2])] = (tuple(macroName[2]), tuple(macroLines))
            macroName = None
            line = file.readline()
            continue
        if macroName == None:
            pre_lines.append((line, lineNum))
        else:
            macroLines.append((line, lineNum))
        line = file.readline()
    lines = []
    for (line, lineNum) in pre_lines:
        tmp = line.split()
        for i in range(len(tmp)): tmp[i] = tmp[i].strip()
        while "" in tmp: tmp.remove("")
        i = 0
        while i < len(tmp) and tmp[i].startswith(":"): i += 1
        if i == len(tmp):
            lines.append( (line, lineNum) )
            continue
        macroName = (tmp[i], len(tmp) - (i+1))
        if macroName in macros:
            if i > 0:
                lines.append((" ".join(tmp[0:i]), lineNum))
            args = {}
            for x in range(macroName[1]):
                args[ macros[macroName][0][x] ] = tmp[(i+1) + x]
            for (line, lineNum) in macros[macroName][1]:
                lines.append( (replaceArgs(line, args), lineNum) )
        else:
            lines.append( (line, lineNum) )
    return lines

def assemble(lines):
    global lineNum
    
    curAddress = 0x0000
    labels = {}
    instructions = []
    for (line, lineNum) in lines:
        line = removeComment(line).strip()
        
        label = getLabel(line)
        line = removeLabel(line)
        if label != "":
            labels[label] = curAddress
        
        op = getOp(line)
        args = getArgs(removeOp(line))
        instruction = assemble_stage1(op, args, curAddress)
        if instruction[1] != 0:
            if instruction[2] == 0x10000:   #split up dat statements
                nextWord = instruction[4]
                for dat in nextWord:
                    newInstruct = copy.deepcopy(instruction)
                    newInstruct[0] = curAddress
                    newInstruct[1] = 1
                    newInstruct[4] = [dat]
                    instructions.append(newInstruct)
                    curAddress += 1
            else:
                instructions.append(instruction)
                curAddress += instruction[1]

    #optimize by allowing one word jmps (ex: SET PC, 0x10)
    changed = {}
    unchanged = False
    while not unchanged:
        unchanged = True
        for i in range(len(instructions)):  #Update information for labels and instructions to new address
            opcode = instructions[i][2]
            operands = instructions[i][3]
            if len(operands) <= 0: continue
            nextWord = instructions[i][4]
            removed = 0;
            
            if operands[0] == 0x1f and len(nextWord) > 0:
                value = nextWord[0]
                if value[0] == ARG_LABEL and value[1] in labels and labels[value[1]] <= 0x1F:
                    if i not in changed:
                        changed[i] = [False, False]
                    if not changed[i][0]:
                        removed += 1
                        changed[i][0] = True
            x = 0
            if 0x10 <= operands[0] <= 0x17 or operands[0] in [0x1E, 0x1F]:  #[PC++] detection
                x = 1
            if len(operands) > 1 and operands[1] == 0x1f and len(nextWord) > x:
                value = nextWord[-1]
                if value[0] == ARG_LABEL and value[1] in labels and labels[value[1]] <= 0x1F:
                    if i not in changed:
                        changed[i] = [False, False]
                    if not changed[i][1]:
                        removed += 1
                        changed[i][1] = True
            if removed > 0:
                unchanged = False
                address = instructions[i][0]
                for x in range(len(instructions)):
                    if instructions[x][0] > address:
                        instructions[x][0] -= removed;
                for x in labels.keys():
                    if labels[x] > address:
                        labels[x] -= removed;
    addressLookup = {}
    for i in range(len(instructions)):  #Actually shorten the instruction (and resolve some labels) and build address lookup table
        opcode = instructions[i][2]
        operands = instructions[i][3]
        nextWord = instructions[i][4]

        if len(operands) > 0:
            if operands[0] == 0x1f and len(nextWord) > 0:
                value = nextWord[0]
                if value[0] == ARG_LABEL and value[1] in labels and labels[value[1]] <= 0x1F:
                    operands[0] = 0x20 | labels[value[1]]
                    nextWord.pop(0)
            x = 0
            if 0x10 <= operands[0] <= 0x17 or operands[0] in [0x1E, 0x1F]:  #[PC++] detection
                x = 1
            if len(operands) > 1 and operands[1] == 0x1f and len(nextWord) > x:
                value = nextWord[-1]
                if value[0] == ARG_LABEL and value[1] in labels and labels[value[1]] <= 0x1F:
                    operands[1] = 0x20 | labels[value[1]]
                    nextWord.pop(-1)
            instructions[i][3] = operands
            instructions[i][4] = nextWord
            instructions[i][1] = 1 + len(nextWord)
        
        if instructions[i][0] in addressLookup:
            printError("Address collision, Multiple instructions have the same address")
        for pointer in range(instructions[i][0], instructions[i][0] + instructions[i][1]):
            addressLookup[pointer] = instructions[i]
    
    obj = []
    for i in instructions:
        lineNum = i[5]
        obj.extend(assemble_stage2(i, labels, addressLookup))
    return bytes(obj)

def main(argv):
    asm = open(argv[0], 'r')
    result = assemble(preprocess(asm))
    asm.close()
    obj = open(argv[1], 'wb')
    obj.write(result)
    obj.close()

if __name__ == "__main__":
    main(sys.argv[1:])
#main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
