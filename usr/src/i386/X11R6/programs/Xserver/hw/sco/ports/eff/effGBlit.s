	.file	"effGBlit.s"
.text
	.align 4

/
/	@(#) effGBlit.s 11.1 97/10/22
/
/ Modification History
/
/ S001, 30-Aug-91, staceyc
/	fixed bug in queue check code - runs faster - doesn't hang
/	up chip when running text2 - this routine is now called from
/	poly glyph blit
/
/ S000, 28-Aug-91, staceyc
/ 	created - called for image glyphs - need to check that it's
/	okay to call from poly glyph
/

/
/ Coded in assembler, this routine speeds up x11perf:
/	-fitext		by 50 per cent
/	-tr10itext	by 30 per cent
/	-tr24itext	by 25 per cent
/

.globl effStretchGlyphBlit
effStretchGlyphBlit:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx

/ need a full 8 slots in the queue, makes cmp faster and smaller

	movl $0,%ebx
	movl $39656, %edx
.L2:
	inw (%dx)			/ why can't inw set the zero flag? :-)
	cmpb $0, %al
	jnz .L2

	movl 32(%ebp), %eax
	movl $61160, %edx
	outw (%dx)

	movl 8(%ebp),%eax
	movl $50920, %edx
	outw (%dx)

	movl 12(%ebp),%eax
	movl $49896, %edx
	outw (%dx)

	movl 16(%ebp),%eax
	movl $52968, %edx
	outw (%dx)

	movl 20(%ebp),%eax
	movl $51944, %edx
	outw (%dx)

	movl 24(%ebp), %eax
	movl $55016, %edx
	outw (%dx)

	movl 28(%ebp), %eax
	movl $65256, %edx
	outw (%dx)

	movl $49395, %eax
	movl $39656, %edx
	outw (%dx)

	leal -4(%ebp),%esp
	popl %ebx
	leave
	ret

