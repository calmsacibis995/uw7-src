#ident	"@(#)pcnfsd_v1.c	1.3"
#ident	"$Header$"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**      @(#)pcnfsd_v1.c	1.4     1/29/93
**=====================================================================
*/

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

#include "common.h"
#include "pcnfsd.h"
#include <stdio.h>
#include <pwd.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <limits.h>

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/

extern void     scramble();
extern char    *bigcrypt();

#ifdef WTMP
extern void wlogin();
#endif

extern struct passwd  *get_password();

/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

int             buggit = 0;

/*
**=====================================================================
**                      C O D E   S E C T I O N                       *
**=====================================================================
*/


/*ARGSUSED*/
void *pcnfsd_null_1(arg)
void *arg;
{
static char dummy;
return((void *)&dummy);
}

auth_results *pcnfsd_auth_1(arg)
auth_args *arg;
{
static auth_results r;

char            uname[32];
char            pw[64];
int             c1, c2;
char		*msgp;
struct passwd  *p;

	r.stat = AUTH_RES_FAIL;	/* assume failure */
	r.uid = (int)-2;
	r.gid = (int)-2;

	scramble(arg->id, uname);
	scramble(arg->pw, pw);

#ifdef USER_CACHE
	if(check_cache(uname, pw, &r.uid, &r.gid)) {
		 r.stat = AUTH_RES_OK;
#ifdef WTMP
		wlogin(uname);
#endif
		 return (&r);
   }
#endif

	p = get_password(uname, &msgp);
	if (p == (struct passwd *)NULL)
	   return (&r);

	c1 = strlen(pw);
	c2 = strlen(p->pw_passwd);
	if ((c1 && !c2) || (c2 && !c1) ||
	   (strcmp(p->pw_passwd, bigcrypt(pw, p->pw_passwd)))) 
           {
	   return (&r);
	   }

	r.stat = AUTH_RES_OK;
	r.uid = p->pw_uid;
	r.gid = p->pw_gid;
#ifdef WTMP
		wlogin(uname);
#endif

#ifdef USER_CACHE
	add_cache_entry(p);
#endif

return(&r);
}

pr_init_results *pcnfsd_pr_init_1(pi_arg)
pr_init_args *pi_arg;
{
static pr_init_results pi_res;

	pi_res.stat =
	  (pirstat) pr_init(pi_arg->system, pi_arg->pn, &pi_res.dir);

return(&pi_res);
}

pr_start_results *pcnfsd_pr_start_1(ps_arg)
pr_start_args *ps_arg;
{
static pr_start_results ps_res;
char *dummyptr;

	ps_res.stat =
	  (psrstat) pr_start2(ps_arg->system, ps_arg->pn, ps_arg->user,
	  ps_arg ->file, ps_arg->opts, &dummyptr);

return(&ps_res);
}
