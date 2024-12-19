#include "main.hpp"
#include <cstdint>
#include <hls_stream.h>
#include <stdio.h>

#define TIMES 32

uint8_t key[16] = {'k', 'k', 'k', 'k', 'e', 'e', 'e', 'e',
                   'y', 'y', 'y', 'y', '.', '.', '.', '.'};

uint8_t plaintext[] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x31, 0x32,
                       0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30};

uint8_t ciphertext[] = {0x39, 0x62, 0x8b, 0xcc, 0xc1, 0xcd, 0x48, 0xe4,
                        0x5f, 0xdd, 0xb5, 0xe8, 0x9c, 0xbf, 0x9d, 0x02};

uint8_t result[4096];

int main() {
  hls::stream<ap_uint8_t, 1024> as_input("tb_input");
  hls::stream<ap_uint8_t, 1024> as_key("tb_key");
  hls::stream<ap_uint8_t, 1024> as_key_size("tb_key_size");
  hls::stream<ap_uint8_t, 1024> as_output("tb_output");

  ap_uint8_t tmp;
  tmp.data = 16;
  as_key_size.write(tmp);

  for (int i = 0; i < 16; ++i) {
    ap_uint8_t tmp;
    tmp.data = key[i];
    as_key << tmp;
  }

  for (int j = 0; j < TIMES; ++j) {
    for (int i = 0; i < 16; ++i) {
      ap_uint8_t tmp;
      tmp.data = plaintext[i];
      tmp.last = (j == TIMES - 1 && i == 15);
      as_input << tmp;
    }
  }

  aes_encrypt(as_input, as_key, as_key_size, as_output);

  int result_size = 0;
  while (true) {
    ap_uint8_t tmp;
    as_output >> tmp;
    result[result_size++] = tmp.data;
    if (tmp.last)
      break;
  }

  if (result_size != 16 * TIMES) {
    printf("Result size: %d\n", result_size);
    return 1;
  }

  for (int j = 0; j < TIMES; ++j) {
    for (int i = 0; i < 16; ++i) {
      if (result[i + j * 16] != ciphertext[i]) {
        printf("\nCipher[%d] = %02X is different from correct answer %02X\n", i,
               result[i + j * 16], ciphertext[i]);
        return 1;
      }
    }
  }

  return 0;
}