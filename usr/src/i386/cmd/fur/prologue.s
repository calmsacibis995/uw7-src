#ident	"@(#)fur:i386/cmd/fur/prologue.s	1.1"
call .next
.next:
call _prologue
popl %edx / pop off argument to _prologue
