/*
 * Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
 *                                                                         
 *        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
 *                   SANTA CRUZ OPERATION INC.                             
 *                                                                         
 *   The copyright notice above does not evidence any actual or intended   
 *   publication of such source code.                                      
 */

/*
 * Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
 *                                                                         
 *        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
 *                   SANTA CRUZ OPERATION INC.                             
 *                                                                         
 *   The copyright notice above does not evidence any actual or intended   
 *   publication of such source code.                                      
 */

#ident	"@(#)popen.c	1.7"

/* Copyright (c) 1988 The Regents of the University of California. All rights
 * reserved.
 *
 * This code is derived from software written by Ken Arnold and published in
 * UNIX Review, Vol. 6, No. 8.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef lint
static char sccsid[] = "@(#)popen.c 5.9 (Berkeley) 2/25/91";
#endif /* not lint */

#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_GETRLIMIT
#include <time.h>
#include <sys/resource.h>
#endif

/* 
 * Special version of popen which avoids call to shell.  This insures noone
 * may create a pipe to a hidden program as a side effect of a list or dir
 * command. 
 */
static int *pids;
static int fds;
#define MAX_ARGV 100
#define MAX_GARGV 1000
FILE *
#ifdef __STDC__
ftpd_popen(char *program, char *type, int closestderr)
#else
ftpd_popen(program,type,closestderr)
char *program;
char *type;
int closestderr;
#endif
{
    register char *cp;
    FILE *iop;
    int argc,
      gargc,
      pdes[2],
      pid,i;
    char **pop,
     *argv[MAX_ARGV],
     *gargv[MAX_GARGV],
     *vv[2];
#ifdef __STDC__
    extern char **ftpglob(register char *v),
    **copyblk(register char **v),
     *strspl(register char *cp, register char *dp),
#else
    extern char **ftpglob(),
    **copyblk(),
     *strspl(),
#endif
      *globerr;
#ifdef HAVE_GETRLIMIT
        struct rlimit rlp;

		rlp.rlim_cur = rlp.rlim_max = RLIM_INFINITY;
		if (getrlimit( RLIMIT_NOFILE, &rlp ) )
			return(NULL);
		fds = rlp.rlim_cur;
#else
#ifdef HAVE_GETDTABLESIZE
        if ((fds = getdtablesize()) <= 0)
            return (NULL);
#else
#ifdef HAVE_SYSCONF
       fds = sysconf(_SC_OPEN_MAX);
#else
#ifdef OPEN_MAX
    fds=OPEN_MAX; /* need to include limits.h somehow */
#else
    fds = 31; /* XXX -- magic cookie*/
#endif
#endif
#endif
#endif
    if (*type != 'r' && *type != 'w' || type[1])
        return (NULL);

    if (!pids) {
        if ((pids = (int *) malloc((u_int) (fds * sizeof(int)))) == NULL)
              return (NULL);
        (void) memset((void *)pids, 0, fds * sizeof(int));
    }
    if (pipe(pdes) < 0)
        return (NULL);
    (void) memset((void *)argv, 0, sizeof(argv));

    /* break up string into pieces */
    for (argc = 0, cp = program;argc < MAX_ARGV ; cp = NULL)
        if (!(argv[argc++] = strtok(cp, " \t\n")))
            break;

    /* glob each piece */
    gargv[0] = argv[0];
    for (gargc = argc = 1; argc < MAX_ARGV && argv[argc]; argc++) {
        if (!(pop = ftpglob(argv[argc])) || globerr != NULL) 
	{ /* globbing failed */
            vv[0] = strspl(argv[argc], "");
            vv[1] = NULL;
            pop = copyblk(vv);
        }
        argv[argc] = (char *) pop;  /* save to free later */
        while (*pop && gargc < (MAX_GARGV - 1 ))
            gargv[gargc++] = *pop++;
    }
    gargv[gargc] = NULL;

    iop = NULL;
#ifdef UXW
    ENABLE_WORK_PRIVS;
    pid = vfork();
    CLR_WORKPRIVS_NON_ADMIN(0);
    switch (pid) {
#else  /* UXW */
    switch (pid = vfork()) {
#endif /* UXW */
    case -1:                    /* error */
        (void) close(pdes[0]);
        (void) close(pdes[1]);
        goto pfree;
        /* NOTREACHED */
    case 0:                 /* child */
        if (*type == 'r') {
            if (pdes[1] != 1) {
                dup2(pdes[1], 1);
                if (closestderr)
                    (void) close(2);
                else 
                    dup2(pdes[1], 2);  /* stderr, too! */
                (void) close(pdes[1]);
            }
            (void) close(pdes[0]);
        } else {
            if (pdes[0] != 0) {
                dup2(pdes[0], 0);
                (void) close(pdes[0]);
            }
            (void) close(pdes[1]);
        }
	for (i = 3; i < fds; i++)
                close(i);
	/* begin CERT suggested fixes */
	close(0); 
        i = geteuid();
	delay_signaling(); /* we can't allow any signals while euid==0: kinch */
        seteuid(0);
        setgid(getegid());
        setuid(i);
	enable_signaling(); /* we can allow signals once again: kinch */
	/* end CERT suggested fixes */
	execv(gargv[0], gargv);
        _exit(1);
    }
    /* parent; assume fdopen can't fail...  */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        (void) close(pdes[1]);
    } else {
        iop = fdopen(pdes[1], type);
        (void) close(pdes[0]);
    }
    pids[fileno(iop)] = pid;

  pfree:for (argc = 1; argc < MAX_ARGV && argv[argc]; argc++) {
        blkfree((char **) argv[argc]);
        free((char *) argv[argc]);
    }
    return (iop);
}

#ifdef __STDC__
ftpd_pclose(FILE * iop)
#else
ftpd_pclose(iop)
FILE * iop;
#endif
{
    register int fdes;
    int pid;
#if defined (SVR4)
    sigset_t sig, omask;
    int stat_loc;
    sigemptyset(&sig);
    sigaddset(&sig, SIGINT); sigaddset(&sig,SIGQUIT); sigaddset(&sig, SIGHUP);
#elif defined (_OSF_SOURCE)
    int omask;
    int status;
#elif defined(M_UNIX)
	int stat_loc;
	sigset_t oldmask, newmask;
#else
    int omask;
    union wait stat_loc;
#endif


    /* pclose returns -1 if stream is not associated with a `popened'
     * command, or, if already `pclosed'. */
    if (pids == 0 || pids[fdes = fileno(iop)] == 0)
        return (-1);
    (void) fclose(iop);
#ifdef SVR4
    sigprocmask( SIG_BLOCK, &sig, &omask);
#elif defined(M_UNIX)
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigaddset(&newmask, SIGQUIT);
    sigaddset(&newmask, SIGHUP);
    (void) sigprocmask(SIG_BLOCK, &newmask ,&oldmask);
#else
    omask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGHUP));
#endif

#ifdef _OSF_SOURCE
    while ((pid = wait(&status)) != pids[fdes] && pid != -1) ;
#else
#ifndef NeXT
    while ((pid = wait((int *) &stat_loc)) != pids[fdes] && pid != -1) ;
#else
    while ((pid = wait(&stat_loc)) != pids[fdes] && pid != -1) ;
#endif
#endif
    pids[fdes] = 0;
#ifdef SVR4
    sigprocmask( SIG_SETMASK, &omask, (sigset_t *)NULL);
    return(pid == -1 ? -1 : WEXITSTATUS(stat_loc));
#elif defined(M_UNIX)
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t)0);
#else
    (void)sigsetmask(omask);
#ifdef _OSF_SOURCE
    return (pid == -1 ? -1 : status);
#elif defined(M_UNIX) || defined(LINUX)
	return (pid == -1 ? -1 : WEXITSTATUS(stat_loc));
#else
    return (pid == -1 ? -1 : stat_loc.w_status);
#endif
#endif
}



