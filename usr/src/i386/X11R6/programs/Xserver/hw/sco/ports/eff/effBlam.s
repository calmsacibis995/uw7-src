/	@(#) effBlam.s 11.1 97/10/22
/
/ S000, 27-Aug-91, staceyc
/	created

	.file	"effBlam.s"
.text
	.align 4

/
/ Draw a solid rect based on x, y, lx, ly - assume all other registers
/ are set.
/
/ Calling this routine once instead of 5 seperate calls to outw speeds
/ up x11perf -rect1 -rect10 -dot et. al. about 30-50 per cent depending
/ on 8514 card speed and *86 processor speed .
/

.globl effBlamOutBox
effBlamOutBox:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx

/ setup queue check loop - 5 slots required
	movl $8,%ebx
	subl %eax, %eax
	movl $0xDAE8, %edx

/ loop until 5 spaces become available
.L2:
	inw (%dx)
	testw %bx,%ax
	jnz .L2

/ out x
	movl 8(%ebp),%eax
	movl $50920, %edx
	outw (%dx)

/ out y
	movl 12(%ebp),%eax
	movl $49896, %edx
	outw (%dx)

/ out lx
	movl 16(%ebp), %eax
	movl $55016, %edx
	outw (%dx)

/ out ly
	movl 20(%ebp), %eax
	movl $65256, %edx
	outw (%dx)

/ draw solid rect command
	movl $32947, %eax
	movl $39656, %edx
	outw (%dx)

	leal -4(%ebp),%esp
	popl %ebx
	leave
	ret
