/*		copyright	"%c%" 	*/

#ident	"@(#)llib-llpoam.c	1.2"
#ident	"$Header$"
/*LINTLIBRARY*/

#include "oam.h"

/*	from file agettxt.c */

char * agettxt ( long msg_id, char *buf, int buflen)
{
    static char			* _returned_value;
    return _returned_value;
}

/*	from file fmtmsg.c */

/**
 ** fmtmsg()
 **/
int fmtmsg ( char *label, int severity, int seqnum,
	     long int arraynum, int logind, ...)
{
    static int			 _returned_value;
    return _returned_value;
}
