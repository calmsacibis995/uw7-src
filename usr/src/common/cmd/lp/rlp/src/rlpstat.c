/*		copyright	"%c%" 	*/


#ident	"@(#)rlpstat.c	1.3"
#ident  "$Header$"

/*******************************************************************************
 *
 * rlpstat	-remote lpstat for UNIX and XENIX TCP/IP networks.
 *
 * FUNCTIONAL DESCRIPTION:
 *  -rlpstat is a program which allows the user to look at the queue
 *   of a remote printer.  The command is invoked with the name of the
 *   printer as it is known locally:
 *	
 *		rlpstat local_printer_name
 *
 *   rlpstat will find the machine on which the printer is physically
 *   connected and do an 'lpstat -oprinter_name' to show the queue on that
 *   machine for that printer.
 *
 * ASSUMPTIONS:
 *  -User equivalence between the lp accounts of the networked machines is
 *   assumed.
 *  -The documented format of /usr/spool/lp/remote is assumed.  Furthermore,
 *   the first option of the lp command to be executed on the remote machine
 *   should be the destination(-dprinter_name).
 *
 *      Created	    23 FEB 1991	    sco!hishami
 *			
 * Modification History
 *
 *  27-01-97 Paul Cunningham   Ported to UnixWare
 *
 *
 *******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>


#include <locale.h>
#include "rlp_msg.h"
nl_catd catd;

#define DPRINTF


#define TABLE	"/usr/spool/lp/remote"
#define UPATH 	"/usr/spool/lp/admins/lp/printers"
#define XPATH	"/usr/spool/lp/interface"
#define MAX_TO_GET 100	
#define TRUE 1
#define FALSE 0

/* For UnixWare we need to use 'rsh' instead of 'rcmd' (used in OpenServer)
 */
char	rcmd[]		= "/usr/bin/rsh"; 

char	lpstat[]	= "/usr/bin/lpstat";
char	rlpstat[]	= "/usr/bin/rlpstat"; 

extern char	*makepath();
extern char	*calloc();


/*******************************************************************************
 *
 * Discription: Main function for the 'rlpstat' command
 *
 *******************************************************************************
 */

main(argc, argv)
int argc;
char *argv[];

{
	FILE	*fp;
	char	remote_machine[10], rem_printer[10], *remote_printer;
	char    line[MAX_TO_GET], *device, *printer_file, *option, *ret;
	int 	local_printer_len, e, local = 0;
 


	setlocale( LC_ALL,"");
	catd = catopen( MF_RLP, MC_FLAGS);

	/* A quick usage message rather than calling a usage function. */
	if ( argc != 2)
	{
	  printf
	    (
	    MSGSTR
               (
	       RLPM_STAT_USAGE,
	       "Usage: rlpstat local_printer_name\n"
               )
	    );
	  exit(1);
	}

	/* find the length of the local printer's name. */
	local_printer_len = strlen(argv[1]);

	/* Is there a printer by this name configured locally? 
	 * Note that this does not really mean that the printer is physically 
	 * local to this machine.  If we can't find the printer, we bail out 
	 * assuming the printer does not exist.
	 */
	if ( !(is_printer_here( argv[1])))
	{
	  printf
	    (
	    MSGSTR
	       (
	       RLPM_STAT_NCONF,
	       "rlpstat: ERROR: Printer %s is not configured locally.\n"
	       ),
	    argv[1]
	    );

	  printf
	    (
	    MSGSTR
               (
	       RLPM_STAT_RSUBM,
	       "Please resubmit the command with a local printer name.\n"
	       )
	    );
	  exit(1);
	}

	/* The following covers the case in which the table isn't there.
	 * Assume the printer is physically local to this machine.  Simply
	 * invoke lpstat locally.
	 */
	if ( !(fp = fopen( TABLE, "r")))
	  local_lpstat( argv[1], local_printer_len);

	/* reaching here means the table exists and has been opened
	 */
	while ( (ret = fgets(line, MAX_TO_GET, fp)) != NULL)
	{
	  if( !strncmp( argv[1], line, local_printer_len))
	    if(line[local_printer_len] == ':')
	      break;
	}

	fclose(fp);

	/* If we couldn't find the printer in the TABLE, assume it is a local
	 * printer.  Do a local lpstat.
	 */
	if ( !ret) 
	{
	  local_lpstat( argv[1], local_printer_len);
	}

	/* The printer is remote, let's get the remote machine and printer names
	 * This assumes that each line of TABLE conforms to the standard format 
	 * specified in the doc:  
   * local_printer:  /usr/bin/rlpcmd remote_machine /usr/bin/lp -dremote_printer
	 * There could be more lp option  specified after the -d option.
	 */

	sscanf( line, "%*s %*s %s %*s %s", remote_machine, rem_printer);
	remote_printer = rem_printer + 2;   	/* advance past the -d	*/

	/* We now have the remote machine name and the remote printer name.
	 * Now we need to set our uid to that of user lp so we can do the rcmd.
	 */
	if( e = setlpid())
	{
	  exit(e);
	}

	/* We're ready...now we need to tell the remote machine to doi
	 * an 'rlpstat' on its printer. 
	 */
	DPRINTF( "Executing rlpstat on remote machine %s\n", remote_machine);
	execl( rcmd, rcmd, remote_machine, rlpstat, remote_printer, (char *)0);
	e = errno;
	perror(rcmd);

	exit(e);
	/* NOTREACHED */
}

/*******************************************************************************
 *
 * The following function checks whether or not the named printer is
 * configured locally.  It does so by checking whether it can open
 * a configuration file of the printer.   To make this program portable
 * between UNIX and XENIX, we make no assumptions about what OS
 * we're running on.  Printer configuration files are different between
 * UNIX and XENIX.  First we pretend we are on UNIX, if we can't open the
 * configuration file, we pretend we are on XENIX and try to open one of
 * it's configuration files.  If neither are openable, we assume the printer
 * does not exist.
 *
 *******************************************************************************
 */

int is_printer_here( printer)
char *printer;
{
	FILE	*ffp;
	char	*printer_file;

	/* check a unix printer configuration file
	 */
	printer_file = makepath( UPATH, printer, "configuration", (char *)0 );

	if ( !(ffp = fopen( printer_file, "r")))
	{
	  /* check a xenix printer configuration file
	   */
	  printer_file = makepath( XPATH, printer, (char*)0 );
	  if ( !(ffp = fopen( printer_file, "r")))
	  {
	    return(0);
	  }
	}

	fclose( ffp);
	return(1);
}

/*******************************************************************************
 *
 * The following function does a local 'lpstat -oprinter'
 *
 *******************************************************************************
 */

local_lpstat( printer, length)
char *printer;
int length;

{
 	char 	*option;

	/* prepare the option to be passed along to 'lpstat'.  We want to pass 
	 * it "-olocal_printer_name.
	 */
	if( !(option = (char*)calloc( length + 3, sizeof(char))))
 	{
	  perror("calloc");
	  exit(errno);
 	}

	strcpy( option, "-o");
	option = strcat( option, printer);
		
	DPRINTF( "Executing lpstat on local machine\n");
	execl( lpstat, lpstat, option, (char *)0); 
	perror( lpstat);
	exit( errno);
}



int setlpid()
{
	struct passwd *pwd;
	int u;

	if (( pwd = getpwnam( "lp")) == (struct passwd *)0) 
	{
	  (void) fputs
	           (
	           MSGSTR
	             (
	             RLPM_STAT_NUID,
	             "Cannot determine UID of user \"lp\"\n"
	             ),
	            stderr
	           );
	  return(1);
	  /* NOTREACHED */
	}

	u = pwd->pw_uid;
	endpwent();

	if (setuid(u)) 
	{
	  perror( "setuid");
	  return( errno);
	}

	return(0);
}


/*******************************************************************************
 *               End of Module
 *******************************************************************************
 */
