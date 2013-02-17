from assembler import instruction, address, printError
from mathEval import eval_0xSCAmodified, wordString, extractVaribles, tokenize, validate
import copy

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
class value_const16:
    def printError(self, error):
        self.parent.printError(error, lineNum=self.lineNum)
    def __init__(self, value, lineNum, parent):
        self.lineNum = lineNum
        self.parent = parent
        try:
            self.extra = eval_0xSCAmodified(value, {}, preEval=True)
            self.labels = extractVaribles(self.extra, "")
        except Exception as err:
            self.printError(repr(err))
            return
        self.size = None
        self.lastLabels = []
        self.isInt = False

        labels = {"$$globalLabel":""}
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
            globalLabel = labels["$$globalLabel"]
            tmp = True
            for key in self.labels.keys():
                x = key
                if key.startswith("$"):
                    x = globalLabel+key
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
            globalLabel = labels["$$globalLabel"]
            eval_locals = {}
            if labelCache == None:
                for x in self.labels:
                    if x.startswith("$") and not x.startswith("$$"):
                        if (globalLabel+x) in labels:
                            eval_locals[globalLabel+x] = labels[globalLabel+x].getAddress().getAddress()
                    else:
                        if x in labels:
                            eval_locals[x] = labels[x].getAddress().getAddress()
            else:
                eval_locals.extend(labelCache)
            eval_locals["abs"] = abs
            eval_locals["str"] = str
            eval_locals["hex"] = hex
            eval_locals["$$curAddress"] = self.parent
            extra = eval_0xSCAmodified(self.extra, eval_locals, globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
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
    def clone(self):
        return value_const16(copy.deepcopy(self.extra), self.lineNum, self.parent)
        
class value:
    registers = {"a":0x0, "b":0x1, "c":0x2, "x":0x3, "y":0x4, "z":0x5, "i":0x6, "j":0x7}
    cpu = {"POP":0x18, "PEEK":0x19, "PUSH":0x18, "PICK":0x1A, "SP":0x1B, "PC":0x1C, "EX":0x1D}
    def printError(self, error):
        self.parent.printError(error, lineNum=self.lineNum)
        
    def __init__(self, part, lineNum, parent, allowShortLiteral):
        self.lineNum = lineNum
        self.parent = parent
        registers = value.registers
        cpu = value.cpu
        self.value = None
        self.extra = None
        self.shortLiteral = False
        self.allowShortLiteral = allowShortLiteral
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
                elif part.upper() == "SP+[PC++]":
                    self.value = cpu["PICK"]
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
            try:
                self.extra = eval_0xSCAmodified(self.extra, {}, preEval=True)
                self.labels = extractVaribles(self.extra, "")
            except Exception as err:
                self.printError(repr(err))
                return
        if len(self.labels) == 0:    #We need to force an optimize for those short literals
            self.labels = ["NotReally"]
            self.optimize({"NotReally":address(address(0)), "$$globalLabel":""})
            self.labels = []
            self.lastLabels = []
    def optimize(self, labels):
        if self.value != 0x1F:
            return False
        if len(self.labels) == 0:
            return False
        if self.extra == None:
            return False
        #if len(self.labels) > 0 and list(self.labels)[0] != "NotReally":    #Turns off short labels (Unless if program designed to use internal magic values)
        #    return False
        labelCache = {}
        if len(self.lastLabels) == len(labels):
            tmp = True
            for key in self.labels.keys():
                x = key
                if key.startswith("$"):
                    x = globalLabel+key
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
        if self.value == 0x1F and self.allowShortLiteral:
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
            if tmp == 0x10000:
                tmp = 0
            return 0x20 | tmp
        return self.value
    def __extraWords(self, labels):
        if self.extra != None:
            globalLabel = labels["$$globalLabel"]
            eval_locals = {}
            for x in self.labels:
                if x.startswith("$") and not x.startswith("$$"):
                    if (globalLabel+x) in labels:
                        eval_locals[globalLabel+x] = labels[globalLabel+x].getAddress().getAddress()
                else:
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
            eval_locals["$$curAddress"] = self.parent
            extra = eval_0xSCAmodified(self.extra, eval_locals, globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
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
        if self.value == 0x1F and self.extra != None:
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
    def clone(self):
        result = value("A", self.lineNum, self.parent, self.allowShortLiteral)
        result.value = self.value
        result.extra = copy.deepcopy(self.extra)
        result.labels = copy.deepcopy(self.labels)
        result.shortLiteral = self.shortLiteral
        return result

class dcpu16_instruction(instruction):
    opcodes = {         "SET":0x01, "ADD":0x02, "SUB":0x03, "MUL":0x04, "MLI":0x05, "DIV":0x06, "DVI":0x07,
            "MOD":0x08, "MDI":0x09, "AND":0x0A, "BOR":0x0B, "XOR":0x0C, "SHR":0x0D, "ASR":0x0E, "SHL":0x0F,
            "IFB":0x10, "IFC":0x11, "IFE":0x12, "IFN":0x13, "IFG":0x14, "IFA":0x15, "IFL":0x16, "IFU":0x17,
                                    "ADX":0x1A, "SBX":0x1B,                         "STI":0x1E, "STD":0x1F }
    ext_opcodes = {
                        "JSR":0x01,                                                             "HCF":0x07,
            "INT":0x08, "IAG":0x09, "IAS":0x0A, "RFI":0x0B, "IAQ":0x0C,
            "HWN":0x10, "HWQ":0x11, "HWI":0x12 }
    ops = []
    ops.extend(opcodes.keys())
    ops.extend(ext_opcodes.keys())

    def __init__(self, parts, preceding, lineNum, fileName, reader):
        super().__init__(parts, preceding, lineNum, fileName, reader)
        self.a = None
        self.b = None
        
        op = parts[0].upper()
        if op not in dcpu16_instruction.opcodes:
            if op in dcpu16_instruction.ext_opcodes:
                self.op = 0
                self.a = value(dcpu16_instruction.ext_opcodes[op], lineNum, self, False)
            else:
                self.printError("Invalid opcode \"" + op + "\"")
                self.op = None
                return
        else:
            self.op = dcpu16_instruction.opcodes[op]
        
        if (op in dcpu16_instruction.ext_opcodes):
            if len(parts) - 1 != 1:
                self.printError("Opcode \"" + op + "\" requires 1 operand, " + str(len(parts) - 1) + " were given")
                self.op = None
                return
        elif op in dcpu16_instruction.opcodes and len(parts) - 1 != 2:
            self.printError("Opcode \"" + op + "\" requires 2 operands, " + str(len(parts) - 1) + " were given")
            self.op = None
            return
        if op in dcpu16_instruction.ext_opcodes:
            self.b = value(parts[1], lineNum, self, True)
            return
        self.a = value(parts[1], lineNum, self, False)
        self.b = value(parts[2], lineNum, self, True)
    
    def size(self):
        return 1 + self.a.sizeExtraWords() + self.b.sizeExtraWords()
    def build(self, labels):
        result = [self.createInstruction(labels)]
        result.extend(self.b.extraWords(labels))
        result.extend(self.a.extraWords(labels))
        return result
    def createInstruction(self, labels):
        tmpA = self.a.build(labels)
        if (tmpA & 0x1F) != tmpA:
            self.printError("self.a.build() returned "+repr(self.b.build(labels))+", which is outside acceptable bounds. Please report this bug.")
            return 0
        tmpB = self.b.build(labels)
        if (tmpB & 0x3F) != tmpB:
            self.printError("self.b.build() returned "+repr(self.b.build(labels))+", which is outside acceptable bounds. Please report this bug.")
            return 0
        return self.op | (tmpA << 5) | (tmpB << 10)
    def isConstSize(self):
        return self.a.isConstSize() and self.b.isConstSize()
    def optimize(self, labels):
        result = self.b.optimize(labels)
        return self.a.optimize(labels) or result
    def clone(self):
        result = dcpu16_instruction(("SET", "0", "0"), self.preceding, self.lineNum, self.fileName, self.reader)
        result.op = copy.copy(self.op)
        if self.a != None:
            result.a = self.a.clone()
            result.a.parent = result
        if self.b != None:
            result.b = self.b.clone()
            result.b.parent = result
        return result

class dcpu16_jmp(instruction):
    def __init__(self, parts, preceding, lineNum, fileName, reader):
        super().__init__(parts, preceding, lineNum, fileName, reader)
        
        self.b = None
        self.shortJmp = True
        op = parts[0].upper()
        
        if len(parts) - 1 != 1:
            self.printError("Opcode \"" + op + "\" requires 1 operand, " + str(len(parts) - 1) + " were given")
            self.op = None
            return
            
        self.b = value(parts[1], lineNum, self, True)
        if self.b.value == 0x1F:
            self.shortJmp = False
    
    def size(self):
        if self.b == None:
            return 0
        if self.shortJmp:
            return 1
        return 1 + self.b.sizeExtraWords()
    def build(self, labels):
        if self.b == None:
            return ()
        result = dcpu16_instruction(["SET", "PC", self.b.extra], self.preceding, self.lineNum, self.fileName, self.reader)
        result.b = self.b
        if self.shortJmp and result.size() != 1:
            dest = self.b.extraWords(labels)[0]
            start = self.getAddress().getAddress() + 1
            op = "ADD"
            tmp = start ^ dest
            if (tmp == 0xFFFF) or (0 <= tmp <= 0x1E):
                op = "XOR"
                dest = tmp
                start = 0
            elif dest < start:
                op = "SUB"
                dest = (start - dest)
                start = 0
            result = dcpu16_instruction([op, "PC", repr(dest-start)], self.preceding, self.lineNum, self.fileName, self.reader)
            result.optimize(labels)
            if result.size() != 1:
                printError("Failed to optimize jmp instruction as reported")
        return result.build(labels)
    def isConstSize(self):
        if self.b == None:
            return True
        return False
    def optimize(self, labels):
        if self.b == None:
            return False
        result = self.b.optimize(labels)
        lastShortJmp = self.shortJmp
        if self.b.sizeExtraWords() > 0:
            dest = self.b.extraWords(labels)[0]
            start = self.getAddress().getAddress() + 1
            tmp = start ^ dest
            if (tmp == 0xFFFF) or (0 <= tmp <= 0x1E):
                self.shortJmp = True
            else:
                self.shortJmp = (-0x1E <= (dest - start) <= 0x1E)
        else:
            self.shortJmp = True
        return result or (lastShortJmp != self.shortJmp)
    def clone(self):
        result = dcpu16_instruction(("JMP", "0"), self.preceding, self.lineNum, self.fileName, self.reader)
        result.op = copy.copy(self.op)
        if self.b != None:
            result.b = self.b.clone()
            result.b.parent = result
        else:
            result.b = None
        return result

class dat16(instruction):
    def __init__(self, parts, preceding, lineNum, fileName, reader):
        super().__init__(parts, preceding, lineNum, fileName, reader)
        
        op = parts[0].upper()
        self.values = []
        
        if len(parts) - 1 <= 0:
            self.printError("Opcode \"" + op + "\" requires at least 1 operand, none were given")
            self.op = None
            return
        values = []
        for p in parts[1:]:
            values.append(value_const16(p, lineNum, self))
        self.values = values
    
    def size(self):
        result = 0
        for val in self.values:
            result += val.sizeExtraWords()
        return result
    def build(self, labels):
        result = []
        for val in self.values:
            result.extend(val.extraWords(labels))
        return result
    def isConstSize(self):
        for val in self.values:
            if not val.isConstSize():
                return False
        return True
    def optimize(self, labels):
        result = False
        for val in self.values:
            result = result or val.optimize(labels)
        return result
    def clone(self):
        result = dcpu16_instruction(("DAT", "0"), self.preceding, self.lineNum, self.fileName, self.reader)
        values = []
        for v in self.values:
            values.append(v.clone())
        result.values = values
        return result

def registerOps(reader):
    for op in dcpu16_instruction.ops:
        reader.registerOp(op, dcpu16_instruction)
    reader.registerOp("DAT", dat16)
    reader.registerOp("JMP", dcpu16_jmp)