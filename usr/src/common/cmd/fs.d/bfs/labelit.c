/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/labelit.c	1.1.1.2"
#ident "$Header$"
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs/bfs.h>

#define BDSUPER_SZ	sizeof(struct bdsuper)
char *Usage ="bfs Usage:\nlabelit [-F bfs] [generic options] [fsname volume]\n";

main(argc, argv) 
int argc;
char **argv; 
{
	struct stat statarea;

	char *special = NULL;
	char *fsname  = NULL;
	char *volume  = NULL;
	int fd;
	struct bdsuper bds;
	long tblocks;
	long tinodes;
	void exit();

	special = argv[1];
	if (argc > 2) {
		fsname  = argv[2];
		if (strlen(fsname) > 6) {
			fprintf(stderr, Usage);	
			exit(31+8);
		}
	}
	if (argc > 3) {
		volume  = argv[3];
		if (strlen(volume) > 6) {
			fprintf(stderr, Usage);	
			exit(31+8);
		}
	}
	if (argc == 3) {
		fprintf(stderr, Usage);
		exit(31+8);
	}
	if (fsname == NULL)
		fd = open(special, O_RDONLY);
	else {
		if (stat(special, &statarea) < 0) {
			fprintf(stderr, "bfs %s: %s: can not stat\n",
				argv[0], special);
			exit(31+8);
		}
		if (((statarea.st_mode & S_IFMT) == S_IFBLK ||
		     (statarea.st_mode & S_IFMT) == S_IFCHR) &&
		     (statarea.st_flags & _S_ISMOUNTED)) {
			fprintf(stderr, "bfs %s: %s: mounted file system\n",
				argv[0], special);
			exit(31+2);
		}
		fd = open(special, O_RDWR);
	}
	if (fd < 0) {
		fprintf(stderr, "bfs %s: can not open special file %s\n",
			argv[0], special);
		perror("labelit");
		exit(31+8);
	}
	if (read(fd, &bds, BDSUPER_SZ) != BDSUPER_SZ) {

		fprintf(stderr, "bfs %s: can not read\n",argv[0]);
		perror("labelit");
		exit(31+8);
	}
	if (bds.bdsup_bfsmagic != BFS_MAGIC) {
		fprintf(stderr, "bfs %s: %s is not a bfs file system\n",
			argv[0], special);
		exit(31+8);
	}

	tblocks = (bds.bdsup_end + 1) / BFS_BSIZE;
	tinodes = (bds.bdsup_start - BFS_DIRSTART) / sizeof(struct bfs_dirent);

	printf("Current fsname: %.6s, Current volume: %.6s, Blocks: %ld\n",
		bds.bdsup_fsname, bds.bdsup_volume, tblocks);
	printf("Inodes: %d FS Units: 512b\n", tinodes);
	if (fsname == NULL) {
		close(fd);
		exit(0);
	}

	strncpy(bds.bdsup_fsname, fsname, 6);
	strncpy(bds.bdsup_volume, volume, 6);
	printf("\nNEW fsname: %.6s, NEW volume: %.6s\n", fsname, volume);
	printf("(DEL if wrong!!)\n");
	sleep(10);

	lseek(fd, BFS_SUPEROFF, 0);
	if (write(fd, &bds, BDSUPER_SZ) != BDSUPER_SZ) {
		fprintf(stderr, "bfs %s: can not write\n",argv[0]);
		perror("labelit");
		exit(31+8);
	}
	printf("\nCurrent fsname: %.6s, Current volume: %.6s, Blocks: %ld\n",
		bds.bdsup_fsname, bds.bdsup_volume,  tblocks);
	printf("Inodes: %d FS Units: 512b\n", tinodes);
	
	close(fd);
	exit(0);
}
