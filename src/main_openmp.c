#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"
#include <omp.h>
#include <stdalign.h>

#define KEY_SIZE 16
#define BLOCK_SIZE 16
#define ALIGNMENT 64

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize, int numThreads);

int main(int argc, char *argv[])
{
    const char *inputFileName = argv[1];
    int numThreads = atoi(argv[2]);

    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[KEY_SIZE] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));
    BMP_encrypt(inputFileName, key, KEY_SIZE, numThreads);

    return 0;
}

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize, int numThreads) {
    printf("Processing BMP file with %d threads\n", numThreads);
    FILE *file = fopen(fileName, "rb");

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

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
    fclose(file);
    
    unsigned char *encryptedData = aligned_alloc(64, paddedSize);
    
    printf("AES ENCRYPTION START!\n");
    clock_t start = clock();

    #pragma omp parallel for num_threads(numThreads)
    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(data + i * ALIGNMENT, encryptedData + i * BLOCK_SIZE, key, keySize);
    }

    clock_t end = clock();
    printf("AES ENCRYPTION END!\n");
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
}