/*		copyright	"%c%" 	*/

#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/ncheck.c	1.24.1.7"
#ident "$Header$"
/*
 * ncheck -- obtain file names from reading filesystem
 */

/* number of inode blocks to process at a time */
#define	NI	16
#define	NB	300

#define	HSIZE	1740

#include <stdio.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5dir.h>
#include <sys/fs/s5filsys.h>
#include <sys/stat.h>

#define BSIZE 512
/* FSBSIZE is maximum logical block size */
#define FSBSIZE 2048

#define	NIDIR	(FSBSIZE/sizeof(daddr_t))
#define	NBINODE	(FSBSIZE/sizeof(struct dinode))
#define	NDIR	(FSBSIZE/sizeof(struct direct))

struct	filsys	sblock;
struct	dinode	itab[NBINODE*NI];
daddr_t	iaddr[NADDR];
ino_t	ilist[NB];
struct	htab
{
	ino_t	h_ino;
	ino_t	h_pino;
	char	h_name[DIRSIZ];
} htab[HSIZE];

int	aflg;
int	iflg;
int	sflg;
int	eflg = 0;
int	fi;
ino_t	ino;
int	nhent;
int	nxfile;

int	bsize, physblks;
int	nidir;
int	nbinode;
int	ndir;

int	nerror;
daddr_t	bmap();
long	atol();
struct htab *lookup();

main(argc, argv)
char *argv[];
{
	register i;
	register FILE *fp;
	char filename[50];
	long n;
	char *aptr;
	extern char *strchr();

	while (--argc) {
		argv++;
		if (**argv=='-')
			switch ((*argv)[1]) {
			case 'a':
				aflg++;
				if ((*argv)[2] == 's') {
					sflg++;
				}
				continue;

			case 'i':
				for(i=0; i<NB;) {
					if (argv[1] == NULL)
						break;
					aptr=strchr(argv[1], ',');
					if (aptr != 0)
						*aptr++='\0';
					n = atol(argv[1]);
					if(n == 0)
						break;
					argv[1]=aptr;
					ilist[i] = n;
					iflg = ++i;
				}
				eflg++;
				if (eflg > 1) {
					usage();
					break;
				}
				argv++;
				argc--;
				continue;

			case 's':
				sflg++;
				if ((*argv)[2] == 'a') {
					aflg++;
				}
				continue;
			case '?':	/* s5 usage message */
				nerror++;
				usage();
				break;

			default:
				fprintf(stderr, "s5 ncheck: illegal  option --%c\n", (*argv)[1]);
				nerror++;
				usage();
			}
		else break;
	}
	nxfile = iflg;
	ilist[nxfile] = 0;
	if(argc) {		/* arg list has file names */
		while(argc-- > 0)
			check(*argv++);
	}
	return(nerror);
}

usage()
{
	fprintf(stderr, "s5 Usage:\n");
	fprintf(stderr, "ncheck [-F s5] [generic_options] [-i i-numbers ...] [-a] [-s] [special ...]\n");
	exit(31+nerror);
}

check(file)
char *file;
{
	register i, j;
	ino_t mino;
	int pass;

	fi = open(file, 0);
	if(fi < 0) {
		fprintf(stderr, "s5 ncheck: cannot open %s\n", file);
		nerror++;
		return;
	}
	nhent = 0;
	printf("%s:\n", file);
	sync();
	lseek(fi, (long)SUPERBOFF, 0);
	if (read(fi, (char *)&sblock, sizeof(sblock)) != sizeof(sblock)) {
		fprintf(stderr, "s5 ncheck: read error super-block\n");
		close(fi);
		return;
	}
	if(sblock.s_magic != FsMAGIC) {
		fprintf(stderr, "s5 ncheck: %s is not an s5 file system\n",file);
		exit(34);
	}
	if(sblock.s_type == Fs1b) {
		physblks = 1;
		bsize = 512;
	}
	else if(sblock.s_type == Fs2b) {
		physblks = 2;
		bsize = 1024;
	}
	else if(sblock.s_type == Fs4b) {
		physblks = 4;
		bsize = 2048;
	}
	else {
		printf("(%-10s): bad block type\n", file);
		close(fi);
		return;
	}
	nidir = bsize / sizeof(daddr_t);
	nbinode = bsize / sizeof(struct dinode);
	ndir = bsize / sizeof(struct direct);
	mino = (sblock.s_isize-2) * nbinode;
	ino = 0;
	for(i=2;; i+=NI) {
		if(ino >= mino)
			break;
		bread((daddr_t)i, (char *)itab, (sizeof(struct dinode) * nbinode * NI));
		for(j=0; j<nbinode*NI; j++) {
			if(ino >= mino)
				break;
			ino++;
			pass1(&itab[j]);
		}
	}
	ilist[nxfile] = 0;
	for(pass = 2; pass <= 3; pass++) {
		ino = 0;
		for(i=2;; i+=NI) {
			if(ino >= mino)
				break;
			bread((daddr_t)i, (char *)itab, 
			    (sizeof(struct dinode) * nbinode * NI));
			for(j=0; j<nbinode*NI; j++) {
				if(ino >= mino)
					break;
				ino++;
				if((itab[j].di_mode&S_IFMT) == S_IFDIR)
					nxtpass(pass, &itab[j]);
			}
		}
	}
	close(fi);
	reinit();
}

pass1(ip)
register struct dinode *ip;
{
	if((ip->di_mode & S_IFMT) != S_IFDIR) {
		if (sflg != 1) return;
		if (nxfile >= NB){
			++sflg;
			fprintf(stderr,"Too many special files (increase ilist array)\n");
			return;
		}
		if ((ip->di_mode&S_IFMT)==S_IFBLK || (ip->di_mode&S_IFMT)==S_IFCHR
		    || ip->di_mode&(S_ISUID|S_ISGID))
			ilist[nxfile++] = ino;
		return;
	}
	lookup(ino, 1);
}

nxtpass(pass,ip)
register struct dinode *ip;
{
	struct direct dbuf[NDIR];
	long doff;
	struct direct *dp;
	register i, j;
	int k;
	struct htab *hp;
	daddr_t d;
	ino_t kno;

	l3tol(iaddr, ip->di_addr, NADDR);
	doff = 0;
	for(i=0;; i++) {
		if((doff >= ip->di_size)
		    || ((d = bmap(i)) == 0))
			break;
		bread(d, (char *)dbuf, (sizeof(struct direct) * ndir));
		for(j=0; j<ndir; j++) {
			if(doff >= ip->di_size)
				break;
			doff += sizeof(struct direct);
			dp = dbuf+j;
			kno = dp->d_ino;
			if(kno == 0)
				continue;
			switch(pass) {
			case 2:
				if(((hp = lookup(kno, 0)) == 0)
				    || (dotname(dp)))
					continue;
				hp->h_pino = ino;
				for(k=0; k<DIRSIZ; k++)
					hp->h_name[k] = dp->d_name[k];
				break;
			case 3:
				if(aflg==0 && dotname(dp))
					continue;
				if(ilist[0] == 0 && sflg==0)
					goto pr;
				for(k=0; ilist[k] != 0; k++)
					if(ilist[k] == kno)
						goto pr;
				continue;
pr:
				printf("%u	", kno);
				pname(ino, 0);
				printf("/%.14s", dp->d_name);
				if (lookup(kno, 0))
					printf("/.");
				printf("\n");
			}
		}
	}
}

reinit()
{
	register struct htab *hp, *hplim;
	int i;

	hplim = &htab[HSIZE];

	for(hp = &htab[0]; hp < hplim; hp++)
		hp->h_ino = 0;
	nxfile = iflg;
	ilist[nxfile] = 0;
	if(sflg)
		sflg = 1;
}

dotname(dp)
register struct direct *dp;
{

	if (dp->d_name[0]=='.')
		if (dp->d_name[1]==0 || (dp->d_name[1]=='.' && dp->d_name[2]==0))
			return(1);
	return(0);
}

pname(i, lev)
ino_t i;
{
	register struct htab *hp;

	if (i==S5ROOTINO)
		return;
	if ((hp = lookup(i, 0)) == 0) {
		printf("???");
		return;
	}
	if (lev > 10) {
		printf("...");
		return;
	}
	pname(hp->h_pino, ++lev);
	printf("/%.14s", hp->h_name);
}

struct htab *
lookup(i, ef)
ino_t i;
{
	register struct htab *hp;

	for (hp = &htab[i% (ino_t)HSIZE]; hp->h_ino;) {
		if (hp->h_ino==i)
			return(hp);
		if (++hp >= &htab[HSIZE])
			hp = htab;
	}
	if (ef==0)
		return(0);
	if (++nhent >= HSIZE) {
		fprintf(stderr, "s5 ncheck: out of core-- increase HSIZE\n");
		exit(31+1);
	}
	hp->h_ino = i;
	return(hp);
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
	register i;

	lseek(fi, bno*bsize, 0);
	if (read(fi, buf, cnt) != cnt) {
		fprintf(stderr, "s5 ncheck: read error %d\n", bno);
		for(i=0; i<bsize; i++)
			buf[i] = 0;
	}
}

daddr_t
bmap(i)
{
	daddr_t ibuf[NIDIR];

	if(i < NADDR-3)
		return(iaddr[i]);
	i -= NADDR-3;
	if(i > nidir) {
		fprintf(stderr, "s5 ncheck: %u - huge directory\n", ino);
		return((daddr_t)0);
	}
	bread(iaddr[NADDR-3], (char *)ibuf, (sizeof(daddr_t) * nidir));
	return(ibuf[i]);
}

