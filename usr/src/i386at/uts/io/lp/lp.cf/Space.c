#ident	"@(#)Space.c	1.2"
#ident	"$Header$"

#include <config.h>

int lp_select =  LP0SELECT +
		 (LP1SELECT << 1) +
		 (LP2SELECT << 2) +
		 (LP3SELECT << 3)
		;

