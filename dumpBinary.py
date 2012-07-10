f = open("C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin", mode='rb')
pos = 0
while True:
    tmp = hex(pos*8)[2:]
    while len(tmp) < 4: tmp = "0" + tmp
    print(tmp + ": ", end='')
    obj = b''
    try:
        obj = b''
        for i in range(8):
            try:
                for i2 in range(2):
                    try:
                        tmp = f.read(1)
                        obj += tmp
                        tmp = hex(tmp[0])[2:]
                        while len(tmp) < 2: tmp = "0" + tmp
                        print(tmp, end='')
                    except IndexError as err:
                        print("  ", end='')
                        if i2 == 1: raise err
                print(" ", end='')
            except IndexError as err:
                print(" ", end='')
                if i == 7: raise err
        f.peek(1)[0]
        print(" \"", end='')
        for i in range(0, len(obj)):
            tmp = obj[i:i+1]
            if tmp[0] >= 128 or tmp[0] < 32:
                print(" ", end='')
                continue
            tmp = tmp.decode("ascii")
            if tmp == "\n":
                tmp = " "
            print(tmp, end='')
        print("\"")
    except:
        print(" \"", end='')
        for i in range(0, len(obj)):
            tmp = obj[i:i+1]
            if tmp[0] >= 128 or tmp[0] < 32:
                print(" ", end='')
                continue
            tmp = tmp.decode("ascii")
            if tmp == "\n":
                tmp = " "
            print(tmp, end='')
        print("\"")
        break
    pos += 1
f.close()
