import sys
import copy
import string
import os.path
from mathEval import eval_0xSCAmodified, wordString, extractVaribles, tokenize, validate

INCLUDEPATH = ()    #That's right folks, empty. I have no standard libraries for this assembler.

def printError(error, lineNum, file=sys.stderr):
    print("Line " + str(lineNum) + ": " + str(error), file=file)

class eval_register:
    def __init__(self, register):
        self.register = register
        self.offset = 0
        self.isConst = True
    def __add__(self, x):
        tmp = eval_register(self.register)
        tmp.offset = self.offset + x
        while tmp.offset > 0xFFFF:
            tmp.offset -= 0x10000
        return tmp
    __radd__ = __add__            # support for x+t
    def __sub__(self, x):
        tmp = eval_register(self.register)
        tmp.offset = self.offset - x
        while tmp.offset < 0:
            tmp.offset += 0x10000
        return tmp
class value_preprocesser:
    def printError(self, error):
        global printError
        printError(error, self.lineNum)
    def __init__(self, value, lineNum, globalLabel):
        self.globalLabel = globalLabel
        self.lineNum = lineNum
        self.extra = eval_0xSCAmodified(value, {}, preEval=True)
        self.labels = extractVaribles(self.extra, globalLabel)
    def optimize(self, labels):
        return False
    def isConstSize(self):
        return len(self.labels) == 0#False
    def build(self, labels):
        result = None
        try:
            result = self._extraWords(labels)
        except Exception as err:
            self.printError(repr(err))
            return None
        return result
    def _extraWords(self, labels, labelCache=None):
        if self.extra != None:
            eval_locals = {}
            if labelCache == None:
                for x in self.labels:
                    if x in labels:
                        eval_locals[x] = labels[x].getAddress().getAddress()
            else:
                eval_locals.extend(labelCache)
            eval_locals["abs"] = abs
            eval_locals["str"] = str
            eval_locals["hex"] = hex
            return eval_0xSCAmodified(self.extra, eval_locals, self.globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
        return ()
class value_const16:
    def printError(self, error):
        global printError
        printError(error, self.lineNum)
    def __init__(self, value, lineNum, globalLabel):
        self.globalLabel = globalLabel
        self.lineNum = lineNum
        self.extra = eval_0xSCAmodified(value, {}, preEval=True)
        self.labels = extractVaribles(self.extra, globalLabel)
        self.size = None
        self.lastLabels = []
        self.isInt = False

        labels = {}
        for x in self.labels:
            labels[x] = address(address(1))
        try:
            self.size = len(self._extraWords(labels))
        except NameError as err:
            self.printError(repr(err))
        return
    def optimize(self, labels):
        if self.isInt or len(self.labels) == 0:
            return False
        labelCache = {}
        if len(self.lastLabels) == len(labels):
            tmp = True
            for x in self.labels.keys():
                if x not in labels:
                    tmp = False
                    break
                labelCache[x] = labels[x].getAddress().getAddress()
                if (x not in self.lastLabels) or (self.labelCache[x] != self.lastLabels[x]):
                    tmp = False
                    break
            if tmp:
                return False
        old_size = self.size
        self.size = len(self._extraWords(labels, labelCache))
        self.lastLabels = labelCache
        return old_size != self.size
    def isConstSize(self):
        if len(self.labels) == 0:
            return True
        return False
    def build(self, labels):
        return 0x10000
    def sizeExtraWords(self):
        return self.size
    def _extraWords(self, labels, labelCache=None):
        if self.extra != None:
            eval_locals = {}
            if labelCache == None:
                for x in self.labels:
                    if x in labels:
                        eval_locals[x] = labels[x].getAddress().getAddress()
            else:
                eval_locals.extend(labelCache)
            eval_locals["abs"] = abs
            eval_locals["str"] = str
            eval_locals["hex"] = hex
            extra = eval_0xSCAmodified(self.extra, eval_locals, self.globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
            if type(extra) == type("str"):
                return tuple(extra.encode("ascii"))
            if type(extra) == type(wordString(())):
                return tuple(extra)
            if extra != None:
                self.isInt = True
            return (extra,)
        return ()
    def extraWords(self, labels):
        result = None
        try:
            result = self._extraWords(labels)[0:self.size]
        except Exception as err:
            self.printError(repr(err))
            return tuple( [0]*self.size )
        if len(result) < self.size:
            result = list(result)
            result.extend( [0]*(self.size-len(result)) )
            result = tuple(result)
        return result
class value:
    registers = {"a":0x0, "b":0x1, "c":0x2, "x":0x3, "y":0x4, "z":0x5, "i":0x6, "j":0x7}
    cpu = {"POP":0x18, "PEEK":0x19, "PUSH":0x18, "PICK":0x1A, "SP":0x1B, "PC":0x1C, "EX":0x1D}
    def printError(self, error):
        global printError
        printError(error, self.lineNum)
        
    def __init__(self, part, lineNum, globalLabel):
        self.globalLabel = globalLabel
        self.lineNum = lineNum
        registers = value.registers
        cpu = value.cpu
        self.value = None
        self.extra = None
        self.shortLiteral = False
        if type(part) == type(0x42):
            self.value = part & 0x3F
        elif type(part) == type(""):
            if part.startswith("[") and part.endswith("]"):
                part = part[1:-1]
                if part.lower() in registers:
                    self.value = 0x08 | registers[part.lower()]
                elif part.upper() == "SP++":
                    self.value = cpu["POP"]
                elif part.upper() == "--SP":
                    self.value = cpu["PUSH"]
                elif part.upper() == "SP":
                    self.value = cpu["PEEK"]
                elif part.upper() == "[PC++]":
                    self.value = 0x1E
                elif part.upper() == "PC++":
                    self.value = 0x1F
                else:
                    self.value = -1
                    self.extra = part
            elif part.lower() in registers:
                self.value = registers[part.lower()]
            elif part.upper() in cpu:
                self.value = cpu[part.upper()]
            else:
                self.value = 0x1F
                self.extra = part
        else:
            self.value = 0x1F
            self.extra = part
        labels = {}
        self.lastLabels = []
        self.labels = ()
        if self.extra != None:
            self.extra = eval_0xSCAmodified(self.extra, {}, preEval=True)
            self.labels = extractVaribles(self.extra, globalLabel)
        if len(self.labels) == 0:    #We need to force an optimize for those short literals
            self.labels = ["NotReally"]
            self.optimize({"NotReally":address(address(0))})
            self.labels = []
            self.lastLabels = []
    def optimize(self, labels):
        if self.value != 0x1F:
            return False
        if len(self.labels) == 0:
            return False
        #if len(self.labels) > 0 and list(self.labels)[0] != "NotReally":    #Turns off short labels (Unless if program designed to use internal magic values)
        #    return False
        labelCache = {}
        if len(self.lastLabels) == len(labels):
            tmp = True
            for x in self.labels.keys():
                if x not in labels:
                    tmp = False
                    break
                labelCache[x] = labels[x].getAddress().getAddress()
                if (x not in self.lastLabels) or (self.labelCache[x] != self.lastLabels[x]):
                    tmp = False
                    break
            if tmp:
                return False
        self.lastLabels = labelCache
        if self.value == 0x1F:
            extra = self._extraWords(labels)[0]
            if extra != None:
                lastShort = self.shortLiteral
                self.shortLiteral = (extra <= 0x1E or extra == 0xFFFF)
                return self.shortLiteral != lastShort
        return False
    def isConstSize(self):
        if self.value == 0x1F and len(self.labels) != 0:
            return False
        return True
    def sizeExtraWords(self):
        if self.extra != None:
            if self.value == 0x1F and self.shortLiteral:
                return 0
            return 1
        return 0
    def build(self, labels):
        if self.value == -1:
            extra = self._extraWords(labels)[0]
            if extra != None:
                if type(extra) == type(eval_register(0x0)):
                    return extra.register
                else:
                    return 0x1E
            return 0x20 | 0x00
        if self.value == 0x1F and self.shortLiteral:
            tmp = self._extraWords(labels)[0]
            tmp += 1
            if tmp == 0x1000:
                tmp = 0
            return 0x20 | tmp
        return self.value
    def __extraWords(self, labels):
        if self.extra != None:
            eval_locals = {}
            for x in self.labels:
                if x in labels:
                    eval_locals[x] = labels[x].getAddress().getAddress()
            for x in value.registers.keys():
                eval_locals[x.lower()] = eval_register(0x10 | value.registers[x])
                eval_locals[x.upper()] = eval_register(0x10 | value.registers[x])
            eval_locals["SP"] = eval_register(value.cpu["PICK"])
            eval_locals["sP"] = eval_register(value.cpu["PICK"])
            eval_locals["Sp"] = eval_register(value.cpu["PICK"])
            eval_locals["sp"] = eval_register(value.cpu["PICK"])
            eval_locals["abs"] = abs
            extra = eval_0xSCAmodified(self.extra, eval_locals, self.globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
            if type(extra) == type(42):
                while extra > 0xFFFF:
                    extra -= 0x10000
                while extra < 0x0000:
                    extra += 0x10000
            return (extra,)
        return ()
    def _extraWords(self, labels):
        result = (None,)
        try:
            result = self.__extraWords(labels)
        except Exception as err:
            self.printError(repr(err))
            result = (None,)
        return result
    def extraWords(self, labels):
        tmp = self._extraWords(labels)
        if self.value == 0x1F:
            if tmp[0] != None:
                if (tmp[0] <= 0x1E or tmp[0] == 0xFFFF) and self.shortLiteral:
                    return ()
        result = []
        for x in tmp:
            if type(x) == type(eval_register(0x0)):
                result.append(x.offset)
            else:
                result.append(x)
        return tuple(result)
class address:
    def __init__(self, address):
        self.address = address
        self.varibles = []
    def addConst(self, offset):
        self.address += offset
    def addVarible(self, obj):
        self.varibles.append(obj)
    def add(self, obj):
        if obj.isConstSize():
            self.addConst(obj.size())
        else:
            self.addVarible(obj)
    def update(self):
        i = 0
        while i < len(self.varibles):
            var = self.varibles[i]
            if var.isConstSize():
                self.addConst(var.size())
                self.varibles.pop(i)
                i -= 1
            i += 1
    def isConst(self):
        return (len(self.varibles) == 0)
    def getAddress(self):
        self.update()
        offset = self.address
        for var in self.varibles:
            offset += var.size()
        return offset
    def clone(self):
        result = address(self.address)
        for x in self.varibles:
            result.varibles.append(x)
        return result
            
class instruction:
    opcodes = {         "SET":0x01, "ADD":0x02, "SUB":0x03, "MUL":0x04, "MLI":0x05, "DIV":0x06, "DVI":0x07,
            "MOD":0x08, "MDI":0x09, "AND":0x0A, "BOR":0x0B, "XOR":0x0C, "SHR":0x0D, "ASR":0x0E, "SHL":0x0F,
            "IFB":0x10, "IFC":0x11, "IFE":0x12, "IFN":0x13, "IFG":0x14, "IFA":0x15, "IFL":0x16, "IFU":0x17,
                                    "ADX":0x1A, "SBX":0x1B,                         "STI":0x1E, "STD":0x1F,
            "DAT":0x10000, "JMP":0x10001}
    ext_opcodes = {
                        "JSR":0x01,                                                             "HCF":0x07,
            "INT":0x08, "IAG":0x09, "IAS":0x0A, "RFI":0x0B, "IAQ":0x0C,
            "HWN":0x10, "HWQ":0x11, "HWI":0x12 }

    def printError(self, error):
        global printError
        printError(error, self.lineNum)
    def __init__(self, parts, globalLabel, preceding, lineNum):
        self.globalLabel = globalLabel
        self.lineNum = lineNum
        self.a = None
        self.b = None
        self.badRelocate(preceding)
        
        op = parts[0].upper()
        if op not in instruction.opcodes:
            if op in instruction.ext_opcodes:
                self.op = 0
                self.a = value(instruction.ext_opcodes[op], lineNum, globalLabel)
            else:
                self.printError("Invalid opcode \"" + op + "\"")
                self.op = None
                return
        else:
            self.op = instruction.opcodes[op]
        
        if (op in instruction.ext_opcodes or op == "JMP"):
            if len(parts) - 1 != 1:
                self.printError("Opcode \"" + op + "\" requires 1 operand, " + str(len(parts) - 1) + " were given")
                self.op = None
                return
        elif op == "DAT":
            if len(parts) - 1 <= 0:
                self.printError("Opcode \"" + op + "\" requires at least 1 operand, none were given")
                self.op = None
                return
            values = []
            for p in parts[1:]:
                values.append(value_const16(p, lineNum, globalLabel))
            self.values = values
            return
        elif op in instruction.opcodes and len(parts) - 1 != 2:
            self.printError("Opcode \"" + op + "\" requires 2 operands, " + str(len(parts) - 1) + " were given")
            self.op = None
            return
        if op in instruction.ext_opcodes:
            self.b = value(parts[1], lineNum, globalLabel)
            return
        if op == "JMP":
            self.b = value(parts[1], lineNum, globalLabel)
            if self.b.value == 0x1F:
                self.shortJmp = False
                return
            self.op = instruction.opcodes["SET"]
            parts = ["SET", "PC", parts[1]]
            return
        self.a = value(parts[1], lineNum, globalLabel)
        self.b = value(parts[2], lineNum, globalLabel)
    
    #def cycles(self):
    #    return self.size()
    def size(self):
        if self.op == instruction.opcodes["DAT"]:
            result = 0
            for val in self.values:
                result += val.sizeExtraWords()
            return result
        if self.op == instruction.opcodes["JMP"]:
            if self.shortJmp:
                return 1
            return 1 + self.b.sizeExtraWords()
        return 1 + self.a.sizeExtraWords() + self.b.sizeExtraWords()
    def build(self, labels):
        if self.op == instruction.opcodes["DAT"]:
            result = []
            for val in self.values:
                result.extend(val.extraWords(labels))
            return result
        if self.op == instruction.opcodes["JMP"]:
            result = instruction(["SET", "PC", self.b.extra], self.globalLabel, self.preceding, self.lineNum)
            result.b = self.b
            if self.shortJmp and result.size() != 1:
                dest = self.b.extraWords(labels)[0]
                start = self.getAddress().getAddress() + 1
                op = "ADD"
                if dest < start:
                    op = "SUB"
                    dest = (start - dest)
                    start = 0
                result = instruction([op, "PC", repr(dest-start)], self.globalLabel, self.preceding, self.lineNum)
                result.optimize(labels)
                if result.size() != 1:
                    printError("Failed to optimize jmp instruction as reported")
            return result.build(labels)
        result = [self.createInstruction(labels)]
        result.extend(self.b.extraWords(labels))
        result.extend(self.a.extraWords(labels))
        return result
    def createInstruction(self, labels):
        tmp = self.a.build(labels)
        if (tmp & 0x1F) != tmp:
            self.printError("Invalid second operand")
            return 0
        return self.op | (tmp << 5) | (self.b.build(labels) << 10)
    def isConstSize(self):
        if self.op == instruction.opcodes["DAT"]:
            for val in self.values:
                if not val.isConstSize():
                    return False
            return True
        if self.op == instruction.opcodes["JMP"]:
            return False
        return self.a.isConstSize() and self.b.isConstSize()
    def getAddress(self):
        return self.address
    def optimize(self, labels):
        if self.op == instruction.opcodes["DAT"]:
            tmp = False
            for val in self.values:
                tmp = tmp or val.optimize(labels)
            return tmp
        result = self.b.optimize(labels)
        if self.op == instruction.opcodes["JMP"]:
            lastShortJmp = self.shortJmp
            if self.b.sizeExtraWords() > 0:
                dest = self.b.extraWords(labels)[0]
                start = self.getAddress().getAddress() + 1
                self.shortJmp = (-0x1E <= (dest - start) <= 0x1E)
            else:
                self.shortJmp = True
            return result or (lastShortJmp != self.shortJmp)
        return self.a.optimize(labels) or result
    def clone(self):
        result = instruction(("DAT", "\"\""), self.globalLabel, self.preceding, self.lineNum)
        result.op = copy.copy(self.op)
        result.a = copy.deepcopy(self.a)
        result.b = copy.deepcopy(self.b)
        if self.op == "DAT":
            result.values = copy.deepcopy(self.values)
        return result
    def badRelocate(self, preceding):
        self.preceding = preceding
        if preceding != None:
            self.address = preceding.getAddress().clone()
            self.address.add(preceding)
        else:
            self.address = address(0)

class stdwrap:
    def __init__(self, prefix, suffix, file):
        self.prefix = prefix
        self.suffix = suffix
        self.file = file
        self.hadNewline = True
    def write(self, string):
        if self.hadNewline:
            self.file.write(self.prefix)
        lines = string.split("\n")
        self.file.write(lines[0] + self.suffix)
        if len(lines) > 1: self.file.write("\n")
        for x in lines[1:-1]:
            self.file.write(self.prefix + x + self.suffix + "\n")
        if len(lines) > 1:
            if len(lines[-1]) > 0:
                self.file.write(self.prefix)
            self.file.write(lines[-1])
        self.hadNewline = len(lines[-1]) == 0
            
class preprocessor_directive:
    directives = ("align", "echo", "error", "rep", "if", "define", "undefine", "origin", "incbin", "include", "macro", "insert_macro", "label")

    def printError(self, error, file=sys.stderr, lineNum=None):
        global printError
        if lineNum == None:
            lineNum = self.lineNum
        printError(error, lineNum, file=file)
    def __init__(self, parts, globalLabel, preceding, lineNum, labels={}):
        self.globalLabel = globalLabel
        self.lineNum = lineNum
        self.badRelocate(preceding)
        self.directive = ""
        self._size = 0
        self.codeblock = None
        self.codeblockInstance = None
        self.nextIf = None
        self.passIf = False
        self.value = None
        if not (parts[0].startswith(".") or parts[0].startswith("#")):
            self.printError("Preproccessor directives must start with a '.' or '#'")
            return
        if parts[0][1:].lower() == "org":
            parts[0] = ".origin"
        if parts[0][1:].lower() == "def": parts[0] = ".define"
        if parts[0][1:].lower() == "undef": parts[0] = ".undefine"
        if parts[0][1:].lower() == "ifdef":
            parts[0] = ".if"
            parts[1] = "isdef( " + parts[1].strip() + " )"
        if parts[0][1:].lower() == "ifndef":
            parts[0] = ".if"
            parts[1] = "!isdef( " + parts[1].strip() + " )"
        if parts[0][1:].lower() not in self.directives:
            self.printError("Preproccessor directive \"" + parts[0] + "\" does not exist")
            return
        self.directive = parts[0][1:].lower()
        extra = ""
        if self.directive in ("define", "undefine"):
            nParts = []
            for x in parts[1:]:
                if type(x) != type(""):
                    nParts.append(x)
                else:
                    nParts.extend(x.split(" "))
            self.varible = nParts[0]
            if self.directive in ("define"):
                if len(nParts) > 1:
                    if type(nParts[1]) == type(""):
                        extra = ",".join(nParts[1:])
                    else:
                        extra = nParts[1]
                else:
                    extra = "1"
            else:
                extra = "0"
        elif len(parts) >= 2:
            extra = parts[1]
            for i in range(2, len(parts)):
                extra += "," + parts[i]
        if self.directive == "incbin":
            self._size = 0
            if extra.startswith("\"") and extra.endswith("\""):
                self.filepath = extra[1:-1]
                if not os.path.isfile(self.filepath):
                    self.printError("Could not include \""+repr(extra[1:-1])+"\" as the file does not exist")
                    return
            elif extra.startswith("<") and extra.endswith(">"):
                self.filepath = ""
                for x in INCLUDEPATH:
                    filepath = os.path.join(x, extra[1:-1])
                    if os.path.isfile(self.filepath):
                        self.filepath = filepath
                        break
                self.printError("Could not find \""+extra[1:-1]+"\" in the assembler include directories")
                return
            else:
                self.printError("Could not understand the arguments "+repr(extra)+" for incbin")
            _size = os.path.getsize(self.filepath)
            if _size % 2 != 0:
                size += 1
            self._size = int(_size)
            return
        if self.directive == "include":
            self._size = 0
            if extra.startswith("\"") and extra.endswith("\""):
                self.filepath = extra[1:-1]
                if not os.path.isfile(self.filepath):
                    self.printError("Could not include \""+extra[1:-1]+"\" as the file does not exist")
                    return
            elif extra.startswith("<") and extra.endswith(">"):
                self.filepath = ""
                for x in INCLUDEPATH:
                    filepath = os.path.join(x, extra[1:-1])
                    if os.path.isfile(self.filepath):
                        self.filepath = filepath
                        break
                self.printError("Could not find \""+extra[1:-1]+"\" in the assembler include directories")
                return
            else:
                self.printError("Could not understand the arguments "+repr(extra)+" for include")
            oldStdout = sys.stdout
            oldStderr = sys.stderr
            sys.stdout = stdwrap("File "+extra+": ", "", sys.stdout)
            sys.stderr = stdwrap("File "+extra+": ", "", sys.stderr)
            codeblock = reader()
            codeblock.instructions.append(preceding)
            codeblock.globalLabel = globalLabel
            asm = open(self.filepath, 'r')
            codeblock.read(asm)
            asm.close()
            for l in codeblock.labels:
                labels[l] = codeblock.labels[l]
            self.setCodeblock(codeblock.instructions[1:])
            sys.stdout = oldStdout
            sys.stderr = oldStderr
            #We don't need the actual filepath anymore, and this allows both clone() to work
            self.filepath = extra             # and the use of self.filepath for writing errors
            return
        if self.directive == "macro":
            self.extra = extra; operation = None
            try:
                operation = validate( tokenize( extra, ("func",) ) )
            except Exception as e:
                self.printError(e.args[0])
                return
            if type(operation) != type(()) or operation[0] != "func":
                self.printError("Expected macro definition (Ex: \"aMacro()\", \"aMacro(param1, param2)\"")
                return
            self.macroName = operation[1]
            for tokens in operation[2]:
                if type(tokens) == type(()):
                    self.printError("Unexpected function call in macro definition")
                    return False
                elif type(tokens) == type(""):
                    if tokens[0].isdecimal() or (tokens[0] in ("\"", "'") and tokens[-1] in ("\"", "'")):
                        self.printError("Unexpected literal in macro definition")
                        return False
            self.macroArgs = operation[2]
            return
        if self.directive == "insert_macro":
            self.extra = extra; operation = None
            try:
                operation = eval_0xSCAmodified(extra, {}, preEval=True)
            except Exception as e:
                self.printError(e.args[0])
                return
            if type(operation) != type(()) or operation[0] != "func":
                self.printError("Expected macro call (Ex: \"aMacro()\", \"aMacro(param1, param2)\"")
                return
            self.macroName = operation[1]
            macroArgs = []
            for arg in operation[2]:
                macroArgs.append(value_preprocesser(arg, lineNum, globalLabel))
            self.macroArgs = tuple(macroArgs)
            return
        if self.directive == "label":
            self.extra = extra
            return
        self.a = value_preprocesser(extra, lineNum, globalLabel)
    
    #def cycles(self):
    #    return self.size()
    def size(self):
        if self.directive in ("echo", "error", "macro", "label"):
            return 0
        if self.directive in ("align", "incbin"):
            return self._size
        if self.directive == "rep":
            size = 0
            if self.codeblockInstance == None: return 0
            for i in self.codeblockInstance:
                size += i.size()
            return size
        if self.directive == "if":
            if self.passIf:
                if self.nextIf != None:
                    return self.nextIf.size()
                return 0
            if self.codeblock == None: return 0
            size = 0
            for i in self.codeblock:
                size += i.size()
            return size
        if self.directive == "include":
            if self.codeblock == None: return 0
            oldStdout = sys.stdout; sys.stdout = stdwrap("File "+self.filepath+": ", "", sys.stdout)
            oldStderr = sys.stderr; sys.stderr = stdwrap("File "+self.filepath+": ", "", sys.stderr)
            
            size = 0
            for i in self.codeblock:
                size += i.size()
            
            sys.stdout = oldStdout
            sys.stderr = oldStderr
            return size
        if self.directive == "insert_macro":
            if self.codeblock == None: return 0
            size = 0
            for i in self.codeblock:
                size += i.size()
            return size
        return 0
    def build(self, labels):
        if self.directive == "": return ()
        if self.directive == "incbin":
            file = open(self.filepath, 'rb')
            dat = file.read()
            while len(dat) < (self._size*2):
                dat = dat + b'\0'
            obj = []
            for i in range(0, self._size*2, 2):
                tmp = ((dat[i] & 0xFF) << 8) | (dat[i+1] & 0xFF)
                obj.append(tmp)
            file.close()
            return tuple(obj)
        if self.directive == "include":
            if self.codeblock == None: return ()
            oldStdout = sys.stdout; sys.stdout = stdwrap("File "+self.filepath+": ", "", sys.stdout)
            oldStderr = sys.stderr; sys.stderr = stdwrap("File "+self.filepath+": ", "", sys.stderr)
            
            code = []
            for i in self.codeblock:
                code.extend(i.build(labels))
            
            sys.stdout = oldStdout
            sys.stderr = oldStderr
            return tuple(code)
        if self.directive == "align":
            return (0,)*self._size
        if self.directive == "macro":
            labels["$$macro$"+self.macroName] = (self.macroArgs, self.codeblock)
            return ()
        if self.directive == "insert_macro":
            if self.codeblock == None: return ()
            if "$$macro$"+self.macroName not in labels:
                self.printError("Macro "+repr(self.macroName)+" was not defined")
                return
            macro = labels["$$macro$"+self.macroName]
            if len(self.macroArgs) != len(macro[0]):
                self.printError("Macro "+repr(self.macroName)+" is defined for "+str(len(macro[0]))+", not "+str(len(self.macroArgs)))
                return
            labels_ = copy.copy(labels)
            args = {}
            for i in range(len(macro[0])):
                argName = macro[0][i]
                arg = self.macroArgs[i]
                arg.optimize(labels)
                args[argName] = address(address(arg.build(labels)))
                labels_[argName] = args[argName]
            code = []
            for i in self.codeblock:
                code.extend(i.build(labels_))
            for label in labels_.keys():
                if label not in labels:
                    if label not in macro[0] or labels_[label] != args[label]:
                        labels[label] = labels_[label]
            return tuple(code)
        if self.directive == "label":
            labels["$$labels"][self.extra] = self
            return ()
        value = self.a.build(labels)
        if self.directive == "echo":
            self.printError(value, file=sys.stdout)
        if self.directive == "error":
            self.printError(value)
            raise Exception("User-generated error")
        if self.directive == "rep":
            code = []
            if self.codeblockInstance == None: return ()
            for i in self.codeblockInstance:
                code.extend(i.build(labels))
            return tuple(code)
        if self.directive == "if":
            if self.passIf:
                if self.nextIf != None:
                    return self.nextIf.build(labels)
                return ()
            if self.codeblock == None: return ()
            code = []
            for i in self.codeblock:
                code.extend(i.build(labels))
            return tuple(code)
        if self.directive == "define":
            labels[self.varible] = address(address(value))
        if self.directive == "undefine":
            try:
                labels.pop(self.varible)
            except KeyError:
                pass
        if self.directive == "origin":
            labels["$$origin"] = address(address(value))
        return ()
    def isConstSize(self):
        if self.directive == "include":
            if self.codeblock == None: return True
            for i in self.codeblock:
                if i.isConstSize(): return True
            return False
        return self.directive in ("echo", "error", "define", "undefine", "origin", "incbin", "macro", "label")
    def getAddress(self):
        return self.address
    def optimize(self, labels):
        if self.directive == "": return False
        if self.directive in ("echo", "error", "incbin"):
            return False
        if self.directive == "include":
            oldStdout = sys.stdout; sys.stdout = stdwrap("File "+self.filepath+": ", "", sys.stdout)
            oldStderr = sys.stderr; sys.stderr = stdwrap("File "+self.filepath+": ", "", sys.stderr)
            result = self.optCodeblock(labels)
            sys.stdout = oldStdout
            sys.stderr = oldStderr
            return result
        if self.directive == "macro":
            labels["$$macro$"+self.macroName] = (self.macroArgs, self.codeblock)
            return False
        if self.directive == "insert_macro":
            if "$$macro$"+self.macroName not in labels:
                self.printError("Macro "+repr(self.macroName)+" was not defined")
                return
            macro = labels["$$macro$"+self.macroName]
            if len(self.macroArgs) != len(macro[0]):
                self.printError("Macro "+repr(self.macroName)+" is defined for "+str(len(macro[0]))+", not "+str(len(self.macroArgs)))
                return
            labels_ = copy.copy(labels)
            args = {}
            for i in range(len(macro[0])):
                argName = macro[0][i]
                arg = self.macroArgs[i]
                arg.optimize(labels)
                args[argName] = address(address(arg.build(labels)))
                labels_[argName] = args[argName]
            codeblock = []
            for code in macro[1]:
                codeblock.append(code.clone())
            self.codeblock = codeblock
            while self.optCodeblock(labels_): magic=42 #magic=42 doesn't actually do anything, unlike more magic
            for label in labels_.keys():
                if label not in labels:
                    if label not in macro[0] or labels_[label] != args[label]:
                        labels[label] = labels_[label]
            return False
        if self.directive == "label":
            labels["$$labels"][self.extra] = self
            return False
        result = self.a.optimize(labels)
        if self.directive == "align":
            tmp = self.a.build(labels)
            if type(tmp) != type(42):
                self.printError("Can't align to an non-int boundary")
                return False
            lastSize = self._size
            self._size = tmp - (self.getAddress().getAddress() % tmp)
            return (lastSize != self._size) or result
        if self.directive == "rep":
            tmp = self.a.build(labels)
            if type(tmp) != type(42):
                self.printError("Can't repeat an non-int amount of times")
                return False
            lastSize = self._size
            self._size = tmp
            if lastSize != self._size:
                if self._size < lastSize:
                    self.codeblockInstance = self.codeblockInstance[:self._size]
                else:
                    instance = []
                    for foo in range(self._size):
                        for code in self.codeblock:
                            i = code.clone()
                            if len(instance) == 0:
                                i.badRelocate(self.preceding)
                            else:
                                i.badRelocate(instance[-1])
                            instance.append(i)
                    self.codeblockInstance = tuple(instance)
            return (lastSize != self._size) or result or self.optCodeblockInstance(labels)
        if self.directive == "if":
            tmp = self.a.build(labels)
            if not (type(tmp) == type(42) or type(tmp) == type(False)):
                self.printError("Can't compare an non-int amount with 0")
                return False
            lastPass = self.passIf
            self.passIf = (tmp == 0)
            if self.passIf:
                result = self.optCodeblock(copy.copy(labels))
            else:
                result = self.optCodeblock(labels)
            if self.nextIf != None:
                if self.passIf:
                    result = self.nextIf.optimize(labels) or result
                else:
                    result = self.nextIf.optimize(copy.copy(labels)) or result
            return (lastPass != self.passIf) or result
        if self.directive == "define":
            lastValue = self.value
            self.value = self.a.build(labels)
            labels[self.varible] = address(address(self.value))
            return (lastValue != self.value)
        if self.directive == "undefine":
            try:
                labels.pop(self.varible)
            except KeyError:
                pass
            return False
        if self.directive == "origin":
            labels["$$origin"] = address(address(self.a.build(labels)))
            return False
        return result
    def optCodeblock(self, labels):
        if self.codeblock == None: return False
        result = False
        for i in self.codeblock:
            result = i.optimize(labels) or result
        return result
    def optCodeblockInstance(self, labels):
        if self.codeblockInstance == None: return False
        result = False
        for i in self.codeblockInstance:
            result = i.optimize(labels) or result
        return result
    def setCodeblock(self, codeblock):
        if self.directive == "if" and self.codeblock != None:
            if self.nextIf == None:
                printError("Unneeded codeblock given to .if statement", lineNum=("~"+self.lineNum))
                return
            return self.nextIf.setCodeblock(codeblock)
        self.codeblock = codeblock
    def needsCodeblock(self):
        if self.directive == "rep":
            return (self.codeblock == None)
        if self.directive == "if":
            if self.codeblock == None: return True
            if self.nextIf == None: return False
            return self.nextIf.needsCodeblock()
        if self.directive == "macro":
            return (self.codeblock == None)
        return False
    def endCodeblock(self, parts, lineNum):
        if not (parts[0].startswith(".") or parts[0].startswith("#")):
            printError("Directives must start with a . or #")
            return
        if self.directive == "rep":
            if parts[0][1:].lower() != "end":
                printError("Can't end .rep codeblock with a directive besides .end", lineNum=lineNum)
                return
        elif self.directive == "if":
            if self.nextIf != None:
                return self.nextIf.endCodeblock(parts, lineNum)
            if parts[0][1:].lower() == "elif":
                parts[0] = ".elseif"
            if parts[0][1:].lower() == "elseif":
                parts[0] = ".if"
                self.nextIf = preprocessor_directive(parts, self.globalLabel, self.preceding, lineNum)
            elif parts[0][1:].lower() == "else":
                parts[0] = ".rep"
                parts.insert(1, "1")
                self.nextIf = preprocessor_directive(parts, self.globalLabel, self.preceding, lineNum)
            elif parts[0][1:].lower() == "end":
                self.nextIf = None
            else:
                printError("Can't use \"" + parts[0] + "\" to end .if directive", lineNum=lineNume)
        elif self.directive == "macro":
            if parts[0][1:].lower() != "end":
                printError("Can't end .macro codeblock with a directive besides .end", lineNum=lineNum)
                return
    def clone(self):
        result = None
        if self.directive in ("define", "undefine"):
            result = preprocessor_directive(("." + self.directive, self.varible, self.a.extra), self.globalLabel, self.preceding, self.lineNum)
        elif self.directive in ("incbin"):
            result = preprocessor_directive(("." + self.directive, "\""+self.filepath+"\""), self.globalLabel, self.preceding, self.lineNum)
        elif self.directive in ("include"):
            result = preprocessor_directive(("." + self.directive, self.filepath), self.globalLabel, self.preceding, self.lineNum)
        elif self.directive in ("macro", "insert_macro", "label"):
            result = preprocessor_directive(("." + self.directive, self.extra), self.globalLabel, self.preceding, self.lineNum)
        else:
            result = preprocessor_directive(("." + self.directive, self.a.extra), self.globalLabel, self.preceding, self.lineNum)
        if self.codeblock != None and result.needsCodeblock():
            codeblock = []
            for code in self.codeblock:
                codeblock.append(code.clone())
            result.codeblock = tuple(codeblock)
        if self.nextIf != None:
            result.nextIf = self.nextIf.clone()
        result.passIf = self.passIf
        return result
    def badRelocate(self, preceding):
        self.preceding = preceding
        if preceding != None:
            self.address = preceding.getAddress().clone()
            self.address.add(preceding)
        else:
            self.address = address(0)

class origin:
    def __init__(self, origin):
        self.address = address(0)
        self.origin = origin
    def isConstSize(self):
        return False
    def size(self):
        return self.origin
    def setOrigin(self,origin):
        self.origin = origin
    def getOrigin(self):
        return self.origin
    def getAddress(self):
        return self.address
class reader:
    def __init__(self):
        self.instructions = []
        self.parts = []
        self.labels = {}
        self.cur_labels = []
        self.isLabel = False
        self.current = ""
        self.inString = False
        self.reading = True
        self.lineNum = 1
        self.globalLabel = ""
        self.strChar = ""
        self.inCodeblock = False
        self.codeblock = None
        self.origin = origin(0)
    def readChar(self, char):
        if char == ";" and not self.inString:
            self.readChar(",")
            self.reading = False
        elif char == "\n":
            if self.inString:
                if self.inCodeblock: self.codeblock.readLine("")
                printError("Expected a " + self.strChar + " before end of line", self.lineNum)
                self.current = ""
                self.inString = False
            else:
                self.readChar(",")
                if len(self.parts) > 0:
                    preceding = self.origin
                    if len(self.instructions) > 0:
                        preceding = self.instructions[-1]
                    tmp = None
                    if self.inCodeblock:
                        if (self.parts[0].startswith(".") or self.parts[0].startswith("#")) and \
                                          self.parts[0][1:].lower() in ("end", "elseif", "elif", "else") and (not self.codeblock.inCodeblock):
                            #if self.parts[0][1:].lower() == "end":
                            self.inCodeblock = False
                            if len(self.codeblock.labels) + len(self.codeblock.cur_labels) > 0:
                                printError("Labels within codeblocks are not supported", "~" + str(self.lineNum))
                            preceding.setCodeblock( self.codeblock.instructions[1:] )
                            preceding.endCodeblock(self.parts, self.lineNum)
                            if preceding.needsCodeblock():
                                self.inCodeblock = True
                                self.codeblock = reader()
                                self.codeblock.instructions.append(self.instructions[-2])
                                self.codeblock.globalLabel = self.globalLabel
                                self.codeblock.lineNum = self.lineNum+1
                        else:
                            self.codeblock.readLine(",".join(self.parts))
                    else:
                        if self.parts[0].startswith(".") or self.parts[0].startswith("#"):
                            tmp = preprocessor_directive(self.parts, self.globalLabel, preceding, self.lineNum, self.labels)
                            if tmp.needsCodeblock():
                                self.inCodeblock = True
                                self.codeblock = reader()
                                self.codeblock.instructions.append(preceding)
                                self.codeblock.globalLabel = self.globalLabel
                                self.codeblock.lineNum = self.lineNum+1
                        else:
                            if "(" in self.parts[0]:    #Prob a macro insertion
                                nParts = [".insert_macro"]
                                nParts.extend(self.parts)
                                tmp = preprocessor_directive(nParts, self.globalLabel, preceding, self.lineNum, self.labels)
                            else:
                                tmp = instruction(self.parts, self.globalLabel, preceding, self.lineNum)
                        self.instructions.append(tmp)
                        for label in self.cur_labels:
                            self.labels[label] = self.instructions[-1]
                        self.cur_labels = []
                else:
                    if self.inCodeblock: self.codeblock.readLine("")
            self.lineNum += 1
            self.parts = []
            self.reading = True
        elif not self.reading:
            return
        elif char == ":" and not self.inString:
            self.isLabel = True
            if len(self.parts) > 0:
                printError("Unexpected Label")
            self.parts = []
        elif char.isspace() and len(self.parts) == 0 and not self.inString:
            self.readChar(",")                
        elif char == "," and not self.inString:
            newLabel = None
            if self.current != "":
                if self.isLabel:
                    tmp = True
                    local = False
                    if self.current[0] == "_":
                        self.current = self.current[1:]
                        local = True
                    for c in self.current:
                        if not (c.isalnum() or c == "_"):
                            tmp = False
                            break
                    if tmp:
                        if local:
                            self.current = self.globalLabel + "$" + self.current
                        else:
                            self.globalLabel = self.current
                        self.cur_labels.append(self.current)
                        newLabel = self.current
                    else:
                        if local: self.current = "_" + self.current
                        printError("Invalid label \"" + self.current + "\", labels names may only consist of alphanumeric characters and underscores", self.lineNum)
                else:
                    self.parts.append(self.current.strip())
            self.isLabel = False
            self.current = ""
            if newLabel != None:
                self.readLine(".label " + str(newLabel))
        elif (char == "\"" or char == "'") and self.strChar in ["", char]:
            if self.isLabel:
                printError("Unexpected String")
                self.isLabel = False
            self.inString = not self.inString
            if self.inString:
                self.strChar = char
            else:
                self.strChar = ""
            self.current += char
        else:
            self.current += char
    def readLine(self, line):
        for c in line:
            self.readChar(c)
        if len(line) == 0 or line[-1] != "\n":
            self.readChar("\n")
    def read(self, file):
        line = file.readline()
        while line != "":
            self.readLine(line)
            line = file.readline()
        if len(self.cur_labels) > 0:
            self.readLine("SET A, A")
            self.instructions.pop(-1)

class writer:
    def __init__(self, instructions, labels, origin):
        self.instructions = instructions
        if "$$origin" not in labels:
            labels["$$origin"] = address(address(0))
        self.labels = labels
        self.origin = origin
    def splitWord(self, i):
        return [(i & 0xFF00) >> 8, i & 0x00FF]
    def genObj(self):
        wasOptimized = True
        labels = {}
        labels["$$labels"] = copy.copy(self.labels)
        labels["$$origin"] = labels["$$labels"].pop("$$origin")
        while wasOptimized:
            wasOptimized = False
            tmp = labels
            labels = labels["$$labels"]
            labels["$$labels"] = {}
            labels["$$origin"] = tmp["$$origin"]
            for x in self.instructions:
                wasOptimized = x.optimize(labels) or wasOptimized
            newOrigin = labels["$$origin"].getAddress().getAddress()
            if self.origin.getOrigin() != newOrigin:
                wasOptimized = True
                self.origin.setOrigin(newOrigin)
        obj = []
        tmp = labels
        labels = labels["$$labels"]
        labels["$$labels"] = {}
        labels["$$origin"] = tmp["$$origin"]
        for x in self.instructions:
            code = x.build(labels)
            for d in code:
                obj.extend(self.splitWord(d))
        return bytes(obj)
    def writeFile(self, file):
        file.write(self.genObj())
def main(argv):
    asm = open(argv[0], 'r')
    read = reader()
    read.read(asm)  #TODO: Preprocessor, handle labels within code blocks, support @
    asm.close()
    obj = open(argv[1], 'wb')
    write = writer(read.instructions, read.labels, read.origin)
    write.writeFile(obj)
    obj.close()

if __name__ == "__main__":
    main(sys.argv[1:])
#main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
