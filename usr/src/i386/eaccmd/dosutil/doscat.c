/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/doscat.c	1.3.3.1"
#ident  "$Header$"
/*	@(#) doscat.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983
 *
 *	doscat - 'cat' file from MS-DOS disks.  This program relies
 *		on the existence of disk parameters (the BPS) in the
 *		boot sector, or else makes guesses based on the media
 *		descriptor byte in the FAT.
 *
 *	NOTE:	A DOS file which happens to be a directory has size 0 in its
 *		directory entry.
 *
 *	MODIFICATION HISTORY
 *	M000	Dec 7, 1984	ericc			16 bit FAT Support
 *	- rewrote the program, allowing lowercase characters to alias
 *	  device names and preventing concurrent access to a DOS disk.
 *	M001	ncm	Mar 28, 1986
 *	- added -m flag to force <cr><lf> mapping to occur, bypassing
 *	  the canprint() check, since it is sometimes wrong.
 *	M002	buckm	Sep 21, 1987
 *	- add call to setup_perms() to deal with possibly being
 *	  a setuid program.
 *
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
int	bigfat;				/* 16 or 12 bit FAT flag */
int	dirflag  = TRUE;		/* FALSE if directories not allowed */
int	exitcode = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */
int	flag  = UNKNOWN;		/* check for <cr><lf> conversion M001 */

int	Bps = 512;

main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	char *doscmd;
	char *c;
	int i, filecount = 0;			/* number of file arguments */
	struct file file[NFILES];		/* file arguments */

	f_name = basename(*argv);
	doscmd = argv[0];
	setup_perms();					/* M002 */

	while (--argc > 0){
		c = *(++argv);				/*	parse      */
		if (*c == '-')				/* command options */
			switch(*(++c)){
			case 'r':
				flag = RAW;		/* M001 */
				break;
			case 'm':			/* M001 */
				flag = MAP;		/* M001 */
				break;			/* M001 */
			default:
				sprintf(errbuf,"unknown option \"-%c\"",*c);
				fatal(errbuf,1);
			}
		else{					/* 	parse     */
			decompose(c,			/* file pathname  */
				  &(file[filecount].unx),
				  &(file[filecount].dos));
#ifdef DEBUG
			fprintf(stderr,"DEBUG xpath = %s\tdospath = %s\n",
				file[filecount].unx,file[filecount].dos);
#endif
			if (filecount++ >= NFILES)
				fatal("too many files",1);
		}
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
	char dirent[DIRBYTES];

	zero(dirent,DIRBYTES);

	if (!(dev = setup(dev,O_RDONLY)))
		return;

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
	for( ; Bps <= MAX_BPS; Bps += 512 ) {
		if (readfat(fat)){
			break;
		}
	}

	if( Bps > MAX_BPS ) {
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		fatal(errbuf,0);
	}

	if (!*filename)
		fatal("missing DOS file name",0);

	else if (search(ROOTDIR,filename,dirent) == NOTFOUND){
		sprintf(errbuf,"%s:%s not found",dev,filename);
		fatal(errbuf,0);
	}
	else if (dirent[ATTRIB] == SUBDIR){
		sprintf(errbuf,"%s:%s is a DOS directory",dev,filename);
		fatal(errbuf,0);
	}
	else
		cat(word(&dirent[CLUST]),longword(&dirent[SIZE]),
							fileno(stdout),flag);
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
}


usage()
{
	fprintf(stderr,"Usage: %s [-rm] device:path  . . .\n",f_name);
	exit(1);
}
