# dpuFuzzing

dpuFuzzing is a tool to synthesize data with various patterns for data process unit (DPU) fuzzing. Data patterns are quantified using the entropy metric, which indicates data randomness.
The initial target application of dpuFuzzing is a compressor, which needs to be validated with various data patterns for functional correctness, compression ratio, and speed. Actually, dpuFuzzing can be used to validate any other data processing modules, including crypto module, regex, etc.

