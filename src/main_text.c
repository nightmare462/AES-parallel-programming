#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"

#define KEY_SIZE 16
#define BLOCK_SIZE 16

void printHex(const unsigned char *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%2.2x%c", data[i], ((i + 1) % 16) ? ' ' : '\n');
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Input file: %s.\n", argv[0]);
        return 1;
    }

    char *inputFileName = argv[1];
    char *fileExtension = strrchr(inputFileName, '.');

    FILE *inputFile = fopen(inputFileName, "rb");
    if (!inputFile) {
        perror("Cannot open file.");
        return 1;
    }

    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    int keySize = KEY_SIZE;
    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;

    unsigned char *inputData = calloc(paddedSize, sizeof(unsigned char));
    unsigned char *outputData = calloc(paddedSize, sizeof(unsigned char));
    unsigned char *decryptedData = calloc(paddedSize, sizeof(unsigned char));

    if (!inputData || !outputData || !decryptedData) {
        perror("Memory allocation is fail.");
        fclose(inputFile);
        return 1;
    }

    fread(inputData, sizeof(unsigned char), fileSize, inputFile);
    fclose(inputFile);

    // Key size: 128/ 192/ 256
    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, keySize, sizeof(expandedKey));

    printf("AES encrypt start.");
    clock_t Start = clock();

    // AES Encryption
    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(inputData + i * BLOCK_SIZE, outputData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t End = clock();
    printf("\nTime: %ld", (Start-End));

    FILE *encryptedFile = fopen("output", "wb");
    fwrite(outputData, sizeof(unsigned char), paddedSize, encryptedFile);
    fclose(encryptedFile);
    printf("\nFile saved");

    printf("\nEncryption text:\n");
    printHex(outputData, paddedSize);

    // AES Decryption
    for (size_t i = 0; i < numBlocks; i++) {
        aes_decrypt(outputData + i * BLOCK_SIZE, decryptedData + i * BLOCK_SIZE, key, keySize);
    }

    printf("Decrypted text:\n");
    printHex(decryptedData, paddedSize);

    free(inputData);
    free(outputData);
    free(decryptedData);

    return 0;
}