/*		copyright	"%c%" 	*/

#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/labelit.c	1.31.1.14"
/* labelit with 512, 1K, and 2K block size support */
#include <stdio.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5dir.h>
#include <sys/fs/s5ino.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5filsys.h>
#include <locale.h>
#include <archives.h>
#include <ctype.h>
#include <pfmt.h>
#include <time.h>

/* write fsname, volume # on disk superblock */
struct {
	char fill1[SUPERBOFF];
	union {
		char fill2[UBSIZE];
		struct filsys fs;
	} f;
} super;
#define IFTAPE(s) (equal(s,"/dev/rmt",8)||equal(s,"rmt",3))
#define TAPENAMES "'/dev/rmt'"
#define NAME_MAX	64
#define FSTYPE		"s5"

struct volcopy_label    Tape_hdr;

int fsi, fso;
#define DATE_FMT	gettxt(":1","%a %b %e %H:%M:%S %Y\n")
static char time_buf[80];
char *myname, fstype[]=FSTYPE;


void
sigalrm()
{
	void	(*signal())();

	signal(SIGALRM,sigalrm);
}

extern  int	optind;
extern	char	*optarg;

main(argc, argv)
char **argv;
{
	long curtime;
	int i, c;
	int nflag = 0;

	char *dev = NULL;
	char *fsname = NULL;
	char *volume = NULL;
	char *magicsave = NULL;
	char string[NAME_MAX];
	struct timeval tp;

	struct stat statarea;

	void	(*signal())();
	static char *usage = ":2:Usage:\n%s [-F %s] [generic_options] [-n] special [fsname volume]\n";


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxlabelit");
	myname = (char *)strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(string, "UX:%s %s", fstype, myname);
	(void)setlabel(string);

	signal(SIGALRM, sigalrm);

	while ((c = getopt(argc, argv, "?n")) != EOF) {
		switch(c) {
		case 'n':
			nflag++;
			break;
		case '?':
			pfmt(stderr, MM_ACTION, usage, myname, fstype);
			exit(32);
		}
	}
	if ((argc - optind) < 1) {
		pfmt(stderr, MM_ACTION, usage, myname, fstype);
		exit(32);
	}
	dev = argv[optind++];
	/* for backwards compatability, -n may also appear as the last */
	/* in the command line, labelit device [fsname volname] -n */
	if (optind < argc) {
		if (strcmp(argv[optind],"-n") == 0) {
			nflag++;
			optind++;
		}
		else {
			fsname=argv[optind++];
		}
	}
	if (optind < argc) {
		if (strcmp(argv[optind],"-n") == 0) {
			nflag++;
		}
		else {
			volume=argv[optind];
		}
		optind++;
	}
	if (!nflag)
		if (optind < argc) {
			if (strcmp(argv[optind],"-n") == 0) {
				nflag++;
			}
		}
	if (nflag) {
		if(!IFTAPE(dev)) {
			pfmt(stderr, MM_ERROR,
			    ":3:`-n' option for tape only.\n");
			pfmt(stderr, MM_NOSTD,
			    ":4:\t'%s' is not a valid tape name.\n", dev);
			pfmt(stderr, MM_NOSTD,
			    ":5:\tValid tape names begin with %s.\n", TAPENAMES);
			pfmt(stderr, MM_ACTION, usage, myname, fstype);
			exit(33);
		}
		pfmt(stdout, MM_INFO, ":6:Skipping label check!\n");
		goto do_it;
	}

	if((fsi = open(dev,0)) < 0) {
		pfmt(stderr, MM_ERROR, ":7:cannot open device\n");
		exit(31+2);
	}

	if(IFTAPE(dev)) {
		alarm(5);
		read(fsi, &Tape_hdr, sizeof(Tape_hdr));
		magicsave = Tape_hdr.v_magic;
		alarm(0);
		if(!(equal(Tape_hdr.v_magic, "Volcopy", 7)||
		    equal(Tape_hdr.v_magic, "VOLCOPY", 7)||
		    equal(Tape_hdr.v_magic,"Finc",4))) {

			pfmt(stderr, MM_ERROR,
				":8:media not labelled!\n");
			exit(31+2);
		}
		pfmt(stdout, MM_NOSTD,
		    ":9:%s floppy volume: %s, reel %d of %d reels\n",
		    	Tape_hdr.v_magic, Tape_hdr.v_volume, Tape_hdr.v_reel, Tape_hdr.v_reels);
		cftime(time_buf, DATE_FMT, &Tape_hdr.v_time);
		pfmt(stdout, MM_NOSTD, ":10:Written: %s", time_buf);
		if((argc==2 && Tape_hdr.v_reel>1) || equal(Tape_hdr.v_magic,"Finc",4))
			exit(0);
	}

	if(fstat(fsi, &statarea) < 0) {
		pfmt(stderr, MM_ERROR, ":11:cannot stat %s\n", dev);
		exit(33);
	}
	if ((statarea.st_mode & S_IFMT) != S_IFBLK &&
	    (statarea.st_mode & S_IFMT) != S_IFCHR) {
		pfmt(stderr, MM_ERROR,
			":12:%s is not special device\n", dev);
		exit(33);
	}

	if((i=read(fsi, &super, sizeof(super))) != sizeof(super))  {
		pfmt(stderr, MM_ERROR, ":14:cannot read superblock\n");
		exit(31+2);
	}

#define	S	super.f.fs
	if (!IFTAPE(dev))
	    if ((S.s_magic != FsMAGIC)
	        ||  (S.s_nfree < 0)
	        ||  (S.s_nfree > NICFREE)
	        ||  (S.s_ninode < 0)
	        ||  (S.s_ninode > NICINOD)
	        ||  (S.s_type < 1)) {
	    	pfmt(stderr, MM_ERROR,
			":15:%s is not an s5 file system\n", dev);
		exit(31+2);
	    }

	switch(S.s_type)  {
	    case Fs1b:
		pfmt(stdout, MM_NOSTD,
		    ":16:Current fsname: %.6s, Current volname: %.6s, Blocks: %ld, Inodes: %d\nFS Units: 512b, ", 
		    	S.s_fname, S.s_fpack, S.s_fsize, (S.s_isize - 2) * 8);
		break;
	    case Fs2b:
		pfmt(stdout, MM_NOSTD,
		    ":17:Current fsname: %.6s, Current volname: %.6s, Blocks: %ld, Inodes: %d\nFS Units: 1Kb, ", 
		    	S.s_fname,S.s_fpack,S.s_fsize * 2,(S.s_isize - 2) * 16);
		break;
	    case Fs4b:
		pfmt(stdout, MM_NOSTD,
		    ":18:Current fsname: %.6s, Current volname: %.6s, Blocks: %ld, Inodes: %d\nFS Units: 2Kb, ",
		    	S.s_fname,S.s_fpack,S.s_fsize * 4,(S.s_isize - 2) * 32);
		break;
	    default:
		if (!IFTAPE(dev)) {
			pfmt(stderr, MM_ERROR, ":19:bad block type\n");
			pfmt(stdout, MM_NOSTD,
			":20:Current fsname: %.6s, Current volname: %.6s\n",
				S.s_fname, S.s_fpack);
		}
	}
	cftime(time_buf, DATE_FMT, &S.s_time);
	pfmt(stdout, MM_NOSTD, ":21:Date last modified: %s", time_buf);
	if(argc==2)
		exit(0);
do_it:
	if (nflag && argc < 5) {
		pfmt(stderr, MM_ACTION, usage, myname, fstype);
		exit(34);
	} else if (!nflag && argc < 4) {
		pfmt(stderr, MM_ACTION, usage, myname, fstype);
		exit(34);
	}
	if (statarea.st_flags & _S_ISMOUNTED) {
		pfmt(stderr, MM_ERROR, ":13:%s is mounted\n", dev);
		exit(33);
	}
	pfmt(stdout, MM_NOSTD,
		":22:NEW fsname = %.6s, NEW volname = %.6s -- DEL if wrong!!\n",
	    		fsname, volume);
	sleep(10);
	sprintf(super.f.fs.s_fname, "%.6s", fsname);
	sprintf(super.f.fs.s_fpack, "%.6s", volume);

	close(fsi);
	fso = open(dev,1);
	if(IFTAPE(dev)) {
		gettimeofday(&tp, 0);
		Tape_hdr.v_time = tp.tv_sec;
		strcpy(Tape_hdr.v_magic, "Volcopy");
		sprintf(Tape_hdr.v_volume, "%.6s", volume);
		if(write(fso, &Tape_hdr, sizeof(Tape_hdr)) < 0)
			goto cannot;
	}

	if(write(fso, &super, sizeof(super)) < 0) {
cannot:
		pfmt(stderr, MM_ERROR, ":23:cannot write label\n");
		exit(31+2);
	}
	exit(0);
}
equal(s1, s2, ct)
char *s1, *s2;
int ct;
{
	register i;

	for(i=0; i<ct; ++i) {
		if(*s1 == *s2) {
			;
			if(*s1 == '\0') return(1);
			s1++; 
			s2++;
			continue;
		} else return(0);
	}
	return(1);
}
/* heuristic function to determine logical block size of System V file system
 *  on tape
 */
tps5bsz(dev, blksize)
char	*dev;
long	blksize;
{

	int count;
	int skipblks;
	long offset;
	long address;
	char *buf;
	struct dinode *inodes;
	struct direct *dirs;
	char * p1;
	char * p2;


	buf = (char *)malloc(2 * blksize);

	close(fsi);

	/* look for i-list starting at offset 2048 (indicating 1KB block size) 
	 *   or 4096 (indicating 2KB block size)
	 */
	for(count = 1; count < 3; count++) {

		address = 2048 * count;
		skipblks = address / blksize;
		offset = address % blksize;
		if ((fsi = open(dev, 0)) == -1) {
			pfmt(stderr, MM_ERROR,
				":24:Cannot open %s for input\n", dev);
			exit(31+1);
		}
		/* skip over tape header and any blocks before the potential */
		/*   start of i-list */
		read(fsi, buf, sizeof Tape_hdr);
		while (skipblks > 0) {
			read(fsi, buf, blksize);
			skipblks--;
		}

		if (read(fsi, buf, blksize) != blksize) {
			close(fsi);
			continue;
		}
		/* if first 2 inodes cross block boundary read next block also*/
		if ((offset + 2 * sizeof(struct dinode)) > blksize) {
			if (read(fsi, &buf[blksize], blksize) != blksize) {
				close(fsi);
				continue;
			}
		}
		close(fsi);
		inodes = (struct dinode *)&buf[offset];
		if((inodes[1].di_mode & S_IFMT) != S_IFDIR)
			continue;
		if(inodes[1].di_nlink < 2)
			continue;
		if((inodes[1].di_size % sizeof(struct direct)) != 0)
			continue;

		p1 = (char *) &address;
		p2 = inodes[1].di_addr;
		*p1++ = 0;
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1   = *p2;

		/* look for root directory at address specified by potential */
		/*   root inode */
		address = address << (count + 9);
		skipblks = address / blksize;
		offset = address % blksize;
		if ((fsi = open(dev, 0)) == -1) {
			pfmt(stderr, MM_ERROR,
				":24:Cannot open %s for input\n", dev);
			exit(31+1);
		}
		/* skip over tape header and any blocks before the potential */
		/*   root directory */
		read(fsi, buf, sizeof Tape_hdr);
		while (skipblks > 0) {
			read(fsi, buf, blksize);
			skipblks--;
		}

		if (read(fsi, buf, blksize) != blksize) {
			close(fsi);
			continue;
		}
		/* if first 2 directory entries cross block boundary read next 
		 *   block also 
		 */
		if ((offset + 2 * sizeof(struct direct)) > blksize) {
			if (read(fsi, &buf[blksize], blksize) != blksize) {
				close(fsi);
				continue;
			}
		}
		close(fsi);

		dirs = (struct direct *)&buf[offset];
		if(dirs[0].d_ino != 2 || dirs[1].d_ino != 2 )
			continue;
		if(strcmp(dirs[0].d_name,".") || strcmp(dirs[1].d_name,".."))
			continue;

		if (count == 1) {
			free(buf);
			return(Fs2b);
		}
		else if (count == 2) {
			free(buf);
			return(Fs4b);
		}
	}
	free(buf);
	return(-1);
}
