#ident	"@(#)bootstrap.c	15.1	98/03/04"
/*
	This is the code for init, the first process run by the 
	ISL kernel. Parse INITSTATE (if it's a number), and pass it
	to do_install.
*/

#include <fcntl.h>
#include <signal.h>
#include <sys/uadmin.h>
#include <sys/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/bootinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/mkdev.h>
#include <sys/sysi86.h>
#include <sys/param.h>
#include <sys/mod.h>
#include <sys/mount.h>
#include <sys/fs/memfs.h>
#include <string.h>


#define FD_MAJOR	1
#define TRUE		1
#define FALSE		0


static void handler(int sig);
static void vt0_ksh(int do_sleep);
static int mod_reg();


static char *initstate_env=NULL;
static char mem_env[30];

int main(int argc, char *argv[], char *envp[])
{
	int 		ttyfd, i, fd; 
	dev_t		consdev;
	char		*p;
/*	char 		buffer[BOOTINFO_LOC+0x1000];
*/	int		memavailcnt;
	unsigned long 	memsize;	
	struct bootmem	*memavail;	


	(void) setuid((uid_t)0); /* root */
	(void) setgid((gid_t)3); /* sys */

	(void) sigset(SIGINT,  handler);
	(void) sigset(SIGFPE,  handler);
	(void) sigset(SIGEMT,  handler);
	(void) sigset(SIGUSR1, handler);
	(void) sigset(SIGHUP,  handler);
	(void) sigset(SIGQUIT, handler);
	(void) sigset(SIGALRM, handler);
	(void) sigset(SIGTERM, handler);
	(void) sigset(SIGUSR2, handler);

	for(i=0; i< 20; i++)
		(void) close(i);


	/*
		Parse the INITSTATE. Is it a number?
	*/
	i=TRUE;
	for(p=argv[2]; *p; p++)
		if ((*p < '0') || (*p > '9')) {
			i=FALSE;
			break;
		}

	/*
		Do we need to convert it?
	*/
	if (i) {
		/*
			It is a number. So it is in the old
			format, and must be converted to the new format.
			If bit 4 is set, set it to "ksh". 
			If bit 3 is set, append "all".
		*/
		int	n=0;
		
		initstate_env=(char *)malloc(20);
		if (!initstate_env) {
			printf("init: Internal error: can't malloc.\n");
			while(1) ;  /* hang here */
		}
		strcpy(initstate_env, "INITSTATE=");
		n=atoi(argv[2]);
		if (n & 4)
			strcat(initstate_env, "all ");
/*		if (n & 8)
			strcat(initstate_env, "ksh "); */
		putenv(initstate_env);
	} else {
		/*
			It is not a number, so it's in the new format.
			Pass it on down to do_install.
		*/
		int	len;

		len=strlen(argv[2]);
		initstate_env=(char *)malloc(len + 11);
		if (!initstate_env) {
			printf("init: Internal error: can't malloc.\n");
			while(1) ;  /* hang here */
		}
		strcpy(initstate_env, "INITSTATE=");
		strcat(initstate_env, argv[2]);
		putenv(initstate_env);
	}
		

	/*
		Figure out how much memory this machine has.
		Pass it to do_install as an enviornment variable.
	
	fd = open("/dev/mem", O_RDONLY);
	(void) read(fd, buffer, sizeof(buffer));
	(void) close(fd);

	memavail = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavail;
	memavailcnt = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavailcnt;

	for (memsize = i = 0; i < memavailcnt; ++i)
		memsize += (memavail++)->extent;
	sprintf(mem_env, "MEMSIZE=%d", memsize);
	putenv(mem_env);

*/
	/* temporarily assing stdin, stdout, and stderr */
	if ((ttyfd = open("/dev/sysmsg", O_RDWR)) < 0)
		ttyfd = open("/dev/null", O_RDWR);
	dup(ttyfd);
	dup(ttyfd);

	/* remount the root */
	(void) uadmin(A_REMOUNT, 0, 0);

	/* create the /dev/console etc. nodes based on kernel console type */
	(void) umask((mode_t)0);
	if (sysi86(SI86GCON, &consdev) != -1 ){
		mknod( "/dev/console", S_IFCHR|S_IRWXU|S_IWGRP|S_IWOTH,
			makedev( major(consdev), minor(consdev)));
		mknod( "/dev/systty", S_IFCHR|S_IRWXU|S_IWGRP|S_IWOTH,
			makedev( major(consdev), minor(consdev)));
		mknod( "/dev/syscon", S_IFCHR|S_IRWXU|S_IWGRP|S_IWOTH,
			makedev( major(consdev), minor(consdev)));
	}else{
		(void) printf("init: FAULT: Cannot make console nodes. \n");
		while(1);	/* go no further!! */
	}

	if ( mod_reg() )
		(void) printf("WARNING: Can't register fd DLM.\n");

	(void) umask((mode_t)022);
	switch(fork()) {
	case 0:
		execl("/sbin/autopush", "autopush", "-f", "/etc/ap/chan.ap", (char *)0);
		(void) printf("Can't run autopush\n");
		perror("");
		break;
	}
	wait(&i);

	switch(fork()) {
	case 0:
		execl("/sbin/wsinit", "wsinit", (char *)0);
		(void) printf("Can't run wsinit\n");
		perror("");
		break;
	}
	wait(&i);

/*
	if(do_vt0) { 
		vt0_ksh(TRUE);
		wait(&i);
	}
*/

	switch(fork()) {
	case 0:
		(void) setsid();
		(void) close(2);
		(void) close(1);
		(void) close(0);
		if (major(consdev) == 3){
			(void) open("/dev/console" , O_RDWR);
			dup(0);
			dup(0);
			putenv("SERIALCONS=Y");
			putenv("TERM=ANSI");
		}else{
			(void) open("/dev/vt01" , O_RDWR);
			dup(0);
			dup(0);
/*			putenv("TERM=AT386-ie"); */
			putenv("TERM=AT386");
			if ((fd = open("/dev/video", O_RDWR)) != -1) {
				ioctl(fd, VT_ACTIVATE, 1);
				(void) close(fd);
			}
		}


		execl("/sbin/sh", "ksh", "/isl/do_install", (char *)0);
		perror("");
		break;
	}


	/* note: we assume that major number for the serial port driver is 3 */
	if (major(consdev) != 3) { 
		vt0_ksh(FALSE);
	} else {
		(void) close(2);
		(void) close(1);
		(void) close(0);
		(void) open("/dev/vt00", O_RDWR);
		dup(0);
		dup(0);
	}
	for (;;)
		wait(&i);
}



static void handler(int sig)
{
	static int allow_reboot = 1;

	switch (sig) {
	case SIGINT:
	case SIGFPE:
	case SIGEMT:
		if (allow_reboot)
			uadmin(A_REBOOT, AD_BOOT, 0);
		break;
	case SIGUSR1:
		allow_reboot = 0;
		break;
	case SIGHUP:
	case SIGQUIT:
	case SIGALRM:
	case SIGTERM:
	case SIGUSR2:
		break;
	default:
		(void) printf("init: Internal Error: handler received unexpected signal.\n");
		break;
	}
}



static void vt0_ksh(int do_sleep)
{
	switch(fork()) {
	case 0:
		(void) setsid();
		(void) close(2);
		(void) close(1);
		(void) close(0);
		(void) open("/dev/vt00", O_RDWR);
		dup(0);
		dup(0);

		putenv("ENV=/funcrc");
		putenv("PS1=VT0>");
		putenv("HISTFILE=/tmp/.sh_history");
		putenv("PATH=:/usr/bin:/sbin:/usr/sbin:/mnt/usr/bin:/mnt/sbin:/mnt/usr/sbin");
		putenv("FPATH=/etc/inst/scripts");
		putenv("EDITOR=vi");
/*		putenv("TERM=AT386-ie"); */
		putenv("TERM=AT386");
		putenv(initstate_env);
		if (do_sleep)
			sleep(10);
		execl("/sbin/sh", "ksh", "-i", (char *)0);
		perror("");
		break;
	}

}

/*
 * Register the fd dlm so that we can access the floppy drive.
 */
static int mod_reg()
{
	struct mod_mreg mreg;

	mreg.md_typedata = (void *)FD_MAJOR;
	strcpy(mreg.md_modname, "fd");
	if( modadm(MOD_TY_BDEV, 1, (void *)&mreg) < 0 )
		return 1;
	if(modadm(MOD_TY_CDEV, 1, (void *)&mreg) < 0)
		return 1;
	return 0;
}


