#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "AES.h"

// AES-128
#define KEY_SIZE 16 // bytes
#define BLOCK_SIZE 16 // bytes

void BMP__encrypt(const char *fileName, const unsigned char *key, int keySize);
void JPEG__encrypt(const char *fileName, const unsigned char *key, int keySize);
void PNG__encrypt(const char *fileName, const unsigned char *key, int keySize);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <Please input a file>\n", argv[0]);
        return 1;
    }

    const char *inputFileName = argv[1];
    const char *fileExtension = strrchr(inputFileName, '.');

    // Rijndael's key expansion 
    // Expands an 128 bytes key into an 176 bytes key
    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));

    printf("<=== AES ENCRYPTION START ===>\n");
    clock_t start = clock();

    // AES Encryption with EBC
    if (strcmp(fileExtension, ".bmp") == 0) {
        BMP__encrypt(inputFileName, key, KEY_SIZE);
    } else if (strcmp(fileExtension, ".png") == 0) {
        // PNG__encrypt(inputFileName, key, KEY_SIZE);
    } else {
        printf("Unsupported file format: %s\n", fileExtension);
        return 1;
    }

    clock_t end = clock();
    printf("<=== %ld clocks ===>\n", (end - start));

    return 0;
}

void BMP__encrypt(const char *fileName, const unsigned char *key, int keySize) {
    printf("Processing BMP file: %s\n", fileName);

    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Cannot open BMP file");
        return;
    }

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    if (header[0] != 'B' || header[1] != 'M') {
        printf("Error: Invalid BMP file\n");
        fclose(file);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    unsigned char *data = calloc(fileSize, sizeof(unsigned char));
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;
    unsigned char *encryptedData = calloc(paddedSize, sizeof(unsigned char));

    for (size_t i = 0; i < numBlocks; i++) {
        aes_encrypt(data + i * BLOCK_SIZE, encryptedData + i * BLOCK_SIZE, key, keySize);
    }

    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    free(data);
    free(encryptedData);
    printf("BMP encryption complete\n");
}
