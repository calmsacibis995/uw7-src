/*	copyright	"%c%"	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */
#ident	"@(#)ufs.cmds:i386/cmd/fs.d/ufs/fsck/main.c	1.9.11.1"

/*  "errexit()" and "pfatal()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "pwarn()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include <sys/signal.h>
#include <string.h>
#include "fsck.h"
#include <sys/vfstab.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include <nl_types.h>
#include <langinfo.h>
#include <regex.h>

#define	NAME_MAX	64
#define	FSTYPE		"ufs"

char	*myname, fstype[]=FSTYPE;
extern	char *strrchr();

int	mflag = 0;		/* sanity check only */
char	hotroot;

extern int	optind;
extern char	*optarg;

int mnt_passno = 1;

int	exitstat;	/* exit status (set to 8 if 'No' response) */
char	*rawname(), *unrawname(), *blockcheck(), *hasvfsopt();
int	catch(), catchquit(), voidquit();
int	returntosingle;
int	rootfs = 0;		/* am I checking the root? */

regex_t	yesre, nore;

char *subopts [] = {
#define PREEN		0
	"p",
#define BLOCK		1
	"b",
#define DEBUG		2
	"d",
#define READ_ONLY	3
	"r",
#define	ONLY_WRITES	4
	"w",
	NULL
};

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int pid, passno, anygtr, sumstatus;
	char	*name;
	int	c;
	char	*suboptions,	*value;
	int	suboption;
	char	filename[20];	/* "max" length of name "/.fsck.L.<pid>" */
	char	string[NAME_MAX+8];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxfsck");
	myname = strrchr(argv[0],'/');
	myname = (myname != 0)? myname+1:argv[0];
	sprintf(string, "UX:%s %s", fstype, myname);
	(void)setlabel(string);

	wflg = bflag =bflg=Pflag=preen=wflag=yflag=nflag=Lflag=0;
	Parent_notified = B_TRUE;
	setbuf(stdout, NULL);
	sync();

	while ((c = getopt (argc, argv, "PbwmnNo:VyYL")) != EOF) {
		switch (c) {

		case 'b':
			bflg++;
			break;
		case 'L':
			Lflag++;
			sprintf(filename, "/.fsck.L.%d", getpid());
			if ((print_fp = fopen(filename, "w+")) == NULL) {
				pfmt(stderr, MM_ERROR, ":284:Cannot open %s\n", filename);
				exit(1);
			}
			Parent_notified = B_FALSE;
			break;
		case 'm':
			mflag++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'o':
			/*
			 * ufs specific options.
			 */
			suboptions = optarg;
			while (*suboptions != '\0') {
				switch ((suboption = getsubopt(&suboptions, subopts, &value))) {
		
				case PREEN:
					preen++;
					break;
		
				case BLOCK:
					if (value == NULL) {
						usage ();
					} else {
						bflag = atoi(value);
					}
					if (!bflg)
					    pfmt(stdout, MM_NOSTD,
						":208:Alternate super block location: %d\n",
					    		bflag);
					break;
		
				case DEBUG:
					debug++;
					break;
		
				case READ_ONLY:
					break;
		
				case ONLY_WRITES:	/* check only writable filesystems */
					wflag++;
					break;
		
				default:
					usage();
				}
			}
			break;

		case 'P':
			Pflag++;
			break;
		case 'V':
			{
				int	opt_count;
				char	*opt_text;

				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, "fsck -F ufs %s \n", opt_text);
					else
						(void) fprintf (stdout, "fsck -F ufs\n ");
				}
			}
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;
		case 'w':
			wflg++;
			bflg++;
			break;
		case '?':
			usage();
		}
	}
	if (Pflag)
		semid = getsemid();
	argc -= optind;
	argv = &argv[optind];
	rflag++; /* check raw devices */
	if (signal(SIGINT, SIG_IGN) != (int)SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	if (argc) {
		while (argc-- > 0) {
			if (wflag && !writable(*argv)) {
				(void) myfprintf (stderr, MM_ERROR, ":209:not writeable '%s'\n", *argv);
				argv++;
			} else {
				if (strcmp (*argv, "/") == 0)
					rootfs = 1;
				checkfilesys(*argv);
				rootfs = 0;
				argv++;
			}
		}
		exit(exitstat);
	}
	sumstatus = 0;
	passno = 1;
	do {
		FILE *vfstab;
		struct vfstab vfsbuf;
		char	*raw;

		anygtr = 0;
		if ((vfstab = fopen(VFSTAB, "r")) == NULL)
			errexit(":210:Cannot open checklist file: %s\n", VFSTAB);
		while ((getvfsent(vfstab, &vfsbuf)) == 0) {
			if (vfsbuf.vfs_fstype &&
				strcmp(vfsbuf.vfs_fstype, MNTTYPE_UFS)) {
				continue;
			}
			if (wflag && hasvfsopt(&vfsbuf, MNTOPT_RO)) {
				continue;
			}
			if (numbers(vfsbuf.vfs_fsckpass))
				mnt_passno = atoi(vfsbuf.vfs_fsckpass);
			if (preen == 0 ||
			    passno == 1 && mnt_passno == passno) {
				if ((preen == 1) && rflag) {
					if (strcmp (vfsbuf.vfs_mountp, "/") == 0)
						rootfs = 1;
					raw =
					    rawname(unrawname(vfsbuf.vfs_special));
					checkfilesys(raw);
					rootfs = 0;
				} else {
					name = blockcheck(vfsbuf.vfs_special);
					if (name != NULL)
						checkfilesys(name);
					else if (preen)
						exit(36);
				}
			} else if (mnt_passno > passno)
				anygtr = 1;
			else if (mnt_passno == passno) {
				pid = fork();
				if (pid < 0) {
					pfmt(stderr,MM_ERROR,
					    ":211:fork: %s\n", strerror(errno));
					exit(36);
				}
				if (pid == 0) {
					(void)signal(SIGQUIT, voidquit);
					if ((preen == 1) && rflag) {
						raw =
						    rawname(unrawname(vfsbuf.vfs_special));
						checkfilesys(raw);
						exit(exitstat);
					} else {
						name = blockcheck(vfsbuf.vfs_special);
						if (name == NULL)
							exit(36);
						checkfilesys(name);
						exit(exitstat);
					}
				}
			}
		}
		fclose(vfstab);
		if (preen) {
			int status;
			while (wait(&status) != -1)
				sumstatus |= WHIBYTE(status);
		}
		passno++;
	} while (anygtr);
	if (sumstatus)
		exit(36);
	if (returntosingle)
		exit(31+2);
	exit(exitstat);
}

checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;
	int err;

	mountedfs = 0;
	if ((devname = setup(filesys)) == 0) {
		if (preen)
			mypfatal(":212:CANNOT CHECK FILE SYSTEM.");
		exit(36);
	}

	/* setup langinfo */
	err = regcomp(&yesre, nl_langinfo(YESEXPR), REG_EXTENDED | REG_NOSUB);
	if (err != 0) {
		char buf[BUFSIZ];

		regerror(err, &yesre, buf, BUFSIZ);
		pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
		exit(2);
	}
	err = regcomp(&nore, nl_langinfo(NOEXPR), REG_EXTENDED | REG_NOSUB);
	if (err != 0) {
		char buf[BUFSIZ];

		regerror(err, &nore, buf, BUFSIZ);
		pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
		exit(2);
	}

	if (mflag)
		check_sanity (filesys);	/* this never returns */
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if ((preen == 0) && !bflg) {
		if (mountedfs) 
			myprintf(":213:** Currently Mounted on %s\n", sblock.fs_fsmnt);
		else
			myprintf(":214:** Last Mounted on %s\n", sblock.fs_fsmnt);
		if (mflag)
			myprintf(":215:** Phase 1 - Sanity Check only\n");
		else
			myprintf(":216:** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			mypfatal(":217:INTERNAL ERROR: dups with -p");
		if (!bflg)
			myprintf(":218:** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 * if 
	 */
	if ((preen == 0) && !bflg)
		myprintf(":219:** Phase 2 - Check Pathnames\n");
	if (MEM)
		pass2();
	else
		Opass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if ((preen == 0) && !bflg)
		myprintf(":220:** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if ((preen == 0) && !bflg)
		myprintf(":221:** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if ((preen == 0) && !bflg)
		myprintf(":222:** Phase 5 - Check Cyl groups\n");
	pass5();
	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	getsem();
	if (Pflag && Parent_notified == B_FALSE) {
		pfmt(print_fp, MM_NOSTD|MM_NOGET, "%s: ", devname);
		pfmt(print_fp, MM_NOSTD, ":223:%d files, %d used, %d free ",
			n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
		if (preen)
			fprintf(print_fp,"\n");
		pfmt(print_fp, MM_NOSTD, ":224:(%d frags, %d blocks, %.1f%% fragmentation)\n",
			n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	} else if (Pflag) {
		pfmt(stdout, MM_NOSTD|MM_NOGET, "%s: ", devname);
		pfmt(stdout, MM_NOSTD, ":223:%d files, %d used, %d free ",
			n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
		if (preen)
			printf("\n");
		pfmt(stdout, MM_NOSTD, ":224:(%d frags, %d blocks, %.1f%% fragmentation)\n",
			n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	} else {
		pfmt(stdout, MM_NOSTD, ":223:%d files, %d used, %d free ",
			n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
		if (preen)
			printf("\n");
		pfmt(stdout, MM_NOSTD, ":224:(%d frags, %d blocks, %.1f%% fragmentation)\n",
			n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	}
	relsem();
	if (debug && (n_files -= imax - SFSROOTINO - sblock.fs_cstotal.cs_nifree))
		myprintf(":225:%d files missing\n", n_files);
	if (debug) {
		n_blks += sblock.fs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (n_blks -= fmax - (n_ffree + sblock.fs_frag * n_bfree))
			myprintf(":226:%d blocks missing\n", n_blks);
		if (duplist != NULL) {
			myprintf(":227:The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				myprintf(":350: %d,", dp->dup);
			Sprintf("\n");
		}
		if (zlnhead != NULL) {
			myprintf(":228:The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				myprintf(":350: %d,", zlnp->zlncnt);
			Sprintf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
	if (dfile.mod)
		fixstate = 1;
	else
		fixstate = 0;
	if (sblock.fs_state + (long)sblock.fs_time != FSOKAY) {
		if (nflag) {
			myprintf(":229:%s FILE SYSTEM STATE NOT SET TO OKAY\n",
				devname);
			fixstate = 0;
		} else	{
			myprintf(":230:%s FILE SYSTEM STATE SET TO OKAY\n", devname);
			fixstate = 1;
		}
		
	}	
	if (fixstate) {
		(void)time(&sblock.fs_time);
		sblock.fs_state = FSOKAY - (long)sblock.fs_time;
		sbdirty();
	}
	ckfini();
	free(blockmap);
	if (MEM)
		free((char *)statemap.nstate);
	else
		free((char *)statemap.ostate);
	free((char *)lncntp);
	if (!dfile.mod)
		return;
	if (!preen && !bflg) {
		myprintf(":231:***** FILE SYSTEM WAS MODIFIED *****\n");
	}
	if (mountedfs || rootfs) {
		exit(40);
	}
	exit(0);	
}

char *
blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	rootfs = 0;
	if (stat("/", &stslash) < 0)
		if(!bflg){
			myprintf(":232:Cannot stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0) {
		if(!bflg)
			myprintf(":233:Cannot stat %s\n", name);
		return (0);
	}
	if (stblock.st_mode & S_IFBLK) {
		raw = rawname(name);
		if (stat(raw, &stchar) < 0) {
			if(!bflg)
				myprintf(":233:Cannot stat %s\n", raw);
			return (0);
		}
		if (stchar.st_mode & S_IFCHR) {
			if (stslash.st_dev == stblock.st_rdev) {
				rootfs++;
				if (!rflag)
					raw = unrawname(name);
			}
			return (raw);
		} else {
			if (!bflg)
				myprintf(":234:%s is not a character device\n", raw);
			return (0);
		}
	} else if (stblock.st_mode & S_IFCHR) {
		if (looped) {
			if (!bflg)
				myprintf(":235:Cannot make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	if (!bflg)
		myprintf(":235:Cannot make sense out of name %s\n", name);
	return (0);
}



/*
 * exit 0 - file system is unmounted and okay
 * exit 32 - file system is unmounted and needs checking
 * exit 33 - file system is mounted
 *
 *          for root file system
 * root is mounted readonly at boot time
 * exit 101 - indicates it is okay, remount it.
 *
 * exit 32 - needs checking
 * exit 34 - cannot stat device
 */

check_sanity(filename)
char	*filename;
{
	struct stat stbd, stbr;

	if (stat(filename, &stbd) < 0) {
		myfprintf(stderr, MM_ERROR, ":161:sanity check failed : cannot stat %s\n", filename);
		exit(34);
	}
	stat("/", &stbr);
	if (strcmp(filename, "/dev/rroot") == 0) {	/* root file system */
		if ((sblock.fs_state + (long)sblock.fs_time) != FSOKAY) {
			myfprintf(stderr, MM_ERROR, ":162:sanity check: root file system needs checking\n");
			exit(32);
		} else {
			myfprintf(stderr, MM_INFO, ":163:sanity check: root file system okay\n");
			exit(101);
		}
	}
	if (stbd.st_flags & _S_ISMOUNTED) {
		myfprintf(stderr, MM_ERROR, ":164:sanity check: %s already mounted\n", filename);
		exit(33);
	}

	if ((sblock.fs_state + (long)sblock.fs_time) != FSOKAY) {
		myfprintf(stderr, MM_INFO, ":236:time=%x, state=%x\n", sblock.fs_time, sblock.fs_state);
		myfprintf(stderr, MM_ERROR, ":165:sanity check: %s needs checking\n", filename);
		exit(32);
	}
	if (!bflg)
		myfprintf(stderr, MM_INFO, ":166:sanity check: %s okay\n", filename);
	exit(0);
}

char *
unrawname(cp)
	char *cp;
{
	char *dp = strrchr(cp, '/');
	struct stat stb;
	static char rawbuf[MAXPATHLEN];

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/dsk/%s", dp+1);
	if (stat(rawbuf, &stb) == 0)
		return(rawbuf);
	
	/* for device naming convention /dev/save */
	if (*(dp+1) != 'r')
		return (cp);
	(void)strcpy(dp+1, dp+2);
	return (cp);
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp = strrchr(cp, '/');
	struct stat statb;

	if (dp == 0)
		return (0);
	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/rdsk/%s", dp+1);
	if (stat(rawbuf, &statb) == 0)
		return (rawbuf);
	/* for device naming convention /dev/save */
	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);
	return (rawbuf);
}

static char *
mntopt(p)
        char **p;
{
        char *cp = *p;
        char *retstr;

        while (*cp && isspace(*cp))
                cp++;
        retstr = cp;
        while (*cp && *cp != ',')
                cp++;
        if (*cp) {
                *cp = '\0';
                cp++;
        }
        *p = cp;
        return (retstr);
}

char *
hasvfsopt(vfs, opt)
        register struct vfstab *vfs;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
        strcpy(tmpopts, vfs->vfs_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + vfs->vfs_mntopts);
        }
        return (NULL);
}

char *
hasmntopt(mnt, opt)
        register struct mnttab *mnt;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
        strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + mnt->mnt_mntopts);
        }
        return (NULL);
}


usage ()
{
	(void)pfmt (stderr, MM_ACTION,
	    ":237:Usage: %s [-F %s] [generic options] [-o p,b=#,w] [special ....]\n",myname,fstype);
	exit (31+1);
}
