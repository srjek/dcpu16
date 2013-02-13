from assembler import instruction, address

class dcpu16_instruction(instruction):
    opcodes = {         "SET":0x01, "ADD":0x02, "SUB":0x03, "MUL":0x04, "MLI":0x05, "DIV":0x06, "DVI":0x07,
            "MOD":0x08, "MDI":0x09, "AND":0x0A, "BOR":0x0B, "XOR":0x0C, "SHR":0x0D, "ASR":0x0E, "SHL":0x0F,
            "IFB":0x10, "IFC":0x11, "IFE":0x12, "IFN":0x13, "IFG":0x14, "IFA":0x15, "IFL":0x16, "IFU":0x17,
                                    "ADX":0x1A, "SBX":0x1B,                         "STI":0x1E, "STD":0x1F,
            "DAT":0x10000, "JMP":0x10001}
    ext_opcodes = {
                        "JSR":0x01,                                                             "HCF":0x07,
            "INT":0x08, "IAG":0x09, "IAS":0x0A, "RFI":0x0B, "IAQ":0x0C,
            "HWN":0x10, "HWQ":0x11, "HWI":0x12 }
    ops = []
    ops.extend(opcodes.keys())
    ops.extend(ext_opcodes.keys())

    def printError(self, error):
        global printError
        printError(error, self.lineNum)
    def __init__(self, parts, preceding, lineNum):
        self.lineNum = lineNum
        self.a = None
        self.b = None
        self.badRelocate(preceding)
        
        op = parts[0].upper()
        if op not in dcpu16_instruction.opcodes:
            if op in dcpu16_instruction.ext_opcodes:
                self.op = 0
                self.a = value(instruction.ext_opcodes[op], lineNum, False)
            else:
                self.printError("Invalid opcode \"" + op + "\"")
                self.op = None
                return
        else:
            self.op = dcpu16_instruction.opcodes[op]
        
        if (op in dcpu16_instruction.ext_opcodes or op == "JMP"):
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
                values.append(value_const16(p, lineNum))
            self.values = values
            return
        elif op in dcpu16_instruction.opcodes and len(parts) - 1 != 2:
            self.printError("Opcode \"" + op + "\" requires 2 operands, " + str(len(parts) - 1) + " were given")
            self.op = None
            return
        if op in dcpu16_instruction.ext_opcodes:
            self.b = value(parts[1], lineNum, True)
            return
        if op == "JMP":
            self.b = value(parts[1], lineNum, True)
            if self.b.value == 0x1F:
                self.shortJmp = False
                return
            self.op = dcpu16_instruction.opcodes["SET"]
            parts = ["SET", "PC", parts[1]]
            return
        self.a = value(parts[1], lineNum, False)
        self.b = value(parts[2], lineNum, True)
    
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
            result = instruction(["SET", "PC", self.b.extra], self.preceding, self.lineNum)
            result.b = self.b
            if self.shortJmp and result.size() != 1:
                dest = self.b.extraWords(labels)[0]
                start = self.getAddress().getAddress() + 1
                op = "ADD"
                if dest < start:
                    op = "SUB"
                    dest = (start - dest)
                    start = 0
                result = instruction([op, "PC", repr(dest-start)], self.preceding, self.lineNum)
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
        if (self.b.build(labels) & 0x3F) != self.b.build(labels):
            self.printError("self.b.build() returned "+repr(self.b.build(labels))+", which is outside acceptable bounds")
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
        result = instruction(("DAT", "\"\""), self.preceding, self.lineNum)
        result.op = copy.copy(self.op)
        result.a = copy.deepcopy(self.a)
        result.b = copy.deepcopy(self.b)
        if self.op == instruction.opcodes["DAT"]:
            result.values = copy.deepcopy(self.values)
        return result
    def badRelocate(self, preceding):
        self.preceding = preceding
        if preceding != None:
            self.address = preceding.getAddress().clone()
            self.address.add(preceding)
        else:
            self.address = address(0)
