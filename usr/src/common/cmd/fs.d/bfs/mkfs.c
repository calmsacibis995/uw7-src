/*	copyright	"%c%"	*/

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/mkfs.c	1.15.5.7"
#include <stdio.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include "sys/fs/bfs.h"
#include <sys/vtoc.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>

#define	UMASK		0755
#define RND512(x)	((x) & ~(512 -1))
#define EXC512(x)	((x) & (512 -1))
#define BFSDOTSIZE	16	/* Max # of blks. for the ROOT directory */
#define	NAME_MAX	64
#define	FSTYPE		"bfs"
char sector[BFS_BSIZE];
off_t offset;
char zeros[BFS_BSIZE];

char *Usage =
	":1:Usage:\n%s [-F %s] [-m] special #blocks [#inodes]\n";
char *myname, fstype[]=FSTYPE;

main(argc, argv)
int argc;
char **argv;
{
	char *special;
	int size;
	int i;
	int files;
	time_t utime;
	int data_size;
	int ino_size;
	int fd;
	int maxinodes;
	struct bdsuper super, *bp;
	struct bfs_dirent dirent, *dir;
	struct bfs_ldirs ldirs, *lds;
	struct stat statarea;

	extern char *optarg;
	extern int optind, opterr;
	char mflag=0;
	int c;
	int errflg=0;
	char	label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmkfs");
	myname = (char *)strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s %s", fstype, myname);
	(void)setlabel(label);

	while ((c = getopt(argc, argv, "m")) != -1)
		switch (c) {
		case 'm':
			mflag++;
			break;
		case '?':
			errflg++;
			break;
		}
	if (errflg || argc < 3) {
		pfmt(stderr, MM_ACTION, Usage, myname, fstype);
		exit(31+8);
	}

	for (i=0; i < BFS_BSIZE; i++)
		zeros[i] = '\0';

	special= argv[optind++];

	if (mflag)
		exit (dump(special));

	if(stat(special, &statarea) < 0) {
		pfmt(stderr, MM_ERROR,
			":2:%s: cannot stat\n", special);
		exit(31+8);
	}
	if(statarea.st_flags & _S_ISMOUNTED) {
		pfmt(stderr, MM_ERROR,
			":3:%s: mounted file system\n", special);
		exit(31+2);
	}

	maxinodes = (BFSDOTSIZE * BFS_BSIZE) / sizeof(struct bfs_ldirs);
	size = atoi(argv[optind++]);
	if (argc > 3) {
		files = atoi(argv[optind]);
		if (files > maxinodes) {
			pfmt(stderr, MM_ERROR,
				":4:maximum # of inodes is %d\n",
					maxinodes);
			exit(31+8);
		}
	}
	else {
		files = size / 100;
		files = RND512(files * sizeof(struct bfs_dirent));
		files = files / sizeof(struct bfs_dirent);
		if (files < 48)
			files = 48;
		/*
		 * The number of inodes can not be greater than the
		 * maximum number of allowable directory entries.
		 */
		if (files > maxinodes)
			files =  maxinodes;
	}
	/*
	 * The number of inodes can not be greater than the maximum number
	 * of allowable directory entries.
	 */
	if (files > (BFSDOTSIZE * BFS_BSIZE) / sizeof(struct bfs_ldirs))
		files = (BFSDOTSIZE * BFS_BSIZE) / sizeof(struct bfs_ldirs);


	bp  = &super;
	dir = &dirent;
	/*
	 * Prepare the superblock info
	 */
	ino_size = files * sizeof(struct bfs_dirent);
	bp->bdsup_start = (off_t)BFS_DIRSTART + ino_size;
	data_size = (size * BFS_BSIZE) - bp->bdsup_start;
	bp->bdsup_end = (size * BFS_BSIZE) -1;
	ino_size = (ino_size / BFS_BSIZE) + 
			(((ino_size % BFS_BSIZE) + 511) / BFS_BSIZE);
	if ( data_size <= 0 || (ino_size + (2 * BFSDOTSIZE)) > size ) {

		pfmt(stderr, MM_ERROR,
			":5:%d is not big enough\n",size);
		pfmt(stderr, MM_ERROR,
			":6:file system size should be at least %d\n",
				1 + ino_size + (2 * BFSDOTSIZE));
		exit(31+8);
	}
	pfmt(stdout, MM_NOSTD, ":7:%d bytes per block\n", BFS_BSIZE);
	pfmt(stdout, MM_NOSTD, ":8:%d total blocks\n", size);
	pfmt(stdout, MM_NOSTD, ":9:%d total inodes\n", files);
	pfmt(stdout, MM_NOSTD,
		":10:%d free blocks\n", (data_size/BFS_BSIZE) - BFSDOTSIZE);
	pfmt(stdout, MM_NOSTD, ":11:%d free inodes\n", files -1);
	pfmt(stdout, MM_NOSTD, ":12:%d blocks for inodes\n", ino_size);
	pfmt(stdout, MM_NOSTD,
		":13:start byte  %d\nend byte  %d\n",
			bp->bdsup_start, bp->bdsup_end);

	bp->bdsup_bfsmagic = BFS_MAGIC;
	bp->bdcp_fromblock = -1;
	bp->bdcp_toblock = -1;
	bp->bdcpb_fromblock = -1;
	bp->bdcpb_toblock = -1;

	/*
	 * Prepare the ROOT inode info
	 */
	dir->d_ino = BFSROOTINO;
	dir->d_sblock = RND512(bp->bdsup_start) / BFS_BSIZE;
	if ( EXC512(bp->bdsup_start) > 0 )
		dir->d_sblock++;
	dir->d_eblock = dir->d_sblock + BFSDOTSIZE -1;
	dir->d_eoffset = (dir->d_sblock * BFS_BSIZE) + (2 * sizeof(ldirs)) -1;
	dir->d_fattr.va_type = VDIR;
	dir->d_fattr.va_mode = S_IFDIR | UMASK;
	dir->d_fattr.va_uid = 0;
	dir->d_fattr.va_gid = 1;
	dir->d_fattr.va_nlink = 2;
	time(&utime);
	dir->d_fattr.va_atime = utime;
	dir->d_fattr.va_mtime = utime;
	dir->d_fattr.va_ctime = utime;
	
	fd = creat(special, 0666);
	if (fd < 0)
	{
		pfmt(stderr, MM_ERROR, ":14:Could not create %s.\n",
			special);
		exit(31+8);
	}
	/*
	 * Zero out the first FS block.
	 */
	offset = 0;
	my_zero(fd);
	/*
	 * Write out the superblock
	 */
	offset = 0;
	lseek(fd, (long)0, 0);
	my_write(fd, &super, sizeof(super));

	/*
	 * Write out the ROOT inode
	 */
	my_write(fd, &dirent, sizeof(dirent));
	/*
	 * Zero out rest of the space to be used for inodes
	 */
	my_padd(fd);
	while (offset < bp->bdsup_start)
		my_zero(fd);

	/*
	 * Prepare and write out initial info of the DOT directory
	 */
	lds = &ldirs;
	lds->l_ino = BFSROOTINO;
	strncpy(lds->l_name, ".", BFS_MAXFNLEN);
	my_write(fd, &ldirs, sizeof(ldirs));
	strncpy(lds->l_name, "..", BFS_MAXFNLEN);
	my_write(fd, &ldirs, sizeof(ldirs));
	/*
	 * Zero out the rest of space preallocated for the DOT directory
	 */
	my_padd(fd);
	for (i=0; i<(dir->d_eblock - dir->d_sblock +1); i++)
		my_zero(fd);

	/*
	 * Make sure that a possible UFS superblock is zero out
	 */
	while (offset < BFS_BSIZE * 32)
		my_zero(fd);

	close(fd);
	exit (0);
}

my_zero(fd)
int fd;
{
	write(fd, zeros, BFS_BSIZE);
	offset += BFS_BSIZE;
}

my_padd(fd)
{
	my_write(fd, zeros, BFS_BSIZE - (offset % BFS_BSIZE));
}

my_write(fd, buffer, len)
int fd;
char *buffer;
int len;
{
	int i,j;

	j=0;

	for (i=offset % BFS_BSIZE; i < BFS_BSIZE && j < len; i++,j++)
		sector[i] = buffer[j];

	if (i == BFS_BSIZE)
	{
		write(fd, sector, BFS_BSIZE);
		if (len-j)
			my_write(fd, buffer+j, len-j);
		
	}

	offset +=  j;

	return (0);
}

dump(special)
char *special;
{

	int fd;
	struct bdsuper bd;
	int size;
	int inodes;

	if ((fd = open(special, O_RDONLY)) == -1) {
		pfmt(stderr, MM_ERROR,
			":15:Cannot open special file %s\n",
				special);
		return(31+8);
	}
	if ((read(fd, &bd, sizeof(struct bdsuper))) == -1) {
		pfmt(stderr, MM_ERROR,
			":16:read of special file %s failed\n",
				special);
		return(31+8);
	}
	if (bd.bdsup_bfsmagic != BFS_MAGIC) {
		pfmt(stderr, MM_ERROR,
			":17:%s is not a bfs file system\n",
				special);
		return(31+8);
	}
	size = (bd.bdsup_end +1)/BFS_BSIZE;
	inodes = (bd.bdsup_start - BFS_DIRSTART) / sizeof(struct bfs_dirent);

	printf("mkfs -F bfs %s %ld %ld\n", special, size, inodes);
	return(0);
}
