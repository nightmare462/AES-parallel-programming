#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"
#include <omp.h>
#include <stdalign.h>

// AES-128
#define KEY_SIZE 32 // bytes
#define BLOCK_SIZE 32 // bytes
#define ALIGNMENT 64

// void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);
void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);
void BMP_decrypt(const char *fileName, unsigned char *key, int keySize);

int main(int argc, char *argv[])
{
    const char *inputFileName = argv[1];
    const char *fileExtension = strrchr(inputFileName, '.');

    // Rijndael's key expansion 
    // Expands an 128 bytes key into an 176 bytes key
    const int expandedKeySize = 240;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[KEY_SIZE] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.', 'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

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
    printf("Processing BMP file for encryption\n");
    FILE *file = fopen(fileName, "rb");

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);
;
    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    
    
    unsigned char *data = aligned_alloc(64, numBlocks * 64);
    unsigned char *temp = aligned_alloc(64, fileSize);
    fread(temp, sizeof(unsigned char), fileSize, file);

    for (size_t i = 0; i < numBlocks; i++) {
        memcpy(data + i * ALIGNMENT, temp + i * BLOCK_SIZE, BLOCK_SIZE);
        memset(data + i * ALIGNMENT + BLOCK_SIZE, 0, ALIGNMENT - BLOCK_SIZE);
    }
    free(temp);
    //unsigned char *data = aligned_alloc(64, fileSize);
    //fread(data, sizeof(unsigned char), fileSize, file);

    fclose(file);
    
    printf("AES ENCRYPTION START!\n");
    clock_t start = clock();

    //unsigned char *encryptedData = calloc(paddedSize, sizeof(unsigned char));
    unsigned char *encryptedData = aligned_alloc(64, paddedSize);

    //printf("Number of threads: %d\n", omp_get_max_threads());
    printf("%d\n", numBlocks);

    #pragma omp parallel for num_threads(4)
    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(data + i * ALIGNMENT, encryptedData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
    printf("BMP encryption complete\n");
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

    #pragma omp parallel for num_threads(4) schedule (guided)
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
