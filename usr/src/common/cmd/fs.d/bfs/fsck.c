/*	copyright	"%c%"	*/

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/fsck.c	1.9.8.1"

/***************************************************************************
 * Command: fsck
 * Inheritable Privileges: P_DACREAD,P_DACWRITE,P_MACWRITE,P_COMPAT,P_DEV
 *       Fixed Privileges: None
 *
 ***************************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fs/bfs.h>
#include <sys/stat.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <nl_types.h>
#include <langinfo.h>
#include <regex.h>

#define	NAME_MAX	64
#define	FSTYPE		"bfs"

char *myname, fstype[]=FSTYPE;
char *Usage = ":1:Usage:\n%s [-F %s] [generic options] [-y | -n] special\n";
extern	char *strrchr();

struct sanityw {
	daddr_t fromblock;
	daddr_t toblock;
	daddr_t bfromblock;
	daddr_t	btoblock;
};

#ifndef BFSBUFSIZE
#	define	BFSBUFSIZE 8192
#endif

char bfs_buffer[BFSBUFSIZE];
char superblk[512];
int superblkno = -1;
char buf4ino[512];
int bufblkno=-1;

/*
 * Procedure:     main
 *
 * Restrictions:
 *                getopt: none
 *                fprintf: none
 *                stat(2): none
 *                open(2): none
 *                printf: none
*/

main(argc, argv)
char **argv;
int argc;
{
	int fd;
	struct bdsuper bd;
	char *special = NULL;
	char mflag = 0;
	char yflag = 0;
	char nflag = 0;
	char mod = 0;
	int r;
	struct stat statarea;
	extern char *optarg;
	extern int optind, opterr;
	int cc;
	int errflg=0;
	char string[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxfsck");
	myname = strrchr(argv[0],'/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(string, "UX:%s %s", fstype, myname);
	(void)setlabel(string);

	/*
	 * To enforce least privilege, all privileges are cleared at the start
	 * of execution. Needed privileges will be reinstated only when
	 * needed.
	 */
	while ((cc = getopt(argc, argv, "mny")) != -1)
		switch (cc) {
		case 'm':
			if (nflag || yflag)
				errflg++;
			else
				mflag++;
			break;
		case 'n':
			if (mflag || yflag)
				errflg++;
			else
				nflag++;
			break;
		case 'y':
			if (mflag || nflag)
				errflg++;
			else
				yflag++;
			break;
		case '?':
			errflg++;
			break;
		}

	if (errflg || (argc -optind) < 1) {
		pfmt(stderr, MM_ACTION, Usage, myname, fstype);
		exit (31+8);
	}

	special= argv[optind];

	if (special == NULL) {
		pfmt(stderr, MM_ACTION, Usage, myname, fstype);
		exit(31+8);
	}

	if(stat(special, &statarea) < 0) {
		pfmt(stderr, MM_ERROR, ":2:%s: can not stat\n", special);
		exit(31+8);
	}

	if((statarea.st_mode & S_IFMT) == S_IFBLK) {
		if ((statarea.st_flags & _S_ISMOUNTED) && !nflag) {
			    pfmt(stderr, MM_ERROR, ":3:%s: mounted file system\n", special);
			exit(31+2);
		}
	}
	else if((statarea.st_mode & S_IFMT) == S_IFCHR) {
		if ((statarea.st_flags & _S_ISMOUNTED) && !nflag) {
			pfmt(stderr, MM_ERROR,
			    ":4:%s file system is mounted as a block device, ignored\n",
				special);
			exit(31+3);
		}
	}
	else 
	{
	      pfmt(stderr, MM_ERROR,
		    ":5:%s is not a block or character device\n",
			special);
		exit(31+8);
	}

	fd = open(special, O_RDWR, O_SYNC);

	if (fd < 0)
	{
		pfmt(stderr, MM_ERROR,
			":6:can not open special file %s\n", special);
		pfmt(stderr, MM_NOGET|MM_ERROR, "%s\n", strerror(errno));
		exit(31+8);
	}

	seek_read(fd, BFS_SUPEROFF, &bd, sizeof(struct bdsuper));

	if (bd.bdsup_bfsmagic != BFS_MAGIC) {
		pfmt(stderr, MM_ERROR, ":7:%s is not a %s file system\n",
			special, fstype);
		exit(31+8);
	}

	if (!mflag)
		pfmt(stdout, MM_INFO, ":8:Checking %s:\n", special);

	r = check_compaction(fd, &bd, mflag, nflag, yflag, &mod);
	if (mflag)
	{
		if (r)
		{
			pfmt(stderr, MM_ERROR, ":9:%s not okay\n", special);
			exit(r);
		}

		pfmt(stdout, MM_INFO, ":10:%s okay\n", special);
		exit(0);
	}

	check_dirents(fd, &bd);
	if (mod)
		pfmt(stdout,MM_INFO,":11:File system was modified.\n");
	else
		pfmt(stdout,MM_INFO,":12:File system was not modified.\n");

	exit(0);
}

/*
 * Procedure:     check_compaction
 *
 * Restrictions:
 *                printf: none
 *                fflush: none
 *                scanf: none
*/

check_compaction(fd, bd, mflag, nflag, yflag, mod)
int fd;
struct bdsuper *bd;
char mflag,nflag,yflag,*mod;
{
	struct bfs_dirent dir;
	char ans[10];
	register int i;
	long fromblock,toblock;
	struct sanityw sw;
	int len;
	regex_t yesre;
	int err;
	

	if ((bd->bdcpb_fromblock == -1) || (bd->bdcpb_toblock == -1))
		return(0);

	if (mflag)
		return(31+1);

	pfmt(stdout, MM_INFO,
		":13:File system was in the middle of compaction.\n");

	if (nflag)
		return (0);

	err = regcomp(&yesre, nl_langinfo(YESEXPR), REG_EXTENDED | REG_NOSUB);
	if (err != 0) {
		char buf[BUFSIZ];

		regerror(err, &yesre, buf, BUFSIZ);
		pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
		exit(2);
	}
	if (!yflag)
	{
		pfmt(stdout, MM_NOSTD, ":14:Complete compaction? ");
		fflush(stdout);
		scanf("%s", ans);
		len = strlen(ans);
		if (len && regexec(&yesre, ans, (size_t)0, (regmatch_t *)0, 0) == 0)
			return(0);
	}
	if ((bd->bdcp_fromblock == -1) || (bd->bdcp_toblock == -1))
	{
		fromblock = bd->bdcpb_fromblock;
		toblock = bd->bdcpb_toblock;
		sw.fromblock = fromblock;
		sw.bfromblock = fromblock;
		sw.toblock = toblock;
		sw.btoblock = toblock;
		wr_sanityw(fd, BFS_SANITYWSTART,
		   &sw, sizeof(struct sanityw));
	}
	else
	{
		fromblock = bd->bdcp_fromblock;
		toblock = bd->bdcp_toblock;
	}


	for (i=BFS_DIRSTART; i < bd->bdsup_start; i+= sizeof(struct bfs_dirent))
	{
		get_ino(fd, i, &dir, sizeof(struct bfs_dirent));
		if ((dir.d_sblock <= fromblock) && (dir.d_eblock > fromblock))
			break;
	}

	if ((dir.d_sblock > fromblock) || (dir.d_eblock <= fromblock))
	{
		/*
		 * Data blocks of file were already shifted and the inode
		 * of the file was updated. However, the system must have
		 * have crashed just before the sanity words were updated.
		 * Therefore will update them now.
		 */
		sw.fromblock = -1;
		sw.bfromblock = -1;
		sw.toblock = -1;
		sw.btoblock = -1;
		wr_sanityw(fd, BFS_SANITYWSTART,
		   &sw, sizeof(struct sanityw));
		(*mod)++;
		return (0);
	}

	pfmt(stdout, MM_INFO,
	    ":15:Finishing compaction of file (inode %ld)\n", dir.d_ino);

	bfs_shiftfile(fd, &dir, fromblock, i, toblock);

	(*mod)++;
	return (0);
}


/*
 * Procedure:     check_dirents
 *
 * Restrictions:
 *                printf: none
*/

check_dirents(fd, bd)
int fd;
struct bdsuper *bd;
{
	register int i;
	struct bfs_dirent dir;
	long freeblocks, freefiles;
	long totblocks, totfiles;

	freeblocks = (bd->bdsup_end + 1 - bd->bdsup_start) / BFS_BSIZE;
	totblocks =  (bd->bdsup_end +1) / BFS_BSIZE;
	totfiles = (bd->bdsup_start - BFS_DIRSTART) / sizeof(struct bfs_dirent);
	freefiles = totfiles;

	for (i=BFS_DIRSTART; i < bd->bdsup_start; i+=sizeof(struct bfs_dirent))
	{
		get_ino(fd, i, &dir, sizeof(struct bfs_dirent));
		if (dir.d_ino == 0)
			continue;
		freefiles--;
		if (dir.d_eblock != 0)
			freeblocks -= (dir.d_eblock + 1 - dir.d_sblock);
	}
	pfmt(stdout, MM_INFO,
	    ":16:%ld total blocks\n%ld free blocks\n%ld total inodes\n",
		totblocks, freeblocks, totfiles);
	pfmt(stdout, MM_INFO,":17:%ld free inodes\n", freefiles);
}


/*
 * Procedure:     min
 *
 * Restrictions:	none
*/

min(a,b)
int a,b;
{
	if (a > b)
		return (b);
	else
		return (a);
}


/*
 * Procedure:     bfs_shiftfile
 *
 * Restrictions:	none
 *
 * Notes: Shift the file described by dirent "dir", begining from 
 *        "fromblock" to "toblock".  "Offset" describes the location 
 *        on the disk of the dirent.
 */

bfs_shiftfile(fd, dir, fromblock, offset, toblock)
	int fd;
	struct bfs_dirent *dir;
	daddr_t fromblock;
	off_t offset;
	daddr_t toblock;
{
	long gapsize;
	long maxshift;
	long filesize;
	off_t eof;
	long w4fsck[2];
	struct sanityw sw;

	gapsize = fromblock - toblock;
	maxshift = min(BFSBUFSIZE, gapsize*512);

	/*
	 * Write sanity words for fsck to denote compaction is in progress.
	 */
	sw.fromblock = fromblock;
	sw.toblock = toblock;
	sw.bfromblock = sw.fromblock;
	sw.btoblock = sw.toblock;
	wr_sanityw(fd, BFS_SANITYWSTART,
		      &sw, sizeof(struct sanityw));

	/*
	 * Calculate the new EOF.
	 */
	if (dir->d_eoffset / 512 == dir->d_eblock &&
	    dir->d_eblock >= sw.fromblock) {
		eof = (dir->d_eoffset - (dir->d_sblock * 512)) +
		       ((dir->d_sblock - gapsize) * 512);
		dir->d_eoffset = eof;
	}

	w4fsck[0] = -1;
	w4fsck[1] = -1;
	filesize = (dir->d_eblock - dir->d_sblock +1) * 512;

	/*
	 * Write as many sectors of the file at a time.
	 */
	while (sw.fromblock != (dir->d_eblock + 1)) {
		/*
		 * Must recalculate "maxshift" each time.
		 */
		maxshift = min(maxshift, (dir->d_eblock-sw.fromblock+1)*512);

		/*
		 * If gapsize is less than file size, must write words for fsck
		 * to denote that compaction is in progress (i.e which blocks
		 * are being shifted.)
		 * Otherwise, there is no need to write sanity words. If the
		 * system crashes during compaction, fsck can take it from 
		 * the top without data lost. 
		 */
		if (gapsize*512 < filesize) {
			sw.bfromblock = sw.fromblock;
			sw.btoblock = sw.toblock;
			wr_sanityw(fd, BFS_SANITYWSTART,
			  &sw, sizeof(struct sanityw));
		}

		seek_read(fd, sw.fromblock*BFS_BSIZE,
		  bfs_buffer, maxshift); 

		seek_write(fd, sw.toblock*BFS_BSIZE,
		  bfs_buffer, maxshift);

		sw.fromblock+= (maxshift / BFS_BSIZE);
		sw.toblock+= (maxshift / BFS_BSIZE);

		/*
		 * If gapsize is less than file size, must write a "-1" to
		 * the first 2 sanity words to let fsck know where compaction
		 * is.
		 */
		if (gapsize*512 < filesize)
			wr_sanityw(fd, BFS_SANITYWSTART,
			  w4fsck, sizeof(w4fsck));
	}

	/*
	 * Calculate the new values for inode and write it to disk.
	 */
	dir->d_sblock -= gapsize;
	dir->d_eblock -= gapsize;
	put_ino(fd, offset, dir, sizeof(struct bfs_dirent));

	/*
	 * Must write "-1" to all 4 sanity words for fsck to denote that
	 * compaction is not in progress.
	 */
	sw.fromblock = -1;
	sw.toblock = -1;
	sw.bfromblock = -1;
	sw.btoblock = -1;
	wr_sanityw(fd, BFS_SANITYWSTART,
	  &sw, sizeof(struct sanityw));
	return 0;
}


/*
 * Procedure:     seek_read
 *
 * Restrictions:
 *                read(2): none
*/

int
seek_read(fd, offset, buf, len)
	int fd;
	off_t offset;
	char *buf;
	long len;
{
	lseek(fd, offset, 0);
	read(fd, buf, len);
	return 0;
}


/*
 * Procedure:     seek_write
 *
 * Restrictions:
 *                write(2): none
*/

int
seek_write(fd, offset, buf, len)
	int fd;
	off_t offset;
	char *buf;
	long len;
{
	lseek(fd, offset, 0);
	write(fd, buf, len);
	return 0;
}


/*
 * Procedure:     get_ino
 *
 * Restrictions:
 *                read(2): none
*/

int
get_ino(fd, ioffset, ibuf, len)
int  fd;
int ioffset;
char ibuf[];
int  len;
{
	int i = 0;
	int j;
	long iblk;

	iblk = ioffset / 512;
	if (bufblkno != iblk) {
		seek_read(fd, (iblk * 512), buf4ino, 512);
		bufblkno = iblk;
	}

	j = ioffset % 512;
	while (i < len) {
		if (j < 512)
			ibuf[i++] = buf4ino[j++];
		else {
			read(fd, buf4ino, 512);
			j = 0;
			bufblkno++;
		}
	}
	return(0);
}

/*
 * Procedure:     wr_sanityw
 *
 * Restrictions:
*/

int
wr_sanityw(fd, swoffset, swbuf, len)
int  fd;
int swoffset;
char swbuf[];
int  len;
{
	int i = 0;
	int j;

	if (superblkno == -1) {
		seek_read(fd, BFS_SUPEROFF, superblk, 512);
		superblkno = 0;
	}
	j = swoffset;
	while (i < len)
		superblk[j++] = swbuf[i++];
	seek_write(fd, BFS_SUPEROFF, superblk, 512);
}


/*
 * Procedure:     put_ino
 *
 * Restrictions:	none
*/

int
put_ino(fd, ioffset, ibuf, len)
int  fd;
int ioffset;
char ibuf[];
int  len;
{
	int i = 0;
	int j;
	long iblk;
	char onekbuf[1024];

	iblk = ioffset / 512;
	seek_read(fd, (iblk * 512), onekbuf, 1024);

	j = ioffset % 512;
	while (i < len)
		onekbuf[j++] = ibuf[i++];
	seek_write(fd, (iblk * 512), onekbuf, 1024);

	return(0);
}
