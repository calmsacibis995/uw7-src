/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosrm.c	1.3.1.4"
#ident  "$Header$"
/*	@(#) dosrm.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983.
 *
 *	This program can be invoked either as
 *		dosrm    --  remove DOS files
 *		dosrmdir --  remove empty DOS directories
 *
 *	NOTE:   DOS directories are not compacted; ie. after a file is removed,
 *		no attempt is made to free a cluster from its immediate parent
 *		directory, even if this were possible.
 *
 */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	"dosutil.h"
#include	"globals.h"
#ifdef	INTL
#include 	<ctype.h>
#endif

#undef		remove

#define		DOSRMDIR	"dosrmdir"

#define		NOPRIV		1	/* return codes from scomp_remove() */
#define		NOTFILE		2
#define		NOTEMPTY	3
#define		NOTDIR		4


int	dirrem   = FALSE;		/* TRUE if directory removal enabled */
int	RECURSIVE = 0;		/* recursive copy */
int	filecount = 0;		/* number of command line arguments */
int	src_cnt=0;		/* number of matched source patterns */
int	num_rm=0;		/* total number of files removed */
char	*doscmd;


main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid, pgid;
	int i, j, dospath[NFILES], STAR_CNT=0;
	char *c, dest[PATH_MAX], *temp, *match_name, star[2]="*";
	struct file file[NFILES], *last; /* file args */

	f_name = basename(*argv);
	doscmd = argv[0];
	dirrem   = strcmp(f_name,DOSRMDIR ) ? FALSE : TRUE;
	setup_perms();

	while (--argc > 0){
		dospath[filecount]=0;
		c = *(++argv);		/* parse command options */
		if ( strchr(c,':') != NULL )
		   dospath[filecount]=1;
		if (*c == '-') {		/* command options */
			if (filecount)
				usage("bad format");
			switch(*(++c)){
			case 'R':
				RECURSIVE++;
				dirrem = TRUE;
				break;	
			default:
				sprintf(errbuf,"unknown option \"-%c\"",*c);
				fatal(errbuf,1);
			}
			continue;
		}
		if ( strchr(c,':') == NULL ) {
			sprintf(errbuf,"%s is not a dos pathname\n", c);
			fatal(errbuf,1);
		}
		/* decompose file pathname  */
		decompose(c, &(file[filecount].unx), &(file[filecount].dos));
		if (filecount++ >= NFILES)
			fatal("too many file arguments",1);
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
		usage("specify dos files to be removed");

	/* First match patterns for the pathnames.
	 * For each pathname in file[i], match pattern and
	 * save the ptr to resulting pathnames in a_match_info[i].name.
	 * Save the ptr to pathtypes in a_match_info[i].type. */

	for (i=0; i < filecount; i++ ) {
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
		usage("no matching pathname");

#ifdef DEBUG
	j=0;
	for (;;) {
		match_name=a_match_info[filecount-1].name[j++];
		fprintf(stderr,"matching pathname=%s\n", match_name);
		if ( ! match_name) break;
	}
#endif

	for (i = 0; i < filecount; i++){
		if (a_match_info[i].name[0] == (char) NULL){
			sprintf(errbuf,"can't remove root directory on %s",
				file[i].unx);
			fatal(errbuf,0);
			continue;
		}
		if (setup_dosmedium(file[i].unx, O_RDWR | O_SYNCW, 0 ) < 0 )
			return; 
		doit(file[i].unx,&a_match_info[i]);
		close_dosmedium();
	}
	exit(exitcode);
}

doit(dev,matchlist)
char *dev;
struct match_info *matchlist;
{
	int result, j=0;
	unsigned dirclust;
	char dirent[DIRBYTES], *match_name;

	zero(dirent,DIRBYTES);
	for (;;) {
		match_name = matchlist->name[j++];
		if ( ! match_name)
			break;
#ifdef DEBUG
	fprintf(stderr,"dosrm:doit:filename=[%s] fileno=%d\n",match_name,j);
#endif
		if ((dirclust = search(ROOTDIR,match_name,dirent)) == NOTFOUND){
			sprintf(errbuf,"%s:%s not found",dev,match_name);
			fatal(errbuf,0);
			continue;
		}
		switch( scomp_remove(dirclust,dirent,match_name) ){

		   case NOPRIV:
			sprintf(errbuf,"can't remove %s:%s",dev,match_name);
			fatal(errbuf,0);
			break;
		   case NOTFILE:
			sprintf(errbuf,"%s:%s is a directory",dev,match_name);
			fatal(errbuf,0);
			break;
		   case NOTEMPTY:
			sprintf(errbuf,
				"directory %s:%s not empty",dev,match_name);
			fatal(errbuf,0);
			break;
		   case NOTDIR:
			sprintf(errbuf,"%s:%s isn't a directory",dev,match_name);
			fatal(errbuf,0);
			break;
		}
	}
#ifdef DEBUG
	fprintf(stderr,"doit: total files removed=%d\n", num_rm);
#endif
}


/*	isempty()  --  returns TRUE if the directory is empty.  NOTE:  This
 *		routine refuses to examine the root directory !!
 *		dirclust :  starting cluster of the directory to examine
 */

isempty(dirclust)
unsigned dirclust;
{
	char *bufend, *j;

	if (dirclust == ROOTDIR)
		fatal("LOGIC ERROR isempty(root directory) !!",1);

	bufend = buffer + (frmp->f_sectclust * Bps);
	while (goodclust(dirclust)){
		if (!readclust(dirclust,buffer)){
			sprintf(errbuf,"can't read cluster %d",dirclust);
			fatal(errbuf,1);
		}
		for (j = buffer; j < bufend; j += DIRBYTES){
#ifdef DEBUG
			fprintf(stderr,"DEBUG isempty %.11s\n",j);
#endif
			if (*j == DIREND)
				return(TRUE);
			if (((*j & 0xff) != WASUSED) && (*j != DIRECT)){
				return(FALSE);
			}
		}
		dirclust = nextclust(dirclust);
	}
	return(TRUE);
}
/*	scomp_remove(dirclust,dirent,pathname)
 *			   verify the situation, then remove a file.
 *		dirclust : cluster containing the directory entry
 *		dirent   : directory entry of the file
 *		pathname : pathname of the entry  to be removed
 */
scomp_remove(dirclust,dirent,pathname)
unsigned dirclust;
char dirent[], *pathname;
{
	char attrib;
	unsigned start;

	attrib = dirent[ATTRIB];
	start  = word(&dirent[CLUST]);

	if (attrib & (RDONLY | HIDDEN | SYSTEM | VOLUME)){

#ifdef DEBUG
		fprintf(stderr,"%.8s %.3s\t attribute %.2x\n",
				&dirent[NAME],&dirent[EXT],attrib);
#endif
		return(NOPRIV);
	}
	if (attrib & SUBDIR) {
		if (!dirrem)
			return(NOTFILE);
		if(RECURSIVE)  {
			num_rm += dosrm_subdir(ROOTDIR,dirent,pathname);
			dosunlink(dirclust,dirent);
			num_rm++;
#ifdef DEBUG
		fprintf(stderr," scomp_remove : %d files removed in %s\n",
				num_rm, pathname);
#endif
			return(0);
		}
		if (! isempty(start))
			return(NOTEMPTY);
	}
	else if (dirrem && ! RECURSIVE )
		return(NOTDIR);

	dosunlink(dirclust,dirent);
	num_rm++;
#ifdef DEBUG
	fprintf(stderr,"scomp_remove: file removed: %s total=%d\n", pathname, num_rm);
#endif
	return(0);
}
/* walk the file tree removing the files on the way */
dosrm_subdir(parclust,pardirent,pathname)
unsigned parclust;
char pardirent[], *pathname;
{
	unsigned	newclustno, clustno;
	int 		k, totrm = 0, num_names=0;
	char		*bufend, *j, fdflag, fname[30];
	char		dirent[DIRBYTES], newpathname[PATH_MAX+1];;

#ifdef DEBUG
	fprintf(stderr," dosrm_subdir: pathname=%s parclust=[%d]\n", pathname,parclust);
#endif

	/* read contents of directory */
	clustno = word(&pardirent[CLUST]);
	bufend   = buffer+ (frmp->f_sectclust * Bps);
	while (goodclust(clustno) && (num_names >= 0)){
		if (!readclust(clustno,buffer)){
		       sprintf(errbuf,"dosrm_subdir:can't read cluster %u",clustno);
		       fatal(errbuf,1);
		}
		for (j = buffer; j < bufend; j += DIRBYTES){
			if ((num_names = get_dosname(j,fname, &fdflag)) < 0)
				break;
			if (num_names == 0 )
				continue;
			movchar(dirent,j,DIRBYTES);
#ifdef DEBUG
fprintf(stderr,"dosrm_subdir: file=%s flag=%c totrm=%d\n",fname,fdflag,totrm+1);
#endif
			if ( fdflag == 'F' ) {
				dosunlink(clustno,dirent);
				readclust(clustno,buffer);
				totrm += 1;
				continue;
			}
			sprintf(newpathname,"%s/%s",pathname,fname);
			/* remove contents of the directory */
			totrm += dosrm_subdir(clustno,dirent,newpathname);
			/* remove the directory entry, itself */
			dosunlink(clustno,dirent); 
			totrm++;
			/* buffer is global, refresh it */
			readclust(clustno,buffer);
		}
		clustno = nextclust(clustno);
	}
#ifdef DEBUG
	fprintf(stderr,"dosrm_subdir : totrm=%d\n", totrm);
#endif
	return (totrm);
}

usage(msg)
char *msg;
{
	if (msg) {
		fprintf(stderr,"%s: %s\n", f_name, msg);
	}
	fprintf(stderr,"Usage: %s [-R] device:path  . . .\n",f_name);
	exit(1);
}
