	.ident	"@(#)rtld:i386/rtboot.s	1.19"
	.file	"rtboot.s"

/ Bootstrap routine for run-time linker.
/ 2 versions of the bootstrapper are provided.
/ The first is for running the Gemini rtld on OSR5 in
/ compatibility mode.  In this case, we get control
/ from the OSR5 dynamic linker.  We are passed the
/ base address and memory size of the original (OSR5)
/ dynamic linker and must unmap it.  We then must
/ pass control to the Gemini dynamic linker.
/ In the second case, we get control from exec which
/ has loaded our text and data into the process' address 
/ space and created the process stack
/ In either case, we must calculate the address of the
/ dynamic section of rtld and of argc, and pass both to _rt_setup.
/ _rt_setup then calls _rtld - on return we jump to the entry
/ point for the a.out.
/
/ When we get control from exec, the process stack looks like
/ this:
/
/_______________________#  high addresses
/	strings		#  
/_______________________#
/	0 word		#
/_______________________#
/	Auxiliary	#
/	entries		#
/	...		#
/	(size varies)	#
/_______________________#
/	0 word		#
/_______________________#
/	Environment	#
/	pointers	#
/	...		#
/	(one word each)	#
/_______________________#
/	0 word		#
/_______________________#
/	Argument	# low addresses
/	pointers	#
/	Argc words	#
/_______________________#
/	argc		# 
/_______________________# <- %esp

/ When we get control from the OSR5 rtld, the stack looks the
/ same until argc; from argc on it looks like this:
/_______________________#
/	argc		# 
/_______________________# 
/	old rtld size	#
/_______________________#
/	old rtld addr	#
/_______________________#
/	return addr from#
/	orig. _rt_boot	# <- %esp
/_______________________#
/

_m4_ifdef_(`GEMINI_ON_OSR5',`
	.section	.rodata
	.align	4
.M1:
	.ascii	"dynamic linker: munmap failed\n"
	.set	.err_len,.-.M1
',`')
	.text
	.globl	_rt_boot
	.globl	_rt_setup
	.globl	_GLOBAL_OFFSET_TABLE_
	.type	_rt_boot,@function
	.align	16
_m4_ifdef_(`GEMINI_ON_OSR5',`
	/ alternate compatibility version
_rt_boot:
	pushl	$0		/ clear 2 words for a fake stack 
	pushl	$0 		/ frame for debuggers
	movl	%esp,%ebp
	call	.L1
.L1:
	popl	%ebx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.L1],%ebx
	pushl	16(%esp)	/ olen
	pushl	16(%esp)	/ obase
	call	_rtmunmap@PLT	/ munmap(obase, olen)
	addl	$8,%esp
	cmpl	$-1,%eax
	je	.unmap_error
	leal	20(%esp),%eax	/ &argc
	movl	_DYNAMIC@GOT(%ebx),%edx
	pushl	%eax
	pushl	%edx
	call	_rt_setup@PLT
	addl	$28,%esp
	movl	_rt_do_exit@GOT(%ebx),%edx
	jmp	*%eax 		/ transfer control to a.out
	/
	/ unmap failed - print an error message and kill ourselves
.unmap_error:
	leal	.M1@GOTOFF(%ebx),%eax	/ address of message
	movl	$.err_len,%ecx	/ length of string
	pushl	%ecx		/ length of the string - NULL byte
	pushl	%eax		/ address of the string
	pushl	2		/ stderr
	call	_rtwrite@PLT	/ write(fd, str, length)
				/ fall through on error 
				/ (what else can we do?)
	call	_rtgetpid@PLT
	pushl	$9
	pushl	%eax
	call	_rtkill@PLT		/ kill(pid, SIGKILL)
	.size	_rt_boot,.-_rt_boot
	.align	4
	.text
',`
	/ native version
_rt_boot:
	movl	%esp,%eax	/ save address of argc
	pushl	$0		/ clear 2 words for a fake stack 
	pushl	$0 		/ frame for debuggers
	movl	%esp,%ebp
	pushl	%eax		/ push &argc
	call	.L1		/ only way to get IP into a register
.L1:
	popl	%ebx		/ pop the IP we just "pushed"
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.L1],%ebx
	pushl	_DYNAMIC@GOT(%ebx)	/ address of dynamic structure
	call	_rt_setup@PLT	/ _rt_setup(_DYNAMIC, &argc)
	addl	$16,%esp
_m4_ifdef_(`GEMINI_ON_UW2',`
	/ for UnixWare2, we do not do compatibility checks
	movl	_rt_do_exit@GOT(%ebx),%edx
	jmp	*%eax 		/ transfer control to a.out
',`
	cmpl	$0,%eax		/ OSR5 compatibility - map alternate
				/ dynamic linker
	je	.alternate_rtld
	movl	_rt_do_exit@GOT(%ebx),%edx
	jmp	*%eax 		/ transfer control to a.out

.alternate_rtld:
				/ alternate dynamic linker mapped
				/ call its entry point, passing
				/ base address and size of old rtld
	movl	_rt_boot_info@GOT(%ebx),%eax
	pushl	8(%eax)		/ old size
	pushl	4(%eax)		/ old base addr
	movl	(%eax),%edx	/ alternate entry addr
	call	*%edx
				/ never returns
')
	.size	_rt_boot,.-_rt_boot
	.align	4
	.text
')
