/*		copyright	"%c%" 	*/

#ident	"@(#)adt_proc.c	1.2"
#ident  "$Header$"

/**
 ** This file contains routines to allocate and extract process data blocks.
 ** A list of process data blocks (plist structures) is maintained in
 ** decreasing order of pid (i.e. headp points to the process with the highest
 ** pid), since it is most likely that a higher pid will be searched.
 ** Since process and groups records are always dumped before the first
 ** event record is dumped for a process, if log files are processed in a 
 ** chronological order since auditon, the process block information 
 ** (proc and groups members of the dump record) will always be saved in this 
 ** list when an auditable event record for a given process is encountered.
 **/

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <sys/privilege.h>
#include "audit.h"
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include "auditrec.h"
#include "auditrptv1.h"

/**
 ** type definition of the process block entry kept for each process in the list
 **/
struct	plist {
	struct	plist	*next;
	pid_t		pid;
	struct proc_r	proc;
	long		ngroups;
	gid_t		groups[NGROUPS_MAX];
};

static	struct	plist	*head = (struct plist *)NULL;

static	struct	plist	*listent(),
			*makeproc(),
			*findproc();

#define	SIZ_GID		sizeof(gid_t)
#define	SIZ_PLIST	sizeof(struct plist)

/**
 ** Set or create process information for the process indicated by
 ** cmn->c_pid.
 **/
void
setproc(cmn, proc)
cmnrec_t	*cmn;
struct proc_r	*proc;
{
	struct	plist	*p;
	p = makeproc(cmn->c_pid);
	(void)memcpy(&p->proc, proc, SIZ_PROC);
}

/**
 ** Change or create the multiple groups list for the process indicated by
 ** cmn->c_pid.
 **/
void
setgrplist(cmn, n, grps)
cmnrec_t	*cmn;
long		n;
long		*grps;
{
	struct	plist	*p;

	p = makeproc(cmn->c_pid);
	(void)memcpy(p->groups, grps, n * SIZ_GID);
	p->ngroups = n;
}

/**
 ** Get the process information of the process indicated by dump_rec->cmn.c_pid
 ** and fill dump_rec appropriately.  Return -1 if the process information is
 ** not found.
 **/

int
getproc(dump_rec)
struct	rec	*dump_rec;
{
	struct	plist	*p;

	p = findproc(dump_rec->cmn.c_pid);
	if(!p){
		return (-1);
	}
	(void)memcpy(&dump_rec->proc, &p->proc, SIZ_PROC);
	dump_rec->ngroups = p->ngroups;
	if (p->ngroups > 0)
		(void)memcpy(dump_rec->groups,p->groups,(p->ngroups) * SIZ_GID);
	return(0);
}

/**
 ** Look up a particular process data block (based on PID) and return a
 ** pointer to the data.  If the process is not found, return a NULL
 ** pointer.
 **/
struct plist *
findproc(pid)
pid_t	pid;
{
	struct	plist	*p;

	p = head;
	while(p){
		if(p->pid == pid){
			return(p);
		}
		if(p->pid < pid){
			return((struct plist *)NULL);
		}
		p = p->next;
	}
	return((struct plist *)NULL);
}

/**
 ** Find the requested process data block in the list of process data
 ** blocks.  If the block is not found, allocate a new one, insert it in
 ** the list (in decreasing order of pid) and initialize the data.
 **/
struct plist *
makeproc(pid)
pid_t	pid;
{
	struct plist	*lp,*plp;

	lp = findproc(pid);
	if(lp){
		return(lp);
	}
	lp = head;
	if(!head || (head->pid < pid)){
		head = listent(pid, lp);
		return(head);
	}
	plp = lp = head;
	while(lp && lp->pid > pid){
		plp = lp;
		lp = plp->next;
	}
	plp->next = listent(pid,lp);
	return(plp->next);
}


/**
 ** Allocate and initialize a list entry and append the list indicated by
 ** 'next' to it.
 **/
struct plist	*
listent(pid,next)
pid_t		pid;
struct plist	*next;
{
	struct	plist	*p;

	if ((p = (struct plist *)malloc(SIZ_PLIST)) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}
	p->next = next;
	p->pid = pid;
	p->proc.pr_uid = -1;
	p->proc.pr_ruid = -1;
	p->proc.pr_gid = -1;
	p->proc.pr_rgid = -1;
	p->proc.pr_sid = -1;
	p->proc.pr_lid = 0;
	p->ngroups = -1;
	return(p);
}
