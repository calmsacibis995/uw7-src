#ident	"@(#)fur:i386/cmd/fur/epilogue.s	1.1"
pushf
pushl %eax
pushl %ebx
pushl %ecx
pushl %edx
call .next
.next:
call _epilogue
popl %edx
popl %edx
popl %ecx
popl %ebx
popl %eax
popf
