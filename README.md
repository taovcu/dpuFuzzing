# dpuFuzzing

dpuFuzzing is a tool to synthesize data with various patterns for data process unit (DPU) fuzzing. Data patterns are quantified using the entropy metric, which indicates data randomness.
The initial target application of dpuFuzzing is a compressor, which needs to be validated with various data patterns for functional correctness, compression ratio, and speed. Actually, dpuFuzzing can be used to validate any other data processing modules, including crypto module, regex, etc.


# Execution

## Generate a 4KB file with entropy value of 2.0

python3 gen\_tv.py -l 4096 -e 2 ent2\_4k.bin


## Generate a 4KB file with compresion ratio of 3.0

python3 gen\_tv.py -l 4096 -r 3 cr3\_4k.bin

