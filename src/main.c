#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"

// The key and block size of AES-128 is 16 bytes
#define KEY_SIZE 16
#define BLOCK_SIZE 16

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *inputFileName = argv[1];
    const char *fileExtension = strrchr(inputFileName, '.');

    // Expands key into 192 bytes
    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));

    // AES Encryption with EBC
    if (strcmp(fileExtension, ".bmp") == 0) {
        BMP_encrypt(inputFileName, key, KEY_SIZE);
    } else {
        printf("Unsupported file format %s\n", fileExtension);
        return 1;
    }

    return 0;
}

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize) 
{
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        printf("Error opening file\n");
    }

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *data = calloc(fileSize, sizeof(unsigned char));
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    unsigned char *encryptedData = calloc(paddedSize, sizeof(unsigned char));

    clock_t start = clock();
    
    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(data + i * BLOCK_SIZE, encryptedData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start)/CLOCKS_PER_SEC *1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
}