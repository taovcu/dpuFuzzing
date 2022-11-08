import sys
import argparse
import entSeqEngine
import cr2ent

def generate():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--length", type=int,
                        help="size in bytes of a file to be generated")
    parser.add_argument("-e", "--entropy", type=float,
                        help="entropy value should be a float in range [0, 7.96]")
    parser.add_argument("-r", "--ratio", type=float,
                        help="target compress ratio of the data, should be a float in range [0, 35]")
    parser.add_argument("output_file_path", type=str,
                        help="the output file path must be specified")
    args = parser.parse_args()
    out_bin = args.output_file_path

    if args.ratio:
        args.entropy = cr2ent.cr2ent_fun(round(args.ratio, 1))
        if not args.entropy:
            print("Compression ratio failed to be converted to entropy")
            sys.exit(1)

    if not args.entropy:
        print("Exit: entropy parameter is missing")
        sys.exit(1)

    h = ''
    e, s = entSeqEngine.gen_ent_seq(args.length, args.entropy)

    if s:
        for i in s:
            t = hex(i)[2:]
            if len(t) != 2:
                t = '0' * (2-len(t)) + t
            h = h + t
        entSeqEngine.hex2binfile(h, out_bin)
    else:
        print('Data generation failed, supported MAX entropy parameter is 7.96')

# python3 ssd_compress.py gzip file_name 
if __name__ == '__main__':
    generate()
