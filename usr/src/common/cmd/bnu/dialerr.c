#ident	"@(#)dialerr.c	1.2"
#ident	"$Header$"

#include "uucp.h"
#include <dial.h>

/*	This function maps the dial(3) return codes	*/
/*	to the Uerror codes and bnu exit codes.		*/

int
dialerr(ret)
int ret;
{
	int code;

	switch (ret) {
		case D_HUNG:	Uerror = SS_DIAL_FAILED;
				code = 49;
				break;
		case NO_ANS:	Uerror = SS_CHAT_FAILED;
				code = 50;
				break;
		case L_PROB:	Uerror = SS_DEVICE_FAILED;
				code = 51;
				break;
		case DV_NT_A:	Uerror = SS_NO_DEVICE;
				code = 52;
				break;
		case DV_W_TM:	Uerror = SS_TIME_WRONG;
				code = 55;
				break;
		case BAD_SYS:	Uerror = SS_BADSYSTEM;
				code = 53;
				break;
		default:	Uerror = SS_CS_PROB;
				code = 43;
				break;
	}

	return(code);
}
