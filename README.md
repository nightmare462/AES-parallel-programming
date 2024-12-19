# Parallalize AES encryption
This is the final project for the Parallel Programming course at NYCU.

The project implements AES-128 encryption on a BMP image using ECB mode.
## Reference
https://github.com/m3y54m/aes-in-c/tree/main
## Compile and run
Build all: `make`
Build pthread: `make aes_pthread`
Build openMP: `make aes_openMP`

Run: 
`./build/aes <test_img>`
`./build/aes_openmp <test_img> <thread_num>`
`./build/aes_pthread <test_img> <thread_num>`
