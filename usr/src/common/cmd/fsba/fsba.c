/*		copyright	"%c%" 	*/

#ident	"@(#)fsba:common/cmd/fsba/fsba.c	1.10.1.4"
#ident "$Header$"
/* fsba.c
 *		fsba (file system block analyzer) determines the 
 *	number of sectors (1 sector has 512 bytes) that would be needed 
 *	in a new file system to store all of the data presently in an
 *	existing file system when the file systems are of differing logical
 *	block  sizes. Logical block sizes of 512, 1024, and 2048 bytes
 *	for either the new or existing file systems are supported. The
 *	block  size  for the new file system can be specified with the -b option
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
/*
#include <sys/vnode.h>
#include <sys/fs/s5inode.h>
*/
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>
#include <fcntl.h>
#include "fsba.h"

#define	NBINODE(b)	(b/sizeof(struct dinode))
#define	INDIR(b)	(b/sizeof(daddr_t))
#define MAXINO  (1024/sizeof(struct dinode)) * 10
char *dash ="--------------------------------------------";

int	obsize, nbsize;		/* logical block size of (old,new) file system */
int	of_blks, nf_blks;	/* number of file blocks in (old,new) file system */
int	fp;
int	n_inodes;

main(argc, argv)
int argc;
char *argv[];
{
	register struct dinode *inop;
	struct dinode *getino();
	int blkold, blknew, blkwaste;
	int filnum;
	int old, new, waste;
	int nino ;
	int c;
	int bflag = 0;
	extern	char	*optarg;
	extern	int	optind;
	float pwaste;

	while ((c = getopt(argc, argv, "b:")) != -1) {
		if (c == 'b') {
			bflag++;
			nbsize = atoi(optarg);
			if (nbsize != 512 && nbsize != 1024 && nbsize != 2048) {
				fprintf(stderr, "%d is invalid target block size\n", nbsize);
				exit(1);
			}
		}
		else {
			fprintf(stderr, "usage: %s [-b target block size] file-system1 [file-system2 ...]\n", argv[0]);
			exit(1);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "usage: %s [-b target block size] file-system1 [file-system2 ...]\n", argv[0]);
		exit(1);
	}
	if (bflag == 0)
		nbsize = DEF_BLOCKSIZE;

	for (; optind < argc; optind++) {
		printf("\n\n");
		blkold = blknew = blkwaste = filnum =  0;
		if ((nino = init(argv[optind])) == 0)
			continue;
		if (lseek(fp, (long)(2 * obsize), 0) < 0) {
			fprintf(stderr, "can't seek to start of ilist on %s\n", argv[optind]);
			continue;
		}
		printf(" total inodes :          %d\n", nino);
		while ((inop = getino()) != NULL) {
			switch(inop->di_mode & S_IFMT) {
			case S_IFDIR:	/* directory */
			case S_IFREG:   /* regular file */
				old = fileblk(inop->di_size, obsize);
				new= fileblk(inop->di_size, nbsize);
				waste = new - old;
				blkold += old;
				blknew += new;
				blkwaste += waste;
			case S_IFBLK:   /* block special */
			case S_IFCHR:   /* character special */
			case S_IFIFO:   /* pipe */
				filnum++;
				break;
			default:
				/* must be free inode */
				break;
			}
		}
		pwaste = 100.0 * blkwaste / blkold;
		printf(" allocated inodes :      %d\n", filnum);
		printf(" free inodes :           %d\n", nino-filnum);
		printf("%s\n", dash);
		printf(" for old file system (%d bytes/block)\n", obsize);
		printf(" allocated sectors :     %d\n", blkold);
		printf(" free sectors :          %d\n", of_blks - blkold);
		printf("%s\n", dash);
		printf(" for new file system (%d bytes/block)\n", nbsize);
		printf(" allocated sectors :     %d\n", blknew);
		printf(" free sectors :          %d\n", nf_blks-blknew);
		if(nf_blks < blknew) 
			printf(" the entire volume size is not big enough\n");
		printf("%s\n", dash);
		printf(" extra sectors needed for new file system :\n");
		printf("                         %d   (%4.1f%%)\n", blkwaste, pwaste);
		printf(" * 1 sector contains 512 bytes\n");
	}
	exit(0);
}

/* n argument is byte size of a file, fileblk function
 * returns the number of blocks allocated 
 */
fileblk(n, bsize)
long n;
int bsize;
{
	int db, bt, indirt, physblks;

	indirt = INDIR(bsize);
	physblks = SECTPERLOG(bsize);
	db = round(n, bsize); 	/* compute the data blocks */
	if(db <= 10)		/* direct addresses */
		return(physblks * db);
	if(db <= 10 + indirt)	/* single indirect address */
		return(physblks * (db + 1));
	if(db <= 10 + indirt * indirt) {	/* double indirect address */
		bt = round(db -10, indirt);
		return(physblks * (db +1 + bt));
	}
	/* triple indirect address */
	bt = round(db-10, indirt);
	bt += round(bt, indirt);
	return(physblks * (db + 1 + bt));
}

round(d1, d2)
long	d1;
int	d2;
{
	if (d1 % d2 == 0)
		return(d1/d2);
	else
		return(d1/d2 + 1);
}



struct dinode *
getino()	/* read 10 blocks of inodes at a time */
{
	static struct dinode buf[MAXINO];
	static int ino_index = 0;

	if (n_inodes-- == 0)
		return(0);
	if ((ino_index == MAXINO) || (ino_index == 0)) {
		if (n_inodes >= MAXINO) {
			if (read(fp, buf, sizeof buf) < sizeof buf) {
				printf("read inode blocks error\n");
				return(0);
			}
			ino_index = 0;
		}
		else {
	   		if(read(fp, buf, sizeof(struct dinode) * (n_inodes + 1))
				< sizeof(struct dinode) * (n_inodes + 1)) {
	   			printf("read inode blocks error\n");
	   			return(0);
			}
			ino_index = 0;
		}
	}
	return(&buf[ino_index++]);	/* return the address of next inode */
}




init(f)
char *f;
{
	struct filsys super;
	struct stat sbuf;
	int	nsects;

	if (stat(f, &sbuf) != 0) {
		fprintf(stderr, " can't stat file %s\n", f);
		return(0);
	}
	if (((sbuf.st_mode & S_IFMT) != S_IFCHR) &&
		((sbuf.st_mode & S_IFMT) != S_IFBLK)) {
		fprintf(stderr, " : block special or character special \n");
		return(0);
	}
	if ((fp = open(f, O_RDONLY)) < 0) {
		fprintf(stderr, " can't open file %s \n", f);
		return(0);
	}
	if (lseek(fp, (long)512, 0) < 0) {
		fprintf(stderr, " can't seek file %s\n", f);
		return(0);
	}
	if (read(fp, &super, sizeof super) < sizeof super) {
		fprintf(stderr, " can't read superblock of %s\n", f);
		return(0);
	}
	if (super.s_magic != FsMAGIC)
		super.s_type = Fs1b;

	switch (s5bsize(fp, &super)){
	case Fs1b:
		obsize = 512;
		break;
	case Fs2b:
		obsize = 1024;
		break;
	case Fs4b:
		obsize = 2048;
		break;
	default:
	     fprintf(stderr, "can't determine logical block size of %s\n", f);
	     return(0);
	}

	n_inodes = (super.s_isize - 2) * NBINODE(obsize);
	nsects = super.s_fsize * SECTPERLOG(obsize);

	if (nbsize == obsize) {
		printf("%s is a %d byte block file system already\n", f,nbsize);
		return(0);
	}
	of_blks = (super.s_fsize - super.s_isize) * SECTPERLOG(obsize);
	nf_blks = (nsects/SECTPERLOG(nbsize) - (n_inodes/NBINODE(nbsize) + 2))*SECTPERLOG(nbsize);
	printf(" file-sys name :         %.6s   (%s)\n", super.s_fname, f);
	return(n_inodes);
}
