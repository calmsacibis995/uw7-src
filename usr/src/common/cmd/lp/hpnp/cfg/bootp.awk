#ident	"@(#)bootp.awk	1.2"
#		copyright	"%c%"

#
# (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
# (c)Copyright 1983 Regents of the University of California
# (c)Copyright 1988, 1989 by Carnegie Mellon University
# 
#                          RESTRICTED RIGHTS LEGEND
# Use, duplication, or disclosure by the U.S. Government is subject to
# restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
# Technical Data and Computer Software clause in DFARS 252.227-7013.
#
#                          Hewlett-Packard Company
#                          3000 Hanover Street
#                          Palo Alto, CA 94304 U.S.A.

/Tag #144/ {
	printf "  Configuration File:   "
	lbracket = index($0, "[");
	rbracket = index($0, "]");
	filename = substr($0, lbracket+1, (rbracket-lbracket)-1);
	n=split(filename, chars, ",");
	for(i=1; i<=n; i++)
	    printf "%c", chars[i]
	printf "\n"
	next
}

{
	print 
}
