import sys

def printError(error, lineNum, file=sys.stderr):
    print("Line " + str(lineNum) + ": " + str(error), file=file)

class token:
    def __init__(self, string, lineNum, charNum):
        self.string = string
        self.lineNum = lineNum
        self.charNum = charNum
    def isOp(self):
        if self.string.isalnum():
            return False
        if self.string[0] == '"' and self.string[-1] == '"':
            return False
        return True
    
    def __str__(self):
        return str(self.string)
    def __repr__(self):
        return "(Line "+str(self.lineNum)+", Char "+str(self.charNum)+", "+repr(self.string)+")"

class valType:
    def __init__(self, tokens):
        self._tokensUsed = 0
        tokensUsed = 0
        
        self.signed = True
        if tokens[0].string in ("signed", "unsigned"):
            if tokens[0].string == "unsigned":
                self.signed = False
            tokensUsed += 1
        
        if len(tokens) < tokensUsed+1:
            return
        
        if tokens[tokensUsed].string in ("int", "char"):
            self.type = "int"
            tokensUsed += 1
        elif tokens[tokensUsed].string in ("void"):
            self.type = "void"
            tokensUsed += 1
        else:
            return
        
        if len(tokens) < tokensUsed+1:
            return
        
        self.pointerLevel = 0
        while (len(tokens) >= tokensUsed+1) and tokens[tokensUsed].string == "*":
            self.pointerLevel += 1
            tokensUsed += 1
        
        self._tokensUsed = tokensUsed
            
    def isValid(self):
        return (self._tokensUsed != 0)
    def tokensUsed(self):
        return self._tokensUsed
    def __repr__(self):
        result = ""
        if not self.signed:
            result = "unsigned"
        return result + self.type + "*"*self.pointerLevel

class operation:
    def __init__(self, op, a, b): # a <op> b
        self.op = op
        self.a = a
        self.b = b
    def __repr__(self):
        result = self.op.string
        if self.a != None:
            if isinstance(self.a, operation):
                result = "(" + str(self.a) + ")" + result
            else:
                result = str(self.a) + result
        if self.b != None:
            if isinstance(self.b, operation):
                result += "(" + str(self.b) + ")"
            else:
                result += str(self.b)
        return result

class functionCall:
    def __init__(self, tokens):
        self.name = tokens[0].string
        self.args = []
        
        #TODO: input validation
        i = 2
        while tokens[i].string != ")":
            if i == 2:
                self.args.append(tokens[i].string)
                i += 1
            else:
                self.args.append(tokens[i+1].string)
                i += 2
    
    def __repr__(self):
        result = self.name + "("
        for x in self.args:
            result += x + ", "
        if len(self.args) > 0:
            result = result[:-2]
        result += ")"
        return result

class expression:
    def processTokens(self, tokens):
        result = []
        
        group = []
        group_level = 0
        for x in tokens:
            if x.string == "(":
                group_level += 1
                if group_level == 1 and result[-1].string.isalnum():
                    group = [result[-1]]
                    result = result[:-1]
                else:
                    group = []
            if group_level > 0:
                group.append(x)
            else:
                result.append(x)
            if x.string == ")":
                group_level -= 1
                if group_level == 0:
                    if group[0].string.isalnum():
                        result.append(functionCall(group))
                    else:
                        result.append(self.processTokens(group[1:-1]))
                        if result[-1] == None:
                            return None
        
        #TODO: array indexing
        #TODO: type casting
        table = [ ("ltr", {"++":False, "--":False, ".":True, "->":True}),
                  ("rtl", {"++":False, "--":False, "+":False, "-":False, "!":False, "~":False, "*":False, "&":False}),
                  ("ltr", {"*":True, "/":True, "%":True}),
                  ("ltr", {"+":True, "-":True}),
                  ("ltr", {"<<":True, ">>":True}),
                  ("ltr", {"<":True, "<=":True, ">":True, ">=":True}),
                  ("ltr", {"==":True, "!=":True}),
                  ("ltr", {"&":True}),
                  ("ltr", {"^":True}),
                  ("ltr", {"|":True}),
                  ("ltr", {"&&":True}),
                  ("ltr", {"||":True}),
                  #TODO: ternary conditional
                  ("rtl", {"=":True, "+=":True, "-=":True, "*=":True, "/=":True, "%=":True, "<<=":True, ">>=":True, "&=":True, "^=":True, "|=":True}),
                  ("ltr", {",":True}) ]
        for entry in table:
            ops = entry[1]
            tokens = result
            result = []
            if entry[0] == "ltr":
                i = 0
                while i < len(tokens):
                    if isinstance(tokens[i], token) and tokens[i].string in ops:
                        op = tokens[i]
                        b = None
                        if ops[tokens[i].string]:
                            b = tokens[i+1]
                            i += 1
                        if b == None and ((i+1) < len(tokens)) and (isinstance(tokens[i+1], operation) or (not tokens[i+1].isOp())):
                            result.append(op)
                        else:
                            tmp = operation(op, result[-1], b)
                            result = result[:-1]
                            result.append(tmp)
                    else:
                        result.append(tokens[i])
                    i += 1
            else:
                i = len(tokens)-1
                while i >= 0:
                    if isinstance(tokens[i], token) and tokens[i].string in ops:
                        op = tokens[i]
                        a = None
                        if ops[tokens[i].string]:
                            a = tokens[i-1]
                            i -= 1
                        if a == None and (i > 0) and (isinstance(tokens[i-1], operation) or (not tokens[i-1].isOp())):
                            result.insert(0, op)
                        else:
                            tmp = operation(op, a,result[0])
                            result = result[1:]
                            result.insert(0, tmp)
                    else:
                        result.insert(0, tokens[i])
                    i -= 1
        if len(result) == 1:
            result = result[0]
        else:
            result = None
        return result
        
    def __init__(self, tokens):
        self._tokensUsed = 0
        if tokens[-1].string != ";":
            return
        self._tokensUsed = len(tokens)
        self.ops = self.processTokens(tokens[:-1])
        if self.ops == None:
            self._tokens = 0
        
    def isValid(self):
        return (self._tokensUsed != 0)
    def tokensUsed(self):
        return self._tokensUsed
    
    def expectsCode(self):
        return False
    def setCode(self, code):
        return
    
    def __repr__(self):
        return str(self.ops) + ";"

class variableDeclaration:
    def __init__(self, tokens):
        self.type = None
        self.variables = {}

        self.code = None
        self._tokensUsed = 0
        tokensUsed = 0
        
        tmp = valType(tokens[tokensUsed:])
        if not tmp.isValid():
            return
        tokensUsed += tmp.tokensUsed();
        self.type = tmp
        
        if len(tokens) < tokensUsed+1:
            return
        
        tmp = expression(tokens[tokensUsed:])
        if not tmp.isValid():
            return
        tokensUsed += tmp.tokensUsed()
        
        tmp = tmp.ops
        while True:
            if not isinstance(tmp, operation):
                return
            if tmp.op.string == "=":
                if (not isinstance(tmp.a, token)) or (not tmp.a.string.isalnum()):
                    return
                self.variables[tmp.a.string] = tmp.b
                if (not isinstance(tmp.a, token)) or (not tmp.a.string.isalnum()):
                    return
                self.variables[tmp.a.string] = tmp.b
                break
            elif tmp.op.string == ",":
                if (not isinstance(tmp.a, operation)) or (not tmp.a.op == "="):
                    return
                if (not isinstance(tmp.a.a, token)) or (not tmp.a.a.string.isalnum()):
                    return
                self.variables[tmp.a.a.string] = tmp.a.b
            else:
                return
            tmp = tmp.b
        
        self._tokensUsed = tokensUsed
        
    def isValid(self):
        return (self._tokensUsed != 0)
    def tokensUsed(self):
        return self._tokensUsed
        
    def expectsCode(self):
        return False
    def setCode(self, code):
        return
    
    def __repr__(self):
        result = ""
        for x in self.variables:
            result += repr(self.type) + " " + x + " = "
            if isinstance(self.variables[x], token):
                result += str(self.variables[x])
            else:
                result += repr(self.variables[x])
            result += ";\n"
        if len(self.variables) > 0:
            result = result[:-1]
        return result

class function:
    def __init__(self, tokens):
        self.retType = None
        self.code = None
        self._tokensUsed = 0
        
        tokensUsed = 0
        
        self.static = False
        if tokens[0].string == "static":
            self.static = True
            tokensUsed += 1;
        
        if len(tokens) < tokensUsed+1:
            return
        
        tmp = valType(tokens[tokensUsed:])
        if not tmp.isValid():
            return
        
        tokensUsed += tmp.tokensUsed()
        self.retType = tmp
        
        
        if len(tokens) < tokensUsed+1:
            return
        
        self.name = tokens[tokensUsed].string 
        tokensUsed += 1
        if not self.name.isalnum():
            return
        
        if len(tokens) < tokensUsed+1:
            return
        
        if tokens[tokensUsed].string != "(":
            return
        tokensUsed += 1
        
        
        if len(tokens) < tokensUsed+1:
            return
        
        args = []
        while tokens[tokensUsed].string != ")":
            val = valType(tokens[tokensUsed:])
            if not val.isValid():
                return
            tokensUsed += val.tokensUsed()
            
            
            if len(tokens) < tokensUsed+1:
                return
            
            name = tokens[tokensUsed].string
            if not name.isalnum():
                return
            tokensUsed += 1
            args.append((val, name))
            
            if len(tokens) < tokensUsed+1:
                return
            
            if tokens[tokensUsed].string == ",":
                tokensUsed += 1
                if len(tokens) < tokensUsed+1:
                    return
                if tokens[tokensUsed].string == ")":
                    return
        tokensUsed += 1
        self.args = args
        
        self._tokensUsed = tokensUsed
        
    def isValid(self):
        return (self._tokensUsed != 0)
    def tokensUsed(self):
        return self._tokensUsed
    
    def expectsCode(self):
        return (self.code == None)
    def setCode(self, code):
        self.code = code
    
    def __repr__(self):
        result = repr(self.retType) + " " + self.name + "("
        for x in self.args:
            result += repr(x[0]) + " " + x[1] + ", "
        if len(self.args) > 0:
            result = result[:-2]
        result += ")"
        if self.code != None:
            result += " " + repr(self.code)
        else:
            self.code += ";"
        return result

class codeBlock:
    def __init__(self, tokens):
        self.code = None
        self.closed = False
        
        if tokens[0].string != "{":
            return
        self.code = []
    
    def isValid(self):
        return (self.code != None)
    def tokensUsed(self):
        if self.code != None:
            return 1
        return 0
    
    def expectsCode(self):
        return not self.closed
    def setCode(self, code):
        if isinstance(code, codeBlockEnd):
            self.closed = True
        else:
            self.code.append(code)
    
    def __repr__(self):
        result = "{"
        if len(self.code) > 0:
            result += "\n"
        else:
            result += " "
        for x in self.code:
            tmp = repr(x).split("\n")
            for y in tmp:
                if y.strip() != "":
                    result += "\t" + y + "\n"
        result += "}"
        
        return result

class codeBlockEnd:
    def __init__(self, tokens):
        self.valid = False
        if tokens[0].string != "}":
            return
        self.valid = True
    
    def isValid(self):
        return self.valid
    def tokensUsed(self):
        if self.valid:
            return 1
        return 0
    
    def expectsCode(self):
        return False
    def setCode(self, code):
        pass
    
    def __eq__(self, other):
        return self.valid == other.valid
    def __ne__(self, other):
        return not self.__eq__(other)

class stage2:
    def printError(self, error, token, charNumOffset=0):
        charNum = token.charNum + charNumOffset
        printError("Char " + str(charNum) + ": " + error, token.lineNum)
    
    def __init__(self, callback):
        self.tokens = []
        self.stack = []
        
        self.pushToken = callback
    
    def handleToken(self, token):
        self.pushToken(token)
        self.tokens.append(token)
        
        for tokenClass in [function, codeBlock, codeBlockEnd, variableDeclaration, expression]:
            tmp = tokenClass(self.tokens)
            if tmp.isValid():
                self.tokens = self.tokens[tmp.tokensUsed():]
                if tmp.expectsCode():
                    self.stack.append(tmp)
                else:
                    while len(self.stack) > 0:
                        self.stack[-1].setCode(tmp)
                        tmp = self.stack[-1]
                        if tmp.expectsCode():
                            return
                        self.stack = self.stack[:-1]
                    self.pushToken(tmp)
                return

#TODO: preprocessor
class stage1:
    symbols = ("+", "-", "*", "/", "%", "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
               "~", "&", "|", "^", "<<", ">>", "!", "&&", "||", "?", ":", "==", "!=", "(", ")", "++", "--",
               ".", "->", "<", "<=", ">", ">=", "[", "]", ",", '"', "'", "{", "}", ";")
    
    def printError(self, error, lineNum=None, charNum=None):
        if lineNum == None:
            lineNum = self.lineNum
        if charNum == None:
            charNum = self.charNum
        printError("Char " + str(charNum) + ": " + error, lineNum)
        
    def __init__(self, callback):
        self.state = None
        self.escapeNextChar = False
        
        self.curPart = ""
        self.startCharNum = 0
        self.startLineNum = 0
        
        self.charNum = 0
        self.lineNum = 1
        
        self.pushToken = callback
    
    def handleState(self, char, curState, nextState):
        if curState != nextState:
            if curState == "alnum":
                self.pushToken(token(self.curPart, self.startLineNum, self.startCharNum))
            elif curState == "symbol":
                if self.curPart not in self.symbols:
                    self.printError("Unrecognized symbol "+repr(self.curPart), charNum=self.startCharNum, lineNum=self.startLineNum)
                self.pushToken(token(self.curPart, self.startLineNum, self.startCharNum))
            elif curState == "str":
                if nextState != None:
                    self.printError("Unexpected end of string")
                self.pushToken(token('"'+self.curPart+'"', self.startLineNum, self.startCharNum))
            elif curState == "char":
                if (nextState != None) or (len(self.curPart) < 1):
                    self.printError("Unexpected end of char")
                    return
                if len(self.curPart) > 1:
                    self.printError("A char literal can only contain one character", charNum=self.startCharNum, lineNum=self.startLineNum)
                self.pushToken(token("'"+self.curPart[0]+"'", self.startLineNum, self.startCharNum))
            
            if nextState in ("alnum", "symbol", "str", "char"):
                self.curPart = char
                if nextState in ("char", "str"):
                    self.curPart = ""
                self.startCharNum = self.charNum
                self.startLineNum = self.lineNum
        else:
            if curState in ("char", "str", "alnum", "symbol"):
                self.curPart += char
            if curState == "symbol":
                if self.curPart not in self.symbols:
                    if self.curPart[:-1] in self.symbols:
                        self.curPart = self.curPart[:-1]
                        self.handleState(char, curState, None)
                        self.handleState(char, None, curState)
                    else:
                        self.printError("Unrecognized symbol "+repr(self.curPart), charNum=self.startCharNum, lineNum=self.startLineNum)
                        self.handleState(char, None)
                        self.curPart = self.curPart[:-1]
    
    def readChar(self, char):
        self.charNum += 1
        
        nextState = self.state
        if self.escapeNextChar and self.state in ("char", "str"):
            pass
        elif char == '\\' and self.state in ("char", "str"):
            self.escapeNextChar = True
            return
        elif char == '\n':
            nextState = None #"newLine"
        elif self.state == "str":
            if char == '"':
                nextState = None
        elif self.state == "char":
            if char == "'":
                nextState = None
        elif char.isalnum():
            nextState = "alnum"
        elif char.isspace():
            nextState = None
        elif char == '"':
            nextState = "str"
        elif char == "'":
            nextState = "char"
        else:
            nextState = "symbol"
        
        self.handleState(char, self.state, nextState)
        
        self.state = nextState
        if char == '\n':
            self.lineNum += 1
            self.charNum = 0

class reader:
    operations = ("while", "for", "if", "do", "break")
    
    def printError(self, error, lineNum=None, charNum=None):
        if lineNum == None:
            lineNum = self.lineNum
        if charNum != None:
            error = "Char " + str(charNum) + ": " + error
        printError(error, lineNum)
        
    def __init__(self):
        self.stage2 = stage2(self.pushToken)
        self.stage1 = stage1(self.stage2.handleToken)
        pass
        
    #This compiler prints code as it is being parsed/translated
    #This function controls what actually gets printed
    def printCode(self, code, compilerLevel):
        if compilerLevel > 0:  #First 2 code levels are just echos of the input code
            print(code)
    
    def pushToken(self, token):
        self.printCode(repr(token), 2)
    def finish(self):
        return
        for x in self.stage2.tokens:
            self.pushToken(x)
    
    def readChar(self, char):
        self.stage1.readChar(char)
    
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

def main(argv):
    asm = open(argv[0], 'r')
    read = reader()
    read.read(asm)
    asm.close()
    read.readChar("\n")
    read.finish()
    #obj = open(argv[1], 'wb')
    #write = writer(read.instructions, read.labels, read.origin)
    #write.writeFile(obj)
    #obj.close()

if __name__ == "__main__":
    main(sys.argv[1:])
