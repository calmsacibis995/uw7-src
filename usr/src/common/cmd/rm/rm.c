/*	copyright	"%c%"	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)rm:rm.c	1.21.5.8"

/***************************************************************************
 * Command: rm
 *
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACREAD,
 *			   P_DACWRITE,P_COMPAT,P_FILESYS
 *       Fixed Privileges: None
 *
 * Notes: rm [-firR] file ...
 *
 *
 ***************************************************************************/



#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<limits.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<priv.h>
#include	<memory.h>	/* for memset */
#include	<stdlib.h>	/* for getcwd, malloc  etc... */
#include	<nl_types.h>	/* for nl_langinfo */
#include	<langinfo.h>	/* for nl_langinfo, YESSTR/NOSTR */
#ifdef __STDC__
#include	<stdarg.h>
#else
#include	<varargs.h>
#endif
#include	<regex.h>

#define ARGCNT		5		/* Number of arguments */
#define CHILD		0	 
#define	DIRECTORY	((buffer.st_mode&S_IFMT) == S_IFDIR)
#define	SYMLINK		((buffer.st_mode&S_IFMT) == S_IFLNK)
#define	FAIL		-1
#define MAXFORK		100		/* Maximum number of forking attempts */
#define MAXFILES        OPEN_MAX  - 2	/* Maximum number of open files */
#define	MAXLEN		DIRBUF-24  	/* Name length (1024) is limited */
				        /* stat(2).  DIRBUF (1048) is defined */
				        /* in dirent.h as the max path length */
#define	NAMESIZE	MAXNAMLEN + 1	/* "/" + (file name size) */
#define TRUE		1
#define FALSE		0
#define	WRITE		02

/* Structure used to record failed deletes during recursion after
  running out of file descriptors and starting to close + reopen 
  directories. It's yet another linked list. */
/* It would be sufficient just to keep name of LAST failure in current 
   directory, except some (hypothetical) file system might decide 
   to have readdir() return directories in another order on a second pass,
   especially if some entries have been deleted (imagine compaction) */
struct	elt {
	struct	elt	*next;
	char		*name;
};

static	int	errcode;
static	int interactive, recursive, silent; /* flags for command line options */
static	int clean;

void	exit();
void	free();
void	*malloc();
unsigned sleep();
int	getopt();
int	access();
int	isatty();
int	rmdir();
int	unlink();
int	chdir();

static	struct elt *in_list();			
static	int	rm();
static	int	dorm();
static	int	undir(char *, char *, dev_t, ino_t, mode_t, int);
static	void	mktarget();
#ifdef __STDC__
static	int	yes(const char *, ...);
int	lstat(const char*, struct stat *);
#else
static	int	yes();
int	lstat();
#endif
static	int	mypath(dev_t, ino_t);

static const char badopen[] = ":4:Cannot open %s: %s\n";

static const char askdir[] = ":1242:rm: Directory %s.";

static const char askmode[] = ":1243:rm: %s: %o mode.";

static const char askfile[] = ":1244:rm: File %s.";

static const char nomem[] = ":312:Out of memory: %s\n";

static const char cdback[] =
		":11:Cannot change back to %s: %s\n";
static const char rmdot[] = 
		":1184:Cannot remove '.' or '..'\n";
static const char pwdfail[] =
		":1185:Cannot determine current directory\n";
static const char badcwd[] =
		":1186:Internal working directory corrupted\n";

#define DOT	"."
#define DOTDOT	".."


struct related_dir {
	dev_t rd_dev ;
	ino_t rd_ino ;
	char  *rd_name;
	struct related_dir *rd_next ;
} ;

static struct related_dir *savedir = NULL;
#ifdef __STDC__
static int rd_chdir(struct related_dir *);
static struct related_dir *rd_getpath(void);
static char *rd_lookup(char *, dev_t, ino_t);
static void rd_free(struct related_dir *);
#else
static int rd_chdir();
static struct related_dir *rd_getpath();
static char *rd_lookup();
static void rd_free();
#endif 

/*
 * Procedure:     main
 *
 * Restrictions:
 *                setlocale: none
 *                getopt: none
 *                pfmt: none
 */

main(argc, argv)
	int	argc;
	char	*argv[];
{
	extern int	optind;
	int	errflg = 0;
	int	c;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:rm");

	while ((c = getopt(argc, argv, "friCR")) != EOF)
		switch (c) {
		case 'f':
			interactive = FALSE;
			silent = TRUE;
			break;
		case 'i':
			silent = FALSE;
			interactive = TRUE;
			break;
		case 'R':
		case 'r':
			recursive = TRUE;
			break;
		case 'C':
			clean = TRUE;
			break;
		case '?':
			errflg = 1;
			break;
		}

	/* 
	 * For BSD compatibility allow '-' to delimit the end 
	 * of options unless -- was also specified.
	 */
	if ( optind < argc &&
	     (optind <= 1 || strcmp(argv[optind-1], "--") != 0) &&
	     strcmp(argv[optind], "-") == 0)
		optind++;

	argc -= optind;
	argv = &argv[optind];
	
	if (argc < 1 || errflg) {
		if (!errflg)
			(void)pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		(void)pfmt(stderr, MM_ACTION,
			":1187:Usage: rm [-fiRr] file ...\n");
		exit(2);
	}

	while (argc-- > 0) {
		(void) rm (*argv);
		argv++;
	}

	exit(errcode ? 2 : 0);
	/* NOTREACHED */
}

/*
 * Procedure:     rm
 */

static int once = 0;

static int
rm(path)
	char	*path;
{
	register int c ;
	char *basedir;
	char *target;
	struct stat buffer;

	/* 
	 * While target has trailing  '/'
	 * remove them (unless only "/")
	 */
	while (((c = strlen(path)) > 1)
	    &&  (path[c-1] == '/'))
		 path[c-1]=NULL;

	if ((int)strlen(path) >= PATH_MAX) {
		pfmt(stderr, MM_ERROR,
			":568:Path name too long: %s\n", path);
		errcode ++;
		return 0;
	}

	if (lstat(path, &buffer) == FAIL) {
		if (!silent) {	
			pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
				path, strerror(errno));
			++errcode;
		}
		return 0;
	}
	
	/*
	 * If it's a directory, remove its contents.
	 */
	if (DIRECTORY) {
		/*
		 * If "-r" wasn't specified, trying to remove directories
		 * is an error.
		 */
		if (!recursive) {
			pfmt(stderr, MM_ERROR,
				":437:%s not removed: directory\n", path);
			++errcode;
			return 0;
		}
		mktarget(path, &basedir, &target) ;

		/* Do not remove directory '.' or '..' */
		if ((strcmp(target, DOT) == 0)
			|| (strcmp(target, DOTDOT) == 0)) {
			pfmt(stderr, MM_ERROR, rmdot, path) ;
			++errcode;
			if (basedir)
				free(basedir) ;
			return 0;
		}

		if (once == 0) {
			once ++ ;
			if ((savedir = rd_getpath()) == NULL) {
				pfmt(stderr, MM_ERROR, pwdfail) ;
			    	exit(3) ;
			}
		}
		if (basedir) {
			if (chdir(basedir) < 0) {
				pfmt(stderr, MM_ERROR,
					":5:Cannot access %s: %s\n",
					path, strerror(errno));
				errcode ++;
				return 0;
			}
		}
		undir(path, target, buffer.st_dev, buffer.st_ino, 
							buffer.st_mode,clean);
		if (rd_chdir(savedir) < 0) {
			pfmt(stderr, MM_ERROR, badcwd);
			exit(2) ;
		}
		if (basedir)
			free(basedir) ;
		return 0;
	}
	return dorm(path, path);
}

/*
 * Procedure:     dorm
 *
 * Restrictions:
 *                lstat(2): none
 *                pfmt: none
 *                strerror: none
 *                access(2): none
 *                isatty: none
 *                unlink(2): none
 * Returns 1 if file is deleted, else 0.
 */

static int
dorm(path, target)
	char	*path;
	char	*target;
{
	struct stat buffer;

	/* 
	 * Check file to see if it exists.
	 */

	if (lstat(target, &buffer) == FAIL) {
		if (!silent) {
			pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
				path, strerror(errno));
			++errcode;
		}
		return 0;
	}
	
	/*
	 * If it's a directory, remove its contents.
	 */
	if (DIRECTORY) {
		return undir(path, target, buffer.st_dev, buffer.st_ino,
							buffer.st_mode,0);
	}
	
	/*
	 * If interactive, ask for acknowledgement.
	 */
	if (interactive) {
		if (!yes(askfile, path))
			return 0;
	} else if (!silent) {
		/* 
		 * If not silent, and stdin is a terminal, and there's
		 * no write access, and the file isn't a symblic link,
		 * ask for permission.
		 */
		if (access(target, WRITE) == FAIL
		   && !SYMLINK) {
		        if (isatty(fileno(stdin))) {
				if (!yes(askmode, path, buffer.st_mode & 0777))
					return 0;
			}
		}
	}

	/*
	 * If the unlink fails, inform the user if interactive or not silent.
	 */
	if (unlink(target) == FAIL){
		if (!silent || interactive)
			pfmt(stderr,MM_ERROR,":436:%s not removed: %s.\n",path, strerror(errno));
		++errcode;
		return 0;
	}
	return 1;
}


/*
 * Procedure:     undir
 *
 * Restrictions:
 *                pfmt: none
 *                opendir: none
 *                strerror: none
 *                sprintf: none
 *                rmdir(2): none
 * Returns 1 if dir is deleted, else 0.
 */

static int
undir(path, target, dev, ino, mode, cflg)
	char	*path;
	char	*target;
	dev_t	dev;
	ino_t	ino;
	mode_t	mode;
	int	cflg;
{
	char	*newpath;
	char 	*newtarget ;
	DIR	*name;
	struct dirent *direct;
	struct	elt	*fail=NULL, *tail=NULL, *ep;
	char	*ltarget;

	/*
	 * If interactive and this file isn't in the path of the
	 * current working directory, ask for acknowledgement.
	 */

	if (interactive && !mypath(dev, ino)) {
		if (!yes(askdir, path))
			return 0;
	} else if (!silent) {
		/* 
		 * If not silent, stdin is a terminal and there's
		 * no write access, ask for permission.
		 */
		if (access(target, WRITE) == FAIL) {
		        if (isatty(fileno(stdin))) {
				if (!yes(askmode, path, mode & 0777))
					return 0;
			}
		}
	}

	/*
	 * If interactive, need to ask once more,
	 * so don't try to rmdir() here.
	 */
	if (!interactive && !cflg && rmdir(target) == 0) {
		return 1;
	}

	if (chdir(target) < 0) {
		pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
			path, strerror(errno));
		errcode ++;
		return 0;
	}

	if ((name = (DIR *) opendir(".")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, path, strerror(errno));	
		errcode ++;
		if (chdir(DOTDOT) < 0) {
			pfmt(stderr, MM_ERROR, cdback,
					DOTDOT, strerror(errno));
			exit(2) ;
		}
		return 0;
	}
	
	/*
	 * Read every directory entry.
	 */
	while ((direct = readdir(name)) != NULL) {
		/*
		 * Ignore "." and ".." entries.
 		 */
		if(!strcmp(direct->d_name, ".")
		  || !strcmp(direct->d_name, ".."))
			continue;

		/* Skip already failed entries, if any */
		if (in_list(fail, direct->d_name))
			continue;
			
		/*
		 * Try to remove the file.
		 */
		newpath = (char *)malloc((strlen(path)
				+ strlen(direct->d_name) + 2));

		if (newpath == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			exit(1);
		}
		
		(void) sprintf(newpath, "%s/%s", path, direct->d_name);

		newtarget = strrchr(newpath, '/') +1 ;
 
		/*
		 * If a spare file descriptor is available, just call the
		 * "dorm" function with the file name; otherwise close the
		 * directory and reopen it when the child is removed.
		 */
		if (name->dd_fd >= MAXFILES) {
			ltarget = strdup(direct->d_name);
			if (!ltarget) {
				pfmt(stderr, MM_ERROR, nomem, 
					strerror(errno));
				exit(1);
			}
			closedir(name);
			if (!dorm(newpath, newtarget)) {
				/* Failure means we need to remember */
				ep = malloc(sizeof(struct elt));
				if (!ep) {
					pfmt(stderr, MM_ERROR, nomem, 
						strerror(errno));
					exit(1);
				}
				if (tail)
					tail->next = ep;
				else
					fail = tail = ep;
				ep->next = NULL;
				ep->name = ltarget;
			} else {
				free(ltarget);
			}
			if ((name = (DIR *)
				opendir(".")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, path,
					strerror(errno));
				errcode ++;
				free(newpath);
				return 0;
			}
		} else
			(void) dorm(newpath, newtarget);
 
		free(newpath);
	}

	/*
	 * Close the directory we just finished reading.
	 */
	(void) closedir(name);

	while (fail) {
		ep = fail;
		fail = ep->next;
		free(ep->name);
		free(ep);
	}
	
	if (chdir(DOTDOT) < 0) {
		pfmt(stderr, MM_ERROR, cdback, DOTDOT, strerror(errno));
		exit(2) ;
	}

	/*
	 * The contents of the directory have been removed.  If the
	 * directory itself is in the path of the current working
	 * directory, don't try to remove it.
	 * When the directory itself is the current working directory, mypath()
	 * has a return code == 2.
	 */
	switch (mypath(dev, ino)) {
	case 2:
		pfmt(stderr, MM_ERROR,
			":439:Cannot remove any directory in the path\n\tof the current working directory\n\t%s\n",path);
		++errcode;
		return 0;
	case 1:
		return 0;
	case 0:
		break;
	}
	if (cflg)
		return 1;
	
	/*
	 * If interactive, ask for acknowledgement.
	 */
	if (interactive) {
		if (!yes(askdir, path))
			return 0;
	}
	if (rmdir(target) == FAIL) {
		pfmt(stderr, MM_ERROR, ":440:Cannot remove directory %s: %s\n",
			path, strerror(errno));
		++errcode;
		return 0;
	}
	return 1;
}

/* Check if an element is in a list */
static	struct elt	*in_list(list, name) 
	struct	elt	*list;
	char	*name;
{
	while (list) {
		if (strcmp(list->name, name) == 0) 
			return list;
		list = list->next;
	}
	return NULL;
}

/*
 * Procedure:     yes
 *
 * Restrictions:
 *                 fgets: none
 */

static int
#ifdef __STDC__
yes(const char *fmt, ...)
#else
yes(va_alist)va_dcl
#endif
{
	static char *yesstr, *nostr;
	static regex_t yesre;
	char resp[MAX_INPUT];
	register size_t len;
	va_list ap;
	int err;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	if (yesstr == 0) {
		yesstr = nl_langinfo(YESSTR);
		nostr = nl_langinfo(NOSTR);
		err = regcomp(&yesre, nl_langinfo(YESEXPR),
			REG_EXTENDED | REG_NOSUB);
		if (err != 0) {
	badre:;
			regerror(err, &yesre, resp, MAX_INPUT);
			pfmt(stderr, MM_ERROR, ":1234:RE failure: %s\n", resp);
			exit(2);
		}
	}
	vpfmt(stderr, MM_NOSTD, fmt, ap);
	pfmt(stderr, MM_NOSTD, ":1245: Remove (%s/%s)? ", yesstr, nostr);
	va_end(ap);
	resp[0] = '\0';
	(void)fgets(resp, MAX_INPUT, stdin);
	len = strlen(resp);
	if (len && resp[len - 1] == '\n')
		resp[--len] = '\0';
	if (len) {
		err = regexec(&yesre, resp, (size_t)0, (regmatch_t *)0, 0);
		if (err == 0)
			return 1;
		if (err != REG_NOMATCH)
			goto badre;
	}
	return 0;
}


/*
 * Procedure:     mypath
 *
 * Restrictions:
 *               pfmt: none
 *               strerror: none
 *               lstat(2): none
 */

static int
mypath(dev, ino)
	dev_t	dev;
	ino_t	ino;
{
	struct related_dir *rd ;
	/*
	 * If we find a match, the directory (dev, ino) passed to mypath()
	 * is an ancestor of ours. Indicated by return 1;
	 *
	 * If (rd->rd_next == NULL) the 
	 * directory (dev, ino) passed to mypath() is our
	 * current working directory. Indicated by return 2;
	 * 
	 */
	for (rd = savedir ; rd != NULL ; rd = rd->rd_next)
		if (rd->rd_dev == dev && rd->rd_ino == ino)
			return((rd->rd_next == NULL) ? 2:1) ;
	return (0) ;
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

	tmp = (char *)malloc(strlen(str)+1);
	if (tmp == NULL ) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		exit(1);
	}
	front = tmp;
	while ( *str != '\0' ) {
		if ( *str == '/' ) {
			*tmp++ = *str++;
			while ( *str != '\0' && *str == '/' )
				str++;
		} else
			*tmp++ = *str++;
	}
	*tmp = '\0';
	return(front);
} /* compress() */


/*
 * mktarget
 */

static void
mktarget(path, basedir, target)
char *path ;
char **basedir ;
char **target ;
{
	register char *p, *d;
	p = compress(path) ;

	if ((d = strrchr(p, '/')) != NULL) {
		if (d == p ) {
			if (strlen(p) == 1)
				*target = path ;
			else
				*target = strrchr(path, '/') + 1 ;
			*(d+1) = '\0';
		} else {
			*target = strrchr(path, '/') + 1 ;
			*d = '\0' ;
		}
		*basedir = p;
	} else {
		*target = path ;
		*basedir = NULL ;
		free(p) ;
	}
}

/*
 * Procedure:     rd_getpath
 */

static struct related_dir *
#ifdef __STDC__
rd_getpath(void)
#else
rd_getpath()
#endif 
{
	struct stat buffer;
	char	path[PATH_MAX];
	char	*name;
	int	needback = 0;
	struct related_dir *rd ;
	struct related_dir *rd_root ;
	int	i, j;

	rd_root = (struct related_dir *)
			malloc(sizeof(struct related_dir));
	if (rd_root == NULL)
		return(NULL) ;

	if (lstat("/", &buffer)	< 0) {
		free(rd_root);
		return(NULL);
	}

	rd_root->rd_dev = buffer.st_dev;
	rd_root->rd_ino = buffer.st_ino;
	rd_root->rd_next = NULL;
	rd_root->rd_name = "/";

	for (i = 1; ; i++) {
		/*
		 * Starting from ".", walk toward the root, looking at
		 * each directory along the way.
		 */

	 	if ((3 * (uint)i) >= (PATH_MAX - (MAXNAMLEN + 5))) {
			if (chdir(path) < 0)
				goto err_ret;
			i=2;
			needback = 1;
		}

		(void) strcpy(path, ".");
		for (j = 1; j < i; j++)
			if (j == 1)
				(void) strcpy(path, "..");
			else
				(void) strcat(path,"/..");

		if (lstat(path, &buffer) < 0)
			goto err_ret;

		if (buffer.st_dev == rd_root->rd_dev
				&& buffer.st_ino == rd_root->rd_ino) {
			break;
		}
		
		if ((name = rd_lookup(path,
				buffer.st_dev, buffer.st_ino)) == NULL){
			goto err_ret;
		}

		rd = (struct related_dir *)
				malloc(sizeof(struct related_dir));
		rd->rd_dev  = buffer.st_dev;
		rd->rd_ino  = buffer.st_ino;
		rd->rd_name = name;
		rd->rd_next = rd_root->rd_next ;
		rd_root->rd_next = rd ;
	}
	if (needback)
		if (rd_chdir(rd_root) < 0)
			goto err_ret ;
	return (rd_root);
err_ret:
	rd_free(rd_root);
	return (NULL);
}


/*
 * Procedure:     rd_lookup
 */

static char *
#ifdef __STDC__
rd_lookup(char *path, dev_t dev, ino_t ino)
#else
rd_lookup(path, dev, ino)
char *path ;
dev_t dev;
ino_t ino;
#endif
{
	char	myname[PATH_MAX];
	char 	*namep;
	char 	*dirname = NULL;
	DIR	*dp;
	struct dirent *direct;
	struct stat buffer;
	int   statreq;

	(void) strcpy(myname, path) ;
	(void) strcat(myname, "/..") ;
	namep = &myname[strlen(myname)];

	if ((dp = (DIR *) opendir(myname)) == NULL) {
		free(myname) ;
		return(NULL);
	}
	if (fstat(dp->dd_fd, &buffer) < 0) {
		free(myname) ;
		(void) closedir(dp) ;
		return(NULL);
	}
	statreq = (buffer.st_dev != dev);

	while ((direct = readdir(dp)) != NULL) {
		if(!strcmp(direct->d_name, ".")
		  || !strcmp(direct->d_name, ".."))
			continue;
		if (statreq) {
			(void) strcpy(namep, "/") ;
			(void) strcat(namep, direct->d_name) ;

			if (stat(myname, &buffer) < 0)
				continue;

			if (buffer.st_dev == dev
					&& buffer.st_ino == ino) {
				dirname = strdup(direct->d_name) ;
				break;
			}
		} else if (direct->d_ino == ino) {
			dirname = strdup(direct->d_name) ;
			break;
		}
	}
	(void) closedir(dp) ;
	return(dirname);
}


/*
 * Procedure:     rd_chdir
 */

static int
#ifdef __STDC__
rd_chdir(struct related_dir *rd_root)
#else
rd_chdir(rd_root)
struct related_dir *rd_root ;
#endif
{
	char jumpbuf[PATH_MAX];
	register int  pathlen = 0;
	register int  namelen;
	struct related_dir *rd ;

	(void) memset(jumpbuf, '\0', PATH_MAX);
	for (rd = rd_root ; ; rd = rd->rd_next) {
		namelen = strlen(rd->rd_name);

		if ((pathlen + namelen + 1) >= PATH_MAX) {
			if (chdir(jumpbuf) < 0)
				break;
			pathlen = 0;
			(void) memset(jumpbuf, '\0', PATH_MAX);
		}
		if (pathlen != 0
				&& jumpbuf[pathlen-1] != '/' ) {
			(void) strcat(jumpbuf, "/") ;
			pathlen++;
                }
                (void) strcat(jumpbuf, rd->rd_name) ;
                pathlen += namelen ;

		if (rd->rd_next == NULL) {
			if (chdir(jumpbuf) < 0)
				break;
			return(0);
		}
	}
	return(-1) ;
}

/*
 * Procedure:     rd_free
 */

static void
#ifdef __STDC__
rd_free(struct related_dir *rd_root)
#else
rd_free(rd_root)
struct related_dir *rd_root;
#endif
{
	struct related_dir *rd = rd_root;
	struct related_dir *nextp;
	while(rd) {
		if (rd->rd_name != NULL && rd != rd_root)
			free (rd->rd_name);
		nextp = rd->rd_next;
		free (rd);
		rd = nextp ;
	}
}
