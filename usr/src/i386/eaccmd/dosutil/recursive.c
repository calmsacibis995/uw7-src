#ident	"@(#)eac:i386/eaccmd/dosutil/recursive.c	1.1.1.2"
#ident  "$Header$"

#include	<unistd.h>
#include	<dirent.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	"dosutil.h"

void make_destname();
FILE *open_file();
unsigned make_dos_dir(), get_fileclust();
long get_filebytes();

char errmsg[100];

extern	int Bps;
extern	int flag;

char	errbuf[BUFMAX];			/* error message string	*/

/*	recur_cpdx() - do a recursive copy of a dos directory to a unix dir.
	clustno:	cluster no for the directory we'll copy.
	operation:	whether to do cp or rm
	src:	 	source directory on the dos medium
	target:		unix target directory 
*/

recur_cpdx(clustno,operation,src,target, make_unix_dir)
unsigned clustno, make_unix_dir;
char *src, *target, operation;
{
	unsigned	dirclust, fcn; 	/* this dir cluster and file clustno */
	int 		ret_recur, totfiles = 0, num_names = 0;
	long		nbytes, fnb;	/* nbytes and file nbytes; */
	char		*bufend, *j;
	char		fdflag,newdest[PATH_MAX],fname[30],temp[30],dirent[DIRBYTES];

#ifdef DEBUG
	fprintf(stderr,"recur_cpdx: src=[%s] target=[%s]\n",src,target);
#endif
	if ((search(clustno,src,dirent))==NOTFOUND) {
		(void)sprintf(errbuf,"(recur_cpdx):%s not found",src);
		fatal(errbuf,1);
	}

	/* Make target unix directory, if it does not exist.
	   This behavior is consistent with cp -r */
	if ( make_unix_dir ) 
		totfiles+=unix_dir(target, MAK_UNXDIR); 

	dirclust = word(&dirent[CLUST]);
	bufend   = buffer+ (frmp->f_sectclust * Bps);
 
	/* get names in this directory. */

	while (goodclust(dirclust) && (num_names >= 0)){
		if (!readclust(dirclust,buffer)){
		       (void)sprintf(errbuf,"recur_cpdx:can't read cluster %u",dirclust);
		       fatal(errbuf,1);
		}
		for (j = buffer; j < bufend; j += DIRBYTES){
			if ((num_names = get_dosname(j,fname, &fdflag)) < 0)
				break;
			if (num_names == 0 )
				continue;

			/* for subdirectories call recur_cpdx  */
			if ( fdflag == 'D' ) {
				strcpy(temp,fname);
				lowshift(temp,strlen(temp));
				make_destname(target,temp,newdest);
				totfiles+=unix_dir(newdest,MAK_UNXDIR);

				/* 0 as the fifth argument to recur_cpdx() means
				   not the make the unix subdirectory, since we
				   just made it, after shifting the names to 
				   lower case. */
				ret_recur=recur_cpdx(dirclust,operation,
							fname,newdest,0);
				totfiles += ret_recur;
				/* buffer is global, refresh it */
				readclust(dirclust,buffer);
				continue;
			}
			fcn=get_fileclust(j);
			fnb=get_filebytes(j);

			switch (operation) {
			   case O_CPDX:	totfiles +=  \
					   cp_dos2unix(fcn,fnb,target,fname);
					break;
			   default:	fprintf(stderr,"recur_cpdx:Unknown operation code %d", operation);
					return(-1);
			}
			readclust(dirclust,buffer);
		}
		dirclust = nextclust(dirclust);
	}
#ifdef DEBUG
	fprintf(stderr,"recur_cpdx: totfiles=[%d] src=[%s] target=[%s]\n",totfiles,src,target);
#endif
	return (totfiles);
}

/*	cp_dos2unix()	copies a dos file to unix file.
 *	clustno:	clustno of the dos file.
 *	nbytes:		file size.
 *	dest:		target of the destination file.
 *	fname:		dos filename
 */
cp_dos2unix(clustno,nbytes,dest,fname)
unsigned clustno;
long nbytes;
char *fname, *dest;
{
	char destfile[PATH_MAX], temp[30];
	int outfd;
	int totfiles=0;

	strcpy(destfile,dest);
	/* when copying dos to unix, shift filename to lower case */
	if (fname ) {
		strcpy(temp,fname);
	        lowshift(temp,strlen(temp));
		make_destname(dest,temp,destfile);
	}

	if ((outfd = open(destfile, O_RDWR|O_CREAT|O_TRUNC, 0644)) != -1) {
		cat(clustno,nbytes,outfd,flag);
		close(outfd);
		totfiles++;
	}
#ifdef DEBUG
fprintf(stderr,"cp_dos2unx: destfile=[%s] totfiles=[%d]\n",destfile,totfiles);
#endif
	return(totfiles);
}

/*	dest is the target directory
 */
unix_dir(dest, FLG)
char *dest;
unsigned FLG;
{
	/*
	 * For recursive copy the last argument (the target) must 
	 * be a directory which really exists.
	 */

	struct stat s1;
	int c, ret_code, totfiles=0;

	ret_code = stat(dest, &s1); 

	switch(FLG) {
	   case CHK_UNXDIR:  /* dest must be an existing target directory */
		if ( ret_code < 0 ) {
			(void)sprintf(errmsg,"Cannot access target directory %s",dest);
			usage(errmsg);
		}
	
		if (!ISDIR(s1)) { /* target must be a directory */
			(void)sprintf(errmsg,"target %s must be a directory",dest);
			usage(errmsg);
		}

		/* 
	 	* While target has trailing 
	 	* DELIM (/), remove them (unless only "/")
	 	*/
		c =strlen(dest);
		while ((c > 1) && (*(dest+c-1) == DELIM)) {
		 	*(dest+c-1)=NULL;
			c--;
		}
		break;

	   case MAK_UNXDIR: /* make dir if it does not exist */

		if ( ret_code < 0 ) { /* cannot access target directory */
			if (mkdir(dest,(s1.st_mode & MODEBITS) | S_IRWXU) < 0) {
				(void)sprintf(errmsg,"Cannot create %s: %s",
						dest,strerror(errno));
				usage(errmsg);
			}
			totfiles++;
		} else 
			if (!(ISDIR(s1))) {
				(void)sprintf(errmsg,"target %s is not a directory",dest);
				usage(errmsg);
			}
		break;
	    default:
		(void)sprintf(errmsg,"unix_dir() unknowm opertaion %d",FLG);
		usage(errmsg);
	}
	return(totfiles);
}
unsigned get_fileclust(entry)
char entry[];
{
	return(word(&(entry[CLUST])));
}
long get_filebytes(entry)
char entry[];
{
	return(longword(&(entry[SIZE])));
}

/*	recur_cpxd() - recursive copy from unix to dos.
 *	clustno:	dos file clustno.
 *	dev:		dos device.
 *	source:		unix file
 *	dest:		dos file
 *	targetflag:	whether target is DOSDIR of DOSFILE
 */

recur_cpxd(clustno,dev,source,dest,targetflag)
unsigned clustno, targetflag;	/*targetflag is either DOSFILE or DOSDIR */
char *dev, *source, *dest;
{
	FILE *in;
	struct stat s1;
	int c, ret_code, totfiles=0;
	char fulldest[PATH_MAX];

#ifdef DEBUG
	fprintf(stderr,"recur_cpdx: DOSFILE=%d DOSDIR=%d tragetflg=%d\n",
		DOSFILE,DOSDIR,targetflag);
#endif
	if ( stat(source, &s1) < 0 ) {
		(void)sprintf(errmsg,"Cannot access source %s",source);
		usage(errmsg);
	}
	/* fail if doing recursive copy of a directory to a DOS file */
	if (ISDIR(s1) && targetflag == DOSFILE ) {
		(void)sprintf(errmsg,"Cannot recursively copy %s to DOS file %s:%s",
					source,dev,dest);
		usage(errmsg);
	}

	/* call mkcopy to copy a unix file  to dos file or directory*/
	if ( ! ISDIR(s1) ) {
		if ( targetflag == DOSDIR ) {
		   if ( *dest ) {
			if (! make_dos_dir (clustno,dev,dest)){ /* out of space */
				close_dosmedium();
				exit(1);
			}
			totfiles++;
		   }
		   make_destname(dest,source,fulldest);
		}
		else {
			make_destname(dest,(char *) NULL,fulldest);
		}
		ret_code = mkcopy(clustno,dev,fulldest,source);
		ret_code = chk_mkcopy_retcode(ret_code,dev,fulldest);
		if ( ret_code)  {
			close_dosmedium();
			exit(1);
		}
		return(++totfiles);
	}
	/* copy recursive, unix directory to dos */
	totfiles+=recur_copy_x_dos(clustno,dev,source,dest);
#ifdef DEBUG
fprintf(stderr,"recur_cpxd: source=[%s] destfile=[%s] totfiles=[%d]\n",source,dest,totfiles);
#endif
	return(totfiles);
}
recur_copy_x_dos(clustno,dev,source,dest)
unsigned clustno;
char *dev, *source, *dest;
{
	DIR *unx_dir;
	struct dirent *dp;
	struct stat statb, s1;
	char fromname[PATH_MAX+1], fulldest[PATH_MAX+1];
	char src[PATH_MAX+1], bname[PATH_MAX+1], subdir[PATH_MAX+1]; 
	int ret_code, totfiles=0;

        unx_dir = opendir(source);
	if (unx_dir == 0 || (fstat(unx_dir->dd_fd, &statb) < 0)) {
		(void)sprintf(errmsg,"Error with dir_ent  [%s]",source);
		usage(errmsg);
	}

	/* extract the basename of the source. For example, if source is
         * /test1/tmp/foo - bname will be 'foo'. First copy source in src
	 * because basename messes up original string in source */
	strcpy(src,source);
	strcpy(bname,basename(src));

	if ( *dest ) 
		(void) sprintf(subdir,"%s%c%s",dest,DIRSEP,bname);
	else
		(void) sprintf(subdir,"%s",bname);
	if (! make_dos_dir (clustno,dev,subdir)){ /* out of space */
		close_dosmedium();
		exit(1);
	}
	totfiles++;

#ifdef DEBUG
	fprintf(stderr,"recur_copy_x_dos: source=%s dest=%s subdir=%s \n", source,dest,subdir);
#endif
	for (;;) {
		dp = readdir(unx_dir);
		if (dp == 0) {	/* end of dirents */
			(void) closedir(unx_dir);
			return(totfiles) ;
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if ((int)strlen(source)+1+(int)strlen(dp->d_name) >= PATH_MAX) {
			fprintf(stderr,"%s: %s/%s: Name too long\n",
				doscmd, source, dp->d_name);
			continue;
		}
		(void) sprintf(fromname, "%s/%s", source, dp->d_name);
		ret_code = stat(fromname, &s1); 
		if ( ret_code < 0 ) {
			(void)sprintf(errmsg,"Cannot access source entry [%s]",fromname);
			usage(errmsg);
		}
		if (ISDIR(s1)) {
			totfiles+=recur_copy_x_dos(clustno,dev,fromname,subdir);
			continue;
		}
		(void) sprintf(fulldest,"%s%c%s",subdir,DIRSEP,dp->d_name);
#ifdef DEBUG
	fprintf(stderr,"recur_copy_x_dos: totfiles=%d fulldest=%s fromname=%s \n",
				totfiles,fulldest,fromname);
#endif
		ret_code=mkcopy(clustno,dev, fulldest, fromname);
		ret_code = chk_mkcopy_retcode(ret_code,dev,fulldest);
		if ( ret_code) {
			close_dosmedium();
			exit(1);
		}
		totfiles++;
	}
}

/*	
*	open_file() - opens a file 'filename' for read or write 
*/

FILE *open_file(filename,oflg)
char *filename, *oflg;
{
	char	flgmsg[20];
	FILE *fp;

	switch (*oflg) {
		case 'r':
			strcpy(flgmsg,"reading");
			break;
		case 'w':
			strcpy(flgmsg,"writing");
			break;
		default:
			strcpy(flgmsg,"unknown fopen flag");
			break;
	}
	if ((fp = fopen(filename,oflg)) == NULL){
		(void)sprintf(errbuf,"can't open %s for %s - %s",
				filename,flgmsg,strerror(errno));
		fatal(errbuf,0);
	}
	return(fp);
}
