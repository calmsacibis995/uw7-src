/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/actrec.c	1.38.3.6"

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "menudefs.h"
#include "ctl.h"
#include "terror.h"
#include "message.h"
#include "moremacros.h"
#include <unistd.h>

#define I18NMAX 1024            /*** Maximum length of output strings ***/

/* Functions for manipulating activation records */
static struct actrec *AR_head;
/* a pointer to the activation record for the current FMLI object */
       struct actrec *AR_cur;
static int AR_serial = 0;

void ar_dump();

/* LES: becoming a MACRO

struct actrec *
ar_get_current()
{
	return(AR_cur);
}
*/

struct actrec *
ar_create(rec)
register struct actrec *rec;
{
	register struct actrec *newrec;
	register struct actrec	*ap;

	newrec = (struct actrec *)new(struct actrec);
	*newrec = *rec;
	newrec->serial = AR_serial++;

	/* link the new record right after the current record */

	if (AR_head == NULL) {
		/* nobody has been made yet, so make a degenerate list */
		AR_head = newrec;
		newrec->nextrec = newrec;
		newrec->prevrec = newrec;
		newrec->backup = newrec;
	}
	else {
		ap = (AR_cur ? AR_cur : AR_head);
		newrec->prevrec = ap;
		newrec->nextrec = ap->nextrec;
		newrec->backup = ap;
		ap->nextrec = newrec;
		newrec->nextrec->prevrec = newrec;
	}
	return(newrec);
}

int
ar_reread(rec)
register struct actrec *rec;
{
	int ret;				   /* abs k15 */
	
	ret = arf_reread(rec, rec);
	if (rec == AR_cur && ret == SUCCESS)      /* abs k15 */
		ar_setslks(rec->slks, rec->flags & AR_ALTSLKS);
	return ret;
}

struct actrec *
ar_close(rec, logout)
register struct actrec *rec;
bool logout;
{
    register struct actrec	*ap;
    register struct actrec	*new_cur;
    struct actrec *ar_cur_prev();

    char i18n_msg[I18NMAX];

    if (rec == NULL && (rec = AR_cur) == NULL)
    {
	error(MUNGED, "NULL actrec");
	return(rec);
    }
    /* don't allow recursive closes! */
    if (rec->lifetime == AR_CLOSING) /* abs k17 */
	return(rec);		     /* abs k17 */

    /* re-evaluate lifetime - if immortal, don't close */
    if (rec->lifetime != AR_INITIAL)
	(void) ar_ctl(rec, CTSETLIFE, 0);
    if ((rec->lifetime == AR_IMMORTAL ||
	rec->lifetime == AR_INITIAL) && logout == FALSE) 	/* abs s15 */
    {
	(void)mess_err( gettxt(":195","Can't close this frame\n") ); /* abs s15 */
	return rec;
    }
    else
	rec->lifetime = AR_CLOSING; /* abs k17 */
    
    /* close the internals of the record */

    if (arf_close(rec, rec) == FAIL)
    {
        sprintf(i18n_msg, gettxt(":196","can't close %s"), "actrec" );
	error(MUNGED, i18n_msg); 
	return(AR_cur);
    }

    /* if closing current record and not exiting fmli, choose a new one */
    if (rec == AR_cur && !logout)
	new_cur = AR_cur->backup;
    else {
	if ( logout )
	    AR_cur = AR_head;
	new_cur = NULL;
    }

    /* relink the lists */
    if (rec->nextrec == rec)
	AR_head = NULL;
    else
    {
	if (rec == AR_head)
	    AR_head = rec->prevrec;
	rec->nextrec->prevrec = rec->prevrec;
	rec->prevrec->nextrec = rec->nextrec;
	for (ap = AR_cur; ap; )
	{
	    if (ap->backup == rec)
	    {
		ap->backup = rec->backup;
		break;
	    }
	    if ((ap = ap->backup) == AR_cur)
	    {
		/* rec = NULL;  causes core dump.  abs k14 */
		break;
	    }
	}
    }
    if (new_cur == rec)
	AR_cur = NULL;
    else if (new_cur)
    {
	register struct actrec	*old_AR_cur;
	void	ar_cur_end();

	if (new_cur->lifetime == AR_CLOSING) 	/* abs k17 */
	    AR_cur = new_cur;			/* abs k17 */
	else					/* abs k17 */
	{
	    /* -tf- 021992 : patch for reread/close - problem 
	       if arf_reinit() calls ar_close() new_cur & AR_cur
	       are invalid on return. So, store them before*/

		old_AR_cur = AR_cur; 	/* remember old value */
		AR_cur = new_cur;	/* reinit actual record */
		(void) arf_reinit(new_cur, new_cur);
					/* AR_cur is automatically set ... */
		ar_cur_end(old_AR_cur);
	}
    }
#ifdef _DEBUG
    _debug(stderr, "FREEING ACTREC %x\n", rec);
#endif
    if (rec)
	free(rec);
#ifdef _DEBUG5
    ar_dump("AFTER ar_close");
#endif
    return AR_cur;
}

struct actrec *
ar_cur_next()
{
	register struct actrec	*ap;
	struct actrec *ar_current();

	for (ap = AR_cur->nextrec; ap && (ap->flags & AR_SKIP); ap = ap->nextrec)
		if (ap == AR_cur)
			break;
	if (ap != AR_cur)
		ap = ar_current(ap, TRUE); /* abs k15 */
	return ap;
}

struct actrec *
ar_cur_prev()
{
	register struct actrec	*ap;
	struct actrec *ar_current();

	for (ap = AR_cur->prevrec; ap && (ap->flags & AR_SKIP); ap = ap->prevrec)
		if (ap == AR_cur)
			break;
	if (ap != AR_cur)
		ap = ar_current(ap, TRUE); /* abs k15 */
	return ap;
}

/*
 * front end for ar_current and ar_backup
 * cleans up previously current actrec and calls reinit on new current one
 */
static void
ar_cur_front(ap, do_reinit)
register struct actrec	*ap;
bool do_reinit;
{
	/*
	 * if there is a current record, and that record is not the same
	 * as the one we are making current, then either close it or make
	 * it non-current, depending on its lifetime.
	 * (call ar_ctl with CTSETLIFE to determine whether
	 * the current object's "lifetime" has changed) 
	 * NOTE: lifetime can not change for initial objects
	 */
	if (AR_cur && AR_cur != ap) {
		if (AR_cur->lifetime != AR_INITIAL)
			(void) ar_ctl(AR_cur, CTSETLIFE, 0);
		if (AR_cur->lifetime == AR_SHORTERM) {
			mess_lock();		/* don't ask ... */
			(void) ar_close(AR_cur, FALSE);
			mess_unlock();
		}
		(void) arf_noncur(AR_cur, AR_cur);
		mess_frame("");			/* clear frame message */
	}
	if (do_reinit == TRUE)			/* abs k15 */
	    (void) arf_reinit(ap, ap);
}

/*
 * back end for ar_current and ar_backup
 * calls current function and sets slks
 * "ap" is actrec to make current if it fails
 */
static void
ar_cur_end(ap)
register struct actrec	*ap;
{
    char i18n_msg[I18NMAX];

    ar_setslks(AR_cur->slks, AR_cur->flags & AR_ALTSLKS);
    if (arf_current(AR_cur, AR_cur) == FAIL)
    {
	if (AR_cur->lifetime == AR_INITIAL && AR_cur->nextrec == AR_cur) {

            sprintf(i18n_msg, gettxt(":197","can't make only %s current"), "actrec");
	    fatal(MUNGED, i18n_msg);
        }
	else
	{
            sprintf(i18n_msg, gettxt(":198","can't make %s current"), "actrec");
	    error(MUNGED, i18n_msg);
	    AR_cur = ap;
	    ar_setslks(AR_cur->slks, AR_cur->flags & AR_ALTSLKS);
	}
    }
    else
	(void)ar_ctl(AR_cur, CTSETMSG, FALSE);


#ifdef _DEBUG5
    ar_dump("at end of ar_current");
#endif
}

struct actrec *
ar_backup()
{
	register struct actrec	*ap;
	register struct actrec	*old_AR_cur;

	ap = AR_cur->backup;
	ar_cur_front( ap , TRUE ); /* miked k17 */
	old_AR_cur = AR_cur;
	AR_cur = ap;
	ar_cur_end(old_AR_cur);
	return AR_cur;
}

struct actrec *
ar_current(rec, do_reinit)
register struct actrec *rec;
bool do_reinit;			/* abs k15 */
{
	register struct actrec	*ap;
	pid_t pid;		/* miked k17 */
  
	/* do not make rec current if it's closing. abs k17 */
	if (rec && rec->lifetime == AR_CLOSING && /* abs k17 */
	    ar_ctl(rec, CTGETPID, &pid) == FAIL )	 /* miked k17 */
	    return(AR_cur);			 /* abs k17 */
	
	ar_cur_front(rec, do_reinit); /* abs k15 */
	/*
	 * backup is a circularly linked list
	 */
	if (AR_cur == NULL)
		/* produce degenerate list */
		rec->backup = rec;
	else if (AR_cur != rec) {
		/* traverse entire list */
		for (ap = AR_cur; ap->backup != AR_cur; ap = ap->backup) {
			/*
			 * if rec is in list and is not where we want it
			 *	unlink it from list
			 */
			if (ap->backup == rec && rec->backup != AR_cur)
				ap->backup = ap->backup->backup;
		}
		/* if rec is not where we want it, link it in befre cur */
		if (ap != rec) {
			rec->backup = ap->backup;
			ap->backup = rec;
		}
	}
	AR_cur = rec;
	ar_cur_end(AR_cur->backup);
	return AR_cur;
}



/* a minimal version  of ar_current for use when the frame is 
 * made current only for internal purposes. for example, used 
 * when doing a reread from ar_checkworld.  The user doesn't
 * see a change in what frame is current but internally we
 * make the frame current so we can update it.
 */

int
ar_cur_temp(rec)
register struct actrec *rec;
{
    char i18n_msg[I18NMAX];
  
    /* do not make rec current if it's closing. abs k17 */
    if (rec && rec->lifetime == AR_CLOSING)  /* abs k17 */
	return(FAIL);			     /* abs k17 */
	
    if (AR_cur && AR_cur != rec)
	(void) arf_noncur(AR_cur, AR_cur);
    if (rec->backup == NULL)
	/* produce degenerate list */
	rec->backup = rec;
    AR_cur = rec;
    if (arf_temp_current(AR_cur, AR_cur) == FAIL)
    {
	if (AR_cur->lifetime == AR_INITIAL && AR_cur->nextrec == AR_cur) {
            sprintf(i18n_msg, gettxt(":199","can't make only %s current"), "actrec");
	    fatal(MUNGED, i18n_msg); /* exit fmli */
        }
	else
	{
            sprintf(i18n_msg, gettxt(":198","can't make %s current"), "actrec");
	    error(MUNGED, i18n_msg);
	    AR_cur = AR_cur->backup;
	    return FAIL;
	}
    }
    return SUCCESS;
}


ar_setslks(s, flags)
struct slk	*s;
int flags;
{
	setslks(s, flags);
	return SUCCESS;
}

token
actrec_stream(t)
token t;
{
	return arf_odsh(AR_cur, t);
}

/* find an activation record via its window number */

struct actrec *
wdw_to_ar(wdw)
int wdw;
{
	struct actrec *p;

	for (p = AR_head; p; p = p->nextrec)
		if (wdw == ar_ctl(p, CTGETWDW))
			return p;
		else if (p->nextrec == AR_head)
			return NULL;
	return NULL;
}

/* find an activation record via its path */

struct actrec *
path_to_ar(s)
char *s;
{
	struct actrec *p;

	for (p = AR_head; p; p = p->nextrec)
		if (p->path && strcmp(p->path, s) == 0)
			return p;
		else if (p->nextrec == AR_head)
			return NULL;
	return NULL;
}

bool
path_isopen(s, op, exact)
char *s, *op;
bool exact;	/* if TRUE, don't allow exact match */
{
	struct actrec *p;
	int len = strlen(s);

	for (p = AR_head; p; p = p->nextrec) {
		if (exact && p->path && strcmp(p->path, s) == 0) {

                        if (strcmp(op,"move") == 0)
                             (void)mess_err( gettxt(":200","Can't move an open object, close it first") );

                        else if (strcmp(op,"copy") == 0)
                             (void)mess_err( gettxt(":201","Can't copy an open object, close it first") );

                        else if (strcmp(op,"rename") == 0)
                             (void)mess_err( gettxt(":202","Can't rename an open object, close it first") );

                        else if (strcmp(op,"delete") == 0)
                             (void)mess_err( gettxt(":203","Can't delete an open object, close it first") );


			return(TRUE);
		} else if (p->path && strncmp(p->path, s, len) == 0 && p->path[len] == '/') {

                        if (strcmp(op,"move") == 0)
                             (void)mess_err( gettxt(":204","Can't move a folder with open sub-folders, close them first") );

                        else if (strcmp(op,"copy") == 0)
                             (void)mess_err( gettxt(":205","Can't copy a folder with open sub-folders, close them first") );

                        else if (strcmp(op,"rename") == 0)
                             (void)mess_err( gettxt(":206","Can't rename a folder with open sub-folders, close them first") );

                        else if (strcmp(op,"delete") == 0)
                             (void)mess_err( gettxt(":207","Can't delete a folder with open sub-folders, close them first") );

			return(TRUE);
		} else if (p->nextrec == AR_head)
			break;
	}
	return FALSE;
}

/* find the activation record past the argument given with a certain path */

struct actrec *
nextpath_to_ar(ar)
register struct actrec *ar;
{
	register struct actrec *p;

	for (p = ar->nextrec; p != ar; p = p->nextrec)
		if (p->path && ar->path && strcmp(p->path, ar->path) == 0)
			return(p);
	return(NULL);
}

struct actrec *
ar_cleanup(life)		/* clean up all records with lifetime <= life */
register int life;
{
    register struct actrec *p, *nextp;
    register bool logout;
    
    logout = (life == AR_INITIAL) ? TRUE : FALSE; /* are we exiting fmli? */
/*    p = AR_head   miked k17+  */;
    nextp = AR_head->nextrec;		/* miked k17+ */
    do
    {
/*	p = p->nextrec;   miked k17+  */
	p = nextp;			/* miked k17+ */
	nextp = p->nextrec;		/* miked k17+ */
	if (p->lifetime <= life && !(life < AR_IMMORTAL && p->flags & AR_SKIP))
	    (void) ar_close(p, logout);
    } while ((AR_head != NULL) && (p != AR_head));

    return(AR_cur);
}

ar_help(rec)	/* do help on current actrec */
struct actrec *rec;
{
	return(arf_help(rec, rec));
}

/* global to keep vt's from moving to the top when made current
 * (temporarily) by ar_checkworld. Also prevents temporary and frame
 * messages from being written by these frames. abs s16,W2
 */
bool CheckingWorld = FALSE;	/* abs s16,W2 */
#define CHECKING_CUR	(-1)	/* must be different from TRUE and FALSE
				 * CheckingWorld has this value when
				 * checking the current frame. abs s16,W2 */


void
ar_checkworld(force)
bool force;	/* if TRUE, forced check */
{
    struct actrec *rec, *start_rec, *sav_cur = AR_cur;
    static time_t last_check;      /* EFT abs k16 */
    static bool first_time = TRUE; /* abs k15 */
    extern long Mail_check;
    extern time_t Cur_time;        /* EFT abs k16 */

    if (force == FALSE &&
	(Cur_time <= last_check + Mail_check || first_time == TRUE)) /* abs k15 */
    {
	if (first_time == TRUE) 	/* abs s15 */
	{
	    last_check = Cur_time; 	/* abs s15 */
	    first_time = FALSE;		/* abs k15 */
	}
	return;
    }
    last_check = Cur_time;

    start_rec = AR_cur->backup->backup;  /* miked k17+ */
    if ( start_rec == AR_cur )
        start_rec = AR_cur->backup;

/*    for (rec = AR_head; rec; rec = rec->nextrec) miked k17+ */
    for (rec = start_rec; rec; rec = rec->backup)
    {
        CheckingWorld = (rec == sav_cur) ? CHECKING_CUR : TRUE; /* abs s16,W2 */
	if (ar_cur_temp(rec) == SUCCESS) /* abs k15 */
	    (void) arf_reinit(rec, rec);
/*	if (rec->nextrec == AR_head)  miked k17+ */
	if (rec->backup == start_rec)
	    break;
    }
    CheckingWorld = CHECKING_CUR;                               /* abs s16,W2 */
    (void) ar_cur_temp(sav_cur);
    (void) ar_ctl(AR_cur, CTSETMSG, FALSE);
    CheckingWorld = FALSE;                                      /* abs s16,W2 */
}

ar_isfirst(ar1, ar2)
struct actrec *ar1, *ar2;
{
	register struct actrec *p;

	for (p = AR_cur; p->backup != AR_cur; p = p->backup) {
		if (p == ar1)
			return(TRUE);
		else if (p == ar2)
			return(FALSE);
	}
	return(TRUE);	/* should not get this far */ 
}

struct actrec *
menline_to_ar(n)
int n;
{
	register int i;
	register struct actrec *p;

	for (p = AR_head, i = -1; p; p = p->nextrec) {
		/* records with NULL path fields are not listed */
		if (p->path) {
			if (++i == n)
				return(p);
		}
		if (p->nextrec == AR_head)
			break;
	}
	return NULL;
}

struct menu_line
ar_menudisp(n, ptr)
register int n;
register char *ptr;
{
	register struct actrec *p;
	struct menu_line m;

	m.description = NULL;
	m.flags = 0;
	if (p = menline_to_ar(n))
		(void) ar_ctl(p, CTGETITLE, &m.highlight);
	else
		m.highlight = NULL;
	return m;
}

int	 /* >>>>>> NONPORTABLE!!! change to "char *" <<<<<<< */
ar_ctl(rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6)
struct actrec *rec;
int cmd;
int arg1, arg2, arg3, arg4, arg5, arg6;
{
	if (rec == NULL)
		return(FAIL);
/*	if (cmd == CTSETLIFE && arg1 != 0)
	{
		rec->lifetime = arg1;
		return(SUCCESS);
	}
	else
		return((*(rec->fcntbl[AR_CTL]))
		     (rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
abs */
	switch (cmd)
	{
	case CTSETLIFE:
	    if (arg1 != 0)
	    {
		rec->lifetime = arg1;
		return(SUCCESS);
	    }
	    else
		return((*(rec->fcntbl[AR_CTL]))
		     (rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
	    break;
	case CTSETINTR:		/* >>> ADD FREE STR CODE <<< */
	    if ((char *)arg1 == NULL)
		rec->interrupt = (char *)strsave(init_ctl(CTGETINTR));
	    else
		rec->interrupt = (char *)strsave((char *)arg1);
	    return(SUCCESS);
	case CTGETINTR:
	    return((int)rec->interrupt);
	case CTSETONINTR: 	/* >>> ADD FREE STR CODE <<< */
	    if ((char *)arg1 == NULL)
		rec->oninterrupt = (char *)strsave(init_ctl(CTGETONINTR));
	    else
		rec->oninterrupt = (char *)strsave((char *)arg1);
	    return(SUCCESS);
	case CTGETONINTR:
	    return((int)rec->oninterrupt);
	default:
	    return((*(rec->fcntbl[AR_CTL]))
		   (rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
	}
}
    
#ifdef _DEBUG5

/* for debugging - print out activation record info */
void
ar_dump(s)
char	*s;
{
	struct actrec *p;

	_debug5(stderr, "---------- Activation Record Dump: %s ----------\n", s);
	_debug5(stderr, " #\tprev\tnext\tbackup\tpath\n");

	for (p = AR_head; p; p = p->nextrec) {
		_debug5(stderr, "%2d\t", p->serial);
		_debug5(stderr, "%2d\t", p->prevrec?p->prevrec->serial:-1);
		_debug5(stderr, "%2d\t", p->nextrec?p->nextrec->serial:-1);
		_debug5(stderr, "%2d\t", p->backup?p->backup->serial:-1);
		_debug5(stderr, "%5.5s", (p==AR_cur)?"CUR>>":"     ");
		_debug5(stderr, "%s\n", p->path);
		if (p->nextrec == AR_head)
			break;
	}
	_debug5(stderr, "Current = %s\n", AR_cur?AR_cur->path:"NULL");
	_debug5(stderr, "Head    = %s\n", AR_head?AR_head->path:"NULL");
	_debug5(stderr, "-------------**Dump End**------------------\n");
}
#endif
