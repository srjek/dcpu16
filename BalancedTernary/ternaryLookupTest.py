#tbLookup = []

#tbDict = {"-":0, "0":1, "+"}
#for i in range(256):

print("1 tryte lookups require 16bits for both tryte and binary")
print("1 tryte lookups need " + str(2*pow(2,12)/1024) + " KB for the lookup table\n")
print("2 tryte lookups require 24bits for both trytes and binary")
print("2 tryte lookups need " + str(3*pow(2,24)/1024) + " KB for the lookup table")
print("2 tryte lookups need " + str(3*pow(2,24)/pow(1024,2)) + " MB for the lookup table\n")
print("2 tryte lookups require 40bits(5bytes) for trytes and 32bits for binary")
print("3 tryte lookups need " + str(4*pow(2,36)/1024) + " KB for the lookup table")
print("3 tryte lookups need " + str(4*pow(2,36)/pow(1024,2)) + " MB for the lookup table")
print("3 tryte lookups need " + str(4*pow(2,36)/pow(1024,3)) + " GB for the lookup table")

failme()
print("1 tryte lookups need " + str(pow(2,12)/1024) + " KB for the lookup table\n")
print("2 tryte lookups need " + str(pow(2,24)/1024) + " KB for the lookup table")
print("2 tryte lookups need " + str(pow(2,24)/pow(1024,2)) + " MB for the lookup table\n")
print("3 tryte lookups need " + str(pow(2,36)/1024) + " KB for the lookup table")
print("3 tryte lookups need " + str(pow(2,36)/pow(1024,2)) + " MB for the lookup table")
print("3 tryte lookups need " + str(pow(2,36)/pow(1024,3)) + " GB for the lookup table")
    
#2 tryte lookup with 1-tryte table
result = lookup[tryte[0]] + lookup[tryte[1]] * pow(3, 6)
result = lookup[tryte[1]]
result *= pow(3, 6)
result += lookup[tryte[0]]
#3 tryte lookup with 1-tryte table
result = lookup[tryte[0]] + lookup[tryte[1]] * pow(3, 6) + lookup[tryte[2]] * pow(3, 12)
result = lookup[tryte[2]]
result *= pow(3, 6)
result = lookup[tryte[1]]
result *= pow(3, 6)
result += lookup[tryte[0]]

#3 tryte lookup with 2-tryte table
result = lookup[tryte[0-1]] + lookup[tryte[2]] * pow(3, 12)
result = lookup[tryte[2]]
result *= pow(3, 12)
result += lookup[tryte[0-1]]

#2 tryte rlookup with 1-tryte table
magic = int((pow(3, 6)-1)/2)
tmp = int(num/magic)
result[0] = rlookup(int(num/magic))
result[1] = rlookup(int(num - tmp*magic))
#result = lookup[tryte[0]] + lookup[tryte[1]] * pow(3, 6)
#result = lookup[tryte[1]]
#result *= pow(3, 6)
#result += lookup[tryte[0]]
a = 2
result = ""
while a >= 0:
    magic = pow(3, a)
    for i in range(a):
        magic -= pow(3, i)
    if x >= magic:
        result += "+"
        x -= pow(3, a)
    elif x <= -magic:
        result += "-"
        x += pow(3, a)
    else:
        result += "0"
    a -= 1

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
