#f = open("C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin", mode='rb')
#w = open("C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.out", mode='w')
f = open("C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.bin", mode='rb')
w = open("C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.out", mode='w')
pos = 0
output = ""
while True:
    tmp = hex(pos*8)[2:]
    while len(tmp) < 4: tmp = "0" + tmp
    output += tmp + ": "
    obj = b''
    try:
        obj = b''
        for i in range(8):
            try:
                w.write("DAT 0x")
                for i2 in range(2):
                    try:
                        tmp = f.read(1)
                        obj += tmp
                        tmp = hex(tmp[0])[2:]
                        while len(tmp) < 2: tmp = "0" + tmp
                        output += tmp
                        w.write(tmp)
                    except IndexError as err:
                        output += "  "
                        if i2 == 1: raise err
                w.write("\n")
                output += " "
            except IndexError as err:
                w.write("00\n")
                output += " "
                if i == 7: raise err
        f.peek(1)[0]
        output += " \""
        for i in range(0, len(obj)):
            tmp = obj[i:i+1]
            if tmp[0] >= 128 or tmp[0] < 32:
                output += " "
                continue
            tmp = tmp.decode("ascii")
            if tmp == "\n":
                tmp = " "
            output += tmp
        output += "\"\n"
    except:
        output += " \""
        for i in range(0, len(obj)):
            tmp = obj[i:i+1]
            if tmp[0] >= 128 or tmp[0] < 32:
                output += " "
                continue
            tmp = tmp.decode("ascii")
            if tmp == "\n":
                tmp = " "
            output += tmp
        output += "\"\n"
        break
    pos += 1
print(output)
w.close()
f.close()
