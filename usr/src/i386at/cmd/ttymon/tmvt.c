#ident	"@(#)tmvt.c	1.2"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <sys/termio.h>
#include <sys/stat.h>
#include <errno.h>
#include <pfmt.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/utssys.h>
#include <nl_types.h>
#include <langinfo.h>

#define	SERVER_FILE	"/dev/X/server.0.pid"
#define XDM_FILE1	"/dev/X/xdm-serverPid"
#define XDM_FILE2	"/dev/X/xdm-userPid"

extern int log();

extern int errno;

static	void	relsignal();
static	void	acqsignal();
static	int	display_list();
static	int	safe_ioctl();
static	int	kill_msg();
static  int 	check_graphical_login();
static	int	check_pid();
static  pid_t	getPID();

static	pid_t pid_server;  /* pid of process that owns /dev/vt01 */
static	pid_t pid_xdm;  	  /* pid of process that owns /dev/vt01 */

static int	Vtdes;
static int      Update = 1;
static FILE	*Vtf_p;
static int	acqsig = 0;
static int	graphical_login = 0; /* set if graphical login in-progress */


static void
relsignal(x)
int x;
{

	Update = 0;
	signal(SIGUSR1, relsignal);
	(void)safe_ioctl(Vtdes, VT_RELDISP, 1);
}

static void
acqsignal(x)
int x;
{
	acqsig++;
	Update = 1;
	signal(SIGUSR2, acqsignal);
	(void)safe_ioctl(Vtdes, VT_RELDISP, VT_ACKACQ);
}


void
killvts(device)
char	*device;
{
	int	high = 0, cnt, arg, killall = 0;
	char	ans, *devstr_p, *str_p;
	ushort	vt_mask;
	struct vt_mode	vtmode;
	struct vt_stat	vtinfo;
	struct stat	dstat;

	if (device == (char *)NULL || *device == '\0')
		return;

	errno = 0;
	if ((Vtdes = open(device, O_RDWR)) < 0) {
		(void)log(MM_ERROR, ":1135:Failed to open device %s", device);
		return;
	}

	if (ioctl(Vtdes,KIOCINFO,0) < 0) {
		close(Vtdes);
		return;
	}

	if ((Vtf_p = fdopen(Vtdes, "w")) == (FILE *)NULL) {
		(void)log(MM_ERROR, ":1137:fdopen failed");
	}

	vtmode.mode = VT_PROCESS;
	vtmode.relsig = SIGUSR1;
	vtmode.acqsig = SIGUSR2;
	vtmode.frsig = SIGUSR1;

	for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++) {
		if (cnt == SIGCLD || cnt == SIGINT)
			continue;
		sigset(cnt, SIG_IGN);	/* ignore all signals */
	}

	signal(SIGUSR1, relsignal);	/* reset these signals */
	signal(SIGUSR2, acqsignal);

	errno = 0;
	if (fstat(Vtdes, &dstat) < 0) {
		(void)log(MM_ERROR, ":1138:stat() failed on device %s", device);
		(void)fclose(Vtf_p);
		return;
	}

	vt_mask = (1 << VTINDEX(dstat.st_rdev));
	if (safe_ioctl(Vtdes, VT_GETSTATE, &vtinfo) < 0) { 
		(void)log(MM_ERROR, ":1139:Cannot determine vt status");
		for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++) {
			if (cnt == SIGCLD || cnt == SIGINT)
				continue;
			signal(cnt, SIG_DFL);
		}
		(void)fclose(Vtf_p);
		return;
	}

	vtinfo.v_state &= ~vt_mask;
	if (vtinfo.v_state) /* only enter process mode if necessary */
		(void)safe_ioctl(Vtdes, VT_SETMODE, &vtmode);
 	while (killall != 1 && vtinfo.v_state) {
		while (!Update)
			pause();
		if (safe_ioctl(Vtdes, VT_GETSTATE, &vtinfo) < 0) {
			log(MM_ERROR, ":1139:Cannot determine vt status");
			(void)fclose(Vtf_p);
			return;
		}
		vtinfo.v_state &= ~vt_mask;
		if (vtinfo.v_state) {
			high = display_list(vtinfo.v_state);
			if (high == 0 && graphical_login)
				break;
			if ((killall = kill_msg()) == 0)
				safe_ioctl(Vtdes, VT_ACTIVATE, high);
		}
	}
	if (killall == 1) {
		vtinfo.v_signal = SIGTERM;
		(void)safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		vtinfo.v_signal = SIGHUP;
		(void)safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		sleep(2);
		vtinfo.v_signal = SIGKILL;
		(void)safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		sleep(2);
		if (safe_ioctl(Vtdes,
			VT_GETSTATE, &vtinfo) < 0) {
			(void)log(MM_ERROR,
				":1139:Cannot determine vt status");
		}
		else {
			vtinfo.v_state &= ~vt_mask;
			if (vtinfo.v_state)
				display_list(vtinfo.v_state);
		}
	}

	if (acqsig && killall == -1)
		(void)fprintf(Vtf_p, "\007\033c");
	vtmode.mode = VT_AUTO;
	(void)safe_ioctl(Vtdes, VT_SETMODE, &vtmode);
	for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++)
		signal(cnt, SIG_DFL);
	(void)fclose(Vtf_p);
	return;
}


static int
display_list(vtinfo)
ushort vtinfo;
{
	register int high = 0, cnt;
	
	graphical_login = 0;
	(void)fprintf(Vtf_p, "\007\033c");
	if (check_graphical_login()) {
		graphical_login = 1;
	}
	(void)pfmt(Vtf_p, MM_NOSTD, ":1140:The following virtual terminals are still open\n\n");
	for (cnt = 0; cnt < VTMAX; cnt++) {
		if (vtinfo & (1 << cnt)) {
			if (graphical_login && cnt == 1) {
				pfmt(Vtf_p, MM_NOSTD, ":1150:\t/dev/vt%02d is used by Graphical Login\n\n", cnt);
				continue;
			}
			fprintf(Vtf_p, "\t/dev/vt%02d\n\n", cnt);
			high = (high > cnt) ? high : cnt;
		}
	}
	(void)fflush(Vtf_p);
	return(high);
}

int
kill_msg()
{
	char ans;
	char yes_lower;
	char no_lower;
	char *ptr;

	(void)pfmt(Vtf_p, MM_NOSTD, ":1141:You may close these virtual terminals all at once\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1142:or one by one.  If you decide not to have all virtual\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1143:terminals closed at once you will be switched to each\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1144:of the currently opened virtual terminals so that you\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1145:can exit your application and close the virtual terminal.\n\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1146:Type 'y' followed by ENTER if you want your virtual\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1147:terminals closed at once.  Type 'n' followed by ENTER\n");
	(void)pfmt(Vtf_p, MM_NOSTD, ":1148:if you want to close them yourself: ");

	(void)fflush(Vtf_p);
	errno = 0;
	if (read(Vtdes, &ans, 1) < 0 && errno == EINTR)
		return(-1);
	(void)fprintf(Vtf_p, "\n");
	(void)fflush(Vtf_p);
	/*
	 * get localized y/n responses, just in case, force it to lower.
	 */
	ptr = nl_langinfo(YESSTR);
	yes_lower = islower(ptr[0]) ? ptr[0] : tolower(ptr[0]);
	ptr = nl_langinfo(NOSTR);
	no_lower = islower(ptr[0]) ? ptr[0] : tolower(ptr[0]);
	
	if (ans == yes_lower)
		return(1);
	else if (ans == no_lower)
		return(0);
}

static int
safe_ioctl(des, cmd, arg)
int des, cmd, arg;
{
	register int cnt = 0, rv;

	errno = 0;
	while ((rv = ioctl(des, cmd, arg)) < 0 && errno == EINTR) {
		if (++cnt > 16) {
			log(MM_ERROR, ":1149:ioctl error in tmvt.c"); 
			return(-1);
		}
		errno = 0;
	}
	return(rv);
}

/*
 * This function returns 1 if graphical login (X process)
 * is the owner of /dev/vt01
 */

static int
check_graphical_login()
{
	FILE	 *fp;
	f_user_t users[40];
	int 	nusers;
	struct	stat 	stbuf;
	
	if (access(XDM_FILE2, EFF_ONLY_OK | F_OK) == 0)
		return(0);

	if ( (access(SERVER_FILE, EFF_ONLY_OK | F_OK) == -1) ||
		(access(XDM_FILE1, EFF_ONLY_OK | F_OK) == -1) ) {
		return (0);
			
	}
	pid_server = getPID(SERVER_FILE);
	if (pid_server == 0)
		return (0);
	pid_xdm	   = getPID(XDM_FILE1);
	if (pid_xdm == 0 || pid_xdm != pid_server)
		return (0);

	nusers = utssys("/dev/vt01", 0, UTS_FUSERS, users);

	if (nusers == -1)
		return(0);
	else
		return(check_pid(users, nusers));
}

static int
check_pid(users, nusers)
f_user_t *users;
int	 nusers;
{
	for ( ; nusers; nusers--, users++) {
		if (pid_server == users->fu_pid) {
			return(1);
		}
	}
	return(0);
}

static pid_t
getPID(file)
char	*file;
{
	FILE	*fp;
	char	buf[128];

	fp = fopen(file, "r");
	if (fp == (FILE *)NULL) {
		return(0);
	}
	if ( ! fgets(buf, sizeof buf, fp) ) {
		(void)fclose(fp);
		return(0);
	}
	(void)fclose(fp);
	return(atol(buf));
}
