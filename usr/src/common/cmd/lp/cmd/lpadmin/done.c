/*		copyright	"%c%" 	*/

#ident	"@(#)done.c	1.2"
#ident	"$Header$"

extern	void	exit();

#include "lp.h"
#include "msgs.h"

#include "lpadmin.h"

/**
 ** done() - CLEAN UP AND EXIT
 **/

void			done (rc)
	int			rc;
{
	(void)mclose ();
	exit (rc);
}
