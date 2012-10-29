
##def _grouped(string):
##    if not (string.startswith("(") and string.endswith(")")):
##        return False
##    string = string[1:-1]
##    grouping_level = 1
##    for c in string:
##        if c in ("("):
##            grouping_level += 1
##        elif c in (")"):
##            grouping_level -= 1
##            if grouping_level <= 0:
##                return False
##    return grouping_level == 1

class wordString(tuple):
    def __init__(self, value):
        self.value = None
        if type(value) == type(""):
            self.value = tuple(value.encode("ascii"))
            return
        if type(value) == type(b''):
            self.value = tuple(value)
            return
        if type(value) == type((42,)):
            for i in value:
                if (i & 0xFFFF) != i:
                    raise Exception("Value out of bounds for a string represented as a tuple")
            self.value = value
            return
    def __add__(self, b):
        return wordString(self.value + b.value)
    def __str__(self):
        tmp = []
        for i in self.value:
            tmp.append(i & 0x7F)
        return bytes(tmp).decode("ascii")
    def __repr__(self):
        return repr(self.value)
def tokenize(string, ops):
    tokens = []
    grouping_level = 0
    inString = None
    params = None
    literal = ""
    mode = "none"
    nextCharEscaped = False
    
    for i in range(1, len(string)+1):
        c = string[i-1]
        if not nextCharEscaped:
            if c in ('"',"'"):
                if inString == None:
                    inString = c
                elif c == inString:
                    inString = None
            elif c in ("(",) and inString == None:
                grouping_level += 1
            elif c in (")",) and inString == None:
                grouping_level -= 1

        if mode == "op" and (c.isalnum() or c.isspace() or c in ("_", "(", ")", '"', "'", ".")):
            if literal != "":
                if literal not in ops:
                    raise Exception("Unexpected operater \"" + literal + "\" at char " + repr(i-len(literal)+1))
                tokens.append( ("op", literal) )
            literal = ""
        
        if c.isspace() and inString == None and grouping_level == 0:
            if mode == "literal":
                tokens.append( ("literal", literal) )
                mode = "expectOp"
            continue
        
        if (c in ("(", ")") or grouping_level != 0) and inString == None:
            if mode in ("literal"):
                if "func" not in ops:
                    raise Exception("Unexpected \""+c+"\" at char " + repr(i) + "    (Functions are not supported)")
                params = [literal]
                literal = ""
                mode = "func"
                continue
            if mode in ("expectOp"):
                raise Exception("Unexpected \""+c+"\" at char " + repr(i))
            if mode not in ("func", "group"):
                mode = "group"
                literal = ""
                continue
            if mode == "func" and ((c == "," and grouping_level == 1) or (c == ")" and grouping_level == 0)):
                if literal == "" and not (len(params) == 1 and c == ")" and grouping_level == 0):
                    raise Exception("Param at char "+repr(i-1)+" is empty")
                if literal != "":
                    params.append( tokenize(literal, ops) )
                literal = ""
                if not (c == ")" and grouping_level == 0):
                    continue
            if c == ")" and grouping_level == 0:
                if mode == "group":
                    if literal == "":
                        raise Exception("Grouping at char "+repr(i-1)+" is empty")
                    tokens.append( ("group", tokenize(literal, ops)) )
                elif mode == "func":
                    tokens.append( ("func", params[0], tuple(params[1:])) )
                mode = "expectOp"
                continue
            literal += c
            continue
        if inString != None or c in ('"',"'"):
            if mode in ("literal", "expectOp"):
                raise Exception("Unexpected string at char " + repr(i))
            mode="str"
            literal += c
            if (c != "\\" and not nextCharEscaped) or nextCharEscaped:
                nextCharEscaped = False
            else:
                nextCharEscaped = True
                
            if not inString:
                tokens.append(( "literal", literal) )
                literal = ""
                mode = "expectOp"
            continue
            
        if c.isalnum() or c in ("_",) or (c=="." and literal==""):
            if mode in ("expectOp"):
                raise Exception("Unexpected character \""+c+"\" at char " + repr(i))
            mode = "literal"
            literal += c
            continue
        
        #Its an Op!
        if mode in ("literal",):
            tokens.append( ("literal", literal) )
        if mode in ("literal", "expectOp", "none"):
            literal = ""
            mode = "op"
        literal += c
        if len(literal) > 1:
            if literal[:-1] in ops and literal not in ops:
                tokens.append( ("op", literal[:-1]) )
                literal = literal[-1]
    if mode in ("literal",):
        tokens.append( ("literal", literal) )
    if mode in ("func", "group", "str"):
        raise Exception("Unexpected end of line")
    if mode in ("op",):
        if literal != "":
            if literal not in ops:
                raise Exception("Unexpected operater \"" + literal + "\" at char " + repr(i-len(literal)+1))
            tokens.append( ("op", literal) )
        
    return tuple(tokens)

def _split(tokens, spliters):
    if tokens == None:
        return None
    splitTokens = []
    operation = None
    operand = []
    for i in range(len(tokens)):
        t = tokens[i]
        if t[0] == "group":
            operand.append( ("group", _split(t[1], spliters)) )
        elif t[0] == "func":
            params = []
            for x in range(len(t[2])):
                params.append(_split(t[2][x], spliters))
            operand.append( ("func", t[1], tuple(params)) )
        elif t[0] in ("pOp"):
            operand.append( ("pOp", t[1], _split(t[2], spliters), _split(t[3], spliters)) )
        elif t[0] in ("literal"):
            operand.append( t )
        if t[0] in ("literal", "pOp", "group"):
            if operation != None:
                if not operation[1].startswith("@"):      #operation[1].endswith("@"):
                    operation = ("pOp", operation[1], operation[2], tuple(operand))
                    operand = [operation]
                    operation = None
        elif t[0] == "op":
            op = t[1]
            if i-1 < 0 or tokens[i-1][0] == "op":
                op += "@"
            if i+1 >= len(tokens) or tokens[i+1][0] == "op":
                op = "@" + op
            if op in spliters:
                if operation != None:
                    operation = ("pOp", operation[1], operation[2], tuple(operand))
                    operand = [operation]
                    operation = None
                if op.endswith("@"):
                    splitTokens.extend(operand)
                    operation = ("pOp", op, None, None)
                else:
                    operation = ("pOp", op, tuple(operand), None)
                operand = []
                if op.startswith("@"):
                    splitTokens.append(tuple(operation))
                    operation = None
            else:
                operand.append( t )
    if operation != None:
        while len(operand) == 1 and operand[0][0] == "group":
            operand = operand[0][1]
        operation = ("pOp", operation[1], operation[2], tuple(operand))
        splitTokens.append(operation)
        operation = None
    else:
        splitTokens.extend(operand)
    return tuple(splitTokens)

def validate(tokens):
    if tokens == None:
        return None
    if len(tokens) != 1:
        raise Exception("Unable to understand expression")
    if tokens[0][0] == "group":
        return validate(tokens[0][1])
    if tokens[0][0] == "pOp":
        return (tokens[0][1], validate(tokens[0][2]), validate(tokens[0][3]))
    if tokens[0][0] == "literal":
        return tokens[0][1]
    if tokens[0][0] == "func":
        oldParams = tokens[0][2]
        newParams = []
        for x in oldParams:
            newParams.append(validate(x))
        return ("func", tokens[0][1], tuple(newParams))
    raise Exception("Unable to understand expression")

def _evaluate(operator_callbacks, operation, labels, globalLabel=""):
    if type(operation) == type((10,)):
        op = operation[0]
        if op == "constant":
            return _evaluate(operator_callbacks, operation[1], labels, globalLabel)
        a = operation[1]
        if a != None and op != "func":
            a = _evaluate(operator_callbacks, a, labels, globalLabel)
        b = operation[2]
        if b != None:
            if op == "func":
                if a not in ("isdef"):
                    params = []
                    for param in b:
                        params.append(_evaluate(operator_callbacks, param, labels, globalLabel))
                    b = tuple(params)
            else:
                b = _evaluate(operator_callbacks, b, labels, globalLabel)
        return operator_callbacks[ op ]( a, b )
    if type(operation) == type(""):
        if operation.startswith("_") or operation.startswith("."):
            operation = globalLabel + "$" + operation[1:]
        if operation.lower().startswith("0x"):
            return int(operation[2:], 16)
        elif operation.lower().startswith("0b"):
            return int(operation[2:], 2)
        elif operation.lower().startswith("0o"):
            return int(operation[2:], 8)
        elif operation.isdecimal():
            return int(operation)
        elif operation.startswith("\"") and operation.endswith("\""):
            return wordString( tuple(operation[1:-1].encode("UTF").decode("unicode_escape").encode("ascii")) )
            #return tuple(operation[1:-1].encode("UTF").decode("unicode_escape").encode("ascii"))
        elif operation.startswith("'") and operation.endswith("'"):
            tmp = tuple(operation[1:-1].encode("UTF").decode("unicode_escape").encode("ascii"))
            if len(tmp) != 1:
                raise(Exception("Characters literals can not be empty and can not contain more than one character"))
            return tmp[0]
        else:
            try:
                return labels[operation]
            except KeyError as err:
                raise(NameError("'"+err.args[0]+"'", *(err.args[1:])))
def evaluate(operator_callbacks, opOrder, expression, labels, globalLabel="", preEval=False):
    ops = []
    for x in opOrder:
        for y in x:
            op = y
            if op.startswith("@"):
                op = op[1:]
            if op.endswith("@"):
                op = op[:-1]
            if op not in ops:
                ops.append(op)
    expression = expression.strip()
    
    tokens = tokenize(expression, tuple(ops))
    for op in opOrder:
        tokens = _split(tokens, op)
    operation = validate(tokens)

    if preEval:
        if type(operation) == type(""):
            return ("constant", operation)
        return operation
    return _evaluate(operator_callbacks, operation, labels, globalLabel)

def extractVaribles(operation, globalLabel=""):
    if operation == None:
        return ()
    if type(operation) == type(()):
        if operation[0] == "func":
            result = ()
            for x in operation[2]:
                result += extractVaribles(x, globalLabel)
            return result
        elif operation[0] == "constant":
            return extractVaribles(operation[1], globalLabel)
        else:
            if operation[0] == "@$@":
                return ("$$curAddress",)
            return extractVaribles(operation[1], globalLabel) + extractVaribles(operation[2], globalLabel)
    if type(operation) == type(""):
        if operation[0].isdecimal():
            return ()
        elif operation.startswith("_") or operation.startswith("."):
            return (globalLabel + "$" + operation[1:],)
        elif operation.startswith("\"") and operation.endswith("\""):
            return ()
        elif operation.startswith("'") and operation.endswith("'"):
            return ()
        else:
            return (operation,)
    raise Exception("Unabled to understand operation")
def eval_0xSCAmodified(expression, labels, globalLabel="", preEval=False):
    operators = {}
    def function(a, b):
        if a == "isdef":
            if len(b) != 1:
                raise Exception("Function \"" + a + "\" is not defined for " + repr(len(b)) + " parameters")
            if type(b[0]) == type(()) and len(b[0]) != 1:
                raise Exception("Function \"" + a + "\" is defined only for a single varible parameter")
            if b[0] in labels:
                return 1
            return 0
        if not (a in labels and (type(labels[a]) == type(function) or type(labels[a]) == type(hex) or type(labels[a]) == type(int))):
            raise Exception("Function \"" + a + "\" is undefined")
        func = labels[a]
        params = []
        result = func(*b)   #Potential TODO: adapt internal tuple represention to a string or bytes
        if type(result) == type(""):
            return wordString( tuple(result.encode("ascii")) )
            #return tuple(result.encode("ascii"))
        return result
        #if len(b) != 0:
        #    raise Exception("Function \"" + a + "\" is not defined for " + repr(len(b)) + " parameters")
        #return 42
    operators["func"] = function
    
    operators["-@"] = lambda a, b: -b
    operators["~@"] = lambda a, b: ~b
    def NOT(a, b):
        if b == 0:
            return 1
        return 0
    operators["!@"] = NOT
    
    operators["+"] = lambda a, b: a + b
    operators["-"] = lambda a, b: a - b
    operators["*"] = lambda a, b: a * b
    operators["/"] = lambda a, b: int(a / b)
    operators["%"] = lambda a, b: a % b
    
    operators["=="] = lambda a, b: a == b
    operators["!="] = lambda a, b: a != b
    operators["<>"] = operators["!="] #Bah
    operators["<"] = lambda a, b: a < b
    operators["<="] = lambda a, b: a <= b
    operators[">"] = lambda a, b: a > b
    operators[">="] = lambda a, b: a >= b

    def bAND(a, b):
        if type(a) == type(wordString(())) and type(b) == type(wordString(())):
            if len(a) < len(b):
                a = list(a)
                a.append((0 for i in range(len(b) - len(a))))
            elif len(b) < len(b):
                b = list(a)
                b.append((0 for i in range(len(a) - len(b))))
            return wordString(tuple(( (a[i] & b[i]) for i in range(len(a)) )))
        if type(a) == type(wordString):
            return wordString(tuple( ((x & b) for x in a) ))
        if type(b) == type(wordString):
            return bAND(b, a)
        return a & b
    def bOR(a, b):
        if type(a) == type(wordString(())) and type(b) == type(wordString(())):
            if len(a) < len(b):
                a = list(a)
                a.append((0 for i in range(len(b) - len(a))))
            elif len(b) < len(b):
                b = list(a)
                b.append((0 for i in range(len(a) - len(b))))
            return wordString(tuple(( (a[i] | b[i]) for i in range(len(a)) )))
        if type(a) == type(wordString(())):
            return wordString(tuple( ((x | b) for x in a) ))
        if type(b) == type(wordString(())):
            return bOR(b, a)
        return a | b
    def bXOR(a, b):
        if type(a) == type(wordString(())) and type(b) == type(wordString(())):
            if len(a) < len(b):
                a = list(a)
                a.append((0 for i in range(len(b) - len(a))))
            elif len(b) < len(b):
                b = list(a)
                b.append((0 for i in range(len(a) - len(b))))
            return wordString(tuple(( (a[i] ^ b[i]) for i in range(len(a)) )))
        if type(a) == type(wordString(())):
            return wordString(tuple( ((x ^ b) for x in a) ))
        if type(b) == type(wordString(())):
            return bXOR(b, a)
        return a ^ b
    operators["&"] = bAND
    operators["|"] = bOR
    operators["^"] = bXOR
    operators["&&"] = lambda a, b: a and b
    operators["||"] = lambda a, b: a or b
    operators["^^"] = operators["!="]   #Yep

    opOrder = [("*", "/"), ("%",), ("+", "-")]
    opOrder.extend([("==", "!=", "<>", "<", ">", "<=", ">="), ("&", "^", "|"), ("&&", "||", "^^")])

    opOrder.extend([("-@","~@","!@")])   #This needs to be split first (or on the end of the list)
    operators["@$@"] = lambda a, b: labels["$$curAddress"]  #add support for curAddress token
    opOrder.append( ("@$@",) )
    opOrder.append( ("func",) )      #Not actually a legal operater, but enables function support (make sure there is a "func" callback)
    
    opOrder.reverse()  #actually want ops from last to first

    try:
        if preEval and type(expression) == type(()):
            return expression
        if type(expression) == type(()):
            return _evaluate(operators, expression, labels, globalLabel)
        return evaluate(operators, opOrder, expression, labels, globalLabel, preEval)
    except Exception as err:
        if err.args[0] == "Unable to understand expression":
            print(repr(expression))
        raise err

def eval_0xSCA(expression, labels, globalLabel=""):
    operators = {}
    operators["+"] = lambda a, b: a + b
    operators["-"] = lambda a, b: a - b
    operators["*"] = lambda a, b: a * b
    operators["/"] = lambda a, b: int(a / b)
    operators["%"] = lambda a, b: a % b
    
    operators["=="] = lambda a, b: a == b
    operators["="] = operators["=="] #Bah
    operators["!="] = lambda a, b: a != b
    operators["<>"] = operators["!="] #Bah
    operators["<"] = lambda a, b: a < b
    operators["<="] = lambda a, b: a <= b
    operators[">"] = lambda a, b: a > b
    operators[">="] = lambda a, b: a >= b
    
    operators["&"] = lambda a, b: a & b
    operators["|"] = lambda a, b: a | b
    operators["^"] = lambda a, b: a ^ b
    operators["&&"] = lambda a, b: a and b
    operators["||"] = lambda a, b: a or b
    operators["^^"] = operators["!="]   #Yep

    opOrder = ["!", "*", "/", "%", "+", "-"]
    opOrder.extend(["==", "=", "!=", "<>", "<", ">", "<=", ">=", "&", "^", "|", "&&", "||", "^^"])
    
    opOrder.reverse()  #actually want ops from last to first
    return evaluate(operators, opOrder, expression, labels, globalLabel)
