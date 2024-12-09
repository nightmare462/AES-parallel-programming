#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wmmintrin.h>
#include <time.h>
#include <pthread.h>

#define KEY_SIZE 16
#define BLOCK_SIZE 16
#define numThreads 4

typedef struct {
    unsigned char *data;
    unsigned char *encryptedData;
    unsigned char *key;
    size_t start;
    size_t end;
} ThreadData;

void expandKey_AESNI(unsigned char *expandedKey, unsigned char *key) {
    __m128i keySchedule = _mm_load_si128((__m128i *)key);
    _mm_store_si128((__m128i *)expandedKey, keySchedule);

    for (int i = 1; i <= 10; i++) {
        keySchedule = _mm_aeskeygenassist_si128(keySchedule, i);
        _mm_store_si128((__m128i *)(expandedKey + i * BLOCK_SIZE), keySchedule);
    }
}

void aes_encrypt_AESNI(unsigned char *input, unsigned char *output, unsigned char *expandedKey) {
    __m128i block = _mm_load_si128((__m128i *)input);
    block = _mm_xor_si128(block, _mm_load_si128((__m128i *)expandedKey));

    for (int i = 1; i < 10; i++) {
        block = _mm_aesenc_si128(block, _mm_load_si128((__m128i *)(expandedKey + i * BLOCK_SIZE)));
    }

    block = _mm_aesenclast_si128(block, _mm_load_si128((__m128i *)(expandedKey + 10 * BLOCK_SIZE)));
    _mm_store_si128((__m128i *)output, block);
}

void aes_decrypt_AESNI(unsigned char *input, unsigned char *output, unsigned char *expandedKey) {
    __m128i block = _mm_load_si128((__m128i *)input);
    block = _mm_xor_si128(block, _mm_load_si128((__m128i *)(expandedKey + 10 * BLOCK_SIZE)));

    for (int i = 9; i > 0; i--) {
        block = _mm_aesdec_si128(block, _mm_load_si128((__m128i *)(expandedKey + i * BLOCK_SIZE)));
    }

    block = _mm_aesdeclast_si128(block, _mm_load_si128((__m128i *)expandedKey));
    _mm_store_si128((__m128i *)output, block);
}

void *encrypt_block(void *arg) {
    ThreadData *threadData = (ThreadData *)arg;
    for (size_t i = threadData->start; i < threadData->end; i++) {
        aes_encrypt_AESNI(threadData->data + i * BLOCK_SIZE,
                    threadData->encryptedData + i * BLOCK_SIZE, 
                    threadData->key);
    }
    return NULL;
}

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize) {
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Error opening file");
        return;
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *data = malloc(fileSize);
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);

    unsigned char expandedKey[176];
    expandKey_AESNI(expandedKey, key);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    unsigned char *encryptedData = calloc(numBlocks * BLOCK_SIZE, sizeof(unsigned char));

    //printf("%d\n", numBlocks);

    clock_t start = clock();

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    size_t blocksPerThread = numBlocks / numThreads;
    for (int i = 0; i < numThreads; i++) {
        threadData[i].data = data;
        threadData[i].encryptedData = encryptedData;
        threadData[i].key = expandedKey;
        threadData[i].start = i * blocksPerThread;
        threadData[i].end = (i == numThreads - 1) ? numBlocks : (i + 1) * blocksPerThread;
        pthread_create(&threads[i], NULL, encrypt_block, &threadData[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), numBlocks * BLOCK_SIZE, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <BMP file>\n", argv[0]);
        return 1;
    }

    const char *fileName = argv[1];
    unsigned char key[KEY_SIZE] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    BMP_encrypt(fileName, key, KEY_SIZE);
    return 0;
}
