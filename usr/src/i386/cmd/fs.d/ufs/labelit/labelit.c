/*		copyright	"%c%" 	*/

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

#ident	"@(#)ufs.cmds:i386/cmd/fs.d/ufs/labelit/labelit.c	1.6.10.6"

/*
 * Label a file system volume.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mntent.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <fcntl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/sysmacros.h>
#include <sys/fs/sfs_fs.h>
#include <archives.h>
#include <sys/stat.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include <time.h>

union sbtag {
	char		dummy[SBSIZE];
	struct fs	sblk;
} sb_un, altsb_un;

#define sblock sb_un.sblk
#define altsblock altsb_un.sblk

#define IFTAPE(s)       (equal(s, "/dev/rmt", 8) || equal(s, "rmt", 3))
#define TAPENAMES "'/dev/rmt'"
#define NAME_MAX	64
#define FSTYPE		"ufs"

char	*myname, fstype[]=FSTYPE;

struct volcopy_label	Tape_hdr;

extern int	optind;
extern char	*optarg;

int	status;
int	blk;

static char * getfsnamep();
static char time_buf[80];
#define DATE_FMT	gettxt(":1", "%a %b %e %H:%M:%S %Y\n")

extern int gettimeofday();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	opt, i;
	int	nflag = 0;
	char	*special = NULL;
	char	*fsname = NULL;
	char	*volume = NULL;
	char	*p;
	char	string[NAME_MAX];
	struct timeval tp;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxlabelit");
	myname = (char *)strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(string, "UX:%s %s", fstype, myname);
	(void)setlabel(string);

	while ((opt = getopt (argc, argv, "?no:")) != EOF) {
		switch (opt) {

		case 'o':		/* specific options (none defined yet) */
			break;
		case 'n':
			nflag++;
			break;

		case '?':
			usage();
		}
	}
	if (optind > (argc - 1)) {
		usage ();
	}
	argc -= optind;
	argv = &argv[optind];
	special = argv[0];
	if (argc > 1) {
		fsname = argv[1];
		if (strlen(fsname) > 6) {
			(void)pfmt(stderr, MM_ERROR,
				":25:fsname must be less than 7 characters\n");
			usage();
		}
	}
	if (argc > 2) {
		volume = argv[2];
		if (strlen(volume) > 6) {
			(void)pfmt(stderr, MM_ERROR,
				":26:volume must be less than 7 characters\n");
			usage();
		}
	}
	if (nflag) {
		if (!IFTAPE(special)) {
			pfmt(stderr, MM_ERROR,
				":29:`-n' option for tape only\n");
			pfmt(stderr, MM_ERROR,
				":30:'%s' is not a valid tape name\n", special);
			pfmt(stderr, MM_ACTION,
				":31:Valid tape names begin with %s\n", TAPENAMES);
			usage();
			exit(33);
		}
		if (!fsname || !volume) {
			pfmt(stderr, MM_ERROR,
				":32:`-n' option requires fsname and volume\n");
			usage();
			exit(33);
		}
	}
        if (IFTAPE(special)) {
                int     fso;

                if ((fso = open(special, O_RDONLY)) < 0) {
                        pfmt(stderr, MM_ERROR, ":28:open: %s\n",
				strerror(errno));
                        exit (31+1);
                }

		if (!nflag) {
			if(read(fso, &Tape_hdr, sizeof(Tape_hdr)) != sizeof(Tape_hdr)) {
				pfmt(stderr, MM_ERROR,
					":33:cannot read label\n");
				exit(31+1);
			}
			pfmt(stdout, MM_INFO,
				":34:%s floppy volume: %.6s, reel %d of %d reels\n",
    			Tape_hdr.v_magic, Tape_hdr.v_volume, Tape_hdr.v_reel, Tape_hdr.v_reels);
			cftime(time_buf, DATE_FMT, &Tape_hdr.v_time);
			pfmt(stdout, MM_INFO, ":10:Written: %s", time_buf);
			if((argc==2 && Tape_hdr.v_reel>1) || equal(Tape_hdr.v_magic,"Finc",4))
				exit(0);
			if (read(fso, &sblock, BBSIZE) != BBSIZE) {
				pfmt(stderr, MM_ERROR,
					":36:cannot read boot block\n");
				exit(31+1);
			}
			if (read(fso, &sblock, SBSIZE) != SBSIZE) {
				pfmt(stderr, MM_ERROR,
					":14:cannot read superblock\n");
				exit(31+1);
				}
			cftime(time_buf, DATE_FMT, &sblock.fs_time);
			pfmt(stdout, MM_INFO, ":21:Date last modified: %s", time_buf);
			p = getfsnamep(&sblock);
			pfmt(stdout, MM_INFO,
				":37:fsname: %.6s\n",p);
			pfmt(stdout, MM_INFO,
				":38:volume: %.6s\n",p+6);
			if(argc == 1){
				close(fso);
				exit(0);
			}
      	     	}
		else /* nflag */
			pfmt(stdout, MM_INFO, ":6:Skipping label check!\n");
               	pfmt(stdout, MM_INFO,
			":39:Labeling tape - any contents will be destroyed!!\n");
                pfmt(stdout, MM_INFO,
		    ":22:NEW fsname = %.6s, NEW volname = %.6s -- DEL if wrong!!\n",
			fsname,volume);
                sleep(10);
		memset(&Tape_hdr, '\0', sizeof(Tape_hdr));
		memset(&sb_un, '\0', SBSIZE);
                strcpy(Tape_hdr.v_magic, "Volcopy");
                sprintf(Tape_hdr.v_volume, "%.6s", volume);
		/* get a new date */
		gettimeofday(&tp, 0);
		Tape_hdr.v_time = tp.tv_sec;
		close(fso);
                if ((fso = open(special, O_WRONLY)) < 0) {
                        (void)pfmt(stderr, MM_ERROR,
				":28:open: %s\n", strerror(errno));
                        exit (31+1);
                }
                if (write(fso, &Tape_hdr, sizeof(Tape_hdr)) != sizeof(Tape_hdr)) {
                        pfmt(stderr, MM_ERROR,
				":23:cannot write label\n");
                        exit(31+1);
                }
		if (write(fso, &sblock, BBSIZE) != BBSIZE) {
       			pfmt(stderr, MM_ERROR,
				":41:cannot write bootblock\n");
       			exit(31+1);
		}
		p = getfsnamep(&sblock);
		for (i = 0; i < 12; i++)
			p[i] = '\0';		
		for (i = 0; (i < 6) && (fsname[i]); i++, p++)
			*p = fsname[i];
		if (i < 6) 
			p = p + (6 - i);
		for (i = 0; (i < 6) && (volume[i]); i++, p++)
			*p = volume[i];
                if (write(fso, &sblock, SBSIZE) != SBSIZE) {
                        pfmt(stderr, MM_ERROR,
				":42:cannot write superblock\n");
                        exit(31+1);
                }
                close(fso);
        } else 
                label(special, fsname, volume);
        exit(0);
}

usage ()
{

	(void)pfmt(stderr, MM_ACTION,
		":43:Usage: %s [-F %s] [generic options] [-n] special [fsname volume]\n",
			myname, fstype);
	exit (31+1);
}

label (special, fsname, volume)
	char		*special;
	char		*fsname;
	char		*volume;
{
	int	f;
	int	i;
	char	*p, *savep;
	int	offset;
	struct	stat statarea;

	if (fsname == NULL) {
		f = open(special, O_RDONLY);
	} else {
		f = open(special, O_RDWR);
	}
	if (f < 0) {
		pfmt(stderr, MM_ERROR,
			":28:open: %s\n", strerror(errno));
		exit (31+1);
	}

	if(fstat(f, &statarea) < 0) {
		pfmt(stderr, MM_ERROR,
			":44:fstat: %s\n", strerror(errno));
		exit(32);
	}
	if ((statarea.st_mode & S_IFMT) != S_IFBLK &&
	    (statarea.st_mode & S_IFMT) != S_IFCHR) {
		(void)pfmt(stderr, MM_ERROR,
			":12:%s is not special device\n", special);
		exit(32);
	}

	if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
		pfmt(stderr, MM_ERROR,
			":45:lseek: %s\n", strerror(errno));
		exit (31+1);
	}
	if (read(f, &sblock, SBSIZE) != SBSIZE) {
		pfmt(stderr, MM_ERROR,
			":46:read: %s\n", strerror(errno));
		exit (31+1);
	}
	if (sblock.fs_magic != UFS_MAGIC) {
		(void)pfmt(stderr, MM_ERROR,
			":47:bad super block magic number\n");
		exit (31+1);
	}
	p = getfsnamep(&sblock);
	savep = p;
	if (fsname != NULL) {
		for (i = 0; i < 12; i++)
			p[i] = '\0';		
		for (i = 0; (i < 6) && (fsname[i]); i++, p++)
			*p = fsname[i];
	}
	if (volume != NULL) {
		if (i < 6) 
			p = p + (6 - i);
		for (i = 0; (i < 6) && (volume[i]); i++, p++)
			*p = volume[i];
	}
	if (fsname != NULL) {
		if (statarea.st_flags & _S_ISMOUNTED) {
			(void)pfmt(stderr, MM_ERROR,
				":13:%s is mounted\n", special);
			exit(32);
		}
		if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
			pfmt(stderr, MM_ERROR,
				":45:lseek: %s\n", strerror(errno));
			exit (31+1);
		}
		if (write(f, &sblock, SBSIZE) != SBSIZE) {
			pfmt(stderr, MM_ERROR,
				":48:write: %s\n", strerror(errno));
			exit (31+1);
		}
		for (i = 0; i < sblock.fs_ncg; i++) {
		offset = cgsblock(&sblock, i) * sblock.fs_fsize;
		if (lseek(f, offset, 0) < 0) {
			pfmt(stderr, MM_ERROR,
				":45:lseek: %s\n", strerror(errno));
			exit (31+1);
		}
		if (read(f, &altsblock, SBSIZE) != SBSIZE) {
			pfmt(stderr, MM_ERROR,
				":46:read: %s\n", strerror(errno));
			exit (31+1);
		}
		if (altsblock.fs_magic != UFS_MAGIC) {
			(void)pfmt(stderr, MM_ERROR,
				":49:bad alternate super block(%i) magic number\n", i);
			exit (31+1);
		}
		memcpy((char *)&(altsblock.fs_rotbl[blk]),
		       (char *)&(sblock.fs_rotbl[blk]), 12);
		
		if (lseek(f, offset, 0) < 0) {
			pfmt(stderr, MM_ERROR,
				":45:lseek: %s\n", strerror(errno));
			exit (31+1);
		}
		if (write(f, &altsblock, SBSIZE) != SBSIZE) {
			pfmt(stderr, MM_ERROR,
				":48:write: %s\n", strerror(errno));
			exit (31+1);
		}
		}
	}
	pfmt(stdout, MM_INFO, ":37:fsname: %.6s\n", savep);
	pfmt(stdout, MM_INFO, ":38:volume: %.6s\n", savep+6);
}

equal(s1,s2,ct)
        char *s1, *s2;
        int ct;
{
        return  !strncmp(s1, s2, ct);
}

/* Determine where in fs_rotbl the fsname should be placed */

char *
getfsnamep(sblockp)
struct fs *sblockp;
{
	int i;

	if (sblockp->fs_nspf == 0) {
		return((char *)&(sblockp->fs_rotbl[0]));
	}
	blk = sblockp->fs_spc * sblockp->fs_cpc/sblockp->fs_nspf;
	for (i = 0; i < blk; i += sblockp->fs_frag)
		/* void */;
	i -= sblockp->fs_frag;
	blk = i / sblockp->fs_frag;
	return((char *)&(sblockp->fs_rotbl[blk]));
}
