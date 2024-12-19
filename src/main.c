#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"

// AES-128
#define KEY_SIZE 16 // bytes
#define BLOCK_SIZE 16 // bytes

// void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);
void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);
void BMP_decrypt(const char *fileName, unsigned char *key, int keySize);

int main(int argc, char *argv[])
{
    const char *inputFileName = argv[1];
    const char *fileExtension = strrchr(inputFileName, '.');

    // Rijndael's key expansion 
    // Expands an 128 bytes key into an 176 bytes key
    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));

    // Check if we want to encrypt or decrypt
    if (strcmp(fileExtension, ".bmp") == 0) {
        if (argc > 2 && strcmp(argv[2], "decrypt") == 0) {
            BMP_decrypt(inputFileName, key, KEY_SIZE);
        } else {
            BMP_encrypt(inputFileName, key, KEY_SIZE);
        }
    } else {
        printf("Unsupported file format: %s\n", fileExtension);
        return 1;
    }

    return 0;
}

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize) {
    FILE *file = fopen(fileName, "rb");

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *data = calloc(fileSize, sizeof(unsigned char));
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);
    
    clock_t start = clock();

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    unsigned char *encryptedData = calloc(paddedSize, sizeof(unsigned char));

    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(data + i * BLOCK_SIZE, encryptedData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
}

void BMP_decrypt(const char *fileName, unsigned char *key, int keySize) {
    printf("Processing BMP file for decryption\n");
    FILE *file = fopen(fileName, "rb");

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *encryptedData = calloc(fileSize, sizeof(unsigned char));
    fread(encryptedData, sizeof(unsigned char), fileSize, file);
    fclose(file);
    
    printf("AES DECRYPTION START!\n");
    clock_t start = clock();

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    unsigned char *decryptedData = calloc(fileSize, sizeof(unsigned char));

    for (size_t i = 0; i < numBlocks; i++) {
        aes_decrypt(encryptedData + i * BLOCK_SIZE, decryptedData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("decrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(decryptedData, sizeof(unsigned char), fileSize, outputFile);
    fclose(outputFile);

    free(encryptedData);
    free(decryptedData);
    printf("BMP decryption complete\n");
}
