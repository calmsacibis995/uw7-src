/*		copyright	"%c%" 	*/

#ident	"@(#)fstyp:common/cmd/fstyp/S5fstyp.c	1.3.7.2"
#ident "$Header$"

/* s5: fstyp */
#include	<sys/param.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/fs/s5param.h>
#include	<sys/fs/s5ino.h>
#include 	<sys/vnode.h>
#include	<sys/fs/s5dir.h>
#include	<stdio.h>
#include	<setjmp.h>
#include	<sys/fs/s5filsys.h>
#include	<sys/stat.h>
#include	<sys/fcntl.h>

void	exit();
long	lseek();

main(argc,argv)
int	argc;
char	*argv[];
{

	int	fd;
	int 	vflag = 0;
	int 	i;
	int	result;
	long	bsize = 0;
	char	*dev;
	struct	stat	buf;
	struct	filsys	sblock;

	if (argc == 3) {
		dev = argv[2];
		vflag = 1;
	} 
	else
		dev = argv[1];
		
	/*
	 *	Read the super block associated with the device. 
	 */

	if ((fd = open(dev, O_RDONLY)) < 0) {
		fprintf(stderr,"s5 fstyp: cannot open <%s>\n",dev);
		exit(1);
	}
	
	if (lseek(fd, (long)SUPERBOFF, 0) < 0
		|| read(fd, &sblock, (sizeof sblock)) != (sizeof sblock)) {
		fprintf(stderr,"s5 fstyp: cannot read superblock\n");
		close(fd);
		exit(1);
	}
	
	/*
	 *	Determine block size for sanity check and if it is type s5
 	 */
	
	if(sblock.s_magic == FsMAGIC) {
		switch(s5bsize(fd, &sblock)){
		case Fs1b:
			bsize = 512;
			break;
		case Fs2b:
			bsize = 1024;
			break;
		case Fs4b:
			bsize = 2048;
			break;
		default:
			exit(1);
		}
		fprintf(stdout,"s5\n");
	} else {
		exit(1);
	}
	if (vflag) {
		printf("file system name (s_fname): %s\n", sblock.s_fname);
		printf("file system packname (s_fpack): %s\n", sblock.s_fpack);
		printf("magic number (s_magic): 0x%x\n", sblock.s_magic);
		printf("type of new file system (s_type): %ld\n", sblock.s_type);
		printf("file system state (s_state): 0x%x\n", sblock.s_state);
		printf("size of i-list (in blocks) (s_isize): %d\n", sblock.s_isize);
		printf("size of volume (in blocks) (s_fsize): %ld\n", sblock.s_fsize);
		printf("lock during freelist manipulation (s_flock): %d\n", sblock.s_flock);
		printf("lock during i-list manipulation (s_ilock): %d\n", sblock.s_ilock);
		printf("super block modified flag (s_fmod): %d\n", sblock.s_fmod);
		printf("super block read-only flag (s_ronly): %d\n", sblock.s_ronly);
		printf("last super block update (s_time): %d\n", sblock.s_time);
		printf("total free blocks (s_tfree): %ld\n", sblock.s_tfree);
		printf("total free inodes (s_tinode): %d\n", sblock.s_tinode);
		printf("gap in physical blocks (s_dinfo[0]): %d\n", sblock.s_dinfo[0]);
		printf("cylinder size in physical blocks (s_dinfo[1]): %d\n", sblock.s_dinfo[1]);
		printf("(s_dinfo[2]): %d\n", sblock.s_dinfo[2]);
		printf("(s_dinfo[3]): %d\n", sblock.s_dinfo[3]);
		printf("number of addresses in s_free (s_nfree): %d\n", sblock.s_nfree);
		printf("freelist (s_free[NICFREE)): \n");
		for (i=1; i< NICFREE; i++) {
			printf("%ld ", sblock.s_free[i]);
			if ((i%8) == 0)
				printf("\n");
		}
		printf("\nnumber of inodes in s_inode (s_ninode): %d\n", sblock.s_ninode);
	  	printf("free i-node list (s_inode[NICINOD]):\n");
	
		for (i=1; i< NICINOD; i++) {
			printf("%ld ", sblock.s_inode[i]);
			if ( (i%8) == 0)
				printf("\n");
		}
		printf("\n\nfill (s_fill[12]):\n");
		for (i=0; i< 12; i++) {
			printf("%ld ", sblock.s_fill[i]);
			if ((i != 0) && ((i%6) == 0))
				printf("\n");
		}
		printf("\n");
		
	}
	close(fd);

	exit(0);
}
