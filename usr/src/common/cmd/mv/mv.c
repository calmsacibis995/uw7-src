/*		copyright	"%c%" 	*/
/********************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#ident	"@(#)mv:mv.c	1.30.14.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/****************************************************************************
 * Command: cp/ln/mv
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACREAD,P_DACWRITE,
			   P_FSYSRANGE,P_COMPAT,P_OWNER,P_FILESYS
 *       Fixed Privileges: None
 * Notes: move files
  
 ***************************************************************************/
/*
 * Combined cp/ln/mv command:
 *	mv file1 file2
 *	mv dir1 dir2
 *	mv file1 ... filen dir1
 */



#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<utime.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/param.h>
#include	<dirent.h>
#include	<stdlib.h>
#include	<acl.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<string.h>
#include	<priv.h>
#include	<sys/mman.h>
#include	<unistd.h>
#include	<sys/statvfs.h>
#include	<sys/fs/vx_ioctl.h>
#include	<limits.h>
#include	<nl_types.h>
#include	<langinfo.h>
#ifdef __STDC__
#include	<stdarg.h>
#else
#include	<varargs.h>
#endif
#include	<regex.h>

#define MAX_LEN_MMAP	(1024 * 1024 * 8)

#define FTYPE(A)	(A.st_mode)
#define FMODE(A)	(A.st_mode)
#define	IDENTICAL(A,B)	(A.st_dev==B.st_dev && A.st_ino==B.st_ino)
#define ISBLK(A)	((A.st_mode & S_IFMT) == S_IFBLK)
#define ISCHR(A)	((A.st_mode & S_IFMT) == S_IFCHR)
#define ISDIR(A)	((A.st_mode & S_IFMT) == S_IFDIR)
#define ISFIFO(A)	((A.st_mode & S_IFMT) == S_IFIFO)
#define ISLNK(A)	((A.st_mode & S_IFMT) == S_IFLNK)
#define ISREG(A)	((A.st_mode & S_IFMT) == S_IFREG)
#define ISDEV(A)	((A.st_mode & S_IFMT) == S_IFCHR || \
                         (A.st_mode & S_IFMT) == S_IFBLK || \
                         (A.st_mode & S_IFMT) == S_IFIFO)

#define BLKSIZE	4096
#define PATHSIZE 1024
#define	DOT	"."
#define	DOTDOT	".."
#define	DELIM	'/'
#define EQ(x,y)	!strcmp(x,y)
#define	FALSE	0
#define ALL_MODEBITS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
/*
 * Since POSIX.1 leaves the treatment of all bits other than
 * (S_IRWXU|S_IRWXG|S_IRWXO) during a creat or open function undefined,
 * we don't need to mask them out to comply.
 */
#define MODEMASK     ALL_MODEBITS
#define MODEBITS     (MODEMASK & ~cmask)
#define TRUE 1

static	mode_t cmask;		/* umask value */
char	*dname();
extern  char *optarg;
extern	int optind, opterr;
struct stat s1, s2, s3;
char	*earg = "warn";
int	cpy = FALSE;	
int	mve = FALSE;	
int	lnk = FALSE;	
char	*cmd;
int	silent = 0;
int	iflg = 0;
int	nflg = 0;
int	pflg = 0;
int	Rflg = 0;
int	rflg = 0;
int	sflg = 0;
int	targetexists = 0;
int 	recursive = 0;
#ifdef __STDC__
int	getresp(const char *, ...);
#endif

static char posix_var[] = "POSIX2";
static int posix;

static const char
	isdir[] = ":331:<%s> is a directory\n",
	badstat[] = ":5:Cannot access %s: %s\n",
	errmsg[] = ":12:%s: %s\n",
	badunlink[] = ":332:Cannot unlink %s: %s\n",
	badcreate[] = ":148:Cannot create %s: %s\n",
	badwrite[] = ":333:Write error in %s: %s\n",
	partwrite[] = ":1080:Cannot complete write to %s\n",
	nomem[] = ":334:Not enough memory to %s %s %s: %s\n",
	badchown[] = ":1177:User and Group id for \"%s\" couldn't be reset\n",
	badutime[] =
	  ":1012:Access and modification times for \"%s\" couldn't be reset\n",
	badchmod[] = ":1178:File mode for \"%s\" couldn't be reset\n",
	askwrite[] = ":1238:Overwrite %s",
	askremove[] = ":1239:Existing %s: %o mode. Remove";

/*
 * Following are the comments on the changed code for ACL.
 *
 * 1. cp
 *	If target does not exist or the -p option is specified, the
 *	ACL and permission bits of the target will be that of the source;
 *	otherwise, the existing target retains its ACL and permission bits.
 *
 * 2. mv
 *	If target is on another file system, the ACL and permission
 *	bits of the source must be transferred to that of the target.
 *
 */	

#define	NENTRIES	128		/* initial number of entries */
struct acl aclbuf[NENTRIES];		/* initial number of entries allocated */
static struct acl *aclbufp = aclbuf;	/* pointer to allocated buffer */
static int Nentries = NENTRIES;		/* max entries allowed for buffer */
static int nentries;			/* actual number of entries */
static int aclpkg = 1;			/* enhanced security package toggle */

/*
 * TRANSFERDAC will transfer the discretionary access control information
 * of the source to the target.
 *
 * Notes:
 *	1. perform only if the  enhanced security package is installed.
 *	2. double allocated buffer size if not large enough; in most
 *	   cases a system call is saved (that of getting the count of ACL entries);
 *	   if not enough memory, skip ACL transfer for this entry.
 *	3. don't take any action on failure from acl call.
 */


#define	TRANSFERDAC(source, target) { \
	int aclerr = 0; \
	if (aclpkg) { \
		while ((nentries = acl(source, ACL_GET, Nentries, aclbufp)) \
			== -1) { \
			struct acl *tmpbufp; \
			int tmpentries; \
			if (errno != ENOSPC) \
				break; \
			tmpentries = Nentries * 2; \
			if ((tmpbufp = (struct acl *)malloc(tmpentries * \
				sizeof(struct acl))) == (struct acl *)NULL) { \
				pfmt(stderr, MM_ERROR,\
				":814:Not enough memory to get ACL for %s\n", \
				source); \
				break; \
			} \
			if (aclbufp != aclbuf) \
				free(aclbufp); \
			aclbufp = tmpbufp; \
			Nentries = tmpentries; \
		} /* end while */ \
		if (nentries != -1) { \
			aclerr = acl(target, ACL_SET, nentries, aclbufp); \
		} \
	} /* end "if (aclpkg)" */ }


/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 getopt: 	none
                 pfmt: 		none
                 stat(2): 	none
                 strerror: 	none
                 acl(2): 	none
 */
main(argc, argv)
register char *argv[];
{
	register int c, i, r, errflg = 0;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");

	if (getenv(posix_var) != 0) {
		posix = 1;
	} else {
		posix = 0;
	}

	/*
	 * Determine command invoked (mv, cp, or ln) 
	 */

	if (cmd = strrchr(argv[0], '/'))
		++cmd;
	else
		cmd = argv[0];

	/*
	 * Set flags based on command.
	 */

	  if (EQ(cmd, "mv")){
	  	(void)setlabel("UX:mv");
	  	mve = TRUE;
	  }
	  else if (EQ(cmd, "ln")) {
		(void)setlabel("UX:ln");
		lnk = TRUE;
	  }
	  else {
	  	(void)setlabel("UX:cp");
		cpy = TRUE;   /* default */
	  }

	/*
	 * Check for options:
	 * 	cp [-e ignore|warn|force] [-i] [-p] file1 file2
	 * 	cp [-e ignore|warn|force] [-i] [-p] file1 ... filen dir1
	 * 	cp [-e ignore|warn|force] [-i] [-p] [-r|-R] dir1 dir2
	 * 	ln [-f] [-n] [-s] file1 file2
	 * 	ln [-f] [-n] [-s] file1 ... filen dir1
	 * 	ln [-f] [-n] [-s] dir1 dir2
	 * 	mv [-e ignore|warn|force] [-f] [-i] file1 file2
	 * 	mv [-e ignore|warn|force] [-f] [-i] file1 ... filen dir1
	 * 	mv [-e ignore|warn|force] [-f] [-i] dir1 dir2
	 */

	if (cpy) {
		while ((c = getopt(argc, argv,"e:fiprR")) != EOF)
			switch (c) {
                                case 'e':
					earg = optarg;
					if (!EQ(earg, "warn") &&
					    !EQ(earg, "ignore") &&
					    !EQ(earg, "force"))
						errflg++;
                                        break;
                                case 'f':
                                        silent++;
					iflg = 0;
                                        break;
                                case 'i':
                                        iflg++;
					silent = 0;
                                        break;
                                case 'p':
                                        pflg++;
                                        break;
                                case 'R':
                                        Rflg++;
					recursive++;
                                        break;
                                case 'r':
                                        rflg++;
					recursive++;
                                        break;
                                default:
                                        errflg++;
			}
	}
	else if (mve) {
		while ((c = getopt(argc, argv,"e:fis")) != EOF)
			switch(c) {
                                case 'e':
					earg = optarg;
					if (!EQ(earg, "warn") &&
					    !EQ(earg, "ignore") &&
					    !EQ(earg, "force"))
						errflg++;
                                        break;
                                case 'f':
                                        silent++;
					iflg = 0;
                                        break;
                                case 'i':
                                        iflg++;
					silent = 0;
                                        break;
                                default:
                                        errflg++;
			}
		/* For BSD compatibility allow - to delimit the end of
	 	* options for mv.
	 	*/
		if (optind < argc && 
	   	(optind <= 1 || strcmp(argv[optind-1], "--") != 0) &&
	    	strcmp(argv[optind], "-") == 0)
			optind++;
	}
	else {
		while ((c = getopt(argc, argv,"fns")) != EOF)
			switch(c) {
                                case 'f':
                                        silent++;
					nflg = 0;
                                        break;
				case 'n': 
					nflg++;	
					silent = 0;
					break;
                                case 's':
                                        sflg++;
                                        break;
				default:
                                	errflg++;
			}
	}


	/*
	 * Check for sufficient arguments 
	 * or a usage error.
	 */

	argc -= optind;	 
	argv  = &argv[optind];

	if ((argc < 2 && !errflg) || (cpy && rflg && Rflg)) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		usage();
	}

	if (errflg != 0)
		usage();

	/* set file creation mask  */
	if (mve || cpy)
		cmask = umask(0) ;

	/*
	 * If there is more than a source and target,
	 * the last argument (the target) must be a directory
	 * which really exists.
	 */

	if (argc > 2 || (recursive && posix)) {
		if (stat(argv[argc-1], &s2) < 0) {
			if (!(recursive && argc == 2)) {
				pfmt(stderr, MM_ERROR, 
					badstat, argv[argc-1], strerror(errno));
				exit(2);
			}
		} else if (!ISDIR(s2)) {
			pfmt(stderr,MM_ERROR, ":335:Target %s must be a directory\n",
				argv[argc-1]);
			usage();
		}
	}

	/* 
	 * While target has trailing 
	 * DELIM (/), remove them (unless only "/")
	 */
	while (((c = strlen(argv[argc-1])) > 1)
	    &&  (argv[argc-1][c-1] == DELIM))
		 argv[argc-1][c-1]=NULL;

        /* establish up front if the Enhanced Security package is installed */
        if ((acl("/", ACL_CNT, 0, 0) == -1) && (errno == ENOPKG))
                aclpkg = 0;

	/*
	 * Perform a multiple argument mv|cp|ln by
	 * multiple invocations of move().
	 */

	r = 0;
	for (i=0; i<argc-1; i++)
		r += move(argv[i], argv[argc-1]);

	/* 
	 * Show errors by nonzero exit code.
	 */

	exit(r?2:0);
}

move(source, target)
char *source, *target;
{
	register last;

	/* 
	 * While source has trailing 
	 * DELIM (/), remove them (unless only "/")
	 */

	while (((last = strlen(source)) > 1)
	    &&  (source[last-1] == DELIM))
		 source[last-1]=NULL;

	if (lnk)
		return(lnkfil(source, target));
	else
		return(cpymve(source, target));
}


/*
 * Procedure:     lnkfil
 *
 * Restrictions:
                 stat(2): 	none
                 pfmt: 		none
                 strerror: 	none
                 sprintf: 	none
                 symlink(2): 	none
                 link(2): 	none
 */
lnkfil(source, target)
char *source, *target;
{
	char	*buf = (char *)NULL;

 	if (sflg) {

		/*
		 * If target is a directory make complete
		 * name of the new symbolic link within that
		 * directory.
		 */

 		if ((stat(target, &s2) == 0) && ISDIR(s2)) {
			if ((buf = (char *)malloc(strlen(target) + strlen(dname(source)) + 4)) == NULL) {
				pfmt(stderr, MM_ERROR, nomem, cmd, source,
					target, strerror(errno));
				exit(3);
			}
			sprintf(buf, "%s/%s", target, dname(source));
			target = buf;
 		}


 		if (symlink(source, target) < 0) {
			pfmt(stderr, MM_ERROR, badcreate, target, strerror(errno));
			if (buf != NULL)
				free(buf);
 			return(1);
 		}
		if (buf != NULL)
			free(buf);
 		return(0);
 	}

        /*
         * Make sure source file is not a directory,
	 * we can't link directories...
	 */

	if (lstat(source, &s1) == -1 ) {
		pfmt(stderr, MM_ERROR, badstat, source, strerror(errno));
		return(1);
	}

	if (ISDIR(s1)) {
		pfmt(stderr, MM_ERROR, isdir, source);
		return(1);
	}

        if (chkfiles(source, &target))
		return(1);

        /*
	 * hard link, call link() and return.
	 */

        if(link(source, target) < 0) {
                if(errno == EXDEV)
                	pfmt(stderr, MM_ERROR,
                		":336:%s and %s are on different file systems\n",
                		source, target);
                else
                     	pfmt(stderr, MM_ERROR, ":337:Cannot link %s to %s: %s\n",
                     		source, target, strerror(errno));
                if(buf != NULL)
                        free(buf);
                return(1);
	}
        else {
                if(buf != NULL)
                        free(buf);
                 return(0);
	}
}

/*
 * Procedure:     cpymve
 * actually copy or move a file (even if is a directory)
 *
 * Restrictions:
                 pfmt: 		none
                 stat(2): 	none
                 mkdir(2): 	none
                 strerror: 	none
                 rename(2): 	none
                 unlink(2): 	none
                 readlink(2): 	none
                 symlink(2): 	none
                 mknod(2): 	none
                 access(2): 	none
                 open(2): 	none
                 creat(2): 	none
                 read(2): 	none
                 write(2): 	none
		 fstatvfs(2):	none
		 ioctl(2):	none
 */
cpymve(source, target)
char *source, *target;		/* filenames */
{
 	int n, ret, tmp_err;	/* temporary registers */
	off_t filesize;		/* temporary variable to hold file size */
	int fi, fo;		/* file ids: in & out */
	struct stat spd;        /* stat of source's parent */
	int uid;                /* real uid */
	char *addr1, *addr2;	/* pointers for mmap */
	long Pagesize;		/* the page size for mmap */
	static char buf[8192];	/* this buffer is only used during the
				 * block-at-time copy.  It is static to
				 * save stack space esp. during recursion.
				 */

	struct	statvfs sstatvfs, tstatvfs;
	struct	vx_ext extbuf;
	off_t	extinbytes; 
	daddr_t	resinbytes;
	int	dosetext, noextend;
	struct	stat statb;

	int	attempts = 0;
	int	remainder, nbytes;
	long	for_cnt, i;

        if (n = chkfiles(source, &target))
		return ((n == -1)? 1:0);

        /*
         * If it's a recursive copy and source
	 * is a directory, then call rcopy.
	 */
	if (cpy) {
		mode_t fixmode = (mode_t)0;	/* saved mode & flag */

		if (! ISDIR(s1)) {
			if (Rflg && !ISREG(s1)) {
				if (targetexists && ISDEV(s1) &&
						(access(target, W_OK) < 0)){
					pfmt(stderr, MM_ERROR, errmsg,
						target, strerror(errno));
					return 1;
				}
				goto copy_file_type ;
			}
			else 
				goto copy ;
		}
		/*
		 * here is the special case treatment of copying a directory
		 */
		if (!recursive) {
			pfmt(stderr, MM_ERROR, isdir, source);
			return 1;
		}
		if (pflg)
			statb = s1;
		if (stat(target, &s2) < 0) {
			if (mkdir(target,
				(s1.st_mode & MODEBITS) | S_IRWXU) < 0) {
				pfmt(stderr, MM_ERROR, errmsg, target
					, strerror(errno));
				return 1;
			}
			fixmode = s1.st_mode;
		} else if (!(ISDIR(s2))) {
			pfmt(stderr, MM_ERROR, ":338:%s: Not a directory.\n",
				target);
			return 1;
		} else if (pflg) {
			fixmode = s1.st_mode;
		}
		n = rcopy(source, target);
		if (fixmode) {
			TRANSFERDAC(source, target);
		}
					
		if (pflg) {
			/*
			 * BUG FIX : rcopy causes recursive call of
			 *     move() ; s1 contains now the last
			 *     copied file, we have to reset it
			 */
			if (chmod(target, fixmode & MODEMASK) < 0){
				pfmt(stderr, MM_WARNING,
					badchmod, target);
				if (!mve)
					n++;
			}
			s1 = statb ;
			n += setattr(target);
		} else if (fixmode) {
			if (chmod(target,
				(fixmode & MODEBITS)) < 0){
				pfmt(stderr, MM_WARNING,
					badchmod, target);
				n++ ;
			}
		}
		return n;
		/*
		 * end of special case of cpy of dir
		 */
	}

	if (! mve) {
		/*
		 * this is an internal error condition:
		 * a copy/move operation that is neither
		 * a copy nor a move.
		 */
		return 1;
	}
	if (rename(source, target) >= 0) {
		return 0;
	}
	if (errno != EXDEV) {
		pfmt(stderr, MM_ERROR, ":339:Cannot rename %s to %s: %s\n",
			source, target, strerror(errno)); 
		return 1;
	}
	if (ISDIR(s1)) {
		n = mvdir(source, target);
		return n;
	}

copy_file_type:

	/*
	 * File can't be renamed, try to recreate the symbolic
	 * link or special device, or copy the file wholesale
	 * between file systems.
	 */
	if (ISLNK(s1)) {
		register m;
		char symln[MAXPATHLEN + 1];

		if (targetexists && unlink(target) < 0) {
			pfmt(stderr, MM_ERROR, badunlink, target,
				strerror(errno));
			return 1;
		}
		m = readlink(source, symln, sizeof (symln) - 1);
		if (m < 0) {
			pfmt(stderr, MM_ERROR, errmsg, source,strerror(errno));
			return 1;
		}
		symln[m] = '\0';
		if (symlink(symln, target) < 0) {
			pfmt(stderr, MM_ERROR, errmsg, source,strerror(errno));
			return 1;
		}

		if (mve || (cpy &&  pflg))
			n = setattr(target) ;
		if (cpy)
			return n;
		goto cleanup;
	}
	if (ISDEV(s1)) {
		
		register int m;

		if (targetexists && unlink(target) < 0) {
			pfmt(stderr, MM_ERROR, badunlink, target,
				strerror(errno));
			return 1;
		}

		if (mve || (cpy && pflg))
			m = mknod(target, s1.st_mode, s1.st_rdev);
		else
			m = mknod(target, s1.st_mode & ~cmask,
				s1.st_rdev ) ;
		if (m < 0) {
			pfmt(stderr, MM_ERROR, errmsg, target,
				strerror(errno));
			return 1;
		}
		
		TRANSFERDAC(source, target) ;
		if (mve || (cpy && pflg)) {
			n = setattr(target);
		} else if (aclpkg) {
			if (chmod(target, FMODE(s1) & MODEBITS) < 0){
				pfmt(stderr, MM_WARNING,
						badchmod, target);
				n++;
			}
		}
		if (cpy)
			return n;
		goto cleanup;
	}

	if (! ISREG(s1)) {
		pfmt(stderr, MM_ERROR, ":342:Unknown file type %o\n",
			s1.st_mode);
		return 1;
	}

	/*
	 * at this point we know we have been asked to move a regular
	 * file.
	 */
	if (accs_parent(source, 2, &spd) == -1) {
		goto unlink_fail;
	}

	/*
	 * If sticky bit set on source's parent, then move
	 * only when : superuser, source's owner, parent's
	 * owner, or source is writable. Otherwise, we
	 * won't be able to unlink source later and will be
	 * left with two links and an error exit from mv.
	 */
	if ((spd.st_mode & S_ISVTX) && (s1.st_uid != (uid = getuid()) )
		&& (spd.st_uid != uid) && (access(source, 2) < 0)) {
		pfmt(stderr, MM_ERROR, badunlink, source, strerror(errno));
		return 1;
	}
copy:
	if (targetexists && (mve || lnk || (cpy && silent)) && 
	    unlink(target) < 0) {
		pfmt(stderr, MM_ERROR, badunlink, target, strerror(errno));
		return 1;
	}

	fi = open(source, O_RDONLY|O_LARGEFILE);
	if (fi < 0) {
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			source, strerror(errno));
		return 1;
	}

	if (mve || (cpy && pflg)) {
		fo = creat(target, s1.st_mode & MODEMASK);
	}
	else {
		fo = creat(target, s1.st_mode & MODEBITS);
	}
	if (fo < 0) {
		pfmt(stderr, MM_ERROR, badcreate, target, strerror(errno));
		close(fi);
		return 1;
	}

	/*
	 * If we created a target, set its permissions
	 * to the source before any copying so that any
 	 * partially copied file will have the source's 
	 * permissions (at most) or umask permissions
	 * whichever is the most restrictive.
	 */

	if (!targetexists || pflg || mve) {
		TRANSFERDAC(source, target);
	}
	FTYPE(s2) = S_IFREG;
	/*
	 * check if need to copy attributes
	 */
	dosetext = 0;
	noextend = 0;
	extbuf.ext_size = 0;
	extbuf.a_flags = 0;
	if (!ISREG(s1))
		goto noattr;
	if (fstatvfs(fi, &sstatvfs)) {
		pfmt(stderr,MM_ERROR,
                                  ":1083:fstatvfs() on %s failed: %s\n", source,
				strerror(errno));
		goto failcopy;
	}
	if (fstatvfs(fo, &tstatvfs)) {
		pfmt(stderr,MM_ERROR,
                                  ":1083:fstatvfs() on %s failed: %s\n", target,
				strerror(errno));
		goto failcopy;
	}
	if (EQ(sstatvfs.f_basetype, "vxfs") &&
	    !EQ(earg, "ignore")) {
		if (ioctl(fi, VX_GETEXT, &extbuf)) {
			pfmt(stderr,MM_ERROR,
                                           ":1128:%s: ioctl() %s failed: %s\n",
				source,"getext",strerror(errno));
			goto failcopy;
		}
		if (extbuf.reserve || extbuf.ext_size ||
		    extbuf.a_flags) {
			dosetext = 1;
			extinbytes = (off_t)sstatvfs.f_frsize * 
							extbuf.ext_size;
			resinbytes = (daddr_t)sstatvfs.f_frsize * 
							extbuf.reserve;
		}
	}

	/*
	 * if we need to copy attributes, check to see that
	 * target supports attributes
	 */
	if (dosetext && !EQ(tstatvfs.f_basetype, "vxfs")) {
		dosetext = 0;
		if (EQ(earg, "warn")) {
			pfmt(stderr,MM_WARNING,":1084:Cannot maintain attributes of %s\n",source);
		} else {
			pfmt(stderr,MM_ERROR,":1084:Cannot maintain attributes of %s\n",source);
			goto failcopy;
		}
	}
	/*
	 * if we need to copy attributes, check to see that
	 * target supports required sizes
	 */
	if (dosetext &&
	    (extinbytes % (off_t)tstatvfs.f_frsize ||
	     resinbytes % (daddr_t)tstatvfs.f_frsize)) {
		dosetext = 0;
		if (EQ(earg, "warn")) {
			pfmt(stderr,MM_WARNING,":1084:Cannot maintain attributes of %s\n",source);
		} else {
			pfmt(stderr,MM_ERROR,":1084:Cannot maintain attributes of %s\n",source);
			goto failcopy;
		}
	}

	
	/*
	 * Attempt to duplicate attributes.  If NOEXTEND was set
	 * and the i_size was > the reservation, we have to turn 
	 * it off until after all the writing is done.
	 */
	if (dosetext) {
		extbuf.reserve = (resinbytes / (daddr_t)tstatvfs.f_frsize);
		extbuf.ext_size = (extinbytes / (off_t)tstatvfs.f_frsize);
		if ((extbuf.a_flags & VX_NOEXTEND) &&
		    s1.st_size > resinbytes) {
			noextend++;
			extbuf.a_flags &= ~VX_NOEXTEND;
		}
		n = ioctl(fo, VX_SETEXT, &extbuf);
		if (n) {
			noextend = 0;
			dosetext = 0;
			if (EQ(earg, "warn")) {
				pfmt(stderr, MM_WARNING,
				     ":1084:Cannot maintain attributes of %s\n",
				     source);
			} else {
				pfmt(stderr, MM_ERROR,
				     ":1128:%s: ioctl() %s failed: %s\n", 
				     target, "setext", strerror(errno));
				goto failcopy;
			}
		}
	}
noattr:
	if ( attempts )
		goto do_reads_writes;

	filesize = s1.st_size;
	Pagesize = sysconf(_SC_PAGESIZE);
	/*
	 * try to map the input and write it all in one write.
	 * iff the monolithic write only partially succeeds
	 * is there a problem; total failures fall through to
	 * the old fashioned way; sucesses clear "filesz" to skip OFW.
	 *
	 * If the file is not greater than Pagesize, copy it in the normal
	 * way to work around a bug in mmap() which means it doesn't
	 * update the last accessed time.
	 *
	 * To conserve system resources we use multiple mmap()'s and writes
	 * for input files greater than MAX_MMAP_LEN.
	 * we will use multiple mmap()'s and writes.
	 */
	if ((filesize > Pagesize) && ((addr1= (char *)sbrk(0)) != (char *)-1)) {
		/*
		 * we want a single region of address space so
		 * we compute the address of the beginning of the
		 * next page after the end of our bss (sbrk).
		 * NOTE: it is not safe to call malloc between the
		 * mmap & the munmap.
		 */
		addr1 += Pagesize - 1;
		addr1 = (char *)( (long)addr1 & (~ (Pagesize - 1)));

		for_cnt = (filesize/MAX_LEN_MMAP);
		if ( (remainder=(filesize % MAX_LEN_MMAP)) > 0 )
			for_cnt++;
		for_cnt--;
		for( i=0 ; i <= for_cnt ; i++ )
		{
			if ( (remainder == 0) || (i < for_cnt) )
				nbytes = (size_t) MAX_LEN_MMAP;
			else /* if ( (remainder != 0) && (i == for_cnt) ) */
				nbytes = (size_t) remainder;

			addr2 = mmap(addr1, (size_t) nbytes, PROT_READ,
				MAP_SHARED|MAP_FIXED, fi,
				(off_t)i * MAX_LEN_MMAP);
			if (addr2 != (caddr_t)-1) {
				ret = write(fo, addr2, (size_t) nbytes);
				(void) munmap(addr2, (size_t)nbytes);
				/*
				 * DO NOT SEPARATE THE write & THE munmap
				*/
				if (ret > 0) {
					if (ret != nbytes) {
						pfmt(stderr,MM_ERROR,partwrite,
							target);
						goto failcopy;
					}
				} else {
					/* write failed, make another attempt */
					attempts++;
					goto copy;
				}
			} else {
				/* 
				 * if the first mmap failed, continue on 
				 * otherwise start the copy over and
				 * plan on doing reads & writes
				*/
				if( i == 0 ) 
					goto do_reads_writes;
				else {
					attempts++;
					goto copy;
				}
			}
		}; /* end of for for multiple mmap()'s and write()'s */
		/*
		* success
		*/
		goto goodcopy;
	};

do_reads_writes:
	/*
	 * Attempt to use preallocation to get all the
	 * space that will be needed.  The ext_size and
	 * a_flags elements have already been set if
	 * we're preserving attributes.  Otherwise, they
	 * have been set to 0.
	 */

	if (ISREG(s1) && EQ(tstatvfs.f_basetype, "vxfs") &&
	    s1.st_size > 8192) {
		extbuf.reserve = (daddr_t)
			((s1.st_size + tstatvfs.f_frsize - 1) /
			  (long)tstatvfs.f_frsize);
		extbuf.a_flags |= VX_NORESERVE;
		(void)ioctl(fo, VX_SETEXT, &extbuf);
	}
	for (;;) {
		n = read(fi, buf, sizeof buf);
		if (n == 0) {
			goto goodcopy;
		} else if (n < 0) {
			pfmt(stderr, MM_ERROR, ":341:Read error in %s: %s\n",
				source, strerror(errno));
			break;
		}
		if ((ret = write(fo, buf, n)) != n) {
			if(ret == -1) {
				pfmt(stderr, MM_ERROR, badwrite, 
					target, strerror(errno));
			} else {
				pfmt(stderr, MM_ERROR, partwrite, target);
			}
			break;
		}
	}

failcopy:
	/*
	 * copy has failed; clean up.
	 */
	close(fi);
	close(fo);
	/* don't delete block, char and fifo files. */
	if ((stat(target, &s2) == 0 && !(ISDEV(s2))) && (unlink(target) < 0)) {
		pfmt(stderr, MM_ERROR, badunlink, target, strerror(errno));
	}
	return 1;

goodcopy:
	if (fsync(fo) < 0) {
		pfmt(stderr, MM_ERROR, badwrite, target, strerror(errno));
		goto failcopy;
	}
	/*
	 * If NOEXTEND was on in the source and we've delayed setting it 
	 * in the target we'll have to turn it on now, after the writes.  
	 * Had we turned it on up front, the writing would have failed.  
	 * extbuf should still contain valid info.
	 */
	n = 0;
	if (dosetext && noextend) {
		extbuf.a_flags |= VX_NOEXTEND;
		if (ioctl(fo, VX_SETEXT, &extbuf)) {
			if (EQ(earg, "warn")) {
				pfmt(stderr,MM_WARNING,":1084:Cannot maintain attributes of %s\n",source);
			} else {
				pfmt(stderr,MM_ERROR,":1084:Cannot maintain attributes of %s\n",source);
				goto failcopy;
			}
		}
	}
	close(fi);
	if (mve || (cpy && pflg)) {
		if (targetexists && !mve) {
			if (chmod(target, FMODE(s1) & MODEMASK) < 0){
				pfmt(stderr, MM_WARNING, badchmod, target);
				n++ ;
			}
		}
		n += setattr(target);
	} else if (!targetexists && aclpkg) {
		if (chmod(target, FMODE(s1) & MODEBITS) < 0) {
			pfmt(stderr, MM_WARNING, badchmod, target);
			n++ ;
		}
	}
	if (close(fo) < 0) {
		pfmt(stderr, MM_ERROR, badwrite, target, strerror(errno));
		return 1;
	}
	if (cpy) {
		return n;
	}
cleanup:
	if (unlink(source) < 0) {
		(void) unlink(target);
unlink_fail:
		pfmt(stderr, MM_ERROR, badunlink, source, strerror(errno));
		return 1;
	}
	return 0;
}

/*
 * Procedure:     chkfiles
 *
 * Restrictions:
                 stat(2): 	none
                 lstat(2): 	none
                 pfmt: 		none
                 strerror: 	none
                 sprintf: 	none
                 isatty: 	none
                 access(2): 	none
                 unlink(2): 	none
 */
chkfiles(source, to)
char *source, **to;
{
	char	*buf = (char *)NULL;
	int	(*statf)() = (cpy  && !Rflg) ? stat : lstat;
	int	n;
	char    *target = *to;

        /*
         * Make sure source file exists.
	 */

	if ((*statf)(source, &s1) < 0) {
		pfmt(stderr, MM_ERROR, badstat, source, strerror(errno));
		return(-1);
	}

	/*
	 * If stat fails, then the target doesn't exist,
	 * we will create a new target with default file type of regular.
 	 */	

	FTYPE(s2) = S_IFREG;
	targetexists = 0;
	if ((*statf)(target, &s2) >= 0) {
		if(ISLNK(s2))
			stat(target, &s2);
		/*
		 * If target is a directory,
		 * make complete name of new file
		 * within that directory.
		 */
		if (ISDIR(s2)) {
			if ((buf = (char *)malloc(strlen(target) + strlen(dname(source)) + 4)) == NULL) {
				pfmt(stderr, MM_ERROR, nomem, cmd, source,
					target, strerror(errno));
				exit(3);
			}
			sprintf(buf, "%s/%s", target, dname(source));
			*to = target = buf;
		}

		/*
		 * If filenames for the source and target are
		 * the same and the inodes are the same, it is
		 * an error.
		 */

		if ((*statf)(target, &s2) >= 0) {
			targetexists++;
			if (!silent && IDENTICAL(s1,s2)) {
				pfmt(stderr, MM_ERROR,
					":343:%s and %s are identical\n",
				        source, target);
				if (buf != NULL)
		                        free(buf);
				return(-1);
			}
			if (lnk &&
				( (!posix && nflg && !silent) ||
				(posix && (nflg || !silent)) ) ) {
				pfmt(stderr, MM_ERROR, ":344:%s: File exists\n",
					target);
				return(-1);
			}

			/*
			 * If the user does not have access to
			 * the target and the user invoked the command
			 * with the "-i" option --- ask
			 *  mv: only if it is not silent and stdin is a tty
			 *  cp: always
			 */

			if (iflg && ( posix && !(cpy && ISDIR(s2)) ||
				( !posix &&
				((mve && !silent && isatty(fileno(stdin))) ||
				cpy) ) )) {
				if (getresp(askwrite, target)) {
					if (buf != NULL)
						free(buf);
					return(1);
				}
				return(0) ;
			}

			if (( mve || (!posix && !cpy)) &&
				(access(target, 2) < 0) &&
				!silent && isatty(fileno(stdin))
				&& !ISLNK(s2)) {
				if (getresp(askremove, target,
					FMODE(s2) & ALL_MODEBITS)) {
					if (buf != NULL)
						free(buf);
					return(1);
				}
			}
			if(lnk && unlink(target) < 0) {
				pfmt(stderr, MM_ERROR, badunlink, target,
					strerror(errno));
				return(-1);
			}
		}
	}
	return(0);
}

/*
 * Procedure:     mvdir
 */

mvdir(from, to)
char *from, *to;
{

	int errs = 0;
	struct stat statb;

	if (targetexists && rmdir(to) < 0) {
		pfmt(stderr, MM_ERROR, errmsg, to, strerror(errno));
		return (1);
	}

	if (stat(from, &statb) < 0) {
		pfmt(stderr, MM_ERROR, errmsg, from, strerror(errno));
		return(1);
	}

	if (mkdir(to, (statb.st_mode & MODEBITS) | S_IRWXU) < 0) {
		pfmt(stderr, MM_ERROR, errmsg, to, strerror(errno));
		return(1);
	}

	/*
 	 * invoke "cp -Rp from to"
 	 */

	pflg = cpy = Rflg = recursive = 1;
	errs = rcopy(from, to);
	pflg = cpy = Rflg = recursive = 0;
        TRANSFERDAC(from, to);
	if (chmod(to, FMODE(statb) & MODEMASK) < 0)
		pfmt(stderr, MM_WARNING, badchmod, to);
	if (errs) {
		return(errs) ;
	} else 	{
		/*
 	  	 * invoke "rm -r from "
 	 	 */
		errs = rm(from) ;
	}
	
	s1 =  statb ;
	return (setattr(to) + errs) ;
}


/*
 * Procedure:     rcopy
 *
 * Restrictions:
                 opendir: 	none
                 pfmt: 		none
                 strerror: 	none
                 sprintf: 	none
 */
rcopy(from, to)
char *from, *to;
{
	DIR *fold;
	struct dirent *dp;
	struct stat statb;
	int errs = 0;
	char fromname[MAXPATHLEN + 1];

        fold = opendir(from);
	if (fold == 0 || (pflg && fstat(fold->dd_fd, &statb) < 0)) {
		pfmt(stderr, MM_ERROR, errmsg, from, strerror(errno));
		return (1);
	}
	for (;;) {
		dp = readdir(fold);
		if (dp == 0) {
			(void) closedir(fold);
			return (errs);
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(from)+1+strlen(dp->d_name) >= sizeof fromname - 1) {
			(void) pfmt(stderr, MM_ERROR, ":347:%s/%s: Name too long\n",
                            from, dp->d_name);
			errs++;
			continue;
		}
		(void) sprintf(fromname, "%s/%s", from, dp->d_name);
		errs += move(fromname, to);
	}
}

/*
 * Procedure:     accs_parent
 *
 * Restrictions:
                 pfmt: 		none
                 strerror: 	none
                 access(2): 	none
                 stat(2): 	none
 */

/* In addition to checking write access on parent dir, do a stat on it */
accs_parent(name, amode, sdirp)
register char *name;
register int amode;
register struct stat *sdirp;
{
	register c;
	register char *p, *q;
	char *buf;

	/*
	 * Allocate a buffer for parent.
	 */

	if ((buf = malloc(strlen(name) + 2)) == NULL) {
		pfmt(stderr, MM_ERROR, ":149:Not enough memory: %s\n",
			strerror(errno));
		exit(3);
	}
	p = q = buf;

	/* 
	 * Copy name into buf and set 'q' to point to the last
	 * delimiter within name.
	 */

	while (c = *p++ = *name++)
		if (c == DELIM)
			q = p-1;

	/*
	 * If the name had no '\' or was "\" then leave it alone,
	 * otherwise null the name at the last delimiter.
	 */

	if (q == buf && *q == DELIM)
		q++;
	*q = NULL;

	/*
	 * Find the access of the parent.
	 * If no parent specified, use dot.
	 */

	if ((c = access(buf[0] ? buf : DOT,amode)) == -1) {
	/* No write access to directory : no need to check sticky bit */
		free(buf);
		return(c);
	}

	/*
	 * Stat the parent : needed for sticky bit check.
	 * If stat fails, move() should fail the access, 
	 * since we cannot proceed anyway.
	 */
	c = stat(buf[0] ? buf : DOT, sdirp);
	free(buf);
	return(c);
}

char *
dname(name)
register char *name;
{
	register char *p;

	/* 
	 * Return just the file name given the complete path.
	 * Like basename(1).
	 */

	p = name;

	/*
	 * While there are characters left,
	 * set name to start after last
	 * delimiter.
	 */

	while (*p)
		if (*p++ == DELIM && *p)
			name = p;
	return(name);
}

/*
 * Procedure:     getresp
 *
 * Restrictions:
                 getchar: 	none
 */
int
#ifdef __STDC__
getresp(const char *fmt, ...)
#else
getresp(va_alist)va_dcl
#endif
{
	static const char cmdfmt[] = ":1240:%s: ";
	static const char prompt[] = ":1241: (%s/%s)? ";
	static char *yesstr, *nostr;
	static regex_t yesre;
	char resp[MAX_INPUT];
	size_t len;
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
	pfmt(stderr, MM_NOSTD, cmdfmt, cmd);
	vpfmt(stderr, MM_NOSTD, fmt, ap);
	pfmt(stderr, MM_NOSTD, prompt, yesstr, nostr);
	va_end(ap);
	resp[0] = '\0';
	(void)fgets(resp, MAX_INPUT, stdin);
	len = strlen(resp);
	if (len && resp[len - 1] == '\n')
		resp [--len] = '\0' ;
	if (len) {
		err = regexec(&yesre, resp, (size_t)0, (regmatch_t *)0, 0);
		if (err == 0)
			return 0;
		if (err != REG_NOMATCH)
			goto badre;
	}
	return(1);
}                                        

/*
 * Procedure:     usage
 *
 * Restrictions:
                 pfmt: 	none
 */
usage()
{
	register char *opt;

	/*
	 * Display usage message.
	 */

 	opt = cpy ? " [-e ignore|warn|force] [-f] [-i] [-p]" : lnk ? " [-s] [-f] [-n]" : " [-e ignore|warn|force] [-f] [-i]";
	pfmt(stderr, MM_ACTION,
		":348:Usage: %s%s f1 f2\n                      %s%s f1 ... fn d1\n", 
		cmd, opt, cmd, opt);
	if(mve)
		pfmt(stderr, MM_NOSTD, ":1085:                      mv [-e ignore|warn|force] [-f] [-i] d1 d2\n");
	else if (lnk)
		pfmt(stderr, MM_NOSTD, ":1221:                      ln -s [-f] [-n] d1 d2\n");
	else if (cpy) {
		pfmt(stderr, MM_NOSTD, ":1208:                      cp [-e ignore|warn|force] [-f] [-i] [-p] [-r|-R] d1 d2\n");
	}
	exit(2);
}

/*
 * Procedure:     setattr
 *
 * Restrictions:
                 utime(2): 	none
 */

setattr(to)
char *to;
{
 	struct 	utimbuf *times;
	int	retval = 0;
	int	(*chownf)() = ISLNK(s1) ? lchown : chown;
	
        times = (struct utimbuf *)malloc((unsigned)sizeof(struct utimbuf));

	if (!ISLNK(s1)){
		times->actime = s1.st_atime;
		times->modtime = s1.st_mtime;
		if (utime(to, times) < 0) {
			pfmt(stderr, MM_WARNING, badutime, to);
			retval++ ;
		}
	}

	if ((*chownf)(to, s1.st_uid, s1.st_gid) < 0) {
		pfmt(stderr, MM_WARNING, badchown, to) ;
		retval++ ;
	} else if (!ISLNK(s1)
			&& (FMODE(s1) & (S_ISUID|S_ISGID|S_ISVTX))) {
		if (chmod(to, FMODE(s1) & ALL_MODEBITS) < 0) {
			pfmt(stderr, MM_WARNING, badchmod, to);
			retval++ ;
		}
	}
	free(times);
	return(mve ? 0: retval) ;
}

/*
 * Procedure:     rm
 *
 * Restrictions:
 *                lstat(2): none
 *                pfmt: none
 *                strerror: none
 *                access(2): none
 *                isatty: none	
 *                unlink(2): none
 */				

rm(path)
	char	*path;
{
	struct stat buffer;

	/*
	 * Check file to see if it exists.
	 */

	if (lstat(path, &buffer) < 0) {
		pfmt(stderr, MM_ERROR, errmsg, path, strerror(errno));
		return(1) ;
	}

	/*
	 * If it's a directory, remove its contents.
	 */

	if (ISDIR(buffer)) {
		return(undir(path));
	}

	/*
	 * remove the file.
	 */

	if (unlink(path) < 0) {
		pfmt(stderr, MM_ERROR, badunlink, path, strerror(errno));
		return(1) ;
	}
	return(0) ;
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
 */

undir(path)
	char	*path;
{
	DIR	*name;
	struct dirent *dp;
	char delpath[MAXPATHLEN + 1];
	int errs = 0;

	/*
	 * Open the directory for reading.
	 */

	if ((name = opendir(path)) == NULL) {
		pfmt(stderr, MM_ERROR, errmsg, path, strerror(errno));
		return (1);
	}

	/*
	 * Read every directory entry.
	 */
	while ((dp = readdir(name)) != NULL) {
		/*
		 * Ignore "." and ".." entries.
 		 */
		if(!strcmp(dp->d_name, ".")
		  || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(path)+1+strlen(dp->d_name)
						>= sizeof delpath - 1) {
			(void) pfmt(stderr, 
				MM_WARNING, ":347:%s/%s: Name too long\n",
                            path, dp->d_name);
			errs++;
			continue;
		}

		sprintf(delpath, "%s/%s", path, dp->d_name);
 
		/*
		 * If a spare file descriptor is available, just call the
		 * "rm" function with the file name; otherwise close the
		 * directory and reopen it when the child is removed.
		 */

		if (name->dd_fd >= (OPEN_MAX -2)) {
			closedir(name);
			errs += rm(delpath);
			if ((name = opendir(path)) == NULL) {
				pfmt(stderr, MM_ERROR, errmsg,
						path, strerror(errno));
				return(++errs) ;
			}
		} else
			errs += rm(delpath);
	}

	/*
	 * Close the directory we just finished reading.
	 */

	closedir(name);

	if (errs)
		return(errs) ;

	/* remove the directory */
	if (rmdir(path) < 0) {
		pfmt(stderr, MM_ERROR, errmsg, path, strerror(errno));
		++errs;
	}
	return(errs) ;
}
