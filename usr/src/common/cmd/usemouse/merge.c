#ident "@(#)merge.c	1.2 "
/*
 *	Copyright (C) The Santa Cruz Operation, 1988-1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/**********************************************************************/
/*                                                                    */
/*                 SCO XENIX 2.3 MOUSE MERGE UTILITY                  */
/*                                                                    */
/*    This program maps input from some ``graphics input (GIN) device */
/* and merges the result into the input stream of a tty. By default,  */
/* motions from a mouse ared translated to arrow keys. The rising     */
/* and falling transitions of a button are mappable.                  */
/*                                                                    */
/*    The two mechanisms which make this possible are the ``Event     */
/* Manager'' and pseudo ttys.                                         */
/*                                                                    */
/* ALGORITHM                                                          */
/*                                                                    */
/*    An event queue is opened taking in mouse and keyboard input.    */
/* The mouse input is mapped to ascii strings and merged through      */
/* pseudo ttys with the keyboard input stream.                        */
/*                                                                    */
/*    A pseudo-tty is established. One process reads from an event    */
/* queue and writes to the master tty. One process reads the master   */
/* tty and prints what it reads. That provides echo. A third process  */
/* reads and writes to the slave tty. By default, that process will   */
/* be a shell process.                                                */
/*                                                                    */
/**********************************************************************/

/*
 *      MODIFICATION HISTORY
 *
 *      L000    01 Nov 1990     scol!dipakg
 *      - Allow for long filenames
 *	L001	23 Sep 1994	scol!ianw
 *	- Changes to removed compiler warnings.
 *	L002	17 Feb 1997	scol!keithp
 *	- Ported to Gemini
 */

/*
 * merge.c
 *
 * This program opens a pseudo-tty, spawns three processes.
 * One translates mouse and keyboard events to ascii and writes them 
 * to the pseudo tty (through stdout). Another process is spawned to 
 * read from the master tty, by default, thus providing echo. 
 * A third process is spawned reading from and writing to the slave
 * pseudo tty; this is initially a shell. This process becomes
 * a process group leader.
 *
 * The parent process saves the pid of the shell process and sends
 * SIGTERM when this process gets SIGHUP.
 *
 * After starting the shell and the echo children, the parent process
 * executes code in the file source.c. It reads an event queue and
 * writes to the master ptty. The clean-up stuff for the program
 * gets executed by this process. That is where the terminal state
 * is restored and wayward children are executed.
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/termio.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <limits.h>				/* L002, for PATH_MAX */

#include "merge.h"
#include "source.h"

char	progname[80];
int	errlev = 0;

#define	ASCIIBELL	7

int	echopid;
int	shellpid;
struct termio tsave;
static char	deffile[] = DEFFILE;
static char	*conffile = &deffile[0];
static char	*usrcmd = (char*) NULL;
static int	bells=1;

static void	read_args(int, char **);
static int	open_ptty(char *);
static int	spawn_echoserver(int);
static int	spawn_shell(char *);
static int	readconff(char *);
static int	readtypef(char *);
static int	readfile(char *);
static void	fail(char *);
static void	warn(char *);
static void	theend();					/* L001 */
static int	setparm(char *, char *);
static void	print_usage();
static int	isnumber(char *);
static int	value(char *);

extern void	readq(int);
extern void	set_sensitivity(char, int);

main(argc, argv)
int	argc;
char	**argv;
{

	char	slavenam[80];
	int	master;

	signal(SIGINT, theend);
	signal(SIGQUIT, theend);
	signal(SIGTERM, theend);
	signal(SIGCLD, theend);
	signal(SIGHUP, theend);

	strcpy(progname, argv[0]);
	/* grab and save terminal state */
	ioctl(0, TCGETA, &tsave);
	/* grab and open an event queue */
	read_args(argc,argv);
	master = open_ptty(slavenam);
	echopid = spawn_echoserver(master);
	shellpid = spawn_shell(slavenam);
	/* read event queue and write data to master pseudo tty */
	readq(master);
	/* should never ever be reached */
	return 0;
}

static int
spawn_echoserver(master)
int	master;
{
	char	buf[4096];
	register int n, i;
	int	ret;

	/* spawn echoserver */
	if ( (ret = fork()) > 0 )
		return ret;
	else if ( ret == -1 )
		fail("fork() failed on echo process");

	while( (n = read(master, buf, sizeof buf)) > 0)	{
		if ( bells )
			write(1, buf, (unsigned) n);
		else
			for (i=0; i<n; i++)
				if ( buf[i] != ASCIIBELL )
					write(1, buf+i, 1);
	}
	exit(0);
	return 0;	/* To supress a lint warning */
}

#define	NOEXEC	"cannot exec %s (try absolute pathnames)"

static int
spawn_shell(slave)
char	*slave;
{
	struct termio termio;
	int	ret, fd;
	char	*getenv(char*);
	char	*command;

	if ( (ret = fork()) > 0 )
		return ret;
	else if ( ret == -1 )
		fail("fork shell failed");

	setpgrp();
	fd = open(slave, O_RDWR);
	if ( fd == -1 )
		fail("can't open slave");
	/* want to start slave pseudo tty with normal tty settings */
	ioctl(0, TCGETA, &termio);
	close(0);
	close(2);
	dup(fd);
	dup(fd);
	ioctl(0, TCSETA, &termio);
#ifdef NEVER
	/* If we don't turn off ONLCR on the original tty, */
	/* we'll have nl->crlf mapping happening twice. bad. */
	termio.c_oflag &= ~ONLCR; 
	ioctl(1, TCSETA, &termio);
#endif
	close(1);
	dup(fd);
	close(fd);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGCLD, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	if ( usrcmd == (char*) NULL )	{
		if ( (command = getenv("SHELL")) == (char*) NULL )
			command = "/bin/sh";
		if ( strrchr(command,'/') == (char*) NULL )
			ret = execl(command, command, (char*) 0);
		else
			ret = execl(command, strrchr(command,'/')+1, (char*) 0);
	}
	else
		ret = execl("/bin/sh", "sh", "-c", usrcmd, (char*) 0);

	if ( ret == -1 )	{
		char	buf[128];
		extern int errno;
		errno = 0;
		sprintf(buf, NOEXEC, command);
		fail(buf);
	}
}

static int
setparm(parm, value)
char	*parm;
char	*value;
{
	struct xlat *xp;
	int	num;

#ifdef DEBUG
	printf("setparm: enter, parm==`%s', value==`%s'\n",parm,value);
#endif
	if ( value == (char*) NULL )
		value = "\0";

	for (xp = xlat; xp->item != (char*) NULL; xp++)
		if ( ! strcmp(parm, xp->item) )	{
			strcpy(xp->buf, value);
			xp->len = (short) strlen(value);
			return 0;
		}
	if ( ! strcmp(parm, HSENSE) )
		if ( sscanf(value, "%d", &num) == 1 )	{
			set_sensitivity('h', num);
			return 0;
		}
	if ( ! strcmp(parm, VSENSE) )
		if ( sscanf(value, "%d", &num) == 1 )	{
			set_sensitivity('v', num);
			return 0;
		}
	if ( ! strcmp(parm, BELLS) )	{
		if ( toupper(value[0]) == 'Y' )
			bells = 1;
		else
			bells = 0;
		return 0;
	}
	return -1;
}

static void
fail(s)
char	*s;
{
	extern int errno;

	if ( errno )
		printf("%s: %s(errno %d)\n", progname, s, errno);
	else
		printf("%s: %s\n", progname, s);
	theend();
}

static void
warn(s)
char	*s;
{
	printf("%s: warning -- %s\n", progname, s);
}

static void							/* L001 */
theend()
{
	exit(0);
}

#define	NOCONF	"cannot open configuration file %s"
#define	TEXCLUDEF	"-f and -t flags are mutually exclusive"
#define	NONUMBERH	"bad (i.e. nonnumeric) argument to -h flag"
#define	NONUMBERV	"bad (i.e. nonnumeric) argument to -v flag"

static void 
read_args(argc, argv)
int	argc;
char	**argv;
{
	int	c, num;
	char	buf[10];
	char	*typefile = (char*) NULL;

	opterr = 0;
	while ( (c=getopt(argc,argv,"bt:f:c:h:v:")) != EOF )
		switch (c)	{
			case 'b':	bells = 0;
					break;
			case 't':	if ( conffile != deffile )
						fail(TEXCLUDEF);
					typefile = optarg;
					break;
			case 'f':	if ( typefile != (char*) NULL )
						fail(TEXCLUDEF);
					conffile = optarg;
					break;
			case 'h':
			case 'v':	if ( ! isnumber(optarg) )
						fail(NONUMBERV);
					set_sensitivity('v', value(optarg));
					break;
			case 'c':	usrcmd = optarg;
					break;
			case '?':	print_usage();
					exit(0);
		}

	/* If the current argv argument is not a parameter (has no =) */
	/* then assume it is a typefile. */
	if ( optind < argc && strchr(argv[optind], '=') == (char*) NULL )
		if ( typefile == (char*) NULL )
			typefile = argv[optind++];

	if ( !typefile || readtypef(typefile) == -1 )
		if ( !conffile || readconff(conffile) == -1 )
			warn("no default configuration file");

	while ( optind < argc )	{
#ifdef DEBUG
		printf("command line contains \"%s\"\n", argv[optind]);
#endif
		parseparms(setparm,argv[optind++]);
	}
}

static int 
readtypef(file)
char	*file;
{
	char	buf[PATH_MAX];			/* L002 */
	extern int errno;

	if ( file == (char*) NULL )	
		return -1;
	sprintf(buf, "/usr/lib/mouse/%s", file);
	if ( readfile(buf) == -1 )	{
		sprintf(buf, "cannot open map file /usr/lib/mouse/%s", file);
		errno = 0;
		fail(buf);
	}
	return 0;
}

static int 
readconff(file)
char	*file;
{
	char	buf[256];
	extern int errno;

	if ( file == (char*) NULL )	
		return -1;
	if ( readfile(file) != -1 )
		return 0;		/* success */
	if ( file == deffile )		/* If they requested no special file */
		return -1;		/* warn them no file present later */
	/* If we were unable to open a file they requested, fail */
	sprintf(buf, "cannot open map file /usr/lib/mouse/%s", file);
	errno = 0;
	fail(buf);
	return 0;	/* supress a warning */
}

#define	LINVALID	"invalid entry in file %s on line %d"
static int
readfile(file)
char	*file;
{
	FILE	*fp;
	char	buf[256];
	int	lineno=0;

	if ( (fp = fopen(file, "r")) == (FILE*) NULL )
		return -1;
	
	while( fgetline(fp, buf, sizeof buf) != -1 )	{
		lineno++;
		if ( buf[0] == '#' || buf[0] == '\n' )
			continue;
		if ( parseparms(setparm, buf) == -1 )	{
			sprintf(buf, LINVALID, file, lineno);
			fail(buf);
		}
	}
	fclose(fp);
	return 1;
}

/*
 * Open master pseudo tty.
 *
 * Write filename of slave into parameter.
 */
#define	MASTER	"/dev/ptyp%d"
#define	SLAVE	"/dev/ttyp%d"
#define	NOMORE	"no more pseudo ttys available"
#define	NOPTTY	"pseudo ttys not present"
static int
open_ptty(s)
char	*s;
{
	int	fd;
	int	count;
	char	buf[20];
	extern int errno;

	count = 0;
	while (1)	{
		sprintf(buf, MASTER, count);
		fd = open(buf, O_RDWR);
		if ( fd >= 0 )	{
			/* success, opened a master pseudo tty */
			sprintf(s, SLAVE, count);
			return fd;
		}
		/* failed to open */
		switch (errno)	{
			case ENXIO:	fail(NOMORE);
			case ENOENT:	if ( count == 0 )
						fail(NOPTTY);
					fail(NOMORE);
		}
		/* Try next device */
		count++; 
	}
}

static void
print_usage()
{
	int	i;
	static char *msg[] = {
		"Usage is:\n\n",
		"  ", progname,
		" [-f file] [-h num] [-v num] [-c cmd] [-b] [-t file | file] [parms]\n",
		"\nWhere:\n",
		"   -f   file	",
		"Selects a map file other than ", deffile, ".\n",
		"   -h   num	",
		"Set sensitivity for horizontal mouse motions. (default 5)\n",
		"   -v   num	",
		"Set sensitivity for vertical mouse motions. (default 5)\n",
		"   -c   cmd	",
		"Executes cmd. The command may be quoted to contain spaces.\n",
		"   -b		",
		"Supress echoing of bell (^G) characters.\n",
		"   [-t] file	",
		"Selects a mapping file from drectory /usr/lib/mouse.\n\t\t",
		"Specify the -t flag if the filename contains an `='.\n",
		"   parms	",
		"Are mouse-event to ascii-string mappings. These include:\n\n",
		"\trbu= (rt butn up)",
		"\tmbu= (md butn up)",
		"\tlbu= (lt butn up)\n",
		"\trbd= (rt butn dn)",
		"\tmbd= (md butn dn)",
		"\tlbd= (lt butn dn)\n",
		"\trt= (right)",
		"\t\tlt= (left)",
		"\t\tup= (up)\n",
		"\tdn= (down)",
		"\t\tul= (up-left)",
		"\t\tur= (up-right)\n",
		"\tdr= (down-right)",
		"\tdl= (down-left)\n",
		"\n",
		"Notes:\tStrings containing spaces must be quoted twice:\n",
		"\t\tusemouse  rbd='\"cd /usr/project/src/mydir\\015\"'\n",
		"\tThe -c flag and a type file [-t] are mutually exclusive.\n",
		0
	};

	for ( i=0; msg[i]; i++ )
		printf("%s",msg[i]);
}

static int
isnumber(s)
char	*s;
{
	int	n;
	char	buf[10];

	if ( sscanf(s, "%d", &n) == 0)
		return 0;
	sprintf(buf, "%d", n);
	buf[sizeof buf] = '\0';
	return ! strcmp(s,buf);
}

static int 
value(s)
char	*s;
{
	int	n;
	sscanf(s, "%d", &n);
	return n;
}
