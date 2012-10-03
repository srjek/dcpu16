import sys

def printError(error, lineNum, file=sys.stderr):
    print("Line " + str(lineNum) + ": " + str(error), file=file)

class code0:
    def __init__(self, token):
        self.lineNum = token[0]
    def needNextCode(self):
        return False
    def receiveNextCode(self, code):
        return
    def __repr__(self):
        return "?"

class statement(code0):
    def __init__(self, token):
        code0.__init__(self, token)
        if token[1] != "statement":
            printError("Passed token was a \"" + token[1] + "\" instead of a statement.", self.lineNum)
        self.statement = token[2]
    def __repr__(self):
        return self.statement + ";"
class declaration(code0):
    def __init__(self, token):
        code0.__init__(self, token)
        if token[1] != "declaration":
            printError("Passed token was a \"" + token[1] + "\" instead of a declaration.", self.lineNum)
        self.type = token[2]
        self.name = token[3]
    def __repr__(self):
        return self.type + " " + self.name + ";"
class block(code0):
    def __init__(self, token):
        code0.__init__(self, token)
        if token[1] != "{":
            printError("Passed token was a \"" + token[1] + "\" instead of a '{'.", self.lineNum)
        self.code = []
        self.closed = False
    def needNextCode(self):
        return not self.closed
    def receiveNextCode(self, code):
        if len(self.code) > 0 and self.code[-1].needNextCode():
            self.code[-1].receiveNextCode(code)
        elif code == "}":
            self.closed = True
        else:
            self.code.append(code)
    def __repr__(self):
        if len(self.code) == 0: return ";"
        result = "{\n"
        for c in self.code:
            tmp = repr(c).split("\n");
            for line in tmp:
                result += "\t" + line + "\n"
        return result + "}"

class op0(code0):
    def __init__(self, token):
        code0.__init__(self, token)
        self.code = None
        if token[1] != "operation":
            printError("Passed token was a \"" + token[1] + "\" instead of a operation.", self.lineNum)
    def needNextCode(self):
        if self.code == None:
            return True
        return self.code.needNextCode()
    def receiveNextCode(self, code):
        if self.code == None:
            self.code = code
        else:
            self.code.receiveNextCode(code)
class opWhile(op0):
    def __init__(self, token):
        op0.__init__(self, token)
        if token[2].lower() != "while":
            printError("Operation was \"" + token[2] + "\", not \"while\" as expected.", self.lineNum)
        self.test = token[3]
    def printError(self, error):
        printError(error, self.lineNum)
    def __repr__(self):
        return "while (" + self.test + ")" + " " + repr(self.code)
class opFor(op0):
    def __init__(self, token):
        op0.__init__(self, token)
        if token[2].lower() != "for":
            printError("Operation was \"" + token[2] + "\", not \"for\" as expected.", self.lineNum)
        
        tmp = token[3].split(";")
        if len(tmp) < 3:
            printError("Expected ';' before ')'", self.lineNum)
            tmp.append("")
            tmp.append("")
        if len(tmp) > 3:
            printError("Unexpected ';' before ')'", self.lineNum)
            
        self.initializer = tmp[0]
        self.test = tmp[1]
        self.increment = tmp[2]
        
    def printError(self, error):
        printError(error, self.lineNum)
    def __repr__(self):
        return "for (" + self.initializer + ";" + self.test + ";" + self.increment + ")" + " " + repr(self.code)
        

def operation(token):
    if token[1].lower() != "operation":
        printError("Passed token was a \"" + token[1] + "\" instead of a operation.")
    if token[2].lower() == "while":
        return opWhile(token)
    elif token[2].lower() == "for":
        return opFor(token)
    elif token[2].lower() == "if":
        return opIf(token)
    elif token[2].lower() == "do":
        return opDoWhile(token)
    else:
        printError("Operation \"" + token[2] + "\" is unknown")
    
class reader:
    symbols = ("+", "-", "*", "/", "%", "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
               "~", "&", "|", "^", "<<", ">>", "!", "&&", "||", "?", ":", "==", "!=", "(", ")", "++", "--",
               ".", "->", "<", "<=", ">", ">=", "[", "]", ",", '"', "'")
    operations = ("while", "for", "if", "do")
    def __init__(self):
        self.curPart = ""
        self.mode = "none"
        self.lineNum = 1
        self.charNum = 0
        self.inStr = None
        self.grouping_level = 0
        self.comment = None
        self.tokens = []
        self.parseLevel = 0
        self.tokens2 = []
        self.code = []
    def printError(self, error, lineNum=None, charNum=None):
        if lineNum == None:
            lineNum = self.lineNum
        if charNum != None:
            error = "Char " + str(charNum) + ": " + error
        printError(error, lineNum)
    #This compiler prints code as it is being parsed/translated
    #This function controls what actually gets printed
    def printCode(self, code, compilerLevel):
        #if compilerLevel > 2:  #First 2 code levels are just echos of the input code
            print(code)
    
    def getParam(self, startI, endChars):
        printError = self.printError
        i = startI
        grouping = []
        param = ""
        while not (len(grouping) == 0 and self.tokens[i][2] in endChars):
            token = self.tokens[i][2]
            if token in ("(", "{", "["):
                grouping.append(token)
            elif token in (")", "}", "]"):
                if grouping[-1] != token:
                    printError("Unexpected char '"+token+"'", self.tokens[i][0], self.tokens[i][1])
                    return None
                grouping.pop(-1)
            param += " " + token
            i += 1
        return (param[1:], i)
    def parseTokens2(self):
        printError = self.printError
        printCode = self.printCode
        while len(self.tokens2) > 0:
            nextToken = self.tokens2[0]
            self.tokens2 = self.tokens2[1:]
            tmp = None
            if nextToken[1] == "{":
                tmp = block(nextToken)
            elif nextToken[1] == "}":
                tmp = "}"
            elif nextToken[1] == "nothing":
                tmp = statement((nextToken[0], "statement", ""));
                #printCode(";", 2)
            elif nextToken[1] == "operation":
                tmp = operation(nextToken)
                #printCode(nextToken[2] + " (" + nextToken[3] + ")", 2)
            elif nextToken[1] == "declaration":
                tmp = declaration(nextToken)
                #printCode(nextToken[2] + " " + nextToken[3] + ";", 2)
            elif nextToken[1] == "statement":
                tmp = statement(nextToken)
                #printCode(nextToken[2] + ";", 2)
            else:
                print("Failed to parse \""+nextToken[1]+"\" token during level 2 parse")
                continue
            if len(self.code) > 0 and self.code[-1].needNextCode():
                self.code[-1].receiveNextCode(tmp)
                if not self.code[-1].needNextCode():
                    printCode(repr(self.code[-1]), 2);
                continue
            self.code.append(tmp);
            if not tmp.needNextCode():
                printCode(repr(tmp), 2);
            
    def parseTokens(self):
        printError = self.printError
        printCode = self.printCode
        while len(self.tokens) > 0:
            nextToken = self.tokens[0]
            if nextToken[2].lower() in self.operations:
                if self.tokens[1][2] != "(":
                    printError("Expected \"(\" after \"" + nextToken + "\" at char " + str(self.tokens[1][1]), self.tokens[1][0])
                param = ""
                i = 2
                while i < len(self.tokens) and self.tokens[i][2] != ")":
                    param += " " + self.tokens[i][2]
                    i += 1
                printCode(nextToken[2] + " (" + param[1:] + ")", 1)
                self.tokens2.append((nextToken[0], "operation", nextToken[2], param[1:]))
                self.tokens = self.tokens[i+1:]
            elif nextToken[2] == ";":
                printCode(";", 1)
                self.tokens2.append((nextToken[0],"nothing"));
                self.tokens = self.tokens[1:]
            elif nextToken[2] in ("{", "}"):
                printCode(nextToken[2], 1);
                self.tokens2.append((nextToken[0], nextToken[2]))
                self.tokens = self.tokens[1:]
            elif nextToken[2].lower() in ("int",):
                name = self.tokens[1]
                if not (name[2].isalnum() and name[2][0].isalpha()):
                    printError("Char " + str(name[1]) + ": Variable names must be alphanumeric and start with a letter", name[0])
                    (param, i) = self.getParam(3, (";",))
                    self.tokens = self.tokens[i+1:]
                    return
                printCode(nextToken[2]+" "+name[2]+";", 1)
                self.tokens2.append((nextToken[0], "declaration", nextToken[2], name[2]))
                i = 2
                while self.tokens[i][2] != ";":
                    if self.tokens[i][2] not in (";", "=", ","):
                        printError("Char " + str(self.tokens[i][1]) + ": Expected an '=', ';', or ','", self.tokens[i][0])
                        (param, i) = self.getParam(3, (";",))
                        break
                    if self.tokens[i][2] == "=":
                        tmpLineNum = self.tokens[i+1][0]
                        (param, i) = self.getParam(i+1, (";", ","))
                        printCode(name[2]+" = "+param+";", 1)
                        self.tokens2.append((tmpLineNum, "statement", name[2]+" = "+param))
                    if self.tokens[i][2] == ",":
                        name = self.tokens[i+1];
                        if not (name[2].isalnum() and name[2][0].isalpha()):
                            printError("Char " + str(name[1]) + ": Variable names must be alphanumeric and start with a letter", name[0])
                            (param, i) = self.getParam(3, (";",))
                            break
                        printCode(nextToken[2]+" "+name[2]+";", 1)
                        self.tokens2.append((self.tokens[i+1][0], "declaration", nextToken[2], name[2]))
                        i += 2
                self.tokens = self.tokens[i+1:]
            else:
                statement = nextToken[2]
                i = 1
                while (self.tokens[i][2] != ";"):
                    statement += " " + self.tokens[i][2]
                    i += 1
                printCode(statement+";", 1)
                self.tokens2.append((nextToken[0], "statement", statement))
                self.tokens = self.tokens[i+1:]
        return
    
    def pushToken(self, charNum=None, token=None):
        if charNum == None:
            charNum = self.charNum
        if token != None or self.curPart != "":
            if token == None:
                token = self.curPart
            self.tokens.append((self.lineNum, charNum - len(token), token))
        self.curPart = ""
    def readChar(self, char):
        printError = self.printError
        self.charNum += 1
        if char in ("\n",):
            if self.comment == "\n":
                self.comment = None
            if self.inStr != None:
                if self.curPart[-1] != "\\":
                    printError("Unexpected linebreak in string")
            else:
                if self.mode in ("alnum", "symbol"):
                    self.pushToken()
            self.lineNum += 1
            self.charNum = 0
            return
        if self.comment != None:
            if self.comment != "\n":
                if self.comment[0] == char:
                    self.comment = self.comment[1:]
                elif char != "*":
                    self.comment = "*/"
                if self.comment == "":
                    self.comment = None
            return
        if self.inStr != None:
            if char == self.inStr and self.curPart[-1] != "\\":
                self.inStr = None
                self.curPart += char
                self.pushToken()
                self.mode = "none"
                return
        elif char.isspace():
            if self.mode in ("alnum", "symbol"):
                self.pushToken()
            self.mode = "none"
            return
        elif char.isalnum():
            if self.mode in ("symbol",):
                self.pushToken()
            self.mode = "alnum"
        else:
            if self.mode in ("alnum",) or char in ("(", ")") or (char in ('"', "'") and self.inStr == None):
                self.pushToken()
            self.mode = "symbol"
            if char in ('"', "'") and self.inStr == None:
                self.inStr = char
            if char in ("(", ")"):
                self.pushToken(token=char); #self.tokens.append(char)
                return
            if self.curPart != "" and self.inStr == None and (self.curPart+char) not in self.symbols:
                self.pushToken()
        self.curPart += char
        if self.mode == "symbol" and self.inStr == None:
            if self.curPart[-2:] in ("//", "/*"):
                if self.curPart[-2:] == "//":
                    self.comment = "\n"
                else:
                    self.comment = "*/"
                if len(self.curPart) > 2:
                    self.curPart = self.curPart[:-2]
                    self.pushToken(self.charNum-1)
                self.curPart = ""
            
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
    #obj = open(argv[1], 'wb')
    #write = writer(read.instructions, read.labels, read.origin)
    #write.writeFile(obj)
    #obj.close()

if __name__ == "__main__":
    main(sys.argv[1:])
