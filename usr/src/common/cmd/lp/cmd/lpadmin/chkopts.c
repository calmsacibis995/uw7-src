/*		copyright	"%c%" 	*/


#ident	"@(#)chkopts.c	1.3"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:	chkopts.c
 *
 * DESCRIPTION:	Check the command line options passed to lpadmin
 *
 * SCCS:	chkopts.c 1.3 4/28/97 at 13:40:50
 *
 * CHANGE HISTORY:
 *
 * 28-04-97  Paul Cunningham       MRs ul96-26007, ul97-05502, ul96-07403 & 
 *                                     ul96-30304
 *           Change function chkopts so that if the user specified the -v 
 *           option but not the -H then  it check to see if the device is a
 *           named pipe, if so set the modules to be pushed to 'none' to prevent
 *           the 'default' setting pushing ldterm onto the streams pipe.
 *
 *******************************************************************************
 */

#include "stdio.h"
#include "string.h"
#include "pwd.h"
#include "sys/types.h"
#include "errno.h"
#include <mac.h>

#include "lp.h"
#include "printers.h"
#include "form.h"
#include "class.h"

#define	WHO_AM_I	I_AM_LPADMIN
#include "oam.h"

#include "lpadmin.h"

extern PRINTER		*printer_pointer;

extern PWHEEL		*pwheel_pointer;

extern struct passwd	*getpwnam();

void			chkopts2(),
			chkopts3();
static void		chksys();

FORM			formbuf;

char			**f_allow,
			**f_deny,
			**u_allow,
			**u_deny;

PRINTER			*oldp		= 0;

PWHEEL			*oldS		= 0;

unsigned short		daisy		= 0;

extern	u_long		Namemax;

static int		root_can_write();

static char		*unpack_sdn();

static char **		bad_list;

#if	defined(__STDC__)
static unsigned long	sum_chkprinter ( char ** , char * , char * , char * , char * , char * );
#else
static unsigned long	sum_chkprinter();
#endif

/*
 * Procedure:     chkopts
 *
 * Restrictions:
 *               getpwnam: None
 *               isprinter: None
 *               isclass: None
 *               tidbit: None
 * Notes -- CHECK LEGALITY OF COMMAND LINE OPTIONS
 */

void			chkopts ()
{
#if defined(CAN_DO_MODULES)
	struct stat		statbuf;
#endif

	/*
	 * Check -d.
	 */
	if (d) {
		if (
			a || c || f || j || m || M || O || p || r || u || x
#if	defined(DIRECT_ACCESS)
		     || C
#endif
#ifdef	NETWORKING
		     || R || s
#endif
		     || strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_DALONE);
			done (1);
		}

		if (
			*d
		     && !STREQU(d, NAME_NONE)
		     && !isprinter(d)
		     && !isclass(d)
		) {
			LP_ERRMSG1 (ERROR, E_ADM_NODEST, d);
			done (1);
		}
		return;
	}
	/*
	 * Check -O.
	 */
	if (O) {
		if (
			a || c || d || f || j || m || M || p || r || u || x
#if	defined(DIRECT_ACCESS)
		     || C
#endif
		     || strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_OALONE);
			done (1);
		}
		return;
	}

	/*
	 * Check -x.
	 */
	if (x) {
		if (	/* MR bl88-02718 */
			A || a || c || f || j || m || M || p || r || u || d
#if	defined(DIRECT_ACCESS)
		     || C
#endif
		     || strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_XALONE);
			done (1);
		}

		if (
			!STREQU(NAME_ALL, x)
		     && !STREQU(NAME_ANY, x)
		     && !isprinter(x)
		     && !isclass(x)
		) {
			LP_ERRMSG1 (ERROR, E_ADM_NODEST, x);
			done (1);
		}
		return;
	}

	/*
	 * Problems common to both -p and -S (-S alone).
	 */
	if (A && STREQU(A, NAME_LIST) && (W != -1 || Q != -1)) {
		LP_ERRMSG (ERROR, E_ADM_LISTWQ);
		done (1);
	}


	/*
	 * Check -S.
	 */
	if (!p && S) {
		if (
			M || a || f || c || r || e || i || m || H || h
		     || l || v || I || T || D || F || u || U || j || o || R
		) {
			LP_ERRMSG (ERROR, E_ADM_SALONE);
			done (1);
		}
		if (!A && W == -1 && Q == -1) {
			LP_ERRMSG (ERROR, E_ADM_NOAWQ);
			done (1);
		}
		if (S[0] && S[1])
			LP_ERRMSG (WARNING, E_ADM_ASINGLES);
		if (!STREQU(NAME_ALL, *S) && !STREQU(NAME_ANY, *S)) 
			chkopts3(1);
		return;
	}

	/*
	 * At this point we must have a printer (-p option).
	 */
	if (!p) {
		LP_ERRMSG (ERROR, E_ADM_NOACT);
		done (1);
	}
	if (STREQU(NAME_NONE, p)) {
		LP_ERRMSG1 (ERROR, E_LP_NULLARG, "p");
		done (1);
	}


	/*
	 * Mount but nothing to mount?
	 */
	if (M && (!f && !S)) {
		LP_ERRMSG (ERROR, E_ADM_MNTNONE);
		done (1);
	}

	/*
	 * -Q isn't allowed with -p.
	 */
	if (Q != -1) {
		LP_ERRMSG (ERROR, E_ADM_PNOQ);
		done (1);
	}

	/*
	 * Fault recovery.
	 */
	if (
		F
	     && !STREQU(F, NAME_WAIT)
	     && !STREQU(F, NAME_BEGINNING)
	     && (
			!STREQU(F, NAME_CONTINUE)
		     || j
		     && STREQU(F, NAME_CONTINUE)
		)
	) {
#if	defined(J_OPTION)
		if (j)
			LP_ERRMSG (ERROR, E_ADM_FBADJ);
		else
#endif
			LP_ERRMSG (ERROR, E_ADM_FBAD);
		done (1);
	}

#if	defined(J_OPTION)
	/*
	 * The -j option is used only with the -F option.
	 */
 	if (j) {
		if (M || a || f || c || r || e || i || m || H || h ||
		    l || v || I || T || D || u || U || o) {
			LP_ERRMSG (ERROR, E_ADM_JALONE);
			done (1);
		}
		if (j && !F) {
			LP_ERRMSG (ERROR, E_ADM_JNOF);
			done (1);
		}
		return;
	}
#endif

#if	defined(DIRECT_ACCESS)
	/*
	 * -C is only used to modify -u
	 */
	if (C && !u) {
		LP_ERRMSG (ERROR, E_ADM_CNOU);
		done (1);
	}
#endif

	/*
	 * The -a option needs the -M and -f options,
	 * Also, -ofilebreak is used only with -a.
	 */
	if (a && (!M || !f)) {
		LP_ERRMSG (ERROR, E_ADM_MALIGN);
		done (1);
	}
	if (filebreak && !a)
		LP_ERRMSG (WARNING, E_ADM_FILEBREAK);

	/*
	 * The "-p all" case is restricted to certain options.
	 */
	if (
		(STREQU(NAME_ANY, p) || STREQU(NAME_ALL, p))
	     && (
			a || h || l || M || D || e || f || H
		     || i || I || m || S || T || u || U || v
		     || banner != -1 || cpi || lpi || width || length || stty
		)
	) {
		LP_ERRMSG (ERROR, E_ADM_ANYALLNONE);
		done (1);

	} 

	/*
	 * Allow giving -v or -U option as way of making
	 * remote printer into local printer.
	 * Note: "!s" here means the user has not given the -s;
	 * later it means the user gave -s local-system.
	 */
	if (!s && (v || U))
		s = Local_System;

#if defined(CAN_DO_MODULES)
        /* If -v is being used and -H isn't, check to see if the device is a
	 * named pipe, if so set the modules to be pushed to 'none' to prevent 
	 * the 'default' setting pushing ldterm onto the streams pipe. This
	 * also applies when changing the device of an existing printer to a
	 * named pipe. If this is an existing printer and both the old and new
	 * devices are named pipes and the old device had modules specified in
	 * the module list then these will be removed and set to 'none' unless
         * the user re-specifies the modules using the -H option. Usually of
	 * course the module list is retained for existing printers when
	 * changing the device with the -v option and not using the -H option.
         */

	if ( v && !H)
	{
	  if ( Stat( v, &statbuf) != -1)
	  {
	    if (( statbuf.st_mode & S_IFMT) == S_IFIFO) 
	    {
	      H = getlist( NAME_NONE, LP_WS, LP_SEP);
	      strcat( modifications, "H");
	    }
	  }
	}
#endif

	/*
	 * Be careful about checking "s" before getting here.
	 * We want "s == 0" to mean this is a local printer; however,
	 * if the user wants to change a remote printer to a local
	 * printer, we have to have "s == Local_System" long enough
	 * to get into "chkopts2()" where a special check is made.
	 * After "chkopts2()", "s == 0" means local.
	 */
	if (!STREQU(NAME_ALL, p) && !STREQU(NAME_ANY, p)) 
		/*
		 * If old printer, make sure it exists. If new printer,
		 * check that the name is okay, and that enough is given.
		 * (This stuff has been moved to "chkopts2()".)
		 */
		chkopts2(1);

	if (!s) {

		/*
		**  If a remote system is not being defined then
		**  this has no meaning.
		*/
		if (R)
		{
			LP_ERRMSG (ERROR, E_ADM_CLNPR);
			done (1);
		}
		/*
		 * Only one of -i, -m, -e.
		 */
		if ((i && e) || (m && e) || (i && m)) {
			LP_ERRMSG (ERROR, E_ADM_INTCONF);
			done (1);
		}

		/*
		 * Check -e arg.
		 */
		if (e) {
			if (!isprinter(e)) {
				LP_ERRMSG1 (ERROR, E_ADM_NOPR, e);
				done (1);
			}
			if (strcmp(e, p) == 0) {
				LP_ERRMSG (ERROR, E_ADM_SAMEPE);
				done (1);
			}
		}

		/*
		 * Check -m arg.
		 */
		if (m && !ismodel(m)) {
			LP_ERRMSG1 (ERROR, E_ADM_NOMODEL, 
					makepath(Lp_Model, m, (char *) 0));
			done (1);
		}

		/*
		 * Need exactly one of -h or -l (but will default -h).
		 */
		if (h && l) {
			LP_ERRMSG2 (ERROR, E_ADM_CONFLICT, 'h', 'l');
			done (1);
		}
		if (!h && !l)
			h = 1;

		/*
		 * Check -c and -r.
		 */
		if (c && r && strcmp(c, r) == 0) {
			LP_ERRMSG (ERROR, E_ADM_SAMECR);
			done (1);
		}


		/*
		 * Are we creating a class with the same name as a printer?
		 */
		if (c) {
			if (STREQU(c, p)) {
				LP_ERRMSG1 (ERROR, E_ADM_CLPR, c);
				done (1);
			}
			if (isprinter(c)) {
				LP_ERRMSG1 (ERROR, E_ADM_CLPR, c);
				done (1);
			}
		}

		/*
		 * The device must be writeable by root.
		 */
		if (v && root_can_write(v) == -1)
			done (1);

		/*
		 * Can't have both device and dial-out.
		 */
		if (v && U) {
			LP_ERRMSG (ERROR, E_ADM_BOTHUV);
			done (1);
		}

	} else
		if (
			A || a || e || F || H || h || i || l || m || M
		     || (o && banner == -1) || U || v || Q != -1 || W != -1
		) {
			LP_ERRMSG (ERROR, E_ADM_NOTLOCAL);
			done(1);
		}


	/*
	 * We need the printer type for some things, and the boolean
	 * "daisy" (from Terminfo) for other things.
	 */
	if (!T && oldp)
		T = oldp->printer_types;
	if (T) {
		short			a_daisy;

		char **			pt;


		if (lenlist(T) > 1 && searchlist(NAME_UNKNOWN, T)) {
			LP_ERRMSG (ERROR, E_ADM_MUNKNOWN);
			done (1);
		}

		for (pt = T; *pt; pt++)
			if (tidbit(*pt, (char *)0) == -1) {
				LP_ERRMSG1 (ERROR, E_ADM_BADTYPE, *pt);
				done (1);
			}

		/*
		 * All the printer types had better agree on whether the
		 * printer takes print wheels!
		 */
		daisy = a_daisy = -1;
		for (pt = T; *pt; pt++) {
			tidbit (*pt, "daisy", &daisy);
			if (daisy == -1)
				daisy = 0;
			if (a_daisy == -1)
				a_daisy = daisy;
			else if (a_daisy != daisy) {
				LP_ERRMSG (ERROR, E_ADM_MIXEDTYPES);
				done (1);
			}
		}
	}
	if (cpi || lpi || length || width || S || f || filebreak)
		if (!T) {
			LP_ERRMSG (ERROR, E_ADM_TOPT);
			done (1);

		}

	/*
	 * Check -o cpi=, -o lpi=, -o length=, -o width=
	 */
	if (cpi || lpi || length || width) {
		unsigned	long	rc;

		if ((rc = sum_chkprinter(T, cpi, lpi, length, width, NULL)) == 0) {
			if (bad_list)
				LP_ERRMSG1 (
					INFO,
					E_ADM_NBADCAPS,
					sprintlist(bad_list)
				);

		} else {
			if ((rc & PCK_CPI) && cpi)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "cpi=");

			if ((rc & PCK_LPI) && lpi)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "lpi=");

			if ((rc & PCK_WIDTH) && width)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "width=");

			if ((rc & PCK_LENGTH) && length)
				LP_ERRMSG1 (ERROR, E_ADM_BADCAP, "length=");

			LP_ERRMSG (ERROR, E_ADM_BADCAPS);
			done(1);
		}
	}

	/*
	 * Check -I (old or new):
	 */
	if (T && lenlist(T) > 1) {

#define BADILIST(X) (lenlist(X) > 1 || X && *X && !STREQU(NAME_SIMPLE, *X))
		if (
			I && BADILIST(I)
		     || !I && oldp && BADILIST(oldp->input_types)
		) {
			LP_ERRMSG (ERROR, E_ADM_ONLYSIMPLE);
			done (1);
		}
	}

	/*
	 * MOUNT:
	 * Only one print wheel can be mounted at a time.
	 */
	if (M && S && S[0] && S[1])
		LP_ERRMSG (WARNING, E_ADM_MSINGLES);

	/*
	 * NO MOUNT:
	 * If the printer takes print wheels, the -S argument
	 * should be a simple list; otherwise, it must be a
	 * mapping list. (EXCEPT: In either case, "none" alone
	 * means delete the existing list.)
	 */
	if (S && !M) {
		register char		**item,
					*cp;

		/*
		 * For us to be here, "daisy" must have been set.
		 * (-S requires knowing printer type (T), and knowing
		 * T caused call to "tidbit()" to set "daisy").
		 */
		if (!STREQU(S[0], NAME_NONE) || S[1])
		    if (daisy) {
			for (item = S; *item; item++) {
				if (strchr(*item, '=')) {
					LP_ERRMSG (ERROR, E_ADM_PWHEELS);
					done (1);
				}
				if (!syn_name(*item)) {
					LP_ERRMSG2 (ERROR, E_LP_NOTNAME, *item,							Namemax);
					done (1);
				}
			}
		    } else {
			register int		die = 0;

			for (item = S; *item; item++) {
				if (!(cp = strchr(*item, '='))) {
					LP_ERRMSG (ERROR, E_ADM_CHARSETS);
					done (1);
				}

				*cp = 0;
				if (!syn_name(*item)) {
					LP_ERRMSG2 (ERROR, E_LP_NOTNAME, *item,							Namemax);
					done (1);
				}
				if (PCK_CHARSET & sum_chkprinter(T, (char *)0, (char *)0, (char *)0, (char *)0, *item)) {
					LP_ERRMSG1 (ERROR, E_ADM_BADSET, *item);
					die = 1;
				} else {
					if (bad_list)
						LP_ERRMSG2 (
							INFO,
							E_ADM_NBADSET,
							*item,
							sprintlist(bad_list)
						);
				}
				*cp++ = '=';
				if (!syn_name(cp)) {
					LP_ERRMSG2 (ERROR, E_LP_NOTNAME, cp,							Namemax);
					done (1);
				}
			}
			if (die) {
				LP_ERRMSG (ERROR, E_ADM_BADSETS);
				done (1);
			}
		}
	}

	/*
	 * NO MOUNT:
	 * The -f option restricts the forms that can be used with
	 * the printer.
	 *	- construct the allow/deny lists
	 *	- check each allowed form to see if it'll work
	 *	  on the printer
	 */
	if (f && !M) {
		register char		*type	= strtok(f, ":"),
					*str	= strtok((char *)0, ":"),
					**pf;

		register int		die	= 0;


		if (STREQU(type, NAME_ALLOW) && str) {
			if ((pf = f_allow = getlist(str, LP_WS, LP_SEP)) != NULL) {
				while (*pf) {
					if (
						!STREQU(*pf, NAME_NONE)
					     && verify_form(*pf) < 0
					)
						die = 1;
					pf++;
				}
				if (die) {
					LP_ERRMSG (ERROR, E_ADM_FORMCAPS);
					done (1);
				}

			} else
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_ALLOW);

		} else if (STREQU(type, NAME_DENY) && str) {
			if ((pf = f_deny = getlist(str, LP_WS, LP_SEP)) != NULL)
			{
				while (*pf) {
					if (
						!STREQU(*pf, NAME_NONE) &&
						!STREQU(*pf, NAME_ANY) &&
						!STREQU(*pf, NAME_ALL) &&
					     getform(*pf,&formbuf,(FALERT *)0,
							(FILE **)0) < 0
					)
						LP_ERRMSG1 (WARNING,E_ADM_BADFORM,*pf);
					pf++;
				}
			} else
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_DENY);

		} else {
			LP_ERRMSG (ERROR, E_ADM_FALLOWDENY);
			done (1);
		}
	}

	/*
	 * The -u option is setting use restrictions on printers.
	 *	- construct the allow/deny lists
	 */
	if (u) {
		register char		*type	= strtok(u, ":"),
					*str	= strtok((char *)0, ":"),
					**pn;
		char			*sysp		= (char *)0,
					*save_pn	= (char *)0,
					*namep		= (char *)0;
		int			islocalsys	= 0;

		if (STREQU(type, NAME_ALLOW) && str) {
			if ((pn = u_allow = getlist(str, LP_WS, LP_SEP)) == NULL)
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_ALLOW);

		} else if (STREQU(type, NAME_DENY) && str) {
			if ((pn = u_deny = getlist(str, LP_WS, LP_SEP)) == NULL)
				LP_ERRMSG1 (WARNING, E_ADM_MISSING, NAME_DENY);

		} else {
			LP_ERRMSG (ERROR, E_LP_UALLOWDENY);
			done (1);
		}

		/*
		 * Parse list of user-specifications to allow/deny
		 * Form is SYS!USER - variations include !USER USER ...
		 * If only part of user-specification is supplied treat that
		 * as USER part.
		 * If USER is a user on the current system, validate USER
		 * and post WARNING if fails
		*/
		while (*pn) {

			save_pn = Strdup (*pn);
			sysp	= strtok (save_pn, BANG_S);
			namep	= strtok((char *)0, "");
		    
		    if (!namep) {
				namep = sysp;
				sysp = (char *)0;
		    }

		    if (!namep || strchr (namep, BANG_C))
				LP_ERRMSG1 (WARNING, E_ADM_BADUSR, *pn);
		    else
		    if (!sysp || STREQU (sysp, Local_System)) {
			islocalsys = 1;
			if (
				   !STREQU(namep, NAME_NONE)
		     		&& !STREQU(namep, NAME_ANY)
				&& !STREQU(namep, NAME_ALL)
				&& !getpwnam(namep)
			)
				LP_ERRMSG1 (WARNING, E_ADM_BADUSR, *pn);
		    }
		    if (islocalsys) {
			Free (*pn);
			*pn = namep;
			islocalsys = 0;
		    }
		    else
			Free (save_pn);

		    pn++;
		}
	}

	return;
}

/*
 * Procedure:     root_can_write
 *
 * Restrictions:
 *               getpwnam: None
 *               endpwent: None
 *               devstat(2): None
 *               lvlvalid: None
 *
 * Notes - CHECK THAT "root" CAN SENSIBLY WRITE TO PATH
 */

static int		root_can_write (path)
	char			*path;
{
	static int		lp_uid		= -1;

	struct passwd		*ppw;

	struct stat		statbuf;

	struct devstat		devbuf;

	int 			n;

	if (Stat(path, &statbuf) == -1) {
		LP_ERRMSG1 (ERROR, E_ADM_NOENT, v);
		return (-1);
	}

	if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		LP_ERRMSG1 (WARNING, E_ADM_ISDIR, v);

	else 
	if ((statbuf.st_mode & S_IFMT) == S_IFBLK)
		LP_ERRMSG1 (WARNING, E_ADM_ISBLK, v);

	if (lp_uid == -1) {
		if (!(ppw = getpwnam(LPUSER)))
			ppw = getpwnam(ROOTUSER);
		endpwent ();
		if (ppw)
			lp_uid = ppw->pw_uid;
		else
			lp_uid = 0;
	}
	if (!STREQU(v, "/dev/null"))  {
	    if ((statbuf.st_uid && statbuf.st_uid != lp_uid)
		|| (statbuf.st_mode & (S_IWGRP|S_IRGRP|S_IWOTH|S_IROTH)))
		LP_ERRMSG1 (WARNING, E_ADM_DEVACCESS, v);
	}
	/* make sure the device is allocated and configured
	 * ul90-34010 abs s19
	 */
	while ((n = devstat(v, DEV_GET, &devbuf)) < 0 && errno == EINTR)
	    continue;
	if ( n < 0 ) {		/* devstat failed */
	    /* don't need to allocate if ES isn't installed or
	     * or if device is an ordinary file */
	    if (errno == ENOPKG || errno == ENODEV)
		return 0;
	    else {
		LP_ERRMSG1 (ERROR, E_ADM_BADDEV, v);
		return -1;
	    }
	}
	if (devbuf.dev_state != DEV_PUBLIC || devbuf.dev_mode != DEV_STATIC) {
	    LP_ERRMSG1 (ERROR, E_ADM_BADDEV, v);
	    return -1;
	}
	else if (devbuf.dev_relflag != DEV_SYSTEM) {
	    while ((n = lvlvalid(&devbuf.dev_hilevel)) < 0 && errno == EINTR)
		continue;
	    if (n < 0) {
		LP_ERRMSG1 (ERROR, E_ADM_BADDEV, v);
		return -1;
	    }
	    while ((n = lvlvalid(&devbuf.dev_lolevel)) < 0 &&
			errno == EINTR)
                continue;
            if (n < 0) {
                LP_ERRMSG1 (ERROR, E_ADM_BADDEV, v);
                return -1;
            }
	}
	return 0;
}

/**
 ** unpack_sdn() - TURN SCALED TYPE INTO char* TYPE
 **/

static char		*unpack_sdn (sdn)
	SCALED			sdn;
{
	register char		*cp;
	extern char		*malloc();

	if (sdn.val <= 0 || 99999 < sdn.val)
		cp = 0;

	else if (sdn.val == N_COMPRESSED)
		cp = strdup(NAME_COMPRESSED);

	else if ((cp = malloc(sizeof("99999.999x"))))
		(void) sprintf(cp, "%.3f%c", sdn.val, sdn.sc);

	return (cp);
}

/*
 * Procedure:     verify_form
 *
 * Restrictions:
 *               getform: None
 * Notes - SEE IF PRINTER CAN HANDLE FORM
 */

int			verify_form (form)
	char			*form;
{
	register char		*cpi_f,
				*lpi_f,
				*width_f,
				*length_f,
				*chset;

	register int		rc	= 0;
	register int		gf	= 0;

	register unsigned long	checks;


	if (STREQU(form, NAME_ANY))
		form = NAME_ALL;

	while ((gf = getform(form, &formbuf, (FALERT *)0, (FILE **)0)) == 0
			|| (STREQU (form, NAME_ALL) && errno != ENOENT)) {

		if (gf < 0)
			continue;

		cpi_f = unpack_sdn(formbuf.cpi);
		lpi_f = unpack_sdn(formbuf.lpi);
		width_f = unpack_sdn(formbuf.pwid);
		length_f = unpack_sdn(formbuf.plen);

		if (
			formbuf.mandatory
		     && !daisy
		     && !search_cslist(
				formbuf.chset,
				(S && !M? S : (oldp? oldp->char_sets : (char **)0))
			)
		)
			chset = formbuf.chset;
		else
			chset = 0;

		if ((checks = sum_chkprinter(
			T,
			cpi_f,
			lpi_f,
			length_f,
			width_f,
			chset
		))) {
			rc = -1;
			if ((checks & PCK_CPI) && cpi_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "cpi");

			if ((checks & PCK_LPI) && lpi_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "lpi");

			if ((checks & PCK_WIDTH) && width_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "width");

			if ((checks & PCK_LENGTH) && length_f)
				LP_ERRMSG1 (INFO, E_ADM_BADCAP, "length");

			if ((checks & PCK_CHARSET) && formbuf.chset) {
				LP_ERRMSG1 (INFO, E_ADM_BADSET, formbuf.chset);
				rc = -2;
			}
			LP_ERRMSG1 (INFO, E_ADM_FORMCAP, formbuf.name);
		} else {
			if (bad_list)
				LP_ERRMSG2 (
					INFO,
					E_ADM_NBADMOUNT,
					formbuf.name,
					sprintlist(bad_list)
				);
		}

		if (!STREQU(form, NAME_ALL))
			return (rc);

	}
	if (!STREQU(form, NAME_ALL)) {
		LP_ERRMSG1 (ERROR, E_LP_NOFORM, form);
		done (1);
	}

	return (rc);
}

/*
 * Procedure:     chkopts2
 *
 * Restrictions:
 *               getprinter: None
 *               getclass: None
 * 
 * Notes:
	Second phase of parsing for -p option.
	In a seperate routine so we can call it from other
	routines. This is used when any or all are used as 
	a printer name. main() loops over each printer, and
	must call this function for each printer found.
*/
void
chkopts2(called_from_chkopts)
int	called_from_chkopts;
{
	/*
		Only do the getprinter() if we are not being called
		from lpadmin.c. Otherwise we mess up our arena for 
		"all" processing.
	*/
	if (!called_from_chkopts)
		oldp = printer_pointer;
	else if (!(oldp = getprinter(p)) && errno != ENODATA) {
		LP_ERRMSG2 (ERROR, E_LP_GETPRINTER, p, PERROR);
		done(1);
	}

	if (oldp) {
		if (
			!c && !d && !f && !M && !r && !R && !u && !x && !A
	     		&& !strlen(modifications)
		) {
			LP_ERRMSG (ERROR, E_ADM_PLONELY);
			done (1);
		}

		/*
		 * For the case "-s local-system", we need to keep
		 * "s != 0" long enough to get here, where it keeps
		 * us from taking the old value. After this, we make
		 * "s == 0" to indicate this is a local printer.
		 */
		if (s && s != Local_System)
			chksys(s);
		if (!s && oldp->remote && *(oldp->remote))
			s = strdup(oldp->remote);
		if (s == Local_System)
			s = 0;

		/*
		 * Check that if the -R option is used, that the
		 * printer is a remote printer.  If local, then
		 * error message
		 */
		if (R && !s) {	                      /* ul90-35224 abs s19 */

		/* ERROR: The -R Option can only be used with remote printer */
			LP_ERRMSG (ERROR, E_ADM_CLNPR);
			done (1);
		}

		/*
		 * A remote printer converted to a local printer
		 * requires device or dial info.
		 */
		if (!s && oldp->remote && !v && !U) {
			LP_ERRMSG (ERROR, E_ADM_NOUV);
			done (1);
		}

#if	defined(CAN_DO_MODULES)
		if (!s)
			if (!H && oldp->modules)
				H = duplist(oldp->modules);
#endif

	} else {
		if (getclass(p)) {
			LP_ERRMSG1 (ERROR, E_ADM_PRCL, p);
			done (1);
		}

		if (!syn_name(p)) {
			LP_ERRMSG2 (ERROR, E_LP_NOTNAME, p, Namemax);
			 done (1);
		}

		if (s == Local_System)
			s = 0;
		if (s)
			chksys(s);

		/*
		 * New printer - if no model, use standard
		 */
		if (!(e || i || m) && !s)
			m = STANDARD;

#if	defined(CAN_DO_MODULES)
		if (!H) {
			if (!s) {
				H = getlist(NAME_DEFAULT, LP_WS, LP_SEP);
				strcat (modifications, "H");
			}
		}
#endif

		/*
		 * A new printer requires device or dial info.
		 */
		if (!v && !U && !s) {
#ifdef	NETWORKING
			LP_ERRMSG (ERROR, E_ADM_NOUV);
#else
			LP_ERRMSG (ERROR, E_ADM_NOV);
#endif
			done (1);
		}

		/*
		 * Can't quiet a new printer,
		 * can't list the alerting for a new printer.
		 */
		if (
			A
		     && (STREQU(A, NAME_QUIET) || STREQU(A, NAME_LIST))
		) {
			LP_ERRMSG1 (ERROR, E_ADM_BADQUIETORLIST, p);
			done (1);
		}

		/*
		 * New printer - if no input types given, assume "simple".
		 */
		if (!I) {
			I = getlist(NAME_SIMPLE, LP_WS, LP_SEP);
			strcat (modifications, "I");
		}

		/*
		 * New printer - if -R is given, s must be specified.
		 */

		/* ERROR: For a new printer, -R Option can only be used 
		for a remote printer (must specify the -s option )*/
		if (R && !s) {
			LP_ERRMSG (ERROR, E_ADM_CLNPR);
			done (1);
		}
	}
}

/*
 * Procedure:     chkopts3
 *
 * Restrictions:
 *               getpwheel: None
 * Notes:
	Second phase of parsing for -S option.
	In a seperate routine so we can call it from other
	routines. This is used when any or all are used as 
	a print wheel name. main() loops over each print wheel,
	and must call this function for each print wheel found.
*/
void
chkopts3(called_from_chkopts)
int	called_from_chkopts;
{
	/*
		Only do the getpwheel() if we are not being called
		from lpadmin.c. Otherwise we mess up our arena for 
		"all" processing.
	*/
	if (!called_from_chkopts)
		oldS = pwheel_pointer;
	else
		oldS = getpwheel(*S);

	if (!oldS) {
		if (!syn_name(*S)) {
			LP_ERRMSG2 (ERROR, E_LP_NOTNAME, *S, Namemax);
			done (1);
		}

		/*
		 * Can't quiet a new print wheel,
		 * can't list the alerting for a new print wheel.
		 */
		if (
			A
		     && (STREQU(A, NAME_QUIET) || STREQU(A, NAME_LIST))
		) {
			LP_ERRMSG1 (ERROR, E_ADM_BADQUIETORLIST, *S);
			done (1);
		}
	}
}

/*
 * Procedure:     chksys
 *
 * Restrictions:
 *               getsystem: None
*/
static void
chksys(s)
char	*s;
{
	char	*cp;

	if (STREQU(s, NAME_ALL) || STREQU(s, NAME_ANY)) {
		LP_ERRMSG (ERROR, E_ADM_ANYALLSYS);
		done(1);
	}

	if ((cp = strchr(s, '!')) != NULL)
		*cp = '\0';

	if (getsystem(s) == NULL) {
		if (errno != ENOENT)
			LP_ERRMSG2 (ERROR, E_ADM_GETSYS, s, PERROR);
		else
			LP_ERRMSG1 (ERROR, E_ADM_NOSYS, s);
		done(1);
	}

	if (cp)
		*cp = '!';

	return;
}

/*
 * Procedure:     sum_chkprinter
 *
 * Restrictions:
 *               chkprinter: None
 *
 * notes - CHECK TERMINFO STUFF FOR A LIST OF PRINTER TYPES
 */

#include "lp.set.h"

static unsigned long
#if	defined(__STDC__)
sum_chkprinter (
	char **			types,
	char *			cpi,
	char *			lpi,
	char *			len,
	char *			wid,
	char *			cs
)
#else
sum_chkprinter (types, cpi, lpi, len, wid, cs)
	char **			types;
	char *			cpi;
	char *			lpi;
	char *			len;
	char *			wid;
	char *			cs;
#endif
{
	char **			pt;

	unsigned long		worst	= 0;
	unsigned long		this	= 0;


	/*
	 * Check each printer type, to see if any won't work with
	 * the attributes requested. However, return ``success''
	 * if at least one type works. Keep a list of the failed
	 * types for the caller to report.
	 */
	bad_list = 0;
	for (pt = types; *pt; pt++) {
		this = chkprinter(*pt, cpi, lpi, len, wid, cs);
		if (this != 0)
			addlist (&bad_list, *pt);
		worst |= this;
	}
	if (lenlist(types) == lenlist(bad_list))
		return (worst);
	else
		return (0);
}
