#Implemented based of some reports from people at the uni of buffalo from the 1970s

#4 ops?
def ME(ternary):
    result = [0, 0]
    result[0] = ternary[0] & (~ternary[1])
    result[1] = ternary[1] & (~ternary[0])
    return tuple(result)

#6 ops?
def minor(ternary):
    result = [0, 0]
    result[0] = (ternary[1] ^ ternary[0]) | ternary[0]
    result[1] = (ternary[1] ^ ternary[0]) ^ result[0]
    return tuple(result)

def negate(ternary):
    return (ternary[1], ternary[0])

#6+ME ops? (10)
def partialAdd(a, b):   #Assumes that (b[1] == 0) is true (hence partial)
    x = a[0] + b[0]
    y = a[0] & (~x)
    r2 = y | a[1]
    r1 = x & (~b[0])
    return ME((r1, r2))

#2*(ME+minor+partialAdd) ops? (40)
def add(a, b):
    a = list(a)
    b = list(b)
    (b[0], a[1]) = ME((b[0], a[1]))
    (a[0], b[0]) = minor((a[0], b[0]))
    c = list(partialAdd(a, b))
    (b[1], c[0]) = ME((b[1], c[0]))
    (c[1], b[1]) = minor((c[1], b[1]))
    return negate( partialAdd( negate(c), negate(b) ))

#4 ops?
def trit(a, pos):
    return (a[0] & (1 << pos)) - (a[1] & (1 << pos))
#2 ops?
def shiftL(a, i):
    return (a[0] << i, a[1] << i)
#12*(2+trit+shiftL+add) ops? (12*48=576)
def multiply(a, b):
    result = (0, 0)
    for i in range(12):
        t = trit(b, i)
        if t > 0:
            result = add(result, shiftL(a, i))
        elif t < 0:
            result = add(result, shiftL(negate(a), i))
    return result

def metaOR(a, b):
    return (a[0] | b[0], a[1] | b[1])

def trinary(trinary):
    result = []
    for i in range(12):
        t = trit(trinary, i)
        if t > 0:
            result.append("+")
        elif t < 0:
            result.append("-")
        else:
            result.append("0")
    result.reverse()
    return "".join(result)
def tb(trinary):
    result = 0
    for i in range(12):
        t = trit(trinary, i)
        if t > 0:
            result += pow(3, i)
        elif t < 0:
            result -= pow(3, i)
    return result

def test(x):
    r1 = 0
    r2 = 0
    a = 5
    while a >= 0:
        magic = pow(3, a)
        for i in range(a):
            magic -= pow(3, i)
        if x >= magic:
            r1 += (1 << a)
            x -= pow(3, a)
        elif x <= -magic:
            r2 += (1 << a)
            x += pow(3, a)
        a -= 1
    return (r1, r2)
metaOR(shiftL(test(1), 6),test(243))
