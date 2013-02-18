import sys
import os.path
import copy

from assembler import preprocessor_directive, address, stdwrap, printError, reader
import assembler
from mathEval import eval_0xSCAmodified, wordString, extractVaribles, tokenize, validate
from dcpu16 import dcpu16_instruction

class value_preprocesser:
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
            if self.parent != None:
                eval_locals["$$curAddress"] = self.parent.getAddress().getAddress()
            return eval_0xSCAmodified(self.extra, eval_locals, globalLabel)#eval(self.extra,{"__builtins__":None},eval_locals)
        return ()

class echo_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        self.a = value_preprocesser(self.extra, lineNum, self)
    
    def build(self, labels):
        value = self.a.build(labels)
        self.printError(value, file=sys.stdout)
        return ()
    
class error_directive(echo_directive):
    def build(self, labels):
        value = self.a.build(labels)
        self.printError(value)
        raise Exception("User-generated error")

## @todo Fix .equ directive
# @todo Fix .define behavior
class define_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        
        if self.directive == "def": self.directive = ".define"
        if self.directive == "undef": self.directive = ".undefine"
        
        extra = ""
        
        nParts = []
        for x in parts[1:]:
            nParts.extend(x.split(" "))
        self.varible = nParts[0]
        if self.directive in ("define", "equ"):
            if len(nParts) > 1:
                extra = " ".join(nParts[1:])
            else:
                extra = "1"
        else:
            extra = "0"
        
        self.extra = extra
        self.value = None
        self.a = value_preprocesser(extra, lineNum, self)
    
    def build(self, labels):
        value = self.a.build(labels)
        
        if self.directive == "define":
            labels[self.varible] = address(address(value))
        elif self.directive == "equ":
            labels["$$labels"][self.varible] = address(address(value))
        elif self.directive == "undefine":
            try:
                labels.pop(self.varible)
            except KeyError:
                pass
        return ()
    def optimize(self, labels):
        result = self.a.optimize(labels)
        
        if self.directive in ("define", "equ"):
            lastValue = self.value
            self.value = self.a.build(labels)
            if self.directive == "equ":
                labels["$$labels"][self.varible] = address(address(self.value))
            else:
                labels[self.varible] = address(address(self.value))
            return (lastValue != self.value)
        elif self.directive == "undefine":
            try:
                labels.pop(self.varible)
            except KeyError:
                pass
        return False
    def clone(self):
        return define_directive(("." + self.directive, self.varible, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
        
class label_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        
        tmp = {"$$labels": labels, "$$globalLabel": labels["$$globalLabel"]}
        self.updateLabel(tmp)
        labels["$$globalLabel"] = tmp["$$globalLabel"]
        
    def updateLabel(self, labels):
        label = self.extra
        if label.startswith("_"):
            label = labels["$$globalLabel"] + "$" + label[1:]
        else:
            labels["$$globalLabel"] = label
        labels["$$labels"][label] = self
        
    def build(self, labels):
        self.updateLabel(labels)
        return ()
    def optimize(self, labels):
        self.updateLabel(labels)
        return False    #We can return False, because we only change in value when another instruction changes in size, which would have already returned True
        
class align_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        
        self.error = None
        self._size = 0
        self.a = value_preprocesser(self.extra, lineNum, self)
    
    def size(self):
        return self._size
    def isConstSize(self):
        return self.a.isConstSize() and self.getAddress().isConst()
    def build(self, labels):
        if self.error != None:
            self.printError(self.error)
        return (0,)*self._size
    def optimize(self, labels):
        result = self.a.optimize(labels)
        
        lastSize = self._size
        tmp = self.a.build(labels)
        if type(tmp) != type(42):
            self.error = "Can't align to an non-int boundary"
            self._size = 0
        else:
            self.error = None
            self._size = tmp - (self.getAddress().getAddress() % tmp)
        return (lastSize != self._size) or result

class origin_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
       
        self._size = 0
        self.error = None
        self.a = value_preprocesser(self.extra, lineNum, self)
    
    def size(self):
        return self._size
    def isConstSize(self):
        return self.a.isConstSize() and self.getAddress().isConst()
    def build(self, labels):
        if self.error != None:
            self.printError(self.error)
        return ()
    def optimize(self, labels):
        result = self.a.optimize(labels)
        
        lastSize = self._size
        tmp = self.a.build(labels)
        if type(tmp) != type(42):
            self.error = "Can't set origin to an non-int boundary"
            self._size = 0
        else:
            self.error = None
            self._size = tmp - self.getAddress().getAddress()
        return (lastSize != self._size) or result
        
class rep_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        
        self.error = None
        self._size = 0
        self.codeblock = None
        self.codeblockInstance = None
        self.a = value_preprocesser(self.extra, lineNum, self)
    
    def size(self):
        size = 0
        if self.codeblockInstance == None: return 0
        for i in self.codeblockInstance:
            size += i.size()
        return size
        
    def isConstSize(self):
        return False
        
    def build(self, labels):
        if self.error != None:
            self.printError(self.error)
        code = []
        if self.codeblockInstance == None: return ()
        for i in self.codeblockInstance:
            code.extend(i.build(labels))
        return tuple(code)
        
    def optCodeblockInstance(self, labels):
        if self.codeblockInstance == None: return False
        result = False
        for i in self.codeblockInstance:
            result = i.optimize(labels) or result
        return result
    def optimize(self, labels):
        result = self.a.optimize(labels)
        
        lastSize = self._size
        tmp = self.a.build(labels)
        if type(tmp) != type(42):
            self.error = "Can't repeat an non-int amount of times"
            self._size = 0
            return (lastSize != self._size) or result
        self.error = None
        
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
        
    def setCodeblock(self, codeblock):
        self.codeblock = codeblock
    def needsCodeblock(self):
        return (self.codeblock == None)
    
    def endCodeblock(self, parts, lineNum):
        if not (parts[0].startswith(".") or parts[0].startswith("#")):
            printError("Directives must start with a . or #")
            return
        parts = list(parts)
        if parts[0][1:].lower() != "end":
            printError("Can't end ."+self.directive+" codeblock with a directive besides .end", lineNum=lineNum)
            return
    def clone(self):
        result = None
        result = rep_directive(("."+self.directive, self.a.extra), self.preceding, self.lineNum, self.fileName, self.reader)
        if self.codeblock != None and result.needsCodeblock():
            codeblock = []
            for code in self.codeblock:
                codeblock.append(code.clone())
            result.codeblock = tuple(codeblock)
        return result

class reserve_directive(rep_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        self.setCodeblock([dcpu16_instruction(("DAT","0"), preceding, lineNum)])
        self.endCodeblock((".end",), lineNum)
    
class if_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        
        self.error = None
        self.codeblock = None
        self.nextIf = None
        self.passIf = False
        self.a = value_preprocesser(self.extra, lineNum, self)
    
    def size(self):
        if self.passIf:
            if self.nextIf != None:
                return self.nextIf.size()
            return 0
        if self.codeblock == None: return 0
        size = 0
        for i in self.codeblock:
            size += i.size()
        return size
    def isConstSize(self):
        return False
    def build(self, labels):
        if self.error != None:
            self.printError(self.error)
        
        if self.passIf:
            if self.nextIf != None:
                return self.nextIf.build(labels)
            return ()
        if self.codeblock == None: return ()
        code = []
        for i in self.codeblock:
            code.extend(i.build(labels))
        return tuple(code)
    def isConstSize(self):
        return False
    def optCodeblock(self, labels):
        if self.codeblock == None: return False
        result = False
        for i in self.codeblock:
            result = i.optimize(labels) or result
        return result
    def optimize(self, labels):
        result = self.a.optimize(labels)
        
        tmp = self.a.build(labels)
        if not (type(tmp) == type(42) or type(tmp) == type(False)):
            self.error = "Can't compare an non-int amount with 0"
            return result
        lastPass = self.passIf
        self.passIf = (tmp == 0)
        if not self.passIf:
            result = self.optCodeblock(labels) or result
        if self.nextIf != None:
            if self.passIf:
                result = self.nextIf.optimize(labels) or result
        return (lastPass != self.passIf) or result
        
    def needsCodeblock(self):
        if self.codeblock == None: return True
        if self.nextIf == None: return False
        return self.nextIf.needsCodeblock()
    def setCodeblock(self, codeblock):
        if self.codeblock != None:
            if self.nextIf == None:
                printError("Unneeded codeblock given to .if statement", lineNum=("~"+self.lineNum))
                return
            return self.nextIf.setCodeblock(codeblock)
        self.codeblock = codeblock
    def endCodeblock(self, parts, lineNum):
        if not (parts[0].startswith(".") or parts[0].startswith("#")):
            printError("Directives must start with a . or #")
            return
        parts = list(parts)
        if self.nextIf != None:
            return self.nextIf.endCodeblock(parts, lineNum)
        if parts[0][1:].lower() == "elif":
            parts[0] = ".elseif"
        if parts[0][1:].lower() == "elseif":
            parts[0] = ".if"

            self.nextIf = if_directive(parts, self.preceding, lineNum, self.fileName, self.reader)
        elif parts[0][1:].lower() == "else":
            parts[0] = ".rep"
            parts.insert(1, "1")
            self.nextIf = if_directive(parts, self.preceding, lineNum, self.fileName, self.reader)
        elif parts[0][1:].lower() == "end":
            self.nextIf = None
        else:
            printError("Can't use \"" + parts[0] + "\" to end .if directive", lineNum=lineNum)
            self.nextIf = None
    def clone(self):
        result = if_directive(("." + self.directive, self.a.extra), self.preceding, self.lineNum, self.fileName, self.reader)
        if self.codeblock != None and result.needsCodeblock():
            codeblock = []
            for code in self.codeblock:
                codeblock.append(code.clone())
            result.codeblock = tuple(codeblock)
        if self.nextIf != None:
            result.nextIf = self.nextIf.clone()
        result.passIf = self.passIf
        return result

class ifdef_directive(if_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        parts = list(parts)
        parts[1] = "isdef( " + parts[1].strip() + " )"
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
class ifndef_directive(if_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        parts = list(parts)
        parts[1] = "!isdef( " + parts[1].strip() + " )"
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)

class include_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)

        self.codeblock = None
        if self.extra.startswith("\"") and self.extra.endswith("\""):
            self.filepath = self.extra[1:-1]
            if not os.path.isfile(self.filepath):
                self.printError("Could not include \""+self.extra[1:-1]+"\" as the file does not exist")
                return
        elif self.extra.startswith("<") and self.extra.endswith(">"):
            self.filepath = ""
            for x in INCLUDEPATH:
                filepath = os.path.join(x, self.extra[1:-1])
                if os.path.isfile(filepath):
                    self.filepath = filepath
                    break
            self.printError("Could not find \""+self.extra[1:-1]+"\" in the assembler include directories")
            return
        else:
            self.printError("Could not understand the arguments "+repr(extra)+" for include")
            
        codeblock = assembler.reader(fileName=self.extra)
        reader.copyRegistration(codeblock)
        
        codeblock.instructions.append(preceding)
        asm = open(self.filepath, 'r')
        codeblock.read(asm)
        asm.close()
        for l in codeblock.labels:
            labels[l] = codeblock.labels[l]
        self.codeblock = codeblock.instructions[1:]
    
    def size(self):
        if self.codeblock == None: return 0
        
        size = 0
        for i in self.codeblock:
            size += i.size()
        
        return size
        
    def isConstSize(self):
        if self.codeblock == None: return True
        for i in self.codeblock:
            if not i.isConstSize(): return False
        return True
        
    def build(self, labels):
        if self.codeblock == None: return ()
        
        code = []
        for i in self.codeblock:
            code.extend(i.build(labels))
        
        return tuple(code)
    
    def optCodeblock(self, labels):
        if self.codeblock == None: return False
        result = False
        for i in self.codeblock:
            result = i.optimize(labels) or result
        return result
    def optimize(self, labels):
        result = self.optCodeblock(labels)
        return result
    
    def clone(self):
        if self.codeblock == None:
            return copy.deepcopy(self)
        return super().clone()

class incbin_directive(preprocessor_directive):
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        if self.directive == "incpack":
            self.directive = ".incbin"
        
        self._size = 0
        if self.extra.startswith("\"") and self.extra.endswith("\""):
            self.filepath = self.extra[1:-1]
            if not os.path.isfile(self.filepath):
                self.printError("Could not include \""+repr(self.extra[1:-1])+"\" as the file does not exist")
                return
        elif self.extra.startswith("<") and self.extra.endswith(">"):
            self.filepath = ""
            for x in INCLUDEPATH:
                filepath = os.path.join(x, self.extra[1:-1])
                if os.path.isfile(filepath):
                    self.filepath = filepath
                    break
            self.printError("Could not find \""+self.extra[1:-1]+"\" in the assembler include directories")
            return
        else:
            self.printError("Could not understand the arguments "+repr(self.extra)+" for incbin")
        _size = os.path.getsize(self.filepath)
        if _size % 2 != 0:
            _size += 1
        self._size = int(_size)
    
    def size(self):
        return self._size
    def isConstSize(self):
        return True
    def build(self, labels):
        if self._size == 0:
            return ()
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
    def clone(self):
        if self._size == 0:
            return copy.deepcopy(self)
        return super().clone()

class macro_directive(preprocessor_directive):
    def __initAlways__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        self.codeblock = None
        self.macroName = None
        self.macroArgs = None
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        self.__initAlways__(parts, preceding, lineNum, fileName, reader, labels)
        
        if len(parts) <= 1:
            self.printError("No definition supplied for macro.")
            return
        
        operation = None
        try:
            operation = validate( tokenize( self.extra, ("func",) ) )
        except Exception as e:
            self.printError("Failed to understand macro definiton: " + str(e.args[0]))
            return
        if type(operation) != type(()) or operation[0] != "func":
            self.printError("Expected macro definition (Ex: \"aMacro()\", \"aMacro(param1, param2)\"")
            return
        self.macroName = operation[1]
        for tokens in operation[2]:
            if type(tokens) == type(()):
                self.printError("Unexpected function call or operation in macro definition")
                return False
            elif type(tokens) == type(""):
                if tokens[0].isdecimal() or (tokens[0] in ("\"", "'") and tokens[-1] in ("\"", "'")):
                    self.printError("Unexpected literal in macro definition")
                    return False
        self.macroArgs = operation[2]
    
    def build(self, labels):
        if self.macroArgs != None:
            labels["$$macro$"+self.macroName] = (self.macroArgs, self.codeblock)
        return ()
    def optimize(self, labels):
        if self.macroArgs != None:
            labels["$$macro$"+self.macroName] = (self.macroArgs, self.codeblock)
        return False
    def needsCodeblock(self):
        return (self.codeblock == None)
    def setCodeblock(self, codeblock):
        self.codeblock = codeblock
    def endCodeblock(self, parts, lineNum):
        if not (parts[0].startswith(".") or parts[0].startswith("#")):
            printError("Directives must start with a . or #")
            self.macroArgs = None
            return
        if parts[0][1:].lower() != "end":
            printError("Can't end .macro codeblock with a directive besides .end", lineNum=lineNum)
            self.macroArgs = None
            return
    def clone(self):
        if self.macroArgs == None:
            result = macro_directive.__new__(macro_directive, ("." + self.directive, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
            result.__initAlways__(("." + self.directive, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
        else:
            result = macro_directive(("." + self.directive, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
        if self.codeblock != None and result.needsCodeblock():
            codeblock = []
            for code in self.codeblock:
                codeblock.append(code.clone())
            result.codeblock = tuple(codeblock)
        return result

class insert_macro_directive(preprocessor_directive):
    def __initAlways__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        super().__init__(parts, preceding, lineNum, fileName, reader, labels)
        self.codeblock = None
        self.macroName = None
        self.macroArgs = None
        self.error = None
    def __init__(self, parts, preceding, lineNum, fileName, reader, labels={}):
        self.__initAlways__(parts, preceding, lineNum, fileName, reader, labels)
        
        operation = None
        try:
            operation = eval_0xSCAmodified(self.extra, {}, preEval=True)
        except Exception as e:
            self.printError("Failed to understand expression to insert macro: " + str(e.args[0]))
            return

        if type(operation) != type(()) or operation[0] != "func":
            self.printError("Expected macro call (Ex: \"aMacro()\", \"aMacro(param1, param2)\"")
            return
        self.macroName = operation[1]
        macroArgs = []
        for arg in operation[2]:
            macroArgs.append(value_preprocesser(arg, lineNum, None))
        self.macroArgs = tuple(macroArgs)
    
    def size(self):
        if self.codeblock == None or self.macroArgs == None: return 0
        size = 0
        for i in self.codeblock:
            size += i.size()
        return size
    def isConstSize(self):
        if self.macroArgs == None: return True
        if self.codeblock == None: return False
        for i in self.codeblock:
            if not i.isConstSize(): return False
        return True
    def build(self, labels):
        if self.error != None:
            self.printError(self.error)
        if self.codeblock == None or self.macroArgs == None: return ()
        if ("$$macro$"+self.macroName) not in labels:
            self.printError("Macro "+repr(self.macroName)+" was not defined")
            return
        macro = labels["$$macro$"+self.macroName]
        if len(self.macroArgs) != len(macro[0]):
            self.printError("Macro "+repr(self.macroName)+" is defined for "+str(len(macro[0]))+" args, not "+str(len(self.macroArgs))+" args.")
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
    def optCodeblock(self, labels):
        if self.codeblock == None: return False
        result = False
        for i in self.codeblock:
            result = i.optimize(labels) or result
        return result
    def optimize(self, labels):
        if self.macroArgs == None: return False
        if "$$macro$"+self.macroName not in labels:
            self.error = "Macro "+repr(self.macroName)+" was not defined"
            self.codeblock = None
            return
        macro = labels["$$macro$"+self.macroName]
        if len(self.macroArgs) != len(macro[0]):
            self.error = "Macro "+repr(self.macroName)+" is defined for "+str(len(macro[0]))+" args, not "+str(len(self.macroArgs))+" args."
            self.codeblock = None
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
        while self.optCodeblock(labels_): pass
        for label in labels_.keys():
            if label not in labels:
                if label not in macro[0] or labels_[label] != args[label]:
                    labels[label] = labels_[label]
        return False
    def clone(self):
        if self.macroArgs == None:
            result = macro_directive.__new__(macro_directive, ("." + self.directive, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
            result.__initAlways__(("." + self.directive, self.extra), self.preceding, self.lineNum, self.fileName, self.reader)
            return result
        return super().clone()
            

def registerDirectives(reader):
    reader.registerDirective("echo", echo_directive)
    reader.registerDirective("error", error_directive)
    reader.registerDirective("define", define_directive)
    reader.registerDirective("def", define_directive)
    reader.registerDirective("undefine", define_directive)
    reader.registerDirective("undef", define_directive)
    reader.registerDirective("equ", define_directive)
    reader.registerDirective("label", label_directive)
    reader.registerDirective("align", align_directive)
    reader.registerDirective("rep", rep_directive)
    reader.registerDirective("reserve", reserve_directive)
    reader.registerDirective("org", origin_directive)
    reader.registerDirective("origin", origin_directive)
    reader.registerDirective("if", if_directive)
    reader.registerDirective("ifdef", ifdef_directive)
    reader.registerDirective("ifndef", ifndef_directive)
    reader.registerDirective("include", include_directive)
    reader.registerDirective("incbin", incbin_directive)
    reader.registerDirective("incpack", incbin_directive)
    reader.registerDirective("macro", macro_directive)
    reader.registerDirective("insert_macro", insert_macro_directive)
