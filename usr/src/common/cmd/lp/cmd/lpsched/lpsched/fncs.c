/*		copyright	"%c%" 	*/


#ident	"@(#)fncs.c	1.2"
#ident  "$Header$"

#include <string.h>
#include "lpsched.h"
#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

/**
 ** walk_ptable() - WALK PRINTER TABLE, RETURNING ACTIVE ENTRIES
 ** walk_ftable() - WALK FORMS TABLE, RETURNING ACTIVE ENTRIES
 ** walk_ctable() - WALK CLASS TABLE, RETURNING ACTIVE ENTRIES
 ** walk_pwtable() - WALK PRINT WHEEL TABLE, RETURNING ACTIVE ENTRIES
 **/


PSTATUS *
#ifdef	__STDC__
walk_ptable (
	int			start
)
#else
walk_ptable (start)
	int			start;
#endif
{
	DEFINE_FNNAME (walk_ptable)

	static PSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = PStatus;
		psend = PStatus + PT_Size;
	}

	while (ps < psend && !ps->printer->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

FSTATUS *
#ifdef	__STDC__
walk_ftable (
	int			start
)
#else
walk_ftable (start)
	int			start;
#endif
{
	DEFINE_FNNAME (walk_ftable)

	static FSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = FStatus;
		psend = FStatus + FT_Size;
	}

	while (ps < psend && !ps->form->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

CSTATUS *
#ifdef	__STDC__
walk_ctable (
	int			start
)
#else
walk_ctable (start)
	int			start;
#endif
{
	DEFINE_FNNAME (walk_ctable)

	static CSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = CStatus;
		psend = CStatus + CT_Size;
	}

	while (ps < psend && !ps->class->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

PWSTATUS *
#ifdef	__STDC__
walk_pwtable (
	int			start
)
#else
walk_pwtable (start)
	int			start;
#endif
{
	DEFINE_FNNAME (walk_pwtable)

	static PWSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = PWStatus;
		psend = PWStatus + PWT_Size;
	}

	while (ps < psend && !ps->pwheel->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

/**
 ** search_ptable() - SEARCH PRINTER TABLE
 ** search_ftable() - SEARCH FORMS TABLE
 ** search_ctable() - SEARCH CLASS TABLE
 ** search_pwtable() - SEARCH PRINT WHEEL TABLE
 **/

PSTATUS *
#ifdef	__STDC__
search_ptable (
	register char *		name
)
#else
search_ptable (name) 
	register char		*name; 
#endif
{ 
	DEFINE_FNNAME (search_ptable)

	register PSTATUS	*ps,
				*psend; 

	for ( 
		ps = & PStatus[0], psend = & PStatus[PT_Size]; 
		ps < psend && !SAME(ps->printer->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

FSTATUS *
#ifdef	__STDC__
search_ftable (
	register char *		name
)
#else
search_ftable (name) 
	register char		*name; 
#endif
{ 
	DEFINE_FNNAME (search_ftable)

	register FSTATUS	*ps,
				*psend; 

	for ( 
		ps = & FStatus[0], psend = & FStatus[FT_Size]; 
		ps < psend && !SAME(ps->form->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

CSTATUS *
#ifdef	__STDC__
search_ctable (
	register char *		name
)
#else
search_ctable (name) 
	register char		*name; 
#endif
{ 
	DEFINE_FNNAME (search_ctable)

	register CSTATUS	*ps,
				*psend; 

	for ( 
		ps = & CStatus[0], psend = & CStatus[CT_Size]; 
		ps < psend && !SAME(ps->class->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

PWSTATUS *
#ifdef	__STDC__
search_pwtable (
	register char *		name
)
#else
search_pwtable (name) 
	register char		*name; 
#endif
{ 
	DEFINE_FNNAME (search_pwtable)

	register PWSTATUS	*ps,
				*psend; 

	for ( 
		ps = & PWStatus[0], psend = & PWStatus[PWT_Size]; 
		ps < psend && !SAME(ps->pwheel->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

SSTATUS *
#ifdef	__STDC__
search_stable (
	char *			name
)
#else
search_stable (name)
	char			*name;
#endif
{
	DEFINE_FNNAME (search_stable)

	int			i;

	if (!SStatus)
		return (0);
    
	for (
		i = 0;
		SStatus[i] && !SAME(SStatus[i]->system->name, name);
		i++
	)
		;

	return (SStatus[i]);
}
SSTATUS *
#ifdef	__STDC__
default_system (
	short 			protocol
)
#else
default_system (protocol)
	short			protocol;
#endif
{
	DEFINE_FNNAME (default_system)

	int i;

	if (!SStatus)
		return (0);
    
	for (i = 0; SStatus[i]; i++)
	{
		if ((SStatus[i]->system->name[0] == '*' ||
			SStatus[i]->system->name[0] == '+') &&
			SStatus[i]->system->name[1] == NULL &&
			SStatus[i]->system->protocol == protocol)
			return(SStatus[i]);
	}

	return (NULL);
}

/**
 ** load_str() - LOAD STRING WHERE ALLOC'D STRING MAY BE
 ** unload_str() - REMOVE POSSIBLE ALLOC'D STRING
 **/

void
#ifdef	__STDC__
load_str (
	char **			pdst,
	char *			src
)
#else
load_str (pdst, src)
	char			**pdst,
				*src;
#endif
{
	DEFINE_FNNAME (load_str)

	if (*pdst)
		Free (*pdst);
	*pdst = Strdup(src);
	return;
}

void
#ifdef	__STDC__
unload_str (
	char **			pdst
)
#else
unload_str (pdst)
	char			**pdst;
#endif
{
	DEFINE_FNNAME (unload_str)

	if (*pdst)
		Free (*pdst);
	*pdst = 0;
	return;
}

/**
 ** unload_list() - REMOVE POSSIBLE ALLOC'D LIST
 **/

void
#ifdef	__STDC__
unload_list (
	char ***		plist
)
#else
unload_list (plist)
	char			***plist;
#endif
{
	DEFINE_FNNAME (unload_list)

	if (*plist)
		freelist (*plist);
	*plist = 0;
	return;
}

/**
 ** load_sdn() - LOAD STRING WITH ASCII VERSION OF SCALED DECIMAL NUMBER
 **/

void
#ifdef	__STDC__
load_sdn (
	char **			p,
	SCALED			sdn
)
#else
load_sdn (p, sdn)
	char			**p;
	SCALED			sdn;
#endif
{
	DEFINE_FNNAME (load_sdn)

	if (!p)
		return;

	if (*p)
		Free (*p);
	*p = 0;

	if (sdn.val <= 0 || 999999 < sdn.val)
		return;

	*p = Malloc(sizeof("999999.999x"));
	(void) sprintf (
		*p,
		"%.3f%s",
		sdn.val,
		(sdn.sc == 'c'? "c" : (sdn.sc == 'i'? "i" : ""))
	);

	return;
}

/*
 * Procedure:     Getform
 *
 * Restrictions:
 *               getform: None
 *
 * Notes - EASIER INTERFACE TO "getform()"
 */

_FORM *
#ifdef	__STDC__
Getform (
	char *			form
)
#else
Getform (form)
	char			*form;
#endif
{
	DEFINE_FNNAME (Getform)

	static _FORM		_formbuf;

	FORM			formbuf;

	FALERT			alertbuf;

	int			ret;


	while (
		(ret = getform(form, &formbuf, &alertbuf, (FILE **)0)) == -1
	     && errno == EINTR
	)
		;
	if (ret == -1)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		case ENODATA:
			if (!STREQU (NAME_ALL, form))
				break;
		default:
			lpnote (WARNING, E_SCH_GETFRMFAIL, form);
			break;
		}

		errno = save;
		return (0);
	}

	_formbuf.plen = formbuf.plen;
	_formbuf.pwid = formbuf.pwid;
	_formbuf.lpi = formbuf.lpi;
	_formbuf.cpi = formbuf.cpi;
	_formbuf.np = formbuf.np;
	_formbuf.chset = formbuf.chset;
	_formbuf.mandatory = formbuf.mandatory;
	_formbuf.rcolor = formbuf.rcolor;
	_formbuf.comment = formbuf.comment;
	_formbuf.conttype = formbuf.conttype;
	_formbuf.name = formbuf.name;

	if ((_formbuf.alert.shcmd = alertbuf.shcmd)) {
		_formbuf.alert.Q = alertbuf.Q;
		_formbuf.alert.W = alertbuf.W;
	} else {
		_formbuf.alert.Q = 0;
		_formbuf.alert.W = 0;
	}

	return (&_formbuf);
}

/*
 * Procedure:     Getprinter
 *
 * Restrictions:
 *               getprinter: None
 *               stat(2): None
 *               lvlfile(2): None
 *               devstat(2): None
*/

/**
 ** Getprinter()
 ** Getrequest()
 ** Getuser()
 ** Getclass()
 ** Getpwheel()
 ** Getsecure()
 ** Getsystem()
 ** Loadfilters()
 **/

#ifdef	__STDC__
PRINTER *
Getprinter (char *name)
#else
PRINTER *
Getprinter (name)

char *	name;
#endif
{
	struct	 stat		statbuf;
	struct   devstat	devbuf;
	register PRINTER	*pp;

	DEFINE_FNNAME (Getprinter)

	ENTRYP
	while (!(pp = getprinter(name)) && errno == EINTR)
		;

	if (!pp)
		goto LOGERR;
	/*
	**  Could be a remote printer.
	**  For remote printers, hilevel and lolevel are
	**  stored as part of the printer configuration
	**  and device is null.  So, there is no need to
	**  go any further.
	*/
	if (!pp->device)
	{
		EXITP
		return	pp;
	}
	TRACEs (pp->device)
	if (stat (pp->device, &statbuf) < 0)
	{
		errno = EBADF;
		goto LOGERR;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFREG ||
	    (statbuf.st_mode & S_IFMT) == S_IFIFO)
	{
		if (lvlfile (pp->device, MAC_GET, &(pp->hilevel)) < 0)
		{
			errno = EBADF;
			goto LOGERR;
		}
		pp->lolevel = pp->hilevel;
		TRACE (pp->hilevel)
		EXITP
		return	pp;
	}
	/*
	**  If the 'device' is not a char special then
	**  consider it bogus.
	*/
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR)
	{
		errno = EBADF;
		goto LOGERR;
	}
	if (devstat (pp->device, DEV_GET, &devbuf) < 0)
	{
		if (errno != ENOPKG)
		{
			errno = EBADF;
			goto LOGERR;
		}
		pp->hilevel = PR_DEFAULT_HILEVEL;
		pp->lolevel = PR_DEFAULT_LOLEVEL;
	}
	else
	{
		/*
		**  We cannot work with PRIVATE or DYNAMIC, or devices
		**  that have other processes using them.
		*/
		if (devbuf.dev_state != DEV_PUBLIC ||
		    devbuf.dev_mode  != DEV_STATIC 
/*****
**		   || (devbuf.dev_usecount != 0 &&
**		    devbuf.dev_relflag  != DEV_SYSTEM)
** Its ok to load a printer thats in-use.  abs s20.1 
*****/
                   )
		{
			errno = EBADF;
			goto LOGERR;
		}
		/*
		**  DEV_SYSTEM devices have their hilevel and lolevel
		**  set to 0, so use the level on the file object as
		**  the lolevel.  Using the level for both hi and lo
		**  would restrict it to a single level.  Not the
		**  desired effect.
		**
		**  '/dev/null' is such a device.
		*/
		if (devbuf.dev_relflag == DEV_SYSTEM)
		{
			if (lvlfile (pp->device, MAC_GET, &(pp->lolevel)) < 0)
			{
				errno = EBADF;
				goto LOGERR;
			}
			pp->hilevel = PR_DEFAULT_HILEVEL;
		}
		else
		{
			pp->hilevel = devbuf.dev_hilevel;
			pp->lolevel = devbuf.dev_lolevel;
		}
	}
	TRACEd (devbuf.dev_hilevel)
	TRACEd (devbuf.dev_lolevel)
	TRACEd (pp->hilevel)
	TRACEd (pp->lolevel)
	EXITP
	return	pp;
LOGERR:
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		case ENODATA:
			if (!STREQU (NAME_ALL, name))
				break;
		default:
        		lpnote (WARNING, E_SCH_GETPRTFAIL, name);
			break;
		}
		
		errno = save;
	}
	return 0;
}

/*
 * Procedure:     Getrequest
 *
 * Restrictions:
 *               getrequest: None
*/

REQUEST *
#ifdef	__STDC__
Getrequest (
	char *			file
)
#else
Getrequest (file)
	char			*file;
#endif
{
	DEFINE_FNNAME (Getrequest)

	register REQUEST	*ret;

	while (!(ret = getrequest(file)) && errno == EINTR)
		;
		
	if (!ret)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		default:
			lpnote (WARNING, E_SCH_GETREQFAIL, file);
			break;
		}

		errno = save;
	}
	return (ret);
}

/*
 * Procedure:     Getuser
 *
 * Restrictions:
 *               getuser: None
*/

USER *
#ifdef	__STDC__
Getuser (
	char *			name
)
#else
Getuser (name)
	char			*name;
#endif
{
	DEFINE_FNNAME (Getuser)

	register USER		*ret;

	while (!(ret = getuser(name)) && errno == EINTR)
		;
	if (!ret)
		lpnote (WARNING, E_SCH_GETUSRFAIL, name);
	return (ret);
}

/*
 * Procedure:     Getclass
 *
 * Restrictions:
 *               getclass: None
*/

CLASS *
#ifdef	__STDC__
Getclass (
	char *			name
)
#else
Getclass (name)
	char			*name;
#endif
{
	DEFINE_FNNAME (Getclass)

	register CLASS		*ret;

	while (!(ret = getclass(name)) && errno == EINTR)
		;
	
	if (!ret)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		case ENODATA:
			if (!STREQU (NAME_ALL, name))
				break;
		default:
			lpnote (WARNING, E_SCH_GETCLSFAIL, name);
			break;
		}

		errno = save;
	}
	return (ret);
}

/*
 * Procedure:     Getpwheel
 *
 * Restrictions:
 *               getpwheel: None
*/

PWHEEL *
#ifdef	__STDC__
Getpwheel (
	char *			name
)
#else
Getpwheel (name)
	char			*name;
#endif
{
	DEFINE_FNNAME (Getpwheel)

	register PWHEEL		*ret;

	while (!(ret = getpwheel(name)) && errno == EINTR)
		;
	
	if (!ret)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		case ENODATA:
			if (!STREQU (NAME_ALL, name))
				break;
		default:
			lpnote (WARNING, E_SCH_GETPWHFAIL, name);
			break;
		}

		errno = save;
	}
	return (ret);
}

/*
 * Procedure:     Getsecure
 *
 * Restrictions:
 *               getsecure: None
*/

SECURE *
#ifdef	__STDC__
Getsecure (
	char *			file
)
#else
Getsecure (file)
	char			*file;
#endif
{
	DEFINE_FNNAME (Getsecure)

	register SECURE		*ret;

	while (!(ret = getsecure(file)) && errno == EINTR)
		;
	
	if (!ret)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		default:
			lpnote (WARNING, E_SCH_GETSECFAIL, file);
			break;
		}

		errno = save;
	}
	return (ret);
}
/*
 * Procedure:     Getsystem
 *
 * Restrictions:
 *               getsystem: None
*/

SYSTEM *
#ifdef	__STDC__
Getsystem (
	char *			file
)
#else
Getsystem (file)
	char			*file;
#endif
{
	DEFINE_FNNAME (Getsystem)

	SYSTEM		*ret;

	while (!(ret = getsystem(file)) && errno == EINTR)
		;
	
	if (!ret)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		default:
			lpnote (WARNING, E_SCH_GETSYSFAIL, file);
			break;
		}

		errno = save;
	}
	return (ret);
}

/*
 * Procedure:     Loadfilters
 *
 * Restrictions:
 *               loadfilters: None
*/
int
#ifdef	__STDC__
Loadfilters (
	char *			file
)
#else
Loadfilters (file)
	char			*file;
#endif
{
	DEFINE_FNNAME (Loadfilters)

	register int		ret;

	while ((ret = loadfilters(file)) == -1 && errno == EINTR)
		;
	
	if (ret == -1)
	{
		int save = errno;

		switch (errno)
		{
		case ENOENT:
			break;
		default:
			lpnote (WARNING, E_SCH_GETFLTRFAIL, file);
			break;
		}

		errno = save;
	}
	return (ret);
}

/**
 ** free_form() - FREE MEMORY ALLOCATED FOR _FORM STRUCTURE
 **/

void
#ifdef	__STDC__
free_form (
	register _FORM *	pf
)
#else
free_form (pf)
	register _FORM		*pf;
#endif
{
	DEFINE_FNNAME (free_form)

	if (!pf)
		return;
	if (pf->chset)
		Free (pf->chset);
	if (pf->rcolor)
		Free (pf->rcolor);
	if (pf->comment)
		Free (pf->comment);
	if (pf->conttype)
		Free (pf->conttype);
	if (pf->name)
		Free (pf->name);
	pf->name = 0;
	if (pf->alert.shcmd)
		Free (pf->alert.shcmd);
	return;
}

/**
 ** getreqno() - GET NUMBER PART OF REQUEST ID
 **/

char *
#ifdef	__STDC__
getreqno (
	char *			req_id
)
#else
getreqno (req_id)
	char			*req_id;
#endif
{
	DEFINE_FNNAME (getreqno)

	register char		*cp;

	if (!(cp = strrchr(req_id, '-')))
		cp = req_id;
	else
		cp++;
	return (cp);
}
#ifdef	__STDC__
void
auto_putsystem (char *sysnamep, short protocol)
#else
void
auto_putsystem (sysnamep, protocol)
char	*sysnamep;
short	protocol;
#endif
{
	DEFINE_FNNAME (auto_putsystem)

	SYSTEM	*systemp,sysbuf;
	static	SYSTEM	DefaultSystem =	{ NULL, NULL, NULL, S5_PROTO, NULL,
					  DEFAULT_TIMEOUT, DEFAULT_RETRY,
					  NULL, NULL, NULL
					}; 
	if (! sysnamep || ! *sysnamep) {
		return ;
	}
	if (systemp = getsystem (sysnamep)) {
		sysbuf = *systemp;
	}
	else {
		sysbuf = DefaultSystem;
		sysbuf.name = sysnamep;
	}
	if (protocol) {
		if (	(protocol != S5_PROTO) &&
			(protocol != BSD_PROTO) &&
			(protocol != NUC_PROTO)	)
				return ;
		else
			sysbuf.protocol = protocol;
	}
	if (putsystem (sysnamep, &sysbuf)) {
                schedlog("Could not add %s to systems file\n", sysnamep);
		return ;
	}
	return ;
}

