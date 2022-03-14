/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#include "shared.h"

uint8_t shared_buffer[128];

void call_app(uint8_t caller_id, uint8_t callee_id){
  uintptr_t sp, ra;
  uintptr_t *t;
  __asm__ volatile ("mv %0, sp" : "=r"(sp));
  __asm__ volatile ("mv %0, ra" : "=r"(ra));

  t = (uintptr_t *)&shared_buffer[SHARED_SP];
  *t = sp; // [0:3]
  t = (uintptr_t *)&shared_buffer[SHARED_RA];
  *t = ra; // [4:7]
  shared_buffer[SHARED_CALLER] = caller_id;
  shared_buffer[SHARED_CALLEE] = callee_id;
  __asm__ volatile("ecall");
}


