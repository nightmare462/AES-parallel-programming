# Parallalize AES encryption
This is the final project for the Parallel Programming course at NYCU.

The project implements AES-128 encryption on a BMP image using ECB mode.
## Reference
https://github.com/m3y54m/aes-in-c/tree/main
## Compile and run
Choose the methods you want to implement.

<option> can be 'main_text' or 'main_pthreads' and so on.
```
cp <option> main.c 
```
Build:
```
make
```
Run:
```
./build/aes ./test/img.bmp
```
## Evaluation
目前做出來 pthreads 並沒有辦法做到平行化，甚至比沒有平行化的時間還多一點