#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>
#include "AES.h"

#define KEY_SIZE 16
#define BLOCK_SIZE 16

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize);

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    const char *inputFileName = argv[1];
    const char *fileExtension = strrchr(inputFileName, '.');

    const int expandedKeySize = 176;
    unsigned char expandedKey[expandedKeySize];
    unsigned char key[16] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};

    expandKey(expandedKey, key, KEY_SIZE, sizeof(expandedKey));
    BMP_encrypt(inputFileName, key, KEY_SIZE);

    MPI_Finalize();
    return 0;
}

void BMP_encrypt(const char *fileName, unsigned char *key, int keySize) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    FILE *file = NULL;
    unsigned char header[54];
    unsigned char *data = NULL;
    long fileSize = 0;

    if (rank == 0) {
        printf("Processing BMP file\n");
        file = fopen(fileName, "rb");
        fread(header, sizeof(unsigned char), 54, file);

        fseek(file, 0, SEEK_END);
        fileSize = ftell(file) - 54;
        fseek(file, 54, SEEK_SET);

        data = (unsigned char*)calloc(fileSize, sizeof(unsigned char));
        fread(data, sizeof(unsigned char), fileSize, file);
        fclose(file);

        printf("AES ENCRYPTION START!\n");
    }

    double start = MPI_Wtime();

    MPI_Bcast(&fileSize, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        data = (unsigned char*)calloc(fileSize, sizeof(unsigned char));
    }

    MPI_Bcast(data, fileSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(header, 54, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t localNumBlocks = numBlocks / size;
    size_t remainingBlocks = numBlocks % size;

    if (rank < remainingBlocks) {
        localNumBlocks++;
    }

    size_t localSize = localNumBlocks * BLOCK_SIZE;
    unsigned char *localData = (unsigned char*)calloc(localSize, sizeof(unsigned char));
    unsigned char *localEncryptedData = (unsigned char*)calloc(localSize, sizeof(unsigned char));

    for (size_t i = 0; i < localNumBlocks; i++) {
        size_t blockIndex = rank + i * size;
        if (blockIndex * BLOCK_SIZE < fileSize) {
            size_t offset = blockIndex * BLOCK_SIZE;
            aes_encrypt(data + offset, localEncryptedData + i * BLOCK_SIZE, key, keySize);
        }
    }

    unsigned char *encryptedData = NULL;
    if (rank == 0) {
        encryptedData = (unsigned char*)calloc(numBlocks * BLOCK_SIZE, sizeof(unsigned char));
    }

    MPI_Gather(localEncryptedData, localSize, MPI_UNSIGNED_CHAR, encryptedData, localSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    double end = MPI_Wtime();
    if (rank == 0) {
        printf("AES ENCRYPTION END!\n");
        printf("Time %.2f ms\n", (end - start) * 1000.0);
        FILE *outputFile = fopen("encrypted_image.bmp", "wb");
        fwrite(header, sizeof(unsigned char), 54, outputFile);
        fwrite(encryptedData, sizeof(unsigned char), numBlocks * BLOCK_SIZE, outputFile);
        fclose(outputFile);
        free(encryptedData);
    }

    free(data);
    free(localData);
    free(localEncryptedData);
}