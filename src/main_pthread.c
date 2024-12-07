#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "AES.h"

// AES-128
#define KEY_SIZE 16 // bytes
#define BLOCK_SIZE 16 // bytes
#define numThreads 4

typedef struct {
    unsigned char *data;
    unsigned char *encryptedData;
    unsigned char *key;
    int keySize;
    size_t start;
    size_t end;
    char padding[64];
} ThreadData;

void *encrypt_block(void *arg);
void BMP_encrypt_pthread(const char *fileName, unsigned char *key, int keySize);

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

    // AES Encryption with EBC
    if (strcmp(fileExtension, ".bmp") == 0) {
        BMP_encrypt_pthread(inputFileName, key, KEY_SIZE);
    } else {
        printf("Unsupported file format: %s\n", fileExtension);
        return 1;
    }

    return 0;
}

void BMP_encrypt_pthread(const char *fileName, unsigned char *key, int keySize) {
    printf("Processing BMP file with %d threads\n", numThreads);
    FILE *file = fopen(fileName, "rb");

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *data = calloc(fileSize, sizeof(unsigned char));
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);

    printf("AES ENCRYPTION START!\n");
    clock_t start = clock();

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    unsigned char *encryptedData = calloc(paddedSize, sizeof(unsigned char));

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    size_t blocksPerThread = numBlocks / numThreads;
    for (int i = 0; i < numThreads; i++) {
        threadData[i].data = data;
        threadData[i].encryptedData = encryptedData;
        threadData[i].key = key;
        threadData[i].keySize = keySize;
        threadData[i].start = i * blocksPerThread;
        threadData[i].end = (i == numThreads - 1) ? numBlocks : (i + 1) * blocksPerThread;
        pthread_create(&threads[i], NULL, encrypt_block, &threadData[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start)/CLOCKS_PER_SEC *1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
    printf("BMP encryption complete\n");
}

void *encrypt_block(void *arg) {
    ThreadData *threadData = (ThreadData *)arg;
    for (size_t i = threadData->start; i < threadData->end; i++) {
        aes_encrypt(threadData->data + i * BLOCK_SIZE,
                    threadData->encryptedData + i * BLOCK_SIZE, 
                    threadData->key, threadData->keySize);
    }
    return NULL;
}
