/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosdir.c	1.3.1.3"
#ident  "$Header$"

/*
 *	@(#) dosdir.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	This program can be invoked either as
 *		dosls    --  only filenames are printed.
 *		dosdir   --  use the DOS 'dir' format.
 *
 *	MODIFICATION HISTORY
 *	M000	Dec 7, 1984	ericc			16 bit FAT Support
 *	- rewrote the program, allowing lowercase characters to alias
 *	  device names and preventing concurrent access to a DOS disk.
 *	M001	buckm	Sep 21, 1987
 *	- add call to setup_perms() to deal with possibly being
 *	  a setuid program.
 */

#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	"dosutil.h"

#define		DOSLS		"dosls"	/* "ls" version of this command */


struct format	 disk;			/* details of current DOS disk */
struct format	*frmp = &disk;
struct dosseg	 seg;			/* locations on current DOS disk */
struct dosseg	*segp = &seg;
char	*buffer = NULL;			/* buffer for DOS clusters */
char	 errbuf[BUFMAX];		/* error message string	*/
char	*fat;				/* FAT of current DOS disk */
char	*f_name   = "";			/* name of this command */
int	bigfat;				/* 16 or 12 bit FAT */
int	dirflag   = TRUE;		/* FALSE if directories not allowed */
int	exitcode  = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */
int	filecount = 0;			/* number of file arguments */



int	dirformat();			/* output formatting functions */
int	lsformat();
int	(*outform)();

int	Bps = 512;


main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	char *doscmd;
	int i;
	char *c;
	struct file file[NFILES];		/* file arguments */

	f_name   = basename(*argv);
	doscmd = argv[0];
	outform  = strcmp(f_name,DOSLS) ? dirformat : lsformat;
	setup_perms();						/* M001 */

	while (--argc > 0){
		decompose(*(++argv),
				&(file[filecount].unx),
				&(file[filecount].dos));
#ifdef DEBUG
		fprintf(stderr,"DEBUG xpath = %s\tdospath = %s\n",
			file[filecount].unx,file[filecount].dos);
#endif
		if (filecount++ >= NFILES)
			fatal("too many directories",1);
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

/*	dirformat()  --  print a directory entry the DOS "dir" way, returning
 *				the number of entries listed.
 */

dirformat(entry)
char	entry[];
{
	char attrib;

	attrib = entry[ATTRIB];
	if (attrib & (HIDDEN | SYSTEM | VOLUME)){

#ifdef DEBUG
		fprintf(stderr,"%.8s %.3s\t attribute %.2x\n",
					&entry[NAME],&entry[EXT],attrib);
#endif
		return(0);
	}
	if (entry[NAME] == 0x05)		/* encoded first byte */
		putchar(0xe5);
	else
		putchar(entry[NAME]);

	printf("%.7s %.3s",&entry[NAME + 1],&entry[EXT]);

	if (attrib & SUBDIR)
		printf(" <DIR>  ");
	else
		printf("% 8ld",longword(&entry[SIZE]));

	printdate( word(&entry[DATE]) );
	printtime( word(&entry[TIME]) );
	putchar('\n');
	return(1);
}


doit(dev,filename)
char *dev,*filename;
{
	unsigned dirclust;
	int i, nfiles, tmp;
	char *bufend, dirent[DIRBYTES], *j;

	nfiles = tmp = 0;			/* nfiles counts files listed */
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

	if (*filename == (char) NULL){		/* root directory */

		prolog(dev,filename);
		for (i = 0; (i < frmp->f_dirsect) && (tmp >= 0); i++){
			readsect(segp->s_dir + i,buffer);
			for (j = buffer; j < buffer + Bps; j += DIRBYTES){
				if ((tmp = printent(j)) < 0)
					break;
				nfiles += tmp;
			}
		}
		epilog(nfiles);
	}
	else if (search(ROOTDIR,filename,dirent) == NOTFOUND){
		sprintf(errbuf,"%s:%s not found",dev,filename);
		fatal(errbuf,0);
	}
	else if (dirent[ATTRIB] & SUBDIR){		/* sub-directory */

		prolog(dev,filename);
		dirclust = word(&dirent[CLUST]);
		bufend   = buffer + (frmp->f_sectclust * Bps);

		while (goodclust(dirclust) && (tmp >= 0)){
			if (!readclust(dirclust,buffer)){
			       sprintf(errbuf,"can't read cluster %u",dirclust);
			       fatal(errbuf,1);
			}
			for (j = buffer; j < bufend; j += DIRBYTES){
				if ((tmp = printent(j)) < 0)
					break;
				nfiles += tmp;
			}
			dirclust = nextclust(dirclust);
		}
		epilog(nfiles);
	}
	else{						/* not a directory */
		prolog( dev, parent(filename) );
		epilog( printent(dirent) );
	}
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
}

/*	epilog(nfiles)  --  print obituarial information after directory
 *				listing.
 */

epilog(nfiles)
int nfiles;
{
	long nfree;				/* free bytes on DOS disk */
	unsigned assess();

	if (outform == lsformat)		/* DOSLS is not verbose */
		return;

	if (nfiles == 0){
		fprintf(stderr," File not found\n");
		return;
	}
	nfree = (long) assess() * frmp->f_sectclust * Bps;
	printf("    % 5d File(s) % 8ld bytes free\n",nfiles,nfree);
}


/*	lsformat(entry)  --  print the directory entry "ls" style, returning
 *				the number of entries listed.
 */

lsformat(entry)
char	*entry;
{
	int i;
	char attrib;

	attrib = entry[ATTRIB];
	if (attrib & (HIDDEN | SYSTEM | VOLUME)){

#ifdef DEBUG
		fprintf(stderr,"%.8s %.3s\t attribute %.2x\n",
					&entry[NAME],&entry[EXT],attrib);
#endif
		return(0);
	}
	switch(entry[NAME]){			/* status of filename */

		case 0x05:	putchar(0xe5);	/*  encoded   */
				break;		/* first byte */
		case ' ':
		case DIRECT:	return(0);

		default:	putchar(entry[NAME]);
	}
	for (i = NAME + 1; (i < NAME + NAMEBYTES) && (entry[i] != ' '); i++)
		putchar(entry[i]);

	if (entry[EXT] != ' '){
		putchar('.');
		for (i = EXT; (i < EXT + EXTBYTES) && (entry[i] != ' '); i++)
			putchar(entry[i]);
	}
	putchar('\n');
	return(1);
}


printdate(code)
short code;
{
	printf("  %2d-",(code >> 5) & 0xf);			/* month */
	printf("%.2d-",code & 0x1f);				/* date */
	printf("%.2d",(((code >> 9) & 0x3f) + 1980) % 100);	/* year */
}


/*	printent(entry)  --  print the directory entry, returning -1 if
 *		this is the end of the directory, 0 if the entry need not
 *		be listed, 1 if the entry was printed.
 */

printent(entry)
char	entry[];
{
	switch(entry[0] & 0xff){
		case DIREND:				/* end of directory */
				return(-1);
		case WASUSED:				/* erased file */
				return(0);
		default:
				return( (*outform)(entry) );
	}
}


printtime(code)
short code;
{
	int ampm, hour;

	hour  = (code >> 11) & 0x1f;
	ampm  = (hour >= 12) ? TRUE : FALSE;
	hour %= 12;

	printf("  %2d:",hour ? hour : 12);	/* hour */
	printf("%.2d",(code >> 5) & 0x3f);	/* minutes */
	printf("%c",ampm ? 'p' : 'a');
}

/*
 *	prolog(dev,dir)  --  announce the device and directory being listed.
 */

prolog(dev,dir)
char *dev, *dir;
{
	char label[NAMEBYTES+EXTBYTES];

	if (filecount > 1)
		putchar('\n');

	if (outform == lsformat)		/* DOSLS is not verbose */
		return;

	printf(" Volume in drive %s ",dev);

	if ( findvol(label) )
		printf("is %.11s\n",label);
	else
		printf("has no label\n");

	printf(" Directory of %s:",dev);

	if (*dir != DIRSEP)
		putchar(DIRSEP);

	printf("%s\n\n",dir);
}
	


usage()
{
	fprintf(stderr,"Usage: %s device:path  . . .\n",f_name);
	exit(1);
}
