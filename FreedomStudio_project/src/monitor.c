/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2022 Shoei Nashimoto */
#include "monitor.h"
#include "shared.h"

#include <stdlib.h>
#include <stdio.h>

#include <metal/cpu.h>
#include <metal/pmp.h>
#include <metal/privilege.h>

#define ECODE_ILLEGAL_INSTRUCTION	2
#define ECODE_LOAD_ACCESS_FAULT         5
#define ECODE_STORE_ACCESS_FAULT        7
#define ECODE_SYSCALL_FROM_U_MODE	8

#define ROM_METAL_SIZE 0x20000 // 128K
#define RAM_METAL_SIZE 0x1000  // 4K
#define ROM_SIZE  0x10000      // 64K
#define RAM_SIZE  0x800	       // 2K
#define RAM_LARGE_SIZE  0x1000 // 4K
#define STACK_SIZE 1024        // 1K

#define FLASH_METAL 0x20400000
#define FLASH_MON   0x20420000
#define FLASH_SEP1  0x20430000
#define FLASH_SEP2  0x20440000
#define FLASH_SEP3  0x20450000

#define RAM_METAL 0x80000000
#define RAM_MON   0x80001000
#define RAM_SEP1  0x80002000
#define RAM_SEP2  0x80003000
#define RAM_SEP3  0x80003800

#define MEM_END   0x20001000 // 0x80004000 >> 2

#define TRIG_PIN 18  // IO(2) on Arty board
#define MARK_PIN 19  // IO(3) on Arty board; for fault range, marking pin
#define CSRW_PIN 20  // IO(4) on Arty board; for fault range, marking pin, non-target
#define CSRC_PIN 21  // IO(5) on Arty board; for fault range, marking pin, non-target
#define CSRS_PIN 22  // IO(6) on Arty board; for fault range, marking pin, non-target

/* --------- global variables ---------------*/
static struct metal_pmp *pmp;

static struct metal_pmp_config config_rom = {
  .L = METAL_PMP_UNLOCKED,
  .A = METAL_PMP_NAPOT,
  .X = 1,
  .W = 0,
  .R = 1,
};

static struct metal_pmp_config config_ram = {
  .L = METAL_PMP_UNLOCKED,
  .A = METAL_PMP_NAPOT,
  .X = 0,
  .W = 1,
  .R = 1,
};

static struct metal_pmp_config config_disable __attribute__ ((aligned (32))) = {
  .L = METAL_PMP_UNLOCKED,
  .A = METAL_PMP_OFF,
  .X = 0,
  .W = 0,
  .R = 0,
};

uintptr_t sep_entries[3] = {};

uintptr_t sep_sps[3] = {};

// whether sep process is working or not
volatile uintptr_t context[3] = {0};
volatile uintptr_t reg_stack[12] = {0}; // store pc and sp
volatile uint8_t sep_ptr = 0;

/* --------- for fault injection ------------------*/
void initGPIO(struct metal_gpio *gpio) {
  metal_gpio_disable_input(gpio, TRIG_PIN);
  metal_gpio_enable_output(gpio, TRIG_PIN);
  metal_gpio_disable_pinmux(gpio, TRIG_PIN);

  metal_gpio_set_pin(gpio, TRIG_PIN, 0);

  metal_gpio_disable_input(gpio, MARK_PIN);
  metal_gpio_enable_output(gpio, MARK_PIN);
  metal_gpio_disable_pinmux(gpio, MARK_PIN);

  metal_gpio_set_pin(gpio, MARK_PIN, 0);

  metal_gpio_disable_input(gpio, CSRS_PIN);
  metal_gpio_enable_output(gpio, CSRS_PIN);
  metal_gpio_disable_pinmux(gpio, CSRS_PIN);

  metal_gpio_set_pin(gpio, CSRS_PIN, 0);

  metal_gpio_disable_input(gpio, CSRC_PIN);
  metal_gpio_enable_output(gpio, CSRC_PIN);
  metal_gpio_disable_pinmux(gpio, CSRC_PIN);

  metal_gpio_set_pin(gpio, CSRC_PIN, 0);

  metal_gpio_disable_input(gpio, CSRW_PIN);
  metal_gpio_enable_output(gpio, CSRW_PIN);
  metal_gpio_disable_pinmux(gpio, CSRW_PIN);

  metal_gpio_set_pin(gpio, CSRW_PIN, 0);
}

#define CONFIG_TO_INT(_config) (*((char *) &(_config)))

/* --------- suprementary functions ---------------*/
size_t napot_addr(size_t addr, size_t size){
  size_t ret_addr = (addr) >> 2;
  ret_addr &= ~(size >> 3);
  ret_addr |= ((size >> 3) - 1);
  return ret_addr;
}

void set_pmp(uint8_t pid){
  int rc;

  rc =  metal_pmp_set_region(pmp, 0, config_rom, napot_addr((size_t) FLASH_METAL, (size_t) ROM_METAL_SIZE));
  rc += metal_pmp_set_region(pmp, 1, config_ram, napot_addr((size_t) RAM_METAL, (size_t) RAM_METAL_SIZE));

  switch(pid){
  case CTX_SEP1:
    rc += metal_pmp_set_region(pmp, 2, config_rom, napot_addr((size_t) FLASH_SEP1, (size_t) ROM_SIZE));
    rc += metal_pmp_set_region(pmp, 3, config_ram, napot_addr((size_t) RAM_SEP1, (size_t) RAM_LARGE_SIZE));
    rc += metal_pmp_set_region(pmp, 4, config_ram, napot_addr((size_t) METAL_SIFIVE_UART0_10013000_BASE_ADDRESS, (size_t) METAL_SIFIVE_UART0_10013000_SIZE));
    break;
  case CTX_SEP2:
    rc += metal_pmp_set_region(pmp, 2, config_rom, napot_addr((size_t) FLASH_SEP2, (size_t) ROM_SIZE));
    rc += metal_pmp_set_region(pmp, 3, config_ram, napot_addr((size_t) RAM_SEP2, (size_t) RAM_SIZE));
    rc += metal_pmp_set_region(pmp, 4, config_disable, 0);
    break;
  case CTX_SEP3:
    rc += metal_pmp_set_region(pmp, 2, config_rom, napot_addr((size_t) FLASH_SEP3, (size_t) ROM_SIZE));
    rc += metal_pmp_set_region(pmp, 3, config_ram, napot_addr((size_t) RAM_SEP3, (size_t) RAM_SIZE));
    rc += metal_pmp_set_region(pmp, 4, config_disable, 0);
    break;
  default:
    rc += metal_pmp_set_region(pmp, 2, config_disable, 0);
    rc += metal_pmp_set_region(pmp, 3, config_disable, 0);
    rc += metal_pmp_set_region(pmp, 4, config_disable, 0);
    break;
  }
  rc += metal_pmp_set_region(pmp, 5, config_disable, 0);
  rc += metal_pmp_set_region(pmp, 6, config_disable, 0);
  rc += metal_pmp_set_region(pmp, 7, config_disable, 0);

  if(rc < 0) {
    printf("Failed to change PMP\n");
    return;
  }
}


void call_sep_process(uintptr_t pc, uintptr_t sp, uintptr_t ra) {
  /* printf("Dropping privilege to User mode\n"); */

  /* Drop to user mode */
  struct metal_register_file regfile = {
    .ra = ra,
    .sp = sp,
  };
  metal_privilege_drop_to_mode(METAL_PRIVILEGE_USER, regfile, pc);
}


/* --------- handler functions ---------------*/
void syscall_from_u_handler(struct metal_cpu *cpu, int ecode)
{
  uintptr_t pc, mepc;
  uintptr_t sp, ra;
  uint8_t caller_id, callee_id;

  __asm__ volatile ("csrr %0, mepc" : "=r"(mepc));

  sp = *((uintptr_t *)&shared_buffer[SHARED_SP]);
  ra = *((uintptr_t *)&shared_buffer[SHARED_RA]);
  caller_id = shared_buffer[SHARED_CALLER];
  callee_id = shared_buffer[SHARED_CALLEE];

  if(ecode == ECODE_SYSCALL_FROM_U_MODE) {
    /* printf("Handling syscall from User mode\n"); */

    // --- Process is finilized -> restore from stack ---
    if (callee_id == CTX_END) {
      if (sep_ptr < 1) {
        /* printf("invalid sep_ptr %d\n", sep_ptr); */
        exit(7);
      }

      // Update the corresponding process to ''stop status''
      context[caller_id] = 0;

      // Restore from stack
      caller_id = reg_stack[--sep_ptr];
      ra = reg_stack[--sep_ptr];
      sp = reg_stack[--sep_ptr];
      pc = reg_stack[--sep_ptr];

      if (caller_id > NUM_CTX) exit(7);

      set_pmp(caller_id);

      // Rectangle trigger
      if (caller_id == CTX_SEP3 && callee_id == CTX_END) {
        struct metal_gpio *gpio0;
        gpio0 = metal_gpio_get_device(0);

        (*(volatile uint32_t *)(0x1001200C)) &= ~(1 << TRIG_PIN);
      }

    }
    // --- Process is callse -> backup to stack ---
    else {
      mepc += 4; // Ecall is 4byte instruction

      // callee proccess is already running
      if (context[callee_id] != 0){
        // Invalid call. Return to the caller process.
        call_sep_process(mepc, sp, ra);
      }
      else {
        // backup to the stack
        reg_stack[sep_ptr++] = mepc;
        reg_stack[sep_ptr++] = sp;
        reg_stack[sep_ptr++] = ra;
        reg_stack[sep_ptr++] = caller_id;
      }

      if (callee_id > NUM_CTX) exit(7);

      // Set pmp according to the callee process ID.
      if (caller_id == CTX_SEP1 && callee_id == CTX_SEP3) {
        struct metal_gpio *gpio0;
        gpio0 = metal_gpio_get_device(0);
        (*(volatile uint32_t *)(0x1001200C)) |=  (1 << TRIG_PIN);
      }

      set_pmp(callee_id);
      context[callee_id] = 1;
      pc = (uintptr_t)sep_entries[callee_id];
      sp = (uintptr_t)sep_sps[callee_id];
      ra = 0;
    }
  }
  else {
    exit(7);
  }
  // Transit to the proper process.
  call_sep_process(pc, sp, ra);
}

void illegal_instruction_fault_handler(struct metal_cpu *cpu, int ecode)
{
  if(ecode == ECODE_ILLEGAL_INSTRUCTION) {
    printf("Caught illegal instruction in User mode\n");
    exit(0);
  } else {
    exit(7);
  }
}

void access_fault_handler(struct metal_cpu *cpu, int ecode)
{
  int i;
  uintptr_t pc, sp, ra;
  uint8_t caller_id = 0xff;

  if(ecode == ECODE_LOAD_ACCESS_FAULT || ecode == ECODE_STORE_ACCESS_FAULT) {
    // Halt all process, and return to app1
      if (sep_ptr < 1) {
        /* printf("invalid sep_ptr %d\n", sep_ptr); */
        exit(7);
      }

      // Update the corresponding process to ''stop status''
      for (i = 0; i < NUM_CTX; i++) {
        context[i] = 0;
      }
      context[CTX_SEP1] = 1;

      while (caller_id != CTX_SEP1) {
        // Restore from stack
        caller_id = reg_stack[--sep_ptr];
        ra = reg_stack[--sep_ptr];
        sp = reg_stack[--sep_ptr];
        pc = reg_stack[--sep_ptr];

        if (caller_id > 3) exit(7);
      }
      set_pmp(caller_id);
      for (i = SHARED_DATA; i < 128; i++) {
        shared_buffer[i] = 0xff;
      }
  } else {
    exit(7);
  }

  // Lower the trigger signal.
  struct metal_gpio *gpio0;
  gpio0 = metal_gpio_get_device(0);
  (*(volatile uint32_t *)(0x1001200C)) &= ~(1 << TRIG_PIN);

  // Transit to the proper process.
  call_sep_process(pc, sp, ra);
}

/* --------- main function ---------------*/
int main() {
  int rc, i;
  struct metal_cpu *cpu;
  struct metal_interrupt *cpu_intr;
  struct metal_gpio *gpio0;


  /* Initialize interrupt handling on CPU 0 */

  printf("Monitor (rewriting PMP)\n");
  cpu = metal_cpu_get(metal_cpu_get_current_hartid());
  if(!cpu) {
    printf("Unable to get CPU 0 handle\n");
    return 1;
  }
  printf("CPU get OK.\n");

  cpu_intr = metal_cpu_interrupt_controller(cpu);
  if(!cpu_intr) {
    printf("Unable to get CPU 0 Interrupt handle\n");
    return 2;
  }
  metal_interrupt_init(cpu_intr);
  printf("interrupt_init OK.\n");

  /* Register a handler for the store access fault exception */
  rc = metal_cpu_exception_register(cpu, ECODE_ILLEGAL_INSTRUCTION, illegal_instruction_fault_handler);
  rc += metal_cpu_exception_register(cpu, ECODE_SYSCALL_FROM_U_MODE, syscall_from_u_handler);
  rc += metal_cpu_exception_register(cpu, ECODE_LOAD_ACCESS_FAULT, access_fault_handler);
  rc += metal_cpu_exception_register(cpu, ECODE_STORE_ACCESS_FAULT, access_fault_handler);
  if(rc < 0) {
    printf("Failed to register exception handler\n");
    return 3;
  }
  printf("exception handler OK.\n");

  /* Initialize GPIO for Fault Injection */
  gpio0 = metal_gpio_get_device(0);
  initGPIO(gpio0);


  /* Initialize PMPs */
  pmp = metal_pmp_get_device();
  if(!pmp) {
    printf("Unable to get PMP Device\n");
    return 4;
  }
  metal_pmp_init(pmp);
  printf("PMP init OK.\n");

  /* Create the register file for user mode execution */
  rc =  metal_pmp_set_region(pmp, 0, config_rom, napot_addr((size_t) FLASH_METAL, (size_t) ROM_METAL_SIZE));
  rc += metal_pmp_set_region(pmp, 1, config_ram, napot_addr((size_t) RAM_METAL, (size_t) RAM_METAL_SIZE));

  // initialize variables
  for (i = 0; i < sizeof(context); i++){
    context[i] = 0;
  }
  for (i = 0; i < sizeof(reg_stack); i++){
    reg_stack[i] = 0;
  }
  sep_entries[CTX_SEP1] = (uintptr_t)sep1_main;
  sep_entries[CTX_SEP2] = (uintptr_t)sep2_main;
  sep_entries[CTX_SEP3] = (uintptr_t)sep3_main;
  sep_sps[CTX_SEP1] = (uintptr_t)sep1_segment_stack_end;
  sep_sps[CTX_SEP2] = (uintptr_t)sep2_segment_stack_end;
  sep_sps[CTX_SEP3] = (uintptr_t)sep3_segment_stack_end;

  if(rc < 0) {
    printf("Failed to pmp set region\n");
    return 5;
  }

  set_pmp(CTX_SEP1);
  printf("Dropping privilege to User mode\n");
  context[CTX_SEP1] = 1;
  call_sep_process(sep_entries[CTX_SEP1], sep_sps[CTX_SEP1], (uintptr_t) 0);

  /* Execution should never return here */
  return 6;
}
