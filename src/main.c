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
        printf("Usage: %s <input BMP file>\n", argv[0]);
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
        perror("Cannot open file.");
        return 1;
    }

    // Read BMP header
    unsigned char bmpHeader[54];
    fread(bmpHeader, sizeof(unsigned char), 54, inputFile);

    // Get image size from BMP header
    int width = *(int*)&bmpHeader[18];
    int height = *(int*)&bmpHeader[22];
    int imageSize = width * height * 3;

    // Allocate memory for image data
    unsigned char *inputData = calloc(imageSize, sizeof(unsigned char));
    unsigned char *outputData = calloc(imageSize, sizeof(unsigned char));
    unsigned char *decryptedData = calloc(imageSize, sizeof(unsigned char));

    if (!inputData || !outputData || !decryptedData) {
        perror("Memory allocation failed.");
        fclose(inputFile);
        return 1;
    }

    fread(inputData, sizeof(unsigned char), imageSize, inputFile);
    fclose(inputFile);

    // Key size: 128/192/256
    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));

    printf("AES encryption start.\n");
    clock_t start = clock();

    // AES Encryption
    for (size_t i = 0; i < imageSize; i += BLOCK_SIZE) {
        aes_encrypt(inputData + i, outputData + i, key, KEY_SIZE);
    }

    clock_t end = clock();
    printf("Time: %ld seconds\n", (end - start) / CLOCKS_PER_SEC);

    FILE *encryptedFile = fopen("encrypted_image.bmp", "wb");
    fwrite(bmpHeader, sizeof(unsigned char), 54, encryptedFile);
    fwrite(outputData, sizeof(unsigned char), imageSize, encryptedFile);
    fclose(encryptedFile);
    printf("Encrypted file saved as 'encrypted_image.bmp'\n");

    free(inputData);
    free(outputData);
    free(decryptedData);

    return 0;
}