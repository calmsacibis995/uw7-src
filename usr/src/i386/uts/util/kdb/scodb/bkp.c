#ident	"@(#)kern-i386:util/kdb/scodb/bkp.c	1.1"
#ident  "$Header$":w

/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
*	all of the code that uses and manipulates breakpoints
*	is in here.
*/

/*
 * Modification History:
 *
 *	L000	scol!nadeem	12may92
 *	- added support for BTLD.  Use the sent_name() function to access
 *	  the name field of a symbol table entry rather than accessing
 *	  it directly (see sym.c).
 *	- ensure in findu() that fixed length strings are null terminated
 *	  before being printed out.  The reason for this is that findu()
 *	  is now being used to scan the old symbol table format (see sym.c)
 *	  where the symbol name strings are not necessarily null terminated.
 *	L001	scol!nadeem	19may92
 *	- call broadcast() instead of scodb_broadcast().  Remove
 *	  scodb_broadcast() from install.d/scodb_mpx.i.
 *	L002	scol!nadeem	11jun92
 *	- parse breakpoint names (ie: of the form "#name") correctly.
 *	  Previously a breakpoint name would be searched for in the symbol
 *	  table and then in the breakpoint table.  This has been fixed so
 *	  that breakpoint names are only searched for in the breakpoint table.
 *	- created new functions bplookup_hashname() and bplookup_debugreg() from
 *	  code in bplookup().  Changes also in val.c.
 *	- fix bug in bp_lookup() which scans the breakpoint table without
 *	  ignoring unset breakpoints.
 *	L003	scol!nadeem	12jun92
 *	- fixed bug whereby once a debug register breakpoint was set and then
 *	  cleared, the next time that a debug breakpoint was set scodb thought
 *	  the original breakpoint was still in place.
 *	- use BKP_CLEARD() instead of BKP_CLEAR() to clear debug register
 *	  breakpoints.
 *	- make the breakpoint list command display debug breakpoints in the
 *	  same output format as ordinary breakpoints, and in particular display
 *	  whether the breakpoint is disabled or not.
 *	L004	scol!nadeem	26jun92
 *	- ensure that we don't panic when listing breakpoints which don't have
 *	  symbols.
 *	L005	scol!nadeem	9jul92
 *	- fix bug whereby doing a "bp mod DR0" command with DR0 previously
 *	  setup causes a panic.
 *	L006	scol!nadeem	21jul92
 *	- fix bug whereby a panic would occur when exiting the debugger after
 *	  clearing a breakpoint, if several cpus have hit the breakpoint.
 *	  This was introduced by L003 - ensure that "bp" is not null before
 *	  trying to dereference it!
 *	L007	scol!nadeem	21jun93
 *	- added functionality for "value" breakpoints.  This is an extension
 *	  to data breakpoints whereby the debugger is entered when the contents
 *	  of the specified breakpoint location matches a certain condition.
 *	  They are specified on the command line as follows:
 *
 *		bp <data specifiers> <address> <condition> <value>
 *
 *	  For example, "bp wl &lbolt >= 1000".
 *
 *	L008	scol!nadeem & scol!blf	6aug93
 *	- fix bug with Value breakpoints.  A debug read/write breakpoint with a
 *	  conditional value (ie: "bp rl ... != 0") causes the debug register
 *	  to fire a second time as the debugger reads the value from memory.
 *	  This causes a recursive loop ending in a kernel panic.  So
 *	  temporarily turn off the debug registers while reading memory.
 *	L009	scol!michaeld		13dec93
 *	- fix kernel text corruption when ('e')ntering a function from
 *	  single-step.  Setup the data byte in sfkbp before marking it as SET.
 *	  (or else the bp_unsetspec in scodb when 'e' is entered will write
 *	  back uninitiated data).  Note, this only happens if you start single
 *	  stepping when on a call instruction.
 *	L010	nadeem@sco.com		5apr94
 *	- remove the confirmations when doing global breakpoint operations
 *	  ("bc *", "bp dis *" and "bp en *"),
 *	- get rid of the "exact" breakpoint concept.  All breakpoints are now
 *	  placed exactly where requested and not at address+3.
 *	- do not prompt for "Breakpoint name:" when setting a breakpoint which
 *	  is not the start of a function.  The functionality still exists, but
 *	  has to be specifically requested using the new "name" option to
 *	  "bp".  For example, "bp name &read+6" will set a breakpoint at
 *	  &read+6 and ask for the name.  If the breakpoint already exists,
 *	  then the command will just assign the new name to the breakpoint.
 *	L011	nadeem@sco.com	13dec94
 *	- added support for user level scodb.  Move ufind() routine from
 *	  here to sym.c.  This is because bkp.c is not compiled into the user
 *	  level binary, but ufind() is needed.  Move do_all() to io.c
 *	  for the same reason.
 */

#include	"sys/types.h"
#include	"sys/reg.h"
#include	"sys/tss.h"
#include	"sys/seg.h"
#include	"sys/user.h"
#include	"sys/debugreg.h"
#include	"stunv.h"
#include	"dbg.h"
#include	"sent.h"
#include	"dis.h"
#include	"bkp.h"
#include	"histedit.h"


#define		MXNDBP	4	/* can't change */

#define		BPINST	0xCC	/* INT3 */

#define		DBP_EXEC	0
#define		DBP_WRITE	1
#define		DBP_READ	2
#define		DBP_SEXEC	4	/* special exec - use DRn */
#define		DBP_NEXEC	8	/* normal exec - use bkp */
#define		DBP_SIO		16	/* I/O breakpoint (Pentium) - L012 */
#define		DBP_IO		2	/* I/O breakpoint bit in DR7 - L012 */

#define		S_LRW		16
#define		S_R(n)		(S_LRW + (n)*4)

#define		EXACT_L		0x0100
#define		EXACT_G		0x0200
#define		EXACT		(EXACT_L|EXACT_G)

#define		L		0x1
#define		G		0x2

#define		SDBR(reg, rdwr, len)				\
			(((((len-1) << 2) | (rdwr)) << S_R(reg))\
			| EXACT |				\
			((G|L) << ((reg)*2)))

#define		CDBR(dr7, reg) ((dr7) &= ~((G|L) << ((reg)*2)))

extern int ICE;		/* if (ICE) then don't sdbr()! */
extern struct bkp scodb_bkp[];
extern int scodb_mxnbp;

STATIC struct bkp sfbkp;	/* breakpoint for step-over-functions */
NOTSTATIC struct dbkp dbkp[MXNDBP];
STATIC union dbr7 dr7;

STATIC char *e_unkbp = "%s: unknown breakpoint.\n";
STATIC char *e_cantall = "can't use * in the way specified.\n";	/* L010 */

extern int *REGP;
STATIC int (*pbpf)() = 0;
struct dbcmd *getcmd();

extern struct ilin scodb_bibufs[];
extern int scodb_nbibufs;
NOTSTATIC struct ilin *bib_freelist;
NOTSTATIC int bib_nfreelist;

/*
 * Command line conditions that can be specified for a value breakpoint.
 * The ordering of these strings must match the COND defines in bkp.h.
 */

STATIC char *bp_cond[] = {					/* L007v */
	"always", "!=", "==", ">=", "<=", "&", NULL
};

extern int processor_index, max_ACPUs;				/* L007^ */
extern char cpu_family;						/* L012 */

NOTSTATIC
struct ilin *
bib_alloc() {
	register struct ilin *il;

	if (il = bib_freelist)
		bib_freelist = il->il_next;
	il->il_narg = 0;
	--bib_nfreelist;
	return il;
}

NOTSTATIC
bib_free(il)
	struct ilin *il;
{
	il->il_next = bib_freelist;
	bib_freelist = il;
	++bib_nfreelist;
}

/*
*	breakpoint set command
*
	bp [enable|disable|mod|name|[rw][b|s|l]|x] address
*		address must be last argument
*/
NOTSTATIC
c_bp(c, v)
	int c;
	char **v;
{
	char *lasta, *nm, *symname();
	int i, dlen = 0, rwx = 0, drn, all = 0, name = 0;	/* L010 */
	int modify = 0, disable = 0, enable = 0, create = 0;	/* L002 L010 */
	register char *s;
	long seg, off, xoff;
	struct sent *sy = NULL, *findsym();
	char *nomore = "No more command lines available for breakpoints.\n";
	register struct bkp *bp, *fbp = NULL;
	register struct dbkp *dbp, *fdbp = NULL;
	struct scodb_list list;
	struct ilin *il, **buflp;
	unsigned linear, lastc;					/* L007 */

	if (c == 3) {	/* may be "modify"/"disable"/"enable" */
		struct bkp *rbp;
		struct dbkp *rdbp;

		if (!strcmp(v[1], "mod") || !strcmp(v[1], "modify"))
			++modify;
		else if (!strcmp(v[1], "dis") || !strcmp(v[1], "disable"))
			++disable;
		else if (!strcmp(v[1], "en") || !strcmp(v[1], "enable"))
			++enable;
		else
		if (!strcmp(v[1], "name"))			/* L010v */
			name++;
		else						/* L010^ */
			goto not3;

		if (v[2][0] == '*' && v[2][1] == '\0') {
			if (modify || name) {			/* L010 */
				printf(e_cantall);
				return DB_ERROR;
			}
			++all;
		}
		else {
			switch (bplookup(&rbp, &rdbp, v[2])) {
			case FOUND_NOTFOUND:
				if (modify || name) {		/* L002v L010 */

					/*
					 * user specified "bp mod <breakpoint>"
					 * or "bp name <breakpoint>" and the
					 * breakpoint doesn't exist.
					 * User wants to create a breakpoint
					 * with command lines, or a breakpoint
					 * with a name.
					 */

					create = 1;		/* L010 */
					break;
				}				/* L002^ */

				printf(e_unkbp, *v);
				return DB_ERROR;

			case FOUND_USERQ:
				return DB_CONTINUE;

			case FOUND_BP:
				if (modify)
					buflp = &rbp->bp_cmds;
				else				/* L010v */
				if (name)
					nm = &rbp->bp_name[0];	/* L010^ */
				else {
					if (enable)
						BKP_ENABLE(rbp);
					else if (disable)
						BKP_DISABLE(rbp);
					return DB_CONTINUE;
				}
				break;

			case FOUND_DBP:
				if (modify)
					buflp = &rdbp->bp_cmds;	/* L005 */
				else				/* L010v */
				if (name)
					nm = &rdbp->bp_name[0];	/* L010^ */
				else {
					if (enable) {
						drn = rdbp - &dbkp[0];
						sdbr(drn, getlinear(rdbp->bp_seg, rdbp->bp_off));
						dr7.d7_val |= SDBR(drn, rdbp->bp_type, rdbp->bp_mode);
						BKP_ENABLE(rdbp);
					}
					else if (disable) {
						CDBR(dr7.d7_val, rdbp - &dbkp[0]);
						BKP_DISABLE(rdbp);
					}
					return DB_CONTINUE;
				}
				break;
			}
		}
		if (all) {	/* enable or disable all breakpoints */
			/* normal */
			for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++) {
				if (BKP_ISSET(bp)) {
					if (disable)
						BKP_DISABLE(bp);
					else
						BKP_ENABLE(bp);
				}
			}
			/* data */
			for (dbp = &dbkp[0];dbp < &dbkp[MXNDBP];dbp++) {
				if (BKP_ISSET(dbp)) {
					if (disable) {
						CDBR(dr7.d7_val, dbp - &dbkp[0]);
						BKP_DISABLE(dbp);
					}
					else
						drn = dbp - &dbkp[0];
						sdbr(drn, getlinear(dbp->bp_seg, dbp->bp_off));
						dr7.d7_val |= SDBR(drn, dbp->bp_type, dbp->bp_mode);
						BKP_ENABLE(dbp);
				}
			}
			return DB_CONTINUE;
		}

		/*
		 * If the user specified "bp mod <breakpoint>" and the
		 * breakpoint exists, then enter edit mode on the command
		 * lines.  If the breakpoint doesn't exist, drop through
		 * to create it.
		 */

		if (modify && !create) {			/* L002 L010 */
			/* modify */
			list.li_bufl		= *buflp;
			list.li_flag		= 0;
			list.li_minum		= 1;
			list.li_pp		= list.li_prompt;
			list.li_mod		= bib_nfreelist;
			list_edit(&list, bib_alloc, bib_free);
			*buflp = list.li_bufl;
			return DB_CONTINUE;
		}

		/*
		 * The user specified "bp name <breakpoint>" and the breakpoint
		 * exists.  Just prompt for and change the name of the
		 * breakpoint.
		 */

		if (name && !create) {				/* L010v */
			prompt_bpname(nm, v[c - 1]);
			return(DB_CONTINUE);
		}						/* L010^ */
	}
not3:
	if (c == 5) {						/* L007v */
		/*
		 * We are decoding a value breakpoint command, for example:
		 *
		 *	bp wl &lbolt >= 100
		 *
		 * Setup the "lasta" variable to point to the breakpoint
		 * address ("&lbolt"), and setup the "lastc" variable to
		 * ensure that the loop below only scans the data breakpoint
		 * specifiers ("wl") and nothing else.
		 */

		lasta = v[2];
		lastc = 2;
	} else {
		lasta = v[c - 1];
		lastc = c - 1;
	}

	if (!modify && !name)					/* L002 L010 */
	    for (i = 1;i < lastc;i++) {				/* L007^ */
		    for (s = v[i];*s;s++) {
			    switch (*s) {
			    case 'r':	/* bp on read	*/
				    if (rwx & (DBP_READ|DBP_SEXEC|DBP_SIO)) /* L012 */
					    return DB_USAGE;
				    rwx |= DBP_READ;
				    break;

			    case 'w':	/* bp on write	*/
				    if (rwx & (DBP_WRITE|DBP_SEXEC|DBP_SIO)) /* L012 */
					    return DB_USAGE;
				    rwx |= DBP_WRITE;
				    break;

			    case 'x':	/* bp on exec	*/
				    if (rwx || dlen)
					    return DB_USAGE;
				    rwx |= DBP_SEXEC;
				    break;

			    case 'i':	/* bp on I/O */		/* L012v */
				    if (cpu_family < 5) {
					    printf("I/O breakpoints not supported on this processor\n");
					    return DB_USAGE;
				    }
				    if (rwx & (DBP_SEXEC|DBP_READ|DBP_WRITE))
					    return DB_USAGE;
				    rwx |= DBP_SIO;
				    break;			/* L012^ */

			    case 'b':	/* byte length	*/
				    if (dlen || (rwx & DBP_SEXEC))
					    return DB_USAGE;
				    dlen = MD_BYTE;
				    break;

			    case 's':	/* short length	*/
				    if (dlen || (rwx & DBP_SEXEC))
					    return DB_USAGE;
				    dlen = MD_SHORT;
				    break;

			    case 'l':	/* long length	*/
				    if (dlen || (rwx & DBP_SEXEC))
					    return DB_USAGE;
				    dlen = MD_LONG;
				    break;
					
			    default:
				    return DB_USAGE;
			    }
		    }
	    }
	if (rwx & DBP_READ)	/* must. */
		rwx |= DBP_WRITE;
	if (rwx == 0)		/* not set, just default to exec */
		rwx = DBP_NEXEC;
	else {

		/*
		 * Here if debug registers specified
		 */

		if (ICE) {
			printf("Sorry, debug registers can't be used with an ICE.\n");
			return DB_ERROR;
		}
		if (c == 5) {					/* L007v */
			/*
			 * Value breakpoint specified.
			 */

			if (rwx & DBP_SEXEC) {
				printf("Can only specify a value breakpoint on data accesses\n");
				return DB_ERROR;
			}
		}						/* L007^ */
	}

	if (dlen == 0)
		dlen = MD_LONG;

	/*
	 * No segment for I/O breakpoints.
	 */

	if (rwx & DBP_SIO)					/* L012v */
		seg = 0;
	else							/* L012^ */
		seg = REGP[T_CS];

	if (!getaddr(lasta, &seg, &off)) {	/* addr to break on */
		perr();
		return DB_ERROR;
	}

	/*
	 * No symbol names for I/O breakpoints.
	 */

	if (rwx & DBP_SIO)					/* L012v */
		sy = NULL;
	else {							/* L012^ */
		sy = findsym(off);

		if (sy && sy->se_vaddr == off)
			s = 0;
		else
			sy = NULL;
	}

	if (rwx == DBP_NEXEC || rwx == DBP_SEXEC) {
		/*
		*	exec breakpoint - make sure it's at an
		*	instruction boundary
		*/
		switch (ioff(seg, off, &xoff)) {
			case 1:
				if (xoff != off) {
					printf("Error: %s is not at beginning of an instruction.\n", symname(off, 0));
					return DB_ERROR;
				}
				break;
			case 0:		/* couldn't find */
				printf("Warning: don't know if %s is at beginning of an instruction.\n", symname(off, 0));
				break;
			case -1:	/* inv addr */
				badaddr(seg, xoff);
				return DB_ERROR;
		}
	}
	if (rwx == DBP_NEXEC) {		/* normal, exec bkp */
		if (sy && (sy->se_flags & SF_TEXT) == 0) {
			printf("Not a text address.\n");
			return DB_ERROR;
		}
		for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++) {
			if (BKP_ISSET(bp) && bp->bp_seg == seg && bp->bp_off == off) {
				printf("Breakpoint already set.\n");
				return DB_ERROR;
			}
			if (BKP_ISCLEAR(bp) && !fbp)
				fbp = bp;
		}
		if (!fbp) {
			printf("The maximum number of breakpoints (%d) is set.\n", scodb_mxnbp);
			return DB_ERROR;
		}
		BKP_SET(fbp);
		fbp->bp_cmds = NULL;
		fbp->bp_seg = seg;
		fbp->bp_off = off;
		nm = fbp->bp_name;
		buflp = &fbp->bp_cmds;
	}
	else {
		for (dbp = &dbkp[0];dbp < &dbkp[MXNDBP];dbp++) {
			if (BKP_ISSET(dbp) && dbp->bp_type == rwx &&
			    dbp->bp_mode == dlen && dbp->bp_seg == seg &&
			    dbp->bp_off == off) {		/* L003 */
				printf("Breakpoint already set.\n");
				return DB_ERROR;
			}
			if (!dbp->bp_type && !fdbp)
				fdbp = dbp;
		}
		if (!fdbp) {
			printf("The maximum number of data breakpoints (%d) is set.\n", MXNDBP);
			return DB_ERROR;
		}
		BKP_SET(fdbp);
		fdbp->bp_seg = seg;
		fdbp->bp_off = off;

		if (rwx == DBP_SIO)				/* L012 */
			rwx = DBP_IO;				/* L012 */

		fdbp->bp_type = rwx;

		if (rwx == DBP_SEXEC) {
			rwx = DBP_EXEC;
			dlen = MD_BYTE;		/* must be: p12-4 */
		}

		fdbp->bp_mode = dlen;
		fdbp->bp_cmds = NULL;
		drn = fdbp - &dbkp[0];

		if (rwx == DBP_IO) {				/* L012v */
			scodb_set_de();
			sdbr(drn, off);
		} else {					/* L012^ */
			linear = getlinear(seg, off);		/* L007v */
			sdbr(drn, linear);
		}

		if (c == 5) {
			/*
			 * Setup value breakpoint.
			 */
			bp_setup_value(drn, linear, dlen, v[3], v[4]);
		}						/* L007^ */

		dr7.d7_val |= SDBR(drn, rwx, dlen);
		lasta = "";
		nm = fdbp->bp_name;
		buflp = &fdbp->bp_cmds;
	}
	if (!name && (sy || (s = symname(off, 3)))) {		/* L010 */
		if (!s)
			s = sent_name(sy);			/* L000 */
		strncpy(nm, s, BPNL);
	}
	else {
		if (!name)					/* L010v */
			strcpy(nm, lasta);
		else
			prompt_bpname(nm, lasta);		/* L010^ */
	}

	/*
	 * If the user created the breakpoint using "bp mod <breakpoint>" then
	 * prompt for the commands to execute.  If the user created the
	 * breakpoint using "bp <breakpoint>" then don't prompt and
	 * assume no commands to execute.
	 */

	if (modify) {						/* L002 */
		if ((il = bib_alloc()) == NULL)
			printf(nomore);
		else {
			bib_free(il);	/* just checking! */
			list.li_flag	= 0;
			list.li_minum	= 1;
			list.li_mod	= bib_nfreelist;
			list.li_pp	= list.li_prompt;
			list.li_bufl	= NULL;
			printf("Enter commands to execute at breakpoint:\n");
			list_edit(&list, bib_alloc, bib_free);
			*buflp = list.li_bufl;
		}
	} else
		*buflp = NULL;					/* L002 */

	return DB_CONTINUE;
}

/*
 * Prompt the user for a breakpoint name and update the breakpoint
 * with it.  The argument passed are:
 *
 *	nm		pointer to name field in breakpoint structure,
 *	default_name	default name to use if the user just typed <Return>.
 */

prompt_bpname(char *nm, char *default_name)			/* L010v */
{
	char *s;

	for (;;) {
		char buf[DBIBFL];

		if (getistre("Breakpoint name: ", buf, 1)) {
			for (s = buf ; *s == ' ' || *s == '\t' ; s++)
				;
		} else
			s = "";
		if (s[0] == 'D' && s[1] == 'R' && (s[2] >= '0' && s[2] <= '3'))
			printf("Invalid breakpoint name.\n");
		else
			break;
	}
	strcpy(nm, *s ? s : default_name);
}								/* L010^ */

/*
 * This routine is called to setup a value breakpoint (ie: a data breakpoint
 * which fires only when a location reaches a certain value).
 *
 * Arguments:
 *	drn		debug register number (0-3),
 *	vaddr		virtual address of breakpoint,
 *	dlen		data length (MD_BYTE, MD_SHORT, MD_LONG),
 *	cond_str	conditional ("==", "!=", ">=", "<="),
 *	value_str	value to fire on.
 */

bp_setup_value(drn, vaddr, dlen, cond_str, value_str)		/* L007v */
unsigned	drn;		/* debug number */
unsigned	vaddr;		/* address of breakpoint */
unsigned	dlen;		/* size of breakpoint (byte, short, long) */
char		*cond_str;	/* one of ==, !=, >=, <= */
char		*value_str;	/* value of breakpoint */
{
	register char **cp;
	unsigned seg, off;
	struct bp_value *p;
	int i;

	if (!getaddr(value_str, &seg, &off)) {
		printf("Invalid value \"%s\"\n", value_str);
		return(-1);
	}

	p = &bp_values[BP_CURPROC].dr[drn];

	/*
	 * Find out which conditional was specified.
	 */

	for (cp = &bp_cond[0], i = 0  ;  *cp != NULL  ;  cp++, i++) {
		if (!strcmp(cond_str, *cp)) {
			p->cond = i;
			break;
		}
	}

	if (*cp == NULL) {
		printf("Invalid conditional \"%s\" ignored - must be one of: !=, ==, >=, <= or &\n",
			cond_str);
		return(-1);
	}

	p->value = off;
	p->vaddr = vaddr;
	p->length = dlen;

	return(0);
}

/*
 * Clear a value breakpoint.
 */

bp_clear_value(drn)
unsigned drn;
{
	bp_values[BP_CURPROC].dr[drn].cond = COND_ALWAYS;
}								/* L007^ */

/*
*	breakpoint clear command
*
*	can only clear using
*		address		for exec bps
*		DRn		for data bps
*		bp name		for any
*
*	we don't allow lookups of data breakpoints by address because
*	there is more to a data breakpoint than a match on address.
*/
NOTSTATIC
c_bc(c, v)
	int c;
	char **v;
{
	int ret = DB_CONTINUE;
	struct bkp *bp, *bp_lookup();
	struct dbkp *dbp;
	struct sent *sy, *findsym();
	int drn;						/* L007 */

	while (*++v) {
		if (**v == '*' && !(*v)[1]) {	/* clear all. */
			db_bp_init();
			break;
		}
		switch (bplookup(&bp, &dbp, *v)) {
			case FOUND_NOTFOUND:
				printf(e_unkbp, *v);
				ret = DB_ERROR;
				break;

			case FOUND_USERQ:
				break;

			case FOUND_BP:
				BKP_CLEAR(bp);
				cbpcmds(bp->bp_cmds);
				continue;

			case FOUND_DBP:
				BKP_CLEARD(dbp);		/* L003 */
				drn = dbp - &dbkp[0];		/* L007v */
				CDBR(dr7.d7_val, drn);
				bp_clear_value(drn);		/* L007^ */
				cbpcmds(dbp->bp_cmds);
				continue;
		}
		break;
	}
	return ret;
}

/*
*	breakpoint listing command
*/
NOTSTATIC
c_bl(c, v)
	int c;
	char **v;
{
	int n, ret = DB_CONTINUE;
	struct bkp *bp;
	struct dbkp *dbp;

	if (c == 1) {
		n = 0;
		for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++)
			if (BKP_ISSET(bp)) {
				prbp(bp, 0);
				++n;
			}
		for (dbp = &dbkp[0];dbp < &dbkp[MXNDBP];dbp++)
			if (BKP_ISSET(dbp)) {
				prdbp(dbp, 0);
				++n;
			}
		if (!n)
			printf("No breakpoints.\n");
	}
	else {
		while (*++v) {
			switch (bplookup(&bp, &dbp, *v)) {
				case FOUND_NOTFOUND:
					printf(e_unkbp, *v);
					ret = DB_ERROR;
					break;

				case FOUND_USERQ:
					break;

				case FOUND_BP:
					prbp(bp, 1);
					continue;

				case FOUND_DBP:
					prdbp(dbp, 1);
					continue;
			}
			break;
		}
	}
	return ret;
}

NOTSTATIC
bplookup(rbp, rdbp, nam)
	struct bkp **rbp;
	struct dbkp **rdbp;
	char *nam;
{
	int n, r;
	long off, seg;
	struct sent *sy, *findsym();
	register struct bkp *bp;
	struct bkp *bp_lookup();
	extern struct bkp scodb_bkp[];
	extern int scodb_mxnbp, *REGP;

	*rbp = 0;
	*rdbp = 0;

	/*
	*	see if it's a debug register
	*/

	if (nam[0] == 'D' && nam[1] == 'R' && nam[2] >= '0' &&
	    nam[2] <= '3' && nam[3] == '\0')
		return (bplookup_debugreg(rdbp, nam));		/* L002 */


	/*
	*	look for nam == address of breakpoint
	*/
	seg = REGP[T_CS];
	if (getaddr(nam, &seg, &off)) {
		/* try for exact */
		bp = bp_lookup(seg, off);
		if (bp) {
			*rbp = bp;
			return FOUND_BP;
		}

		/* try for +3 */
		sy = findsym(off);
		if (sy && sy->se_vaddr == off) {
			if (sy->se_flags & SF_TEXT)
				off += 3;
		}
		bp = bp_lookup(seg, off);
		if (bp) {
			*rbp = bp;
			return FOUND_BP;
		}
	}
	return FOUND_NOTFOUND;					/* L002 v */
}

/*
 * Lookup a debug breakpoint.  A breakpoint name of the form "DR[0-3]"
 * is passed in argument "nam".  If the breakpoint is not set then return
 * FOUND_NOTFOUND.  If the breakpoint is set then return FOUND_DBP and
 * fill in the "rdbp" argument with a pointer to the breakpoint's
 * information structure.
 */

STATIC
bplookup_debugreg(rdbp, nam)
struct dbkp **rdbp;
char *nam;
{
	int n;

	n = nam[2] - '0';

	if (!BKP_ISSET(&dbkp[n])) {
		printf("%s: breakpoint not set.\n", nam);
		return(FOUND_NOTFOUND);
	}
	*rdbp = &dbkp[n];
	return(FOUND_DBP);
}

/*
 * Lookup a breakpoint name of the form #name.  The name of the
 * breakpoint (without the leading hash) is passed in argument "nam".
 * A name can refer to:
 *
 *	- an explicit debug register (ie: "DR[0-3]"),
 *	- a named normal breakpoint,
 *	- a named debug breakpoint.
 *
 * If no breakpoint of that name is found, return FOUND_NOTFOUND.
 *
 * If a normal breakpoint of that name is found, return FOUND_BP and
 * fill in the "rbp" argument with a pointer to the breakpoint's information
 * structure.
 *
 * If a debug breakpoint of that name is found, return FOUND_DBP and fill
 * in the "rdbp" argument with a pointer to the breakpoint's information
 * structure.
 */

NOTSTATIC
bplookup_hashname(rbp, rdbp, nam)
struct bkp **rbp;
struct dbkp **rdbp;
char *nam;
{
	int n, r;

	/*
	 * Check if its a debug register
	 */

	if (nam[0] == 'D' && nam[1] == 'R' && nam[2] >= '0' &&
	    nam[2] <= '3' && nam[3] == '\0') 
		return (bplookup_debugreg(rdbp, nam));

	/*
	*	check if its a named normal breakpoint
	*/
	r = ufind(0,
		  nam,
		  (char *)scodb_bkp,
		  sizeof(scodb_bkp[0]), 
		  scodb_mxnbp,
		  (int)scodb_bkp[0].bp_name - (int)&scodb_bkp[0],
		  sizeof(scodb_bkp[0].bp_name),
		  0,
		  &n);
	if (r == FOUND_BP) {
		*rbp = &scodb_bkp[n];
		return FOUND_BP;
	}

	/*
	*	check if its a named data breakpoint
	*/
	r = ufind(0,
		  nam,
		  (char *)dbkp,
		  sizeof(dbkp[0]), 
		  MXNDBP,
		  (int)dbkp[0].bp_name - (int)&dbkp[0],
		  sizeof(dbkp[0].bp_name),
		  0,
		  &n);
	if (r == FOUND_BP) {
		*rdbp = &dbkp[n];
		return FOUND_DBP;
	}

	return r;
}								/* L002 ^ */






/*
*	print a data breakpoint
*
*	lines up to match with prdbp
*/
STATIC
prdbp(dbp, md)
	register struct dbkp *dbp;
	int md;
{
	int r;

	if (md == 2) {
		r = xbpcmds(dbp->bp_cmds);
		if (r != DB_RETURN)
			pdbp(dbp, 1);
		return r;
	}
	else {
		pdbp(dbp, 0);
		if (md == 1)
			pbpcmds(dbp->bp_cmds);
	}
	return DB_CONTINUE;
}

/*
*	print out a breakpoint
*	lines up to match with prdbp
*/
STATIC
prbp(bp, md)
	register struct bkp *bp;
	int md;
{
	int r;

	if (md == 2) {
		r = xbpcmds(bp->bp_cmds);
		if (r != DB_RETURN)
			pbp(bp, 1, 0);
		return r;
	}
	else {
		pbp(bp, 0, 0);
		if (md == 1)
			pbpcmds(bp->bp_cmds);
	}
	return DB_CONTINUE;
}

/*
*	print the bpcmds that a breakpoint has out.
*/
STATIC
pbpcmds(il)
	register struct ilin *il;
{
	register int i, j;
	char bf[DBIBFL];

	for (i = 1;il;i++) {
		db_cplb(bf, 0, 0, il);
		printf("       ");
		for (j = db_ndi(i, 10);j < 2;j++)
			putchar(' ');
		printf("%d> %s\n", i, bf);
		il = il->il_next;
	}
}

/*
*	return the bpcmds that this breakpoint has back to the list
*/
STATIC
cbpcmds(il)
	register struct ilin *il;
{
	register struct ilin *iln;

	while (il) {
		iln = il->il_next;
		bib_free(il);
		il = iln;
	}
}

/*
*	execute breakpoint commands...
*/
STATIC
xbpcmds(il)
	register struct ilin *il;
{
	register int r;
	char **v;
	struct dbcmd *cmd;

	while (il) {
		v = il->il_ivec;
		if ((cmd = getcmd(&il->il_narg, &v, 0)) == 0)
			break;
		if ((r = docmd(cmd, il->il_narg, v)) != DB_CONTINUE) {
			if (r == DB_USAGE)
				usage(cmd);
			return r;
		}
		il = il->il_next;
	}
	return DB_CONTINUE;
}

/*
*	initialize all breakpoints as unset
*/
NOTSTATIC
db_bp_init() {
	int i, index;						/* L007 */
	register struct bkp *bp;
	register struct dbkp *dbp;

	/*
	*	build, or rebuild, scodb_bibuf free list
	*/
	bib_freelist = NULL;
	for (i = 0;i < scodb_nbibufs;i++) {
		scodb_bibufs[i].il_next = bib_freelist;
		bib_freelist = &scodb_bibufs[i];
	}
	bib_nfreelist = scodb_nbibufs;
	dr7.d7_val = 0;
	for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++)
		BKP_CLEAR(bp);
	BKP_CLEAR(&sfbkp);
	for (dbp = &dbkp[0];dbp < &dbkp[MXNDBP];dbp++)
		BKP_CLEARD(dbp);				/* L003 */

	/*
	 * Reset the value-breakpoint structure so that debug
	 * register breakpoints work unconditionally.
	 */

	for (index = 0 ; index <= max_ACPUs ; index++)		   /* L007v */
		for (i = 0 ; i < MXNDBP ; i++)
			bp_values[index].dr[i].cond = COND_ALWAYS; /* L007^ */
}

STATIC
bp_lookupbyname(nm, seg)
	char *nm;
	int *seg;
{
	struct bkp *bp;
	struct dbkp *dbp;

	switch (bplookup(&bp, &dbp, nm)) {
		case FOUND_BP:
			if (seg)
				*seg = bp->bp_seg;
			return bp->bp_off;

		case FOUND_DBP:
			if (seg)
				*seg = dbp->bp_seg;
			return dbp->bp_off;
		
		default:
			return 0;
	}
}

STATIC
struct bkp *
bp_lookup(seg, off)
	long seg, off;
{
	register struct bkp *bp;

	for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++)
		if (BKP_ISSET(bp) && bp->bp_seg == seg &&	/* L002 */
			bp->bp_off == off)
			return bp;
	return NULL;
}

STATIC
pbp(bp, md, off)
	struct bkp *bp;
	int md;
	long off;
{
	char *s, *symname();

	if (bp)
		off = bp->bp_off;	/* in case +3, etc */
	if (md) {
		printf("BREAKPOINT: ");
		db_pproc();
	}
	printf("       ");
	if (bp && BKP_ISDISABLED(bp))				/* L006 */
		printf("(disabled) ");
	else
		printf("           ");
	pnz(off, 8);
	putchar(' ');
	s = symname(off, 4);					/* L004 */
	printf("%s: ", s);
	if (bp)
		printf("%s\n", bp->bp_name);
	else
		printf("UNKNOWN BREAKPOINT!\n");
}

STATIC
pdbp(dbp, md)
	register struct dbkp *dbp;
	int md;
{
	char *s, *symname();
	static char *modes[] = { "", "byte", "short", 0, "long", };
	static char *types[] = { 0, "write", "I/O read/write", "read/write", "execute" }; /* L012 */
	int drn;						/* L007 */
	struct bp_value *p;					/* L007 */

	if (md) {
		printf("DATA BREAKPOINT: ");
		db_pproc();
	}
	printf("       ");					/* L003 v */
	if (BKP_ISDISABLED(dbp))
		printf("(disabled) ");
	else
		printf("           ");
	pnz(dbp->bp_off, 8);					/* L003 ^ */
	putchar(' ');
	s = symname(dbp->bp_off, 4);				/* L004 */
	printf("%s: ", s);

	drn = dbp - &dbkp[0];					/* L007v */
	printf("DR%d: %s %s %s",
		drn,
		types[dbp->bp_type],
		modes[dbp->bp_mode],
		dbp->bp_name);
	
	p = &bp_values[BP_CURPROC].dr[drn];

	if (p->cond != COND_ALWAYS)
		printf(" %s %x\n", bp_cond[p->cond], p->value);	
	else
		printf("\n");					/* L007^ */
}

NOTSTATIC
bp_print(seg, off)
	long seg, off;
{
	struct bkp *bp, *bp_lookup();

	bp = bp_lookup(seg, off);
	if (bp)
		return prbp(bp, 2);
	else {
		pbp(0, 1, off);
		putchar('\t');
		pnz(off, 8);
		printf("\tUNKNOWN\n");
		return DB_CONTINUE;
	}
}

/*
*	unset all breakpoints on the list so that no breakpoints
*	will occur while in the debugger.
*
*	We clear DR7 for now as well.
*/
NOTSTATIC
bp_unsetall() {
	register struct bkp *bp;

	for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++)
		if (!BKP_ISCLEAR(bp))
			bp_unsetit(bp);
	bp_unsetdbkp(0);
}

NOTSTATIC
bp_specisset() {
	return BKP_ISSET(&sfbkp);
}

NOTSTATIC
bp_unsetspec() {
	if (!BKP_ISCLEAR(&sfbkp)) {
		bp_unsetit(&sfbkp);
		BKP_CLEAR(&sfbkp);
	}
}

STATIC
bp_unsetit(bp)
	register struct bkp *bp;
{
	db_putbyte_all(bp->bp_seg, bp->bp_off, bp->bp_svopcode);
}

/*
*	We don't set breakpoints until we exit the debugger,
*	so we don't breakpoint while running it...
*
*	The 386 has code breakpoints, but we don't use them
*	since they're too limited (only can do 4 of them).
*	so we do this by replacing an opcode instruction
*	with an int3.
*
*	Make sure that we don't set a breakpoint that is at
*	cs:eip, or else we'll have an immediate breakpoint -
*	if there is one, we don't set, but get into FROMBP mode
*	(which singlesteps) to the next instr, at which point we
*	set the breakpoint.
*
*	Sets DR7 here.
*/
NOTSTATIC
bp_setall(doitnow)
	int doitnow;
{
	register struct bkp *bp;
	extern int MODE;

	for (bp = &scodb_bkp[0];bp < &scodb_bkp[scodb_mxnbp];bp++)
		if (BKP_ISSET(bp) && !BKP_ISDISABLED(bp)) {
			bp_setit(bp, doitnow);
		}
	if (bp_setdbkp(1)) {
		MODE = FROMBP;
		REGP[T_EFL] |= PS_T;
	}
}

NOTSTATIC
bp_setdbkp(frombc) {
	register struct dbkp *dbp;

	if (!frombc) {
		/*
		*	not from broadcast (yet), so broadcast
		*	this one for MPX, so other systems
		*	will get their debug registers set up.
		*/
		broadcast(bp_setdbkp, 1);			/* L001 */
	}
	if (!ICE)
		sdbr(6, 0);	/* clear it */
	for (dbp = &dbkp[0];dbp < &dbkp[MXNDBP];dbp++)
		if (dbp->bp_type == DBP_SEXEC && dbp->bp_seg == REGP[T_CS] && dbp->bp_off == REGP[T_EIP])
			return 1;
	if (!ICE)
		sdbr(7, dr7.d7_val);
	return 0;
}

NOTSTATIC
bp_unsetdbkp(frombc) {
	if (!frombc) {
		/*
		*	not from broadcast (yet), so broadcast
		*	this one for MPX, so other systems
		*	will get their debug registers disabled.
		*/
		broadcast(bp_unsetdbkp, 1);			/* L001 */
	}
	if (!ICE)
		sdbr(7, 0);
}

NOTSTATIC
bp_setspec() {
	if (BKP_ISSET(&sfbkp)) {
		bp_setit(&sfbkp, 1);
		return 1;
	}
	return 0;
}

STATIC
bp_setit(bp, now)
	register struct bkp *bp;
	int now;
{
	extern int MODE;

	/* sanity check.... */
	if (!db_getbyte(bp->bp_seg, bp->bp_off, &bp->bp_svopcode)) {
		printf("scodb:bp_setit code at bp(%x:%x) has changed\n",
			bp->bp_seg, bp->bp_off);
		return /* eh? */;
	}
	if (now || bp->bp_seg != REGP[T_CS] || bp->bp_off != REGP[T_EIP]) {
		db_putbyte_all(bp->bp_seg, bp->bp_off, BPINST);
	} else {
		/*
		*	go to singlestep mode because
		*	we found a breakpoint AT our
		*	cs:eip, which would cause an
		*	immediate breakpoint fault
		*	again.  so don't set the bp
		*	but singlestep over it.
		*/
		MODE = FROMBP;
		REGP[T_EFL] |= PS_T;
	}
}

NOTSTATIC
setspec(ip)
	struct instr *ip;
{
	struct instr inst;
	extern int MODE;

	if (!ip) {
		ip = &inst;
		ip->in_seg = REGP[T_CS];
		ip->in_off = REGP[T_EIP];
		if (!disi(ip)) {
			badaddr(ip->in_seg, ip->in_off);
			return;
		}
	}
	if (ip->in_flag & I_CALL) {
		/*
		*	set the special function step breakpoint
		*
		*	note that disi() increments in_off already
		*	by in_len, so we don't have to.
		*/
		sfbkp.bp_seg = ip->in_seg;
		sfbkp.bp_off = ip->in_off;

		/* L009 - read the code byte before declaring sfbkp as 'SET' */
		if (db_getbyte(sfbkp.bp_seg, sfbkp.bp_off, &sfbkp.bp_svopcode))
			BKP_SET(&sfbkp);
		else
			BKP_CLEAR(&sfbkp);
	}
	else
		BKP_CLEAR(&sfbkp);
	MODE = SINGLESTEP;
	return;
}

#define	CHKBP(n) {						\
	if (DBPSET(n)) {					\
		if (DBPTYP(n) == DBP_EXEC)			\
			++iscode;				\
		switch (prdbp(&dbkp[n], 2)) {			\
			case DB_RETURN:				\
				++dbret;			\
				break;				\
			case DB_CONTINUE:			\
				break;  			\
			case DB_USAGE:				\
			case DB_ERROR:				\
				++dberr;			\
				break;				\
		}						\
		++set;						\
	}							\
}

/*
*	is this a data breakpoint?
*	if so, print all applicable ones
*
*	note that in "normal" breakpoints (int3 ones) we back up
*	EIP to point again at the int3 instruction; in here, though,
*	we don't mess with EIP at all.
*
*	code breakpoints happen before execution;
*	data breakpoints happen after the instruction.
*		in order to find the instruction, we disassemble
*		before our current location, looking for a match
*		in length that is a "mov" instruction.
*/
NOTSTATIC
databp() {
	int set = 0, iscode = 0, ot, r, dbret = 0, dberr = 0;
	long off, eip;
	union dbr6 dr6;
	char *symname();
	struct instr inst;

	dr6.d6_val = gdbr(6);
	CHKBP(0);
	CHKBP(1);
	CHKBP(2);
	CHKBP(3);
	if (set) {
		eip = REGP[T_EIP];
		if (!iscode) {
			inst.in_seg = KCSSEL;
			for (ot = 8;ot > 0;ot--) {
				inst.in_off = eip - ot;
				if (disi(&inst)) {
					if (ot != inst.in_len)
						continue;
					if (!strncmp(inst.in_opcn, "mov", 3)) {
						/* found it */
						break;
					}
				}
			}
			if (dberr || !dbret) {
				eip -= ot;
				printf("     at %s\n", symname(eip, 0));
			}
		}
	}
	if (dberr)
		return DB_ERROR;
	else if (dbret)
		return DB_RETURN;
	else
		return DB_CONTINUE;
}

#ifndef USER_LEVEL

/*
 * This routine is called whenever any breakpoint is encountered and
 * determines whether the debugger should be entered or, in the case of value
 * breakpoints, whether the breakpoint should be ignored.  
 * The return value from this function determines whether the debugger is
 * entered or not.  If the value 1 is returned, then the debugger is entered,
 * else if 0 is returned the debugger is not entered.
 */

int								/* L007v */
scodb_chkbrkpt()
{
	register unsigned debugstatus;
	register struct bp_value *p;
	unsigned i;

	debugstatus = _dr6();

	/*
	 * Check if the breakpoint trap was caused by a debug register or not
	 */

	if (!(debugstatus & (DR_TRAP0|DR_TRAP1|DR_TRAP2|DR_TRAP3)))
		return(1);

	i = ismpx() ? processor_index : 0;

	/*
	 * Decode which debug register fired, and point to the
	 * appropriate value breakpoint information.
	 */

	if (debugstatus & DR_TRAP0)
		p = &bp_values[i].dr[0];
	else
	if (debugstatus & DR_TRAP1)
		p = &bp_values[i].dr[1];
	else
	if (debugstatus & DR_TRAP2)
		p = &bp_values[i].dr[2];
	else
	if (debugstatus & DR_TRAP3)
		p = &bp_values[i].dr[3];

	if (p->cond == COND_ALWAYS)		/* no value specified */
		return(1);

	/*
	 * Turn off all debug breakpoints while we read the address, to avoid
	 * any recursive traps.
	 */

	sdbr(7, dr7.d7_val &					/* L008 */
	     ~(DR_LOCAL_ENABLE_MASK | DR_GLOBAL_ENABLE_MASK));	/* L008 */


	/*
	 * Grab the appropriate amount of data from the breakpoint address.
	 */

	switch (p->length) {
	case MD_BYTE:
		i = *((unsigned char *)p->vaddr);
		break;
	case MD_SHORT:
		i = *((unsigned short *)p->vaddr);
		break;
	case MD_LONG:
		i = *((unsigned long *)p->vaddr);
		break;
	}

	sdbr(7, dr7.d7_val);					/* L008 */

	/*
	 * Apply the appropriate conditional.
	 */

	switch (p->cond) {
	case COND_EQ:
		return (i == p->value);
	case COND_NE:
		return (i != p->value);
	case COND_GE:
		return (i >= p->value);
	case COND_LE:
		return (i <= p->value);
	case COND_AND:
		return ((i & p->value) != 0);
	}
}								/* L007^ */

#endif /* USER_LEVEL */
