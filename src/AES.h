#ifndef AES_H
#define AES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char getSBoxValue(unsigned char num);
unsigned char getSBoxInvert(unsigned char num);

void rotate(unsigned char *word);
unsigned char getRconValue(unsigned char num);
void core(unsigned char *word, int iteration);
void expandKey(unsigned char *expandedKey, unsigned char *key, int size, size_t expandedKeySize);

// AES Encryption
void subBytes(unsigned char *state);
void shiftRows(unsigned char *state);
void shiftRow(unsigned char *state, unsigned char nbr);
void addRoundKey(unsigned char *state, unsigned char *roundKey);
unsigned char galois_multiplication(unsigned char a, unsigned char b);
void mixColumns(unsigned char *state);
void mixColumn(unsigned char *column);

void aes_round(unsigned char *state, unsigned char *roundKey);
void createRoundKey(unsigned char *expandedKey, unsigned char *roundKey);
void aes_main(unsigned char *state, unsigned char *expandedKey, int nbrRounds);
char aes_encrypt(unsigned char *input, unsigned char *output, unsigned char *key, int size);

// AES Decryption
void invSubBytes(unsigned char *state);
void invShiftRows(unsigned char *state);
void invShiftRow(unsigned char *state, unsigned char nbr);
void invMixColumns(unsigned char *state);
void invMixColumn(unsigned char *column);

void aes_invRound(unsigned char *state, unsigned char *roundKey);
void aes_invMain(unsigned char *state, unsigned char *expandedKey, int nbrRounds);
char aes_decrypt(unsigned char *input, unsigned char *output, unsigned char *key, int size);

#endif // AES_H