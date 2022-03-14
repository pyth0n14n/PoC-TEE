/* Copyright 2022 Shoei Nashimoto */
/* SPDX-License-Identifier: MIT/Apache-2.0 */

OUTPUT_ARCH("riscv")

ENTRY(_enter)

MEMORY
{
	flash_metal (rxai!w) : ORIGIN = 0x20400000, LENGTH = 128K
	flash_mon (rxai!w) : ORIGIN = 0x20420000, LENGTH = 64K
	flash_sep1 (rxai!w) : ORIGIN = 0x20430000, LENGTH = 64K
	flash_sep2 (rxai!w) : ORIGIN = 0x20440000, LENGTH = 64K
	flash_sep3 (rxai!w) : ORIGIN = 0x20450000, LENGTH = 64K
	itim (wx!rai) : ORIGIN = 0x8000000, LENGTH = 0x4000
	ram_metal (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 4K
	ram_mon (wxa!ri) : ORIGIN = 0x80001000, LENGTH = 4K
	ram_sep1 (wxa!ri) : ORIGIN = 0x80002000, LENGTH = 4K
	ram_sep2 (wxa!ri) : ORIGIN = 0x80003000, LENGTH = 2K
	ram_sep3 (wxa!ri) : ORIGIN = 0x80003800, LENGTH = 2K
}

PHDRS
{
	flash_metal PT_LOAD;
	flash_mon PT_LOAD;
	flash_sep1 PT_LOAD;
	flash_sep2 PT_LOAD;
	flash_sep3 PT_LOAD;
	ram_metal PT_LOAD;
	ram_mon PT_LOAD;
	ram_sep1 PT_LOAD;
	ram_sep2 PT_LOAD;
	ram_sep3 PT_LOAD;
	ram_init PT_LOAD;
	itim PT_LOAD;
	itim_init PT_LOAD;
}

SECTIONS
{

	__stack_size = DEFINED(__stack_size) ? __stack_size : 0x400;
	PROVIDE(__stack_size = __stack_size);
	__heap_size = DEFINED(__heap_size) ? __heap_size : 0x800;
	PROVIDE(__metal_boot_hart = 0);
	PROVIDE(__metal_chicken_bit = 0);

        /* ===== Flash ===== */

        /* --- metal --- */
	.init : {
		KEEP (*(.text.metal.init.enter))
		KEEP (*(.text.metal.init.*))
		KEEP (*(SORT_NONE(.init)))
		KEEP (*(.text.libgloss.start))
	} >flash_metal AT>flash_metal :flash_metal

	.fini : {
		KEEP (*(SORT_NONE(.fini)))
	} >flash_metal AT>flash_metal :flash_metal

	.text_metal : {
		*(.text.__metal* .text.metal*)
	} >flash_metal AT>flash_metal :flash_metal

	.rodata_metal : {
		*(.rdata.__metal* .rdata.metal*)
		*(.rodata.__metal* .rodata.metal*)
	} >flash_metal AT>flash_metal :flash_metal

        /* --- monitor --- */
	.text_mon : {
		*monitor.o(.text .text.*)
        } >flash_mon AT>flash_mon :flash_mon

	.rodata_mon : {
		*monitor.o(.rdata .rodata .rodata.*)
	} >flash_mon AT>flash_mon :flash_mon

        /* --- sep1 --- */
	.text_sep1 : {
		*main1.o(.text .text.*)
        } >flash_sep1 AT>flash_sep1 :flash_sep1

	.rodata_sep1 : {
		*main1.o(.rdata .rodata .rodata.*)
	} >flash_sep1 AT>flash_sep1 :flash_sep1

        /* --- sep2 --- */
	.text_sep2 : {
		*main2.o(.text .text.*)
		*aes.o(.text .text.*)
        } >flash_sep2 AT>flash_sep2 :flash_sep2

	.rodata_sep2 : {
		*main2.o(.rdata .rodata .rodata.*)
		*aes.o(.rdata .rodata .rodata.*)
	} >flash_sep2 AT>flash_sep2 :flash_sep2

        /* --- sep3 --- */
	.text_sep3 : {
		*main3.o(.text .text.*)
        } >flash_sep3 AT>flash_sep3 :flash_sep3

	.rodata_sep3 : {
		*main3.o(.rdata .rodata .rodata.*)
	} >flash_sep3 AT>flash_sep3 :flash_sep3

        
        /* --- metal --- */
	.text : {
		*(.text.unlikely .text.unlikely.*)
		*(.text.startup .text.startup.*)
		*(.text .text.*)
		*(.gnu.linkonce.t.*)
	} >flash_metal AT>flash_metal :flash_metal


	PROVIDE (__etext = .);
	PROVIDE (_etext = .);
	PROVIDE (etext = .);


	.rodata : {
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)
		. = ALIGN(8);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
	} >flash_metal AT>flash_metal :flash_metal


	. = ALIGN(8);

	.preinit_array : {
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >flash_metal AT>flash_metal :flash_metal

	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >flash_metal AT>flash_metal :flash_metal

	.fini_array : {
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >flash_metal AT>flash_metal :flash_metal

	.ctors : {
		KEEP (*crtbegin.o(.ctors))
		KEEP (*crtbegin?.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
	} >flash_metal AT>flash_metal :flash_metal

	.dtors : {
		KEEP (*crtbegin.o(.dtors))
		KEEP (*crtbegin?.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
	} >flash_metal AT>flash_metal :flash_metal



        /* ===== ITIM ===== */
	.itim : ALIGN(8) {
		*(.itim .itim.*)
	} >itim AT>flash_metal :itim_init

	PROVIDE( metal_segment_itim_source_start = LOADADDR(.itim) );
	PROVIDE( metal_segment_itim_target_start = ADDR(.itim) );
	PROVIDE( metal_segment_itim_target_end = ADDR(.itim) + SIZEOF(.itim) );

        /* ===== RAM ===== */

        /* === data === */
        /* --- metal --- */
        .data_metal_data : ALIGN(8) {
		*(.data.__metal* .data.metal*)
        } >ram_metal AT>flash_metal :ram_init

        /* --- monitor --- */
        .data_mon_data : ALIGN(8) {
		*monitor.o(.data .data.*)
        } >ram_mon AT>flash_mon :ram_init

        /* --- sep1 --- */
        .data_sep1_data : ALIGN(8) {
		*main1.o(.data .data.*)
        } >ram_sep1 AT>flash_sep1 :ram_init

        /* --- sep2 --- */
        .data_sep2_data : ALIGN(8) {
		*main2.o(.data .data.*)
		*aes.o(.data .data.*)
        } >ram_sep2 AT>flash_sep2 :ram_init

        /* --- sep3 --- */
        .data_sep3_data : ALIGN(8) {
		*main3.o(.data .data.*)
        } >ram_sep3 AT>flash_sep3 :ram_init

        /* --- metal --- */
	.data_data : {
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
	} >ram_metal AT>flash_metal :ram_init

        /* === sdata === */
        .data_metal_sdata : ALIGN(8) {
		*(.sdata.__metal* .sdata.metal*)
		*(.sdata2.__metal* .sdata2.metal*)
        } >ram_metal AT>flash_metal :ram_init

        /* --- monitor --- */
        .data_mon_sdata : ALIGN(8) {
		*monitor.o(.sdata .sdata.* .sdata2.*)
        } >ram_mon AT>flash_mon :ram_init

        /* --- sep1 --- */
        .data_sep1_sdata : ALIGN(8) {
		*main1.o(.sdata .sdata.* .sdata2.*)
        } >ram_sep1 AT>flash_sep1 :ram_init

        /* --- sep2 --- */
        .data_sep2_sdata : ALIGN(8) {
		*main2.o(.sdata .sdata.* .sdata2.*)
		*sep2.o(.sdata .sdata.* .sdata2.*)
        } >ram_sep2 AT>flash_sep2 :ram_init

        /* --- sep3 --- */
        .data_sep3_sdata : ALIGN(8) {
		*main3.o(.sdata .sdata.* .sdata3.*)
        } >ram_sep3 AT>flash_sep3 :ram_init

        /* --- metal --- */
	.data_sdata : {
		*(.sdata .sdata.* .sdata2.*)
		*(.gnu.linkonce.s.*)
		. = ALIGN(8);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
	} >ram_metal AT>flash_metal :ram_init

        /* === rodata === */
        .data_metal_rodata : ALIGN(8) {
		*(.rodata.__metal* .rodata.metal*)
        } >ram_metal AT>flash_metal :ram_init

        /* --- monitor --- */
        .data_mon_rodata : ALIGN(8) {
		*monitor.o(.rdata .rodata .rodata.*)
        } >ram_mon AT>flash_mon :ram_init

        /* --- sep1 --- */
        .data_sep1_rodata : ALIGN(8) {
		*main1.o(.rdata .rodata .rodata.*)
        } >ram_sep1 AT>flash_sep1 :ram_init

        /* --- sep2 --- */
        .data_sep2_rodata : ALIGN(8) {
		*main2.o(.rdata .rodata .rodata.*)
		*aes.o(.rdata .rodata .rodata.*)
        } >ram_sep2 AT>flash_sep2 :ram_init

        /* --- sep3 --- */
        .data_sep3_rodata : ALIGN(8) {
		*main3.o(.rdata .rodata .rodata.*)
        } >ram_sep3 AT>flash_sep3 :ram_init

        /* --- metal --- */
	.data_rodata : {
		*(.rdata .rodata .rodata.*)
		. = ALIGN(8);
        } >ram_metal AT>flash_metal :ram_init

	PROVIDE( metal_segment_data_source_start = LOADADDR(.data_metal_data) );
	PROVIDE( metal_segment_data_target_start = ADDR(.data_metal_data) );
	PROVIDE( metal_segment_data_target_end = ADDR(.data_metal_data) + SIZEOF(.data_metal_data) + SIZEOF(.data_data) + SIZEOF(.data_metal_sdata) + SIZEOF(.data_sdata) + SIZEOF(.data_metal_rodata) + SIZEOF(.data_rodata));

	PROVIDE( mon_segment_data_source_start = LOADADDR(.data_mon_data) );
	PROVIDE( mon_segment_data_target_start = ADDR(.data_mon_data) );
	PROVIDE( mon_segment_data_target_end = ADDR(.data_mon_data) + SIZEOF(.data_mon_data) + SIZEOF(.data_mon_sdata) + SIZEOF(.data_mon_rodata));

	PROVIDE( sep1_segment_data_source_start = LOADADDR(.data_sep1_data) );
	PROVIDE( sep1_segment_data_target_start = ADDR(.data_sep1_data) );
	PROVIDE( sep1_segment_data_target_end = ADDR(.data_sep1_data) + SIZEOF(.data_sep1_data) + SIZEOF(.data_sep1_sdata) + SIZEOF(.data_sep1_rodata));

	PROVIDE( sep2_segment_data_source_start = LOADADDR(.data_sep2_data) );
	PROVIDE( sep2_segment_data_target_start = ADDR(.data_sep2_data) );
	PROVIDE( sep2_segment_data_target_end = ADDR(.data_sep2_data) + SIZEOF(.data_sep2_data) + SIZEOF(.data_sep2_sdata) + SIZEOF(.data_sep2_rodata));

	PROVIDE( sep3_segment_data_source_start = LOADADDR(.data_sep3_data) );
	PROVIDE( sep3_segment_data_target_start = ADDR(.data_sep3_data) );
	PROVIDE( sep3_segment_data_target_end = ADDR(.data_sep3_data) + SIZEOF(.data_sep3_data) + SIZEOF(.data_sep3_sdata) + SIZEOF(.data_sep3_rodata));

        /* ===== bss ===== */

        /* --- monitor --- */
        .bss_mon : ALIGN(8) {
		*monitor.o(.sbss* .bss .bss.* COMMON)
        } >ram_mon AT>ram_mon :ram_mon

        /* --- sep1 --- */
        .bss_sep1 : ALIGN(8) {
		*main1.o(.sbss* .bss .bss.* COMMON)
        } >ram_sep1 AT>ram_sep1 :ram_sep1

        /* --- sep2 --- */
        .bss_sep2 : ALIGN(8) {
		*main2.o(.sbss* .bss .bss.* COMMON)
        } >ram_sep2 AT>ram_sep2 :ram_sep2

        /* --- sep3 --- */
        .bss_sep3 : ALIGN(8) {
		*main3.o(.sbss* .bss .bss.* COMMON)
        } >ram_sep1 AT>ram_sep1 :ram_sep1
        
        /* --- metal --- */
	.bss : ALIGN(8) {
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
	} >ram_metal AT>ram_metal :ram_metal
	PROVIDE( metal_segment_bss_target_start = ADDR(.bss) );
	PROVIDE( metal_segment_bss_target_end = ADDR(.bss) + SIZEOF(.bss) );


        /* ===== stack & heap ===== */
        /* --- metal --- */
        /* --- monitor --- */
	.stack_mon : {
		PROVIDE(metal_segment_stack_begin = .);
		. += __stack_size;
		PROVIDE( _sp = . );
		PROVIDE(metal_segment_stack_end = .);
	} >ram_mon AT>ram_mon :ram_mon

	.heap_mon : {
		PROVIDE( metal_segment_heap_target_start = . );
		. = __heap_size;
		PROVIDE( metal_segment_heap_target_end = . );
		PROVIDE( _heap_end = . );
	} >ram_mon AT>ram_mon :ram_mon

        /* --- sep1 --- */
	.stack_sep1 : ALIGN(16) {
		PROVIDE( __sep1_segment_stack_begin = . );
		. += __stack_size;
		PROVIDE( __sep1_segment_stack_end = . );
	} >ram_sep1 AT>ram_sep1 :ram_sep1

        /* --- sep2 --- */
	.stack_sep2 : ALIGN(16) {
		PROVIDE ( __sep2_segment_stack_begin = . );
		. += __stack_size;
		PROVIDE ( __sep2_segment_stack_end = . );
	} >ram_sep2 AT>ram_sep2 :ram_sep2

        /* --- sep3 --- */
	.stack_sep3 : ALIGN(16) {
		PROVIDE( __sep3_segment_stack_begin = . );
		. += __stack_size;
		PROVIDE( __sep3_segment_stack_end = . );
	} >ram_sep3 AT>ram_sep3 :ram_sep3

}

