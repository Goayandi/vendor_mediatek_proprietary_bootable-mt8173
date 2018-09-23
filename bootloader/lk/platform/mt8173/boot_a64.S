#include <asm.h>

.text

FUNCTION(entry64)
	adr	r2, jump64
	ldr	r3, [r2]
	adr	r4, boot_a64
	str	r4, [r3]
	mrc	p15,0,r2,c12,c0,2
	orr	r2, r2, #1
	mcr	p15,0,r2,c12,c0,2
	orr	r2, r2, #2
	mcr	p15,0,r2,c12,c0,2
	dsb	sy
	isb	sy
	wfi
here:	b	here
jump64:
	.word	0x10200038

	.align 4
boot_a64:
	.incbin "platform/mt8173/boot-a64.bin"
	.align 4
