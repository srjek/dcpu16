""" @package assembler
@todo this description
"""

import sys
import copy
import string
import os.path
from mathEval import eval_0xSCAmodified, wordString, extractVaribles, tokenize, validate

## List of directories to search when including standard assembly files.
#  @todo Check that this is actually used by the assembler
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
    def __init__(self, value, lineNum):
        self.lineNum = lineNum
        try:
            self.extra = eval_0xSCAmodified(value, {}, preEval=True)
            self.labels = extractVaribles(self.extra, "")
        except Exception as err:
            self.printError(repr(err))
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
            return eval_0xSCAmodified(self.extra, eval_locals, globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
        return ()
class value_const16:
    def printError(self, error):
        global printError
        printError(error, self.lineNum)
    def __init__(self, value, lineNum):
        self.lineNum = lineNum
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
class value:
    registers = {"a":0x0, "b":0x1, "c":0x2, "x":0x3, "y":0x4, "z":0x5, "i":0x6, "j":0x7}
    cpu = {"POP":0x18, "PEEK":0x19, "PUSH":0x18, "PICK":0x1A, "SP":0x1B, "PC":0x1C, "EX":0x1D}
    def printError(self, error):
        global printError
        printError(error, self.lineNum)
        
    def __init__(self, part, lineNum, allowShortLiteral):
        self.lineNum = lineNum
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

class address:
    """Represents a position in the resulting binary.
    
    This class keeps track of an address that may change as previous instructions because larger or smaller.
    """
    
    def __init__(self, address):
        """Creates an initial representation of a static address.
        @param address An integer representing the static base address to use.
        """
        self.address = address
        self.varibles = []
    def addConst(self, offset):
        """Alters the static base address
        @param offset An integer representing the offset to apply
        """
        self.address += offset
    def addVarible(self, obj):
        """Adds an instruction with a dynamic size to keep track of
        @param obj An instruction
        """
        self.varibles.append(obj)
    def add(self, obj):
        """Adds an instruction
        @param obj An instruction
        """
        if obj.isConstSize():
            self.addConst(obj.size())
        else:
            self.addVarible(obj)
    def update(self):
        """Optimizes future address resolutions."""
        i = 0
        while i < len(self.varibles):
            var = self.varibles[i]
            if var.isConstSize():
                self.addConst(var.size())
                self.varibles.pop(i)
                i -= 1
            i += 1
    def isConst(self):
        """Returns true if the address is static, will only change during an `update()`."""
        return (len(self.varibles) == 0)
    def getAddress(self):
        """Resolves the current address to an integer, also calls `update()`."""
        self.update()
        offset = self.address
        for var in self.varibles:
            offset += var.size()
        return offset
    def clone(self):
        """Creates an independent copy of the current address"""
        result = address(self.address)
        for x in self.varibles:
            result.varibles.append(x)
        return result

class instruction:
    """Interface used by the assembler to work with instructions.
    
    Notes:
        * The assembler expects implementations of the size(), isConstSize(), build(), optimize(), and clone() functions.
        * The provided `__init__()` is how the assembler will create any instruction-based class. Children should provide an `__init__()` with similar args, and call this class's `__init__()`
        * `printError()` is provided as a helper function for any children.
        * `getAddress()` is expected by the assembler, and should not be re-implemented unless neccesary.
    """
    
    def printError(self, error, lineNum=None, file=sys.stderr):
        """Prints the line number and a message to stderr."""
        global printError
        if lineNum == None:
            lineNum = self.lineNum
        printError(error, lineNum, file=file)
    
    def __init__(self, parts, preceding, lineNum):
        """Initalizes data common to most if not all instructions.
        
        In particular,
            * self.lineNum is set, for use by `printError()`.
            * self.address is set, by relocating the instruction to the position after `preceding`.
        
        `__init__()` should be called by children's `__init__()`, and children should provide an `__init__()` with similar args.
        
        @param parts An array of strings, representing an instruction name, following by it's arguments in order. Not used by interface's `__init__()`.
        @param preceding The previous instruction in the program. Used to determine `self.address`.
        @param lineNum Line number the instruction originated from. Stored in `self.lineNum`.
        """
        
        ##Line number of opposing force
        self.lineNum = lineNum
        self.badRelocate(preceding)
    
    def getAddress(self):
        """Returns an address object representing the address of this instruction."""
        return self.address
    def size(self):
        """Returns the size of this instruction, in 16bit words."""
        return 0
    def isConstSize(self):
        """Returns True only if the current instruction will never change size in the future."""
        return True
    def build(self, labels):
        """Returns an array of 16bit words representing the assembled instruction."""
        return ()
    def optimize(self, labels):
        """Optimizes the current instruction, returns True only if a change occured."""
        return False
    def clone(self):
        """Creates a proper copy of the instruction.
        
        Result should be identical to creating an instruction via `__init__()`, so the use of `badRelocate()` will be safe immediately after.
        """
        result = instruction(("DAT", "\"\""), self.preceding, self.lineNum)
        return result
    def badRelocate(self, preceding):
        """Relocates the current instruction by recreating self.address, see below for warning.
        
        Any instructions or addresses based off this instruction can no longer be guaranteed to be correct.
        This includes any instructions added after this one.
        
        *********** Use only after instruction creation, and before any instructions are added after. ***********
        """
        self.preceding = preceding
        if preceding != None:
            self.address = preceding.getAddress().clone()
            self.address.add(preceding)
        else:
            self.address = address(0)

class preprocessor_directive(instruction):
    """Interface used by the assembler to work with preprocessor directives.
    
    Notes:
        * This class inherits all members of the `instruction` class, the assembler will also expect that those functions are implemented even if they are overriden by `preprocessor_directive`.
        * The assembler also expects implementations of the needsCodeblock(), setCodeblock(), and endCodeblock() functions, but only if codeblocks are used.
        * The provided `__init__()` is how the assembler will create any preprocesser-based class. Children should provide an `__init__()` with similar args, and call this class's `__init__()`
    """
    def __init__(self, parts, preceding, lineNum, labels={}):
        """Initalizes data common to most if not all preprocessor directives.
        
        Nothing special is done for preprocessor_directives, but `super().__init__()` is called, so check the docs for `instruction.__init__()`.
        
        @param parts An array of strings, representing an preprocessor directive, following by it's arguments in order. Not used by interface's `__init__()`.
        @param preceding The previous instruction in the program. Used to determine `self.address`.
        @param lineNum Line number the instruction originated from. Stored in `self.lineNum`.
        @param labels Unknown
        @todo Describe the labels param
        """
        super().__init__(parts, preceding, lineNum);
    def needsCodeblock(self):
        """Returns true if the preprocessor directive is also the start of a codeblock.
        
        Usually called after `__init__()` is called, or after `endCodeblock()` is called, to check for multiple codeblocks.
        
        A default implemention is provided which will always return False.
        """
        return False
    def setCodeblock(self, codeblock):
        """Provides the directive with the next codeblock to manage.
        
        Only called if `needsCodeblock()` returns True.
        
        The default implemention will print an error to the console, but not throw an exception.
        
        @param codeblock Array of instruction and/or preprocessor_directive objects. First instruction has a undefined preceding instruction set.
        @todo Define exact preceding for first instruction of codeblock
        """
        printError("Directive doesn't accept codeblocks", lineNum=("~"+self.lineNum))
    def endCodeblock(self, parts, lineNum):
        """Provides the directive used to end the codeblock to the starting directive.
        
        Only called if `needsCodeblock()` returns True.
        
        The default implemention will do nothing.
        
        @param parts An array of strings, representing an preprocessor directive, following by it's arguments in order.
        @param lineNum Line number of ending directive
        """
        return
    def clone(self):
        """Creates a proper copy of the preprocesser directive.
        
        Result should be identical to creating a preprocesser directive via `__init__()`, so the use of `badRelocate()` will be safe immediately after.
        """
        return preprocessor_directive(None, self.preceding, self.lineNum)

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
        self.curToken = None
        self.curPart = ""
        self.mode = "none"
        self.lineNum = 1
        self.charNum = 0
        self.inStr = None
        self.grouping_level = 0
        self.nextCharEscaped = False
        
        self.tokenQueue = []
        self.inCodeblock = False
        self.instructions = []
        self.labels = {"$$globalLabel":""}
        self.origin = origin(0)
    def printError(self, error, lineNum=None):
        if lineNum == None:
            lineNum = self.lineNum
        printError(error, lineNum)
    def parseTokens(self):
        while len(self.tokenQueue) > 0:
            token = self.tokenQueue.pop(0)
            lineNum = token[0]
            cmd = token[1]
            args = list(token[2])
            args.insert(0, cmd)
            args = tuple(args)
            
            preceding = self.origin
            if len(self.instructions) > 0:
                preceding = self.instructions[-1]

            if self.inCodeblock:
                if cmd.startswith(".") and cmd[1:].lower() in ("end", "elseif", "elif", "else") and (not self.codeblock.inCodeblock):
                    self.inCodeblock = False
                    preceding.setCodeblock( self.codeblock.instructions[1:] )
                    preceding.endCodeblock(args, lineNum)
                    if preceding.needsCodeblock():
                        self.inCodeblock = True
                        self.codeblock = reader()
                        self.codeblock.instructions.append(self.instructions[-2])   #Only an elseif or a else will continue, so this is the correct preceding instruction
                        self.codeblock.labels = self.labels
                else:
                    self.codeblock.tokenQueue.append(token)
                    self.codeblock.parseTokens()
                continue
            if cmd.startswith("."):
                if cmd == ".insert_macro":
                    args = (args[0], args[1]+"("+",".join(args[2:])+")")
                tmp = preprocessor_directive(args, preceding, lineNum, self.labels)
                self.instructions.append(tmp)
                if tmp.needsCodeblock():
                    self.inCodeblock = True
                    self.codeblock = reader()
                    self.codeblock.instructions.append(preceding)
                    self.codeblock.labels = self.labels
            else:
                tmp = instruction(args, preceding, lineNum)
                self.instructions.append(tmp)
    def readChar(self, char):
        printError = self.printError
        self.charNum += 1
        if char in ("\n",):
            if self.mode in ("label", "plabel"):
                self.readChar(" ")
                self.charNum -= 1
            if self.mode in ("op", "macroWillError", "macroError"):
                if self.curPart != "":
                    self.readChar(",")
                    self.charNum -= 1
                self.curToken = (self.curToken[0], self.curToken[1], tuple(self.curToken[2]))
                self.tokenQueue.append(self.curToken)
                self.curToken = None
            if self.mode in ("preprocessor",):
                if self.curPart != "":
                    self.readChar(" ")
                    self.charNum -= 1
                self.curToken = (self.curToken[0], self.curToken[1], tuple(self.curToken[2]))
                self.tokenQueue.append(self.curToken)
                self.curToken = None
            self.parseTokens()
            self.mode = "none"
            self.curPart = ""
            self.lineNum += 1
            self.charNum = 0
            self.inStr = None
            return
        if self.mode in ("error", "macroError", "comment"):
            return  #Ignore the rest of the line
        if char in ('"', "'") and not self.nextCharEscaped:
            if self.inStr == None:
                self.inStr = char
            elif self.inStr == char:
                self.inStr = None
        self.nextCharEscaped = (self.inStr != None and char == "\\")
        if char in ("(",) and self.inStr == None:
            self.grouping_level += 1
        elif char in (")",) and self.inStr == None:
            self.grouping_level -= 1
        if self.mode == "macroWillError":
            if not char.isspace():
                if char == ";" and self.inStr == None:
                    self.mode = "macroError"
                    return
                printError("Char "+str(self.charNum)+": Unexpected character "+repr(char))
                self.mode = "macroError"
                return
            return
        if char == ";" and self.inStr == None:
            self.readChar("\n")
            self.lineNum -= 1
            self.mode = "comment"
            return
        if self.mode == "none":
            if char.isalpha() or char in ("_",):
                self.mode = "label"
            elif char in (":",):
                self.mode = "plabel"
                return
            elif char in (".", "#"):
                self.mode = "preprocessor"
                return
            elif char.isspace():
                return
            else:
                printError("GAH")
                return
        if self.mode in ("label", "plabel"):
            if char == ":" or (char.isspace() and self.mode == "plabel"):
                self.tokenQueue.append((self.lineNum, ".label", (self.curPart,)))
                self.curPart = ""
                self.mode = "none"
                return
            if char.isspace():
                if self.curPart.upper() in instruction.ops:
                    self.mode = "op"
                    self.curToken = (self.lineNum, self.curPart.upper(), [])
                    self.curPart = ""
                    return
                printError("Invalid opcode \""+self.curPart+"\"")
                self.mode = "error"
                return
            if char == "(" and self.mode == "label":
                self.mode = "macro"
                self.curToken = (self.lineNum, ".insert_macro", [self.curPart])
                self.curPart = ""
                return
            if not (char.isalnum() or char in ("_",)):
                printError("Char "+str(self.charNum)+": Unexpected character "+repr(char))
                self.mode = "error"
                return
            self.curPart += char
        if self.mode in ("op", "macro"):
            if self.mode == "macro" and char == ")" and self.inStr == None and self.grouping_level == 0:
                self.grouping_level += 1; self.readChar(","); self.grouping_level -= 1
                self.charNum -= 1
                self.mode = "macroWillError"
                return
            if char == "," and self.inStr == None and ((self.mode == "macro" and self.grouping_level == 1) or (self.mode == "op" and self.grouping_level == 0)):
                self.curToken[2].append(self.curPart.strip())
                self.curPart = ""
                return
            self.curPart += char
            return
        if self.mode in ("preprocessor",):
            if char.isspace() and self.inStr == None and self.grouping_level == 0:
                if self.curPart != "":
                    if self.curToken == None:
                        if self.curPart == "dat":
                            self.mode = "label"
                            self.readChar(char)
                            return
                        self.curToken = (self.lineNum, "."+self.curPart, [])
                    else:
                        self.curToken[2].append(self.curPart.strip())
                self.curPart = ""
                return
            elif char in (":",):
                self.mode = "label"
                self.curPart = "_" + self.curPart
                self.readChar(char)
                return
            self.curPart += char
            return
            
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
            labels["$$globalLabel"] = ""
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
        labels["$$globalLabel"] = ""
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
    read.read(asm)  #TODO: support @, cmd arguments
    asm.close()
    obj = open(argv[1], 'wb')
    write = writer(read.instructions, read.labels, read.origin)
    write.writeFile(obj)
    obj.close()

if __name__ == "__main__":
    main(sys.argv[1:])
#main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])

