#ident	"@(#)vtgetty.c	1.5"

#include <stdio.h>
#include "sys/types.h"
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "sys/vt.h"
#include "sys/termio.h"
#include "sys/stat.h"
#include "errno.h"
#include "varargs.h"

extern int errno;
extern int optind;
extern char *optarg;

int	Vtdes,
	Update = 1;
FILE	*Vtf_p;

void
relsignal(x)
int x;
{

	Update = 0;
	signal(SIGUSR1, relsignal);
	safe_ioctl(Vtdes, VT_RELDISP, 1);
}

void
acqsignal(x)
int x;
{

	Update = 1;
	signal(SIGUSR2, acqsignal);
	safe_ioctl(Vtdes, VT_RELDISP, VT_ACKACQ);
}


main(argc, argv)
int argc;
char *argv[];
{
	int	high = 0, cnt, arg, killall = 0;
	char	ans, *devstr_p, *str_p;
	ushort	vt_mask;
	struct vt_mode	vtmode;
	struct vt_stat	vtinfo;
	struct stat	dstat;

	while (getopt(argc, argv, "") != EOF)	/* skip past options */
		;
	devstr_p = argv[optind];	/* get required argument */
	if (strchr(devstr_p, '/') == (char *)NULL) { /* prepend /dev/ to name */
		str_p = (char *)malloc(strlen(devstr_p) + 6);
		if (str_p == (char *)NULL) {
			perr("Cannot malloc space for device name\n");
			exit(1);
		}
		strcpy(str_p, "/dev/");
		devstr_p = strcat(str_p, devstr_p);
	}


	/*
	 * if we are invoked with /dev/console it is possible that
	 * we are not really  on the VT device. In this case we want
	 * to exec /sbin/getty right away. Previously sysmsg
	 * was used to determine that /dev/console is not VT device. 
	 * however since sysmsg doesn't return flags anymode this is
	 * not possible. We depend on KIOCINFO failing below as
	 * the decision factor to immediatly exec /etc/getty.
	 */
	argv[0] = "/sbin/getty";
	errno = 0;
	if ((Vtdes = open(devstr_p, O_RDWR)) < 0) {
		perr("Cannot open %s: errno = %d\n", devstr_p, errno);
		if (execv("/sbin/getty", argv) < 0)
			exit(1);
	}
	if (ioctl(Vtdes,KIOCINFO,0) < 0) {
		if (execv("/sbin/getty", argv) < 0)
			exit(1);
	}
	if ((Vtf_p = fdopen(Vtdes, "w")) == (FILE *)NULL) {
		perr("Cannot convert %s to a file pointer\n", devstr_p);
		if (execv("/sbin/getty", argv) < 0)
			exit(1);
	}
	vtmode.mode = VT_PROCESS;
	vtmode.relsig = SIGUSR1;
	vtmode.acqsig = SIGUSR2;
	vtmode.frsig = SIGUSR1;
	for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++) {
		if (cnt == SIGCLD)
			continue;
		signal(cnt, SIG_IGN);	/* ignore all signals */
	}
	signal(SIGUSR1, relsignal);	/* reset these signals */
	signal(SIGUSR2, acqsignal);
	errno = 0;
	if (fstat(Vtdes, &dstat) < 0) {
		perr("Cannot stat %s: errno = %d\n", devstr_p, errno);
		exit(1);
	}
	vt_mask = (1 << VTINDEX(dstat.st_rdev));
	if (safe_ioctl(Vtdes, VT_GETSTATE, &vtinfo) < 0) { 
		perr("Cannot determine vt status: errno = %d\n", errno);
		for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++) {
			if (cnt == SIGCLD)
				continue;
			signal(cnt, SIG_DFL);
		}
		if (execv("/sbin/getty", argv) < 0)
			exit(1);
	}
	vtinfo.v_state &= ~vt_mask;
	if (vtinfo.v_state) /* only enter process mode if necessary */
		safe_ioctl(Vtdes, VT_SETMODE, &vtmode);
 	while (killall != 1 && vtinfo.v_state) {
		while (!Update)
			pause();
		if (safe_ioctl(Vtdes, VT_GETSTATE, &vtinfo) < 0) {
			perr("Cannot determine vt status: errno = %d\n", errno);
			exit(1);
		}
		vtinfo.v_state &= ~vt_mask;
		if (vtinfo.v_state) {
			high = display_list(vtinfo.v_state);
			if ((killall = kill_msg()) == 0)
				safe_ioctl(Vtdes, VT_ACTIVATE, high);
		}
	}
	if (killall == 1) {
		vtinfo.v_signal = SIGTERM;
		safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		vtinfo.v_signal = SIGHUP;
		safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		sleep(2);
		vtinfo.v_signal = SIGKILL;
		safe_ioctl(Vtdes, VT_SENDSIG, &vtinfo);
		sleep(2);
		if (safe_ioctl(Vtdes,
			VT_GETSTATE, &vtinfo) < 0) {
			perr("Cannot determine vt status: errno = %d\n", errno);
		}
		else {
			vtinfo.v_state &= ~vt_mask;
			if (vtinfo.v_state)
				display_list(vtinfo.v_state);
		}
	}
	vtmode.mode = VT_AUTO;
	safe_ioctl(Vtdes, VT_SETMODE, &vtmode);
	for (cnt = SIGHUP; cnt <= SIGPOLL; cnt++)
		signal(cnt, SIG_DFL);
	close(Vtdes);
	if (execv("/sbin/getty", argv) < 0)
		exit(1);
}


int
display_list(vtinfo)
ushort vtinfo;
{
	register int high = 0, cnt;
	
	fprintf(Vtf_p, "\007\033c");
	fprintf(Vtf_p, "The following virtual terminals are still open\n\n");
	for (cnt = 0; cnt < VTMAX; cnt++) {
		if (vtinfo & (1 << cnt)) {
			fprintf(Vtf_p, "\t/dev/vt%02d\n\n", cnt);
			high = (high > cnt) ? high : cnt;
		}
	}
	fflush(Vtf_p);
	return(high);
}

int
kill_msg()
{
	char ans;

	fprintf(Vtf_p, "You may close these virtual terminals all at once\n");
	fprintf(Vtf_p, "or one by one.  If you decide not to have all virtual\n");
	fprintf(Vtf_p, "terminals closed at once you will be switched to each\n");
	fprintf(Vtf_p, "of the currently opened virtual terminals so that you\n");
	fprintf(Vtf_p, "can exit your application and close the virtual terminal.\n\n");
	fprintf(Vtf_p, "Type 'y' followed by ENTER if you want your virtual\n");
	fprintf(Vtf_p, "terminals closed at once.  Type 'n' followed by ENTER\n");
	fprintf(Vtf_p, "if you want to close them yourself: ");

	fflush(Vtf_p);
	errno = 0;
	if (read(Vtdes, &ans, 1) < 0 && errno == EINTR)
		return(-1);
	fprintf(Vtf_p, "\n");
	fflush(Vtf_p);
	if (ans == 'y')
		return(1);
	else if (ans == 'n')
		return(0);
}

int
perr(va_alist)
va_dcl
{
	register char *fmt_p;
	va_list v_Args;
	FILE *cons;

	if ((cons = fopen("/dev/sysmsg", "w")) == NULL)
	    cons = fopen("/dev/console", "w");
	va_start(v_Args);
	fmt_p = va_arg(v_Args, char *);
	(void)vfprintf(cons, fmt_p, v_Args);
	va_end(v_Args);
	fclose(cons);
}

safe_ioctl(des, cmd, arg)
int des, cmd, arg;
{
	register int cnt = 0, rv;

	errno = 0;
	while ((rv = ioctl(des, cmd, arg)) < 0 && errno == EINTR) {
		if (++cnt > 16) {
			perr("vtgetty: ioctl failure\n");
			exit(1);
		}
		errno = 0;
	}
	return(rv);
}
