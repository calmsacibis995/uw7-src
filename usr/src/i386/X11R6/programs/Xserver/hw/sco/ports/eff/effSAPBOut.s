/
/	 @(#) effSAPBOut.s 11.1 97/10/22
/
/ Modification History
/
/ S000, 20-Sep-91, staceyc
/	created from effAPBOut.s - do AP drawing without using rep to
/	avoid Matrox WD chip bug in 1280 mode
/
	.file	"effSAPBOut.s"
	.text
        .globl effSlowAPBlockOutW
	.align 4	
effSlowAPBlockOutW:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%esi
	movl 12(%ebp),%edi
	leal 3(,%edi,2),%eax
	andl $-4,%eax
	subl %eax,%esp
	movl %esp,-4(%ebp)
	movl -4(%ebp),%ebx
	movl %edi,%ecx
	jmp .L19

/ The L17-L19 loop has been hand optimized, it converts a single byte
/ to the two byte value to be eventually stuffed into the 8514 data
/ port.
/
/ This single modification speeds up drawing mono images by 20-30
/ per-cent.  As such drawing characters, bitmaps, and stipples will
/ benefit.

/ esi points to next byte value
/ ebx points to next output short element
.L17:
	lodsb			/ load byte into al from input array
	shll $4, %eax		/ shift nibble to ah
	shrb $4, %al		/ move high al nibble to low al nibble
	addl %eax, %eax		/ fast shift left 1
	xchgb %ah, %al		/ swap the two nibbles
	movw %ax,(%ebx)		/ stash into output array of shorts
	addl $2, %ebx		/ bump output short pointer

.L19:
	decl %ecx
	cmpl $-1,%ecx
	jne .L17
	pushl %edi
	pushl -4(%ebp)
	call effSlowBlockOutW@PLT
	leal -16(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	leave
	ret
