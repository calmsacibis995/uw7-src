# limits.s
#
# contains bit patterns for minimum and  maximum long double values

	.ident	"@(#)libc-i386:gen/limits.s	1.1"

	.section	.rodata
	.globl	__ldmin
	.align	4
__ldmin:
	.byte	0,0,0,0,0,0,0,0x80,0x1,0,0,0
        .byte	0x1,0,0,0,0,0,0,0,0,0,0,0
	.type	__ldmin,@object
	.size	__ldmin,24

	.globl	__ldmax
__ldmax:
	.byte	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x7f,0,0
	.type	__ldmax,@object
	.size	__ldmax,12
