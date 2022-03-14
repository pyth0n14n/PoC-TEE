/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

#ifndef MAIN1_H
#define MAIN1_H

#include <metal/tty.h>
#include <stdint.h>

extern uint32_t __sep1_segment_stack_begin;
extern uint32_t __sep1_segment_stack_end;
static uint32_t sep1_segment_stack_begin = (uint32_t)&__sep1_segment_stack_begin;
static uint32_t sep1_segment_stack_end = (uint32_t)&__sep1_segment_stack_end;

void sep1_main(void);

#endif
