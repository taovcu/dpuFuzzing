import sys
import argparse
import entSeqEngine
import cr2ent
import random

sample_ent_adjust = {0.05:1.143, 0.1:1.07}

def generate():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--length", type=int,
                        help="size in bytes of a file to be generated")
    parser.add_argument("-e", "--entropy", type=float,
                        help="entropy value should be a float in range [0, 7.96]")
    parser.add_argument("-s", "--sample_ratio", type=float,
                        help="sampling ratio should be a float in range (0, 1]")
    parser.add_argument("-n", "--number", type=int,
                        help="number of execution times")
    args = parser.parse_args()

    if not args.entropy:
        print("Exit: entropy parameter is missing")
        sys.exit(1)

    if args.sample_ratio <= 0  or args.sample_ratio > 1:
        print("Exit: sampling ratio parameter is out of range (0, 1]")
        sys.exit(1)

    for _ in range(args.number):
        h = ''
        e, s = entSeqEngine.gen_ent_seq(args.length, args.entropy)

        if s:
            orig_ent = entSeqEngine.m_entropy_cal(s)
            sample_size = int(args.length * args.sample_ratio)
            #print('sample size', sample_size)
            sample_bytes = random.choices(s, k=sample_size)
            sample_ent = entSeqEngine.m_entropy_cal(sample_bytes) * sample_ent_adjust[args.sample_ratio]
            print("original entropy, {}, sample entropy, {}".format(orig_ent, sample_ent) )
        else:
            print('Data generation failed, supported MAX entropy parameter is 7.96')

if __name__ == '__main__':
    generate()
