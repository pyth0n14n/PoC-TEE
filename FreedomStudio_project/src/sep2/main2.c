/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#include "main2.h"
#include "../shared.h"
#include "micro-aes/aes.h"

#define AES_BLOCK_SIZE 16

aes_128_context_t ctx;

static uint8_t key[AES_BLOCK_SIZE] = {0x00, 0x01, 0x02, 0x03,
                                      0x04, 0x05, 0x06, 0x07,
                                      0x08, 0x09, 0x0a, 0x0b,
                                      0x0c, 0x0d, 0x0e, 0x0f};

void sep2_main() {
  int i;
  uint8_t block[AES_BLOCK_SIZE] = {0};

  aes_128_init(&ctx, key);
  for (i = 0; i < AES_BLOCK_SIZE; i++) {
    block[i] = shared_buffer[SHARED_DATA + i];
  }

  aes_128_encrypt(&ctx, block);

  for (i = 0; i < AES_BLOCK_SIZE; i++) {
    shared_buffer[SHARED_DATA + i] = block[i];
  }

  call_app(CTX_SEP2, CTX_END); // finish
}
