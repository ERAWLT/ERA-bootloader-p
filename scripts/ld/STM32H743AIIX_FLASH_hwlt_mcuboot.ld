/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                2048Kbytes FLASH and 1056Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2023 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */


/* Defines for calculate version region offset */
/* start of FLASH region. */
_firmware_flash_start = ORIGIN(FLASH);

/* offset of version section.  */
_hwlt_version_offset = _shwlt_versions - _firmware_flash_start;


/* Defines provide all the avalieble space of RAM regions to xHeapRegions of freertos heap 5 */
_ram_d1_heap_start = ORIGIN(RAM_D1_RTOS_HEAP);
_ram_d1_heap_len = LENGTH(RAM_D1_RTOS_HEAP);

_ram_d2_heap_start = ORIGIN(RAM_D2_RTOS_HEAP);
_ram_d2_heap_len = LENGTH(RAM_D2_RTOS_HEAP);

_ram_d3_heap_start = ORIGIN(RAM_D3_RTOS_HEAP);
_ram_d3_heap_len = LENGTH(RAM_D3_RTOS_HEAP);

_ram_dtc_heap_start = ORIGIN(DTCMRAM);
_ram_dtc_heap_len = LENGTH(DTCMRAM);

_ram_itc_heap_start = ORIGIN(ITCMRAM);
_ram_itc_heap_len = LENGTH(ITCMRAM);

/*                                     RAM_D1 layout (512K)
################################################################################################
#  RAM_D1_SHARED  #                            RAM_D1                      #  RAM_D1_RTOS_HEAP #
 ##############################################################################################
#  Shared region  # .data  #  .bss  # newlib #          MSP stack          #   FreeRTOS heap   #
#   between FW    #        #        #  heap  # Reserved by _Min_Stack_Size #                   #
################################################################################################
^-- RAM start      ^-- _end                      _estack--^                         RAM end --^
*/

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)             : ORIGIN = 0x08020400, LENGTH = 125K     /* 1К+1K+1K: header for a new key wrapper + header for image + both TLVs. Full: 128K.*/
  DTCMRAM (xrw)          : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1_SHARED (xrw)    : ORIGIN = 0x24000000, LENGTH = 230K     /* [SHARED SEC] */
  RAM_D1 (xrw)           : ORIGIN = 0x24039800, LENGTH = 42K      /* [DATA-BSS|HEAP-STACK] */
  RAM_D1_RTOS_HEAP (xrw) : ORIGIN = 0x24044000, LENGTH = 240K     /* [FREERTOS HEAP] */
  RAM_D2_RTOS_HEAP (xrw) : ORIGIN = 0x30000000, LENGTH = 288K     /* [FREERTOS HEAP] */
  RAM_D3_RTOS_HEAP (xrw) : ORIGIN = 0x38000000, LENGTH = 64K      /* [FREERTOS HEAP] */
  ITCMRAM (xrw)          : ORIGIN = 0x00000020, LENGTH = 64K-32
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* store version of current firmware  */
  .hwlt_versions :
  {
    . = ALIGN(4);
    _shwlt_versions = .;      /* start of hwlt_versions section */
    KEEP(*(.hwlt_versions))
    . = ALIGN(4);
    _ehwlt_versions = .;      /* end of hwlt_versions section */
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    _stext = .;        /* define a global symbols at start of code */
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    _srodata = .;      /* define a global symbols at start of rodata */
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
    _erodata = .;     /* define a global symbols at end of rodata */

  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  .sharedRamBlock (NOLOAD):
  {
      KEEP (*(.mailboxHandle))
      KEEP (*(.mailboxMemory))
  } >RAM_D1_SHARED

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    _sstack = .;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


