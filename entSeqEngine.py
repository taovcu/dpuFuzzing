###############################################################################
# Execute cmd: python3 entSeqEngine.py 4096 2 ent2_4k.bin
# !!! Note, because random algorithms are used, you may need to try more than 1 time to successfully generated the target file
#
#     Arg 4096 is number of bytes to generate
#     Arg 2 is entropy. The lower this value, the higher the compression ratio
#       -set this value to 3 can generate a file with compression ratio of ~4
#       -set this value to 3.5 can generate a file with compression ratio of ~3
#       -set this value to 4 can generate a file with compression ratio of ~2
#       -set this value to 7.95 can generate a file with compression ratio of ~1
#     Arg ent2_4k.bin is the generated file
#
###############################################################################

import random, math, sys, os
import numpy as np
#from scipy.stats import entropy
import collections
from binascii import unhexlify
from pathlib import Path

def int2hexstr(i):
    """
       Get a byte array of an integer
    """
    try:
        i = i & 0xFFFFFFFF
    except:
        print('int2hexstr tag: ', i)
        sys.exit(1)
    bin_bytes = i.to_bytes(4, 'little')
    return bin_bytes.hex()

def int2binfile(i, binfname):
    """
       Get a byte array of an integer
    """
    try:
        i = i & 0xFFFFFFFF
    except:
        print('int2binfile tag: ', i)
        sys.exit(1)
    bin_bytes = i.to_bytes(4, 'little')

    with open(binfname, "wb") as binFile:
        binFile.write(bin_bytes)

def binfile2int(binfname):
    """
       Return an integer with a byte array
    """
    with open(binfname, "rb") as binFile:
        bin_bytes = binFile.read()
        if (len(bin_bytes) != 4):
            print('binfile2int file size is NOT 4 bytes')
            sys.exit(1)

    return int.from_bytes(bin_bytes, "little")

def str2hexstr(s):
    """
       Get a hex string of a regular string
    """
    l = []
    for c in s:
        hexc = hex(ord(c))[2:]
        if len(hexc) == 1:
            hexc = '0' + hexc
        l.append(hexc)
    hexstr = ''.join(l)
    return hexstr


def hexfile2hexstr(filename):
    """
       Get a hex string from a hex file, changeline symbol will be discarded
    """
    hexstr = ''
    with open(filename, 'r') as fp:
        Lines = fp.readlines()
        for l in Lines:
            hexstr += l
    return hexstr


def file2hexstr(filename):
    """
       Get a hex string from a regular file, changeline symbol will be discarded
    """
    hexstr = ''
    hexlist = []
    with open(filename, 'r') as fp:
        Lines = fp.readlines()
        for line in Lines:
            hexlist += [hex(ord(c))[2:] for c in line]

    for i in hexlist:
        hexstr += i
    return hexstr


def binfile2hexstr(binfile):
    """
       Get a hex string of a binary file
    """
    with open(binfile, 'rb') as fp:
        hexdata = fp.read().hex()
        return hexdata.upper()

def binfile2binstr(binfile):
    """
       Get a hex string of a binary file
    """
    with open(binfile, 'rb') as fp:
        binarray = fp.read()
        return binarray

def hex2binfile(hexstr, binfname):
    p = Path(os.path.dirname(binfname))
    p.mkdir(parents=True,exist_ok=True)

    with open(binfname, "wb") as binFile:
        binFile.write(hex2bin(hexstr))

def hex2bin(hexstr, be = 0, word_aligh = 0):
    if not hexstr:
        return bytearray()
    if len(hexstr) % 2:
        if be == 0:
            hexstr = hexstr[:-1] + '0' + hexstr[-1]
        # indicate big endian
        if be == 1:
            hexstr = '0' + hexstr
        if word_aligh:
            hexstr = '0' * ((len(hexstr)+7)//8 * 8 - len(hexstr)) + hexstr
    return unhexlify(hexstr)

def hex_xor(hexstr1, hexstr2):
    '''Assume two strings have the same length
    '''
    strbin1 = hex2bin(hexstr1)
    strbin2 = hex2bin(hexstr2)

    xor_str = b"".join([(a ^ b).to_bytes(1, 'big') for (a,b) in zip(strbin1, strbin2)])
    return xor_str.hex()

def leword(hexstr):
    ret = []
    int_count = (len(hexstr) + 7) //8
    padding = '0' * (int_count * 8 - len(hexstr))
    hexstr += padding
    for i in range(int_count):
        word = hexstr[i*8: (i+1)*8]
        leword = word[6:8] + word[4:6] + word[2:4] + word[0:2]
        ret.append(leword)
    retstr = ""
    for i in ret:
        retstr += i

    return retstr



def ledoubleword(hexstr):
    ret = []
    int_count = (len(hexstr) + 15) //16
    padding = '0' * (int_count * 16 - len(hexstr))
    hexstr += padding
    for i in range(int_count):
        dword = hexstr[i*16: (i+1)*16]
        ledword = dword[14:16] + dword[12:14] + dword[10:12] + dword[8:10] + dword[6:8] + dword[4:6] + dword[2:4] + dword[0:2]
        ret.append(ledword)
    retstr = ""
    for i in ret:
        retstr += i

    return retstr

def hexbitrvs(hexstr):
    bytelist = [hexstr[i:i+2] for i in range(0, len(hexstr), 2)]
    swtstr = ''
    for b in bytelist:
        val = int(b, 16)
        reverse = bin(val)[-1:1:-1]
        if len(reverse) < 8:
            reverse = reverse + (8 - len(reverse))*'0'
        valrev = int(reverse, 2)
        bhex = int2hexstr(valrev)[:2]
        swtstr += bhex
    return swtstr

# def gettime():
#    return datetime.now().strftime("%d-%b-%Y (%H:%M:%S)")


def checksum(fp):
    """
       Calculate the checksum of the test vector image body
    """

    Lines = fp.read().splitlines()
    cs = 0
    content = ''
    for line in Lines:
        content += line

    # pad a string with
    paddindzeros = '0' * (8 - len(content) % 8)
    line += paddindzeros
    line_segements = [line[i:i+8] for i in range(0, len(line), 8)]
    # print(line_segements)
    for seg in line_segements:
        #print('seg', seg)
        #print(unhexlify(seg))
        val, = struct.unpack('<L', unhexlify(seg))
        cs ^= val

    return cs % 0xFFFFFFFF


def checksummem(bytesbuffer):
    """
       Calculate the checksum of the test vector image body
    """
    ret = 0
    int_count = len(bytesbuffer)//8
    for i in range(int_count):
        #print(bytesbuffer[i*8: (i+1)*8])
        seg = bytesbuffer[i*8: (i+1)*8]
        val, = struct.unpack('<L', unhexlify(seg))
        ret ^= val
        #ret += int.from_bytes(bytesbuffer[i*4: (i+1)*4], "little")

    ret = int(int2hexstr(ret), 16)
    return ret % 0xFFFFFFFF


def word_align_hexstr(s):
    if len(s) % 8 == 0:
        return s
    return s + '0' * ((len(s)//8 + 1)*8 - len(s))



# create a large 100MB random int pool from a file
"""
random_pool = []
with open('random_pool.bin', 'rb') as fp:
    chars = fp.read()
    for c in chars:
        random_pool.append(c)
"""

# seems the random pool method is not good
# no bug was caught
def gen_rnd_bs_method2(n):
    begin_idx = random.randint(0, len(random_pool) - n -1)
    return random_pool[begin_idx: begin_idx+n]

def gen_rnd_bs(n):
    bs = []
    for i in range(n):
        bs.append(random.randint(0, 255))
    return bs

def gen_zero_bs(n):
    return [0]*n


def m_entropy_cal(s):
    """
    s is bytes in memory
    """
    probabilities = [n_x/len(s) for x,n_x in collections.Counter(s).items()]
    e_x = 0
    for p_x in probabilities:
        e_x += (-p_x*math.log(p_x,2))
    return round(e_x, 4)


def gen_ent_seq(n, e):
    """
    generate a n bytes sequence with entropy value of e
    n: int
    e: float
    """

    zero_too_many = []
    zero_too_few = []
    too_many = 0

    BASE_ELEMENTS = 3
    s_base = gen_rnd_bs(BASE_ELEMENTS)

    last_n0 = -1
    last_n1 = n+1
    n0 = n//2
    n1 = n - n0
    s0 = gen_rnd_bs(n0)
    # s1 is a random int duplicated multiple time
    if e > 2:
        s1 = [random.randint(0, 255)] * n1
    else:
        s1 = [0] * n1

    err = m_entropy_cal(s0+s1) - e

    err_bound = 0.01
    ent_range = [0, 7.99]

    if e < ent_range[0] or e > ent_range[1]:
        raise ValueError("e must be in the range of {}".format(ent_range))

    #max_try = 1000
    max_try = 200

    while abs(err) > err_bound and max_try > 0 and n0 <= n and n1 <= n:
        max_try -= 1
        #print(n0, n1, err)
        #print(zero_too_few, zero_too_many)

        if err > 0:
            # reduce random sequence and add 0s
            while n1 in zero_too_few:
                n1 -= 1
            zero_too_few.append(n1)
            
            # last try has too many zeros
            # this try has too few zeros
            if too_many == 1:
                try:
                    n1 = (n1 + zero_too_many[-1] ) // 2
                except:
                    n1 = n1 // 2
                n0 = n - n1
                too_many = 0
            # last try has too few zeros
            # this try also has too few zeros
            # reduce random by half
            else:
                n0 = n0 // 2
                n1 = n - n0

            s0 = s0[:n0]
            #s1 = s1 + [s_base[random.randint(0, BASE_ELEMENTS-1)]] * (n - n0 - len(s1))
            if e > 2:
                s1 = s1 + [random.randint(0, 255)] * (n - n0 - len(s1))
            else:
                s1 = s1 + [0] * (n - n0 - len(s1))


        else:
            # add random sequence and reduce 0s
            while n1 in zero_too_many:
                n1 += 1
            zero_too_many.append(n1)

            # last try has too many zeros
            # this try has too many zeros
            if too_many == 1:
                n1 = n1 // 2
                n0 = n - n1
            # last try has too few zeros
            # this try also has too many zeros
            # reduce random by half
            else:
                try:
                    n1 = (n1 + zero_too_few[-1] ) // 2
                except:
                    n1 = n1 // 2
                n0 = n - n1
                too_many = 1


            s1 = s1[:n1]
            s0 = s0 + gen_rnd_bs(n0 - len(s0))

        s = s0 + s1
        err = m_entropy_cal(s) - e

    if not (max_try > 0 and n0 <= n and n1 <= n):
        return None, None

    # chunk s with 6 bytes each group
    csz = random.randint(3, 8)
    try:
        s_chunks = [s[x: x+csz] for x in range(0, len(s), csz)]
    except:
        return None, None

    # shuffle a list
    random.shuffle(s_chunks)

    s = []
    for c in s_chunks:
        s += c

    assert(len(s) == n)

    return m_entropy_cal(s), s

def generator(n, e):
    h = ''
    e, s = gen_ent_seq(n, e)

    if s:
        for i in s:
            t = hex(i)[2:]
            if len(t) != 2:
                t = '0' * (2-len(t)) + t
            h = h + t
        return e, len(s), h

    return None

if __name__=="__main__":
    h = ''
    e, s = gen_ent_seq(int(sys.argv[1]), float(sys.argv[2]))

    if s:
        for i in s:
            t = hex(i)[2:]
            if len(t) != 2:
                t = '0' * (2-len(t)) + t
            h = h + t
        print(e, len(s), h)
        hex2binfile(h, sys.argv[3])
    else:
        print('Data generation failed, supported MAX entropy parameter is 7.96')
