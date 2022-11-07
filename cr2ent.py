import sys
import zstd
import entSeqEngine
import json, os

cr = round(float(sys.argv[1]), 1)
ent_list = [x*0.1 for x in range(1, 80)]
json_file = "cr2ent.json"

cr_ent_dict = {}

if os.path.exists(json_file):
    with open(json_file) as jf:
        cr_ent_dict = json.load(jf)
else:
    for e in ent_list:
        a, b = entSeqEngine.gen_ent_seq(4096, e)
        retry = 20
        while not b and retry:
            a, b = entSeqEngine.gen_ent_seq(4096, e)
            retry -= 1
    
        if b:
            h = ''
            for i in b:
                t = hex(i)[2:]
                if len(t) != 2:
                    t = '0' * (2-len(t)) + t
                h = h + t
            if h:
                byte_content = entSeqEngine.hex2bin(h)
                ret = zstd.compress(byte_content, 1, 1)
                cr_ent_dict[str(round(4096/len(ret), 1))] = round(e,1)

    with open(json_file, "w") as outfile:
        json.dump(cr_ent_dict, outfile)

#print(cr)
#print(cr_ent_dict)
while str(round(cr,1)) not in cr_ent_dict and cr <= 40:
    cr += 0.1

try:
    print(cr_ent_dict[str(round(cr,1))])
except:
    print("Compression ratio {} is too high to be achieved!".format(round(cr,1)))
