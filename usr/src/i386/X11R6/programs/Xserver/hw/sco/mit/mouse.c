/*
 *	@(#)mouse.c	1.2	6/30/89 16:11:37
 *
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Mouse api library routines
 *
 * MODS
 * - If someone requests D_STRING in ev_open(), they get
 *   fd 0 as the keyboard regardless of the data files.
 *
 * S000	sco!daniel Tue Jun  7 16:12:41 PDT 1988
 *	- rename ev_flushq() to ev_flush() for consistency.
 * S001 sco!daniel Tue Jun  7 11:06:35 PDT 1988
 *	- ev_open now takes a pointer to a mask rather than a mask.
 *	- The mask is set to indicate what kinds of devices are found.
 *	- The type of a device mask is not dmask_t.
 *	- ev_pop returns number of event lost due to overrun.
 * S002 sco!daniel Fri Jul 29 03:20:17 PDT 1988
 *	Major modifications
 *	- Implement d_suspend and d_resume.
 *	- Change a member of the userinfo struct from array to pointer.
 *	- Remove the unnecessary function notyet().
 *	- Change a return value from ev_setemask().
 *	- Fill in user info for open devices.
 * S003	sco!daniel	Mon Sep 26 10:36:39 PDT 1988
 *	- Implement ev_count to return the number of events in the queue.
 * S004	sco!daniel	Mon Sep 26 11:37:08 PDT 1988
 *	- Add a BUTTONS field to the devices database and record the
 *	  (numeric) argument in the userinfo structure.
 * S005	mikep@sco.com	Thu Apr 04 12:51:20 PST 1991
 *	- Close the tty and devices files as soon as they are read in.
 *	- Check stdin instead of stdout for event sanity
 * S006	Fri Sep 04 14:30:35 PDT 1992	hiramc@sco.COM
 *	Do standard ANSI declarations of function arguments to eliminate
 *	compiler warnings.  And remove clashing declarations of fopen.
 * S007	Thu Sep 24 11:16:49 PDT 1992	mikep@sco.com
 *	- Don't pass shorts to scanf().  Problem only shows up with -Oe.
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/termio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/page.h>
#include <sys/immu.h>
#include <sys/event.h>
#include "mouse.h"

#if defined(M_I386)
# define	FAR
#else
# define	FAR	far
#endif

/* Our configuration files w/ default values */
static char *evtermfile = EVTERM;
static char *evdevfile = EVDEV;

int	ev_errlev=0;	/* if nonzero, errors are printed */

extern int errno;

/* A flag encoding the current status of this package of routines */
static char status = 0;

/* Bit definitions in our status byte */
#define	S_INIT		0x01		/* If we are initialized */
#define	S_QOPEN		0x02		/* If the event queue is open */
#define	S_ERROR		0x04		/* Configuration error */
					/* e.g. Can't run w/o a term */
#define	S_SUSPENDED	0x08		/* We've suspended ourselves */

/* A place to save the tty settings of the keyboard */
static struct termio tsave;

#define	WHITESPACE	" \t:" /* Allow : to delimit like gettydefs */
#define	DELIMITERS	" \t,"
#define	ISQUOTE(c)	( (c)=='\"' || (c)=='`' || (c)=='\'' )

struct gin_status {
	char	devstatus;	/* OPEN, SUSPENDED */
	short	class;		/* ABS, REL, STRING, OTHER */
	char	type[SHORT];	/* What hardware */
	short	fd;		/* file descriptor when open */
	char	pathname[LONG];	/* device file */
	char	init[SHORT];
	short	ratio;
	short	stty;		/* bit mask of baud/cs/parenb present*/
	long	cflag;		/* struct termio bits */
	struct devinfo d_userinfo;	/* for reporting to application */
};

/* Bit definitions for devstatus field of a device info structure */
#define	D_ISVALID	0x01	/* If slot has a device in it */
#define	D_OPEN		0x02
#define	D_SUSPENDED	0x04

/* Bit definitions in the stty field */
#define	STTY_HAVEBAUD	0x01
#define	STTY_HAVECS	0x02
#define	STTY_HAVEPARENB	0x04
#define	STTY_HAVEPARODD	0x08
#define	STTY_HAVESTOPB	0x10
#define	STTY_HAVEIXON	0x20
#define	STTY_HAVEIXOFF	0x40

/* A table containing all the information about all our GIN devices */
static struct gin_status table[MAXDEVS];

/* Helpful macros */
#define	Class(i)	(table[(i)].class)
#define	Type(i)		(table[(i)].type)
#define	Init(i)		(table[(i)].init)
#define	Ratio(i)	(table[(i)].ratio)
#define	Devstatus(i)	(table[(i)].devstatus)

/* Our event queue */
QUEUE FAR	*qp;
/* and its file descriptor */
static short	qfd;

/**********************************************************************/
/*                                                                    */
/*          FUNCTION PROTOTYPES FOR STATIC FUNCTIONS                  */
/*                                                                    */
/**********************************************************************/

static void error(char*, char*);
static void initialize();
static int load_keys(char*, char*);
static int load_devices(char*);
static void load_kbd();
static short findkey(char*);
static int loadinfo(short,char*);
static int parseparms(short, char*);
static int setparm(short, char*, char*);
static char * getvalue(char*);
static char * strtoken(char* , char*);
static short xlat_class(char*);
static int setstty(int, char*);
static int setbaud(int, int);
static int setparenb(int, int);
static int setparodd(int, int);
static int setixon(int, int);
static int setixoff(int, int);
static int setcs(int, int);
static int setstopb(int, int);
static int d_open(int, int);
static int d_close(int);
static int d_suspend(int);
static int d_activate(int);
static short openqueue();
static check4events(emask_t);
static int fgetline(FILE*, char*, short);
static void make_upper(char*);
static void xlat_octals(char*);
static int isodigit(char);

#ifdef DEBUG
void
ev_print()
{
	int	i;

	printf("chan	stat	class	type	fd	path\n");
	printf("----	----	-----	----	--	----\n");
	for (i=0; i<MAXDEVS; i++)
		printf("%d	%o	%o	%s	%d	%s\n",
			i,Devstatus(i),Class(i),Type(i),
			table[i].fd, table[i].pathname);
	printf("\n\n");
}
#endif

/*
 * 			ev_init()	
 *
 * parameters:	none
 * globals:	table		Table of devices information
 *		evterm		device associations configuration file
 *		evdev		device information configuration file
 *		status		Status of the ``module''
 * calls:	none
 * returns:	0		All data structures consistent
 *		-1		Error while reading configuration files
 * files:	/usr/lib/evterms	Device associations
 *		/usr/lib/evdevs		Device types and information
 *
 * purpose:	Read in data from the configuration files and load the
 *		structure of device information. Initialize global data.
 *
 *		First we determine our terminal. Open and read the
 *		device associations file to get the keys of the devices
 *		associated with our terminal. Then read the data for
 *		those devices out of the devicess-information file
 *		into our local data structure.
 *
 *		Create an entry in the table for the keyboard.
 *		The keyboard is always available by
 *		order of the product manager.
 *
 *		Flag all the devices as unopened in the devices table.
 *		Flag the queue as unopened in the status byte. 
 *
 *		ttyname(S) returns /dev/tty?? which we need to convert
 *		to tty?? to agree with the data files.
 */
int
ev_init()
{
	char	*ttyname(), *ttyp;

#ifdef DEBUG
	printf("ev_init: enter\n");
#endif
#ifndef DEBUG
	if ( ! isatty(0) ) {					/* S005 */
		error("ev_init","stdout must be a tty");
		status |= S_ERROR;
		return -1;
	}
#endif
	
	initialize();

	/* Load the keys of GIN devices associated with our terminal */
	ttyp = strrchr(ttyname(0), '/') + 1;			/* S005 */
	if (load_keys(evtermfile, ttyp) == -1)	{
		error("ev_init","cannot load GIN device keys");
		status |= S_ERROR;
		return(-1);
	}

	/* Load device information into memory */
	if (load_devices(evdevfile) == -1)	{
		error("ev_init","cannot load GIN device information");
		status |= S_ERROR;
		return(-1);
	}

	/* Load an artificial keyboard entry */
#ifdef DEBUG
	printf("ev_init: artificially load keyboard\n");
#endif
	load_kbd();
	status |= S_INIT;
#ifdef DEBUG
	printf("ev_init: normal exit\n");
#endif
	return(0);
}

/*
 * 			ev_open()
 *
 * parameters:	dmask		Mask of device types
 * globals:	table		Device information table
 *		status		Module status byte
 *		qp		Our event queue pointer
 *		qfd		Our event queue file descriptor
 * calls:	?????
 * returns:	-1		Configuration error
 *				e.g. have no terminal/associations
 *		-2		No devices found to attach
 *		-3		Error on device open/attach
 *		-4		Could not open event queue
 *		>=0		File descriptor of event queue
 * files:	/dev/event	The event queue device
 *		/dev/?????	GIN devices
 *
 * purpose:	This routine opens the event queue and attaches
 *		all devices whose type is masked into the parameter. 
 *
 *		This routine may be called after the event
 *		queue and GIN devices have already been opened.
 *		In that case, any attached devices whose
 *		type is not masked into the argument are closed,
 *		and any whose type is masked in are opened,
 *		if they are not already attached.
 *
 *		This function returns an error if:
 *		-	configuration error is flagged in status byte
 *		-	cannot open event queue
 *		-	have no devices to attach (e.g. mask of 0)
 *		-	Attempting to open any device fails
 *
 *		When this function completes successfully, it returns
 *		an event queue file descriptor. This file descriptor
 *		should ONLY be used for poll(S)ing the event queue.
 *
 * S001 -- changed the parameter to a pointer.
 */
int
ev_open(dmaskp)
dmask_t *dmaskp;						/*S001*/
{
	int	i,retcode, got_one;
	dmask_t	found_mask=0;

#ifdef DEBUG
	printf("ev_open: enter\n");
#endif
	if (status & S_ERROR)
		return -1;
	if ((qfd = openqueue()) == -1)	{
		error("ev_open","cannot open the event queue");
		return -4;
	}
	fcntl(qfd, F_SETFD, 1);		/* Set close-on-exec bit */
	got_one = 0;
	for (i=0; i<MAXDEVS; i++)	{
		/* attach if device valid and masked in */
		if (Devstatus(i) & D_ISVALID && Class(i) & *dmaskp) {
#ifdef DEBUG
			printf("ev_open: device %d can be added\n",i);
#endif
			retcode = d_open(qfd, i);
#ifdef DEBUG
			printf("ev_open: dopen returned %d\n",retcode);
#endif
			if (retcode == -1)	{
				error("ev_open","error attaching device");
				ev_close();
				return -3;
			}
			got_one++;				/*S001*/
			found_mask |= Class(i);
		}
		else 		/* Device should not be attached */
			if (Devstatus(i) & D_OPEN && Devstatus(i) & D_ISVALID)
				d_close(i);
	}
#ifdef DEBUG
	printf("ev_open: exit %s\n", 
			got_one ? "successfully" : "nodevices" );
#endif
	*dmaskp = found_mask;					/*S001*/
	if (got_one)
		return qfd;
	else
		return -2;
}

		/* S003 vvv */
/*
 * 			ev_count()
 *
 * parameters:	none
 * globals:	status		Module status byte
 *		qp		Event queue pointer
 * calls:	?????
 * returns:	-1		Not currently open
 *		>=0		Number of events in the queue
 *
 * files:	/dev/event	The event queue device
 *
 * purpose:	This routine informs the app how many events are in the queue.
 *
 */
int
ev_count()
{
	if (status & S_ERROR || ! (status & S_QOPEN))
		return -1;
	
	if (qp->head >= qp->tail)
		return qp->head - qp->tail;
	return (int) QSIZE - qp->tail + qp->head;
}

		/* S003 ^^^ */

/*
 * 			ev_close()
 *
 * parameters:	none
 * globals:	table		Device information table
 *		status		Module status byte
 * calls:	?????
 * returns:	0		success
 *		-1		Not currently open
 *		-2		Error on closing a device 
 *
 * files:	/dev/event	The event queue device
 *		/dev/?????	GIN devices
 *
 * purpose:	This routine closes the event queue and 
 *		all affiliated devices. 
 *
 */
int
ev_close()
{
	int	i;

	if (status & S_ERROR || ! (status & S_QOPEN))
		return -1;
	
	for (i=0; i<MAXDEVS; i++)
		if ( Devstatus(i) & D_ISVALID )
			if ( Devstatus(i) & D_OPEN )
				d_close(i);
	close(qfd);
	return(0);
}

/*
 * 			ev_getdev()
 *
 * parameters:	dmask		A device-types mask
 *		devp		A pointer to a device information
 *				structure or else NULL.
 * globals:	status		Current module status
 *		table		Device information
 * calls:	??????
 * returns:	&devinfo	A pointer to a device information
 *				structure
 *		-1		Queue not open
 *		-2		No device of that type found
 * files:	none
 * purpose:	An application (or user) uses this routine to
 *		examine the GIN devices that are open and attached
 *		to the queue. This could be used to select one
 *		of several mice, for example, or else just to
 *		look at the names of the devices.
 *
 *		The arguments are a device mask and a pointer to 
 *		a device structure. The device mask indicates what
 *		kinds of devices the application is interested in.
 *		The pointer is of the same type as the function
 *		returns, and is used for iteration. On the first
 *		call, the pointer should be passed in as 0. Thereafter
 *		the pointer returned by the function should be passed
 *		back in.
 *
 *		The per-device data structures returned by this routine
 *		can be examined across calls because the routine
 *		does not return a pointer to a static data structure.
 *		e.g. an app can get two mouse structures and ask the
 *		user which to use.
 */
struct devinfo *
ev_getdev( dmask_t dmask, struct devinfo * devp)		/* S006 */
{
	int	d=0;

	if ( devp != (struct devinfo *) NULL )
		d = devp->handle + 1;

	while ( d<MAXDEVS )	{
		if (Class(d) & dmask && Devstatus(d) & D_OPEN)
			return(&table[d].d_userinfo);
		d++;
	}
	return (struct devinfo *) NULL;
}

/*
 * 			ev_gindev()
 *
 * parameters:	devp		A pointer to a device structure
 *		action		EXCLUDE || REINCLUDE
 * globals:	table		Table of devices information
 *		status		Status of the ``module''
 * calls:	??????
 * returns:	0		success
 *		-1		Queue not open
 *		-2		Nonexistent device (invalid pointer)
 *		-3		Device not attached (EXCLUDE)
 *		-3		Device not excluded (REINCLUDE)
 *		-4		Invalid argument
 * files:	none
 *
 * purpose:	This function is used to exclude or later re-include
 *		a gin device feeding the event queue. A pointer to 
 *		a device structure is passed in identifying the
 *		the device to operate upon. That pointer was obtained
 *		with ev_getdev().
 *
 *		After successful execution of this function, the
 *		indicated GIN device is either suspended, or
 *		or reactivated, depending on the value of the
 *		second parameter.
 */
int
ev_gindev(struct devinfo * devp, char clude)		/* S006 */
{
	short	d;

	if ( devp == (struct devinfo *) NULL )
		return -2;

	d = devp->handle;

	if ( Class(d) == 0 || (Devstatus(d) & D_OPEN) == 0 )
		return -2;

	switch (clude) {
		case EXCLUDE:
				if (Devstatus(d) & D_SUSPENDED)
					return -3;
				return d_suspend(d);
		case INCLUDE:
				if ( (Devstatus(d) & D_SUSPENDED) == 0)
					return -3;
				return d_activate(d);
		default:	return -4;
	}
}

/*
 *			ev_flush()
 *
 * parameters:	none
 * globals:	status		Status of this ``module''
 *		qp		Pointer to the event queue
 * calls:	none
 * returns:	0		success
 *		-1		Queue not open
 * files:	none
 *
 * purpose:	This routine flushes an open event queue.
 *		This is done by setting the tail pointer equal
 *		to the head pointer.
 */
int
ev_flush()
{
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return -1;
	qp->tail = qp->head;
	return 0;
}

/*
 *			ev_setemask(emask)
 *
 * parameters:	emask		An event mask
 * globals:	qp		Pointer to the event queue
 *		status		Status of the ``module''
 * calls:	??????
 * returns:	0		success
 *		-1		Queue not open
 *		-2		No events would be generated
 * files:	none
 *
 * purpose:	Set the current event mask on the event queue. 
 *		Events not masked in are discarded before they enter
 *		the queue. This has no effect on events already in the 
 *		queue.
 *
 *		This function does nothing and returns an error if 
 *		the indicated mask would cause no events to be enqueued,
 *		based on the types of devices feeding the queue.
 */
int
ev_setemask(emask_t emask)				/* S006 */
{
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return -1;
	if (! check4events(emask))
		return -2;
	ioctl(qfd, EQIO_SETEMASK, emask);;
	return 0;
}

/*
 *			ev_getemask(emaskp)
 *
 * parameters:	emaskp		A pointer to an event mask word
 *				The mask is written here.
 * globals:	qp		Pointer to the event queue
 *		status		Status of the ``module''
 * calls:	??????
 * returns:	0		Success, writes the current event mask
 *				into where the parameter points.
 *		-1		Queue not open
 * files:	none
 *
 * purpose:	Get the event queue's current event mask.
 *
 *		The event mask is written into where the parameter
 *		points provided the queue is open.
 *
 */
ev_getemask(emaskp)
emask_t	*emaskp;
{
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return -1;
	ioctl(qfd, EQIO_GETEMASK, emaskp);
	return 0;
}

/*
 *			ev_read()
 *
 * parameters:	none
 * globals:	qp		The event queue pointer
 *		status		Event queue (module) status 
 * calls:	??????
 * returns:	evp		An event pointer (success)
 *		0		Can't read an event: no events in queue
 *				OR configuration error or queue not open
 * files:	none
 *
 * purpose:	Return a pointer to the next event in the queue.
 */
EVENT FAR *
ev_read()
{
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return 0;
	if (qp->head == qp->tail)
		return 0;
	return (EVENT FAR *) &qp->queue[qp->tail];
}

/*
 *                      ev_block()
 *
 * parameters:	none
 * globals:	qfd
 *		status
 * calls:	none
 * returns:	0		success
 *		-1		error (queue not open, for example)
 * files:	none
 * purpose:	sleep until there's an event in the queue
 * algorithm:	The event driver has an ioctl which puts
 *		us to sleep until there's an event.
 *		Issue that ioctl.
 */
int
ev_block()
{
	int	tmp;
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return -1;
	if (qp->head != qp->tail)
		return 0;
	tmp = ioctl(qfd, EQIO_BLOCK, 0);
	return tmp;
}


/*
 *			ev_pop()
 *
 * parameters:	none
 * globals:	qp		The event queue pointer
 *		status		Status of this ``module''
 * calls:	??????
 * returns:	>=0		success, number of events recently lost
 *		-1		Queue not open
 *		-2		Queue empty
 * files:	none
 *
 * purpose:	Pops the next event off the queue.
 *		Returns a failure condition if the queue is empty.
 *
 *		Queue overrun occurs when the event driver cannot 
 *		write new events because the queue is full. This 
 *		routine indicates queue overrun by returning the
 *		number of events that have been lost since the last
 *		call to popq(). This information is available in 
 *		qp->overrun.
 */
int
ev_pop()
{
	if ( ! (status & S_QOPEN) || status & S_SUSPENDED )
		return -1;
	if (qp->head == qp->tail)
		return -2;
	qp->tail = (qp->tail + 1) % QSIZE;
	return qp->overrun;					/*S001*/
}

/*
 *			ev_suspend()
 *
 * parameters:	none
 * globals:	table		Table of devices information
 *		status		Status of this ``module''
 * calls:	??????
 * returns:	0		success
 *		-1		Queue not open
 *		-2		Already suspended
 * files:	/dev/?????	GIN devices
 *
 * purpose:	Suspend the event queue and GIN devices. This allows
 *		the process to fork() and exec() and sleep while
 *		a new graphics applications uses the GIn devices.
 *		They would otherwise be busy.
 *
 *		Implemented as part of S002.
 */
int
ev_suspend()
{
	int	ret;

	if ( ! (status & S_QOPEN) )
		return -1;
	if ( status & S_SUSPENDED )
		return -2;
	ret = ioctl(qfd, EQIO_SUSPEND, 0);
	if ( ! ret )
		status |= S_SUSPENDED;
	return ret;
}

/*
 *			ev_resume()
 *
 * parameters:	a		a is for apple
 * globals:	table		Table of devices information
 *		status		Status of this ``module''
 * calls:	??????
 * returns:	0		success
 *		-1		Queue not open
 *		-2		Queue not suspended
 * files:	none
 *
 * purpose:	Resumes the suspended event queue and devices.
 *
 *		Implemented as part of S002.
 */
int
ev_resume()
{
	int	ret;

	if ( ! (status & S_QOPEN) )
		return -1;
	if ( ! (status & S_SUSPENDED) )
		return -2;
	ret = ioctl(qfd, EQIO_RESUME, 0);
	if ( ! ret )
		status &= ~S_SUSPENDED;
	return ret;
}


/*********************************************************************/
/*                                                                   */
/*                STATIC FUNCTION DEFINITIONS                        */
/*                                                                   */
/*********************************************************************/

static void
load_kbd()
{
	short i;

	for (i=0; i<MAXDEVS; i++)
		if ( (Devstatus(i) & D_ISVALID) == 0 )
			break;

	if ( i == MAXDEVS )
		return;

	Devstatus(i) |= D_ISVALID;
	Class(i) = D_STRING;
	strcpy(Type(i),"keyboard");
	table[i].fd = 0;
	strcpy(table[i].pathname, "stdin");
	table[i].init[0] = '\0';
	table[i].stty = 0;
	table[i].d_userinfo.class = D_STRING;
	table[i].d_userinfo.type = Type(i);
	table[i].d_userinfo.handle = i;
	strcpy(table[i].d_userinfo.key,"keyboard");
	strcpy(table[i].d_userinfo.name,"the keyboard");
#ifdef DEBUG
	printf("load_kbd: keyboard is device %d, devstatus == %d\n",
				i, Devstatus(i));
#endif
}

static short
openqueue()
{
	int	fd;

	fd = open(EVENT_QUEUE, O_RDONLY);
	if ( fd == -1 )
		return -1;
	if ( ioctl(fd, EQIO_GETQP, &qp) == -1 )	{
		error("openqueue","can't get (ioctl) a queue pointer");
		close(fd);
		return -1;
	}
#ifdef DEBUG
	printf("openqueue: opened queue, got queue pointer, fd==%d\n",
			fd);
#endif
	status |= S_QOPEN;
	return (short) fd;
}

static void
error(func,s)
char	*func, *s;
{
	if ( ev_errlev )
		fprintf(stderr, "%s: %s\n", func, s);
}


static void
initialize()
{
	short i;

#ifdef DEBUG
	printf("initialize...\n");
#endif
	for (i=0; i<MAXDEVS; i++)	{
		Devstatus(i) &= ~D_ISVALID;
		Class(i) = 0;
		table[i].init[0] = 0;
		table[i].stty = 0;	/* have neither baud nor char size */
	}
}

#define	TOOLONG	"device key ``%s'' exceeds %d characters"
static int
load_keys(fname, ttname) 
char	*fname;
char	*ttname;
{
	FILE	*fp;		/* *fopen(char*, char*) in stdio.h S005 */
	char	buf[256], *p;
	int	nd = 0; 	/* # devices found */

#ifdef DEBUG
	printf("load_keys: enter, tty is %s\n", ttname);
#endif
	if (fname == 0 || ttname == 0)	{
		error("load_keys","invalid (i.e. NULL) argument");
		return -1;
	}
	if ( (fp = fopen(fname, "r") ) == (FILE*) NULL )	{
		error("load_keys","cannot open evterms file");
		return -1;
	}
	while ( fgetline(fp, buf, sizeof(buf)) != -1 ) {
		if ( *buf == 0 || *buf == '#' )
			continue;
#ifdef DEBUG
		printf("load_keys: line is ``%s''\n",buf);
#endif
		if ( (p = strtoken(buf, WHITESPACE)) == (char*) NULL )
			break;
		if ( strcmp(p, ttname) )
			continue;
		/* Found our tty, load associated keys */
#ifdef DEBUG
		printf("load_keys: found %s, key string is ``%s''\n",
				ttname, buf);
#endif
		while ( (p = strtoken(buf, WHITESPACE) ) != (char*) NULL ) {
#ifdef DEBUG
			printf("load_keys: key %s\n",p);
#endif
			if (strlen(p) >= SHORT)	{
				char	toolong[256];
				sprintf(toolong, TOOLONG, p, SHORT);
				error("load_keys",toolong);
			}
			else	{
				table[nd].devstatus |= D_ISVALID;
				strcpy(table[nd].d_userinfo.key, p);
#ifdef DEBUG
				printf("###key %d is `%s'\n",nd,p);
#endif
				nd++;
			}
		}
		break;
	}
	fclose(fp);						/* S005 */
#ifdef DEBUG
	printf("load_keys: normal exit\n");
#endif
/*	return nd ? 0 : -1;	*/
	return 0;
}

/*
 * Algorithm:	1) count the number of keys we have to find.
 *		2) iterate through the event-devices file
 *		   matching keys and reading in the device into.
 *		3) each match decrements the counter
 *		4) if we terminate with nonzero counter, that's
 *		   an unmatched key.
 */
#define	PARSERR	"error parsing information for device key ``%s''"
static
load_devices(fname)
char	*fname;
{
	FILE	*fp;						/* S005 */
	char	*key,buf[512];
	short	i, nkeys = 0;

#ifdef DEBUG
	printf("load_devices: enter\n");
#endif
	if (fname == 0 )	{
		error("load_devices","invalid (i.e. NULL) argument");
		return -1;
	}
	if ( (fp = fopen(fname, "r") ) == (FILE*) NULL )	{
		error("load_devices","cannot open ev_devices file");
		return -1;
	}

	for (i=0; i<MAXDEVS; i++)
		if ( Devstatus(i) & D_ISVALID ) 
			nkeys++;
	
#ifdef DEBUG
	printf("load_devices: %d keys to find and parse\n", nkeys);
#endif
	while ( fgetline(fp, buf, sizeof(buf)) != -1 )	{
		if ( *buf == 0 || *buf == '#' )
			continue;
#ifdef DEBUG
		printf("load_devices: line is ``%s''\n", buf);
#endif
		key = strtoken(buf, WHITESPACE);
		i = findkey(key);
#ifdef DEBUG
		printf("load_devices: key is ``%s'', findkey is %d\n",
					key, i);
#endif
		if ( i != -1 )	{
			strcpy(table[i].d_userinfo.key, key);
			if (loadinfo(i, buf) == -1)	{
				Devstatus(i) &= ~D_ISVALID;
				sprintf(buf, PARSERR, key);
				error("load_devices",buf);
				return -1;
			}
			if ( --nkeys == 0 )
				break;
		}
	}
	if ( nkeys )	{
		error("load_devs","unmatched key");
		return -1;
	}
	fclose(fp);						/* S006 */
#ifdef DEBUG
	printf("load_devices: normal exit\n");
#endif
	return 0;
}

static short
findkey(key)
char	*key;
{
	short	i;

	for (i=0; i<MAXDEVS; i++)	{
#ifdef DEBUG
		printf("find: target(`%s'), found(`%s'), ndx(%d), valid(%d)\n",
			key,
			table[i].d_userinfo.key,
			i,
			Devstatus(i)&D_ISVALID );
#endif
		if  ( Devstatus(i) & D_ISVALID )
			if ( ! strcmp(table[i].d_userinfo.key, key) )
				return i;
	}
	
	return (short) -1;
}

/*
 * This routine loads and interprets data from the event-devices
 * configuration file. The second argument is the line in the file
 * for the indicated device entry.
 *
 * info syntax:
 *
 *	device class type parm=value ...
 *
 *	device = pathname of device file
 *	class = ABS, REL, STRING or OTHER. A b may be appended if
 *		the device is capable of generating button events
 *	type = keyboard, mcs_sermouse, mcs_busmouse, mss_sermouse
 *		lt_sermouse0, lt_sermouse1, lt_sermouse2, lt_sermouse3,
 *		lt_sermouse4, lt_sermouse5, lt_sermouse6, lt_busmouse,	
 *		ol_busmouse, ps2_mouse	
 */
 #define	UNKNOWN_CLASS	"unrecognized device class ``%s''"
 #define	UNKNOWN_TYPE	"unrecognized device type ``%s''"
static
loadinfo(short ndx, char * info)				/* S006 */
{
	char	*p;
	char	buf[256];

#ifdef DEBUG
	printf("load_info: enter with line ``%s''\n",info);
#endif
	/* Parse device file name */
	if ( (p = strtoken(info, WHITESPACE)) == NULL)	{
		error("loadinfo", "missing device pathname");
		return -1;
	}
#ifdef DEBUG
	printf("loadinfo: pathname is ``%s''\n",p);
#endif
	strcpy(table[ndx].pathname, p);

	/* Parse device class */
	if ( (p = strtoken(info, WHITESPACE)) == NULL)	{
		error("loadinfo", "missing device class");
		return -1;
	}
	if ( (Class(ndx) = xlat_class(p)) == -1 )	{
#ifdef DEBUG
		printf("loadinfo: class is untranslatable\n");
#endif
		sprintf(buf, UNKNOWN_CLASS, p);
		error("loadinfo", buf);
		return -1;
	}
	/* Parse device type */
	if ( (p = strtoken(info, WHITESPACE)) == NULL)	{
		error("loadinfo", "missing device type");
		return -1;
	}
#ifdef DEBUG
	printf("loadinfo: type is ``%s''\n",p);
#endif
	strcpy(Type(ndx),p);
	return parseparms(ndx,info);
}

/*
 * parseparms()
 *
 * This routine parses a string of name=value parameters.
 * The results are entered into the table entry for the
 * device whose index is passed in as a paramter. The device
 * class and device type in the table entry are valid.
 *
 * Valid names include:	
 *				NAME=%s
 *				STTY=%s
 *				INIT=%s
 *				SENSITIVITY=%x
 */
static
parseparms(short ndx, char * s)					/* S006 */
{
	char	*parm, *val, buf[256];

#ifdef DEBUG
	printf("parseparms: enter with ``%s''\n",s);
#endif
	strcat(s, " ");
	while (*s)	{
		while ( isspace(*s) )
			s++;
		if ( ! *s )
			break;
		if ( (parm = strtoken(s, "=")) == NULL)	{
			error("parseparms","bad syntax");
			return -1;
		}
#ifdef DEBUG
		printf("parseparms: got ``%s''\n", parm);
#endif
		while ( isspace(*s) )
			s++;
		if ( (val = getvalue(s)) == (char*) NULL)	{
			sprintf(buf, "can't parse value of %s", parm);
			error("parseparms",buf);
			return -1;
		}
#ifdef DEBUG
		printf("parseparms: value of ``%s'' is ``%s''\n", 
							parm, val);
#endif
		if (setparm(ndx, parm, val) == -1) {
			sprintf(buf,"can't set argument to ``%s=''",parm);
			error("parseparms",buf);
			return -1;
		}
	}
#ifdef DEBUG
	printf("parseparms: normal exit\n");
#endif
	return 0;
}

/*
 * setparm
 *
 * Given a (parm,value) pair, determine what they mean.
 * We are given the index of the device to which the info corresponds.
 *
 */
static
setparm(short ndx, char * parm, char * value)		/*	S005	*/
{
	char	buf[80];

	make_upper(parm);
	if ( ! strcmp(parm, "NAME") )	{
		strcpy(table[ndx].d_userinfo.name, value);
		return 0;
	}
		/* S004 vvv */
	if ( ! strcmp(parm, "BUTTONS") )	{
		int buttons;					/* S007 */
		if (sscanf(value, "%d", &buttons) != 1) {	/* S007 */
			error("setparm", "cannot parse value of BUTTONS");
			return -1;
		}
		if (table[ndx].d_userinfo.buttons = buttons)	/* S007 */
			Class(ndx) |= D_BUTTON;
#ifdef DEBUG
		printf("For device index %d, buttons == %d('%s')\n",ndx, 
				table[ndx].d_userinfo.buttons, value);
#endif
		return 0;
	}
		/* S004 ^^^ */
	if ( ! strcmp(parm, "INIT") )	{
		xlat_octals(value);
		strcpy(table[ndx].init, value);
		return 0;
	}
	if ( ! strcmp(parm, "SENSITIVITY") )	{
		int	ratio;					/* S007 */
		/* Prevent dangerous overflow */
		if ( strlen(value) > 4 ) {
			value[4] = '\0';				/*S002*/
			fprintf(stderr, "warning: sensitivity %s too large\n",
				value);
		}
		if (sscanf(value, "%x", &ratio) == 0)	{
			error("setparm", "cannot parse value of sensitivity");
			return -1;
		}
		Ratio(ndx) = ratio;
		return 0;
	}
	if ( ! strcmp(parm, "STTY") )	{
		char	*p;
		while ( (p=strtoken(value, DELIMITERS)) != (char*) NULL)
			if ( setstty(ndx, p) == -1 )	{
				error("setparm","can't set stty parms");
				return -1;
			}
		return 0;
	}
	sprintf(buf, "``%s'' is an unrecognized parameter", parm);
	error("setparm",buf);
	return -1;
}

/*
 * setstty
 *
 * Passed in a device index and an STTY parameter, acts on the parameter.
 */
static int
setstty(ndx, value)
int	ndx;
char	*value;
{
	char	buf[80];

	make_upper(value);
#ifdef DEBUG
	printf("setstty: assigning STTY value ``%s''\n",value);
#endif
	if ( ! strcmp(value, "300") )
		return setbaud(ndx, 300);
	if ( ! strcmp(value, "1200") )
		return setbaud(ndx, 1200);
	if ( ! strcmp(value, "2400") )
		return setbaud(ndx, 2400);
	if ( ! strcmp(value, "4800") )
		return setbaud(ndx, 4800);
	if ( ! strcmp(value, "9600") )
		return setbaud(ndx, 9600);
	if ( ! strcmp(value, "CS7") )
		return setcs(ndx, 7);
	if ( ! strcmp(value, "CS8") )
		return setcs(ndx, 8);
	if ( ! strcmp(value, "PARODD") )
		return setparodd(ndx,1);
	if ( ! strcmp(value, "-PARODD") )
		return setparodd(ndx,0);
	if ( ! strcmp(value, "PARENB") )
		return setparenb(ndx,1);
	if ( ! strcmp(value, "-PARENB") )
		return setparenb(ndx,0);
	if ( ! strcmp(value, "CSTOPB") )
		return setstopb(ndx,1);
	if ( ! strcmp(value, "-CSTOPB") )
		return setstopb(ndx,0);
	if ( ! strcmp(value, "IXON") )
		return setixon(ndx, 1);
	if ( ! strcmp(value, "-IXON") )
		return setixon(ndx, 0);
	if ( ! strcmp(value, "IXOFF") )
		return setixoff(ndx, 1);
	if ( ! strcmp(value, "-IXOFF") )
		return setixoff(ndx, 0);
	sprintf(buf, "``%s'' is an unrecognized stty parameter", value);
	error("setstty", buf);
	return -1;
}

static int
setbaud(ndx, baud)
{
	long	work=0;

	if ( table[ndx].stty & STTY_HAVEBAUD )	{
		error("setbaud","baud rate assigned more than once");
		return -1;
	}
	switch (baud) {
		case 300:	work = B300;
				break;
		case 1200:	work = B1200;
				break;
		case 2400:	work = B2400;
				break;
		case 4800:	work = B4800;
				break;
		case 9600:	work = B9600;
				break;
		default:	return -1;
	}
	table[ndx].cflag &= ~CBAUD;
	table[ndx].cflag |= work;
	table[ndx].stty |= STTY_HAVEBAUD;
	return 0;
}

/* set character size for serial devices */
static int
setcs(ndx, cs)
{
	long	work=0;

	if ( table[ndx].stty & STTY_HAVECS )	{
		error("setcs","character size assigned more than once");
		return -1;
	}
	if (cs == 7)
		work |= CS7;
	else if (cs == 8)
		work |= CS8;
	else
		return -1;
	table[ndx].cflag &= ~CSIZE;
	table[ndx].cflag |= work;
	table[ndx].stty |= STTY_HAVECS;
	return 0;
}

static int
setixoff(ndx, yesno)
{
	table[ndx].stty |= STTY_HAVEIXOFF;
	if (yesno)
		table[ndx].cflag |= IXOFF;
	else
		table[ndx].cflag &= ~IXOFF;
	return 0;
}

static int
setixon(ndx, yesno)
{
	table[ndx].stty |= STTY_HAVEIXON;
	if (yesno)
		table[ndx].cflag |= IXON;
	else
		table[ndx].cflag &= ~IXON;
	return 0;
}

static int
setparenb(ndx, yesno)
{
	table[ndx].stty |= STTY_HAVEPARENB;
	if (yesno)
		table[ndx].cflag |= PARENB;
	else
		table[ndx].cflag &= ~PARENB;
	return 0;
}

static int
setparodd(ndx, yesno)
{
	table[ndx].stty |= STTY_HAVEPARODD;
	if (yesno)
		table[ndx].cflag |= PARODD;
	else
		table[ndx].cflag &= ~PARODD;
	return 0;
}

static int
setstopb(ndx, yesno)
{
	table[ndx].stty |= STTY_HAVESTOPB;
	if (yesno)
		table[ndx].cflag |= CSTOPB;
	else
		table[ndx].cflag &= ~CSTOPB;
	return 0;
}

static short
xlat_class(s)
char	*s;
{
	char	buf[80];
	short	butt=0;

	make_upper(s);
	if (s[strlen(s)-1] == 'B')	{
		butt = D_BUTTON;
		s[strlen(s)-1] = 0;
	}

	if ( ! strcmp(s, C_ABS) )
		return (short) (D_ABS | butt);
	if ( ! strcmp(s, C_REL) )
		return (short) (D_REL | butt);
	if ( ! strcmp(s, C_STRING) )
		return (short) (D_STRING | butt);
	if ( ! strcmp(s, C_OTHER) )
		return (short) (D_OTHER | butt);

	sprintf(buf, "``%s'' is an unrecognized class", s);
	error("xlat_class", buf);
	return -1;
}


/*
 * This returns a token from a string.
 * A token is either a string of nonspace characters or a quoted
 * string of any characters.
 */
static char *
getvalue(s)
char	*s;
{
	char	*p, tok[6];;

	if ( ! ISQUOTE(*s) )
		strcpy(tok, WHITESPACE);
	else	{
		tok[0] = *s;
		tok[1] = 0;
	}
	if ( (p = strtoken(s, tok)) == NULL)	{
		error("getvalue","no value found");
		return (char*) NULL;
	}
	return p;
}

/*
 * d_open
 *
 * Open a device for events.
 *
 * This routine opens a graphics input device and attaches
 * it to the open event queue.
 *
 * Algorithm:	1) open device, RDWR if there is an initialization
 *		string, otherwise RDONLY.
 *		2) set STTY parameters is any.
 *		3) write the initialization string, if any.
 *		4) set to event line discipline (2).
 *		5) ioctl in the device type.
 *		6) ioctl in the device ratio, if any.
 *		7) ioctl to attach the device to the event queue.
 */
static
d_open(evqfd, ndx)
int	evqfd;
int	ndx;
{
	char	buf[80],*path;
	int	fd, mode;
	struct termio work;

#ifdef DEBUG
	printf("d_open: enter with evqfd==%d, ndx==%d\n",evqfd,ndx);
#endif
	mode = O_RDONLY;
	if ( *table[ndx].init )
		mode = O_RDWR;

	path = table[ndx].pathname;
#ifdef DEBUG
	printf("d_open: open %s for %s\n", path,
			mode == O_RDWR ? "read/write" : "read only");
#endif
	if ( ! strcmp(path, "stdin") )	{
		fd = 0;
		ioctl(fd, TCGETA, &tsave);
	}
	else if ( ! strcmp(path, "stdout") )
		fd = 1;
	else if ( ! strcmp(path, "stderr") )
		fd = 2;
	else	{
		fd = open(path, mode);
		fcntl(qfd, F_SETFD, 1);	/* Set close-on-exec bit */
	}
	if ( fd == -1 )	{
		sprintf(buf, "unable to open device ``%s''", path);
		if ( mode == O_RDWR )
			strcat(buf, " for read/write");
		else
			strcat(buf, "for read only");
		error("d_open",buf);
		return -1;
	}
#ifdef DEBUG
	printf("d_open: new device is fd %d\n", fd);
#endif
	table[ndx].fd = fd;
	if ( ioctl(fd, TCGETA, &work) == -1) {
		sprintf(buf, "cannot ioctl (TCGETA) %s (errno %d)",
							path, errno);
		error("d_open", buf);
		close(fd);
		return -1;
	}
	/* Set the new stty parameters, if any */
	if (table[ndx].stty & STTY_HAVEBAUD) {
		work.c_cflag &= ~CBAUD;
		work.c_cflag |= (table[ndx].cflag & CBAUD);
	}
	if (table[ndx].stty & STTY_HAVECS) {
		work.c_cflag &= ~CSIZE;
		work.c_cflag |= (table[ndx].cflag & CSIZE);
	}
	if (table[ndx].stty & STTY_HAVEPARENB)	{
		work.c_cflag &= ~PARENB;
		work.c_cflag |= (table[ndx].cflag & PARENB);
	}
	if (table[ndx].stty & STTY_HAVEPARODD)	{
		work.c_cflag &= ~PARODD;
		work.c_cflag |= (table[ndx].cflag & PARODD);
	}
	if (table[ndx].stty & STTY_HAVESTOPB)	{
		work.c_cflag &= ~CSTOPB;
		work.c_cflag |= (table[ndx].cflag & CSTOPB);
	}
	if ( table[ndx].stty && ioctl(fd, TCSETA, &work) == -1 ) {
		sprintf(buf, "can't set serial parameters for %s",path);
		error("d_open", buf);
		close(fd);
		return -1;
	}

#ifdef DEBUG
	printf("d_open: setting device to event line discipline\n");
#endif
	/* Set the device to the event line discipline */
	work.c_line = EVENT_LD;
	if ( ioctl(fd, TCSETA, &work) == -1 ) {
		sprintf(buf, "can't change %s to event line discipline, errno %d", path, errno);
		error("d_open", buf);
		if ( fd > 2 )
			close(fd);
		else
			ioctl(fd, TCSETA, &tsave);
		return -1;
	}
	/* Set device type */
#ifdef	DEBUG
	printf("d_open: type of %s is %s\n",path,Type(ndx));
#endif
	if ( ioctl(fd, LDEV_SETTYPE, Type(ndx)) == -1 ) {
		sprintf(buf, "can't set type of %s to %s, errno (%d)", 
						path, Type(ndx),errno);
		error("d_open", buf);
		if ( fd != 0 )
			close(fd);
		else
			ioctl(fd, TCSETA, &tsave);
		return -1;
	}
	/* Write the initialization string, if any */
	/* Must set type before writing initialization, */
	/* that way if we fail in setting type, (device busy?), */
	/* we don't screw up the other user */
	if ( Init(ndx)[0] )
		write(fd, Init(ndx), (unsigned) strlen(Init(ndx)) );
	/* Attach the device to the event queue */
	if ( ioctl(fd, LDEV_ATTACHQ, evqfd) == -1 ) {
		sprintf(buf, "can't attach %s to the event queue, errno %d",path, errno);
		error("d_open", buf);
		if ( fd != 0 )
			close(fd);
		else
			ioctl(fd, TCSETA, &tsave);
		return -1;
	}
	/* Set device ratio */
	if ( Ratio(ndx) && ioctl(fd, LDEV_SETRATIO, Ratio(ndx)) == -1 ){
		sprintf(buf, "can't set ratio of %s to %d", path, Ratio(ndx));
		error("d_open", buf);
		close(fd);
		return -1;
	}
	/* device is open, configured, and attached */
	Devstatus(ndx) |= D_OPEN;
	/* Fill in user available information */
	table[ndx].d_userinfo.handle = ndx;				/*S002*/
	table[ndx].d_userinfo.class = Class(ndx);			/*S002*/
	table[ndx].d_userinfo.type = Type(ndx);				/*S002*/
	table[ndx].d_userinfo.handle = ndx;				/*S002*/
	return 0;
}

static
d_close(ndx)
int	ndx;
{
	if ( (Devstatus(ndx) & D_OPEN) == 0 )
		return -1;
	Devstatus(ndx) &= ~D_OPEN;
	Class(ndx) = 0;
	/* Never close file descriptor 0 */
	if ( table[ndx].fd == 0 )
		ioctl(0, TCSETA, &tsave);
	else
		close(table[ndx].fd);
	return 0;
}

/*
 * Close a device. Implemented as part of S002.
 */
static
d_suspend(ndx)
int	ndx;
{
	int ret;

	printf("closing %s\n",table[ndx].d_userinfo.name);
	/* Never close file descriptor 0 */
	if ( table[ndx].fd == 0 )
		ret = ioctl(0, TCSETA, &tsave);
	else
		ret = close(table[ndx].fd);
	if ( ! ret )
		Devstatus(ndx) |= D_SUSPENDED;
	return ret;
}

/*
 * Reopen a device. Implemented as part of S002.
 */
static
d_activate(ndx)
int	ndx;
{
	int ret;

	ret = d_open(qfd, ndx);
	if ( ! ret ) 
		Devstatus(ndx) &= ~D_SUSPENDED;
	return 0;
}

/*
 * This routine compares an even mask to the types of the devices feeding
 * the queue to determine if the combination would produce events. If setting
 * this as the event mask would still allow events to be enqueued, the function
 * returns 1, otherwise it returns 0.
 */
static
check4events(emask_t emask)				/*	S005	*/
{
	short	i;

	for (i=0; i<MAXDEVS; i++)
		if ( table[i].devstatus & D_ISVALID )
			if ( table[i].devstatus & D_OPEN )
				if ( ! ( table[i].devstatus & D_SUSPENDED) )
					if ( table[i].class & emask )
						return 1;
	return 0;
}

/*
 * fgetline()
 *
 * Entries in data files used by these routines may span multiple
 * lines if a '\' character is the last character on the line. This
 * routine does that parsing by reading the entire line/entry into
 * a buffer.
 *
 * It returns the number of characters read or -1 on EOF.
 */
#define	BACKSLASH	0x5c

static int
fgetline(FILE * fp, char * s, short len)		/*	S005	*/
{
	int	tmp, end = -1, gotsomething = 0;

	*s = 0;
	while(1)	{
		tmp = strlen(s);
		if ( fgets(s+tmp, len - tmp, fp) == (char*) NULL )
			break;
		gotsomething++;
		if ( (end = strlen(s) - 1) == -1 )
			break;
		if ( s[end] == '\n' )
			s[end--] = 0;	/* fgets leaves the '\n' */
		if ( s[end] == BACKSLASH )
			s[end] = 0;
		else
			break;
	}
	if ( ! gotsomething )
		end = -1;
	else
		end++;
	return end;
}

/*
 * similar to strtok(S) except that you do not pass in
 * zero for the second and subsequent calls. You pass in a
 * a string and a token string and a pointer to the first word in the
 * string is returned.
 */
static char *
strtoken(s, tokens)
char	*s;
char	*tokens;
{
	int	i=0, start;
	static char buf[256];

	if ( ! *s )
		return (char*) NULL;
	while ( strchr(tokens, s[i]) != (char*) NULL )
		i++;		/* Skip leading tokens */
	start = i;
	while ( s[i] && strchr(tokens, s[i]) == (char*) NULL )
		i++;		/* Skip to next token */
	strncpy(buf, s+start, i-start);
	buf[i-start] = 0;
	if (s[i] == 0)
		*s = 0;
	else
		strcpy(s, s+i+1);
	start = strlen(s);
	strcpy(s + start + 1, buf);
	return s + start + 1;
}

static void
make_upper(s)
char	*s;
{
	while (*s)	{
		if ( islower(*s) )
			*s = toupper(*s);
		s++;
	}
}

/*
 * xlat_octals
 *
 * Go through the string and convert \ooo to its character (binary)
 * equivalent.
 */
static void
xlat_octals(s)
char	*s;
{
	int	num,i;
	char	*writep;

	writep = s;
	while (*s)	{
		if ( *s !=  BACKSLASH )	{	/* normal character */
			*writep++ = *s++;
			continue;
		}
		s++;				/* "read" the backslash */
		if ( *s == BACKSLASH ) {		/* '\\' */
			*writep++ = *s++;
			continue;
		}
		/* It's probably the beginning of an octal */
		/* see if there's at least one digit, if not, remove the \ */
		if ( ! isodigit(*s) )
			continue;
		for ( num=0, i=0 ; i<3 && isodigit(*s) ; i++, s++)
			num = num * 8 + (*s - '0');
		*writep++ = (char) num;
	}

}

static int
isodigit(char c)					/*	S005	*/
{
	return (int) strchr("01234567", c);
}

#ifdef NEVER
error("ev_init","stdout must be a tty");
error("ev_init","cannot load GIN device keys");
error("ev_init","cannot load GIN device information");
error("ev_open","cannot open the event queue");
error("ev_open","error attaching device");
error("openqueue","can't get (ioctl) a queue pointer");
error("load_keys","invalid (i.e. NULL) argument");
error("load_keys","cannot open evterms file");
error("load_keys",toolong);
"error parsing information for device key ``%s''"
error("load_devices","invalid (i.e. NULL) argument");
error("load_devices","cannot open ev_devices file");
error("load_devs","unmatched key");
"unrecognized device type ``%s''"
"unrecognized device class ``%s''"
error("loadinfo", "missing device pathname");
error("loadinfo", "missing device class");
printf("loadinfo: class is untranslatable\n");
error("loadinfo", "missing device type");
error("parseparms","bad syntax");
sprintf(buf, "can't parse value of %s", parm);
sprintf(buf,"can't set argument to ``%s=''",parm);
error("setparm", "cannot parse value of sensitivity");
error("setparm","can't set stty parms");
sprintf(buf, "``%s'' is an unrecognized parameter", parm);
sprintf(buf, "``%s'' is an unrecognized stty parameter", value);
error("setbaud","baud rate assigned more than once");
error("setcs","character size assigned more than once");
sprintf(buf, "``%s'' is an unrecognized class", s);
sprintf(buf, "``%s'' is an unrecognized type", s);
sprintf(buf, "unable to open device ``%s''", path);
	strcat(buf, " for read/write");
	strcat(buf, "for read only");
sprintf(buf, "cannot ioctl (TCGETA) %s", path);
sprintf(buf, "can't set serial parameters for %s",path);
sprintf(buf, "can't change %s to event line discipline, erno %d", path, errno);
sprintf(buf, "can't set type of %s to %d", path, Type(ndx));
sprintf(buf, "can't attach %s to the event queue, errno %d",path, errno);
sprintf(buf, "can't set ratio of %s to %d", path, Ratio(ndx));
#endif
