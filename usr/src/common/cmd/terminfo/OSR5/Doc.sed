#ident	"@(#)terminfo:common/cmd/terminfo/OSR5/Doc.sed	1.1"
:
#	@(#) Doc.sed 23.2 91/06/12 
#
#	      UNIX is a registered trademark of AT&T
#		Portions Copyright 1976-1989 AT&T
#	Portions Copyright 1980-1989 Microsoft Corporation
#    Portions Copyright 1983-1991 The Santa Cruz Operation, Inc
#		      All Rights Reserved
#
#	This script is used to strip info from the terminfo
#	source files.
#
sed -n '
	/^# \{1,\}Manufacturer:[ 	]*\(.*\)/s//.M \1/p
	/^# \{1,\}Class:[ 	]*\(.*\)/s//.C \1/p
	/^# \{1,\}Author:[ 	]*\(.*\)/s//.A \1/p
	/^# \{1,\}Info:[ 	]*/,/^[^#][^	]/ {
		s/^# *Info:/.I/p
		/^#[	 ]\{1,\}/ {
			s/#//p
		}
		/^#$/ i\
.IE
	}
	/^\([^#	 ][^ 	]*\)|\([^|,]*\),[ 	]*$/ {
		s//Terminal:\
	"\2"\
		\1/
		s/|/, /g
		p
	}
' $*
