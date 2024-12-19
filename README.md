# Parallalize AES encryption
This is the final project for the Parallel Programming course at NYCU.

The project implements AES-128 encryption on a BMP image using ECB mode.

## Compile and run
### Software
Build all: `make`

Run: 

`./build/aes <test_img>`

`./build/aes_openmp <test_img> <numThreads>`

`./build/aes_pthread <test_img> <numThreads>`

`./build/aes-ni <test_img>`

`./build/aes-ni-pthread <test_img>`

### Hardware
Import hls folder as "hls components" into Vitis IDE 2024.1.

## Performance testing
You can use `cnt.py` to measure the performance of different executables by running each of them 500 times. 

### Usage
Run the following command:

`python3 src/cnt.py ./build/<executable> <test_img> <numThreads>`

## Reference
https://github.com/m3y54m/aes-in-c/tree/main
