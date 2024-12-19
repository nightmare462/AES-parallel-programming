#include "main.hpp"
#include "hls_stream.h"
#include "rcon.hpp"
#include "sbox.hpp"

#include <cstdint>
#include <stdio.h>

template <class T>
void copy_stream(hls::stream<T> &hs_in, hls::stream<T> &hs_out,
                 unsigned int count) {
#pragma HLS FUNCTION_INSTANTIATE variable = count
  for (int i = 0; i < count; ++i) {
#pragma HLS LOOP_TRIPCOUNT max = 32 avg = 4 min = 0
#pragma HLS UNROLL
    hs_out << hs_in.read();
  }
}

void createRoundKey(hls::stream<uint8_t> &hs_expanded_key,
                    hls::stream<uint8_t> &hs_round_key) {
  copy_stream(hs_expanded_key, hs_round_key, 16);
}

void createRoundKey(hls::stream<uint8_t> &hs_expanded_key_inp,
                    hls::stream<uint8_t> &hs_round_key,
                    int const expanded_key_size,
                    hls::stream<uint8_t> &hs_expanded_key_out) {
  copy_stream(hs_expanded_key_inp, hs_round_key, 16);
  copy_stream(hs_expanded_key_inp, hs_expanded_key_out, expanded_key_size - 16);
}

void addRoundKey(hls::stream<uint8_t> &hs_state_inp,
                 hls::stream<uint8_t> &hs_round_key,
                 hls::stream<uint8_t> &hs_state_out) {
  for (int i = 0; i < 16; ++i) {
    hs_state_out << (hs_state_inp.read() ^ hs_round_key.read());
  }
}

void subBytes(hls::stream<uint8_t> &hs_state_inp,
              hls::stream<uint8_t> &hs_state_out) {
  for (int i = 0; i < 16; ++i) {
    hs_state_out << getSBoxValue(hs_state_inp.read());
  }
}

void shiftRow(hls::stream<uint8_t> &hs_state_inp,
              hls::stream<uint8_t> &hs_state_out, uint8_t nbr) {
  // #pragma HLS DATAFLOW
  hls::stream<uint8_t, 4> hs_tmp;
  // hls::stream<uint8_t, 4> hs_tmp2;

  // for (int i = 0; i < 4; ++i) {
  //   hs_state_out << ((i < 4 - nbr) ? hs_tmp2.read() : hs_tmp.read());
  // }

  copy_stream(hs_state_inp, hs_tmp, nbr);
  copy_stream(hs_state_inp, hs_state_out, 4 - nbr);
  copy_stream(hs_tmp, hs_state_out, nbr);
}

void shiftRows(hls::stream<uint8_t> &hs_state_inp,
               hls::stream<uint8_t> &hs_state_out) {
#pragma HLS DATAFLOW

  hls::stream<uint8_t, 4> hs_state_column[4];
  hls::stream<uint8_t, 4> hs_state_shifted_column[4];

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      hs_state_column[j] << hs_state_inp.read();
    }
  }

SHIFT_ROWS_LOOP:
  for (int i = 0; i < 4; ++i) {
#pragma HLS UNROLL
    shiftRow(hs_state_column[i], hs_state_shifted_column[i], i);
  }

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      hs_state_out << hs_state_shifted_column[j].read();
    }
  }
}

uint8_t galois_multiplication(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  uint8_t hi_bit_set;
  for (uint8_t counter = 0; counter < 8; counter++) {
    if ((b & 1) == 1)
      p ^= a;
    hi_bit_set = (a & 0x80);
    a <<= 1;
    if (hi_bit_set == 0x80)
      a ^= 0x1b;
    b >>= 1;
  }
  return p;
}

void mixColumn(hls::stream<uint8_t> &hs_column_inp,
               hls::stream<uint8_t> &hs_column_out) {
#pragma HLS PIPELINE II = 1
  uint8_t cpy[4];
#pragma HLS ARRAY_PARTITION variable = cpy dim = 1 type = complete
  for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
    hs_column_inp >> cpy[i];
  }
  hs_column_out << (galois_multiplication(cpy[0], 2) ^
                    galois_multiplication(cpy[3], 1) ^
                    galois_multiplication(cpy[2], 1) ^
                    galois_multiplication(cpy[1], 3));

  hs_column_out << (galois_multiplication(cpy[1], 2) ^
                    galois_multiplication(cpy[0], 1) ^
                    galois_multiplication(cpy[3], 1) ^
                    galois_multiplication(cpy[2], 3));

  hs_column_out << (galois_multiplication(cpy[2], 2) ^
                    galois_multiplication(cpy[1], 1) ^
                    galois_multiplication(cpy[0], 1) ^
                    galois_multiplication(cpy[3], 3));

  hs_column_out << (galois_multiplication(cpy[3], 2) ^
                    galois_multiplication(cpy[2], 1) ^
                    galois_multiplication(cpy[1], 1) ^
                    galois_multiplication(cpy[0], 3));
}

void mixColumns(hls::stream<uint8_t> &hs_state_inp,
                hls::stream<uint8_t> &hs_state_out) {
  for (int i = 0; i < 4; ++i) {
    mixColumn(hs_state_inp, hs_state_out);
  }
}

void aesMainLoopIteration(hls::stream<uint8_t> &hs_expanded_key,
                          hls::stream<uint8_t> &hs_state_inp,
                          hls::stream<uint8_t> &hs_state_out) {
#pragma HLS DATAFLOW
  hls::stream<uint8_t, 16> hs_round_key("hs_round_key");
  hls::stream<uint8_t, 16> hs_state_after_sub_bytes("hs_state_after_sub_bytes");
  hls::stream<uint8_t, 16> hs_state_after_shift_rows(
      "hs_iteration_state_after_shift_rows");
  hls::stream<uint8_t, 16> hs_state_after_mix_columns(
      "hs_state_after_mix_columns");

  createRoundKey(hs_expanded_key, hs_round_key);
  subBytes(hs_state_inp, hs_state_after_sub_bytes);
  shiftRows(hs_state_after_sub_bytes, hs_state_after_shift_rows);
  mixColumns(hs_state_after_shift_rows, hs_state_after_mix_columns);
  addRoundKey(hs_state_after_mix_columns, hs_round_key, hs_state_out);
}

void aesMain(hls::stream<uint8_t> &hs_state_inp,
             hls::stream<uint8_t> &hs_state_out,
             hls::stream<uint8_t> &hs_expanded_key, int nbrRounds) {
  hls::stream<uint8_t, 16> hs_round_key("hs_round_key");
  hls::stream<uint8_t, 16> hs_state("hs_state");
  hls::stream<uint8_t, 16> hs_state_next("hs_state_next");
  hls::stream<uint8_t, 16> hs_state_after_sub_bytes("hs_state_after_sub_bytes");
  hls::stream<uint8_t, 16> hs_main_state_after_shift_rows(
      "hs_main_state_after_shift_rows");
  hls::stream<uint8_t, 16> hs_state_after_mix_columns(
      "hs_state_after_mix_columns");

  for (int i = 0; i < 16; ++i) {
    hs_state_next << hs_state_inp.read();
  }

  createRoundKey(hs_expanded_key, hs_round_key);
  addRoundKey(hs_state_next, hs_round_key, hs_state);

AES_MAIN_LOOP:
  for (int i = 1; i < nbrRounds; i++) {
// #pragma HLS LOOP_TRIPCOUNT max = 13 min = 9
#pragma HLS LOOP_TRIPCOUNT max = 9

    aesMainLoopIteration(hs_expanded_key, hs_state, hs_state_next);
    copy_stream(hs_state_next, hs_state, 16);
  }

  createRoundKey(hs_expanded_key, hs_round_key);
  subBytes(hs_state, hs_state_after_sub_bytes);
  shiftRows(hs_state_after_sub_bytes, hs_main_state_after_shift_rows);
  addRoundKey(hs_main_state_after_shift_rows, hs_round_key, hs_state_out);
}

// void aesMain(hls::stream<uint8_t> &hs_state_inp,
//              hls::stream<uint8_t> &hs_state_out,
//              hls::stream<uint8_t> &hs_expanded_key_inp, int const nbrRounds,
//              int const expanded_key_size) {
// #pragma HLS DATAFLOW
//   hls::stream<uint8_t, 16> hs_round_key_first("hs_round_key_first");
//   hls::stream<uint8_t, 16> hs_round_key_last("hs_round_key_last");
//   hls::stream<uint8_t, 16> hs_state_2loop("hs_state_2loop");
//   hls::stream<uint8_t, 16> hs_state_first("hs_state_first");
//   hls::stream<uint8_t, 16> hs_state_last("hs_state_last");
//   hls::stream<uint8_t, 16> hs_state_next("hs_state_next");
//   hls::stream<uint8_t, 16>
//   hs_state_after_sub_bytes("hs_state_after_sub_bytes"); hls::stream<uint8_t,
//   16> hs_main_state_after_shift_rows(
//       "hs_main_state_after_shift_rows");
//   hls::stream<uint8_t, 16> hs_state_after_mix_columns(
//       "hs_state_after_mix_columns");

//   hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key_2loop;
//   hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key_last;

//   for (int i = 0; i < 16; ++i) {
//     hs_state_first << hs_state_inp.read();
//   }

//   createRoundKey(hs_expanded_key_inp, hs_round_key_first, expanded_key_size,
//                  hs_expanded_key_2loop);
//   addRoundKey(hs_state_first, hs_round_key_first, hs_state_2loop);

// AES_MAIN_LOOP:
//   for (int i = 1; i < nbrRounds; i++) {
// // #pragma HLS LOOP_TRIPCOUNT max = 13 min = 9
// #pragma HLS LOOP_TRIPCOUNT max = 9

//     hls::stream<uint8_t, 16> hs_state("hs_state");
//     hls::stream<uint8_t, 16> hs_expanded_key("hs_expanded_key");
//     if (i == 1) {
//       copy_stream(hs_state_2loop, hs_state, 16);
//       copy_stream(hs_expanded_key_2loop, hs_expanded_key,
//                   expanded_key_size - 16);
//     }

//     aesMainLoopIteration(hs_expanded_key, hs_state, hs_state_next);
//     if (i == nbrRounds - 1) {
//       copy_stream(hs_state_next, hs_state_last, 16);
//       copy_stream(hs_expanded_key, hs_expanded_key_last, 16);
//     } else
//       copy_stream(hs_state_next, hs_state, 16);
//   }

//   createRoundKey(hs_expanded_key_last, hs_round_key_last);
//   subBytes(hs_state_last, hs_state_after_sub_bytes);
//   shiftRows(hs_state_after_sub_bytes, hs_main_state_after_shift_rows);
//   addRoundKey(hs_main_state_after_shift_rows, hs_round_key_last,
//   hs_state_out);
// }

void rotate(hls::stream<uint8_t> &hs_word_inp,
            hls::stream<uint8_t> &hs_word_out) {
  uint8_t c = hs_word_inp.read();
  copy_stream(hs_word_inp, hs_word_out, 3);
  hs_word_out << c;
}

void core(hls::stream<uint8_t> &hs_word_inp, hls::stream<uint8_t> &hs_word_out,
          int const iteration) {
  hls::stream<uint8_t, 4> hs_word_after_rotate("hs_word_after_rotate");
  hls::stream<uint8_t, 4> hs_word_after_sbox("hs_word_after_sbox");

  rotate(hs_word_inp, hs_word_after_rotate);

  for (int i = 0; i < 4; ++i) {
    hs_word_after_sbox << getSBoxValue(hs_word_after_rotate.read());
  }

  for (int i = 0; i < 4; ++i) {
    uint8_t word = hs_word_after_sbox.read();
    if (i == 0)
      word ^= getRconValue(iteration);
    hs_word_out << word;
  }
}

void expandKeyLoop(hls::stream<uint8_t> &hs_expanded_key_inp,
                   hls::stream<uint8_t> &hs_expanded_key_previous_inp,
                   hls::stream<uint8_t> &hs_next_t_arr_inp, int const key_size,
                   int const expanded_key_size,
                   hls::stream<uint8_t> &hs_expanded_key_out) {

  int rcon_iteration = 1;
  hls::stream<uint8_t, 4> hs_next_t_arr("hs_next_t_arr");
  hls::stream<uint8_t, 4> hs_t_arr_after_core("hs_t_arr_after_core");
  hls::stream<uint8_t, 4> hs_t_arr_after_core_tmp("hs_t_arr_after_core_tmp");
  hls::stream<uint8_t, 4> hs_t_arr_after_sbox("hs_t_arr_after_sbox");
  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key_previous(
      "hs_expanded_key_previous");

  copy_stream(hs_next_t_arr_inp, hs_next_t_arr, 4);
  copy_stream(hs_expanded_key_inp, hs_expanded_key_out, key_size);
  copy_stream(hs_expanded_key_previous_inp, hs_expanded_key_previous, key_size);

EXPAND_KEY_LOOP:
  for (int current_size = key_size; current_size < expanded_key_size;
       current_size += 4) {
// #pragma HLS LOOP_TRIPCOUNT max = 56 min = 40
#pragma HLS LOOP_TRIPCOUNT max = 40

    if (current_size % key_size == 0) {
      core(hs_next_t_arr, hs_t_arr_after_core, rcon_iteration++);
      // copy_stream(hs_t_arr_after_core, hs_t_arr_after_core_tmp, 4);
    } else {
      copy_stream(hs_next_t_arr, hs_t_arr_after_core, 4);
    }

    if (key_size == 32 && (current_size % key_size) == 16) {
      for (int i = 0; i < 4; ++i) {
        hs_t_arr_after_sbox << getSBoxValue(hs_t_arr_after_core.read());
      }
    } else {
      copy_stream(hs_t_arr_after_core, hs_t_arr_after_sbox, 4);
    }

    for (int i = 0; i < 4; ++i) {
      uint8_t old_key = hs_expanded_key_previous.read();
      uint8_t new_key = old_key ^ hs_t_arr_after_sbox.read();

      hs_expanded_key_out.write(new_key);
      if (current_size + i < expanded_key_size - 4)
        hs_next_t_arr.write(new_key);
      if (current_size + i < expanded_key_size - key_size)
        hs_expanded_key_previous.write(new_key);
    }
  }
}

void expandKey(hls::stream<ap_uint8_t> &hs_key, int const key_size,
               hls::stream<uint8_t> &hs_expanded_key,
               int const expanded_key_size) {
#pragma HLS DATAFLOW

  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_new_expanded_key(
      "hs_new_expanded_key");
  hls::stream<uint8_t, 4> hs_next_t_arr("hs_next_t_arr");
  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key_tmp(
      "hs_expanded_key_tmp");

LOAD_KEY_LOOP:
  for (int i = 0; i < key_size; ++i) {
// #pragma HLS LOOP_TRIPCOUNT max = 32 min = 16
#pragma HLS LOOP_TRIPCOUNT max = 16
    uint8_t key_byte = hs_key.read().data;
    hs_expanded_key_tmp << key_byte;
    hs_new_expanded_key << key_byte;
    if (i >= key_size - 4) {
      hs_next_t_arr << key_byte;
    }
  }

  expandKeyLoop(hs_expanded_key_tmp, hs_new_expanded_key, hs_next_t_arr,
                key_size, expanded_key_size, hs_expanded_key);
}

void aesEncryptLoop(int const expanded_key_size, int const nbrRounds,
                    uint8_t const expanded_key[MAX_EXPANDED_KEY_SIZE],
                    hls::stream<uint8_t> &hs_block_inp,
                    hls::stream<uint8_t> &hs_block_out) {
#pragma HLS DATAFLOW
  // #pragma HLS INLINE
  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key(
      "hs_expanded_key");

  for (int i = 0; i < expanded_key_size; ++i) {
    hs_expanded_key << expanded_key[i];
  }

  aesMain(hs_block_inp, hs_block_out, hs_expanded_key, nbrRounds);
}

void aes_encrypt(hls::stream<ap_uint8_t> &hs_input,
                 hls::stream<ap_uint8_t> &hs_key,
                 hls::stream<ap_uint8_t> &hs_key_size,
                 hls::stream<ap_uint8_t> &hs_output) {
#pragma HLS DATAFLOW
#pragma HLS INTERFACE mode = s_axilite port = return
#pragma HLS INTERFACE mode = axis port = hs_input
#pragma HLS INTERFACE mode = axis port = hs_key
#pragma HLS INTERFACE mode = axis port = hs_key_size
#pragma HLS INTERFACE mode = axis port = hs_output

  int nbrRounds;
  int expanded_key_size;
  int key_size = hs_key_size.read().data;
  uint8_t expanded_key[THREAD_COUNT][MAX_EXPANDED_KEY_SIZE];
#pragma HLS ARRAY_PARTITION variable = expanded_key dim = 1 factor =           \
    THREAD_COUNT type = block

  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key;

  hls::stream<uint8_t, 1024> hs_block_inp[THREAD_COUNT];
  hls::stream<uint8_t, 1024> hs_block_out[THREAD_COUNT];
  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE>
      hs_expanded_key_2loop[THREAD_COUNT];
  hls::stream<uint8_t, 4> hs_block_inp_column[4];
  hls::stream<uint8_t, 4> hs_block_out_column[4];

  // Determine the size
  if (key_size == 16)
    nbrRounds = 10;
  else if (key_size == 24)
    nbrRounds = 12;
  else if (key_size == 32)
    nbrRounds = 14;
  else
    nbrRounds = 10; // Fallback

  expanded_key_size = (16 * (nbrRounds + 1));

  expandKey(hs_key, key_size, hs_expanded_key, expanded_key_size);

  bool last_one = false;
  int data_size = 0;

  // 32 512
  // 30000 48000
  // 135009 2160144

  while (!last_one) {
#pragma HLS LOOP_TRIPCOUNT max = 48000
    ap_uint8_t tmp;
    hs_input >> tmp;
    hs_block_inp[(data_size / 16) % THREAD_COUNT] << tmp.data;
    data_size++;
    last_one = tmp.last;
  }

  for (int i = 0; i < expanded_key_size; ++i) {
#pragma HLS LOOP_TRIPCOUNT max = 240
    uint8_t key;
    hs_expanded_key >> key;
    for (int j = 0; j < THREAD_COUNT; ++j) {
      expanded_key[j][i] = key;
    }
  }

  for (int j = 0; j < data_size; j += 16 * THREAD_COUNT) {
#pragma HLS DATAFLOW
#pragma HLS LOOP_TRIPCOUNT max = 30000 / (THREAD_COUNT)
    for (int i = 0; i < expanded_key_size; ++i) {
      hs_expanded_key_2loop[0] << expanded_key[0][i];
    }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[1] << expanded_key[1][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[2] << expanded_key[2][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[3] << expanded_key[3][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[4] << expanded_key[4][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[5] << expanded_key[5][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[6] << expanded_key[6][i];
    // }
    // for (int i = 0; i < expanded_key_size; ++i) {
    //   hs_expanded_key_2loop[7] << expanded_key[7][i];
    // }
  }

  for (int i = 0; i < data_size; i += 16 * THREAD_COUNT) {
#pragma HLS DATAFLOW
#pragma HLS LOOP_TRIPCOUNT max = 30000 / (THREAD_COUNT)

    aesMain(hs_block_inp[0], hs_block_out[0], hs_expanded_key_2loop[0],
            nbrRounds);
    // aesMain(hs_block_inp[1], hs_block_out[1], hs_expanded_key_2loop[1],
    //         nbrRounds);
    // aesMain(hs_block_inp[2], hs_block_out[2], hs_expanded_key_2loop[2],
    //         nbrRounds);
    // aesMain(hs_block_inp[3], hs_block_out[3], hs_expanded_key_2loop[3],
    //         nbrRounds);
    // aesMain(hs_block_inp[4], hs_block_out[4], hs_expanded_key_2loop[4],
    //         nbrRounds);
    // aesMain(hs_block_inp[5], hs_block_out[5], hs_expanded_key_2loop[5],
    //         nbrRounds);
    // aesMain(hs_block_inp[6], hs_block_out[6], hs_expanded_key_2loop[6],
    //         nbrRounds);
    // aesMain(hs_block_inp[7], hs_block_out[7], hs_expanded_key_2loop[7],
    //         nbrRounds);

    // aesEncryptLoop(expanded_key_size, nbrRounds, expanded_key,
    // hs_block_inp[0],
    //                hs_block_out[0]);
    // aesEncryptLoop(expanded_key_size, nbrRounds, expanded_key,
    // hs_block_inp[1],
    //                hs_block_out[1]);
    // aesEncryptLoop(expanded_key_size, nbrRounds, expanded_key,
    // hs_block_inp[2],
    //                hs_block_out[2]);
    // aesEncryptLoop(expanded_key_size, nbrRounds, expanded_key,
    // hs_block_inp[3],
    //                hs_block_out[3]);
  }

  for (int i = 0; i < data_size; ++i) {
#pragma HLS LOOP_TRIPCOUNT max = 48000
    ap_uint8_t tmp;
    tmp.data = hs_block_out[(data_size / 16) % THREAD_COUNT].read();
    tmp.last = i == data_size - 1;
    hs_output.write(tmp);
  }
}