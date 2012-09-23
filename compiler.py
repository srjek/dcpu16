import sys

def printError(error, lineNum, file=sys.stderr):
    print("Line " + str(lineNum) + ": " + str(error), file=file)
    
class reader:
    symbols = ("+", "-", "*", "/", "%", "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
               "~", "&", "|", "^", "<<", ">>", "!", "&&", "||", "?", ":", "==", "!=", "(", ")", "++", "--",
               ".", "->", "<", "<=", ">", ">=", "[", "]", ",", '"', "'")
    operations = ("while", "for", "if")
    def __init__(self):
        self.curPart = ""
        self.mode = "none"
        self.lineNum = 1
        self.charNum = 0
        self.inStr = None
        self.grouping_level = 0
        self.comment = None
        self.tokens = []
    def printError(self, error, lineNum=None, charNum=None):
        if lineNum == None:
            lineNum = self.lineNum
        if charNum != None:
            error = "Char " + str(charNum) + ": " + error
        printError(error, lineNum)

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
    def parseTokens(self):
        printError = self.printError
        nextToken = self.tokens[0]
        if nextToken[2] in self.operations:
            if self.tokens[1][2] != "(":
                printError("Expected \"(\" after \"" + nextToken + "\" at char " + str(self.tokens[1][1]), self.tokens[1][0])
            param = ""
            i = 2
            while i < len(self.tokens) and self.tokens[i][2] != ")":
                param += " " + self.tokens[i][2]
                i += 1
            print(nextToken[2] + " (" + param[1:] + ")")
        elif nextToken[2] in ("int",):
            name = self.tokens[1]
            if not (name[2].isalnum() and name[2][0].isalpha()):
                printError("Char " + str(name[1]) + ": Variable names must be alphanumeric and start with a letter", name[0])
                (param, i) = self.getParam(3, (";",))
                self.tokens = self.tokens[i+1:]
                return
            print(nextToken[2]+" "+name[2]+";")
            i = 2
            while self.tokens[i][2] != ";":
                if self.tokens[i][2] not in (";", "=", ","):
                    printError("Char " + str(self.tokens[i][1]) + ": Expected an '=', ';', or ','", self.tokens[i][0])
                    (param, i) = self.getParam(3, (";",))
                    break
                if self.tokens[i][2] == "=":
                    (param, i) = self.getParam(i+1, (";", ","))
                    print(name[2]+" = "+param+";")
                if self.tokens[i][2] == ",":
                    name = self.tokens[i+1];
                    if not (name[2].isalnum() and name[2][0].isalpha()):
                        printError("Char " + str(name[1]) + ": Variable names must be alphanumeric and start with a letter", name[0])
                        (param, i) = self.getParam(3, (";",))
                        break
                    i += 2
                    print(nextToken[2]+" "+name[2]+";")
            self.tokens = self.tokens[i+1:]
                    
        return
    
    def pushToken(self, charNum=None):
        if charNum == None:
            charNum = self.charNum
        if self.curPart != "":
            self.tokens.append((self.lineNum, charNum - len(self.curPart), self.curPart))
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
                self.tokens.append(char)
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
