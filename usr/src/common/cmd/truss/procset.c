/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/procset.c	1.2.9.1"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procset.h>
#include <stdio.h>
#include <string.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"

#include <sys/procset.h>
#include <sys/wait.h>

/*
 * Function prototypes for static routines in this module.
 */
static const char * idop_enum( idop_t );


void
show_procset(process_t *Pr, ulong_t offset)
{
	procset_t procset;
	procset_t * psp = &procset;

	if (Pread(Pr, offset, psp, sizeof *psp) == sizeof *psp) {
		(void) printf("%s\top=%s",
			pname, idop_enum(psp->p_op));
		(void) printf("  ltyp=%s lid=%ld",
			idtypename(psp->p_lidtype), (long)psp->p_lid);
		(void) printf("  rtyp=%s rid=%ld\n",
			idtypename(psp->p_ridtype), (long)psp->p_rid);
	}
}

static const char *
idop_enum(idop_t arg)
{
	const char * str;

	switch (arg) {
	case POP_DIFF:	str = "POP_DIFF";	break;
	case POP_AND:	str = "POP_AND";	break;
	case POP_OR:	str = "POP_OR";		break;
	case POP_XOR:	str = "POP_XOR";	break;
	default:	(void) sprintf(code_buf, "%d", arg);
			str = (const char *)code_buf;
			break;
	}

	return str;
}

const char *
idtypename(int arg)
{
	const char * str;

	switch (arg) {
	case P_PID:	str = "P_PID";		break;
	case P_PPID:	str = "P_PPID";		break;
	case P_PGID:	str = "P_PGID";		break;
	case P_SID:	str = "P_SID";		break;
	case P_CID:	str = "P_CID";		break;
	case P_UID:	str = "P_UID";		break;
	case P_GID:	str = "P_GID";		break;
	case P_ALL:	str = "P_ALL";		break;
	case P_LWPID:	str = "P_LWPID";	break;
	default:	(void) sprintf(code_buf, "%d", arg);
			str = (const char *)code_buf;
			break;
	}

	return str;
}

const char *
woptions(int arg)
{
	char * str = code_buf;

	if (arg == 0)
		return "0";
	if (arg & ~(WEXITED|WTRAPPED|WUNTRACED|WCONTINUED|WNOHANG|WNOWAIT)) {
		(void) sprintf(str, "0%-6o", arg);
		return (const char *)str;
	}

	str[0] = str[1] = '\0';
	if (arg & WEXITED)
		(void) strcat(str, "|WEXITED");
	if (arg & WTRAPPED)
		(void) strcat(str, "|WTRAPPED");
	if (arg & WUNTRACED)
		(void) strcat(str, "|WUNTRACED");
	if (arg & WCONTINUED)
		(void) strcat(str, "|WCONTINUED");
	if (arg & WNOHANG)
		(void) strcat(str, "|WNOHANG");
	if (arg & WNOWAIT)
		(void) strcat(str, "|WNOWAIT");

	return str+1;
}

void
show_statloc(process_t *Pr, ulong_t offset)
{
	siginfo_t siginfo;

	if (Pread(Pr, offset, &siginfo, sizeof siginfo) == sizeof siginfo)
		(void) printf("%s\t%s, %s, pid=%d, status=0x%04x\n",
			      pname,
			      signame(siginfo.si_signo),
			      codename(siginfo.si_signo, siginfo.si_code),
			      siginfo.si_pid, siginfo.si_status);
}
