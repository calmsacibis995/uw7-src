#ident	"@(#)announce.c	1.4"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include "ctl.h"

#include <sys/stat.h>
#include <sgtty.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>

extern int errno;
extern char hostname[];
int nofork = 0;		/* to be set from the debugger */

/*
 * Because the tty driver insists on attaching a terminal-less
 * process to any terminal that it writes on, we must fork a child
 * to protect ourselves
 */

announce(request, remote_machine)
CTL_MSG *request;
char *remote_machine;
{
    pid_t pid, val;
    int status;

    if (nofork) {
	return(announce_proc(request, remote_machine));
    }

    if ( pid = fork() ) {

	    /* we are the parent, so wait for the child */

	if (pid == (pid_t)-1) {
		/* the fork failed */
	    return(FAILED);
	}

	do {
	    val = wait(&status);
	    if (val == (pid_t)-1) {
		if (errno == EINTR) {
		    continue;
		} else {
			/* shouldn't happen */
		    print_error("wait");
		    return(FAILED);
		}
	    }
	} while (val != pid);

	if (status&0377 > 0) {
		/* we were killed by some signal */
	    return(FAILED);
	}

	    /* Get the second byte, this is the exit/return code */

	return((status>>8)&0377);

    } else {
	    /* we are the child, go and do it */
	_exit(announce_proc(request, remote_machine));
    }
}
	

    /* See if the user is accepting messages. If so, announce that 
       a talk is requested.
     */

announce_proc(request, remote_machine)
CTL_MSG *request;
char *remote_machine;
{
    char full_tty[32];
    FILE *tf;
    struct stat stbuf;


    (void) snprintf(full_tty, sizeof(full_tty), "/dev/%s", request->r_tty);

    if (access(full_tty, 0) != 0) {
	return(FAILED);
    }

    if ((tf = fopen(full_tty, "w")) == NULL) {
	return(PERMISSION_DENIED);
    }

	/* open gratuitously attaches the talkd to
	   any tty it opens, so disconnect us from the
	   tty before we catch a signal */

#ifdef SYSV
    (void) setsid();
#else
    ioctl(fileno(tf), TIOCNOTTY, (struct sgttyb *) 0);
#endif SYSV

    if (fstat(fileno(tf), &stbuf) < 0) {
	return(PERMISSION_DENIED);
    }

    if ((stbuf.st_mode&020) == 0) {
	return(PERMISSION_DENIED);
    }

    print_mesg(tf, request, remote_machine);
    fclose(tf);
    return(SUCCESS);
}

#define max(a,b) ( (a) > (b) ? (a) : (b) )
#define N_LINES 5
#define N_CHARS 120
static void convert(char *buf, char *, int);

    /*
     * build a block of characters containing the message. 
     * It is sent blank filled and in a single block to
     * try to keep the message in one piece if the recipient
     * in in vi at the time
     */

print_mesg(tf, request, remote_machine)
FILE *tf;
CTL_MSG *request;
char *remote_machine;
{
    struct timeval clock;
    struct tm *localtime();
    struct tm *localclock;
    char line_buf[N_LINES][N_CHARS];
    int sizes[N_LINES];
    char big_buf[N_LINES*N_CHARS];
    char *bptr, *lptr;
    int i, j, max_size;
    char con_name[N_CHARS];
    char con_host[N_CHARS];

    i = 0;
    max_size = 0;

    gettimeofday(&clock, (struct timezone *) NULL);
    localclock = localtime( &clock.tv_sec );

    (void) snprintf(line_buf[i], N_CHARS, " ");

    sizes[i] = strlen(line_buf[i]);
    max_size = max(max_size, sizes[i]);
    i++;

    (void) snprintf(line_buf[i], N_CHARS,
	"Message from Talk_Daemon@%s at %d:%02d ...",
	hostname, localclock->tm_hour , localclock->tm_min );

    sizes[i] = strlen(line_buf[i]);
    max_size = max(max_size, sizes[i]);
    i++;

    convert(request->l_name, con_name, sizeof(con_name));
    convert(remote_machine, con_host, sizeof(con_host));
    (void) snprintf(line_buf[i], N_CHARS,
	"talk: connection requested by %s@%s.", con_name, con_host);

    sizes[i] = strlen(line_buf[i]);
    max_size = max(max_size, sizes[i]);
    i++;

    (void) snprintf(line_buf[i], N_CHARS, "talk: respond with:  otalk %s@%s",
		con_name, con_host);

    sizes[i] = strlen(line_buf[i]);
    max_size = max(max_size, sizes[i]);
    i++;

    (void) snprintf(line_buf[i], N_CHARS, " ");

    sizes[i] = strlen(line_buf[i]);
    max_size = max(max_size, sizes[i]);
    i++;

    bptr = big_buf;
    *(bptr++) = '\007';	/* send something to wake them up */
    *(bptr++) = '\r';	/* add a \r in case of raw mode */
    *(bptr++) = '\n';
    for(i = 0; i < N_LINES; i++) {

	    /* copy the line into the big buffer */

	lptr = line_buf[i];
	while (*lptr != '\0') {
	    *(bptr++) = *(lptr++);
	}

	    /* pad out the rest of the lines with blanks */

	for(j = sizes[i]; j < max_size + 2; j++) {
	    *(bptr++) = ' ';
	}

	*(bptr++) = '\r';	/* add a \r in case of raw mode */
	*(bptr++) = '\n';
    }
    *bptr = '\0';

    fprintf(tf, big_buf);
    fflush(tf);
#ifdef SYSV
    (void) setsid();
#else
    ioctl(fileno(tf), TIOCNOTTY, (struct sgttyb *) 0);
#endif
}

static void
convert(char *buf, char *con_buf, int size)
{
	--size;
	for ( ; *buf && size; ++buf)
	{
	    if (isprint(*buf))
	    {
		    *con_buf++ = *buf;
		    --size;
	    }
	    else
	    {
		if (size < 4)
		    break;

		/* show it as three octal digits */
		sprintf(con_buf, "\\%03o", *buf);

		con_buf += 4;
		size -= 4;
	    }
	}
	*con_buf = '\0';
}
