#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wmmintrin.h>
#include <time.h>

#define KEY_SIZE 16
#define BLOCK_SIZE 16

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

    #pragma omp parallel for num_threads(4) schedule (guided)
    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt_AESNI(data + i * BLOCK_SIZE, encryptedData + i * BLOCK_SIZE, expandedKey);
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

void BMP_decrypt(const char *fileName, unsigned char *key, int keySize) {
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
    unsigned char *decryptedData = calloc(numBlocks * BLOCK_SIZE, sizeof(unsigned char));

    //printf("%d\n", numBlocks);

    clock_t start = clock();

    #pragma omp parallel for num_threads(4) schedule (guided)
    for (size_t i = 0; i < numBlocks; i++) {
        aes_decrypt_AESNI(data + i * BLOCK_SIZE, decryptedData + i * BLOCK_SIZE, expandedKey);
    }

    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    FILE *outputFile = fopen("decrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(decryptedData, sizeof(unsigned char), numBlocks * BLOCK_SIZE, outputFile);
    fclose(outputFile);

    free(data);
    free(decryptedData);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <BMP file>\n", argv[0]);
        return 1;
    }

    const char *fileName = argv[1];
    const char *fileExtension = strrchr(fileName, '.');
    unsigned char key[KEY_SIZE] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    if (strcmp(fileExtension, ".bmp") == 0) {
        if (argc > 2 && strcmp(argv[2], "decrypt") == 0) {
            BMP_decrypt(fileName, key, KEY_SIZE);
        } else {
            BMP_encrypt(fileName, key, KEY_SIZE);
        }
    } else {
        printf("Unsupported file format: %s\n", fileExtension);
        return 1;
    }

    return 0;
}
