/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/doscp.c	1.4.1.8"
#ident  "$Header$"
/*	@(#) doscp.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983.
 *
 *	doscp - copy files between Unix and MS-DOS
 *
 *	Usage:	1.  doscp unxfile dosfile
 *		2.  doscp dosfile unxfile
 *		3.  doscp dosfile [ dosfile ... ] unxdir
 *		4.  doscp unxfile [ unxfile ... ] unxdir
 *
 *	"-r" flag indicates raw copy, don't map CR during transfers.
 *	"-m" flag indicates forced CR mapping.
 *	"-R" flag indicates recursive copy like 'r' option of 'cp' command.
 *	Default is to map CR only if a file is printable.
 *
 */

#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	"dosutil.h"
#include	"globals.h"
#ifdef	INTL
#include 	<ctype.h>
#endif

void	make_destname();
int	RECURSIVE = 0;		/* recursive copy */
char	tflag;			/* 'F' or 'D' flag for target */
int	filecount = 0;		/* number of command line arguments */
int	src_cnt=0, target_cnt=0;/* number of matched src and target patterns */
char	*doscmd;

/* Possible forms of the command line are:
 *
 *	doscp xf dev:path		(copy Unix file to dos file)
 *	doscp xf [...] dev[:dir]	(copy Unix files to dos dir)
 *	doscp dev:path xf		(copy dos file to Unix file)
 *	doscp dev:path [...] xd		(copy dos files to Unix dir)
 *
 * This is all particularly grungy to decode. We look at the
 * last argument given and decide which form to use based on
 * its type.
 */

main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	int i, j, dospath[NFILES], totdx=0, totxd=0, STAR_CNT=0;
	char *c, dest[PATH_MAX], *temp, *match_name, star[2]="*";
	struct file file[NFILES], *last; /* file args */

	f_name = basename(*argv);
	doscmd = argv[0];
	setup_perms();

	while (--argc > 0){
		dospath[filecount]=0;
		c = *(++argv);		/* parse command options */
		if ( strchr(c,':') != NULL )
		   dospath[filecount]=1;
		if (*c == '-') {			
			if (filecount) 
				usage("bad format");
			switch(*(++c)){
			case 'r':
				flag = RAW;
				break;
			case 'm':
				flag = MAP;
				break;	
			case 'R':
				RECURSIVE++;
				break;	
			default:
				sprintf(errbuf,"unknown option \"-%c\"",*c);
				fatal(errbuf,1);
			}
		}
		else{ 					/* 	parse     */
			decompose(c,			/* file pathname  */
				  &(file[filecount].unx),
				  &(file[filecount].dos));
#ifdef DEBUG
			fprintf(stderr,"filecount=%d dospath=%d unx=[%s] dos=[%s]\n", filecount, dospath, file[filecount].unx, file[filecount].dos);
#endif
			if (filecount++ >= NFILES)
				fatal("too many file arguments",1);
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
	if (filecount < 2)
		usage("too few arguments");

	/* First match patterns for the source pathnames.
	 * For each source pathname in file[i], match pattern and
	 * save the ptr to resulting pathnames in a_match_info[i].name.
	 * Save the ptr to pathtypes in a_match_info[i].type. */

	for (i=0; i < filecount-1; i++ ) {
		if ( dospath[i] == 1 && RECURSIVE && 
	     	     *file[i].unx != NULL && *file[i].dos == NULL) {
				file[i].dos =   star;
		   dospath[i]=2;
		}
		STAR_CNT = match_pattern(&file[i],&a_match_info[i],0);
		src_cnt += STAR_CNT;
		if ( dospath[i] == 2 && STAR_CNT == 0 )  {
			sprintf(errbuf,"device %s is empty", file[i].unx);
			fatal(errbuf,0);
		}
	}

	if (src_cnt == 0)
		usage("no matching source pathname");

	/* Now match pattern for target pathname.  There must be one
         * target pathname.					*/

	target_cnt=match_pattern(&file[filecount-1],&a_match_info[filecount-1],1);

	if (target_cnt == 0 )
		 usage("no matching target pathname");	

	if (target_cnt != 1) {			/*multiple matching targets */
		j=0;
		for (;;) {
			match_name=a_match_info[filecount-1].name[j++];
			if ( ! match_name)
				break;
			fprintf(stderr,"matching target name=%s\n", match_name);
		}
	   usage("too many matching target pathnames");
	}

#ifdef DEBUG
	j=0;
	for (;;) {
		match_name=a_match_info[filecount-1].name[j++];
		fprintf(stderr,"matching target name=%s\n", match_name);
		if ( ! match_name)
			break;
	}
	fprintf(stderr,"filecount=%d matching source names=%d\n", filecount,src_cnt);
#endif
	last = &(file[filecount - 1]);
	tflag = a_match_info[filecount - 1].type[0];
	switch( whattarget(last) ){ 

	case DOSDIR:
		/* Check that no files are DOS files and */
		/* Fail if trying to copy DOS file to DOS dir */
		for (i = 0; i < filecount - 1; i++) 
			if (strlen(file[i].dos) != 0) {
				sprintf(errbuf, "Can't copy DOS to DOS");
				fatal(errbuf, 4);
			}
	   	match_name = a_match_info[filecount-1].name[0];
		if (setup_dosmedium(last->unx, O_RDWR | O_SYNCW, 0) < 0)
			return; 
		for ( i = 0; i < filecount - 1; i++){
			totxd+=cpxd(file[i].unx,last->unx,match_name,DOSDIR);
		}
		close_dosmedium();
		break;

	case UNXDIR:
		totdx+=unix_dir(last->unx,MAK_UNXDIR);
		for (i = 0; i < filecount - 1; i++){
			j=0;
			for (;;) {
	   			match_name = a_match_info[i].name[j++];
#ifdef DEBUG
				fprintf(stderr,"UNXDIR:dosname=%s j=%d\n",match_name,j);
#endif
				if ( ! match_name)
					break;
				temp = lastname(match_name);
				make_destname(last->unx,temp,dest);
				totdx+=cpdx(file[i].unx, match_name, dest);
			}
		}
		break;

	case DOSFILE:
		/* Fail if trying to copy DOS file to DOS file */
		if (strlen(file[0].dos) != 0) {
			sprintf(errbuf, "Can't copy DOS to DOS");
			fatal(errbuf, 4);
		}
		if (src_cnt > 2){
		   switch(tflag){
			 case 'U': sprintf(errbuf, "Cannot access %s:%s no such DOS file or DOS directory",
						last->unx,last->dos);
				break;
			 case 'F': sprintf(errbuf, "Target %s:%s must be a DOS directory",
						last->unx,last->dos);
				break;
			 default: sprintf(errbuf, "tflag=%c Cannot overwrite multiple files to the DOS file %s\n",
						tflag,last->dos);
				break;
		   }
		   fatal(errbuf,1);
		}
		if (setup_dosmedium(last->unx, O_RDWR | O_SYNCW, 0 ) < 0 )
			return; 
		totxd+=cpxd(file[0].unx,last->unx,a_match_info[filecount-1].name[0],DOSFILE);
		close_dosmedium();
		break;

	case UNXFILE:
		if (filecount > 2){
			sprintf(errbuf,"%s not a directory",last->unx);
			fatal(errbuf,1);
		}
		totdx+=cpdx(file[0].unx, a_match_info[0].name[0], last->unx);
 		break;
	default:
		sprintf(errbuf,"%s:%s not a DOS or UNIX pathname",
						last->unx,last->dos);
		fatal(errbuf,1);
	}
#ifdef DEBUG
	fprintf(stderr,"main:  totdx=%d totxd=%d\n",totdx,totxd);
#endif
	exit(exitcode);
}

/*
 *	cpdx()  --  copy from a DOS file to a Unix file.
 */

cpdx(dev, src, dest)
char *dev, *src, *dest;
{
	char dirent[DIRBYTES];
	int totfiles=0;

	zero(dirent,DIRBYTES);

	if ( setup_dosmedium(dev, O_RDONLY, 0) < 0 )
		return(0) ;
#ifdef DEBUG
	fprintf(stderr,"cPdx:dev=%s src=%s dest=%s\n",dev,src,dest);
#endif

	if (*src == (char) NULL) {
		sprintf(errbuf,"can't copy from %s to another Unix file",dev);
		fatal(errbuf,0);
		close_dosmedium();
		return(0);
	}
	if (search(ROOTDIR,src,dirent) == NOTFOUND){
		sprintf(errbuf,"%s(cpdx):%s not found",dev,src);
		fatal(errbuf,0);
		close_dosmedium();
		return(0);
	}
	if (dirent[ATTRIB] == SUBDIR){
		if ( RECURSIVE ) {
			/* 1 as the fifth argument will make the parent
			   directory after src is found on the dos medium */
			totfiles=recur_cpdx(ROOTDIR,O_CPDX,src,dest,1);
			close_dosmedium();
			return(totfiles);
		}
		sprintf(errbuf,"%s:%s is a DOS directory",dev,src);
		fatal(errbuf,0);
		close_dosmedium();
		return(0);
	}
	totfiles = cp_dos2unix(word(&dirent[CLUST]),longword(&dirent[SIZE]),dest,"");
	close_dosmedium();
#ifdef DEBUG
	fprintf(stderr,"cPdx:totfiles=%d\n",totfiles);
#endif
	return(totfiles);
}

/*	cpxd()  --  copy from a Unix file to a DOS file.
 *		source     : name of Unix file to be copied
 *		dev        : device on which the DOS disk is mounted
 *		dest       : path name of DOS file to be created
		targetflag :	target is a DOSFILE or DOSDIR
 */

cpxd(source,dev,dest,targetflag)
char *source, *dev, *dest;
unsigned targetflag;
{
	FILE *in;
	unsigned dirclust, ret_code, totfls=0;
	struct stat statbuf;
	char  dirent[DIRBYTES], dosdest[PATH_MAX], temp[5];

	if (!stat(source,&statbuf) &&
		((statbuf.st_mode & S_IFMT) == S_IFBLK)){

		sprintf(errbuf,"both %s and %s are DOS disks",source,dev);
		fatal(errbuf,0);
		return(0);
	}
	if ( RECURSIVE ) {
		return( recur_cpxd(ROOTDIR,dev,source,dest,targetflag ));
	}
	/* make target dos pathname in dosdest */
	strcpy(dosdest,dest);
	if (targetflag == DOSDIR) {
		if ( *dest == NULL ) {
			strcpy(temp,"/");
			make_destname(temp,source,dosdest);
		}
		else 
			make_destname(dest,source,dosdest);
	}

	if (! strlen(dosdest) ){
		sprintf(errbuf,"LOGIC ERROR missing DOS filename for %s",dev);
		fatal(errbuf,1);
	}
	ret_code=mkcopy(ROOTDIR,dev,dosdest,source);
	ret_code =  chk_mkcopy_retcode(ret_code,dev,dosdest);
	return(1);
}
	

usage(msg)
char *msg;
{

	if (msg) {
		fprintf(stderr,"%s: %s\n", f_name, msg);
	}
	fprintf(stderr,"Usage: %s [-rmR] device:path  . . .  device:path\n",
									f_name);
	exit(1);
}

/*	whattarget()  --  returns DOSDIR, UNXDIR, DOSFILE or UNXFILE,
 *		target    : the file in question
 */

whattarget(target)
struct file *target;
{
	struct stat statbuf;

	if (*(target->dos) == (char) NULL){

		if (stat(target->unx,&statbuf) != 0)
		   if (RECURSIVE && src_cnt > 1 )
			return(UNXDIR);
		   else return(UNXFILE);

		switch(statbuf.st_mode & S_IFMT){
			case S_IFBLK:
			case S_IFCHR:
				return(DOSDIR);
			case S_IFDIR:
				return(UNXDIR);
			case S_IFREG:
				return(UNXFILE);
			default:
				sprintf(errbuf,"bad file %s",target->unx);
				fatal(errbuf,1);
		}
	}

	/* a_match_info[filecount-1].type[0] must be 'F' or 'D' for target */

#ifdef DEBUG
	fprintf(stderr,"whattarget:tflag=%c\n",tflag);
#endif
	switch ( tflag ) {
		case  'U':
		case  'F':
			return(DOSFILE);
		case  'D':
			return(DOSDIR);
	     	default:
			return(-1);
	}
}
