#ident "@(#)source.h	1.2"
/*
 *	@(#)source.h	1.2 "@(#)source.h	1.2"
 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
#define	HSENSITIVITY	5
#define	VSENSITIVITY	5

#define	MAXSTRLEN	40

/* 40 spaces	"         1         2         3         4"	*/
#define	EMPTY	"                                        "

struct xlat {
	char	*item;
	char	buf[MAXSTRLEN];
	short	len;
	int	(*isthis)();
};

extern struct xlat xlat[];
