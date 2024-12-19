#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "AES.h"

#define KEY_SIZE 16
#define BLOCK_SIZE 16

typedef struct {
    unsigned char *data;
    unsigned char *encryptedData;
    unsigned char *key;
    int keySize;
    size_t start;
    size_t end;
} ThreadData;

void *encrypt_block(void *arg);
void BMP_encrypt(const char *fileName, unsigned char *key, int keySize, int numThreads);

int main(int argc, char *argv[])
{
    const char *inputFileName = argv[1];
    int numThreads = atoi(argv[2]);

    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

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

    unsigned char *data = aligned_alloc(64, fileSize);
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    unsigned char *encryptedData = aligned_alloc(64, paddedSize);

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    clock_t start = clock();

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
