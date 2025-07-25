ENTRY(__entry)

base_address = 0x80000000;

PHDRS
{
  init   PT_LOAD FLAGS(5); /* read-only, executable*/
  text   PT_LOAD FLAGS(5); /* read-only, executable*/
}

SECTIONS
{
	/* bootloader は物理アドレスの基底アドレスを 0x80000 としてコンパイルされる */
	/* in machine/raspix/kernel/Makefile */
	/* 	 LOAD_ADDRESS  = 0x80000  */
	/*   ARCH_LD_FLAGS = --defsym=start_address=$(LOAD_ADDRESS) */
	.init start_address : AT(start_address) ALIGN(4096)
	{
    KEEP(*(.init .init.*))
		*/start*.o(.text .text.*)
		*/start*.o(.rodata .rodata.*)
		*/start*.o(.data .data.*)
		*/start*.o(.bss .bss.*)
	} : init
	. = . + 0x4000;
	_svc_start_stack = .;
	_init_end = ALIGN(4096);

	/* カーネルは仮想アドレスの基底アドレスを2MG(0x8000_0000)としてコンパイルされる */
	/*kernel will be compiled with virtual address base at 2GB*/
	PROVIDE(_kernel_start = base_address + _init_end);

	.text base_address + _init_end : AT(_init_end) ALIGN(4096)
	{
		KEEP(*(.text))
		*(.text)
	} : text
	. = ALIGN(4096);

	.data : { /** data segment */
		*(.data*)
	}
	. = ALIGN(4096);

	_bss_start = .;
	.bss :
	{
		*(.bss*)
	}
	. = ALIGN(4096);
	_bss_end = .;

	. = . + 0x4000;
	_irq_stack = .;

	. = . + 0x4000;
	_abt_stack = .;

	. = . + 0x4000;
	_undef_stack = .;

	. = . + 0x4000;
	_svc_stack = .;
	PROVIDE(_kernel_sp = .);

	. = ALIGN(4096);
	PROVIDE(_kernel_end = .);
}
