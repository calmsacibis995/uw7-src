/*		copyright	"%c%" 	*/

#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/df.c	1.15.8.7"
#ident "$Header$"
/* s5 df */
#include <stdio.h>
#include <sys/param.h>
#include <sys/fs/s5param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mnttab.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5fblk.h>
#include <sys/fs/s5dir.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5ino.h>
#include <locale.h>
#include <pfmt.h>

#define EQ(x,y) (strcmp(x,y)==0)
#define MOUNTED fs_name[0]
#define DEVLEN	1024
#define DEVLSZ 200	/* number of unique file systems we can remember */

extern char *optarg;
extern int optind, opterr;

struct	mnttab	Mp;
struct stat	S;
struct filsys	sblock;
struct statvfs	Fs_info;

int	physblks;
long	bsize;
int 	bshift;
int	freflg, totflg, bflg, eflg, kflg,ebflg=0;
int	header=0;
int	fd;
daddr_t	blkno	= 1;
daddr_t	alloc();

char nfsmes[] = ":280:%-10.32s is not a special device or directory\n";
char nfsmes2[] = ":281:%-10.32s is not a s5 file system\n";
char usage[] = ":282:Usage:\ndf [-F s5] [generic_options] [-f] [directory | special ...]\n";
char badstat[] = ":5:Cannot access %s: %s\n";
char badopen[] = ":4:Cannot open %s: %s\n";
char fmtname[] = "%-19.32s";

char	mnttab[] = MNTTAB;

char *
basename(s)
char *s;
{
	int n = 0;

	while(*s++ != '\0') n++;
	while(n-- > 0)
		if(*(--s) == '/')
			return(++s);
	return(--s);
}

main(argc, argv)
char **argv;
{
	FILE	*fi;
	register i;
	char	 *res_name, *s;
	int	 c, k, errcnt = 0;
	struct	stat	S2;
	
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	s = basename(argv[0]);
	(void)setlabel("UX:df s5");

	while((c = getopt(argc,argv,"bektf")) != EOF) {
		switch(c) {
		case 'b':	/* print blocks free */
			bflg++;
			break;

		case 'e':	/* print only file entries free */
			eflg++;
			break;

		case 'k':	/* print allocation in kilobytes */
			kflg++;
			break;

		case 'f':	/* freelist count only */
			freflg++;
			break;

		case 't':	/* include total allocations */
			totflg = 1;
			break;

		case '?':	/* usage message */

		default:
			pfmt(stderr, MM_ACTION, usage);
			exit(31+1);
		}
	}

	if((fi = fopen(mnttab, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
		exit(31+1);
	}
	if (eflg && bflg) {
		header++;
		ebflg++;
	}
	while ((i = getmntent(fi, &Mp)) == 0) {
		if(argc > optind) {
			/* we are looking for specific file systems
				   in the mount table */
			/* What device are we dealing with ? 
			   Get it into S2.st_rdev 	*/
			if ((stat(Mp.mnt_special, &S2) == (-1))
			|| (S2.st_mode & S_IFMT != S_IFBLK))
				S2.st_rdev = -1;
			for(k=optind;k < argc; k++) {
				if(argv[k][0] == '\0')
					continue;
				if(stat(argv[k], &S) == -1)
					S.st_dev = -1;

				/* does the argument match either
					   the mountpoint or the device? */
				if(EQ(argv[k], Mp.mnt_special)
				    || EQ(argv[k], Mp.mnt_mountp)) {
					errcnt += printit(Mp.mnt_special, Mp.mnt_mountp);
					argv[k][0] = '\0';
				} else	
					/* or is it on one of 
						the mounted devices? */
					if( ((S.st_mode & S_IFMT) == S_IFDIR)
					    && (S.st_dev != (-1))
					    && (S.st_dev == S2.st_rdev)) {
						errcnt += printit(Mp.mnt_special, argv[k]);
						argv[k][0] = '\0';
					}
			}
		} else {
			/* doing all things in the mount table */
			if(stat(Mp.mnt_mountp, &S) == -1)
				S.st_dev = -1;
			errcnt += printit(Mp.mnt_special, Mp.mnt_mountp);
		}
	}

	/* process arguments that were not in /etc/mnttab */
	for(i = optind; i < argc; ++i) {
		if(argv[i][0])
			errcnt += printit(argv[i], "\0");
	}
	fclose(fi);
	if (errcnt) {
		exit(31+errcnt);
	}
	exit(0);
}

printit(dev, fs_name)
char *dev, *fs_name;
{

	if(!MOUNTED || freflg) {
		if((fd = open(dev, 0)) < 0) {
			pfmt(stderr, MM_ERROR, badopen, dev, strerror(errno));
			close(fd);
			return(1);
		}
		if(stat(dev, &S) < 0) {
			pfmt(stderr, MM_ERROR, badstat, dev, strerror(errno));
			close(fd);
			return(1);
		}

		if(((S.st_mode & S_IFMT) == S_IFREG)
		    || ((S.st_mode & S_IFMT) == S_IFIFO)) {
			pfmt(stdout, MM_ERROR, nfsmes, dev);
			close(fd);
			return(1);
		}

		sync();
		if(lseek(fd, (long)SUPERBOFF, 0) < 0
		    || read(fd, &sblock, (sizeof sblock)) != (sizeof sblock)) {
			close(fd);
			return(1);
		}
		if(sblock.s_magic == FsMAGIC) {
			if(sblock.s_type == Fs1b) {
				physblks = 1;
				bsize = 512;
				bshift = 9;
			} else if(sblock.s_type == Fs2b) {
				physblks = 2;
				bsize = 1024;
				bshift = 10;
			} else if(sblock.s_type == Fs4b) {
				physblks = 4;
				bsize = 2048;
				bshift = 11;
			} else {
				pfmt(stdout, MM_NOSTD,
				    ":285:          (%-12s): can't determine logical block size\n", dev);
				return(1);
			}
		} else {
			physblks = 1;
			bsize = 512;
			bshift = 9;
		}
		/* file system sanity test */
		if(sblock.s_fsize <= (daddr_t)sblock.s_isize
		    || sblock.s_fsize < sblock.s_tfree
		    || sblock.s_isize < (ushort)sblock.s_tinode*sizeof(struct dinode)/bsize
		    || (long)sblock.s_isize*bsize/sizeof(struct dinode) > 0x10000L) {
			pfmt(stdout, MM_ERROR, nfsmes2, dev);
			return(1);
		}

	} else {	/* mounted file system */

		if(statvfs(fs_name, &Fs_info) != 0) {
			pfmt(stderr, MM_ERROR, ":250:statvfs() on %s failed: %s\n",
			    dev, strerror(errno));
			return(1);
		}
		/* copy statvfs info into superblock structure */
		sblock.s_tfree = Fs_info.f_bfree*(Fs_info.f_frsize/512);
		sblock.s_tinode = Fs_info.f_ffree;
		strncpy(sblock.s_fname,Fs_info.f_fstr,6);
		strncpy(sblock.s_fpack,&Fs_info.f_fstr[6],6);
		physblks = 1;
	}
	if (kflg) {

		long cap, kbytes, used, avail;

		if (MOUNTED)
			kbytes = Fs_info.f_blocks*Fs_info.f_frsize/1024;
		else
			kbytes=sblock.s_fsize*physblks/2;
		avail= (sblock.s_tfree*physblks)/2;
		used = kbytes - avail;
		cap = kbytes > 0 ? ((used*100L)/kbytes + 0.5) : 0;
		pfmt(stdout, MM_NOSTD,
		    ":274:filesystem         kbytes   used     avail    capacity  mounted on\n");
		pfmt(stdout, MM_NOSTD,
		    ":286:%-18s %-8ld %-8ld %-8ld %2ld%%        %-19.6s\n", 
		    dev,
		    kbytes,
		    used,
		    avail,
		    cap,
		    fs_name);
		exit(0);
	}

	if(!freflg) {
		if(totflg) {
			if(!MOUNTED)
				printf(fmtname, sblock.s_fname);
			else
				printf(fmtname, fs_name);
			pfmt(stdout, MM_NOSTD, ":287:(%-16s):", dev);
			pfmt(stdout, MM_NOSTD,
			    ":288:  %8d blocks%8u files\n",
			    sblock.s_tfree*physblks, sblock.s_tinode);
			pfmt(stdout, MM_NOSTD,
			    ":289:                                total:");
			if(MOUNTED && !freflg)
				pfmt(stdout, MM_NOSTD,
				    ":288:  %8d blocks%8u files\n",
				    Fs_info.f_blocks*(Fs_info.f_frsize/512),
				    Fs_info.f_files);
			else
				pfmt(stdout, MM_NOSTD,
				    ":288:  %8d blocks%8u files\n",
				    sblock.s_fsize*physblks, 
				    (unsigned)((sblock.s_isize - 2)
				    *bsize/sizeof(struct dinode)));
			exit(0);
		}
		if ( bflg || eflg ) {
			if (bflg)  {
				if (ebflg) {
					if(!MOUNTED)
						printf(fmtname, sblock.s_fname);
					else
						printf(fmtname, fs_name);
					pfmt(stdout, MM_NOSTD,
					    ":290:(%-16s):  %8d kilobytes\n", 
					    dev,
					    ((sblock.s_tfree*physblks)/2));
				}
				else {
					if (!header) {
						pfmt(stdout, MM_NOSTD,
						    ":291:Filesystem              avail\n");
						header++;
					}
					if(!MOUNTED) {
						if (sblock.s_fname[0] != NULL) {
							printf(fmtname, sblock.s_fname);
						}
						else {
							printf("%-19s", dev);
						}
					}
					else
						printf(fmtname, fs_name);
					printf("     %-8d\n", ((sblock.s_tfree*physblks)/2));

				}
			}
			if (eflg) {
				if (ebflg) {
					if(!MOUNTED)
						printf(fmtname, sblock.s_fname);
					else
						printf(fmtname, fs_name);
					pfmt(stdout, MM_NOSTD, ":287:(%-16s):", dev);
					pfmt(stdout, MM_NOSTD, ":292:  %8u files\n", sblock.s_tinode);
				}
				else {
					if (!header) {
						pfmt(stdout, MM_NOSTD, ":293:Filesystem              ifree\n");
						header++;
					}
					if(!MOUNTED) {
						if (sblock.s_fname[0] != NULL) {
							printf(fmtname, sblock.s_fname);
						}
						else {
							printf("%-19s", dev);
						}
					}
					else
						printf(fmtname, fs_name);
					printf("     %-8u\n", sblock.s_tinode);
				}
			}
		}
		else {
			if(!MOUNTED)
				printf(fmtname, sblock.s_fname);
			else
				printf(fmtname, fs_name);
			pfmt(stdout, MM_NOSTD, ":287:(%-16s):", dev);
			pfmt(stdout, MM_NOSTD, ":288:  %8d blocks%8u files\n",
			    sblock.s_tfree*physblks, sblock.s_tinode);
		}

	} else {
		daddr_t	i;

		i = 0;
		while(alloc())
			i++;
		if(!MOUNTED)
			printf(fmtname, sblock.s_fname);
		else
			printf(fmtname, fs_name);
		if(totflg) {
			pfmt(stdout, MM_NOSTD, ":294:(%-16s):  %8d blocks%8u files\n",dev, i*physblks, sblock.s_tinode);
			pfmt(stdout, MM_NOSTD, ":289:                                total:");
			if(MOUNTED && !freflg)
				pfmt(stdout, MM_NOSTD,
				    ":288:  %8d blocks%8u files\n",
				    Fs_info.f_blocks*(Fs_info.f_frsize/512),
				    Fs_info.f_files);
			else
				pfmt(stdout, MM_NOSTD,
				    ":288:  %8d blocks%8u files\n",
				    sblock.s_fsize*physblks, 
				    (unsigned)((sblock.s_isize - 2)
				    *bsize/sizeof(struct dinode)));
			exit(0);
		}
		pfmt(stdout, MM_NOSTD, ":295:(%-16s):  %8d blocks\n",dev, i*physblks);
		if (bflg) {
			if(!MOUNTED)
				printf(fmtname, sblock.s_fname);
			else
				printf(fmtname, fs_name);
			pfmt(stdout, MM_NOSTD, ":290:(%-16s):  %8d kilobytes\n",dev, ((i*physblks)/2));
		}
		if (eflg) {
			if(!MOUNTED)
				printf(fmtname, sblock.s_fname);
			else
				printf(fmtname, fs_name);
			pfmt(stdout, MM_NOSTD, ":296:(%-16s):  %8u files\n",dev, sblock.s_tinode);
		}
		close(fd);
	}
	return(0);
}

daddr_t
alloc()
{
	int i;
	daddr_t	b;
	/* Would you believe the 3b20 dfc requires read counts mod 64?
		   Believe it! (see dfc1.c).
		*/
	char buf[(sizeof(struct fblk) + 63) & ~63];
	struct fblk *fb = (struct fblk *)buf;

	if (!sblock.s_nfree)
		return(0);
	i = --sblock.s_nfree;
	if(i>=NICFREE) {
		pfmt(stdout, MM_ERROR, ":297:Bad free count, b=%ld\n", blkno);
		return(0);
	}
	b = sblock.s_free[i];
	if(b == 0)
		return(0);
	if(b<(daddr_t)sblock.s_isize || b>=sblock.s_fsize) {
		pfmt(stdout, MM_ERROR, ":298:Bad free block (%ld)\n", b);
		return(0);
	}
	if(sblock.s_nfree <= 0) {

		bread(b, buf, sizeof(buf));
		blkno = b;
		sblock.s_nfree = fb->df_nfree;
		for(i=0; i<NICFREE; i++)
			sblock.s_free[i] = fb->df_free[i];
	}
	return(b);
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
	int n;

	if((lseek(fd, bno<<bshift, 0)) < 0) {
		pfmt(stderr, MM_ERROR, ":299:seek() failed in bread(): %s\n",
		    strerror(errno));
		exit(31+1);
	}
	if((n=read(fd, buf, cnt)) != cnt) {
		pfmt(stderr, MM_ERROR, ":300:Read error in bread() (%x, count = %d): %s\n",
		    bno, n, strerror(errno));
		exit(31+1);
	}
}
