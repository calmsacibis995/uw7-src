#ident	"@(#)pcnfsd_print.c	1.4"
#ident	"$Header$"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**	@(#)pcnfsd_print.c	1.12	1/29/93
**=====================================================================
*/

/*******************************************************************************
 *
 * FILENAME:    pcnfsd_print.c
 *
 * SCCS:	pcnfsd_print.c 1.4  11/11/97 at 16:21:28
 *
 * CHANGE HISTORY:
 *
 * 11-11-97  Paul Cunningham        MR us96-10602
 *           Changed to apply the esculation 132384 fix for ptf3025, see MR
 *           resolution for a description of the fix.
 *           Also changed pcnfsd_misc.c
 *
 *******************************************************************************
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
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif SHADOW_SUPPORT

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef SPOOLDIR
#define SPOOLDIR        "/var/spool/pcnfs"
#endif SPOOLDIR

/*
** The following defintions give the maximum time allowed for
** an external command to run (in seconds)
*/
#define MAXTIME_FOR_PRINT	10
#define MAXTIME_FOR_QUEUE	10
#define MAXTIME_FOR_CANCEL	10
#define MAXTIME_FOR_STATUS	10

#define QMAX 50

/*
** The following is derived from ucb/lpd/displayq.c
*/
#define SIZECOL 62
#define FILECOL 24

extern void     scramble();
extern void     *grab();
extern int      suspicious();
extern void     run_ps630();
extern char    *bigcrypt();
extern FILE    *su_popen();
extern int      su_pclose();
extern char    *my_strdup();
int             build_pr_list();
void		mon_printers();
char 	       *map_printer_name();
char	       *expand_alias();
void           *grab();
void            free_pr_list_item();
void            free_pr_queue_item();
pr_list		list_virtual_printers();

/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

extern int      errno;
extern int	interrupted;	/* in pcnfsd_misc.c */
struct stat     statbuf;
char            pathname[MAXPATHLEN];
char            new_pathname[MAXPATHLEN];
char            sp_name[MAXPATHLEN] = SPOOLDIR;
char            tempstr[256];
char            delims[] = " \t\r\n:()";

pr_list         printers = NULL;
pr_queue        queue = NULL;

/*
** valid_pr
**
** true if a printer name is valid
*/

int
valid_pr(pr)
char *pr;
{
char *p;
pr_list curr;

	mon_printers();

	if(printers == NULL)
		build_pr_list();

	if(printers == NULL)
		return(0); /* can't tell - assume it's bad */

	p = map_printer_name(pr);
	if (p == NULL)
		return(1);	/* must be ok if maps to NULL! */
	curr = printers;
	while(curr) {
		if(!strcmp(p, curr->pn))
			return(1);
		curr = curr->pr_next;
	}

	return(0);
}

/*
** pr_init
**
** create a spool directory for a client if necessary and return the info
*/
pirstat
pr_init(sys, pr, sp)
char *sys;
char *pr;
char**sp;
{
int    dir_mode = 0777;
int rc;
mode_t oldmask;

	*sp = &pathname[0];
	pathname[0] = '\0';

	if(suspicious(sys) || suspicious(pr))
		return(PI_RES_FAIL);

	/* get pathname of current directory and return to client */

	(void)sprintf(pathname,"%s/%s",sp_name, sys);
	oldmask = umask(0);
	(void)mkdir(sp_name, dir_mode);	/* ignore the return code */
	rc = mkdir(pathname, dir_mode);	/* DON'T ignore this return code */
	umask(oldmask);
	if((rc < 0 && errno != EEXIST) ||
	   (stat(pathname, &statbuf) != 0) ||
	   !(statbuf.st_mode & S_IFDIR)) {
	   (void)sprintf(tempstr,
		         gettxt(":156", "%s: unable to set up spool directory %s\n"),
			 "rpc.pcnfsd", pathname);
            msg_out(tempstr);
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_FAIL);
	    }
 	if (!valid_pr(pr)) 
           {
	    pathname[0] = '\0';	/* null to tell client bad vibes */
	    return(PI_RES_NO_SUCH_PRINTER);
	    } 
	return(PI_RES_OK);
}

/*
** pr_start2
**
** start a print job
*/
psrstat
pr_start2(system, pr, user, fname, opts, id)
char *system;
char *pr;
char *user;
char *fname;
char *opts;
char **id;
{
char            snum[20];
static char     req_id[256];
char            cmdbuf[512];
char            resbuf[256];
FILE *fd;
int i;
char *xcmd;
char *cp;
int failed = 0;

	*id = &req_id[0];
	req_id[0] = '\0';


	if(suspicious(system) || 
		suspicious(pr) ||
		suspicious(user) ||
		suspicious(fname))
		return(PS_RES_FAIL);

	(void)sprintf(pathname,"%s/%s/%s",sp_name,
	                         system,
	                         fname);	


	if (stat(pathname, &statbuf)) 
           {
	   /*
           **-----------------------------------------------------------------
	   ** We can't stat the file. Let's try appending '.spl' and
	   ** see if it's already in progress.
           **-----------------------------------------------------------------
	   */

	   (void)strcat(pathname, ".spl");
	   if (stat(pathname, &statbuf)) 
	      {
	      /*
              **----------------------------------------------------------------
	      ** It really doesn't exist.
              **----------------------------------------------------------------
	      */


	      return(PS_RES_NO_FILE);
	      }
	      /*
              **-------------------------------------------------------------
	      ** It is already on the way.
              **-------------------------------------------------------------
	      */


		return(PS_RES_ALREADY);
	     }

	if (statbuf.st_size == 0) 
	   {
	   /*
           **-------------------------------------------------------------
	   ** Null file - don't print it, just kill it.
           **-------------------------------------------------------------
	   */
	   (void)unlink(pathname);

	    return(PS_RES_NULL);
	    }
	 /*
         **-------------------------------------------------------------
	 ** The file is real, has some data, and is not already going out.
	 ** We rename it by appending '.spl' and exec "lpr" to do the
	 ** actual work.
         **-------------------------------------------------------------
	 */
	(void)strcpy(new_pathname, pathname);
	(void)strcat(new_pathname, ".spl");

	/*
        **-------------------------------------------------------------
	** See if the new filename exists so as not to overwrite it.
        **-------------------------------------------------------------
	*/


	if (!stat(new_pathname, &statbuf)) 
	   {
	   (void)strcpy(new_pathname, pathname);  /* rebuild a new name */
	   (void)sprintf(snum, "%d", rand());	  /* get some number */
	   (void)strncat(new_pathname, snum, 3);
	   (void)strcat(new_pathname, ".spl");	  /* new spool file */
	    }
	if (rename(pathname, new_pathname)) 
	   {
	   /*
           **---------------------------------------------------------------
	   ** Should never happen.
           **---------------------------------------------------------------
           */
	   (void)sprintf(tempstr, gettxt(":157", "%s: spool file rename (%s->%s) failed.\n"),
			"rpc.pcnfsd", pathname, new_pathname);
                msg_out(tempstr);
		return(PS_RES_FAIL);
	    }

		if (*opts == 'd') 
	           {
		   /*
                   **------------------------------------------------------
		   ** This is a Diablo print stream. Apply the ps630
		   ** filter with the appropriate arguments.
                   **------------------------------------------------------
		   */
		   char op[3];
		   strncpy(op,opts,2);
		   op[2] = '\0';
		   if (suspicious(op))
			return(PS_RES_FAIL);
		   (void)run_ps630(new_pathname, opts);
		   }
		/*
		** Try to match to an aliased printer
		*/
		xcmd = expand_alias(pr, new_pathname, user, system);
	        /*
                **----------------------------------------------------------
	        **  Use the copy option so we can remove the orignal spooled
	        **  nfs file from the spool directory.
                **----------------------------------------------------------
	        */
		if(!xcmd) {
			sprintf(cmdbuf, "/usr/bin/lp -c -d'%s' '%s'",
				pr, new_pathname);
			xcmd = cmdbuf;
		}
		if ((fd = su_popen(user, xcmd, MAXTIME_FOR_PRINT)) == NULL) {
			sprintf(tempstr, gettxt(":114", "%s: %s failed"),
				"rpc.pcnfsd", "su_popen");
			msg_out(tempstr);
			return(PS_RES_FAIL);
		}
		req_id[0] = '\0';	/* asume failure */
		while(fgets(resbuf, 255, fd) != NULL) {
			i = strlen(resbuf);
			if(i)
				resbuf[i-1] = '\0'; /* trim NL */
			if(!strncmp(resbuf, "request id is ", 14))
				/* New - just the first word is needed */
				strcpy(req_id, strtok(&resbuf[14], delims));
			else if (strembedded("disabled", resbuf))
				failed = 1;
		}
		if(su_pclose(fd) == 255)
			msg_out(gettxt(":158", "rpc.pcnfsd: su_pclose alert"));
		(void)unlink(new_pathname);
		return((failed | interrupted)? PS_RES_FAIL : PS_RES_OK);
}

/*
** Decide whether to blow away the printers list based
** on a change to a specified file or directory.
**
*/
void
mon_printers()
{
	static struct stat buf;
	static time_t	old_mtime = (time_t)0;
	char *mon_path = "/etc/lp/printers";

	if(stat(mon_path, &buf) == -1)
		return;
	if(old_mtime != (time_t)0) {
		if(old_mtime != buf.st_mtime && printers) {
			free_pr_list_item(printers);
			printers = NULL;
		}
	}
	old_mtime = buf.st_mtime;
}

/*
** build_pr_list
**
** build a list of valid printers
*/
int
build_pr_list()
{
pr_list last = NULL;
pr_list curr = NULL;
char buff[256];
char temp[256];
FILE *p;
char *cp;
int saw_system;

/*
 * In SVR4 the command to determine which printers are
 * valid is lpstat -v. The output is something like this:
 *
 * device for lp: /dev/lp0
 * system for pcdslw: hinode
 * system for bletch: hinode (as printer hisname)
 *
 * On SunOS using the SysV compatibility package, the output
 * is more like:
 *
 * device for lp is /dev/lp0
 * device for pcdslw is the remote printer pcdslw on hinode
 * device for bletch is the remote printer hisname on hinode
 * device for billie is on  joy
 * (if the /etc/printcap entry leaves out an "rp" field, using the "lp" default)
 * No device or machine is	specified for billie
 * (the last happens if a "whatever:tc=another" line appears in
 * the /etc/printcap file to establish an alias).
 *
 * It is possible to create logic that will handle any of these
 * possibilities, but note that such code is inherently sensitive
 * to slight reformatting of message output (and breaks horribly
 * under any kind of localization).
 */

	p = popen("/usr/bin/lpstat -v", "r");
	if(p == NULL) {
		printers = list_virtual_printers();
		return(1);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		cp = strtok(buff, delims);
		if(!cp)
			continue;
		if (strcmp(cp, "No") == 0) {
			static char *	badwords[] = {
				"device", "or", "machine", "is",
				"specified", "for", NULL
				};
			int		joy;

			for (joy = 0; badwords[joy] != NULL; ++joy)
				if ((cp = strtok(NULL, delims)) == NULL ||
					strcmp(cp, badwords[joy]) != 0)
						break;
			if (badwords[joy] != NULL)
				continue;
			if ((cp = strtok(NULL, delims)) == NULL)
				continue;
			if (strtok(NULL, delims) != NULL)
				continue;
			curr = (struct pr_list_item *)
				grab(sizeof (struct pr_list_item));
			curr->pn = my_strdup(cp);
			curr->device = my_strdup("?");
			curr->remhost = my_strdup("?");
			curr->cm = my_strdup(gettxt(":159", "(Incomplete configuration detected)"));
			curr->pr_next = NULL;
			goto yo_lo_tengo;
		}
		if(!strcmp(cp, "device"))
			saw_system = 0;
		else if (!strcmp(cp, "system"))
			saw_system = 1;
		else
			continue;
		cp = strtok(NULL, delims);
		if(!cp || strcmp(cp, "for"))
			continue;
		cp = strtok(NULL, delims);
		if(!cp)
			continue;
		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		curr->pn = my_strdup(cp);
		curr->device = NULL;
		curr->remhost = NULL;
		curr->cm = my_strdup("-");
		curr->pr_next = NULL;
		cp = strtok(NULL, delims);

		if(cp && !strcmp(cp, "is")) 
			cp = strtok(NULL, delims);

		if(!cp) {
			free_pr_list_item(curr);
			continue;
		}

		if(saw_system) {
			/* "system" OR "system (as printer pname)" */ 
			curr->remhost = my_strdup(cp);
			
			cp = strtok(NULL, delims);
			if(!cp) {
				/* simple format */
				curr->device = my_strdup(curr->pn);
				sprintf(temp, "on %s", curr->remhost);
			} else {
				/* "sys (as printer pname)" */
				if (strcmp(cp, "as")) {
					free_pr_list_item(curr);
					continue;
				}
				cp = strtok(NULL, delims);
				if (!cp || strcmp(cp, "printer")) {
					free_pr_list_item(curr);
					continue;
				}
				cp = strtok(NULL, delims);
				if(!cp) {
					free_pr_list_item(curr);
					continue;
				}
				curr->device = my_strdup(cp);
				sprintf(temp, "on %s as %s", curr->remhost, curr->device);
			}
			free(curr->cm); curr->cm = my_strdup(temp);
			
		}
		else if(!strcmp(cp, "on")) {	/* deal with missing name */
			cp = strtok(NULL, delims);
			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
			curr->device = my_strdup("lp");
			curr->remhost = my_strdup(cp);
		}
		else if(!strcmp(cp, "the")) {
			/* start of "the remote printer foo on bar" */
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "remote")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "printer")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
			curr->device = my_strdup(cp);
			cp = strtok(NULL, delims);
			if(!cp || strcmp(cp, "on")) {
				free_pr_list_item(curr);
				continue;
			}
			cp = strtok(NULL, delims);
			if(!cp) {
				free_pr_list_item(curr);
				continue;
			}
			curr->remhost = my_strdup(cp);
			sprintf(temp, "on %s as %s", curr->remhost, curr->device);
			free(curr->cm); curr->cm = my_strdup(temp);
		} else {
			/* the local name */
			curr->device = my_strdup(cp);
			curr->remhost = my_strdup("");
		}

yo_lo_tengo:
		if(last == NULL)
			printers = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) pclose(p);
/*
** Now add on the virtual printers, if any
*/
	if(last == NULL)
		printers = list_virtual_printers();
	else
		last->pr_next = list_virtual_printers();

	return(1);
}

/*
** free_pr_list_item
**
** free a list of printers recursively, item by item
*/
void
free_pr_list_item(curr)
pr_list curr;
{
	if(curr->pn)
		free(curr->pn);
	if(curr->device)
		free(curr->device);
	if(curr->remhost)
		free(curr->remhost);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_list_item(curr->pr_next); /* recurse */
	free(curr);
}

/*
** build_pr_queue
**
** generate a printer queue listing
*/

/*
** Print queue handling.
**
** Note that the first thing we do is to discard any
** existing queue.
*/

pirstat
build_pr_queue(pn, user, just_mine, p_qlen, p_qshown)
printername     pn;
username        user;
int            just_mine;
int            *p_qlen;
int            *p_qshown;
{
pr_queue last = NULL;
pr_queue curr = NULL;
char buff[256];
FILE *p;
char *owner;
char *job;
char *totsize;

	if(queue) {
		free_pr_queue_item(queue);
		queue = NULL;
	}
	*p_qlen = 0;
	*p_qshown = 0;

	pn = map_printer_name(pn);
	if(pn == NULL || !valid_pr(pn) || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);


/*
** In SVR4 the command to list the print jobs for printer
** lp is "lpstat lp" (or, equivalently, "lpstat -p lp").
** The output looks like this:
** 
** lp-2                    root               939   Jul 10 21:56
** lp-5                    geoff               15   Jul 12 23:23
** lp-6                    geoff               15   Jul 12 23:23
** 
** If the first job is actually printing the first line
** is modified, as follows:
**
** lp-2                    root               939   Jul 10 21:56 on lp
** 
** I don't yet have any info on what it looks like if the printer
** is remote and we're spooling over the net. However for
** the purposes of rpc.pcnfsd we can simply say that field 1 is the
** job ID, field 2 is the submitter, and field 3 is the size.
** We can check for the presence of the string " on " in the
** first record to determine if we should count it as rank 0 or rank 1,
** but it won't hurt if we get it wrong.
**/
	sprintf(buff, "/usr/bin/lpstat '%s'", pn);
	p = su_popen(user, buff, MAXTIME_FOR_QUEUE);
	if(p == NULL) {
		msg_out(gettxt(":160", "rpc.pcnfsd: unable to popen() lpstat queue query"));
		return(PI_RES_FAIL);
	}
	
	while(fgets(buff, 255, p) != NULL) {
		job = strtok(buff, delims);
		if(!job)
			continue;

		owner = strtok(NULL, delims);
		if(!owner)
			continue;

		totsize = strtok(NULL, delims);
		if(!totsize)
			continue;

		*p_qlen += 1;

		if(*p_qshown > QMAX)
			continue;

		if(just_mine && mystrcasecmp(owner, user))
			continue;

		*p_qshown += 1;

		curr = (struct pr_queue_item *)
			grab(sizeof (struct pr_queue_item));

		curr->position = *p_qlen;
		curr->id = my_strdup(job);
		curr->size = my_strdup(totsize);
		curr->status = my_strdup("");
		curr->system = my_strdup("");
		curr->user = my_strdup(owner);
		curr->file = my_strdup("");
		curr->cm = my_strdup("-");
		curr->pr_next = NULL;

		if(last == NULL)
			queue = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	(void) su_pclose(p);
	return(PI_RES_OK);
}

/*
** free_pr_queue_item
**
** recursively free a pr_queue, item by item
*/
void
free_pr_queue_item(curr)
pr_queue curr;
{
	if(curr->id)
		free(curr->id);
	if(curr->size)
		free(curr->size);
	if(curr->status)
		free(curr->status);
	if(curr->system)
		free(curr->system);
	if(curr->user)
		free(curr->user);
	if(curr->file)
		free(curr->file);
	if(curr->cm)
		free(curr->cm);
	if(curr->pr_next)
		free_pr_queue_item(curr->pr_next); /* recurse */
	free(curr);
}

/*
** get_pr_status
**
** generate printer status report
*/
pirstat
get_pr_status(pn, avail, printing, qlen, needs_operator, status)
printername   pn;
bool_t       *avail;
bool_t       *printing;
int          *qlen;
bool_t       *needs_operator;
char         *status;
{
char buff[256];
char cmd[64];
FILE *p;
int n;
pirstat stat = PI_RES_NO_SUCH_PRINTER;

/*
** New - SVR4 printer status handling.
**
** The command we'll use for checking the status of printer "lp"
** is "lpstat -a lp -p lp". Here are some sample outputs:
**
** 
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp disabled since Thu Feb 21 22:52:36 EST 1991. available.
** 	new printer
** ---
** pcdslw not accepting requests since Fri Jul 12 22:30:00 EDT 1991 -
** 	unknown reason
** printer pcdslw disabled since Fri Jul 12 22:15:37 EDT 1991. available.
** 	new printer
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp now printing lp-2. enabled since Sat Jul 13 12:02:17 EDT 1991. available.
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp now printing lp-2. enabled since Sat Jul 13 12:02:17 EDT 1991. available.
** ---
** lp accepting requests since Wed Jul 10 21:49:25 EDT 1991
** printer lp disabled since Sat Jul 13 12:05:20 EDT 1991. available.
** 	unknown reason
** ---
** pcdslw not accepting requests since Fri Jul 12 22:30:00 EDT 1991 -
** 	unknown reason
** printer pcdslw is idle. enabled since Sat Jul 13 12:05:28 EDT 1991. available.
**
** Note that these are actual outputs. The format (which is totally
** different from the lpstat in SunOS) seems to break down as
** follows:
** (1) The first line has the form "printername [not] accepting requests,,,"
**    This is trivial to decode.
** (2) The second line has several forms, all beginning "printer printername":
** (2.1) "... disabled"
** (2.2) "... is idle"
** (2.3) "... now printing jobid"
** The "available" comment seems to be meaningless. The next line
** is the "reason" code which the operator can supply when issuing
** a "disable" or "reject" command.
** Note that there is no way to check the number of entries in the
** queue except to ask for the queue and count them.
*/
	/* assume the worst */
	*avail = FALSE;
	*printing = FALSE;
	*needs_operator = FALSE;
	*qlen = 0;
	*status = '\0';

	pn = map_printer_name(pn);
	if(pn == NULL || !valid_pr(pn) || suspicious(pn))
		return(PI_RES_NO_SUCH_PRINTER);
	n = strlen(pn);

	sprintf(cmd, "/usr/bin/lpstat -a '%s' -p '%s'", pn, pn);

	p = popen(cmd, "r");
	if(p == NULL) {
		msg_out(gettxt(":161", "rpc.pcnfsd: unable to popen() lp status"));
		return(PI_RES_FAIL);
	}
	
	stat = PI_RES_OK;

	while(fgets(buff, 255, p) != NULL) {
		if(!strncmp(buff, pn, n)) {
			if(!strstr(buff, "not accepting"))
			*avail = TRUE;
			continue;
		}
		if(!strncmp(buff, "printer ", 8)) {
			if(!strstr(buff, "disabled"))
				*printing = TRUE;
			if(strstr(buff, "printing"))
				strcpy(status, "printing");
			else if (strstr(buff, "idle"))
				strcpy(status, "idle");
			continue;
		}
		if(!strncmp(buff, "UX:", 3)) {
			stat = PI_RES_NO_SUCH_PRINTER;
		}
	}
	(void) pclose(p);
	return(stat);
}

/*
** pr_cancel
**
** cancel job id on printer pr on behalf of user
*/

pcrstat pr_cancel(pr, user, id)
char *pr;
char *user;
char *id;
{
char            cmdbuf[256];
char            resbuf[256];
FILE *fd;
pcrstat stat = PC_RES_NO_SUCH_JOB;

	pr = map_printer_name(pr);
	if(pr == NULL || suspicious(pr) || !valid_pr(pr))
		return(PC_RES_NO_SUCH_PRINTER);
	if(suspicious(id))
		return(PC_RES_NO_SUCH_JOB);

	sprintf(cmdbuf, "/usr/bin/cancel '%s'", id);
	if ((fd = su_popen(user, cmdbuf, MAXTIME_FOR_CANCEL)) == NULL) {
		sprintf(tempstr, gettxt(":114", "%s: %s failed"),
			"rpc.pcnfsd", "su_popen");
		msg_out(tempstr);
		return(PC_RES_FAIL);
	}
/*
** For SVR4 we have to be prepared for the following kinds of output:
** 
** # cancel lp-6
** request "lp-6" cancelled
** # cancel lp-33
** UX:cancel: WARNING: Request "lp-33" doesn't exist.
** # cancel foo-88
** UX:cancel: WARNING: Request "foo-88" doesn't exist.
** # cancel foo
** UX:cancel: WARNING: "foo" is not a request id or a printer.
**             TO FIX: Cancel requests by id or by
**                     name of printer where printing.
** # su geoff
** $ cancel lp-2
** UX:cancel: WARNING: Can't cancel request "lp-2".
**             TO FIX: You are not allowed to cancel
**                     another's request.
**
** There are probably other variations for remote printers.
** Basically, if the reply begins with the string
**          "UX:cancel: WARNING: "
** we can strip this off and look for one of the following
** (1) 'R' - should be part of "Request "xxxx" doesn't exist."
** (2) '"' - should be start of ""foo" is not a request id or..."
** (3) 'C' - should be start of "Can't cancel request..."
**
** The fly in the ointment: all of this can change if these
** messages are localized..... :-(
*/
	if(fgets(resbuf, 255, fd) == NULL) 
		stat = PC_RES_FAIL;
	else if(!strstr(resbuf, "UX:"))
		stat = PC_RES_OK;
	else if(strstr(resbuf, "doesn't exist"))
		stat = PC_RES_NO_SUCH_JOB;
	else if(strstr(resbuf, "not a request id"))
		stat = PC_RES_NO_SUCH_JOB;
	else if(strstr(resbuf, "Can't cancel request"))
		stat = PC_RES_NOT_OWNER;
	else	stat = PC_RES_FAIL;

	if(su_pclose(fd) == 255)
		msg_out(gettxt(":158", "rpc.pcnfsd: su_pclose alert"));
	return(stat);
}

/*
** New subsystem here. We allow the administrator to define
** up to NPRINTERDEFS aliases for printer names. This is done
** using the "/etc/pcnfsd.conf" file, which is read at startup.
** There are three entry points to this subsystem
**
** void add_printer_alias(char *printer, char *alias_for, char *command)
**
** This is invoked from "config_from_file()" for each
** "printer" line. "printer" is the name of a printer; note that
** it is possible to redefine an existing printer. "alias_for"
** is the name of the underlying printer, used for queue listing
** and other control functions. If it is "-", there is no
** underlying printer, or the administrative functions are
** not applicable to this printer. "command"
** is the command which should be run (via "su_popen()") if a
** job is printed on this printer. The following tokens may be
** embedded in the command, and are substituted as follows:
**
** $FILE	-	path to the file containing the print data
** $USER	-	login of user
** $HOST	-	hostname from which job originated
**
** Tokens may occur multiple times. If The command includes no
** $FILE token, the string " $FILE" is silently appended.
**
** pr_list list_virtual_printers()
**
** This is invoked from build_pr_list to generate a list of aliased
** printers, so that the client that asks for a list of valid printers
** will see these ones.
**
** char *map_printer_name(char *printer)
**
** If "printer" identifies an aliased printer, this function returns
** the "alias_for" name, or NULL if the "alias_for" was given as "-".
** Otherwise it returns its argument.
**
** char *expand_alias(char *printer, char *file, char *user, char *host)
**
** If "printer" is an aliased printer, this function returns a
** pointer to a static string in which the corresponding command
** has been expanded. Otherwise ot returns NULL.
*/
#define NPRINTERDEFS	128
int num_aliases = 0;
struct {
	char *a_printer;
	char *a_alias_for;
	char *a_command;
} alias [NPRINTERDEFS];


char default_cmd[] = "lp $FILE";

void
add_printer_alias(printer, alias_for, command)
char *printer;
char *alias_for;
char *command;
{
/*
 * Add a little bullet-proofing here
 */
	if(alias_for == NULL || strlen(alias_for) == 0)
		alias_for = "lp";
	if(command == NULL || strlen(command) == 0)
		command = default_cmd;	/* see above */
	if(num_aliases < NPRINTERDEFS) {
		alias[num_aliases].a_printer = my_strdup(printer);
		alias[num_aliases].a_alias_for =
			(strcmp(alias_for,  "-") ? my_strdup(alias_for) : NULL);
		if(strstr(command, "$FILE"))
			alias[num_aliases].a_command = my_strdup(command);
		else {
			alias[num_aliases].a_command =
				(char *)grab(strlen(command) + 8);
			strcpy(alias[num_aliases].a_command, command);
			strcat(alias[num_aliases].a_command, " $FILE");
		}
		num_aliases++;
	}
}

/*
** list_virtual_printers
**
** build a pr_list of all virtual printers (if any)
*/
pr_list list_virtual_printers()
{
pr_list first = NULL;
pr_list last = NULL;
pr_list curr = NULL;
int i;



	if(num_aliases == 0)
		return(NULL);

	for (i = 0; i < num_aliases; i++) {
		curr = (struct pr_list_item *)
			grab(sizeof (struct pr_list_item));

		curr->pn = my_strdup(alias[i].a_printer);
		if(alias[i].a_alias_for == NULL)
			curr->device = my_strdup("");
		else
			curr->device = my_strdup(alias[i].a_alias_for);
		curr->remhost = my_strdup("");
		curr->cm = my_strdup("(alias)");
		curr->pr_next = NULL;
		if(last == NULL)
			first = curr;
		else
			last->pr_next = curr;
		last = curr;

	}
	return(first);
}

/*
** map_printer_name
**
** if a printer name is actually an alias, return the name of
** the printer for which it is an alias; otherwise return the name
*/
char *
map_printer_name(printer)
char *printer;
{
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer))
			return(alias[i].a_alias_for);
	}
	return(printer);
}

/*
** substitute
**
** replace a token in a string as often as it occurs
*/
static void
substitute(string, token, data)
char *string;
char *token;
char *data;
{
char temp[512];
char *c;

	while(c = strstr(string, token)) {
		*c = '\0';
		strcpy(temp, string);
		/* Quote the unquoted */
		if (c > string && c[-1] != '\'') {
			strcat(temp,"'");
			strcat(temp, data);
			strcat(temp,"'");
		} else
			strcat(temp, data);
		c += strlen(token);
		strcat(temp, c);
		strcpy(string, temp);
	}
}


/*
** expand_alias
**
** expand an aliased printer command by substituting file/user/host
*/
char *
expand_alias(printer, file, user, host)
char *printer;
char *file;
char *user;
char *host;
{
static char expansion[512];
int i;
	for (i = 0; i < num_aliases; i++){
		if(!strcmp(printer, alias[i].a_printer)) {
			strcpy(expansion, alias[i].a_command);
			substitute(expansion, "$FILE", file);
			substitute(expansion, "$USER", user);
			substitute(expansion, "$HOST", host);
			return(expansion);
		}
	}
	return(NULL);
}
