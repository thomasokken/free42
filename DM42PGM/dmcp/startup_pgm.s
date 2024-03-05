/**
*/

	.syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb


/* == Following section addresses are defined in ld script == */

/* = initialized data should be copied from flash to RAM before pgm start = */
/* .data section start addr in flash */
.word	_sidata
/* .data section start addr in RAM */
.word	_sdata
/* .data section end addr in RAM */
.word	_edata

/* = .bss section should be zeroed before pgm start = */
/* .bss section start addr in RAM */
.word	_sbss
/* .bss section end addr */
.word	_ebss

/** **/


/**
	Program entry point
*/

	.section	.text.Program_Entry
	.weak	Program_Entry
	.type	Program_Entry, %function
Program_Entry:

/* Copy the data segment initializers from flash to SRAM */
	ldr	r0, =_sidata
	ldr	r1, =_sdata
	ldr	r2, =_edata
	b CopyDataInit0

CopyDataInit:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
CopyDataInit0:
	cmp	r1, r2
	bcc	CopyDataInit

/* Zero fill the bss segment. */
	movs	r0, #0
	ldr	r1, =_sbss
	ldr	r2, = _ebss
	b	FillZerobss0

FillZerobss:
	str	r0, [r1], #4
FillZerobss0:
	cmp	r1, r2
	bcc	FillZerobss


/* Call static constructors */
	bl __libc_init_array
/* Call the application entry point.*/
	bl	program_main
	bl	post_main

/* Just for sure as post_main shouldn't return. */
LoopForever:
	b LoopForever

.size	Program_Entry, .-Program_Entry

