	.ident	"@(#)rtld:i386/rtbinder.s	1.4"
	.file	"rtbinder.s"

/ we got here because a call to a function resolved to
/ a procedure linkage table entry - that entry did a JMP
/ to the first PLT entry, which in turn did a JMP to _rtbinder
/
/ the stack at this point looks like:
/_______________________#  high addresses
/	arguments to	#  
/	foo (if any)	#
/_______________________#
/	return addr 	#
/	from foo	#
/_______________________#
/	offset of 	#
/	relocation	#
/	entry		#
/_______________________#
/	addr of link	# 
/	map entry	# <-%esp
/_______________________# low addresses 
/

/ we must leave stack looking as if foo had just been
/ called (i.e. with %esp pointing at return address from foo)
/
	.text
	.globl	_binder
	.globl	_rtbinder
	.type	_rtbinder,@function
	.align	4
_rtbinder:
	call	_binder@PLT	/ transfer control to rtld
				/ rtld returns address of 
				/ function definition
	addl	$8,%esp		/ fix stack
	jmp	*%eax		/ transfer to the function
