/*		copyright	"%c%" 	*/

#ident	"@(#)failure.c	1.2"
#ident  "$Header$"

/*
 *  Send a message to the keymaster and wait for a reply
 */

#include "cr1.h"
#include "keymaster.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stropts.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/times.h>
#include <pfmt.h>

extern int errno;
extern char *scheme;

static char *message[] = {
	"",
	":5:Incorrect usage\n",
	":6:Usage: %s -l\n",
	":7:Usage: %s [-a | -c | -d] [-s scheme] [local_principal] remote_principal\n",
	":8:Usage: %s [-k] | [-cn] [-s scheme]\n",
	":9:Invalid %s principal.\n",
	":10:New key confirmation failed.\n",
	":11:Pipe %s() operation failed.\n",
	":12:Module (%s) push failed.\n",
	":13:Message %s() failed.\n",
	":13:Message %s() failed.\n",
	":14:XDR decode function (%s) failed.\n",
	":15:XDR encode function (%s) failed.\n",
	":16:Unexpected message received.\n",
	":17:Request rejected by daemon.\n",
	":18:Start failed. %s daemon already running.\n",
	":19:Stop failed. No %s daemon running.\n",
	":20:Cannot obtain lock file (%s).\n",
	":21:Cannot open key database - %s.\n",
	":22:Cannot stat key database - %s.\n",
	":23:Cannot read key database - %s.\n",
	":24:Error encountered in key database - %s.\n",
	":25:Cannot create updated key database - %s.\n",
	":26:Cannot rename key database. Left as %s.\n",
	":27:Memory allocation failed.\n",
	":28:Fork operation failed.\n",
	":29:Master key does not match.\n",
	":30:Cannot fattach() to %s.\n",
	":31:Error in protocol. Wrong %s.\n",
	":32:No key available to '%s'.\n",
	":33:No logname for effective user.\n",
	":34:namemap() failed for '%s'.\n",
	":35:attrmap() failed for '%s'.\n",
	":36:Wrong machine (%s) authenticated.\n",
	":37:Wrong logname (%s) authenticated.\n",
	":38:Wrong service (%s) authenticated.\n",
	":39:putava() failed for %s.\n",
	":40:setava() failed.\n",
	":41:Unexpected end of daemon.\n",
	":42:No local lid for %s.\n",
	":43:Unable to get key from user.\n",
	":44:Invalid encryption type (%0o) specified.\n",
	":45:Invalid checksum type (%0o) specified.\n",
	":46:Unknown failure code %d.\n"
	};

void
failure(enum etype code, void *value)
{

	switch (code) {

	case CR_EOK:
		break;

	case CR_CRUSAGE:
	case CR_CKUSAGE:
	case CR_KMUSAGE:
		pfmt(stderr, MM_ERROR, message[CR_USAGE]);
		pfmt(stderr, MM_ACTION, message[code], (char *)value);
		break; 	

	case CR_PRINCIPAL:
	case CR_PIPE:
	case CR_PUSH:
	case CR_MSGOUT:
	case CR_MSGIN:
	case CR_XDRIN:
	case CR_XDROUT:
	case CR_KMSTART:
	case CR_KMSTOP:
	case CR_KMLOCK:
	case CR_DBOPEN:
	case CR_DBSTAT:
	case CR_DBREAD:
	case CR_DBBAD:
	case CR_DBTEMP:
	case CR_DBLINK:
	case CR_FATTACH:
	case CR_PROTOCOL:
	case CR_NOKEY:
	case CR_NAMEMAP:
	case CR_ATTRMAP:
	case CR_XMACHINE:
	case CR_XLOGNAME:
	case CR_XSERVICE:
	case CR_PUTAVA:
	case CR_LVLIN:
		pfmt(stderr, MM_ERROR, message[code], (char *)value);
		break;

	case CR_CRYPT_TYPE:
	case CR_CKSUM_TYPE:
		pfmt(stderr, MM_ERROR, message[code], (int)value);
		break;

	case CR_USAGE:
	case CR_CONFIRM:
	case CR_BADREPLY:
	case CR_REJECT:
	case CR_MEMORY:
	case CR_FORK:
	case CR_MASTER:
	case CR_LOGNAME:
	case CR_SETAVA:
	case CR_END:
	case CR_INKEY:
		pfmt(stderr, MM_ERROR, message[code]);
		break;

	case CR_UNKNOWN:
	default:
		pfmt(stderr, MM_ERROR, message[CR_UNKNOWN], code);
		break;

	};

	exit(code);
}
