import os
import sys

build_folder = sys.argv[1]
binary_name = sys.argv[2]

# -DCMAKE_MAKE_PROGRAM=ninja -G Ninja
os.system(f"cmake -DCMAKE_BUILD_TYPE=Debug -S ./src -B {build_folder}")
os.system(f"cmake --build {build_folder} --target {binary_name}")