/*	copyright	"%c%"	*/
#ident	"@(#)mkdir:mkdir.c	1.7.13.4"

/***************************************************************************
 * Command: mkdir
 * Inheritable Privileges: P_MULTIDIR,P_SETFLEVEL,P_MACREAD,P_DACREAD,
 *			   P_MACWRITE,P_DACWRITE,P_FSYSRANGE
 *       Fixed Privileges: P_MACUPGRADE
 * Notes: mkdir makes a directory.
 * 	If -m is used with a valid mode, directories will be created in
 *	that mode.  Otherwise, the default mode will be 777 possibly
 *	altered by the process's file mode creation mask.
 *
 * 	If -p is used, make the directory as well as its non-existing
 *	parent directories. 
 *
 * 	-M indicates that the target directory is to be a Multi-Level
 *	Directory (valid only with the Enhanced Security Package).
 *
 * 	-l indicates that the target directory is to be created at a given
 * 	security level (valid only with the Enhanced Security Package).
 *
 *	P_MULTIDIR is needed for creating a Multi-Level Directory
 *	P_SETFLEVEL or P_MACUPGRADE is needed for setting the level on
 *	the directory.
 *	The other privileges may be needed to pass any access checks on
 *	directory path.
 ***************************************************************************/

#include	<signal.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<string.h>
#include	<mac.h>
#include        <deflt.h>
#include	<priv.h>
#include	<stdlib.h>
#include	<pwd.h>
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>
#include 	<ctype.h>	/* for isdigit  */

extern int opterr, optind;
extern char *optarg;

int Mflag, lflag;
level_t level; /*internal format of the level specified in -l option */

static const char
	badmkdir[] = ":328:Cannot create directory \"%s\": %s\n",
	badMkdir[] = ":808:cannot create multilevel directories\n",
	badfilesys[] = ":809:File system for file %s does not support per-file labels\n";

static const char badmode[] = ":14:Invalid mode\n";

#ifdef __STDC__
static mode_t 	newmode(mode_t, char *, int *, mode_t);
static mode_t 	who(void);
static int 	what(void);

static int mkdirp(const char *, mode_t, mode_t);
#else
static mode_t 	newmode();
static mode_t 	who();
static int 	what();

static int mkdirp();
#endif

#define DEFFILE "mac"	/* /etc/default file containing the default LTDB lvl */
#define LTDB_DEFAULT 2  /* default LID for non-readable LTDB */

level_t cmm_lcheck();

/*
 * Procedure: main
 *
 * Restrictions: lvlin: P_MACREAD	mkdirp: <none>
 *		 mkdir(2): <none>	mkmld(2): <none>
 */
main(argc, argv)
int argc;
char *argv[];
{
	int 	pflag, errflg;
	int 	c, saverr = 0;
	mode_t	mode;
	mode_t	parents_mode;
	mode_t	old_mask = 0;
	int	mode_err = 0;
	char 	*p, *l;
	char 	*mode_str = NULL ;
	char 	*endptr;
	char *d;	/* the (path)name of the directory to be created */
	int err;
	void cleandir();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:mkdir");
	errno = 0; /* temporary soln., reset errno in case setlocale fails*/

	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	pflag = errflg = Mflag = lflag = 0;

	while ((c=getopt(argc, argv, "m:pMl:")) != EOF) {
		switch (c) {
		case 'm':
			mode_str = optarg;
			break;
		case 'l':
			lflag++;
			l = optarg;
			level = (level_t) check_lvl (l);
			break;
		case 'M':
			Mflag++;
			break;
		case 'p':
			pflag++;
			break;
		case '?':
			errflg++;
			break;
		}
	}

	argc -= optind;
	if(argc < 1 || errflg) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		if (read_ltdb() < 0)
			pfmt(stderr, MM_ACTION, 
			":329:Usage: mkdir [-m mode] [-p] dirname ...\n");
		else
			pfmt(stderr, MM_ACTION, 
			":810:Usage: mkdir [-m mode] [-p] [-M] [-l level] dirname ...\n");
		exit(2);
	}
	argv = &argv[optind];

	if (pflag || mode_str != NULL) {
		old_mask = umask(0) ;
		mode &= ~old_mask;
		if (pflag)
			parents_mode = mode | (S_IWUSR|S_IXUSR);
		if (mode_str != NULL) {
			mode = newmode(mode | S_IFDIR,
					mode_str, &mode_err, old_mask);
			if (mode_err)
				exit (2) ;
		}
	}

        while(argc--) {
		d = *argv++;
		/* Skip extra slashes at the end of path */
		while ((endptr=strrchr(d, '/')) != NULL){
			p=endptr;
			p++;
			if (*p == '\0') {
				if (endptr != d) 
					*endptr='\0';
				else
					break;
			} else
				break;
		}

		/* 
		 * When -p is set, invokes the local mkdirp().
		 * If -M is set, invokes the mkmldp() instead of mkdirp(). 
		 * This routine performs the same 
		 * function as mkdirp(), but creates an MLD as the 
		 * last component. 
		 */ 

		if (pflag) { 
			/* make parents directory */
			if (Mflag) {
				if ((err = mkmldp(d, 
						parents_mode, mode)) < 0)
					if (errno == ENOSYS)
						pfmt(stderr, MM_ERROR, badfilesys, d);
					else
						pfmt(stderr, MM_ERROR, badMkdir);
			} 
			else {
				/* local mkdirp */
				err = mkdirp(d, parents_mode, mode);
				if (err < 0) {
					pfmt(stderr, MM_ERROR, badmkdir, d,
						strerror(errno));
				} 
			}

		/* No -p. Make only one directory 
		 */

		} else {
			/*
			 * If we are to make an MLD, call mkmld(2) to make
			 * the MLD. Otherwise, call mkdir(2) to make a
			 * regular directory.
			 */
			if (Mflag) 
				err = mkmld(d, mode);
			else
				err = mkdir(d, mode);
			if (err) {
				if (Mflag) {
					if (errno == ENOSYS)
						pfmt(stderr, MM_ERROR, badfilesys, d);
					else
						pfmt(stderr, MM_ERROR, badMkdir);
				} else
					pfmt(stderr, MM_ERROR, badmkdir, d,
						strerror(errno));
			}
		}

		/*
		 * If a level was specified, change the level.
		 * If the level change failed, remove the directory.
		 */
		if (!err && lflag) {
			if ((err = chglevel(d, &level)))
				cleandir(d);
		}
		if (err) {
			saverr = 2;
			errno = 0;
		}

	} /* end while */

	if (saverr)
		errno = saverr;

        exit(errno ? 2: 0);
	/* NOTREACHED */
} /* main() */

/*
 * Procedure:     check_lvl
 *
 * Restrictions:
 *		lvlin: none
 *		lvlproc: none
 *		defopen: none
 *	        defclose: none
 *		defread: none
 *
 *
 * Inputs: Level of directory to be created.
 * Returns: 0 on success. On failure the appropriate error message is
 *          displayed and the process is terminated.
 *
 *
 * Notes: Attempt to read the LTDB at the current process level, if the
 *        call to lvlin fails, get the level of the LTDB (via defread),
 *        change the process level to that of the LTDB and try again.
 *   
 *        If the lvlin or lvlproc calls fail we assume MAC is not installed
 *        and exit with a nonzero status. 
 *
 */
check_lvl (dir_lvl)
char *dir_lvl;
{
	level_t test_lid, ltdb_lid, curr_lvl;

	ltdb_lid = cmm_lcheck(dir_lvl);

	/*
 	*	- Get the current level of the process
 	*	- Set p_setplevel privilege
 	*	- Change process level to the LTDB's level
 	*	- Try lvlin again, read of LTDB should succeed since process
	*	 level is equal to level of the LTDB
 	*
 	*	Failure to obtain the process level or read the LTDB is a
 	*       fatal error.
	*/
	if (ltdb_lid <= 0) {
		pfmt(stderr, MM_ERROR, ":812:LTDB is inaccessible\n");
		exit(5);
	}

	if (lvlproc (MAC_GET, &curr_lvl) == -1 && errno == ENOPKG) {
		pfmt(stderr, MM_ERROR, ":794:System service not installed\n");
		exit(3);
	}

	(void) procprivl (SETPRV, SETPLEVEL_W, 0);
	(void) lvlproc (MAC_SET, &ltdb_lid);

	if (lvlin (dir_lvl, &test_lid) == -1)  {
		if (errno == EINVAL) {
			pfmt(stderr, MM_ERROR, ":811:specified level is not defined\n");
			exit(4);
		}
		else {
			pfmt(stderr, MM_ERROR, ":812:LTDB is inaccessible\n");
			exit(5);
		}
	}

	(void) lvlproc (MAC_SET, &curr_lvl);
	(void) procprivl (CLRPRV, SETPLEVEL_W, 0);

	return (test_lid);

}
/*
 * Procedure:     chglevel
 *
 * Restrictions: lvlfile(2): <none>
 */
	
int
chglevel(dirp, levelp)
char *dirp;
level_t *levelp;
{
	int changed_mode = 0;	/*flag to tell if we changed MLD mode*/
	int curr_mode;		/* holds return from MLD_QUERY */
	int err;

	if (Mflag) {
		if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
			if (mldmode(MLD_REAL) < 0) {
				pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				      strerror(errno));
				exit(4);
			}
			changed_mode = 1;
		} else if (curr_mode < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	} /* if Mflag */
	err = lvlfile(dirp, MAC_SET, levelp);
	if (err) {
		if (errno == ENOSYS)
			pfmt(stderr, MM_ERROR, badfilesys, dirp);
		else
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
	}
	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	}

	return err;

} /* chglevel() */

/*
 * Procedure: cleandir() - removes directory.  
 *
 * Notes: If the directory is a multi-level directory, then it should
 *	  be removed in real mode. 
 *
 * Restrictions: rmdir(2): <none>
 */
void
cleandir(d)
char *d;
{
	int changed_mode = 0;
	int curr_mode;

	if (Mflag) {    /* just created a multi-level directory */
		if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
			if (mldmode(MLD_REAL) < 0) {
				pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				      strerror(errno));
				exit(4);
			}
			changed_mode = 1;
		} else if (curr_mode < 0 && errno != ENOPKG) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	} /* if Mflag */

        (void) rmdir(d);     

	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	}
} /* cleandir() */

/*
 * Procedure: mkmldp - creates an MLD and it's parents if the parents
 *		       do not exist yet.
 *
 * Restrictions: mkdir(2): <none>	mkmld(2): <none>
 *               access(2): <none>
 *
 * Notes: Returns -1 if fails for reasons other than non-existing parents.
 * 	  Does NOT compress pathnames with . or .. in them.
 */

static char *compress();
void free();

int
mkmldp(d, parents_mode,  mode)
const char *d;
mode_t parents_mode;		/* parents directory mode */
mode_t mode;			/* target directory mode */
{
	char  *endptr, *ptr, *slash, *str, *lastcomp;

	/*
	 * Remove extra slashes from pathname.
	 */

	str=compress(d);

	/* If space couldn't be allocated for the compressed names, return. */

	if ( str == NULL )
		return(-1);

	ptr = str;

        /* Try to make the directory */
	if (mkmld(str, mode) == 0){
		free(str);
		return(0);
	}
	if (errno != ENOENT) {
		free(str);
		return(-1);
	}
	endptr = strrchr(str, '\0');
	ptr = endptr;
	slash = strrchr(str, '/');
	lastcomp = slash;

		/* Search upward for the non-existing parent */

	while (slash != NULL) {

		ptr = slash;
		*ptr = '\0';

			/* If reached an existing parent, break */

		if (access(str, 00) ==0)
			break;

			/* If non-existing parent*/

		else {
			slash = strrchr(str,'/');

				/* If under / or current directory, make it. */

			if (slash  == NULL || slash== str) {
				if (mkdir(str, parents_mode)) {
					free(str);
					return(-1);
				}
				break;
			}
		}
	}
	/* Create directories starting from upmost non-existing parent*/

	while ((ptr = strchr(str, '\0')) != lastcomp){
		*ptr = '/';

		/* Skip existing pathnames like ".","..","d1/../d1" */
		if (access(str, 0) == 0)
			continue;

		if (mkdir(str, parents_mode)) {
			free(str);
			return(-1);
		}
	}
	*lastcomp = '/';

	/*
	 * If the final component was already created as a result
	 * of being part of the parent path (e.g. "d1/../d1"), then
	 * rmdir the last component, before recreating it as an MLD.
	 */

	if ((!access(str, 0) && rmdir(str)) || mkmld(str,mode)) {
		free(str);
		return -1;
	}
	
	free(str);
	errno = 0;
	return 0;
} /* mkmldp() */

/*
 * common function to check level.
 */
level_t
cmm_lcheck(dir_lvl)
char *dir_lvl;
{
	FILE    *def_fp;
	static char *ltdb_deflvl;
	level_t test_lid, ltdb_lid;


/*
 *	If lvlin is successful the LTDB is readable at the current process
 *	level, no other processing is required, return 0....
*/

	if (lvlin (dir_lvl, &test_lid) == 0)  
		return (0);
	else
		if (errno == EINVAL) { /* Invalid level w/-l option */
			pfmt(stderr, MM_ERROR, ":811:specified level is not defined\n"); 
			exit(4);
		}
/*
 *	If the LTDB was unreadable:
 *	- Get the LTDB's level (via defread)
 */

	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if (ltdb_deflvl = defread(def_fp, "LTDB"))
			ltdb_lid = (level_t) atol (ltdb_deflvl);
		(void) defclose(NULL);
	}
	
	if ((def_fp == NULL) || !(ltdb_deflvl))
		ltdb_lid = (level_t) LTDB_DEFAULT;
	return(ltdb_lid);
}


/*
 * Procedure:     read_ltdb
 *
 * Restrictions:
 *		lvlin: none
 *		defopen: none
 *
 *
 * Inputs: None
 * Returns: 0 on success w/o changing levels, > 0 if the LTDB is at a level
 *          which is not dominated by this process, but was readable after
 *          lvlproc () call and -1 on failure. 
 *          Levels of LTDB and the current process
 *	    are set here. These are used later when the per-file LIDs are
 *	    translated into text levels.
 *
 *         
 *
 * Notes: Attempt to read the LTDB at the current process level, if the
 *        call to lvlin fails, get the level of the LTDB (via defread),
 *        change the process level to that of the LTDB and try again.
 *   
 *        If the lvlin or lvlproc calls fail we assume MAC is not installed
 *        and exit with a nonzero status. If the call succeeds return the
 *        level of the LTDB for later use.
 *
 */
read_ltdb ()
{

	level_t test_lid, ltdb_lid, curr_lvl;
	
	ltdb_lid = cmm_lcheck("SYS_PRIVATE");
	/*
 	*	- Get the current level of the process
 	*	- Set p_setplevel privilege
 	*	- Change process level to the LTDB's level
 	*	- Try lvlin again, read of LTDB should succeed since process
	*	 level is equal to level of the LTDB
 	*
 	*	Failure to obtain the process level or read the LTDB is a
 	*       fatal error, return -1.
	*/

	if (ltdb_lid <= 0)
		return (-1);

	if (lvlproc (MAC_GET, &curr_lvl) != 0)
		return (-1);

	lvlproc (MAC_SET, &ltdb_lid);


	if (lvlin ("SYS_PRIVATE", &test_lid) != 0) 
		return (-1);


	if (lvlproc (MAC_SET, &curr_lvl) != 0)
		return (-1);


	return (ltdb_lid);

}

/* 
 * Procedure: compress
 */

static char *
compress(str)
char *str;
{

	char *tmp;
	char *front;

	tmp=(char *)malloc(strlen(str)+1);
	if ( tmp == NULL )
		return(NULL);
	front = tmp;
	while ( *str != '\0' ) {
		if ( *str == '/' ) {
			*tmp++ = *str++;
			while ( *str == '/' )
				str++;
			continue;
		}
		*tmp++ = *str++;
	}
	*tmp = '\0';
	return(front);
} /* compress() */

/*
 * Procedure: mkdirp - creates an directory and it's parents if the parents
 *		       do not exist yet.
 *
 * Restrictions: mkdir(2): <none> access(2): <none>
 *
 * Notes: Returns -1 if fails for reasons other than non-existing parents.
 * 	  Does NOT compress pathnames with . or .. in them.
 */

int
#ifdef __STDC__
mkdirp(const char *d, mode_t parents_mode, mode_t mode)
#else
mkdirp(d, parents_mode, mode)
const char *d;		/* directory pathname */
mode_t parents_mode ;	/* parents directory mode */
mode_t mode;		/* target directory mode */
#endif 
{
	char  *endptr, *ptr, *slash, *str, *lastcomp;

	/*
	 * Remove extra slashes from pathname.
	 */

	str=compress(d);

	/* If space couldn't be allocated for the compressed names, return. */

	if ( str == NULL )
		return(-1);

	ptr = str;

        /* Try to make the directory */
	if (mkdir(str, mode) == 0){
		free(str);
		return(0);
	}

	if (errno != ENOENT) {
		free(str);
		return(-1);
	}
	endptr = strrchr(str, '\0');
	ptr = endptr;
	slash = strrchr(str, '/');
	lastcomp = slash;

		/* Search upward for the non-existing parent */
	while (slash != NULL) {

		ptr = slash;
		*ptr = '\0';

			/* If reached an existing parent, break */

		if (access(str, 00) ==0)
			break;

			/* If non-existing parent*/

		else {
			slash = strrchr(str,'/');

			/* If under / or current directory, make it. */

			if (slash  == NULL || slash== str) {
				if (mkdir(str, parents_mode)) {
					free(str);
					return(-1);
				}
				break;
			}
		}
	}
	/* Create directories starting from upmost non-existing parent*/

	while ((ptr = strchr(str, '\0')) != lastcomp){
		*ptr = '/';

		/* Skip existing pathnames like ".","..","d1/../d1" */
		if (access(str, 0) == 0 )
			continue;

		if (mkdir(str, parents_mode)) {
			free(str);
			return(-1);
		}
	}
	*lastcomp = '/';

	/*
	 * If the final component was already created as a result
	 * of being part of the parent path (e.g. "d1/../d1"), then
	 * don't remake it here.
	 */

	if (access(str, 0) && mkdir(str, mode)) {
		free(str);
		return -1;
	}

	free(str);
	errno = 0;
	return 0;
} /* mkdirp() */

/*
 * Procedure: newmode
 */ 

#define	USER	00700	/* user's bits */
#define	GROUP	00070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	00777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */

static char *msp;		/* mode string */

static mode_t
#ifdef __STDC__
newmode(mode_t nm, char *ms, int *errmode, mode_t mask)
#else
newmode(nm, ms, errmode, mask)
mode_t  nm;	/* file mode bits*/
char  *ms;	/* file mode string */
int *errmode;
mode_t mask;	/* process file creation mask */
#endif
{
	/* m contains USER|GROUP|OTHER information
	   o contains +|-|= information	
	   b contains rwx(slt) information  */
	mode_t 		m, b;
	mode_t 		m_lost;
		/* umask bit */
	register int 	o;
	char		*errflg;
	register int 	goon;
	mode_t 		om = nm;
		/* save mode for file mode incompatibility return */

	*errmode = 0;
	msp = ms;
	if (isdigit(*msp)) {
		nm = (mode_t)strtol(msp, &errflg, 8) ;
		if (*errflg != '\0') {
			pfmt(stderr, MM_ERROR, badmode) ;
			*errmode = 5 ;
			return(om);
		}
		return(nm & ALL);
	}
	do {
		m = who();
		/* P2D11/4.7.7*/
		/* if who is not specified */
		if(m == 0) {
			m = ALL & ~mask ;
			/*
		 	 * The m_lost is supplementing umask bits
			 * for '=' operation.
			 */
			m_lost = mask ;
		} else
			m_lost = 0 ;
			
		while (o = what()) {
/*		
	this section processes permissions
*/
			b = 0;
			goon = 0;
			switch (*msp) {
			case 'u':
				b = (nm & USER) >> 6;
				goto dup;
			case 'g':
				b = (nm & GROUP) >> 3;
				goto dup;
			case 'o':
				b = (nm & OTHER);
		    dup:
				b &= (READ|WRITE|EXEC);
				b |= (b << 3) | (b << 6);
				msp++;
				goon = 1;
			}
			while (goon == 0) switch (*msp++) {
			case 'r':
				b |= READ;
				continue;
			case 'w':
				b |= WRITE;
				continue;
			case 'X':
				if(!S_ISDIR(om) && ((om & EXEC) == 0))
					continue;
				/* FALLTHRU */

			case 'x':
				b |= EXEC;
				continue;
			case 'l':
				/* ignore LOCK */
				continue;
			case 's':
				/* ignore SETID */
				continue;
			case 't':
				/* ignore STICKY */
				continue;
			default:
				msp--;
				goon = 1;
			}

			b &= m;

			switch (o) {
			case '+':
				/* create new mode */
				nm |= b;
				break;
			case '-':

				/* create new mode */
				nm &= ~b;
				break;
			case '=':

				/* create new mode */
				nm &= ~(m|m_lost);
				nm |= b;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp) {
		pfmt(stderr, MM_ERROR, badmode) ;
		*errmode = 5 ;
		return(om);
	}
	return(nm);
}

static mode_t
#ifdef __STDC__
who(void)
#else
who()
#endif
{
	register mode_t m;

	m = 0;
	for (;; msp++) switch (*msp) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		return m;
	}
}

static int
#ifdef __STDC__
what(void)
#else
what()
#endif
{
	switch (*msp) {
	case '+':
	case '-':
	case '=':
		return *msp++;
	}
	return(0);
}


