/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)global.c	1.4"
#ident	"$Header$"

#include "msgs.h"

char	*Lhost;		/* Local host name */
char	*Rhost;		/* Remote host name */
char	*Name;		/* Program name */
char	*Printer;	/* Printer name */
char	*Display_Person;/* Person doing an SCO extended displayq */
char	*Display_Host;	/* Host doing an SCO extended displayq */
char	 Msg[MSGMAX];	/* Message buffer */
int	Rhost_trusted;	/* remote host can cancel jobs */
