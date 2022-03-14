/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#ifndef MAIN3_H
#define MAIN3_H

#include <metal/tty.h>
#include <stdint.h>

extern uint32_t __sep3_segment_stack_begin;
extern uint32_t __sep3_segment_stack_end;
static uint32_t sep3_segment_stack_begin = (uint32_t)&__sep3_segment_stack_begin;
static uint32_t sep3_segment_stack_end = (uint32_t)&__sep3_segment_stack_end;

void sep3_main(void);

#endif
