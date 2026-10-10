OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
	ram   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
  .text :
  {
    PROVIDE(_text_start = .);
    *(.text.init)
    *(.text .text.*)
    PROVIDE(_text_end = .);
  } >ram

  .rodata :
  {
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram

  .data :
  {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*)
    *(.data .data.*)
    PROVIDE(_data_end = .);
  } >ram

  .bss :
  {
    . = ALIGN(8);
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(8);
    PROVIDE(_bss_end = .);
  } >ram

  .stack :
  {
    . = ALIGN(16);
    PROVIDE(_stack_start = .);
    . += 4K;
    PROVIDE(_stack_end = .);
    PROVIDE(__freertos_irq_stack_top = .);
  } >ram

  PROVIDE( _end = . );
  PROVIDE( end = . );
}