
/*
 * tcl_interface.c
 *
 * Routines to allow a tcl script to dynamically load us into the interpreter.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "tcl.h"
#include "libpnp.h"


/* Prototypes for the Tcl stubs */

static int PnPidStrCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

static int PnPidValCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

/*
static int PnPassignResCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
*/

int PnPConfigCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

/* Tcl initialization routine */
int Pnp_Init(Tcl_Interp *interp)
{
	/* Register our Tcl stub functions */

	Tcl_CreateCommand(interp, "PnPidStr", PnPidStrCmd,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Tcl_CreateCommand(interp, "PnPidVal", PnPidValCmd,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Tcl_CreateCommand(interp, "PnPConfig", PnPConfigCmd,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	return TCL_OK;
}


#define BAIL(S)	{ interp->result=S; return TCL_ERROR; }



/*
** usage: PnPidStr dec_value
*/
static int PnPidStrCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
	unsigned long numeric=0;
	const char *alpha;
	char *s;

	if(argc != 2)
		BAIL("expected 1 arg")

	for(s=argv[1]; *s; s++)
		{
		numeric*=10;	/* make room for next digit */

		if(!isdigit(*s))
			BAIL("expected numeric arg")

		numeric += *s - '0';
		}

	alpha=PnP_idStr(numeric);

	strncpy(interp->result, alpha, TCL_RESULT_SIZE);

	return TCL_OK;
}



/*
** usage: PnPidVal EISA_id_string
*/
static int PnPidValCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
	if(argc != 2)
		BAIL("expected 1 arg")

	/*
	** We trust sprintf not to give us more than 200 characters
	** from an unsigned long.  If it does, we have worse problems
	** than just overwriting a buffer.  :-)
	*/
	sprintf(interp->result, "%lu", PnP_idVal(argv[1]));
	return TCL_OK;
}

/*
** usage: PnPassignRes 
*
static int PnPassignResCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
	if(argc != 6)
		BAIL("expected 5 arg")

	sprintf(interp->result, "%i", PnP_AssignRes(argv[1], argv[2], argv[3], argv[4], argv[5]));
	return TCL_OK;
}
*/
