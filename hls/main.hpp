#ifndef _MAIN_HPP_
#define _MAIN_HPP_

#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define MAX_EXPANDED_KEY_SIZE 16 * 15
#define THREAD_COUNT 1

#define uint8_t ap_uint<8>
#define ap_uint8_t ap_axiu<8, 1, 1, 1>

void aes_encrypt(hls::stream<ap_uint8_t> &hs_input, hls::stream<ap_uint8_t> &hs_key,
                 hls::stream<ap_uint8_t> &hs_key_size,
                 hls::stream<ap_uint8_t> &hs_output);

#endif