/*		copyright	"%c%" 	*/

#ident	"@(#)scheme.c	1.2"
#ident  "$Header$"

/*  IAF Challenge-Response Scheme #1  */

#include <pwd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <unistd.h>
#include <pfmt.h>
#include <locale.h>

#include <ia.h>
#include <mac.h>
#include <iaf.h>
#include <crypt.h>
#include <cr1.h>
#include "cr1.h"
#include "keymaster.h"
#include "scheme.h"

#include <sys/types.h>
#include <termio.h>

static Principal LID		= "LID=";
static Principal LOGNAME	= "LOGNAME=";
static Principal SCHEME		= "SCHEME=";
static Principal RLID		= "RLID=";

static Principal RLOGNAME	= "RLOGNAME=";
static Principal RMACHINE	= "RMACHINE=";
static Principal RSERVICE	= "RSERVICE=";

static Principal RL		= "RL=";
static Principal RS		= "RS=";
static Principal RU		= "RU=";

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;
extern int t_errno;

extern int tflags;

       Principal Local = "";	/* Local principal name */
       Principal Expected = "";	/* Expected remote principal name */
       Principal Actual = "";	/* Authenticated remote principal name */
static Principal Logname;	/* mapped local logname */
static Principal Lid = "";	/* mapped local lid */

static Nonce mynonce;		/* Nonce identifiers for each side */

extern char mbuf[MLEN];		/* Message buffer */
extern char cbuf[CLEN];		/* encryption buffer */

extern FILE *logfp;

	long mypid;
	int role = 'i';		/* default role is IMPOSER */
	int x_type;		/* encryption algorithm to use */
static char *program = NULL;
FILE *logfp;
FILE *openlog();

static char *loc_mach = NULL;	/* local machine name */
static char *loc_svc  = NULL;	/* local service name */
static char *loc_user = NULL;	/* local user logname */
static char *loc_lid  = NULL;	/* local LID */
static char *rem_mach = NULL;	/* remote machine name */
static char *rem_svc  = NULL;	/* remote service name */
static char *rem_user = NULL;	/* remote user logname */
static char **data;		/* assertion list pointer */

static Pmessage msg1;	/* protocol message buffer #1 */
static Pmessage msg2;	/* protocol message buffer #2 */
static Pmessage msg3;	/* protocol message buffer #3 */

static char *actual_mach = NULL, *actual_user = NULL;

/*  The imposer  */

static int
impose()
{

	/*  Receive first message  */

	if (rd_msg(&msg1) != 0)
		failure(CR_MSGIN, "1");

	/*  Set up values for message #2 */

	strcpy(msg2.principal, msg1.principal);	/* echo their identity */
	msg2.nonce1 = msg1.nonce1;		/* echo their nonce */
	msg2.nonce2 = mynonce;			/* include my own nonce */
	msg2.data[0] = '\0';			/* initialize data string */

	if (loc_lid) {
		(void) strcat(msg2.data, "RL=");
		(void) strcat(msg2.data, loc_lid);
		(void) strcat(msg2.data, " ");
	}

	if (loc_user) {
		(void) strcat(msg2.data, "RU=");
		(void) strcat(msg2.data, loc_user);
		(void) strcat(msg2.data, " ");
	}

	if (loc_svc) {
		(void) strcat(msg2.data, "RS=");
		(void) strcat(msg2.data, loc_svc);
		(void) strcat(msg2.data, " ");
	}

	msg2.size = strlen(msg2.data) + 1;

	/* Send them message #2, and get message #3 */

	if (wr_msg(&msg2) != 0)
		failure(CR_MSGOUT, "2");

	if (rd_msg(&msg3) != 0)
		failure(CR_MSGIN, "3");

	/* Make sure message #3 is what we expected */

	if (strcmp(msg1.principal, msg3.principal) != 0) {
		LOG("Identity of message 3 (%s) doesn't match message 1.\n",
			msg3.principal);
		failure(CR_PROTOCOL, gettxt(":57", "principal switch"));
	} 

	if ( (msg3.nonce2.time != mynonce.time)
		|| (msg3.nonce2.pid != mynonce.pid) )
		failure(CR_PROTOCOL, "nonce");
	
	(void) strcpy(Actual, msg3.principal);

	data = strtoargv(msg1.data);

	DLOG("Imposer side of cr1 protocol succeeded.\n", "");

	return(0);
}

static int
respond()
{
	/* Set up the first message */

	/* It there is an assertion, try machine keys first */

	if (loc_lid || loc_user) {
		(void) sprintf(msg1.principal, "@%s",  loc_mach);
		(void) sprintf(Actual, "@%s", rem_mach);
	} else {
		(void) strcpy(msg1.principal, Local);
		(void) strcpy(Actual, Expected);
	}

	msg1.nonce1 = mynonce;

	/* Set up RL= and RU= strings */

	msg1.data[0] = '\0';

	if (loc_lid) {
		(void) strcat(msg1.data, "RL=");
		(void) strcat(msg1.data, loc_lid);
		(void) strcat(msg1.data, " ");
	}

	if (loc_user) {
		(void) strcat(msg1.data, "RU=");
		(void) strcat(msg1.data, loc_user);
		(void) strcat(msg1.data, " ");
	}


	if (loc_svc) {
		(void) strcat(msg1.data, "RS=");
		(void) strcat(msg1.data, loc_svc);
		(void) strcat(msg1.data, " ");
	}

	msg1.size = strlen(msg1.data) + 1;

	/* Send the first and get reply */

	if (wr_msg(&msg1) != 0)
		failure(CR_MSGOUT, "1");

	if (rd_msg(&msg2) != 0)
		failure(CR_MSGIN, "2");

	/*  Validate the second message  */

	if (msg2.type != TYPE2) 
		failure(CR_PROTOCOL, gettxt(":55", "type"));

	if ((msg2.nonce1.time != mynonce.time)
		|| (msg2.nonce1.pid  != mynonce.pid)) {
		failure(CR_PROTOCOL, "nonce");
	}

	/*  Construct third message  */

	strcpy(msg3.principal, msg1.principal);
	msg3.nonce2 = msg2.nonce2;	/* send their nonce back */
	msg3.data[0] = '\0';
	msg3.size = 0;

	if (wr_msg(&msg3) != 0)
		failure(CR_MSGOUT, "3");

	data = strtoargv(msg2.data);

	DLOG("Responder side of cr1 protocol succeeded.\n", "");

	return(0);
}

static void
set_nonce(Nonce *nonce)
{
	time(&nonce->time);
	nonce->pid = getpid();	/*  In case of coarse clock  */

	return;
}

static void
do_protocol(int role)
{
	struct passwd *entry;	/* passwd file entry */
	struct utsname sysinfo; /*  System information	*/
	extern void set_flag();

	struct termio orig_settings, cr1_settings;
	struct termio *settings = NULL;

	set_nonce(&mynonce);

	/* if we have a line discipline, set line into raw mode */
	/* and save original settings to put them back later on */

	if ( ioctl(FDIN, TCGETA, &orig_settings) == 0 ) {
		int i;

		cr1_settings.c_iflag = 0;
		cr1_settings.c_oflag = 0;
		cr1_settings.c_cflag = (orig_settings.c_cflag & CBAUD) | CS8 | CREAD | HUPCL;
		cr1_settings.c_lflag = 0;
		for (i=0; i<NCC; i++)
			cr1_settings.c_cc[i] = _POSIX_VDISABLE;
		cr1_settings.c_cc[VMIN] = MLEN;		/* max message length */
		cr1_settings.c_cc[VTIME] = 100;		/* 10 sec */
		if ( ioctl(FDIN, TCSETA, &cr1_settings) == 0 )
			settings = &orig_settings;
	}

	msg1.type = TYPE1;
	msg2.type = TYPE2;
	msg3.type = TYPE3;

	set_flag(role);		/* set read()/write() vs t_snd()/t_rcv() flag */

	/* Set up Local principal name */

	if ((entry = getpwuid(geteuid())) == NULL)
		failure(CR_LOGNAME, (char *)geteuid());
	uname(&sysinfo);
	loc_mach = sysinfo.nodename;
	(void) sprintf(Local, "%s@%s", entry->pw_name, loc_mach);

	/* Set up remote principal name, if supplied */

	if (rem_user)
		(void) strcpy(Expected, rem_user);
	(void) strcat(Expected, "@");
	if (rem_mach)
		(void) strcat(Expected, rem_mach);

	/* perform the appropriate protocol role */

	LOG("Performing %s role.\n", role == 'r' ? "RESPONDER" : "IMPOSER");

	if (role == 'r')
		respond();
	else
		impose();

	/* restore origninal terminal settings if we changed them */

	if (settings)
		(void) ioctl(FDIN, TCSETAF, settings);

	return;

}

static FILE *
openlog(char *scheme)
{
 	char filename[128];

        sprintf(filename, "%s/%s/%s", DEF_LOGDIR, scheme , DEF_KMLOG);
	return(freopen(filename, "a+", stderr));
}

static int
x_set(char *name)
{
	char *p;

	if ( (p = strrchr(name, '.')) != NULL) {
		p++;
		if (strcmp(p, "enigma") == 0)
			return(X_ENIGMA);
	}
	
	return(X_DES);
}	

/* Main program of the 'cr1' Identification and Authentication scheme */

main (int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	extern int opterr; 

	int c;
	char *ch;
	char *slash;

	char **avas;	/*  array of pointers to AVAs  */
	char *rl = NULL, *ru = NULL, *rs = NULL;
	char *rlid = NULL, *rlogname = NULL, *rservice = NULL;

	int mac = 0;		/* is MAC running */
	level_t mylevel;	/* for lvlproc() result */

	mypid = getpid();

	if ((slash = strrchr(argv[0], '/')) != NULL) {
		program = slash + 1;
		if (strcmp(program, "scheme") == 0) {
			*slash = '\0';
			if ((program = strrchr(argv[0], '/')) != NULL)
				program = strdup(program+1);
			else
				program = slash + 1;
			*slash = '/';
		}
	}

	if (!program)
		program = argv[0];

	/* are we running with MAC */

	if ( (lvlproc(MAC_GET, &mylevel) == 0) || (errno != ENOPKG) )
		mac++;

	/*
	 * set the encryption/decryption algorithm.
	 * if the scheme is changed to handle multiple
	 * algorithms on the fly, this should be done
	 * for the responder role only. (getopt() case 'r')
	 */

	x_type = x_set(program);
	
	logfp = stderr;

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:cr1");
	(void) setcat("uxnsu");

	/*
	 * Save current AVA list for subsequent modification
	 * Remove the IAF module in case there is a failure
	 */

	avas = retava(FDIN);
	(void) setava(FDOUT, NULL);

	/* Parse the command line */

	opterr = 0;	/*  Disable "illegal option" error message  */

	while ((c = getopt(argc, argv, "l:rs:u:M:S:U:")) != -1) {
		switch (c) {
		case 'l':
			loc_lid = optarg;
			break;
		case 'r':
			role = 'r';
			break;
		case 's':
			loc_svc = optarg;
			break;
		case 'u':
			loc_user = optarg;
			break;
		case 'M':
			rem_mach = optarg;
			break;
		case 'S':
			rem_svc = optarg;
			break;
		case 'U':
			rem_user = optarg;
			break;
		case '?':
			failure(CR_CRUSAGE, program);
			break;
		}
	}

	if ( optind != argc )
		failure(CR_CRUSAGE, program);

	/*  Open log file  */

	if ((logfp = openlog(DEF_SCHEME)) == NULL) {
		logfp = stderr;
		LOG("Cannot open %s log. Using stderr.\n", DEF_SCHEME);
	}

	DLOG("***** %s START *****\n", role == 'r' ? "RESPONDER" : "IMPOSER");
	DLOG("Local user %s\n", loc_user ? loc_user : "<none>");
	DLOG("Local service %s\n", loc_svc ? loc_svc : "<none>");
	DLOG("Local level id %s\n", loc_lid ? loc_lid : "<none>");
	DLOG("Remote user %s\n", rem_user ? rem_user : "<none>");
	DLOG("Remote service %s\n", rem_svc ? rem_svc : "<none>");
   	DLOG("Remote machine %s\n", rem_mach ? rem_mach : "<none>");
	DLOG("Encryption is %s\n", x_type ? "enigma" : "des");

	/* do the cr1 protocol */

	do_protocol(role);

	/* Check authentication information */

	DLOG("Expected identity: '%s'\n", Expected);
	DLOG("Authenticated identity: '%s'\n", Actual);

	/* split the identity into name and system */

	if (ch = strchr(Actual, '@')) {
		*ch = '\0';
		actual_mach = strdup(ch + 1);
		if (ch != Actual)
			actual_user = strdup(Actual);
		*ch = '@';
	} else
		actual_mach = strdup(Actual);

	/* process data field assertions */
	/* data is set in respond()/impose() as appropriate */

	rl = getava(RL, data);

	ru = getava(RU, data);

	rs = getava(RS, data);

	/* if authentication was with machine key, believe assertions */
	/* otherwise we can only believe the remote user's name */

	if ((actual_user == NULL) || (*actual_user == '\0')) {
		rlid = rl;
		rlogname = ru;
		rservice = rs;
	} else {
		rlogname = actual_user;
	}
		
	/* only the Imposer can map rlogname and rlid */

	if (role == 'i') {
		Principal temp;

		if (rlogname) {
			(void) sprintf(temp, "%s@%s", rlogname, actual_mach);
			DLOG("namemap(%s)\n",temp);
			if (namemap(DEF_SCHEME, temp, Logname))
				failure(CR_NAMEMAP, temp);
		}

		/*
		 * if we are running MAC, then we must map whatever
		 * lid we do or do not get from the remote.
		 */

		if (mac) {
			if (!rlid)
				rlid = "";
			(void) sprintf(temp, "%s@%s", rlid, actual_mach);
			if (attrmap("RLID", temp, Lid))
				failure(CR_ATTRMAP, temp);
		}

	}

	/* make sure we're talking to whom we expected */

	if (rem_mach && 
		((actual_mach == NULL) || (strcmp(actual_mach, rem_mach) != 0)))
		failure(CR_XMACHINE, actual_mach);

	if (rem_user && 
		((rlogname == NULL) || (strcmp(rlogname, rem_user) != 0)))
		failure(CR_XLOGNAME, rlogname);

	if (rem_svc && 
		((rservice == NULL) || (strcmp(rservice, rem_svc) != 0)))
		failure(CR_XSERVICE, rservice);

	/* set up available ava's */

	if (rl) {
		(void) strcat(RL, rl);
		if ((avas = putava(RL, avas)) == NULL)
			failure(CR_PUTAVA, "RL");
	}

	if (ru) {
		(void) strcat(RU, ru);
		if ((avas = putava(RU, avas)) == NULL)
			failure(CR_PUTAVA, "RU");
	}

	if (rs) {
		(void) strcat(RS, rs);
		if ((avas = putava(RS, avas)) == NULL)
			failure(CR_PUTAVA, "RS");
	}

	if (rservice) {
		(void) strcat(RSERVICE, rservice);
		if ((avas = putava(RSERVICE, avas)) == NULL)
			failure(CR_PUTAVA, "RSERVICE");
	}

	if (actual_mach) {
		(void) strcat(RMACHINE, actual_mach);
		if ((avas = putava(RMACHINE, avas)) == NULL)
			failure(CR_PUTAVA, "RMACHINE");
	}

	if (rlogname) {
		(void) strcat(RLOGNAME, rlogname);
		if ((avas = putava(RLOGNAME, avas)) == NULL)
			failure(CR_PUTAVA, "RLOGNAME");
	}

	if (rlid) {
		(void) strcat(RLID, rlid);
		if ((avas = putava(RLID, avas)) == NULL)
			failure(CR_PUTAVA, "RLID");
	}

	(void) strcat(SCHEME, DEF_SCHEME);
	if ((avas = putava(SCHEME, avas)) == NULL)
		failure(CR_PUTAVA, "SCHEME");

	if (role == 'i') {
		if (*Logname)
			(void) strcat(LOGNAME, Logname);
		if ((avas = putava(LOGNAME, avas)) == NULL)
			failure(CR_PUTAVA, "LOGNAME");

		if (mac) {
			level_t ILid;

			/* we mapped the level ALIAS, but need to
			 * put the level id on the stream for set_id()
			 */

			if (lvlin(Lid, &ILid) || lvlvalid(&ILid))
				failure(CR_LVLIN, Lid);
			(void) sprintf(LID, "LID=%ld", (long) ILid);

			if ((avas = putava(LID, avas)) == NULL)
				failure(CR_PUTAVA, "LID");
		}

	}


	/* put the new AVA values on the stream */

	if (setava(FDOUT, avas) != 0)
		failure(CR_SETAVA, NULL);

	/* let ava_id() provide info needed but not yet provided */

	if (role == 'i')
		(void) ava_id(NULL, DEF_SCHEME);

	DLOG("***** %s FINISH *****\n", role == 'r' ? "RESPONDER" : "IMPOSER");

	exit(0);

	/* NOTREACHED */

}
