/*		copyright	"%c%" 	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)write:write.c	1.19.1.11"
#ident "$Header$"

/*
 ***************************************************************************
 * Inheritable Privileges : P_MACWRITE,P_DACWRITE
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************
 *
 *
 *	Program to communicate with other users of the system.
 *	Usage:	write user [line]
 */

#include	<stdio.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/utsname.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<time.h>
#include	<utmp.h>
#include	<pwd.h>
#include	<fcntl.h>
#include	<locale.h>
#include	<sys/euc.h>
#include	<getwidth.h>
#include	<ctype.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<priv.h>
#include	<sys/secsys.h>

#define		TRUE	1
#define		FALSE	0
#define		FAILURE	-1
#define		DATE_FMT	"%a %b %e %H:%M:%S"
#define		DATE_FMTID	":541"
/*
 *	DATE-TIME format
 *  %a	abbreviated weekday name
 *  %b  abbreviated month name
 *  %e  day of month
 *  %H  hour - 24 hour clock
 *  %M  minute
 *  %S  second
 *
 */

struct	utsname utsn;

FILE	*fp ;			/* File pointer for recipient's terminal */
char	*rterm,*recipient ;	/* Pointer to recipient's terminal and name */
char	*thissys;
eucwidth_t eucwidth;

static const char multilog[] =
":553:%s is logged on more than one place.\nYou are connected to \"%s\".\nOther locations are:\n";
static const char badperm[] = ":64:Permission denied\n";

static char posix_var[] = "POSIX2";
static int posix = 0;

/*
 *Procedure:     main
 *
 * Restrictions:
 *               getutent:none
 *               getpwuid:none
 *               setlocale:none
 *               ttyname:none
 *               pfmt:none
 *               fprintf:none
 *               putc:none
 *               fwrite:none
 *               fopen:none
 *               cftime:none
 *               gettxt:none
 *               fflush:none
 *               fgets:none
 *               fputs:none
 */
main(argc,argv)

int argc ;
char **argv ;

{
	register int i ;
	register struct utmp *ubuf ;
	static struct utmp self ;
	char ownname[sizeof(self.ut_user) + 1] ;
	static char rterminal[] = "/dev/\0 2345678901";
	extern char *rterm,*recipient ;
	char *terminal,*ownterminal, *oterminal;
	short count ;
	extern FILE *fp ;
	extern void openfail(),eof(),usage(),shellcmd(char *, uid_t) ;
	char input[134] ;
	register char *ptr ;
	long tod ;
	char time_buf[40];
	struct passwd *passptr ;
	char badterm[20][20];
	register int bad = 0;
	uid_t	myuid;
	uid_t priv_uid;
	register char *bp;
	int ibp;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:write");

	if (getenv(posix_var) != 0)
		posix = 1;

	getwidth(&eucwidth);

	myuid = geteuid();
	uname(&utsn);
	thissys = utsn.nodename;
	/*Check to see if LPM is installed and pass priv_uid to the shell_cmd
	 *function to determine if procprivl should be used to clear privileges
	 *or not
	 */
	priv_uid = (uid_t)secsys(ES_PRVID,0);

	/* Set "rterm" to location where recipient's terminal will go. */

	rterm = &rterminal[sizeof("/dev/") - 1] ;
	terminal = NULL ;

	if (--argc <= 0)
		usage();
	else
	    recipient = *++argv ;

	/* Was a terminal name supplied?  If so, save it. */

	if (--argc > 1)
		usage();
	else 
		terminal = *++argv ;

	/*
	 * One of the standard file descriptors must be attached to a
	 * terminal in "/dev".
	 */
	if ((ownterminal = ttyname(fileno(stdin))) == NULL &&
	    (ownterminal = ttyname(fileno(stdout))) == NULL &&
	    (ownterminal = ttyname(fileno(stderr))) == NULL)
	{
	    pfmt(stderr, MM_WARNING,
	    ":554:I cannot determine your terminal name.  No reply possible.\n") ;
	    ownterminal = "/dev/???" ;
	}

	/*
	 * Set "ownterminal" past the "/dev/" at the beginning of
	 * the device name.
	 */
	oterminal = ownterminal + sizeof("/dev/")-1 ;

	/*
	 * Scan through the "utmp" file for your own entry and the
	 * entry for the person we want to send to.
	 */
	for (self.ut_pid=0,count=0;(ubuf = getutent()) != NULL;)
	{
	    /* Is this a USER_PROCESS entry? */
	    if (ubuf->ut_type == USER_PROCESS) 
	    {
		/* Is it our entry?  (ie.  The line matches ours?) */
		if (strncmp(&ubuf->ut_line[0],oterminal,
		    sizeof(ubuf->ut_line)) == 0) 
		    self = *ubuf ;

		/* Is this the person we want to send to? */
		if (strncmp(recipient,&ubuf->ut_user[0],
		    sizeof(ubuf->ut_user)) == 0)
		{
		    /*
	   	     * If a terminal name was supplied, is this login 
		     * at the correct terminal?  If not, ignore.  If it 
		     * is right place, copy over the name.
		     */
		    if (terminal != NULL)
		    {
			if (strncmp(terminal,&ubuf->ut_line[0],
			    sizeof(ubuf->ut_line)) == 0)
			{
			    strncpy(rterm, ubuf->ut_line,
				sizeof(ubuf->ut_line)) ;
			    rterm[sizeof(ubuf->ut_line)] = '\0';

			    if (myuid && !permit(rterminal)) {
				bad++;
				rterm[0] = '\0';
			    }
			}
		    }

		    /*
		     * If no terminal was supplied, then take this 
		     * terminal if no other terminal has been encountered 
		     * already.
		     */
		    else
		    {
			/*
	 		 * If this is the first encounter, copy the 
			 * string into "rterminal".
			 */
			if (*rterm == '\0')
			{
			    strncpy(rterm, ubuf->ut_line,
				sizeof(ubuf->ut_line)) ;
			    rterm[sizeof(ubuf->ut_line)] = '\0';

			    if (myuid && !permit(rterminal)) {
				strcpy(badterm[bad++],rterm);
				rterm[0] = '\0';
			    }
			    else if (bad > 0)
			    {
				pfmt(stdout, MM_INFO, multilog,
				    recipient,rterm) ;
				for (i = 0 ; i < bad ; i++)
				    fprintf(stdout,"\t%s\n",badterm[i]) ;
			    }
			}

			/*
	 		 * If this is the second terminal, print out 
			 * the first.  In all cases of multiple terminals, 
			 * list out all the other terminals so the user 
			 * can restart knowing what her/his choices are.
			 */
			else if (terminal == NULL)
			{
			    char line_buf[sizeof(ubuf->ut_line) + 1];

			    if (count == 1 && bad == 0)
			    {
				pfmt(stdout, MM_INFO, multilog,
				    recipient,rterm) ;
			    }
			    putc('\t', stdout);
			    /*
			     * don't write garbage to the terminal.
			     * copy ut_line to local buffer and print this
			     * buffer. ut_line must not be null terminated.
			     */
			    strncpy(line_buf, ubuf->ut_line,
				sizeof(ubuf->ut_line));
			    line_buf[sizeof(ubuf->ut_line)] = '\0';
			    fputs(line_buf, stdout);
			    fprintf(stdout,"\n") ;
			}

			count++ ;
		    }  /* End of "else" */
		}  /* End of "else if (strncmp" */
	    }  /* End of "if (USER_PROCESS" */
	}  /* End of "for(count=0" */

	/* close the utmp file */
	endutent();

	/*
	 * Did we find a place to talk to?  If we were looking for a
	 * specific spot and didn't find it, complain and quit.
	 */
	if (terminal != NULL && *rterm == '\0')
	{
	    if (bad > 0)
		pfmt(stderr, MM_ERROR, badperm);
	    else
		pfmt(stderr, MM_ERROR,
			":555:%s is not at \"%s\".\n",recipient,terminal) ;
	    exit(1) ;
	}

	/*
	 * If we were just looking for anyplace to talk and didn't find
	 * one, complain and quit.
	 * If permissions prevent us from sending to this person - exit
	 */
	else if (*rterm == '\0')
	{
	    if (bad > 0)
		pfmt(stderr, MM_ERROR, badperm);
	    else
		pfmt(stderr, MM_ERROR, ":556:%s is not logged on.\n",
			recipient) ;
	    exit(1) ;
	}

	/* Did we find our own entry? */

	else if (self.ut_pid == 0)
	{
	    /*
	     * Use the user id instead of utmp name if the entry in the
	     * utmp file couldn't be found.
	     */
	    if ((passptr = getpwuid(getuid())) == (struct passwd *)NULL)
	    {
		pfmt (stderr, MM_ERROR, ":557:Cannot determine who you are.\n") ;
		exit(1) ;
	    }
	    strncpy(&ownname[0],&passptr->pw_name[0],sizeof(ownname)) ;
	}
	else
	{
	    strncpy(&ownname[0],self.ut_user,sizeof(self.ut_user)) ;
	}
	ownname[sizeof(ownname)-1] = '\0' ;

	if (!permit1(1))
	    pfmt(stderr, MM_WARNING,
		":558:You have your terminal set to \"mesg -n\".  No reply possible.\n") ;

	/* Try to open up the line to the recipient's terminal.	*/

	signal(SIGALRM,openfail) ;
	alarm(5) ;
	if ((fp = fopen(&rterminal[0],"w")) == NULL) {
		pfmt(stderr, MM_ERROR, badperm);
		exit(1);
	}
	alarm(0) ;

	/*
	 * Catch signals SIGINT, SIGHUP, SIGQUIT, and 
	 * SIGTERM, and send EOT to recipient before dying.
	 * (For POSIX2, catch SIGINT only.)
	 */
	signal(SIGINT,eof) ;
	if (!posix) {
		signal(SIGQUIT,eof) ;
		signal(SIGHUP,eof) ;
		signal(SIGTERM,eof) ;
	}

	/*
	 * Get the time of day, convert it to a string and throw away the
	 * year information at the end of the string.
	 */
	time(&tod) ;
	cftime(time_buf, gettxt(DATE_FMTID, DATE_FMT), &tod); 
	if (posix) {
		pfmt(fp, MM_NOSTD, ":1015:\n\007\007\007\tMessage from %s (%s) [ %s ] ...\n",
			&ownname[0],oterminal,time_buf) ;
	}
	else {
		pfmt(fp, MM_NOSTD, ":559:\n\007\007\007\tMessage from %s on %s (%s) [ %s ] ...\n",
			&ownname[0],thissys,oterminal,time_buf) ;
	}
	fflush(fp) ;
	fprintf(stdout,"\007\007") ;	

	/*
	 * Get input from user and send to recipient unless it begins
	 * with a !, when it is to be a shell command.
	 */
	while ((ptr = fgets(&input[0],sizeof(input),stdin)) != NULL)
	{
	    /* Is this a shell command?	*/
	    if (*ptr == '!')
	    {
		shellcmd(++ptr,priv_uid) ;
	    }

	    /* Send line to the recipient. */
	    else
	    {
		if (myuid && !permit1(fileno(fp))) {
			pfmt(stderr, MM_ERROR,
				":560:Can no longer write to %s\n",rterminal);
			pfmt(fp, MM_NOSTD, ":1014:EOT\n") ;
			exit(1);
		}

		/*
		 * All non-printable characters are displayed using a 
		 * special notation: Control characters  shall be 
		 * displayed using the two character sequence of 
		 * ^ (carat) and the ASCII character (decimal) 64 greater
		 * that the character being encoded (e.g., a \003 is 
		 * displayed ^C).  Characters with the eighth bit set 
		 * shall be displayed using the three or four character 
		 * meta notation( e.g., \372 is displayed M-z and \203 
		 * is displayed M-^C).
		 */
		i = strlen(input);
		for (bp = &input[0]; --i >= 0; bp++) {
			ibp = (unsigned int) *bp;
			if (*bp == '\n')
				putc('\r', fp);
			/* 
			 * Characters in the LC_CTYPE classifications "print"
			 * and "space", and the alert (bell) character, should
			 * be printed on the recipient's terminal.
			 */
			if (ISPRINT(ibp, eucwidth) || isspace(*bp) || *bp == '\007') {
				putc(*bp, fp);
			} else {
				if (!isascii(*bp)) {
					fputs("M-", fp);
					*bp = toascii(*bp);
				}
				if (iscntrl(*bp)) {
					putc('^',fp);
					/* add decimal 64 to control character */
					putc(*bp + 0100, fp);
				}
				else
					putc(*bp, fp);
			}
			if (*bp == '\n')
				fflush(fp);
			if (ferror(fp) || feof(fp)) {
				pfmt(stderr, MM_ERROR,
					":561:\n\007Write failed (%s logged out?): %s\n",
					recipient, strerror(errno));
				exit(1);
			}
		}  /* end for */
		fflush(fp) ;
	    }  /* end else */
	}  /* end while */

	/* Since "end of file" received, send EOT message to recipient.	*/
	eof() ;
}


/*
 *Procedure:     shellcmd
 *
 * Restrictions:
 *               pfmt:none
 *               execl(2):none
 *               fprintf:none
 */
void
shellcmd(command,priv_uid)

char *command ;
uid_t priv_uid;

{
	register pid_t child ;
	extern void eof() ;

	if ((child = fork()) == (pid_t)FAILURE)
	{
	    pfmt(stderr, MM_ERROR, ":43:fork() failed: %s\n", strerror(errno));
	    pfmt(stderr, MM_ACTION, ":562:Try again later.\n") ;
	    return ;
	}
	else if (child == (pid_t)0)
	{
	    /* Reset the signals to the default actions and exec a shell. */
	    if (setgid(getgid()) < 0)
		exit(1);	
	
	    /*
	     * If priv_uid is less than zero this system is running
	     * with a file based privilege mechanism
	     */
	    if(priv_uid <0 || priv_uid != getuid())
		/*
		 * Clear all privs. in the max set to prevent
		 * newly execed command from inheriting any
		 * privs.
		 */
		 procprivl(CLRPRV,pm_max(P_ALLPRIVS),(priv_t)0);

	    /*
	     * the filehandle might be misused in the sh (security hole).
	     */
	    fclose(fp);

  	    execl("/sbin/sh","sh","-c",command,0) ;
	    exit(0) ;
	}
	else
	{
	    /*
	     * Allow user to type <del> and <quit> without dying during
	     * commands.
	     */
	    signal(SIGINT,SIG_IGN) ;
	    signal(SIGQUIT,SIG_IGN) ;

	    /* As parent wait around for user to finish spunoff command. */
	    while(wait(NULL) != child) ;

	    /* Reset the signals to their normal state.			*/
	    signal(SIGINT,eof) ;
	    if (!posix) {
		    signal(SIGQUIT,eof) ;
		    signal(SIGHUP,eof) ;
		    signal(SIGTERM,eof) ;
	    }
	}
	fprintf(stdout,"!\n") ;
}


/*
 * Procedure:     openfail
 *
 * Restrictions:
 *               pfmt:none
 */
void
openfail()
{
	extern char *rterm,*recipient ;

	pfmt(stderr, MM_ERROR, ":563:Timeout trying to open %s's line(%s).\n",
	    recipient,rterm) ;
	exit(1) ;
}


void
eof()
{
	extern FILE *fp ;

	pfmt(fp, MM_NOSTD, ":1014:EOT\n") ;
	exit(0) ;
}


/*
 * permit: check mode of terminal - if not writable by all disallow 
 * writing to (even the user him/herself cannot therefore write to 
 * their own tty)
 */

/*
 * Procedure:     permit
 *
 * Restrictions:
 *               open(2):none
 */
permit (term)
char *term;
{
	struct stat buf;
	int fildes;

	if ((fildes = open(term,O_WRONLY)) < 0) {
		return(0);
	}
	fstat(fildes,&buf);
	close(fildes);
	return(buf.st_mode & (S_IWGRP|S_IWOTH));
}


/*
 * permit1: check mode of terminal - if not writable by all disallow 
 * writing to. (Even the user him/herself cannot therefore write to 
 * their own tty.) 
 * This is used with fstat (which is faster than stat) where possible
 */

/*
 * Procedure:     permit1
 *
 * Restrictions:
 */
permit1 (fildes)
int fildes;
{
	struct stat buf;

	fstat(fildes,&buf);
	return(buf.st_mode & (S_IWGRP|S_IWOTH));
}


/*
 * Procedure:     usage
 *
 * Restrictions:
 *               pfmt:none
 */
void
usage()
{
	pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, ":564:Usage: write user [terminal]\n") ;
	exit(1) ;
}
