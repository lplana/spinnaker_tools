# run test
ybug $1 < run_core_to_core_variation.ybug

# dump iobuf
ybug $1 < dump_iobuf.ybug

python pp._core_to_core_variability.py > output.log

