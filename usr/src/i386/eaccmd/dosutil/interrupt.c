/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/interrupt.c	1.1.1.3"
#ident  "$Header$"
/*	@(#) interrupt.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*	Routines to enforce mutually exclusive access to a device.  These
 *	routines are to be used in the order
 *
 *		seize()		grab a device, but listen for signals
 *		disable()	delay any SIGHUP, SIGINT, SIGTERM, SIGQUIT
 *		enable()	act upon any pending signals
 *		release()	let others use the device
 *
 *	WARNING:  Within these routines, signal() is used to catch signals.
 *		  Prior signal handlers are not restored.  Hence, calling
 *		  programs should not establish signal handlers.
 *
 *	MODIFICATION HISTORY
 *		M000	sco!rr	20 Sep 88
 *		- changed name of variable "interrupt" to "nterrupt" since interrupt
 *		  is now a keyword (Microsoft 5.? compiler)
 */


#include	<errno.h>
#include	<signal.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"dosutil.h"



static struct devsem{			/* devices with opened semaphores */
	dev_t	d_rdev;			/* 	device id */
	int	d_sem;			/* 	semaphore number */
};

static int	current   = -1;		/* file descr of current DOS disk */
static int	nterrupt = FALSE;	/* TRUE if there was a signal */
static int	(*savehup)(),
		(*saveint)(),
		(*savequit)(),
		(*saveterm)();



/*
 *	catch()  --  routine to catch signals; see enable() and disable().
 */

static int catch(signo)
int signo;
{
#ifdef DEBUG
	fprintf(stderr,"\nDEBUG signal caught and delayed !\n");
#endif
	signal(signo, catch);
	nterrupt = TRUE;
}

/*
 *	disable()  --  delay signals until enable() is called.
 */

disable()
{
	int catch();

	nterrupt = FALSE;

#ifdef DEBUG
	fprintf(stderr,"DEBUG disabling signals\n");
#endif
	if ((savehup = signal(SIGHUP,SIG_IGN)) != SIG_IGN)
		signal(SIGHUP,catch);
	if ((saveint = signal(SIGINT,SIG_IGN)) != SIG_IGN)
		signal(SIGINT,catch);
	if ((savequit = signal(SIGQUIT,SIG_IGN)) != SIG_IGN)
		signal(SIGQUIT,catch);
	if ((saveterm = signal(SIGTERM,SIG_IGN)) != SIG_IGN)
		signal(SIGTERM,catch);
}



/*	enable()  --  allow signals to take effect.  If a signal is pending,
 *		this process dies, but after releasing any seized device.
 */

enable()
{
#ifdef DEBUG
	fprintf(stderr,"DEBUG enabling signals\n");
#endif
	if (nterrupt){
		if (current >= 0)
			release(current);
		exit(1);
	}
	signal(SIGHUP, savehup);
	signal(SIGINT, saveint);
	signal(SIGQUIT,savequit);
	signal(SIGTERM,saveterm);
}



/*
 *	letgo()  --  routine to release a device in a hurry; see seize().
 */

static int letgo(signo)
int signo;
{
#ifdef DEBUG
	fprintf(stderr,"\nDEBUG first signal received, others ignored !\n");
#endif
	signal(signo,SIG_IGN);
	release(current);
	exit(1);
}


/*
 *	release(device)  --  allow another process to access the device.
 */

release(fildes)
int fildes;
{
	int sem_num;

#ifdef DEBUG
	fprintf(stderr,"DEBUG release()\n");
#endif
	if (fildes != current)
		fatal("LOGIC ERROR incorrect seize/release sequence",1);

	if ((sem_num = semap(fildes)) < 0)
		return(FALSE);

	sigsem(sem_num);
	current = -1;

	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
	signal(SIGTERM,SIG_DFL);
}


/*
 *	seize(device)  --  enforce mutually exclusive access to the device.
 *		Ignore signals until the device is released.
 *
 *	NOTE: This routine assumes that nbwaitsem() returns 0 if the
 *		semaphore (device) was busy.  This is an undocumented
 *		characteristic of nbwaitsem() !!
 */

seize(fildes)
int fildes;
{
	extern int errno;
	extern char *f_name;
	int i, letgo(), sem_num;

	if ((sem_num = semap(fildes)) < 0)
		return(FALSE);

#ifdef DEBUG
	fprintf(stderr,"DEBUG nbwaitsem(%d)\n",sem_num);
#endif
	i = 0;
	while (TRUE){
		errno = 0;
		if (nbwaitsem(sem_num) == 0)
			break;;
		if ((errno != ENAVAIL) || (i++ >= NTRYWAIT))
			return(FALSE);

		fprintf(stderr,"%s: device busy; %d of %d waits\n",
							f_name,i,NTRYWAIT);
		sleep(WAITTIME);
	}
	current = fildes;

	if (signal(SIGHUP,SIG_IGN) != SIG_IGN)
		signal(SIGHUP,letgo);
	if (signal(SIGINT,SIG_IGN) != SIG_IGN)
		signal(SIGINT,letgo);
	if (signal(SIGQUIT,SIG_IGN) != SIG_IGN)
		signal(SIGQUIT,letgo);
	if (signal(SIGTERM,SIG_IGN) != SIG_IGN)
		signal(SIGTERM,letgo);

	return(TRUE);
}


/*	semap(fildes)  --  returns the semaphore number of the semaphore
 *		opened for the device  whose file descriptor is given.
 *		To avoid repetitive opensem()s, devtable[] keeps track of
 *		all semaphores already opened.
 */

static int semap(fildes)
int fildes;
{
	static int tabsize = 0;			/* current size of devtable[] */
	static struct devsem devtable[NSEMAP];

	int i;
	char semname[BUFMAX];
	struct stat statbuf;

	if (fstat(fildes,&statbuf)){
		sprintf(errbuf,"LOGIC ERROR can't fstat(%d)",fildes);
		fatal(errbuf,1);
	}
	for (i = 0; i < tabsize; i++){
		if (devtable[i].d_rdev == statbuf.st_rdev) {
			sprintf(semname,"%s/%05ddos",SPOOL,devtable[i].d_rdev);
			unlink(semname);
			return(devtable[i].d_sem);
		}
	}
	if (i > NSEMAP){
		fatal("too many devices for devtable[]",0);
		return(-1);
	}
	devtable[i].d_rdev = statbuf.st_rdev;
	sprintf(semname,"%s/%05ddos",SPOOL,devtable[i].d_rdev);

	if (((devtable[i].d_sem = creatsem(semname,0444)) < 0) &&
	    ((devtable[i].d_sem = opensem(semname))       < 0)){
		perror(semname);
		return(-1);
	}
	tabsize++;
	return(devtable[i].d_sem);
}
