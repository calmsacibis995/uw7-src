#ident	"@(#)fur:i386/cmd/fur/prof.s	1.1.1.1"
pushf
pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
movl $function_number,%edx
leal __FILENAME__ProfCount(,%edx,4),%edx
call _mcount
popl %edx
popl %ecx
popl %ebx
popl %eax
popf
