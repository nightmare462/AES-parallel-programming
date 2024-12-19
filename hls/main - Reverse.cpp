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
    hs_out << hs_in.read();
  }
}

void createRoundKey(hls::stream<uint8_t> &hs_expanded_key,
                    hls::stream<uint8_t> &hs_round_key) {
  //   hls::stream<uint8_t, 4> hs_round_key_column[4];
  // REARRANGE_LOOP:
  //   for (int i = 0; i < 4; ++i) {
  // #pragma HLS LOOP_FLATTEN
  //     for (int j = 0; j < 4; ++j) {
  //       hs_round_key_column[j] << hs_expanded_key.read();
  //     }
  //   }

  // OUTPUT_LOOP:
  //   for (int i = 0; i < 4; ++i) {
  //     copy_stream(hs_round_key_column[i], hs_round_key, 4);
  //   }
  copy_stream(hs_expanded_key, hs_round_key, 16);
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
  hls::stream<uint8_t, 4> hs_tmp;

  copy_stream(hs_state_inp, hs_tmp, nbr);
  copy_stream(hs_state_inp, hs_state_out, 4 - nbr);
  copy_stream(hs_tmp, hs_state_out, nbr);
}

void shiftRows(hls::stream<uint8_t> &hs_state_inp,
               hls::stream<uint8_t> &hs_state_out) {
  hls::stream<uint8_t, 4> hs_state_row[4];

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      hs_state_row[j] << hs_state_inp.read();
    }
  }

  for (int i = 0; i < 4; ++i) {
    shiftRow(hs_state_row[i], hs_state_row[i], i);
  }

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      hs_state_out << hs_state_row[j].read();
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
  hls::stream<uint8_t, 4> hs_column_inp[4];
  hls::stream<uint8_t, 4> hs_column_out[4];

  for (int i = 0; i < 4; ++i) {
    copy_stream(hs_state_inp, hs_column_inp[i], 4);
  }

  for (int i = 0; i < 4; ++i) {
#pragma HLS UNROLL
    mixColumn(hs_column_inp[i], hs_column_out[i]);
  }

  for (int i = 0; i < 4; ++i) {
    copy_stream(hs_column_inp[i], hs_state_out, 4);
  }
}

void aesMainLoopIteration(hls::stream<uint8_t> &hs_expanded_key,
                          hls::stream<uint8_t> &hs_state_inp,
                          hls::stream<uint8_t> &hs_state_out) {
#pragma HLS DATAFLOW
  hls::stream<uint8_t, 16> hs_round_key("hs_round_key");
  hls::stream<uint8_t, 16> hs_state_after_sub_bytes("hs_state_after_sub_bytes");
  hls::stream<uint8_t, 16> hs_state_after_shift_rows(
      "hs_state_after_shift_rows");
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
  hls::stream<uint8_t, 16> hs_state_after_shift_rows(
      "hs_state_after_shift_rows");
  hls::stream<uint8_t, 16> hs_state_after_mix_columns(
      "hs_state_after_mix_columns");

  for (int i = 0; i < 16; ++i) {
    hs_state << hs_state_inp.read();
  }

  createRoundKey(hs_expanded_key, hs_round_key);
  addRoundKey(hs_state, hs_round_key, hs_state);

AES_MAIN_LOOP:
  for (int i = 1; i < nbrRounds; i++) {
#pragma HLS LOOP_TRIPCOUNT max = 13 min = 9
    aesMainLoopIteration(hs_expanded_key, hs_state, hs_state_next);
    copy_stream(hs_state_next, hs_state, 16);
  }

  createRoundKey(hs_expanded_key, hs_round_key);
  subBytes(hs_state, hs_state_after_sub_bytes);
  shiftRows(hs_state_after_sub_bytes, hs_state_after_shift_rows);
  addRoundKey(hs_state_after_shift_rows, hs_round_key, hs_state_out);
}

void rotate(uint8_t word[4]) {
  uint8_t c = word[0];
  for (int i = 0; i < 3; ++i) {
    word[i] = word[i + 1];
  }
  word[3] = c;
}

void rotate(hls::stream<uint8_t> &hs_word_inp,
            hls::stream<uint8_t> &hs_word_out) {
#pragma HLS PIPELINE II = 1
  uint8_t c = hs_word_inp.read();
  copy_stream(hs_word_inp, hs_word_out, 3);
  hs_word_out << c;
}

void core(hls::stream<uint8_t> &hs_word_inp, hls::stream<uint8_t> &hs_word_out,
          int iteration) {
  hls::stream<uint8_t, 4> hs_word_after_rotate("hs_word_after_rotate");
  hls::stream<uint8_t, 4> hs_word_after_sbox("hs_word_after_sbox");

  rotate(hs_word_inp, hs_word_after_rotate);

  for (int i = 0; i < 4; ++i) {
    hs_word_after_sbox << getSBoxValue(hs_word_after_rotate.read());
  }

  hs_word_out << (hs_word_after_sbox.read() ^ getRconValue(iteration));
  copy_stream(hs_word_after_sbox, hs_word_out, 3);
}

void expandKeyLoop(hls::stream<uint8_t> &hs_expanded_key_inp,
                   hls::stream<uint8_t> &hs_expanded_key_previous_inp,
                   hls::stream<uint8_t> &hs_next_t_arr_inp, int const key_size,
                   int const expanded_key_size,
                   hls::stream<uint8_t> &hs_expanded_key_out) {

  int rcon_iteration = 1;

  hls::stream<uint8_t, 4> hs_next_t_arr("hs_next_t_arr");
  hls::stream<uint8_t, 4> hs_t_arr_after_core("hs_t_arr_after_core");
  hls::stream<uint8_t, 4> hs_t_arr_after_sbox("hs_t_arr_after_sbox");
  hls::stream<uint8_t, MAX_EXPANDED_KEY_SIZE> hs_expanded_key_previous(
      "hs_expanded_key_previous");

  copy_stream(hs_next_t_arr_inp, hs_next_t_arr, 4);
  copy_stream(hs_expanded_key_inp, hs_expanded_key_out, key_size);
  copy_stream(hs_expanded_key_previous_inp, hs_expanded_key_previous, key_size);

EXPAND_KEY_LOOP:
  for (int current_size = key_size; current_size < expanded_key_size;
       current_size += 4) {
#pragma HLS LOOP_TRIPCOUNT max = 56 min = 40

    if (current_size % key_size == 0) {
      core(hs_next_t_arr, hs_t_arr_after_core, rcon_iteration++);
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
#pragma HLS LOOP_TRIPCOUNT max = 32 min = 16
    uint8_t key_byte = hs_key.read().data;
    // hs_expanded_key.write(key_byte);
    hs_expanded_key_tmp << key_byte;
    hs_new_expanded_key << key_byte;
    if (i >= key_size - 4) {
      hs_next_t_arr << key_byte;
    }
  }

  expandKeyLoop(hs_expanded_key_tmp, hs_new_expanded_key, hs_next_t_arr,
                key_size, expanded_key_size, hs_expanded_key);
}

void aes_encrypt(hls::stream<ap_uint8_t> &hs_input,
                 hls::stream<ap_uint8_t> &hs_key,
                 hls::stream<ap_uint8_t> &hs_key_size,
                 hls::stream<ap_uint8_t> &hs_output) {
#pragma HLS DATAFLOW
#pragma HLS INTERFACE mode = axis port = hs_input
#pragma HLS INTERFACE mode = axis port = hs_key
#pragma HLS INTERFACE mode = axis port = hs_key_size
#pragma HLS INTERFACE mode = axis port = hs_output

  int nbrRounds;
  int expanded_key_size;
  int key_size = hs_key_size.read().data;

  hls::stream<uint8_t, 240> hs_expanded_key;
  hls::stream<uint8_t, 16> hs_block_inp;
  hls::stream<uint8_t, 16> hs_block_out;
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

  // // Transform the block
  // for (int i = 0; i < 4; ++i) {
  //   for (int j = 0; j < 4; ++j) {
  //     hs_block_inp_column[j] << hs_input.read().data;
  //   }
  // }

  // for (int i = 0; i < 4; ++i) {
  //   copy_stream(hs_block_inp_column[i], hs_block_inp, 4);
  // }

  for (int i = 0; i < 16; ++i) {
    hs_block_inp << hs_input.read().data;
  }

  aesMain(hs_block_inp, hs_block_out, hs_expanded_key, nbrRounds);

  // // Transform back the block
  // for (int i = 0; i < 4; ++i) {
  //   for (int j = 0; j < 4; ++j) {
  //     hs_block_out_column[j] << hs_block_out.read();
  //   }
  // }

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      ap_uint8_t tmp;
      tmp.data = hs_block_out.read();
      tmp.last = (i + j * 4) == 15;
      hs_output.write(tmp);
    }
  }
}