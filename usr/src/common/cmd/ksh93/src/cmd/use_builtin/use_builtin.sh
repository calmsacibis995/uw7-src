#!/u95/bin/sh
#ident	"@(#)ksh93:src/cmd/use_builtin/use_builtin.sh	1.1"
`/usr/bin/sed -e 's!.*/!!' <<!
/$0
! ` "$@"
