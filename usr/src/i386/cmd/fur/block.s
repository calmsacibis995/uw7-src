#ident	"@(#)fur:i386/cmd/fur/block.s	1.1.1.1"
pushf
pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
pushl $block_number
call __FILENAME__blocklog
popl %edx
popl %edx
popl %ecx
popl %ebx
popl %eax
popf
