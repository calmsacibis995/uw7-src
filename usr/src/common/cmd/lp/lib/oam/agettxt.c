/*		copyright	"%c%" 	*/

#ident	"@(#)agettxt.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include <string.h>
#include "oam.h"

char			**_oam_msg_base_	= 0;

char *
#if	defined(__STDC__)
agettxt (
	long			msg_id,
	char *			buf,
	int			buflen
)
#else
agettxt (msg_id, buf, buflen)
	long			msg_id;
	char			*buf;
	int			buflen;
#endif
{
	if (_oam_msg_base_)
		strncpy (buf, _oam_msg_base_[msg_id], buflen-1);
	else
		strncpy (buf, "No message defined--get help!", buflen-1);
	buf[buflen-1] = 0;
	return (buf);
}
