#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdalign.h>
#include <time.h>

#define KEY_SIZE 16 // bytes
#define BLOCK_SIZE 16 // bytes

int main(int argc, char *argv[]) {
    unsigned char key[KEY_SIZE] = {'I', 't', 'i', 's', 'a', 's', 'e', 'c', 'r', 'e', 't', 'e', 'k', 'e', 'y', '.'};
    const char *filename = argv[1];

    FILE *file = fopen(filename, "rb");

    // BMP Header is 54 bytes
    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, file);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file) - 54;
    fseek(file, 54, SEEK_SET);

    size_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t paddedSize = numBlocks * BLOCK_SIZE;

    unsigned char *data = aligned_alloc(64, fileSize);
    fread(data, sizeof(unsigned char), fileSize, file);
    fclose(file);
    printf("AES ENCRYPTION START!\n");

    // 使用 EVP API 初始化加密結構
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        printf("Error initializing EVP context\n");
        return -1;
    }

    // 設置加密密鑰
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) != 1) {
        printf("Error initializing encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    clock_t start = clock();
    unsigned char *encryptedData = aligned_alloc(64, paddedSize);

    // 執行 AES 加密
    #pragma omp parallel for num_threads(4) schedule (guided)
    for (size_t i = 0; i < numBlocks; i++) {
        int len;
        EVP_EncryptUpdate(ctx, encryptedData + i * BLOCK_SIZE, &len, data + i * BLOCK_SIZE, BLOCK_SIZE);
    }

    int len;
    // 完成加密
    EVP_EncryptFinal_ex(ctx, encryptedData + (numBlocks - 1) * BLOCK_SIZE, &len);
    clock_t end = clock();
    printf("Time %f ms\n", ((float)end - (float)start) / CLOCKS_PER_SEC * 1000);

    // 寫入加密結果
    FILE *outputFile = fopen("encrypted_image.bmp", "wb");
    fwrite(header, sizeof(unsigned char), 54, outputFile);
    fwrite(encryptedData, sizeof(unsigned char), paddedSize, outputFile);
    fclose(outputFile);

    EVP_CIPHER_CTX_free(ctx);  // 清理加密上下文
    free(data);
    free(encryptedData);
    printf("BMP encryption complete\n");

    return 0;
}
