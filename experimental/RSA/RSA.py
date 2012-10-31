import random
import copy

INT32_BITMASK = 0xFFFFFFFF

#right rotate
def rrot(int32, n):
    while n > 32: n -= 32
    result = int32 >> n
    result |= (int32 << (32 - n)) & INT32_BITMASK
    return result

def _SHA256(msg, msgLen):
    result = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]
    k = [  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
           0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
           0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
           0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
           0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
           0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
           0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
           0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2]
    while (len(msg) % (512/32)) != (448/32):
        msg.append(0)
    msg.append(msgLen >> 32)
    msg.append(msgLen & 0xFFFFFFFF)

    for i in range(int(len(msg)/(512/32))):
        w = msg[i*int(512/32):(i+1)*int(512/32)]
        for i in range(16, 64):
            s0 = rrot(w[i-15], 7) ^ rrot(w[i-15], 18) ^ rrot(w[i-15], 3)
            s1 = rrot(w[i-2], 17) ^ rrot(w[i-2], 19) ^ rrot(w[i-2], 10)
            w.append( (w[i-16] + s0 + w[i-7] + s1) & INT32_BITMASK )
        
        a = result[0]
        b = result[1]
        c = result[2]
        d = result[3]
        e = result[4]
        f = result[5]
        g = result[6]
        h = result[7]
        
        for i in range(0, 64):
            S0 = rrot(a, 2) ^ rrot(a, 13) ^ rrot(a, 22)
            maj = (a & b) ^ (a & c) ^ (b & c)
            t2 = S0 + maj
            S1 = rrot(e, 6) ^ rrot(e, 11) ^ rrot(e, 25)
            ch = (e & f) ^ ((~ e) & g)
            t1 = (h + S1 + ch + k[i] + w[i]) & INT32_BITMASK
            
            h = g
            g = f
            f = e
            e = (d + t1) & INT32_BITMASK
            d = c
            c = b
            b = a
            a = (t1 + t2) & INT32_BITMASK
            
        result[0] += a
        result[1] += b
        result[2] += c
        result[3] += d
        result[4] += e
        result[5] += f
        result[6] += g
        result[7] += h
        for i in range(0, 8):
            result[i] &= INT32_BITMASK
    return result

def SHA256(msg):
    msgLen = len(msg)*32
    msg.append(0x80000000)
    return _SHA256(msg, msgLen)
def SHA256_byte(msg, bah):
    msgLen = len(msg)*8
    msg = copy.copy(msg)
    msg.append(0x80)
    for i in range(0, len(msg)%4):
        msg.append(0)
    tmp = []
    for i in range(0, int(len(msg)/4)):
        y = msg[i*4]
        for x in range(1, 4):
            y = (y << 8) | msg[(i*4)+x]
        tmp.append(y)
    tmp = _SHA256(tmp, msgLen)
    result = []
    for i in tmp:
        result.append(i >> 24)
        result.append((i >> 16)&0xFF)
        result.append((i >> 8)&0xFF)
        result.append(i&0xFF)
    print("0x", end='')
    for i in range(len(tmp)):
        x = hex(tmp[i])[2:]
        while len(x) < 8:
            x = "0" + x
        print(x, end='')
    print("")
    return result

#tmp = OAEP_encode([0, 1, 2, 3], 64, 32, SHA256_byte, SHA256_byte)
def OAEP_encode(msg, n, k0, G, H):
    msg.extend((n-k0-len(msg))*(0,))
    
    r = []
    for i in range(k0):
        r.append(random.randrange(0, 256))
    g = G(r, n-k0)
    X = []
    for i in range(len(msg)):
        X.append(msg[i] ^ g[i])
    h = H(X, k0)
    Y = []
    for i in range(len(r)):
        Y.append(r[i] ^ h[i])
    result = []
    result.extend(X)
    result.extend(Y)
    return result

#OAEP_decode(tmp, 32, SHA256_byte, SHA256_byte)
def OAEP_decode(msg, k0, G, H):
    X = msg[:(len(msg)-k0)]
    Y = msg[(len(msg)-k0):]
    h = H(X, k0)
    r = []
    for i in range(len(Y)):
        r.append(Y[i] ^ h[i])
    g = G(r, len(msg)-k0)
    result = []
    for i in range(len(X)):
        result.append(X[i] ^ g[i])
    return result

#Use public key to encrypt
#Use private key to decrypt
def _RSA_int64(msg, key):
    return (msg**key[1]) % key[0]
def RSA_int64(msg, key):
    result = []
    for x in msg:
        result.append(_RSA_int64(x, key))
    return result
def RSA_int8(msg, key):
    msg = copy.copy(msg)
    while len(msg) % 8 != 0:
        msg.append(0)
    tmp = []
    for i in range(int(len(msg)/8)):
        y = msg[i*8]
        for x in range(1, 8):
            y = (y << 8) | msg[(i*8)+x]
        tmp.append(y)
    print(tmp)
    tmp = RSA_int64(tmp, key)
    print(tmp)
    result = []
    for i in tmp:
        result.append((i >> 56)&0xFF)
        result.append((i >> 48)&0xFF)
        result.append((i >> 40)&0xFF)
        result.append((i >> 32)&0xFF)
        result.append((i >> 24)&0xFF)
        result.append((i >> 16)&0xFF)
        result.append((i >> 8)&0xFF)
        result.append(i&0xFF)
    return result

def sign(data, privateKey):
    Hash = SHA256_byte(data, 32)
    paddedHash = OAEP_encode(Hash, 64, 32, SHA256_byte, SHA256_byte)
    signedPaddedHash = RSA_int8(paddedHash, privateKey)
    return signedPaddedHash

def verify(data, signature, publicKey):
    Hash = SHA256_byte(data, 32)
    SigPaddedHash = RSA_int8(signature, publicKey)
    SigHash = OAEP_decode(SigPaddedHash, 32, SHA256_byte, SHA256_byte)
    print(repr(SigHash))
    print(repr(Hash))
    return (Hash == SigHash)
