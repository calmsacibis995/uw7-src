#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/ttymon.c	1.19.1.1"
#endif
/*		copyright	"%c%" 	*/
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <sys/stat.h>
#include <errno.h>
#include "uucp.h"
#include "error.h"
#include "ttymon.h"
#include <varargs.h>
#include <siginfo.h>
#include <wait.h>
#include <sys/secsys.h>

#include <Gizmos.h>

static void DoPrivilege(int action);

typedef struct device_type {
	String code;
	String translation;
} device_type;

device_type special_devices[] = {
			"tty00h", "tty00",	/* same as com1 */
			"tty00s", "tty00",	/* same as com1 */
			"00h", "tty00",	/* same as com1 */
			"00s", "tty00",	/* same as com1 */
			"00", "tty00",	/* same as com1 */
			"tty01h", "tty01",	/* same as com2 */
			"tty01s", "tty01",	/* same as com2 */
			"01", "tty01",	/* same as com2 */
			"01h", "tty01",	/* same as com2 */
			"01s", "tty01",	/* same as com2 */
			};

static void Check4SpecialDevices(char *);
static char version[BUFSIZ] = {'\0'};
Boolean CheckMousePort(char *port);
static Boolean Check4PortMonitor();
static pexec_t * popen_execvcmd();
static pexec_t * popen_execlcmd();
static Boolean EnableOrDisablePort();
static void Add_ttymon_PortMonitor();
static void Add_ttyService();
void Remove_ttyService(char *port);
Boolean Check_ttydefsSpeed();
char * Get_ttymonPortSpeed();
int Get_ttymonPortDirection();
static FILE * ForkWithPipe();


#define DIAL_OUT	"> /tmp/Dial.out 2> /tmp/Dial.out"
#define DIALOUT		"/tmp/Dial.out"
static Boolean first_time = True;

Update_ttymon(device)
DeviceData *device;
{

	int type;
	char *ptr;
	char speed[BUFSIZ];
	char longport[BUFSIZ];
	char port[BUFSIZ];
	int modem;
		/* for now monitor is always ttymon1 */
	if (first_time) {
		/* add the ttymon port monitor */
		/* if it exists you will get an error that you can
		ignore, otherwise it will add it */
		first_time = False;
			/* get the ttyadm version once and save */
		Get_ttyadmVersion();
			/* add the port monitor */
		if ((Check4PortMonitor()) == False) {
			 Add_ttymon_PortMonitor();
			}
	}
	type = 0;
	strcpy(speed, device->portSpeed);
	if (strcmp(speed, "Any") == 0) 
		strcpy(speed, "auto");
	if (strcmp(speed, "14400") == 0)
		strcpy(speed, "19200");
	if (strcmp(speed, "28800") == 0)
		strcpy(speed, "38400");

	if ((strcmp(device->portNumber, "com1")) == 0) {
			strcpy(longport, LONG_COM1);
			strcpy(port, COM1_SHORT_ALIAS);
	} else
	if ((strcmp(device->portNumber, "com2")) == 0) {
			strcpy(longport, LONG_COM2);
			strcpy(port, COM2_SHORT_ALIAS);
	} else {
		strcpy(port, device->portNumber);
		strcpy(longport, device->portNumber);
		ptr = (char *) IsolateName(device->portNumber);
		if (ptr) strcpy(port, ptr);
	}
#ifdef DEBUG
	fprintf(stderr,"Update_ttymon: ");
	fprintf(stderr,"portNumber=%s\n",device->portNumber);
	fprintf(stderr,"holdPortDirection=%s\n",device->holdPortDirection);
	fprintf(stderr,"portDirection=%s\n",device->portDirection);
	fprintf(stderr,"portSpeed=%s\n",device->portSpeed);
#endif

	
	if ((strcmp(device->portDirection, "outgoing")) == 0) {
#ifdef DEBUG
			fprintf(stderr,"type= outgoing\n");
#endif
		/* if device direction has not changed and is
			outgoing, then return since there is
			no need to do any pmadm commands on outgoing */

			if (device->holdPortDirection == NULL) return;
			if ((strcmp(device->portDirection, device->holdPortDirection)) == 0)
				return;
			type = OUTGOING;
			Remove_ttyService(port);
			return;
	} else
	if ((strcmp(device->portDirection, "incoming")) == 0) {
#ifdef DEBUG
			fprintf(stderr,"type= incoming\n");
#endif
			type =INCOMING;
	} else
	if ((strcmp(device->portDirection, "bidirectional")) == 0) {
#ifdef DEBUG
			fprintf(stderr,"type= bidirectional\n");
#endif
			type = BIDIRECTIONAL;
	} 

		/* remove old ttymon before adding new one, unless
			the port direction was outgoing */

	modem = 0;
		/* check that the modem is not datakit or some
		form of direct  setting */
	if ((device->modemFamily != NULL) &&
		(strcmp(device->modemFamily, "datakit") != 0) &&
		(strcmp(device->modemFamily, "direct") != 0) &&
		(strcmp(device->modemFamily, "uudirect") != 0)) {
			/* not datakit or direct so set modem to one for true */
			modem = 1;
	} 

	Add_ttyService(port, longport, speed, type, device->portEnabled, modem);
		/* if bidirectional or incoming then we can 
		enable or disable the port after it is added */
}

static void
Add_ttyService(port, longport, speed, type, enable, modem)
char *port;
char *longport;
char *speed;
int type;
char *enable;
int modem;
{

	pexec_t *pexecp;
	int len;
	char *ptr;
	char buf[BUFSIZ];
	Boolean result;

#ifdef DEBUG
fprintf(stderr,"Add_ttyService %s %s %s %d %s\n", port, longport, speed, type,enable);
#endif
	/*always do a remove before an add, if there is a ttymon on
		the port. If direction is outgoing then there is
		no ttymon service on the port */

	if ((Get_ttymonPortDirection(port)) != OUTGOING) {
			Remove_ttyService(port);
	}
	len = 0;
		
	switch (modem) {

	case 0:

		/* line is datakit or direct */

		if (type == BIDIRECTIONAL) {
			pexecp = popen_execlcmd("r", "/usr/sbin/ttyadm", 
				"ttyadm", "-b", "-h", "-r0", "-t", "60",
			 	"-d"  ,longport , "-s", "/usr/bin/shserv", "-l",
				speed, "-m", "ldterm,ttcompat", "-p", "login: ", NULL);
		} else  {
	
			pexecp = popen_execlcmd("r", "/usr/sbin/ttyadm", 
				"ttyadm", "-h", "-r0", "-t", "60",
			 	"-d" , longport , "-s", "/usr/bin/shserv", "-l",
				speed, "-m", "ldterm,ttcompat", "-p", "login: ", NULL);
			}
	
		break;
	case 1:
	default:
	
		/* modems & default case get the -o switch sent to ttyadm */
		if (type == BIDIRECTIONAL) {
			pexecp = popen_execlcmd("r", "/usr/sbin/ttyadm", 
				"ttyadm", "-b", "-h", "-r0", "-t", "60",
				"-o",
			 	"-d"  ,longport , "-s", "/usr/bin/shserv", "-l",
				speed, "-m", "ldterm,ttcompat", "-p", "login: ", NULL);
		} else  {
		
			pexecp = popen_execlcmd("r", "/usr/sbin/ttyadm", 
				"ttyadm", "-h", "-r0", "-t", "60",
				"-o",
			 	"-d" , longport , "-s", "/usr/bin/shserv", "-l",
				speed, "-m", "ldterm,ttcompat", "-p", "login: ", NULL);
			}
		break;
	
	
	}	
	(void)fgets(buf, BUFSIZ, pexecp->pe_fp);
#ifdef DEBUG
	fprintf(stderr,"port=%s version=%s buf: %s\n",port,version,buf);
#endif
	pclose_execcmd(pexecp);

	DoPrivilege(1);
        pexecp = popen_execlcmd("r", "/usr/sbin/pmadm", "pmadm",
                "-a", "-p", "ttymon1", "-s", port, "-v", version,
		"-fu", "-S", "login", "-m", buf, (char *) NULL);

	/* sometimes the enable or disable fails, especially
	if the system is slow, it might not be finished the
	add before enable or disable so delay here */
	sleep(1);
	if ((enable) && (strcmp(enable, "disabled") ==0)) {
			/* if they want the port disabled then
			call EnableOrDisablePort. No need to do this on
			enabled port siunce that is the default when
			adding a ttymon */
		if ((result = EnableOrDisablePort(port, enable)) == INVALID) {
			/* sometimes the enable or disable fails, especially
			if the system is slow, it might not be finished the
			add before the enable/disable request comes in so
			it might fail */
				sprintf(buf,  GGT(string_errorEnable),enable);
				DeviceNotifyUser(df->toplevel,buf);
		}
	}
	
	DoPrivilege(2);
}

Get_ttyadmVersion()
{
	pexec_t *pexecp;	/* pointer to structure returned form
				popen_execlcmd(), which contains an
				opn file pointer to a pipe from which
				the output of the ttyadm command is read.
				*/

        /* -get the ttymon port monitor version if it has not already
	**  been gotten.  code is here for a performance gain.	no need
	**  to get the version every time since it is safe to assume that
	**  the version will not change during the execution of this
	**  command.
	*/
        if (version[0] == '\0'){
		pexecp = popen_execlcmd("r", "/usr/sbin/ttyadm",
		"ttyadm", "-V", (char *)NULL);
		(void)fgets(version, BUFSIZ, pexecp->pe_fp);
		pclose_execcmd(pexecp);
	}

}



static Boolean
EnableOrDisablePort(port, value)
char *port;
char *value;
{
	pexec_t *pexecp;
	char *ptr;
	char buf[BUFSIZ];
	char port2[256];
	int result;
	char enable[3];

#ifdef DEBUG
	fprintf(stderr,"EnableOrDisablePort port=%s value=%s\n",port, value);
#endif
	if (strcmp(value, "enabled") == 0) {
		strcpy(enable, "-e");
	} else {
		strcpy(enable, "-d");
	}


#ifdef DEBUG
	fprintf(stderr,"EnableOrDisable: enable=%s port=%s\n",enable,port);
#endif
	sprintf(buf,"/usr/sbin/pmadm %s -p ttymon1 -s %s %s",
		enable, port, DIAL_OUT);
	result = system(buf);
	unlink(DIALOUT);
	if (!WIFEXITED(result)) {
#ifdef DEBUG
fprintf(stderr,"result of pmadm for %s = %d\n",enable, result);
#endif
		fprintf(stderr, GGT(string_errorEnable),value);
		return INVALID;
	}
	return VALID;
}

void
Remove_ttyService(port)
char *port;
{
	pexec_t *pexecp;
	char *ptr;
	Boolean result;
	char buf[BUFSIZ];
	char port2[256];

#ifdef DEBUG
	fprintf(stderr,"Remove_ttyService port=%s \n",port);
#endif
	if (strcmp(port, "com1") == 0) {
		strcpy(port2, COM1_SHORT_ALIAS);
	} else
	if (strcmp(port, "com2") == 0) {
		strcpy(port2, COM2_SHORT_ALIAS);
	} else {
		strcpy(port2, port);
		ptr = (char *)IsolateName(port2);
		if (ptr) strcpy(port2, ptr);
	}
	/* before we are allowed to remove a port, it must be enabled,
	so enable the port first */
	DoPrivilege(1);
	if ((result = EnableOrDisablePort(port2, "enabled")) == INVALID) {
		/* sometimes the enable or disable fails, especially
		if the system is slow, it might not be finished the
		add before the enable/disable request comes in so
		it might fail */
			sprintf(buf,  GGT(string_errorEnable),"enabled");
			fprintf(stderr,"%s\n",buf);
	}


	sprintf(buf,"/usr/sbin/pmadm -r -p ttymon1 -s %s %s", port2,DIAL_OUT);
	system(buf);	
	unlink(DIALOUT);
	DoPrivilege(2);

}

static Boolean
Check4PortMonitor()
{

	char buf[BUFSIZ];

	pexec_t *pexecp;
	DoPrivilege(1);
	pexecp = popen_execlcmd("r", "/usr/sbin/sacadm", "sacadm",	
		"-L", "-p", "ttymon1", NULL);
	
	DoPrivilege(2);
	(void)fgets(buf, BUFSIZ, pexecp->pe_fp);
	pclose_execcmd(pexecp);
	if ((strncmp(buf, "ttymon1", 7)) == 0) return True;
	return False;
}

static void
Add_ttymon_PortMonitor()
{

	char buf[BUFSIZ];

	DoPrivilege(1);
	sprintf(buf, "/usr/sbin/sacadm -a -p ttymon1 -t ttymon -c /usr/lib/saf/ttymon -v %s -n 3 %s", version, DIAL_OUT);
	system(buf);
	unlink(DIALOUT);
	DoPrivilege(2);
}


int
Get_ttymonPortDirection(portNumber)
char *portNumber;
{
	char *eargv[7];
	FILE *fp;
	int c, direction, len, count, i, pid;
	char *ptr, *token, *begin, *end;
	char port[256];
	char buf[BUFSIZ];

	if (first_time) {
		/* add the ttymon port monitor */
		/* if it exists you will get an error that you can
		ignore, otherwise it will add it */
		first_time = False;
			/* get the ttyadm version once and save */
		Get_ttyadmVersion();
			/* add the port monitor */
		if ((Check4PortMonitor()) == False) {
			 Add_ttymon_PortMonitor();
		}
	}

        if ((strcmp(portNumber, "com1")) == 0) {
                        strcpy(port, COM1_SHORT_ALIAS);
        } else
        if ((strcmp(portNumber, "com2")) == 0) {
                        strcpy(port, COM2_SHORT_ALIAS);
        } else {
                        /* look for last part of other port name */
		strcpy(port, portNumber);
		ptr = (char *) IsolateName(portNumber);
		if (ptr) strcpy(port, ptr);
        }
	eargv[0] = strdup("/usr/sbin/pmadm");
	eargv[1] = strdup( "-L");
	eargv[2] = strdup("-p");
	eargv[3] = strdup("ttymon1");
	eargv[4] = strdup ("-s");
	eargv[5] = strdup (port);
	eargv[6] = '\0';;
#ifdef DEBUG
	fprintf(stderr,"Get_ttymonPortDirection ttymon port monitor cmd=%s %s %s %s %s %s\n",eargv[0], eargv[1], eargv[2], eargv[3], eargv[4], eargv[5]);
#endif
    	if ((fp = ForkWithPipe(&pid, eargv[0], eargv )) == NULL) {
		return;
    	}
	direction = OUTGOING; /* set at outgoing only as default */
    	while (fgets(buf, BUFSIZ, fp) != NULL) {
		ptr = buf;
 		if ((token = strtok(buf, ":")) != NULL) {
			count = 1;
			while ((count < 9 ) && (token != NULL)) {
				token = strtok ((char *) NULL, ":"); 
				count++;
				} /* end while count */

			direction = INCOMING; 	/* got something back
						so it is not outgoing */
			begin = token;
				/* get the end of the flags */
			end = strtok ((char *) NULL, ":");
			if (end == NULL)  {
				len = 0;
			} else  {
		
				len = end - begin -1;
				for (i=0, ptr = begin; ptr < end; i++) {
					c = *ptr++;
					switch (c) {	

					case 'b':

						direction = BIDIRECTIONAL;
						break;
					default:
						break;
					} /* end switch */
				} /* end for */
	
			} /* end if end */
			

		} 	/* end if */
	
	}  /* end while fgets */
	for (i=0; i < 7; i++) free(eargv[i]);

	fclose (fp);
#ifdef DEBUG
	fprintf(stderr,"direction=%d\n",direction);
#endif
	return(direction);
}




static FILE *
ForkWithPipe(pid, cmd, eargv)
int *pid;
char *cmd;
char *eargv[];
{
	int fildes[2];
	int fd;
	char errbuf[512];
	FILE *fp;

	if (pipe(fildes) != 0) {
		sprintf(errbuf,GetGizmoText(err_open_pipe));
		perror(errbuf);
		return(NULL);
		/*exit(0);*/
	}
	switch ((*pid) = fork()) {
	case -1:
		{

			sprintf(errbuf,GetGizmoText(err_fork),cmd);
			perror(errbuf);
			return(NULL);
		}
	case 0:		/* We are the child */
		if (close(1) == -1) {
			sprintf(errbuf,GetGizmoText(err_close_stdout),cmd);
			return(NULL);
			/*exit(1);*/
		}
		if (dup(fildes[1]) == -1) {
			sprintf(errbuf,GetGizmoText(err_dup));
			return(NULL);
		}
		/* Close stderr on forked command */
		(void)close(2);
		if (execv(cmd, eargv) == -1) {
			sprintf(errbuf, GetGizmoText(err_exec),cmd);
			perror(errbuf);
			return(NULL);
			/*exit(0);*/
		}
		break;
	default:	/* We are the parent */
		close(fildes[1]);
		fd = fildes[0];

		if ((fp = fdopen(fd, "r")) == NULL) {
			char errbuf[512];
		
			sprintf(errbuf, GetGizmoText(err_fdopen_pipe), cmd);
			perror(errbuf);
		}
		return(fp);
	}
} /* end of ForkWithPipe */



static pexec_t *
popen_execlcmd(type, cmdpath, arg0, va_alist)
	const	char *type;
		char *cmdpath;
		char *arg0;
		va_dcl		/* no semicolon */
{
	va_list ap;		/* for variable parameters */
	char	**argv;		/* will contain the list of arguments for the
                                ** command
                                */
	pexec_t *pexecp;


	MKARGV(ap, arg0, argv, "popen_execlcmd");
	pexecp = popen_execvcmd(type, cmdpath, argv);
	(void)free((void **)argv);
	return (pexecp);
}


/*
 * Procedure:     popen_execvcmd
 *
 * Restrictions:
                 execv: None
*/
/*
** -popen_execvcmd(const char *type, char *cmdpath, char **argv)
**
**	-sets up a pipe, and forks and execs the specified command
**		-the parent process expects to read from or write to the pipe
**		 depending on the type of command specified.
**
**	-type: indicates whether the parent process is going to be reading
**	 from or writing to the pipe.
**	-cmdpath: the path name of the command to execute.
**	-argv: the argument list of the command to execute.
**
**	-a file pointer to the parent side of the pipe, the child pid, the type
**	 of popen_execcmd, and file descriptor related to the stdin or stdout
**	 of the parent before the pipe was set up are returned to the caller.
**	 this information is necessary restoring stdin or stdout to what it
**	 was before popen_execcmd was called.  this is done via the
**	 pclose_execcmd() function.
**	-if a failure occurs the command is exited.
**	-NOTE: a NULL pexec_t pointer is never returned
*/
static pexec_t *
popen_execvcmd(type, cmdpath, argv)
	const	char *type;
		char *cmdpath;
		char **argv;
{

	pexec_t	*pexecp;

	int	pipefds[2];	/* to save 2 file descriptors from a pipe  */
	int	savestdinfd;	/* save the file associated with stdin before
				** it is replaced by the pipe
				*/
	int	savestdoutfd;	/* save the file associated with stdout before
				** it is replaced by the pipe
				*/

	pid_t	cpid;		/* child process id returned from fork */


	/* -allocate a pexec_t and validate type
	*/
	if ((pexecp = (pexec_t *)calloc((size_t)1, sizeof (pexec_t))) ==
	 (pexec_t *)NULL){
		return;
	
	}
	if (!strcmp(type, "r")){
		pexecp->pe_type = PE_READ;
	}else if (!strcmp(type, "w")){
		pexecp->pe_type = PE_WRITE;
	}




	/* -set up the pipe
	** -save references to stdin and stdout for later restoration
	*/
	if (pipe(pipefds) != 0){
			return;
	}
	savestdinfd = dup(fileno(stdin));
	savestdoutfd = dup(fileno(stdout));
	if (savestdoutfd == -1){
		if (errno == EMFILE){
			return;
		}
	}
	(void)close(fileno(stdin));
	(void)dup(pipefds[0]);	/* stdin now "linked" to one side of pipe */
	(void)close(fileno(stdout));
	(void)dup(pipefds[1]);	/* stdout now "linked" to other side of pipe */
	(void)close(pipefds[0]);
	(void)close(pipefds[1]);


	/* -fork and exec command
	*/
	if ((cpid = fork()) == -1){
			return;

	}
	else if (cpid == 0){
		/* -THE CHILD
		** -close the "read" side of the pipe, since the child will
		**  be writing to the pipe, if type == PE_READ.
		**  re-establish the previous stdin if an open one existed.
		** -close the "write" side of the pipe, since the child will
		**  be reading from the pipe, if type == PE_WRITE.
		**  re-establish the previous stdout if an open one existed.
		** -disassociate processes controlling terminal to avoid
		**  signals
		** -exec the command
		*/
		if (pexecp->pe_type == PE_READ){
			(void)close(fileno(stdin));
			if (savestdinfd != -1){
				(void)dup(savestdinfd);
				(void)close(savestdinfd);
			}
		}else if (pexecp->pe_type == PE_WRITE){
			(void)close(fileno(stdout));
			if (savestdoutfd != -1){
				(void)dup(savestdoutfd);
				(void)close(savestdoutfd);
			}
		}
		(void)setsid();
		if (execv(cmdpath, argv) == -1){
			return;
		} 
	}else{
		/* -THE PARENT
		** -close the "write" side of the pipe, since the parent will
		**  be reading from the pipe, if type == PE_READ.
		**  re-establish the previous stdout if an open one existed.
		** -close the "read" side of the pipe, since the parent will
		**  be writing to the pipe, if type == PE_WRITE.
		**  re-establish the previous stdin if an open one existed.
		** -file out the pexec structure
		*/
		if (pexecp->pe_type == PE_READ){
			(void)close(fileno(stdout));
			if (savestdoutfd != -1){
				(void)dup(savestdoutfd);
				(void)close(savestdoutfd);
			}
			pexecp->pe_fp = stdin;
			pexecp->pe_savedfd = savestdinfd;
		}else if (pexecp->pe_type == PE_WRITE){
			(void)close(fileno(stdin));
			if (savestdinfd != -1){
				(void)dup(savestdinfd);
				(void)close(savestdinfd);
			}
			pexecp->pe_fp = stdout;
			pexecp->pe_savedfd = savestdoutfd;
		}
		pexecp->pe_cpid = cpid;
	}
	return (pexecp);
}



/*
** -pclose_execcmd(pexec_t *pexecp)
**
**	-closes the parent side of the pipe
**	-restores the previous open file associated with stdin or stdout
**	-waits for the child to die and returns its status
**
**	-returns -1 if something fails
**	-returns child exit status otherwise
*/
static int
pclose_execcmd(pexecp)
	pexec_t	*pexecp;
{
	siginfo_t	siginfo;	/* for childs exit status */

	(void)close(fileno(pexecp->pe_fp));
	if (pexecp->pe_savedfd != -1){
		if (dup(pexecp->pe_savedfd) == -1){
			 strerror(errno);
		}
		(void)close(pexecp->pe_savedfd);
	}

	if (waitid(P_PID, pexecp->pe_cpid, &siginfo, WEXITED) == -1){
		return (-1);
	}else{
		return(siginfo.si_status);
	}
}

static void
DoPrivilege(action)
int action;

{

	static Boolean	did_setuid=False;
	static uid_t	orig_uid, _loc_id_priv;

	switch (action) {

	case 1:


	/* Since sacadm initialization still requires
	 * using the root adminstrator uid, we are going to
	 * setuid() for this portion of code.
	 * If we luck out to be root from the start,
	 * make note not to do the restore by leaving
	 * did_setuid = FALSE.
	 * In an SUM (Super-User-Mode) system, we will
	 * be able to to setuid(non-root) without losing
	 * privilege since we assume we are aquiring
	 * privilege for this process via tfadmin, and
	 * therefore the privs are aquired via fixed
	 * privilege.
	 *
	 * Since privilege is required by the init scripts,
	 * we'll have to handle the error case of not being
	 * able to do the setuid().
	 */

	/* get root administrator user & current user id,
	 * censure -1's
	 */
	orig_uid = getuid();
	_loc_id_priv = secsys(ES_PRVID, 0);

	if ((-1 == orig_uid) || (-1 == _loc_id_priv)) {
		/* what besides?! ********/
		return;
	}
	if ((_loc_id_priv >= 0) &&
	    (orig_uid == _loc_id_priv)) {
		did_setuid = FALSE;
	} else {
		if ((setuid(_loc_id_priv)) < 0) {
			/* what besides?! ********/
			return;
		}
		did_setuid = TRUE;
	}

	break;


	case '2':
	default:
	if (did_setuid)
		if ((setuid(orig_uid)) < 0) {
			/* what besides?! ********/
			return;

	}
	break;

	}
} /* DoPrivilege */


Boolean 
CheckMousePort(char * portNumber)
{
	int i;
	Boolean match = False;
	FILE *fp;
	char *ptr;
	char port[256];
	int pid;
	char buf[BUFSIZ];
	char *eargv[3];

        if ((strcmp(portNumber, "com1")) == 0) {
                        strcpy(port, "tty00h");
        } else
        if ((strcmp(portNumber, "com2")) == 0) {
                        strcpy(port, "tty01h");
        } else {
			/* look for last part of other port name */
		strcpy(port, portNumber);
		ptr = (char *) IsolateName(portNumber);
		if (ptr) strcpy(port, ptr);
        }
	/* need to translate the other portnumber to tty equivalents */

	Check4SpecialDevices(&port[0]);	

    eargv[0] = strdup("/usr/bin/mouseadmin");
    eargv[1] = strdup( "-l");
    eargv[2] = '\0';

    	if ((fp = ForkWithPipe(&pid, eargv[0], eargv )) == NULL) {
		return;
		}
		match = False;
    	while (fgets(buf, BUFSIZ, fp) != NULL) {
			if ((ptr = strstr(buf, port)) != NULL){
					match = True;
					break;
			}
		
		}  /* end while fgets */

		fclose(fp);
		for (i=0; i < 3; i++) free(eargv[i]);
	return(match);
}


Boolean 
CheckPrinterPort(char * portNumber)
{
	int i;
	Boolean match = False;
	FILE *fp;
	char *ptr;
	char port[256];
	int pid;
	char buf[BUFSIZ];
	char *eargv[3];
	char *ptr2;

        if ((strcmp(portNumber, "com1")) == 0) {
                        strcpy(port, "tty00h");
        } else
        if ((strcmp(portNumber, "com2")) == 0) {
                        strcpy(port, "tty01h");
        } else {
			/* look for last part of other port name */
		strcpy(port, portNumber);
		ptr = (char *) IsolateName(portNumber);
		if (ptr) strcpy(port, ptr);
        }


	Check4SpecialDevices(&port[0]);
    eargv[0] = strdup("/usr/bin/lpstat");
    eargv[1] = strdup( "-v");
    eargv[2] = '\0';

    	if ((fp = ForkWithPipe(&pid, eargv[0], eargv )) == NULL) {
		return False;
		}
		match = False;
    	while (fgets(buf, BUFSIZ, fp) != NULL) {
			if ((ptr = strstr(buf, port)) != NULL){
					match = True;
					break;
			}
		
		}  /* end while fgets */

		fclose(fp);
		for (i=0; i < 3; i++) free(eargv[i]);
	return(match);
}



Boolean
Check_ttydefsSpeed(speed)
char *speed;
{
	char buf[BUFSIZ];
	char *eargv[5];
	FILE *fp;
	int pid,i;
	Boolean speed_ok;

	if (strcmp(speed, "Any") == 0) return GOOD_SPEED;
		/* return good speed if speed is set to Any */

	eargv[0] = strdup("/usr/bin/grep");
	eargv[1] = strdup(speed);
	eargv[2] = strdup("/etc/ttydefs");
	eargv[3] = '\0';
#ifdef DEBUG
	fprintf(stderr,"Check_ttydefsSpeed cmd=%s %s %s %s \n",eargv[0], eargv[1], eargv[2], eargv[3]);
#endif
	speed_ok = BAD_SPEED;
    	if ((fp = ForkWithPipe(&pid, eargv[0], eargv )) == NULL) {
		for (i=0; i < 3; i++) free(eargv[i]);
		fclose(fp);
		return speed_ok;
    	}
		
    	while (fgets(buf, BUFSIZ, fp) != NULL) {
 		if ((strtok(buf, ":")) != NULL) {
			/* speed found if something is returned */
			/* check that it matches our speed */
			if (strcmp(buf, speed ) == 0) {
			/* found a match on speed label */	
				speed_ok = GOOD_SPEED;
				break;
			}
			
		}
	}
	for (i=0; i < 3; i++) free(eargv[i]);
	fclose(fp);
#ifdef DEBUG
	fprintf(stderr,"speed_ok=%d\n",speed_ok);
#endif
	return(speed_ok);
}


char *
Get_ttymonPortSpeed(portNumber)
char *portNumber;
{
	char *eargv[7];
	FILE *fp;
	int c, speed, len, count, i, pid;
	char *ptr, *ptr2, *token, *begin, *end, *value;
	char port[256];
	char buf[BUFSIZ];
	value = NULL;

	if (first_time) {
		/* add the ttymon port monitor */
		/* if it exists you will get an error that you can
		ignore, otherwise it will add it */
		first_time = False;
			/* get the ttyadm version once and save */
		Get_ttyadmVersion();
			/* add the port monitor */
		if ((Check4PortMonitor()) == False) {
			 Add_ttymon_PortMonitor();
		}
	}

        if ((strcmp(portNumber, "com1")) == 0) {
                        strcpy(port, COM1_SHORT_ALIAS);
        } else
        if ((strcmp(portNumber, "com2")) == 0) {
                        strcpy(port, COM2_SHORT_ALIAS);
        } else {
                        /* look for last part of other port name */
		strcpy(port, portNumber);
		ptr = (char *) IsolateName(portNumber);
		if (ptr) strcpy(port, ptr);
        }
	eargv[0] = strdup("/usr/sbin/pmadm");
	eargv[1] = strdup( "-L");
	eargv[2] = strdup("-p");
	eargv[3] = strdup("ttymon1");
	eargv[4] = strdup ("-s");
	eargv[5] = strdup (port);
	eargv[6] = '\0';;
#ifdef DEBUG
	fprintf(stderr,"Get_ttymonPortDirection ttymon port monitor cmd=%s %s %s %s %s %s\n",eargv[0], eargv[1], eargv[2], eargv[3], eargv[4], eargv[5]);
#endif
    	if ((fp = ForkWithPipe(&pid, eargv[0], eargv )) == NULL) {
		return NULL;
    	}
    	while (fgets(buf, BUFSIZ, fp) != NULL) {
		ptr = buf;
 		if ((token = strtok(buf, ":")) != NULL) {
			count = 1;
			while ((count < 13 ) && (token != NULL)) {
				token = strtok ((char *) NULL, ":"); 
				count++;
				} /* end while count */

			begin = token;
				/* get the end of the flags */
			end = strtok ((char *) NULL, ":");
			if (end == NULL)  {
				value = NULL;
				goto getout;
			} else  {
				len = end - begin -1;
				/* allocate storage for speed value */
				value = malloc(len+1);
				if (!value) goto getout;
				for (ptr2= value, ptr = begin; ptr < end; ) {
					*ptr2++ = *ptr++;
				} /* end for */
	
			} /* end if end */
			

		} 	/* end if */
	
	}  /* end while fgets */
	getout:
	for (i=0; i < 7; i++) free(eargv[i]);

	fclose (fp);
#ifdef DEBUG
	fprintf(stderr,"speed=%s\n",value);
#endif
	
	return(value);
}

static void 
Check4SpecialDevices(port)
char *port;
{
	int i,j;

	if (port == NULL) return;
#ifdef DEBUG
fprintf(stderr,"port=%s\n",port);
#endif
	j = XtNumber(special_devices);
	for (i=0; i <j; i++) { 
		if (strcmp(port, special_devices[i].code) == 0) {
			strcpy(port,special_devices[i].translation);	
#ifdef DEBUG
			fprintf(stderr,"port=%s\n",port);
#endif
			break;
		}
	}

}
