#ident	"@(#)mouseadmin.c	1.7"
#ident	"$Header$"

/*
 * mouseadmin 
 * Mouse subsystem configuration. Set entries in the /usr/lib/mousetab
 * and the /etc/default/mouse.
 * 
 * L001	philk@sco.com	11/11/97
 *	Don't display test option if no mouse entry is present (hangs 
 *  for timeout). Don't rewrite the /etc/conf/pack.d/smse/space.c 
 * 	when setting the smse_MSC_selected variable.
 * 
 * L002 philk@sco.com 	26/11/97
 *	Add f flag to idconfupdate(1M) to force DCU edits out to resmgr(1M). 
 *	If the tunable file is not present when mouseadmin(1M) is invoked 
 *	then create it. 
 * 
 * L003 philk@sco.com	03/12/97
 *	Tunable file (smse space.c) is recreated each time that mouseadmin is
 *	invoked. Adds requirement that mouseadmin updated with tunable edits, 
 *	but obviates any problems due to unrecognised variable initialisation,
 *	or user hand editing breaking test/set editing code. 
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <sys/audit.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stream.h>
#include <sys/sysmacros.h>
#include <sys/mouse.h>
#include <sys/wait.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/mse.h>
#include <sys/resmgr.h>

#define CFG_NAME	"/dev/mousecfg"
#define MOUSETAB	"/usr/lib/mousetab"
#define MAX_DEV	100
#define MAXDEVNAME	64
#define TFILE 	"/etc/conf/pack.d/smse/space.c"
#define WBUFSZ 	128	

/* 
 * L003 {
 * Text of smse space.c file (sets value of tunable smse_MSC_selected)
 * (value determined by user's selection of "Mouse Systems" mouse type).
 * NB! MUST UPDATE WITH ANY OTHER TUNABLES DEFINED IN SMSE SPACE.C FILE 
 */ 

#define	SPACE_Y				\
"/* Created by mouseadmin(1M)	*/\n 	\
int	smse_force_msetype = 0;\n	\
int	smse_MSC_selected  = 1;\n"

#define	SPACE_N				\
"/* Created by mouseadmin(1M)	*/\n 	\
int	smse_force_msetype = 0;\n	\
int	smse_MSC_selected  = 0;\n"

/* } L003 */

int	row, col;
int	c;
char	fname[MAXDEVNAME];
char	msebusy[MAX_MSE_UNIT+1];
int	listing=0, deleting=0, adding=0, testing=0;
int	no_download=0, bus_okay=0;
int	cfg_fd;
int	suserflg=0;
int	setpgrpflg=0;
int	addflg = 0, rmflg = 0;
int msepres = 0;		/* L001 */

struct mousemap	map[MAX_DEV];
struct {
	char	disp[MAXDEVNAME];
	char	mouse[MAXDEVNAME];
} table[MAX_DEV];
int ntdcnt = 0;


int	n_dev;
int	cursing = 0;

#ifdef __STDC__
int	(*print)(const char *, ...) = printf;
#else
int	(*print)() = printf;
#endif

void 	load_table(), download_table(), show_table();
void 	interact();
int 	delete_entry(), add_entry(), test_entry();
int 	config_mod();
void 	delete_rmkey(char *);
int	init_tunable(char);

int	irq = 0;
char	errstr[80];
char	mse_name[10], mouse[10];
int	debug = 0;

/*
 *	Description:
 */

void
fatal_error(fname)
char	*fname;
{
	if (cursing) {
		int	save_err;

		save_err = errno;
		endwin();
		errno = save_err;
	}

	perror(fname);
	exit(1);
}

/*
 *	Description:
 */

void
_fatal_error(msg)
char	*msg;
{
	if (cursing)
		endwin();
	fprintf(stderr, "\n%s.\n\n", msg);
	exit(1);
}

/*
 *	Description:
 */

void
enter_prompt()
{
	char	ch;

	row += 2;
	while (1) {
		mvaddstr(row, col, (char*)gettxt(":1","Strike the ENTER key to continue."));
		refresh();
		ch = getchar();
		if (ch == '\n' || ch == '\r')
			break;
		else
			beep();
	}
	row++;
}

/*
 *	Description:
 */

void
warn_err(msg)
char	*msg;
{
	if (cursing) {
		row+=2;
		beep();
		mvaddstr(row,col,msg);
		enter_prompt();
	}
	else 
		fprintf(stderr, msg);
	return;
}

/*
 *	Description:
 */

get_info(strp, retp)
char *strp, *retp;
{

	*retp='\0';
	row++;
	mvaddstr(row, col, strp);
	refresh();
	attron(A_BOLD);
	getstr(retp);
	attroff(A_BOLD);
}

/*
 *	Description:
 */

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	c, usage = 0, retval, interrupt = 0;
	extern int	optind;
	extern char	*optarg;
	char *device;

	/* Initialize locale info */
	(void) setlocale(LC_ALL,"");

	/* Initialize message label */
	(void) setlabel("UX:mouseadmin");

	/* Initialize catalog */
	(void) setcat("mousemgr");

	device = (char *) NULL;

	while ((c = getopt(argc, argv, "h:ld:a:ntbi:")) != EOF) {
		switch (c) {

		case 'h':
			if (strcmp(optarg,"idden") == 0)
				setpgrpflg++;
			else	
				debug++;
			break;
		case 'l':
			listing++;
			break;
		case 'd':
			if (!(device=(char *)(malloc(strlen(optarg)+1)))) 
				_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
			deleting++;
			strcpy(device, optarg);
			break;
		case 'a':
			if (!(device=(char *)(malloc(strlen(optarg)+1)))) 
				_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
			adding++;
			strcpy(device, optarg);
			break;
		case 'n':
			no_download++;
			break;
		case 'b':
			bus_okay++;
			break;
		case 't':
			testing++;
			break;
		case 'i':
		{
			char *cmd;
			if (!(cmd=(char *)(malloc(1024)))) 
				_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
			interrupt++;
			irq=atoi(optarg);
			if ( irq == 2 )
				irq = 9;
			sprintf(cmd,"FOO=`/etc/conf/bin/idcheck -r -v %d`\n[ \"$FOO\" = \"bmse\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1",irq);
			if ( system(cmd) != 0 ) {
				fprintf(stderr,(char*)gettxt(":2","Interrupt %d is already in use.\n"),irq);
				free(cmd);
				exit(1);
			}
			free(cmd);
			break;
		}
		default:
			usage++;
			break;
		}
	}

	switch (testing + deleting + adding + listing) {
	case 0:
		if (argc - optind != 0)
			usage++;
		break;
	case 1:
		if (deleting && argc - optind != 0)
			usage++;
		else if (adding && argc - optind != 1)
			usage++;
		else if (listing && argc - optind != 0)
			usage++;
		else if (testing && argc - optind != 0)
			usage++;
		break;
	default:
		usage++;
		break;
	}

	if (usage) {
		fprintf(stderr, (char*)gettxt(":3","Usage: mouseadmin { -t | -n | -b | -l | -i irq | -d terminal | -a terminal mouse }\n"));
		exit(1);
	}

	if ((device!=(char *)NULL) && (strcmp(device, "console") == 0))
		strcpy(device, "vt00");

	if((optind<argc) && (strcmp(argv[optind], "320") == 0))
		strcpy(argv[optind], "m320");

	if((optind<argc) && (strcmp(argv[optind], "PS2") == 0))
		strcpy(argv[optind], "m320");


	if((optind<argc) && (strcmp(argv[optind], "BUS") == 0 ||strcmp(argv[optind],"Bus")== 0 || strcmp(argv[optind], "bus")== 0))
		strcpy(argv[optind], "bmse");

	if ((optind<argc)&& (strcmp(argv[optind], "bmse") && interrupt)) {
		fprintf(stderr,(char*) gettxt(":4","-i option only valid with Bus mouse\n"));
		fprintf(stderr,(char*)gettxt(":3","Usage: mouseadmin { -t | -n | -b | -l | -i irq | -d terminal | -a terminal mouse }\n"));
		exit(1);
	}

	load_table();
	if(!no_download && !testing)
		get_mse_opened();

	if (listing) {
		show_table();
		exit(0);
	}

	if (suserflg) {
		fprintf(stderr,(char*)gettxt(":42", "Permission denied\n"));
		exit(1);
	}

	if (testing) {
		exit (test_entry());
	}

	if (deleting) {
		if ((retval = delete_entry(device)) < 0) {
			if(retval == -1)
				fprintf(stderr, (char*)gettxt(":5","\nThere is no mouse assigned for %s.\n"), device);
			else if(retval == -2)
				fprintf(stderr,(char*)gettxt(":6","\nThe mouse on %s is currently busy.\n"),device);
			else if(retval == -3)
				fprintf(stderr, (char*)gettxt(":7","\n%s is not a valid display terminal.\n"), device);
			exit(1);
		}
		unconfig_mod(mse_name);
		download_table();
		exit(0);
	}

	if (adding) {
		if (strcmp(argv[optind],"bmse") == 0) {
		   if (!interrupt) {
			fprintf(stderr,(char*)gettxt(":83","specify -i irq to add %s.\n"),argv[optind]);
			exit(1);
		   }
		}
		else if (strcmp(argv[optind],"m320") == 0) 
			irq = 12; /* no other IVN possible for PS/2 port */

		if (add_entry(device, argv[optind])) 
			exit(1);
		config_mod(argv[optind], irq);
		download_table();
		exit(0);
	}

	interact();
	exit(0);
}


/*
 *	Description:
 */

int
get_dev(name, dev_p)
char	*name;
dev_t	*dev_p;
{
	struct stat	statb;

	if (strncmp(name, "/dev/", 5) == 0) {
		strcpy(fname, name);
		strcpy(name, name + 5);
	} else {
		strcpy(fname, "/dev/");
		strcat(fname, name);
	}

	if (stat(fname, &statb) == -1)
		return -1;
	if ((statb.st_mode & S_IFMT) != S_IFCHR)
		return -2;

	*dev_p = statb.st_rdev;
	return 0;
}


/*
 *	Description:
 */

void
load_table()
{
	FILE	*tabf;
	char	dname[MAXDEVNAME], mname[MAXDEVNAME];
	struct stat	statb;

	n_dev = 0;
	if ((tabf = fopen(MOUSETAB, "r")) == NULL)
		return;

	/* Format is:
	 *	disp_name  mouse_name
	 */

	while (fscanf(tabf, "%s %s", dname, mname) > 0) {
		if (get_dev(dname, &map[n_dev].disp_dev) < 0){
			continue;
		}
		if (debug)
			fprintf(stderr,(char*)gettxt(":84","load_table: mouse = %s\n"),mname);

		if (strncmp(mname, "m320", 4) == 0){
			map[n_dev].type = M320;
		}else 
		if (strncmp(mname, "bmse", 4) == 0){
			map[n_dev].type = MBUS;
		}else  {
			map[n_dev].type = MSERIAL;
		}
		if (debug)
			fprintf(stderr, (char*)gettxt(":85","load_table:disp=%x,type=%x,count=%d\n"),map[n_dev].disp_dev,map[n_dev].type,n_dev);
		if (get_dev(mname, &map[n_dev].mse_dev) < 0) {
			continue;
		}
		strcat(table[n_dev].disp, dname);
		strcat(table[n_dev++].mouse, mname);
		ntdcnt++;
	}

	fclose(tabf);
}


/*
 *	Description:
 */

void
write_table()
{
	FILE	*tabf;
	int	i;

	if ((tabf = fopen(MOUSETAB, "w")) == NULL)
		fatal_error(MOUSETAB);
	chmod(MOUSETAB, 0644);

	if (debug)
		fprintf(stderr, (char*)gettxt(":86","write_table, n_dev is %d\n"), n_dev);
	for (i = 0; i < n_dev; i++)
		fprintf(tabf, (char*)gettxt(":","%s\t\t%s\n"), table[i].disp, table[i].mouse);

	fclose(tabf);
}

/*
 *	Description:
 */

get_mse_opened()
{
	int i;

	if(getuid() != 0)
		suserflg = 1;
	if ((cfg_fd = open(CFG_NAME, O_WRONLY)) < 0)
		fatal_error(CFG_NAME);
	if (ioctl(cfg_fd, MOUSEISOPEN, msebusy) < 0) 
		fatal_error(CFG_NAME);
	close(cfg_fd);
}

/*
 *	Description:
 */

void
download_table()
{
	struct mse_cfg	mse_cfg;

	if (debug)
		fprintf(stderr,(char*)gettxt(":87","entering download_table: no_download is %d\n"),no_download);
	if (no_download) {
		write_table();
		return;
	}
	
	/* Tell the driver about the change */
	if(suserflg)
		fatal_error(CFG_NAME);
	if ((cfg_fd = open(CFG_NAME, O_WRONLY)) < 0)
		fatal_error(CFG_NAME);

	mse_cfg.mapping = map;
	mse_cfg.count = n_dev;
	if (ioctl(cfg_fd, MOUSEIOCCONFIG, &mse_cfg) < 0) 
		if (errno == EBUSY) 
			_fatal_error((char*)gettxt(":9","One or more mice are in use.\nTry again later"));
		else
			fatal_error(CFG_NAME);

	close(cfg_fd);

	if (debug)
		fprintf(stderr, (char*)gettxt(":116","download_table:disp=%x,mse=%x,type=%x,count=%d\n"),map[n_dev-1].disp_dev,map[n_dev-1].mse_dev,map[n_dev-1].type,n_dev);

	/* Write the new table out to the mapping file */
	write_table();
}


/*
 *	Description:
 */

void
show_table()
{
	int	i;

	if (n_dev == 0) {
		(*print)((char*)gettxt(":10","\nThere are no mice assigned.\n\n"));
		msepres = 0;
		return;
	}

	(*print)((char*)gettxt(":11","\nThe following terminals have mice assigned:\n\n"));
	(*print)((char*)gettxt(":12","Display terminal	  Mouse device\n"));
	(*print)((char*)gettxt(":13","----------------	  ------------\n"));

	for (i = 0; i < n_dev; i++) {
		if(strcmp(table[i].disp, "vt00") == 0)
			(*print)("%-22s", "console");
		else
			(*print)("%-22s", table[i].disp);
		if(strncmp(table[i].mouse,"bmse", 4) == 0)
			(*print)((char*)gettxt(":14","Bus mouse\n"));
		else
		if(strncmp(table[i].mouse,"m320", 4) == 0)
			(*print)((char*)gettxt(":15","PS2 mouse\n"));
		else
			(*print)((char*)gettxt(":16","Serial mouse on %s\n"), table[i].mouse);
	}

	(*print)("\n");
}


/*
 *	Description:
 */

int
lookup_disp(disp)
char	*disp;
{
	int	slot;

	for (slot = 0; slot < n_dev; slot++) {
		if (strcmp(disp, table[slot].disp) == 0)
			return slot;
	}
	return -1;
}

/*
 *	Description:
 */

int childpid=-1;
int childpid2=-1;

int
OnSigTerm()
{
 	mvprintw(0,0,""); 
	clear();
	refresh();
	/* only endwin() if not invoked via 'T' option to mouseadmin menu */
	if (testing)
		endwin();
	exit(5);
}

/*
 *	Description:
 * Code here forks two children, one which is in the foreground and runs the mouse
 * test, the other child signals the test application after 20 seconds to kill it.
 * This prevents a hang in a system call (un-signallable) from hanging the test. 
 * The worst case is that the process is killed, but the driver servicing the call 
 * is stuck in an unusable state.
 * The test hangs even if it could drop out early, eg. if it can't open a device, for
 * consistency in ISL procedures.
 */


int
test_entry()
{
	int cnt;
	int xscale = 10;
	int yscale = 10;
	int disp;
	int waitflag, waitflag2;
	int mouse_is_on = 0;
	int msc_button_drop = 0;

	int mousefd, x, y, sx, sy, old_sx, old_sy, sleep_time;
	struct mouseinfo m;
	int buttoncnt=0; /* keep track of how many button state changes we
				have seen in test -- we need to see 2 to both
				see successful button change and not leave
				button state in state such that retry appears
				to see button change even though the user
				didn't see button change.
			  */


	for (cnt=0;cnt < 10; cnt++)
		switch (childpid=fork()) {

		  case -1: { /* retry up to 10 times */
			continue;
			break;
		  }
		  default: { /* parent or child */
			cnt=15; /* break out of for */
			break;
		  }
		}	
	
	if (cnt==10) return (2); /* cnt will be 15 if forked OK */
	if (childpid) { /* parent */
		sigignore(SIGINT);
		sigignore(SIGHUP);
		for (cnt=0;cnt < 10; cnt++)
			switch (childpid2=fork()) {

		  	  case -1: { /* retry up to 10 times */
				continue;
				break;
		  	  }
		  	  default: { /* parent or child */
				cnt=15; /* break out of for */
				break;
		  	  }
		}
		if (cnt==10) {
			kill (childpid,SIGKILL); /* make sure 1st kid dies */
			return (2); /* cnt will be 15 if forked OK */
		}
		if (!childpid2) { /* child */
			int rv;
			sleep(20);
			rv = kill(childpid,SIGTERM);

			exit(0); /* whole purpose in life is to kill other
				  * child
				  */
		}
		waitpid(childpid, &waitflag, 0);
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		kill(childpid2,SIGKILL);
		wait(&waitflag2);
		/* return childpid's exit value */
		if (WIFEXITED(waitflag)) 
			return (WEXITSTATUS(waitflag)); /* return child exit */
		return (1); /* returned because of signal */

	}
	/* Child will run simple app to test mouse input
	 * Return 0 as soon as mouse input detected
	 */

	if (setpgrpflg) {
		int i, fd;

		setpgrp(); /* become group leader */

		for (i=0; i<20; i++)
			close(i);

		fd=open("/dev/console",O_RDWR); /* stdin */
		if (fd==-1)
			exit (4);
		dup(fd);				/* stdout */
		dup(fd);				/* stderr */
	}
	signal(SIGTERM, (void(*)()) OnSigTerm);
	sleep_time = 0;
	mousefd = open ("/dev/mouse", O_RDONLY);

	/*
	 * We *could* fail right here, but the mouse test is
	 * more consistent at installation time if it always
	 * "hangs" even on cases when we can detect right away
	 * that the mouse ain't there...
	 */

	mouse_is_on = 1;
	mvaddstr(LINES - 1, 0, (char*)gettxt(":17","Mouse tracking test program"));
 	if (testing) {
		initscr (); 
	} else {
		mvaddstr(0,0,"");
		erase();
	}
	refresh();

	m.xmotion = 0;
	m.ymotion = 0;
	old_sx = sx = old_sy = sy = 0;
	x = COLS / 2 * xscale;
	y = LINES / 2 * yscale;
	while (1) {

		if ((mousefd >= 0) && (ioctl (mousefd, MOUSEIOCREAD, &m) == -1)) {
			if (testing) {
				mvprintw(0,0,"");
				erase();
				refresh();
				endwin();
			}
			exit (3);
		}

		if ((mousefd >= 0) && (m.status & BUTCHNGMASK)) {
			if (buttoncnt == 0){ 
			  /* button chg found. Wait for next button
			   * change -- otherwise next retry of test will
			   * see it and exit test prematurely
			   */
				 buttoncnt++;
				 continue;
			}
			if (testing) {
				mvprintw(0,0,"");
				erase();
				refresh();
				endwin();
			}
			exit(0);
		}
		x += m.xmotion;
		y += m.ymotion;
		mvaddch (old_sy, old_sx, (int) ' ');
		if ((sx = x / xscale) < 0)
			x = sx = 0;
		else if (sx >= COLS)
			x = (sx = COLS - 1) * xscale;
		if ((sy = y / yscale) < 2)
			y = sy = 2;
		else if (sy >= LINES - 1)
			y = (sy = LINES - 2) * yscale;
		mvaddch (sy, sx, (int) 'X');
		old_sy = sy;
		old_sx = sx;
		mvprintw (0, 0, (char*)gettxt(":18","Press a mouse button to stop test.\n", m.status));
		printw ((char*)gettxt(":19","Test will be canceled automatically in 15 seconds.\n", m.status));
		refresh ();
	}
}

/*
 *	Description:
 */

int
delete_entry(terminal)
char	*terminal;
{
	int		slot;
	dev_t		dummy;

	if (get_dev(terminal, &dummy) < 0 || strcmp(terminal,"vt00") != 0 && !(strncmp(terminal,"s",1)==0 && strchr(terminal,'v') != NULL))
		return -3;
	if ((slot = lookup_disp(terminal)) == -1)
		return -1;
	if (msebusy[slot])
		return -2;
	
	if (!strcmp(table[slot].mouse, "bmse") || !strcmp(table[slot].mouse, "m320")) {
		strcpy(mse_name, table[slot].mouse);
	} else	{
		strcpy(mse_name, "smse");
	}

	for (--n_dev; slot < n_dev; slot++) {
		table[slot] = table[slot + 1];
		map[slot] = map[slot + 1];
	}

	if (debug)
		fprintf(stderr, (char*)gettxt(":88","delete_entry: n_dev=%d\n"), n_dev);
	ntdcnt--;
	return 0;
}

/*
 *	Description:
 */

int
add_entry(terminal, mouse)
char	*terminal, *mouse;
{
	int	slot, i;
	int	newflag = 0;
	dev_t	disp_dev;

	if ((slot = lookup_disp(terminal)) == -1) {
		newflag = 1;
		if ((slot = n_dev) >= MAX_DEV)
		{
			warn_err((char*)gettxt(":20","Too many mice configured, one must be removed before another is added.\n"));
			return(1);
		}
	
		if (get_dev(terminal, &disp_dev) < 0 || strcmp(terminal,"vt00") && (strncmp(terminal,"s",1) && strchr(terminal,'v') != NULL)){
			warn_err((char *)gettxt(":80","Requested display terminal is not valid.\n"));
			return(1);
		}
	}

	if ((strcmp(terminal, "vt00") == 0 && (strcmp(mouse,"bmse") != 0 && strcmp(mouse,"m320") != 0) && strncmp(mouse,"tty",3)!=0)) {
		warn_err((char *)gettxt(":81","Requested display/mouse pair is not valid.\n"));
		return(1);
	}

	if ((strncmp(terminal,"s",1) == 0 && strchr(terminal,'v') != NULL) && (strncmp(mouse,"s",1) != 0 || strchr(mouse,'t') == NULL)){
		warn_err((char *)gettxt(":81","Requested display/mouse pair is not valid.\n"));
		return(1);
	}

	if ((strlen(mouse) > 5 && strcmp("vt00",mouse+5)==0) || strcmp("vt00",mouse) == 0) {
		sprintf(errstr,(char *)gettxt(":82","%s is not a valid mouse device.\n"),mouse);
		warn_err(errstr);
		return(1);
	}

	if (strcmp(terminal,mouse) == 0) {
		warn_err((char*)gettxt(":28","The mouse and display terminal can not be connected to the same port.\n"));
		return(1);
	}

	for (i = 0; i < n_dev; i++) {
		if (strcmp(mouse,table[i].mouse) == 0){
			sprintf(errstr,(char*)gettxt(":30","Device %s is already assigned to a Display terminal.\n"), mouse);
			warn_err(errstr);
			return(1);
		}
	}

	if (msebusy[slot]) {
		sprintf(errstr,(char*)gettxt(":117","Mouse device %s is currently in use. Configuration not changed.\n"), mouse);
		warn_err(errstr);
		return(1);
	}

	if (!newflag)
		disp_dev = map[slot].disp_dev;
	else
		n_dev++;
	if (strncmp(mouse, "m320", 4) == 0){
		map[slot].type = M320;
	}else 
	if (strncmp(mouse, "bmse", 4) == 0){
		map[slot].type = MBUS;
	}else  {
		map[slot].type = MSERIAL;
	}
	strcpy(table[slot].disp, terminal);
	map[slot].disp_dev = disp_dev;
	strcpy(table[slot].mouse, mouse);
	ntdcnt++;
	if (debug)
		fprintf(stderr, (char*)gettxt(":89","add_entry: n_dev=%d, slot=%d, terminal=%s, mouse=%s\n"), n_dev, slot, terminal, mouse);
	return(0);
}

/*
 *	Description:
 */

unconfig_mod(mse_name)
char *mse_name;

{
	/* This routine unconfigures the module in the kernel
	*/

	FILE		*fopen(), *fp, *fp2;
		char		name[10], f2[10], f3[10], f4[10], f5[10], f6[10],
			f7[10], f8[10], f9[10], f10[10], f11[10];
	char		*pathname, *tmpfile;
	char		buffer[250];
	struct stat	sbuf;
	int		notfound=1;

	if (debug)
		fprintf(stderr,(char*)gettxt(":90", "enter unconfig_mod: mouse=%s\n"), mse_name);

	if (!(pathname=(char *)(malloc(strlen(mse_name)+128)))) 
		_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
	if (!(tmpfile=(char *)(malloc(strlen(mse_name)+128)))) 
		_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
	sprintf(pathname,"/etc/conf/sdevice.d/%s",mse_name);
	sprintf(tmpfile,"/etc/conf/sdevice.d/T%s",mse_name);

	if ((fp = fopen(pathname,"r")) == NULL)
		_fatal_error((char*)gettxt(":95","unconfig_mod: can't open sdevice.d file"));
	if ((fp2 = fopen(tmpfile,"w")) == NULL)
		_fatal_error((char*)gettxt(":96","unconfig_mod: can't open tmp file"));
	while (fgets(buffer,250, fp)) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			fputs(buffer, fp2);
			continue;
		}
		if (strcmp(name, mse_name) == 0) 
			fprintf(fp2,"%s	N	%s	%s	%s	%s	%s	%s	%s	%s	%s\n",name,f3,f4,f5,f6,f7,f8,f9,f10,f11);
	}

	
	fclose(fp);
	fclose(fp2);

	/* Get the original file attributes */
	(void)stat(pathname, &sbuf);

	rename(tmpfile, pathname);

	/* Change back to original file attributes */
	(void)chmod(pathname, sbuf.st_mode);
	(void)chown(pathname, sbuf.st_uid, sbuf.st_gid);

	/* now make sure module being unconfigured isn't loaded */
	sprintf(pathname,"modadmin -U %s 1>/dev/null 2>&1",mse_name);
	(void) system(pathname);

	free(pathname);
	free(tmpfile);
	if (debug)
		fprintf(stderr,(char*)gettxt(":91","exit unconfig_mod:mouse=%s\n"),mse_name);
	return(0);
}

/*
 *	Description:
 */

config_mod(mouse_dev, irq)
char *mouse_dev;
int irq;

{
	char		buffer[250];
	FILE		*fopen(), *fp, *fp2;
		char		name[10], f2[10], f3[10], f4[10], f5[10], f6[10],
			f7[10], f8[10], f9[10], f10[10], f11[10];
	char		*pathname, *tmpfile, *command;
	struct stat	sbuf;
	int		slot;
	dev_t		mse_dev;
	char		mouse[10];

	int 		ret; /* return from RMgetbrdkey */
	if (debug)
		fprintf(stderr,(char*)gettxt(":92","entering config_mod: mouse=%s,irq=%d\n"), mouse_dev,irq);
	if (strcmp(mouse_dev, "m320") && strcmp(mouse_dev, "bmse"))
		strcpy(mouse, "smse");
	else	strcpy(mouse, mouse_dev);
	if (!(pathname=(char *)(malloc(strlen(mouse)+128))))
		_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
	if (!(tmpfile=(char *)(malloc(strlen(mouse)+128))))
		_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
	sprintf(pathname,"/etc/conf/sdevice.d/%s",mouse);
	sprintf(tmpfile,"/etc/conf/sdevice.d/T%s",mouse);

	if (!(command=(char *)malloc(strlen(mouse)+256)))
		_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));

	if (!bus_okay) {
		if (irq != 0 ) {
				sprintf(command,"FOO=`/etc/conf/bin/idcheck -r -v %d `\n[ \"$FOO\" = \"%s\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1",irq,mouse);
			if (system(command) !=  0)
				_fatal_error((char*)gettxt(":93","irq conflicting\n"));
		}
	
		/* if bmse addr check failed, return */
		if ( strcmp("bmse", mouse ) == 0 ) {

		/* run idcheck for bus mouse address 0x23c through 0x23f */
				sprintf(command,"FOO=`/etc/conf/bin/idcheck -r -a -l 0x23c -u 0x23f `\n[ \"$FOO\" = \"bmse\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1");
			if (system(command) != 0)
				_fatal_error((char*)gettxt(":94","address 0x23c through 0x23f already in use.\n"));
		}
	}
	
	if ((fp2 = fopen(tmpfile,"w")) == NULL) {
		_fatal_error((char*)gettxt(":40","can't create temporary sdevice file"));
	}
	if ((fp = fopen(pathname,"r")) == NULL) {
		_fatal_error((char*)gettxt(":41","can't re-open sdevice.d file"));
	}
	while (fgets(buffer,250, fp)) {
		if (sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			fputs (buffer, fp2);
			continue;
		}
		if (strcmp(name, mouse) == 0) {
		   if (irq != 0)
			fprintf(fp2,"%s	Y	%s	%s	%s	%d	%s	%s	%s	%s	%s\n",name,f3,f4,f5,irq,f7,f8,f9,f10,f11);
		   else
			fprintf(fp2,"%s	Y	%s	%s	%s	%s	%s	%s	%s	%s	%s\n",name,f3,f4,f5,f6,f7,f8,f9,f10,f11);
		}
		else 
			fputs(buffer, fp2);
	}

	
	fclose(fp);
	fclose(fp2);

	/* Get the original file attributes */
	(void)stat(pathname, &sbuf);

	rename(tmpfile, pathname);

	/* Change back to original file attributes */
	(void)chmod(pathname, sbuf.st_mode);
	(void)chown(pathname, sbuf.st_uid, sbuf.st_gid);

	/* Added entry in the resource manager data base */
	if ((strcmp("bmse", mouse ) == 0 ) || (strcmp("m320",mouse) == 0)) {
		rm_key_t key;
		char buf[128];

		if (RMopen(O_RDWR)) {
			_fatal_error((char*)gettxt(":97","RMopen() failed\n"));
		}

		(void) RMbegin_trans(key, RM_READ);
		ret = RMgetbrdkey(mouse, 0, &key);
		(void) RMend_trans(key);
		if (ret == ENOENT) {

			RMnewkey(&key);
			(void) RMend_trans(key);
			(void) RMbegin_trans(key, RM_RDWR);

			sprintf(buf, "%s", name);
			if (RMputvals(key, CM_MODNAME, buf)) {
				_fatal_error((char*)gettxt(":98","RMputvals() CM_MODNAME failed\n"));
			}
	
			sprintf(buf, "%s", f3);
			if (RMputvals(key, CM_UNIT, buf)) {
				_fatal_error((char*)gettxt(":99","RMputvals() CM_UNIT failed\n"));
			}
	
			sprintf(buf, "%s", f4);
			if (RMputvals(key, CM_IPL, buf)) {
				_fatal_error((char*)gettxt(":100","RMputvals() CM_IPL failed\n"));
			}
	
			sprintf(buf, "%s", f5);
			if (RMputvals(key, CM_ITYPE, buf)) {
				_fatal_error((char*)gettxt(":101","RMputvals() CM_ITYPE failed\n"));
			}
	
			sprintf(buf, "%d", irq);
			if (RMputvals(key, CM_IRQ, buf)) {
				_fatal_error((char*)gettxt(":102","RMputvals() CM_IRQ failed\n"));
			}
	
			sprintf(buf, "%s %s ", f7, f8);
			if ((buf[0]!='0') && (buf[1]!='0') && (buf[2]!='0')) {
				if (RMputvals(key, CM_IOADDR, buf)) {
					_fatal_error((char*)gettxt(":103","RMputvals() CM_IOADDR failed\n"));
				}
			}

			sprintf(buf, "%d", CM_BUS_ISA );
			if ( RMputvals(key, CM_BRDBUSTYPE, buf) ) {
				_fatal_error((char*)gettxt(":104","RMputvals() CM_BRDBUSTYPE failed\n"));
			}
			RMend_trans(key);
		} else {
			(void) RMbegin_trans(key, RM_RDWR);
			(void) (RMdelvals(key, CM_IRQ));

			sprintf(buf, "%d", irq);
			if (RMputvals(key, CM_IRQ, buf)) {
				_fatal_error((char*)gettxt(":105","RMputvals() CM_IRQ failed\n"));
			}
			RMend_trans(key);
		}

		RMclose();
		/* L002 - added -f flag to idconfupdate(1M)	*/
		sprintf(command,"/etc/conf/bin/idconfupdate -f 1>/tmp/mse.log 2>&1");
		(void) system(command);
	}

	/* run idbuild to configure module */
	sprintf(command, "modadmin -U %s 1>/tmp/mse.log 2>&1",mouse);
	(void) system(command);
	sprintf(command, "/etc/conf/bin/idbuild -M %s 1>/tmp/mse.log 2>&1",mouse);
	if (system(command) != 0) {
		unconfig_mod(mouse);
		/* delete entry from RM data base */
		delete_rmkey(mouse);
		_fatal_error((char*)gettxt(":106","idbuild -M failed\n"));
	}

	/* Load module to ensure that the mouse is installed and ready for use */
	sprintf(command, "modadmin -l %s 2>/tmp/mse.log 1>&2",mouse);
	if (system(command) != 0) {
		_fatal_error((char*)gettxt(":107","modadmin -l failed\n"));
	}

	/* Unload it. We don't want it to be loaded all the time. */
	sprintf(command, "modadmin -U %s 1>/tmp/mse.log 2>&1",mouse);
	(void) system(command);
	free(pathname);
	free(tmpfile);
	free(command);

	/* check if device node is created and update mapping table */
	if (get_dev(mouse_dev, &mse_dev) < 0) {
		sprintf(errstr,(char*)gettxt(":82","%s is not a valid mouse device.\n"),mouse_dev);
		_fatal_error(errstr);
	}

	for (slot = 0; slot < n_dev; slot++) {
		if (!strcmp(mouse_dev, table[slot].mouse)) {
			map[slot].mse_dev = mse_dev;
			break;
		}
	}

	if (debug)
		fprintf(stderr,(char*)gettxt(":108","exit config_mod:mouse=%s,irq=%d\n"),mouse,irq);
}

/*
 *	Description:
 */

int
main_menu()
{
	char	ch, terminal[MAXDEVNAME];
	int	oldrow, oldcol, retval, slot;
	dev_t	dummy;
	char	interrupt[MAXDEVNAME];
	char	*cmd;
	char	ans[256];
	int	valid_term = 0;
	char 	*chptr;
	char 	C,msc_mice[256];

	c = show_menu(0);

	move(0,0);
	erase();
	show_table();
	getyx(stdscr, row, col);
	row++;

	while(c) {
	chptr = (char *) gettxt(":118", "B");
	if ((C = toupper(*chptr)) == c) {
		C = 'B';
		break;
	}
	chptr = (char *) gettxt(":119", "P");
	if ((C = toupper(*chptr)) == c) {
		C = 'P';
		break;
	}
	chptr = (char *) gettxt(":120", "S");
	if ((C = toupper(*chptr)) == c) {
		C = 'S';
		break;
	}
	chptr = (char *) gettxt(":121", "T");
	if ((C = toupper(*chptr)) == c) {
		C = 'T';
		break;
	}
	chptr = (char *) gettxt(":124", "E");
	if ((C = toupper(*chptr)) == c) {
		if (cursing)
			endwin();
		exit(1);
	}
	chptr = (char *) gettxt(":123", "U");
	if ((C = toupper(*chptr)) == c) {
		C = 'U';
		break;
	}
	chptr = (char *) gettxt(":122", "R");
	if ((C = toupper(*chptr)) == c) {
		C = 'R';
		break;
	}
	}
	getyx(stdscr, row, col);
	row++;
	if (suserflg) {
		beep();
		mvaddstr(row,col,(char*)gettxt(":109","Permission denied, changes will not be accepted."));
		enter_prompt();
		return(1);
	}


	switch (C) {
	case 'R':
		if (rmflg || addflg) {
			warn_err((char*)gettxt(":110","Please select Update or Exit."));
			break;
		}

		mvaddstr(row++,col,(char*)gettxt(":111","Enter the display terminal from which the mouse will be removed,"));
		mvaddstr(row++,col,(char*)gettxt(":112","or strike the ENTER key to return to the main menu."));
		get_info(gettxt(":45","Display terminal:  "), terminal);
		row++;
		if(strcmp(terminal, "console") == 0)
			strcpy(terminal, "vt00");
		if (terminal[0] == '\0')
			break;
		if ((retval = delete_entry(terminal)) < 0) {
			row++;
			if(retval == -1)
				mvaddstr(row,col,(char*)gettxt(":46","There is no mouse assigned for this terminal."));
			else if(retval == -2)
				mvaddstr(row,col,(char*)gettxt(":47","Cannot remove mouse while busy."));
			else if(retval == -3)
				mvaddstr(row,col,(char*)gettxt(":48","Not a valid display terminal."));
			enter_prompt();
			break;
		}
		else rmflg++;
		break;
	case 'T':
		if (rmflg || addflg) {
			warn_err((char*)gettxt(":110","Please select Update or Exit."));
			break;
		}
		mvaddstr(row++,col,(char*)gettxt(":49","Please try using your mouse when the next screen appears."));
		get_info(gettxt(":50","Strike the ENTER key when ready:  "), terminal);
		if ((retval = test_entry()) > 0) {
			row++;
			mvaddstr(row,col,(char*)gettxt(":51","Unable to detect mouse."));
			enter_prompt();
			mvprintw(0,0,"");
			clear();
			refresh();
			break;
		}
		mvprintw(0,0,"");
		clear();
		refresh();
		row++;
		
		break;
	case 'P': 
	case '3':
		if (rmflg || addflg) {
			warn_err((char*)gettxt(":110","Please select Update or Exit."));
			break;
		}
		if ((slot = lookup_disp("vt00")) >= 0)
			if(msebusy[slot]){
   				mvaddstr(row++,col,(char*)gettxt(":52","Mouse currently assigned to console is busy, change will not be accepted. "));
				enter_prompt();
				break;
			}
		if (add_entry("vt00", "m320"))
			break;
		irq = 12;
		strcpy(mouse, "m320");
		addflg++;
		break;
	case 'B': 
		if (rmflg || addflg) {
			warn_err((char*)gettxt(":110","Please select Update or Exit."));
			break;
		}
				
		getyx(stdscr, row, col);
		oldrow = row;
		oldcol = col;
	
		while (1) {
			mvaddstr(row++,col,(char*)gettxt(":56","Enter the interrupt to be used for the Bus mouse."));
			mvaddstr(row++,col,(char*)gettxt(":44","or strike the ENTER key to return to the main menu."));
			clrtobot();
			refresh();
			get_info(gettxt(":57","Interrupt (i.e. 2, 3, 4, or 5:): "),interrupt);
			row++;

			if (interrupt[0] == '\0')
				return(1);

			irq = atoi(interrupt);
			if (irq == 2)
				irq = 9;
			if (irq != 9 && irq != 3 && irq != 4 && irq != 5) {
				beep();
				continue;
			}

			if (!(cmd=(char *)(malloc(1024))))
				_fatal_error((char *)gettxt(":79","Cannot allocate space. Please try later.\n"));
				sprintf(cmd,"FOO=`/etc/conf/bin/idcheck -r -v %d`\n[ \"$FOO\" = \"bmse\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1",irq);
			if (system(cmd) != 0) {
				mvaddstr(row++,col,(char*)gettxt(":58","Interrupt already in use."));
				for (;;) {
					mvaddstr(row,col,(char*)gettxt(":59","Do you wish to select another? [y or n] "));
					clrtobot();
					refresh();
					attron(A_BOLD);
					getstr(ans);
					attroff(A_BOLD);
					ch = toupper(ans[0]);
					if (ch == 'Y' ) {
						row=oldrow;
						col=oldcol;
						break;
					} 
					else
						if (ch == 'N') 
							return(1);
						else
							beep();
				}
			}
			else 
				break;
		}
		if ((slot = lookup_disp("vt00")) >= 0)
			if(msebusy[slot]){
   				mvaddstr(row++,col,(char*)gettxt(":52","Mouse currently assigned to console is busy, change will not be accepted. "));
				enter_prompt();
				break;
			}
		if (add_entry("vt00", "bmse"))
			break;
		strcpy(mouse, "bmse");
		addflg++;
		break;
	case 'S':
		if (rmflg || addflg) {
			warn_err((char*)gettxt(":110","Please select Update or Exit."));
			break;
		}

		while (1) {
			getyx(stdscr, row, col);
			row++;
			oldrow = row;
			oldcol = col;
			clrtobot();
			mvaddstr(row++,col,(char*)gettxt(":60","Enter the display terminal that will be using the mouse,"));
			mvaddstr(row++,col,(char*)gettxt(":44","or strike the ENTER key to return to the main menu."));
			get_info(gettxt(":61","Display terminal (i.e. console, s0vt00, etc.):  "), terminal);
			row++;
			if (terminal[0] == '\0')
				return(1);
			if (strcmp(terminal, "console") == 0)
				strcpy(terminal, "vt00");
			if (lookup_disp(terminal) >= 0) {
				mvaddstr(row++,col,(char*)gettxt(":62","Requested display terminal is already configured to use a mouse."));
				for (;;) {
					mvaddstr(row,col,(char*)gettxt(":63","Do you wish to continue? [y or n] "));
					clrtobot();
					refresh();
					attron(A_BOLD);
					getstr(ans);
					attroff(A_BOLD);
					ch = toupper(ans[0]);
					if (ch == 'Y' || ch == 'N')
						break;
					beep();
				}
				if (ch == 'N')
					return(1);
				move(row+=2,col);
			}
			if (get_dev(terminal, &dummy) < 0) {
				beep();
				mvaddstr(row,col,(char *)gettxt(":80","Requested display terminal is not valid.\n"));
				refresh();
				row = oldrow;
				col = oldcol;
				continue;
			}
			if (strcmp(terminal,"vt00") == 0) {
				valid_term = 1;
				break;
			}
			else {
				if (strncmp(terminal,"s",1)==0) 
					if (strchr(terminal,'v')!=NULL) {
						valid_term = 2;
						break;
					}
					else {
						row++;
						row = oldrow;
						mvaddstr(row,col,(char *)gettxt(":80","Requested display terminal is not valid.\n"));
						col = oldcol;
						continue;
					}
			}
		}
		mvaddstr(row++,col,(char*)gettxt(":64","Enter the device that the mouse will be attached to,"));
		mvaddstr(row++,col,(char*)gettxt(":44","or strike the ENTER key to return to the main menu."));
		if (valid_term == 1)
			get_info(gettxt(":65","Mouse device (i.e. tty00, tty01): "), mouse);
		else
			get_info(gettxt(":66","Mouse device (i.e. s0tty0, s3tty1): "),mouse);
		row++;
		if (mouse[0] == '\0')
			break;
		if (valid_term == 1) {
			if (strncmp(mouse,"ttyh",4)==0||strncmp(mouse,"ttys",4)==0||strncmp(mouse,"tty",3)!=0 || (strchr(mouse,'0')==NULL && strchr(mouse,'1')==NULL) ){
				warn_err((char *)gettxt(":81","Requested display/mouse pair is not valid.\n"));
				break;
			}
		}
		else {
			if (strncmp(mouse,"s",1) != 0 || strchr(mouse,'v') != NULL || strchr(mouse,'l') != NULL ) {
				warn_err((char *)gettxt(":81","Requested display/mouse pair is not valid.\n"));
				break;
			}
		}
		if (add_entry(terminal, mouse))
			break;
		addflg++;
		irq = 0;
		for (;;) {
		mvaddstr(row,col,(char*)gettxt(":132","Is your mouse configured to Mouse Systems (MSC compatible) mode ? [y or n] : "));
			clrtobot();
			refresh();
			attron(A_BOLD);
			getstr(msc_mice);
			attroff(A_BOLD);
			ch = toupper(msc_mice[0]);
			if (ch == 'Y' || ch == 'N')
				break;
			beep();
		}
		init_tunable(ch);
		row++;
		break;
	case 'U':
		if (addflg) 
			config_mod(mouse, irq);
		else	
			/*
			 * Do not try to remove the resmgr key
			 * for the serial mouse, this does not
			 * have entries in the resmgr database.
			 */
			if (rmflg) {
				if (strcmp(mse_name,"smse"))
					delete_rmkey(mse_name);
				unconfig_mod(mse_name);
			} else
				return(0);
		download_table();
		return(0);
	}
	return 1;
}


/*
 *	Description:
 */

void
interact()
{
	initscr();
	cursing = 1;
	print = (int (*)(const char *, ...))printw;

	do {
		erase();
		show_table();
	} while (main_menu());

	endwin();
}

/* 
 * 	char 
 *	show_menu(int flag)
 *
 *	Description:
 */

/* 
 * L001 
 * Don't display the test option if there is no mouse type configured, its 
 * selection causes a 15s hang until the timout frees the screen.
 */

show_menu(flag)
int flag;
{
	char	ch;
	char	ans[256];
	char	strbuf[256];
	char 	remove_ch;
	int		ndevs;

	getyx(stdscr, row, col);
	row++;

	mvaddstr(row++, col, (char*)gettxt(":67","Select one of the following:"));
	col += 5;
	sprintf(strbuf, "%s) %s", gettxt(":118", "B"), gettxt(":125", "Bus mouse add"));
	mvaddstr(row++, col, strbuf);
	sprintf(strbuf, "%s) %s", gettxt(":119", "P"), gettxt(":126", "PS2 mouse add"));
	mvaddstr(row++, col, strbuf);
	sprintf(strbuf, "%s) %s", gettxt(":120", "S"), gettxt(":127", "Serial mouse add"));

	if (ntdcnt > 0) {						/* L001 */
		mvaddstr(row++, col, strbuf);
		sprintf(strbuf, "%s) %s", gettxt(":121", "T"), gettxt(":128", "Test your mouse configuration"));
	}

	mvaddstr(row++, col, strbuf);
	if (n_dev)	{
		sprintf(strbuf, "%s) %s", gettxt(":122", "R"), gettxt(":129","Remove a mouse"));
		mvaddstr(row++, col, strbuf);
	}
	sprintf(strbuf, "%s) %s", gettxt(":123", "U"), gettxt(":130","Update mouse configuration and quit"));
	mvaddstr(row++, col, strbuf);
	sprintf(strbuf, "%s) %s", gettxt(":124", "E"), gettxt(":131","Exit (no update)"));
	mvaddstr(row++, col, strbuf);
	col -= 5;
	if (!flag) {
		for (;;) {
			mvaddstr(row, col, (char*)gettxt(":75","Enter Selection:  "));
			clrtobot();
			refresh();
			attron(A_BOLD);
			getstr(ans);
			attroff(A_BOLD);
			ch = toupper(ans[0]);
			
			sprintf(strbuf,"%c%c%c%c%c%c",
				toupper(*(char *)gettxt(":118","B")),
				toupper(*(char *)gettxt(":119","P")),
				toupper(*(char *)gettxt(":120","S")),
				toupper(*(char *)gettxt(":121","T")),
				toupper(*(char *)gettxt(":123","U")),
				toupper(*(char *)gettxt(":124","E")));
			
			remove_ch = toupper(*(char*)gettxt(":122","R"));
			if (strchr(strbuf, ch) || (n_dev && (ch == remove_ch)))
				break;
			beep();
	   	}
	   	row++;
	   	return(ch);
	}
}


/*
 *	void
 *	delete_rmkey (char *mouse_name)
 *
 *	Description: 	This function removes the entry in the resmgr for the
 *			specified key.
 */
 
void
delete_rmkey(char *mouse_name)
{
	int 	rm_error;
	rm_key_t	mse_key;

		if(RMopen(O_RDWR)) {
			fprintf(stderr,(char*)gettxt(":116", "\n RMopen() failed for %s \n"), mouse_name);
			return;
		}
		RMbegin_trans(0, RM_READ);
		if(( rm_error = RMgetbrdkey(mouse_name,0,&mse_key)) == ENOENT) {
			fprintf(stderr,(char*)gettxt(":113","\n RMgetbrdkey() failed, %s \n"),mouse_name);
			RMend_trans(0);
		} else {
		RMend_trans(0);
		if(( rm_error = RMdelkey(mse_key)) != 0) {
					fprintf(stderr, (char*)gettxt(":115","\n RMdelkey() failed for %s \n"), mouse_name);
		}
		}
		RMclose();
}

/* 
 * int init_tunable(char msc) 
 * 
 * Calling/Exit: 
 *	Called with 'Y' or 'N' as argument. Set smse_MSC_selected to 0/1 if 
 *	user selects N/Y to "Mouse Systems Corporation" device question. 
 * 
 * Remarks: 
 * 	Ideally would parse C code to set variable smse_MSC_selected to 
 *	correct value, other pattern matching code is not robust, so recreate
 *	file for each invocation. Creates dependency of smse space.c editing
 *	to be reflected here, as comment notes. 
 */

/* L003 { */

int
init_tunable(char msc)
{
	int		fd; 
	char 		ytxt[] = SPACE_Y; 
	char 		ntxt[] = SPACE_N; 
	size_t		nbtxt, nwr; 
	

	/*
	 * Open the file for write only, truncating it to zero bytes in 
	 * size if it already exists, and make all write operations commit
	 * prior to write(2) calls returning.
	 */
	
	if ((fd = open( TFILE, O_WRONLY|O_SYNC|O_TRUNC )) < 0) { 

		/* 
		 * Whinge and quit if cannot create file.
		 */

		_fatal_error((char*)gettxt(":133",
					"Error opening space.c file.")); 
	}
	
	/*
	 * Write out the tunables to set MSC selected or not.
	 * Don't include the text's trailing NUL terminator: it kills 
	 * the link kit compiler!
	 */	

	if (msc == 'Y')	{ 
		nbtxt = sizeof(ytxt) - 1 ; 
		nwr = write (fd, (const void *)ytxt, nbtxt); 
	} else {
		nbtxt = sizeof(ntxt) - 1 ; 
		nwr = write (fd, (const void *)ntxt, nbtxt); 
	}

	/*
	 * Verify write succeeded, error and quit if not. 
	 */

	if (nwr != nbtxt) { 
		_fatal_error((char*) 
			gettxt(":138","Cant write space.c variable")); 
	}

	close (fd); 

}

/* } L003 */

