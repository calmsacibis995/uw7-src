#ident	"@(#)devintf:common/cmd/devintf/groups/mbrship/list/maxcol.awk	1.1.4.1"
#ident  "$Header$"

BEGIN {
	maxcol = 0
}

maxcol < length { maxcol = length }

END {print maxcol}
