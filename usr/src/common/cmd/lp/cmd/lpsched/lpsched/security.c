/*		copyright	"%c%" 	*/


#ident	"@(#)security.c	1.3"
#ident  "$Header$"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/secsys.h>
#include <mac.h>
#include <audit.h>
#include <pwd.h>
#include "lpsched.h"

#ifdef	DEBUG
#include <priv.h>
#endif

int
#if	__STDC__
SecAdvise (SECURE *sp, int op, MESG *mdp)
#else
SecAdvise (sp, op, mdp)

SECURE	*sp;
int	op;
MESG	*mdp;
#endif
{
	int		n;
	struct obj_attr	obj;

	DEFINE_FNNAME (SecAdvise)

	ENTRYP

#ifdef	NETWORKING
	if (strchr (sp->user, '!'))
	{
		/*
		**  The job came from a remote system.
		**  User 'lp' is the local owner for 
		**  the remote user.
		*/
		obj.uid = Lp_Uid;
		obj.gid = Lp_Gid;
		obj.lid = sp->lid;
		obj.mode = 0755;
	}
	else
	{
#endif
		obj.uid = sp->uid;
		obj.gid = sp->gid;
		obj.lid = sp->lid;
		obj.mode = 0755;
#ifdef	NETWORKING
	}
#endif
	TRACEd (obj.uid)
	TRACEd (obj.gid)
	TRACEd (obj.lid)
	TRACEd (obj.mode)

	TRACE (op)

	TRACEd (mdp->uid)
	TRACEd (mdp->gid)
	TRACEd (mdp->lid)
	TRACE  (mdp->credp)

#ifdef	DEBUG
	if (! mdp->credp)
	{
		TRACEP ("NULL credentials!!!")
	}
#endif
	while ((n = secadvise (&obj, op, mdp->credp)) < 0 && errno == EINTR);

	TRACEd (n)
	TRACEd (errno)

	if (n < 0 && errno == EACCES)
	{
		/*
		**  If we are in an ID based privilege mechanism then
		**  uid 'lp' is as privileged as the system defined
		**  privileged uid, for this sub-system.
		*/
		if (secsys (ES_PRVID, 0) >= 0 && mdp->uid == Lp_Uid)
		{
			EXITP
			return	1;
		}
	}
	EXITP
	return	n < 0 ? 0 : 1;
}

/*
 * Procedure:     ValidateAdminUser
 *
 * Restrictions:
 *               stat(2): None
 *               lvlfile(2): None
 *  Notes:
 *	0 == no
 *	1 == yes
*/
#ifdef	__STDC__
int
ValidateAdminUser (MESG *mdp)
#else
int
ValidateAdminUser (mdp)

MESG	*mdp;
#endif
{
	DEFINE_FNNAME (ValidateAdminUser)

	int		n;
	struct obj_attr	obj;
	struct stat	statbuf;

	if (stat (ADMINISTRATOR_OBJECT_PATH, &statbuf) < 0)
	{
		return	0;
	}
/*
	if (lvlfile (ADMINISTRATOR_OBJECT_PATH, MAC_GET, &(obj.lid)) < 0)
	{
		return	0;
	}
*/
	/*
	**  MR#:  ul91-26114a1
	**
	**  Fill in most of the 'obj' struct with the attributes from
	**  the stat struct.  The exception is the mode of the object.
	**  This we force to be execute only by the owner and group.
	**  Then we use secadvise to test for read access to the object.
	**  The intent here is to force the user to have DAC_READ
	**  in their privilege set.
	*/
	obj.uid = statbuf.st_uid;
	obj.gid = statbuf.st_gid;
	obj.lid = statbuf.st_level;
	obj.mode = 0110;		

	while ((n = secadvise (&obj, SA_READ, mdp->credp)) < 0
		&& errno == EINTR);

	if (n < 0 && errno == EACCES)
	{
		/*
		**  If we are in an ID based privilege mechanism then
		**  uid 'lp' is as privileged as the system defined
		**  privileged uid, for this sub-system.
		*/
		if (secsys (ES_PRVID, 0) >= 0 && mdp->uid == Lp_Uid)
		{
			EXITP
			return	1;
		}
	}
	return	n < 0 ? 0 : 1;
}

/*
 * Procedure:     ValidateEnableUser
 *
 * Restrictions:
 *               stat(2): None
 *               lvlfile(2): None
*/
#ifdef	__STDC__
int
ValidateEnableUser (MESG *mdp)
#else
int
ValidateEnableUser (mdp)

MESG	*mdp;
#endif
{
	DEFINE_FNNAME (ValidateEnableUser)

	int		n;
	struct obj_attr	obj;
	struct stat	statbuf;

	if (stat (ENABLE_OBJECT_PATH, &statbuf) < 0)
	{
		return	0;
	}
	if (lvlfile (ENABLE_OBJECT_PATH, MAC_GET, &(obj.lid)) < 0)
	{
		return	0;
	}
	obj.uid = statbuf.st_uid;
	obj.gid = statbuf.st_gid;
	obj.mode = statbuf.st_mode;

	while ((n = secadvise (&obj, SA_EXEC, mdp->credp)) < 0
		&& errno == EINTR);

	if (n < 0 && errno == EACCES)
	{
		/*
		**  If we are in an ID based privilege mechanism then
		**  uid 'lp' is as privileged as the system defined
		**  privileged uid, for this sub-system.
		*/
		if (secsys (ES_PRVID, 0) >= 0 && mdp->uid == Lp_Uid)
		{
			EXITP
			return	1;
		}
	}
	return	n < 0 ? 0 : 1;
}

int
#ifdef	__STDC__
ValidatePrinterUser (PRINTER *pp, MESG *mdp)
#else
ValidatePrinterUser (pp, mdp)

PRINTER	*pp;
MESG	*mdp;
#endif
{
	int	n;

	DEFINE_FNNAME (ValidatePrinterUser)

	while ((n = lvldom (&pp->hilevel, &mdp->lid)) < 0 && errno == EINTR);

	if (n <= 0)
		if (errno == ENOPKG)
			return	1;
		else
			return	0;

	while ((n = lvldom (&mdp->lid, &pp->lolevel)) < 0 && errno == EINTR);

	if (n <= 0)
		if (errno == ENOPKG)
			return	1;
		else
			return	0;

	return	1;
}

int
#ifdef	__STDC__
ValidatePrinter (PRINTER *pp, SECURE *sp)
#else
ValidatePrinter (pp, sp)

PRINTER	*pp;
SECURE	*sp;
#endif
{
	int	n;

	DEFINE_FNNAME (ValidatePrinter)

	while ((n = lvldom (&pp->hilevel, &sp->lid)) < 0 && errno == EINTR);

	if (n <= 0)
		if (errno == ENOPKG)
			return	1;
		else
			return	0;


	while ((n = lvldom (&sp->lid, &pp->lolevel)) < 0 && errno == EINTR);

	if (n <= 0)
		if (errno == ENOPKG)
			return	1;
		else
			return	0;

	return	1;
}

int
#ifdef	__STDC__
ValidateAnyPrinter (SECURE *sp)
#else
ValidateAnyPrinter (sp)

SECURE	*sp;
#endif
{
	DEFINE_FNNAME (ValidateClass)
	register
	PSTATUS	*psp;

	ENTRYP
	for (psp = walk_ptable(1); psp; psp = walk_ptable(0))
	{
		if (ValidatePrinter (psp->printer, sp))
			break;
	}
	EXITP
	return	psp ? 1 : 0;
}

int
#ifdef	__STDC__
ValidateClass (CLASS *cp, SECURE *sp)
#else
ValidateClass (cp, sp)

CLASS	*cp;
SECURE	*sp;
#endif
{
	DEFINE_FNNAME (ValidateClass)
	char	**mp;
	register
	PSTATUS	*psp;

	ENTRYP
	for (mp = cp->members; *mp; mp++)
	{
		for (psp = walk_ptable(1); psp; psp = walk_ptable(0))
			if (psp->printer->name &&
			    !strcmp (psp->printer->name, *mp))
				break;

		if (psp && ValidatePrinter (psp->printer, sp))
			break;
	}
	EXITP
	return	*mp ? 1 : 0;
}

static	char *	SlowFilterp		= "SF";
static	char *  PrinterInterfacep	= "PI";

/*
 * Procedure:     CutStartJobAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/
#ifdef	__STDC__
void
CutStartJobAuditRec (int status, int type, char *userp, char *reqidp)
#else
void
CutStartJobAuditRec (status, type, userp, reqidp)

int	status;
int	type;
char *	userp;
char *	reqidp;
#endif
{
	char *	typep;
	char	adtbuf [128];

	static	char *	Startp = "S";

	switch (type) {
	case	1:
		typep = PrinterInterfacep;
		break;

	case	2:
		typep = SlowFilterp;
		break;

	default:
		typep = "??";
	}

	(void)	sprintf (adtbuf, "%s:%s:%s:%s", Startp,
		typep, userp, reqidp);

	CutAuditRec (ADT_PRT_JOB, status, strlen (adtbuf)+1, adtbuf);

	return;
}

/*
 * Procedure:     CutEndJobAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/
#ifdef	__STDC__
void
CutEndJobAuditRec (int status, int type, char *userp, char *reqidp)
#else
void
CutEndJobAuditRec (status, type, userp, reqidp)

int	status;
int	type;
char *	userp;
char *	reqidp;
#endif
{
	char *	typep;
	char	adtbuf [128];

	static	char *	Endp = "E";

	switch (type) {
	case	1:
		typep = PrinterInterfacep;
		break;

	case	2:
		typep = SlowFilterp;
		break;

	default:
		typep = "??";
	}
	(void)	sprintf (adtbuf, "%s:%s:%s:%s", Endp,
		typep, userp, reqidp);

	CutAuditRec (ADT_PRT_JOB, status, strlen (adtbuf)+1, adtbuf);

	return;
}

/*
 * Procedure:     CutCancelAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/

#ifdef	__STDC__
void
CutCancelAuditRec (int status, uid_t actor, char *reqidp, char *jobownerp)
#else
void
CutCancelAuditRec (status, actor, reqidp, jobownerp)

int	status;
uid_t	actor;
char *	reqidp;
char *	jobownerp;
#endif
{
	char *	namep;
	char	adtbuf [128];

	namep = lp_uidtoname (actor);

	(void)	sprintf (adtbuf, "%s:%s:%s", namep, reqidp, jobownerp);

	CutAuditRec (ADT_CANCEL_JOB, status, strlen (adtbuf)+1, adtbuf);

	Free (namep);

	return;
}

/*
 * Procedure:     CutAdminAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/

#ifdef	__STDC__
void
CutAdminAuditRec (int status, uid_t actor, char *actionp)
#else
void
CutAdminAuditRec (status, actor, actionp)

int	status;
uid_t	actor;
char *	actionp;
#endif
{
	char *	namep;
	char	adtbuf [128];

	namep = lp_uidtoname (actor);

	(void)	sprintf (adtbuf, "%s:%s", namep, actionp);

	CutAuditRec (ADT_LP_ADMIN, status, strlen (adtbuf)+1, adtbuf);

	Free (namep);

	return;
}

/*
 * Procedure:     CutMiscAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/
void
#ifdef	__STDC__
CutMiscAuditRec (int status, char *actorp, char *actionp)
#else
CutMiscAuditRec (status, actorp, actionp)

int	status;
char	*actorp;
char *	actionp;
#endif
{
	char	adtbuf [128];

	(void)	sprintf (adtbuf, "%s:%s", actorp, actionp);

	CutAuditRec (ADT_LP_MISC, status, strlen (adtbuf)+1, adtbuf);

	return;
}
/*
**  Map remote-user attributes into local attributes.
**
*/
#ifdef	__STDC__
int
NormalizeSecureUserAttributes (SECURE *sp)
#else
int
NormalizeSecureUserAttributes (sp)

SECURE *sp;
#endif
#ifdef	NETWORKING
{
	DEFINE_FNNAME (NormalizeSecureUserAttributes)

	char *		cp;
	char		outbuf [128],
			inbuf [128];
	level_t		lid;
	struct passwd *	pwp;

	static	level_t	DefaultUserLID = (level_t) -1;


	ENTRYP
	/*
	**  If lvlproc() fails w/ ENOPKG then MAC is not
	**  installed.  So, map the secure LID to something
	**  reasonable and return.
	*/
	if (lvlproc (MAC_GET, &lid) < 0 && errno == ENOPKG)
	{
		TRACEP ("MAC is not installed")
		sp->lid = DEFAULT_SECURE_LID;
		return	1;
	}
	if (! sp)
	{
		EXITP
		return	0;
	}
	TRACEs (sp->user)
	TRACEs (sp->system)
	TRACEd (sp->uid)
	TRACEd (sp->gid)
	TRACEd (sp->lid)

/*
	cp = strchr (sp->user, '!');
	if (! cp || ! *(++cp))
	{
		EXITP
		return	0;
	}
	TRACEs(cp)

	(void)	sprintf (outbuf, "%s@%s", cp, sp->system);
	TRACEs (outbuf)

	if (namemap (DEFAULT_SCHEME, outbuf, inbuf) < 0)
	{
		EXITP
		return	0;
	}
	TRACEs (inbuf)
	pwp = getpwnam (inbuf);
	endpwent ();
	if (! pwp)
	{
		EXITP
		return	0;
	}
	TRACEs (pwp->pw_name)
	TRACEd (pwp->pw_uid)
	TRACEd (pwp->pw_gid)
*/
	if (sp->lid != (level_t) -1)
	{
		(void)	sprintf (outbuf, "%d@%s", sp->lid, sp->system);

		TRACEs (outbuf)
		if (attrmap ("LID", outbuf, inbuf) < 0)
		{
			EXITP
			return	0;
		}
		TRACEs (inbuf)
		sp->lid = (level_t) atol (inbuf);
	}
	else
	{
		if (DefaultUserLID == (level_t) -1)
		{
			DefaultUserLID = DEFAULT_SECURE_LID;
		}
		sp->lid = DefaultUserLID;
	}
/*
	sp->uid = pwp->pw_uid;
	sp->gid = pwp->pw_gid;
*/

	TRACEd (sp->uid)
	TRACEd (sp->gid)
	TRACEd (sp->lid)

	EXITP
	return	1;
}
#else
{
	return	1;
}
#endif	/*  NETWORKING  */

#ifdef	DEBUG
static	char *	privlist [] =
{
	"owner",
	"audit",
	"compat",
	"dacread",
	"dacwrite",
	"dev",
	"filesys",
	"macread",
	"macwrite",
	"mount",
	"multidir",
	"setplevel",
	"setspriv",
	"setuid",
	"sysops",
	"setupriv",
	"driver",
	"rtime",
	"macupgrade",
	"fsysrange",
	"setflevel",
	"auditwr",
	"tshar",
	"plock",
	"core",
	"loadmod",
	"bind"
};
#ifdef	__STDC__
void
PrintProcPrivs (void)
#else
void
PrintProcPrivs ()
#endif	/*  __STDC__  */
{
	int	n, i;
	priv_t	privs [NPRIVS*2];
	char	fixprivs [256],
		inhprivs [256],
		maxprivs [256],
		wkgprivs [256];
	char *	bp	= (char *) 0;

	fixprivs [0] =
	inhprivs [0] =
	maxprivs [0] =
	wkgprivs [0] = '\0';

	(void) _OpenDebugFile ((char *)0);

	n = procpriv (GETPRV, privs, NPRIVS*2);
	if (n < 0)
		return;
	
	for (i=0; i < n; i++)
	{
		switch (privs [i] & PS_TYPE) {
		case	PS_FIX:
			bp = fixprivs;
			break;
		case	PS_INH:
			bp = inhprivs;
			break;
		case	PS_MAX:
			bp = maxprivs;
			break;
		case	PS_WKG:
			bp = wkgprivs;
			break;
		default:
			return;
		}
		if (*bp)
			(void)	strcat (bp, ",");

		(void)	strcat (bp, privlist [privs [i] & P_ALLPRIVS]);
	}
	(void) fprintf (_DebugFilep, "PrintProcPrivs:\n");
	(void) fprintf (_DebugFilep, "\tFixed:       %s\n", fixprivs);
	(void) fprintf (_DebugFilep, "\tInherited:   %s\n", inhprivs);
	(void) fprintf (_DebugFilep, "\tMaxset:      %s\n", maxprivs);
	(void) fprintf (_DebugFilep, "\tWorking-set: %s\n", wkgprivs);
	return;
}
#endif	/*  DEBUG  */
/*
 * Procedure:     DacWriteDevice
 *
 * Restrictions:
 *               stat(2): None
 *               
 *  Notes:
 *	0 == no
 *	1 == yes
*/
#ifdef	__STDC__
int
DacWriteDevice (PRINTER *pp, SECURE *sp)
#else
int
DacWriteDevice (pp, sp)

PRINTER	*pp;
SECURE	*sp;
#endif
{
	DEFINE_FNNAME (DacWriteDevice)

	struct	stat	statbuf;


	if (!(pp->device))
		return 1; /* A dial-up printer has no device */

	if (stat (pp->device, &statbuf) < 0) {
		return 1;
	}
	/*
	** Return YES since device will be opened
	** with DACWRITE and MACWRITE privileges
	** when the device is owned by LP
	*/
	if (statbuf.st_uid == Lp_Uid)
		return 1;

	if (statbuf.st_mode & S_IWOTH)
		return 1;

	if ((statbuf.st_uid == sp->uid) &&
	     statbuf.st_mode & S_IWUSR)
		return 1;

	if ((statbuf.st_gid == sp->gid) &&
	     statbuf.st_mode & S_IWGRP)
		return 1;

	return 0;
}
/*
 * Procedure:     IsMacInstalled
 *
 * Restrictions:
 *               lvlproc(2): None
 *               
 *  Notes:
 *	0 == no
 *	1 == yes
*/
#ifdef	__STDC__
int
IsMacInstalled (void)
#else
int
IsMacInstalled ()

#endif
{
	DEFINE_FNNAME (IsMacInstalled)

	level_t		lid;

	/*
	**  If lvlproc() fails w/ ENOPKG then MAC is not
	**  installed.  
	*/
	if (lvlproc (MAC_GET, &lid) < 0 && errno == ENOPKG)
	{
		TRACEP ("MAC is not installed")
		return	0;
	}
	return 1;
}
