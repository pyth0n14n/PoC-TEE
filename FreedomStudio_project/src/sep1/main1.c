/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

// Dispatcher application
// This is created with reffer to smartcard APDU.
#include "main1.h"
#include "../shared.h"

#define CMD_LEN 4
#define BLEN 16

//-----------------------------------
//   Variables
//-----------------------------------
uint8_t cmd[CMD_LEN];
uint8_t ack[2] = {0x00, 0x00}; // responce byte; OK if 0x90 0x00
uint8_t buf1[BLEN];

/* Specification of the command packet
  cmd[0]: Command
  cmd[1]: Class
  cmd[2]: Function
  cmd[3]: size
 */

/* Overview of all commands
  0x80
   |-0x10: general functions
   | |-0x10: cmd_echo_cmd()
   | |-0x20: cmd_get_responce()
   | |-0x30: cmd_set_buffer()
   |
   |-0x20: ecall functions
     |-0x10: cmd_set_shared()
     |-0x18: cmd_get_shared()
     |-0x20: cmd_call_sep2()
     |-0x30: cmd_call_sep3()
 */

//-----------------------------------
//   Functions
//-----------------------------------
// ***** common functions *****
void recv_data(uint8_t *dst, uint8_t size)
{
  int i;
  int c;
  for (i = 0; i < size; i++)
  {
    metal_tty_getc(&c);
    dst[i] = (uint8_t)(c & 0xff);
  }
}

void send_data(uint8_t *src, uint8_t size)
{
  int i;
  for (i = 0; i < size; i++)
  {
    metal_tty_putc_raw(src[i]);
  }
}

// ***** general functions *****
void cmd_echo_cmd()
{
  send_data(cmd, CMD_LEN);
  ack[0] = 0x90;
  ack[1] = 0x00;
}

void cmd_get_response(void)
{
  send_data(buf1, BLEN);
  ack[0] = 0x90;
  ack[1] = 0x00;
}

void cmd_set_buffer(void)
{
  recv_data(buf1, BLEN);
  ack[0] = 0x90;
  ack[1] = 0x00;
}

// ***** ecall function *****
void cmd_set_shared_cmd(void)
{
  int i;
  uint8_t buf;

  for (i = 0; i < SHARED_CMD_SIZE; i++) {
    recv_data(&buf, 1);
    shared_buffer[SHARED_CMD + i] = buf;
  }
  ack[0] = 0x90;
  ack[1] = 0x00;
}

void cmd_set_shared(void)
{
  int i;
  if (cmd[3] > BLEN) {
    ack[0] = 0xff;
    ack[1] = 0x03;
  }
  else {
    for (i = 0; i < cmd[3]; i++) {
      shared_buffer[SHARED_DATA + i] = buf1[i];
    }
    ack[0] = 0x90;
    ack[1] = 0x00;
  }
}

void cmd_get_shared(void)
{
  int i;

  if (cmd[3] > BLEN) {
    ack[0] = 0xff;
    ack[1] = 0x03;
  }
  else {
    for (i = 0; i < cmd[3]; i++) {
      buf1[i] = shared_buffer[SHARED_DATA + i];
    }
    ack[0] = 0x90;
    ack[1] = 0x00;
  }
}

void cmd_call_sep2(void)
{
  call_app(CTX_SEP1, CTX_SEP2);
  ack[0] = 0x90;
  ack[1] = 0x00;
}

void cmd_call_sep3(void)
{
  call_app(CTX_SEP1, CTX_SEP3);
  ack[0] = 0x90;
  ack[1] = 0x00;
}

// ***** main function *****
void sep1_main() {
  char motd[] = "Start Freedom OS\n";

  send_data(motd, sizeof(motd));

  // Clear buffer and synchronize
  while (1) {
    recv_data(cmd, 4);
    send_data(cmd, 4);
    if (cmd[0] == 'r' && cmd[1] == 'i' && cmd[2] == 's' && cmd[3] == 'c') {
      break;
    }
  }

  while (1) {
    // read command
    recv_data(cmd, CMD_LEN);

    switch (cmd[0]) {
    // valid cmd
    case 0x80:
      switch (cmd[1]) {
      // general function zone
      case 0x10:
        switch (cmd[2]) {
        case 0x10:
          cmd_echo_cmd();
          break;

        case 0x20:
          cmd_get_response();
          break;

        case 0x30:
          cmd_set_buffer();
          break;

        default:
          ack[0] = 0xff;
          ack[1] = 0x21;
          break;
        }
        break;

      // ecall function zone
      case 0x20:
        switch (cmd[2]) {
        case 0x08:
          cmd_set_shared_cmd();
          break;

        case 0x10:
          cmd_set_shared();
          break;

        case 0x18:
          cmd_get_shared();
          break;

        case 0x20:
          // sep2_main();
          cmd_call_sep2();
          break;

        case 0x30:
          // sep3_main();
          cmd_call_sep3();
          break;

        default:
          ack[0] = 0xff;
          ack[1] = 0x22;
          break;
        }
        break;

      default:
        ack[0] = 0xff;
        ack[1] = 0x18;
        break;
      }
      break;

    default:
      ack[0] = 0xff;
      ack[1] = 0x00;
      break;
    }

    // return ack
    send_data(ack, 2);
  }
}

