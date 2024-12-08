# Parallalize AES encryption
This is the final project for the Parallel Programming course at NYCU.

The project implements AES-128 encryption on a BMP image using ECB mode.
## Reference
https://github.com/m3y54m/aes-in-c/tree/main
## Compile and run
Choose the methods you want to implement.

`main_text` implement AES on text, and the others implement AES on a BMP image in different parallel methods.

Build:
```
make
```

Run: 
```
./build/aes <test_img>
```
```
./build/aes_openmp <test_img>
```
```
./build/aes_pthread <test_img> <thread_num>
```

## Evaluation

## Future work
1. Parallize AES implementation with SIMD.
2. Brutual decryption with parallel programming.
