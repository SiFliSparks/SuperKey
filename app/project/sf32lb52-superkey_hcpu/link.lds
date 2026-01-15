/******************************************************************************
 * @file     link.lds
 * @brief    GNU Linker Script for SF32LB52-SUPERKEY (支持ER_IROM1/2/3分区)
 * @note     主应用新地址: 0x12078000 (DFU在前面)
 ******************************************************************************/
#include "rtconfig.h"
#include "mem_map.h"

/*---------------------- Flash Configuration ----------------------------------*/
/* 使用 ptab 生成的 CODE_START_ADDR (现在是 0x12078000) */
__ROM_BASE = CODE_START_ADDR;
__ROM_SIZE = CODE_SIZE;

/*--------------------- Embedded RAM Configuration ----------------------------*/
__RAM_BASE = HPSYS_RAM0_BASE;
__RAM_SIZE = HCPU_RAM_DATA_SIZE;

/*--------------------- Stack / Heap Configuration ----------------------------*/
__STACK_SIZE = 0x00002000;
__HEAP_SIZE  = 0x00000C00;

__ROM_EX_BASE = HCPU_RO_DATA_START_ADDR;
__ROM_EX_SIZE = HCPU_RO_DATA_SIZE;

/* PSRAM 配置 */
__PSRAM_BASE = PSRAM_DATA_START_ADDR;
__PSRAM_SIZE = PSRAM_DATA_SIZE;

/* 字体数据区域 (ER_IROM2) */
__FONT_DATA_BASE = FONT_DATA_START_ADDR;
__FONT_DATA_SIZE = FONT_DATA_SIZE;

/* EZIP图像数据区域 (ER_IROM3) */
__EZIP_IMAGE_BASE = EZIP_IMAGE_DATA_START_ADDR;
__EZIP_IMAGE_SIZE = EZIP_IMAGE_DATA_SIZE;

MEMORY
{
    ROM (rx) : ORIGIN = __ROM_BASE, LENGTH = __ROM_SIZE
    RAM (rw) : ORIGIN = __RAM_BASE, LENGTH = __RAM_SIZE
    ROM_EX(rw): ORIGIN = __ROM_EX_BASE, LENGTH = __ROM_EX_SIZE
    PSRAM(rw): ORIGIN = __PSRAM_BASE, LENGTH = __PSRAM_SIZE
    ROM2(r): ORIGIN = __FONT_DATA_BASE, LENGTH = __FONT_DATA_SIZE
    ROM3(r): ORIGIN = __EZIP_IMAGE_BASE, LENGTH = __EZIP_IMAGE_SIZE
}

ENTRY(Reset_Handler)

SECTIONS
{
#ifndef FLASH_TABLE_ONLY    
  .vectors :
  {
    _stext = ABSOLUTE(.);  
    KEEP(*(.vectors));
    . = . + 4;
  } > ROM

  .stack :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > RAM
  PROVIDE(__stack = __StackTop);

  .heap :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > RAM

  .retm_data :
  {
    . = ALIGN(4);
    __rw_retm_data_start__ = .;
    * (.*l1_ret_text_*)
    * (.*l1_ret_rodata_*)

    *drv_spi_flash.o     (.text* .rodata*)
    *flash_table.o       (.text* .rodata*) 
    *bf0_hal_mpi.o       (.text* .rodata*)
    *bf0_hal_mpi_ex.o    (.text* .rodata*)
    *bf0_hal_mpi_psram.o (.text* .rodata*)
    *flash.o             (.text* .rodata*)
    *drv_psram.o         (.text* .rodata*)

    *context_gcc.o      (.text* .rodata*)
    *drv_common.o       (.text.HAL_GetTick)
    *bf0_hal_rcc.o      (.text* .rodata*)

    *bf0_pm.o           (.text.sifli_light_handler)
    *bf0_pm.o           (.text.sifli_deep_handler)
    *bf0_pm.o           (.text.sifli_standby_handler)
    *bf0_pm.o           (.text.SystemInitFromStandby)
    *.o                 (.text.SystemPowerOnModeGet)

    *bsp_init.o         (.text* .rodata*)
    *bsp_lcd_tp.o       (.text* .rodata*)
    *bsp_pinmux.o       (.text* .rodata*)
    *bsp_power.o        (.text* .rodata*)
    *bf0_hal_gpio.o     (.text* .rodata*)

    *bf0_hal_hpaon.o    (.text* .rodata*)
    *bf0_hal.o          (.text.HAL_Init)
    *.o                 (.text.HAL_Delay_us)
    *.o                 (.text.HAL_Delay_us_)
    *.o                 (.text.HAL_Delay_us2_)
    *.o                 (.text.HAL_MspInit)
    *.o                 (.text.HAL_Delay)
    *bf0_hal_pinmux.o   (.text* .rodata*)
    *bf0_pin_const.o    (.text* .rodata*)
    *drv_common.o       (.text.rt_hw_us_delay)
    *.o                 (.text.rt_memset)
    *rt_memclr*.o       (.text*)
    *memset*.o          (.text*)      

    *.o (.retm_data_*)

    . = ALIGN(4);
    __rw_retm_data_end__ = .;

  } > RAM AT > ROM

#endif /* !FLASH_TABLE_ONLY */

  .text :
  {
#ifdef FLASH_TABLE_ONLY
    KEEP(*ftab.o(.text* .rodata*)) 
#else
    *(.text*)
    *(.rodata*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* section information for finsh shell */
    . = ALIGN(4);
    __fsymtab_start = .;
    KEEP(*(FSymTab))
    __fsymtab_end = .;

    . = ALIGN(4);
    __vsymtab_start = .;
    KEEP(*(VSymTab))
    __vsymtab_end = .;

    . = ALIGN(4);
    LcdDriverDescTab_start = .;
    KEEP(*(LcdDriverDescTab))
    LcdDriverDescTab_end = .;
    
    . = ALIGN(4);
    __rt_utest_tc_tab_start = .;
    KEEP(*(UtestTcTab))
    __rt_utest_tc_tab_end = .;

    /* section information for initial. */
    . = ALIGN(4);
    __rt_init_start = .;
    KEEP(*(SORT(.rti_fn*)))
    __rt_init_end = .;

    . = ALIGN(4);
    BuiltinAppTab_start = .;
    KEEP(*(BuiltinAppTab))
    BuiltinAppTab_end = .;

    . = ALIGN(4);
    __app_font_start__ = .;
    KEEP(*(.app_font))
    __app_font_end__ = .;

    . = ALIGN(4);
    __SerialTranExport_start__ = .;
    KEEP(*(SerialTranExport))
    __SerialTranExport_end__ = .;

    . = ALIGN(4);
    __sifli_reg_start__ = .;
    KEEP(*(SORT(.sifli_reg*)))
    __sifli_reg_end__ = .;

    . = ALIGN(4);
    __bt_sifli_reg_start__ = .;
    KEEP(*(SORT(.bt_sifli_reg*)))
    __bt_sifli_reg_end__ = .;

    /* section information for modules */
    . = ALIGN(4);
    __rtmsymtab_start = .;
    KEEP(*(RTMSymTab))
    __rtmsymtab_end = .;

    KEEP(*(.eh_frame*))
    _etext = ABSOLUTE(.);
#endif
  } > ROM 

#ifndef FLASH_TABLE_ONLY      
  .rom_ex :
  {
  
    . = ALIGN(4);
    __rw_rom_ex_start__ = .;
    __ER_IROM1_EX$$RO_start__ = .;
    __ER_IROM1_EX$$RO_load_start__ = LOADADDR(.rom_ex);

    *(.l1_non_ret_text_*)
    *(.l1_non_ret_rodata_*)

    . = ALIGN(4);
    __rw_rom_ex_end__ = .;
    __ER_IROM1_EX$$RO_end__ = .;
    __ER_IROM1_EX$$RO_load_end__ = LOADADDR(.rom_ex) + SIZEOF(.rom_ex);
    
  } > ROM_EX AT > ROM

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > ROM

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > ROM
  __exidx_end = .;

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG (LOADADDR(.data))
    LONG (ADDR(.data))
    LONG (SIZEOF(.data))

    LONG (LOADADDR(.retm_data))
    LONG (ADDR(.retm_data))
    LONG (SIZEOF(.retm_data))
    
    LONG (LOADADDR(.rom_ex))
    LONG (ADDR(.rom_ex))
    LONG (SIZEOF(.rom_ex))    

    __copy_table_end__ = .;
  } > ROM
  

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    
    LONG (__bss_start__)
    LONG (__bss_end__ - __bss_start__)    

    LONG (ADDR(.retm_bss))
    LONG (SIZEOF(.retm_bss))

    /* PSRAM BSS sections */
    LONG (ADDR(.RW_PSRAM1))
    LONG (SIZEOF(.RW_PSRAM1))

    LONG (ADDR(.RW_PSRAM_NON_RET))
    LONG (SIZEOF(.RW_PSRAM_NON_RET))

    __zero_table_end__ = .;
  } > ROM

  .retm_bss :
  {
    . = ALIGN(4);
    __rw_retm_bss_start__ = .;
    __RW_IRAM_RET$$ZI_start__ = .;
    * (.bss.retm_bss_*)
 
    . = ALIGN(4);
    __RW_IRAM_RET$$ZI_end__ = .;
  } > RAM

  .RW_IRAM0 :
  {
    *(non_ret)
    *(.*l1_non_ret_data_*)
    *(.*l1_non_ret_bss_*)
  } > RAM 

  __etext = ALIGN (4);

  .data :
  {
    _sdata = ABSOLUTE(.);    
    __data_start__ = .;
    __RW_IRAM1_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)
    *(.l1_ret_data_*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    _sinit = ABSOLUTE(.);
    PROVIDE(__ctors_start__ = .);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.ctors.*)))
    KEEP(*(.ctors))
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    _einit = ABSOLUTE(.);
    PROVIDE_HIDDEN (__init_array_end = .);
    PROVIDE(__ctors_end__ = .);


    . = ALIGN(4);
    /* finit data */
    PROVIDE(__dtors_start__ = .);
    PROVIDE_HIDDEN (__fini_array_start = .);
    
    KEEP(*(SORT(.dtors.*)))
    KEEP(*(.dtors))

    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);
    PROVIDE(__dtors_end__ = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    __data_end__ = .;
    __RW_IRAM1_end__ = .;
    _edata = ABSOLUTE(.);    
  } > RAM AT > ROM

  __etext2 = ALIGN (4);

  /* ========================================================================
   * PSRAM 数据区域 - 存放大型缓冲区
   * ======================================================================== */
  .RW_PSRAM1 :
  {
    . = ALIGN(4);
    __rw_psram1_start__ = .;
    *(.*l2_ret_data_*)
    *(.*l2_ret_bss_*)
    *(.*l2_cache_ret_data_*)
    *(.*l2_cache_ret_bss_*)
    . = ALIGN(4);
    __rw_psram1_end__ = .;
  } > PSRAM

  .RW_PSRAM_NON_RET :
  {
    /* aligned to cache line size */
    . = ALIGN(32);
    __RW_PSRAM_NON_RET_start__ = .;
    *(.nand_cache)
    *(.*l2_non_ret_data_*)
    *(.*l2_non_ret_bss_*)
    *(.*l2_cache_non_ret_data_*)
    *(.*l2_cache_non_ret_bss_*)
    . = ALIGN(4);
    __RW_PSRAM_NON_RET_end__ = .;
  } > PSRAM

  .bss :
  {
    _sbss = ABSOLUTE(.);
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    *(.l1_ret_bss_*)   
    . = ALIGN(4);
    __bss_end__ = .;
    __bss_end = .;
    _ebss = ABSOLUTE(.);
    __end__ = .;
    PROVIDE(end = .);
  } > RAM AT > RAM

  /* ========================================================================
   * ROM3 区域 - EZIP图像数据 (ER_IROM3.bin)
   * ======================================================================== */
  .rom3 :
  {
    . = ALIGN(4);
    __ezip_data_start__ = .;
    
    /* EZIP图像头部和数据 */
    *(.ROM3_IMG_EZIP_HEADER*)
    *(.ROM3_IMG_EZIP*)
    
    . = ALIGN(4);
    __ezip_data_end__ = .;
  } > ROM3

  /* ========================================================================
   * ROM2 区域 - 字体数据 (ER_IROM2.bin)
   * ======================================================================== */
  .rom2 :
  {
    . = ALIGN(4);
    __font_data_start__ = .;
    
    /* 字体数据 */
    *(.font_data)
    *(.font_data.*)
    
    /* 直接指定字体文件 */
    *DroidSansFallback_font.o (.rodata*)
    
    . = ALIGN(4);
    __font_data_end__ = .;
  } > ROM2

#endif
}
