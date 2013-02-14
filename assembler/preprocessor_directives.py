from assembler import preprocessor_directive, address

class default_preprocessor_directive(preprocessor_directive):
    directives = ("align", "echo", "error", "rep", "if", "define", "undefine", "origin", "incbin", "include", "macro", "insert_macro", "label", "equ")

    def printError(self, error, file=sys.stderr, lineNum=None):
        global printError
        if lineNum == None:
            lineNum = self.lineNum
        printError(error, lineNum, file=file)
    def __init__(self, parts, preceding, lineNum, labels={}):
        parts = list(parts)
        
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
        if parts[0][1:].lower() == "reserve":
            parts[0] = ".rep"
            self.__init__(parts, preceding, lineNum, labels)
            self.setCodeblock([instruction(("DAT","0"), preceding, lineNum)])
            self.endCodeblock((".end",), lineNum)
            return
        if parts[0][1:].lower() == "incpack":
            parts[0] = ".incbin"
        if parts[0][1:].lower() not in self.directives:
            self.printError("Preproccessor directive \"" + parts[0] + "\" does not exist")
            return
        self.directive = parts[0][1:].lower()
        extra = ""
        if self.directive in ("define", "undefine", "equ"):
            nParts = []
            for x in parts[1:]:
                if type(x) != type(""):
                    nParts.append(x)
                else:
                    nParts.extend(x.split(" "))
            self.varible = nParts[0]
            if self.directive in ("define", "equ"):
                if len(nParts) > 1:
                    if type(nParts[1]) == type(""):
                        extra = " ".join(nParts[1:])
                    else:
                        extra = nParts[1]
                else:
                    extra = "1"
            else:
                extra = "0"
            if self.directive == "equ":
                labels[self.varible] = address(address(0))
        elif len(parts) >= 2:
            extra = parts[1]
            for i in range(2, len(parts)):
                extra += " " + parts[i]
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
                _size += 1
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
                macroArgs.append(value_preprocesser(arg, lineNum))
            self.macroArgs = tuple(macroArgs)
            return
        if self.directive == "label":
            self.extra = extra
            if extra.startswith("_"):
                extra = labels["$$globalLabel"] + "$" + extra[1:]
            else:
                labels["$$globalLabel"] = extra
            labels[extra] = self
            return
        self.a = value_preprocesser(extra, lineNum)
    
    #def cycles(self):
    #    return self.size()
    def size(self):
        if self.directive in ("echo", "error", "macro", "label"):
            return 0
        if self.directive in ("align", "origin", "incbin"):
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
        if self.directive == "origin":
            return ()
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
            label = self.extra
            if label.startswith("_"):
                label = labels["$$globalLabel"] + "$" + label[1:]
            else:
                labels["$$globalLabel"] = label
            labels["$$labels"][label] = self
            return ()
        labels["$$curAddress"] = self
        value = self.a.build(labels)
        labels.pop("$$curAddress")
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
        if self.directive == "equ":
            labels["$$labels"][self.varible] = address(address(value))
        return ()
    def isConstSize(self):
        if self.directive == "include":
            if self.codeblock == None: return True
            for i in self.codeblock:
                if i.isConstSize(): return True

            return False
        return self.directive in ("echo", "error", "define", "undefine", "equ", "incbin", "macro", "label")
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
            label = self.extra
            if label.startswith("_"):
                label = labels["$$globalLabel"] + "$" + label[1:]
            else:
                labels["$$globalLabel"] = label
            labels["$$labels"][label] = self
            return False
        labels["$$curAddress"] = self
        result = self.a.optimize(labels)
        labels.pop("$$curAddress")
        if self.directive == "align":
            tmp = self.a.build(labels)
            if type(tmp) != type(42):
                self.printError("Can't align to an non-int boundary")
                return False
            lastSize = self._size
            self._size = tmp - (self.getAddress().getAddress() % tmp)
            return (lastSize != self._size) or result
        if self.directive == "origin":
            tmp = self.a.build(labels)
            if type(tmp) != type(42):
                self.printError("Can't set origin to an non-int boundary")
                return False
            lastSize = self._size
            self._size = tmp - self.getAddress().getAddress()
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
            if not self.passIf:
                result = self.optCodeblock(labels)
            if self.nextIf != None:
                if self.passIf:
                    result = self.nextIf.optimize(labels) or result
            return (lastPass != self.passIf) or result
        if self.directive == "define":
            lastValue = self.value
            labels["$$curAddress"] = self
            self.value = self.a.build(labels)
            labels.pop("$$curAddress")
            labels[self.varible] = address(address(self.value))
            return (lastValue != self.value)
        if self.directive == "undefine":
            try:
                labels.pop(self.varible)
            except KeyError:
                pass
            return False
        if self.directive == "equ":
            lastValue = self.value
            labels["$$curAddress"] = self
            self.value = self.a.build(labels)
            labels.pop("$$curAddress")
            labels["$$labels"][self.varible] = address(address(self.value))
            return (lastValue != self.value)
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
        parts = list(parts)
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
                self.nextIf = preprocessor_directive(parts, self.preceding, lineNum)
            elif parts[0][1:].lower() == "else":
                parts[0] = ".rep"
                parts.insert(1, "1")
                self.nextIf = preprocessor_directive(parts, self.preceding, lineNum)
            elif parts[0][1:].lower() == "end":
                self.nextIf = None
            else:
                printError("Can't use \"" + parts[0] + "\" to end .if directive", lineNum=lineNum)
        elif self.directive == "macro":
            if parts[0][1:].lower() != "end":
                printError("Can't end .macro codeblock with a directive besides .end", lineNum=lineNum)
                return
    def clone(self):
        result = None
        if self.directive in ("define", "undefine", "equ"):
            result = preprocessor_directive(("." + self.directive, self.varible, self.a.extra), self.preceding, self.lineNum)
        elif self.directive in ("incbin"):
            result = preprocessor_directive(("." + self.directive, "\""+self.filepath+"\""), self.preceding, self.lineNum)
        elif self.directive in ("include"):
            result = preprocessor_directive(("." + self.directive, self.filepath), self.preceding, self.lineNum)
        elif self.directive in ("macro", "insert_macro", "label"):
            result = preprocessor_directive(("." + self.directive, self.extra), self.preceding, self.lineNum)
        else:
            result = preprocessor_directive(("." + self.directive, self.a.extra), self.preceding, self.lineNum)
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
