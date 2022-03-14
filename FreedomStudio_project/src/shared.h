/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>

#define SHARED_SP     0    // 0:3
#define SHARED_RA     4    // 4:7
#define SHARED_CALLER 8    // 8
#define SHARED_CALLEE 9    // 9
#define SHARED_CMD    10   // 10:11
#define SHARED_DATA   12   // 12:end

#define SHARED_CMD_SIZE    2     // 10:11
#define SHARED_DATA_SIZE   116   // 12:127

#define CTX_SEP1 0
#define CTX_SEP2 1
#define CTX_SEP3 2
#define CTX_END  0xFF
#define NUM_CTX  3

extern uint8_t shared_buffer[128];

void call_app(uint8_t, uint8_t);

#endif
