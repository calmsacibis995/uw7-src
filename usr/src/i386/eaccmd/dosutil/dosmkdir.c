/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosmkdir.c	1.3.1.4"
#ident  "$Header$"

/*
 *	@(#) dosmkdir.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	dosmkdir - create an MS-DOS directory.  Most common formats
 *			are supported.
 *
 *	MODIFICATION HISTORY
 *	M000	barrys	Jul 17/84
 *	- A check is now made for whether the disk can support directories.
 * 	M001	barrys	Aug 30/84
 *	- The date is now set for "." and ".." when a directory is created.
 *	M002	ericc	Dec 7, 1984			16 bit FAT Support
 *	- rewrote the program, allowing lowercase characters to alias
 *	  device names and preventing concurrent access to a DOS disk.
 *	M003	gsj	Oct 10, 1986
 *	- device is now open for sync writes 
 *	M004	buckm	Sep 21, 1987
 *	- add call to setup_perms() to deal with possibly being
 *	  a setuid program.
 */


#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	"dosutil.h"


struct format	 disk;			/* details of current DOS disk */
struct format	*frmp = &disk;
struct dosseg	 seg;			/* locations on current DOS disk */
struct dosseg	*segp = &seg;
char	*buffer  = NULL;		/* buffer for DOS clusters */
char	 errbuf[BUFMAX];		/* error message string	*/
char	*fat;				/* FAT of current DOS disk */
char	*f_name  = "";			/* name of this command */
int	bigfat;				/* 16 or 12 bit FAT */
int	dirflag  = TRUE;		/* FALSE if directories not allowed */
int	exitcode = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */

int	Bps = 512;

main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	char *doscmd;
	int i, filecount = 0;			/* number of file arguments */
	struct file file[NFILES];		/* file arguments */

	f_name   = basename(*argv);
	doscmd = argv[0];
	setup_perms();						/* M004 */

	while (--argc > 0){
		decompose(*(++argv),
				&(file[filecount].unx),
				&(file[filecount].dos));
#ifdef DEBUG
		fprintf(stderr,"DEBUG xpath = %s\tdospath = %s\n",
			file[filecount].unx,file[filecount].dos);
#endif
		if (filecount++ >= NFILES)
			fatal("too many files",1);
	}
	if ( (ppid = getppid()) == -1) {
		fprintf(stderr,"%s: Could not obtain parent process ID\n",doscmd);
		exit(1);
	}
	if ( (pgid = getsid(ppid)) == -1) {
		fprintf(stderr,"%s: Could not obtain process group ID for parent process\n",doscmd);
		exit(2);
	}
	if (setpgid(getpid(),pgid) == -1) {
		fprintf(stderr,"%s: Could not join process group of parent\n",doscmd);
		exit(3);
	}
	if (filecount <= 0)
		usage();

	for (i = 0; i < filecount; i++){
		doit(file[i].unx,file[i].dos);
	}
	exit(exitcode);
}


doit(dev,filename)
char *dev,*filename;
{
	unsigned parclust;
	char dirent[DIRBYTES], *parname, *target;

	target  = lastname(filename);
	parname = parent(filename);
	zero(dirent,DIRBYTES);

	if (!(dev = setup(dev,O_RDWR | O_SYNCW)))	/* M003 	*/
		return;

	if (!dirflag){
		sprintf(errbuf,"%s does not support DOS directories",dev);
		fatal(errbuf,0);
		return;
	}
	if (!seize(fd)){
		sprintf(errbuf,"can't seize %s",dev);
		fatal(errbuf,0);
		close(fd);
		return;
	}
	if (((fat    = malloc(frmp->f_fatsect * MAX_BPS))   == NULL) ||
		((buffer = malloc(frmp->f_sectclust * MAX_BPS)) == NULL)){
		release(fd);
		fatal("no memory for buffers",1);
	}

	for( Bps = 512; Bps <= MAX_BPS; Bps += 512 ) {
		if (readfat(fat)){
			break;
		}
	}

	if( Bps > MAX_BPS ) {
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		fatal(errbuf,0);
	}

	if (!*filename){
		sprintf(errbuf,"can't make root directory on %s",dev);
		fatal(errbuf,0);
	}
	else if (parname && (search(ROOTDIR,parname,dirent) == NOTFOUND)){
		sprintf(errbuf,"%s:%s does not exist",dev,parname);
		fatal(errbuf,0);
	}
	else{
		if (parname)
			parclust = word(&dirent[CLUST]);
		else
			parclust = ROOTDIR;

		if (search(parclust,target,dirent) != NOTFOUND){
			sprintf(errbuf,"%s:%s already exists",dev,filename);
			fatal(errbuf,0);
		}
		else if (dosmkdir(parclust,target,time((long *)0)) == NOTFOUND){
			sprintf(errbuf,"out of space on %s",dev);
			fatal(errbuf,0);
		}
	}
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
}


/*	dosmkdir()  --  make a DOS directory, returning the starting
 *		cluster of the directory or NOTFOUND if impossible.
 *
 *	WARNING: Buffer must not be contaminated !!
 *
 *		parclust : starting cluster of the parent directory
 *		name     : name of the directory to be created
 *		xtime    : creation time
 */

dosmkdir(parclust,name,xtime)
unsigned parclust;
char *name;
long xtime;
{
	unsigned clustno;
	char dirent[DIRBYTES];

	if ((clustno = clustalloc(FIRSTCLUST)) == NOTFOUND)
		return(NOTFOUND);

	zero(buffer,frmp->f_sectclust * Bps);

	forment(buffer,"",SUBDIR,xtime,clustno,(long) 0);
	movchar(buffer,".",1);
	forment(buffer + DIRBYTES,"",SUBDIR,xtime,parclust,(long) 0);
	movchar(buffer + DIRBYTES,"..",2);
	if (!writeclust(clustno,buffer))
		return(NOTFOUND);
 
	forment(dirent,name,SUBDIR,xtime,clustno,(long) 0);

	if (!dirfill(parclust,dirent))
		clustno = NOTFOUND;

	disable();
	if (!writefat(fat))
		clustno = NOTFOUND;
	enable();

	return(clustno);
}



usage()
{
	fprintf(stderr,"Usage: %s device:path  . . .\n",f_name);
	exit(1);
}
