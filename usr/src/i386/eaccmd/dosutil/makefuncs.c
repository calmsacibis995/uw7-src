#ident	"@(#)eac:i386/eaccmd/dosutil/makefuncs.c	1.1"
#ident  "$Header$"

#include	"dosutil.h"

void make_destname();
char *do_malloc();
FILE *open_file();

extern int flag;
extern int dirflag;		/* FALSE if dos directories not supported */
extern int Bps;

extern struct format	*frmp;
extern char		errbuf[BUFMAX];	/* error message string	*/
extern char		*fat;		/* FAT of current DOS disk */
extern int		fd;		/* file descr of current DOS disk */

/* make_dos_dir()  --  chk and make dos dir if not there
 * 		clustno : cluster number of immediate parent
 *		dirname : absolute name of DOS directory
 */

make_dos_dir (clustno,dev,dirname)
unsigned clustno;
char *dev, *dirname;
{
	unsigned parclust;
	char dirent[DIRBYTES], *parname, *target;
	
	target  = lastname(dirname);
	parname = parent(dirname);
	zero(dirent,DIRBYTES);

	if (!dirflag) {	  /* dirflag=FALSE if dos directories not supported */
		sprintf(errbuf,"%s does not support DOS directories",dev);
		fatal(errbuf,0);
		return(FALSE);
	}
	/* starting from top, make the directories in the pathname */	
#ifdef DEBUG
	fprintf(stderr,"dirname=[%s] parname=[%s] target=[%s]\n", dirname, parname, target);
#endif
	while (parname && (search(ROOTDIR,parname,dirent) == NOTFOUND)) {
		if ( make_dos_dir (ROOTDIR,dev,parname) == FALSE ) {
			sprintf(errbuf,"%s:%s can not create",dev,parname);
			fatal(errbuf,1);
		}
	}
	if (parname)
		parclust = word(&dirent[CLUST]);
	else
		parclust = ROOTDIR;

	if (search(parclust,target,dirent) != NOTFOUND){
		/* sprintf(errbuf,"%s:%s already exists",dev,dirname);
		fatal(errbuf,0); 
		return(FALSE); */
#ifdef DEBUG
	fprintf(stderr,"make_dos_dir: dosdir target=[%s] exists\n", target);
#endif
		return(TRUE);
	}
	if (dos_mkdir(parclust,target,time((long *)0)) == NOTFOUND){
		sprintf(errbuf,"out of space on %s",dev);
		fatal(errbuf,0);
		return(FALSE);
	}
#ifdef DEBUG
	fprintf(stderr,"make_dos_dir: made dosdir target=[%s]\n", target);
#endif
	return(TRUE);
}

/*	dos_mkdir()  -- make a DOS directory, returning the starting
 *			cluster of the directory or NOTFOUND if impossible.
 *
 *	WARNING: Buffer must not be contaminated !!
 *
 *		parclust : starting cluster of the parent directory
 *		name     : name of the directory to be created
 *		xtime    : creation time
 *	This routine is a copy of the code used by dosmkdir command.
 */

dos_mkdir(parclust,name,xtime)
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

/*	make_destname() --	fname - first part of the name
 *				lname - last part of the name
				dest  - destination pathname
 *	This procedure returns in 'destpath',  full destination name
 *	by joining fname and lname with a '/' in between.
 *	If both fname and lname are non-null, then it gets the basename
 *	from lname before forming the destination name that is returned.
 *	If fname is not null and lname is null, it returns fname.
 *	If fname is null, it returns lname.
 */
void make_destname(dir_name,file_name, destpath)
char *dir_name, *file_name, *destpath;
{
	char *temp, bname[PATH_MAX];

	if (file_name) {
		strcpy(bname,file_name);
		temp = basename(bname);
	}
#ifdef DEBUG
	fprintf(stderr,"dir_name=[%s] file_name=[%s]\n",dir_name,file_name);
#endif
	if ( dir_name ) { /* concatenate file name after the directory */
		if (file_name) {
			if ((int)(strlen(dir_name)+strlen(temp)+1) > PATH_MAX) {
#ifdef DEBUG
				fprintf(stderr,"destination pathname:%s%c%s exceeds %d",dir_name,DIRSEP,temp,PATH_MAX);
#endif
				sprintf(errbuf,"destination pathname length exceeds maximum.");
				fatal(errbuf,1);
			}
			sprintf(destpath,"%s%c%s",dir_name,DIRSEP,temp);
		}
		else
			sprintf(destpath,"%s",dir_name);
	}
	else
		if (*file_name == '/' ) 
			sprintf(destpath,"%s",temp);
		else
			sprintf(destpath,"%c%s",DIRSEP,temp);
#ifdef DEBUG
	fprintf(stderr,"make_destname:fulldest=[%s]\n",destpath);
#endif
}

/*	This routine has been moved from doscp.c to this file to allow common
 *	underlaying routines to be used for recursive or non-recursive copy.
 *	printable(stream)  --  returns TRUE if a Unix file stream is printable.
 *		Only the first cluster's worth of the file is examined.  The
 *		last byte of the file is allowed to be DOSEOF.  This is similar
 *		to canprint().  Printable characters are:
 *				0x07  -  0x0d
 *				0x20  -  0x7e
 *		WARNING:  This function returns without rewinding the stream !!
 */

printable(stream)
FILE *stream;
{
	int c;
	unsigned i;

	rewind(stream);

	for (i = frmp->f_sectclust * Bps; i > (unsigned) 0; i--){

		if ((c = getc(stream)) == EOF)
			return(TRUE);

#ifdef INTL
		if (!isprint(c) && !isspace(c))
#else
		if ((c < 0x07) || (c > 0x7e) || 
		    ((c > 0x0d) && (c < 0x20)))
#endif
				return(FALSE);
	}
	return(TRUE);
}

/*	This code has been moved from doscp.c to this file to allow common
 *	underlaying routines to be used for recursive or non-recursive copy.
 *
 *	chk_mkcopy_retcode() -- prints diagnostic messages for 
 *	various return codes of the procedure mkcopy().
 */
chk_mkcopy_retcode(code,dev,dest)
int code;
char *dev,*dest;
{
	int rc = 0;
	switch( code) {
		case EXISTS:
			sprintf(errbuf,"can't remove %s:%s",dev,dest);
			fatal(errbuf,0);
			break;

		case NOSPACE:
			sprintf(errbuf,"no space on %s",dev);
			fatal(errbuf,0);
			rc = NOSPACE;
			break;

		case NOPATH:
			sprintf(errbuf,"no path to %s on %s",dest,dev);
			fatal(errbuf,0);
			break;
		case NOSOURCE: /* error msg that src file could not be opened
				  for read is printed by open_file on stderr */
			rc = NOSOURCE;
			break;
	}
	return(rc);
}

/*	This routine has been moved from doscp.c to this file to allow common
 *	underlaying routines to be used for recursive or non-recursive copy.
 *
 *	mkcopy()  --  copy a Unix file stream into a DOS file.
 *		parclust : cluster number of immediate parent
 *		filename : name of DOS file, relative to immediate parent
 *		src      : Unix source file
 */

mkcopy(parclust,dev,filename,src)
unsigned parclust;
char *dev, *filename, *src;
{
	FILE *infp;
	unsigned exist;
	char attrib, dirent[DIRBYTES], *nextlevel;

	nextlevel = makename(filename,dirent);
	exist     = findent(parclust,dirent);

#ifdef DEBUG
	fprintf(stderr,"parclust=%d filename=[%s] nextlevel=[%s] exist=%d \n",parclust,filename,nextlevel,exist);
#endif
	if (*nextlevel == (char) NULL){
		attrib = dirent[ATTRIB];
#ifdef DEBUG
	fprintf(stderr,"attrib=%d NOTFOUND=%d,RDONLY=%d HIDDEN=%d SYSTEM=%d VOLUME=%d SUBDIR=%d\n",attrib,NOTFOUND,RDONLY,HIDDEN,SYSTEM,VOLUME,SUBDIR);
#endif
		if (exist != NOTFOUND){
	       	  /*if( attrib & (RDONLY | HIDDEN | SYSTEM | VOLUME | SUBDIR))*/
	       	  if ( attrib & (RDONLY | HIDDEN | SYSTEM | VOLUME ))
				return(EXISTS);
#ifdef DEBUG
			fprintf(stderr,"DEBUG unlinking %s\n",filename);
#endif
			dosunlink(parclust,dirent);
		}
		if ((infp = open_file(src,"r")) != (FILE *) NULL ) {
			if (!cpover(parclust,filename,infp,flag)){
				fclose(infp);
				return(NOSPACE);
			}
			fclose(infp);
			return(0);
		}
		else  return(NOSOURCE); /* src could not be opened for read */
	}
	else if ((exist == NOTFOUND) || !(dirent[ATTRIB] & SUBDIR))
		return(NOPATH);
	else
		return( mkcopy(word(&dirent[CLUST]),dev,nextlevel,src) );
}

/*	This routine has been moved from doscp.c to this file to allow common
 *	underlaying routines to be used for recursive or non-recursive copy.
 *
 *	cpover() -- copy a Unix file over to a DOS file, returning FALSE
 *		    if unsuccessful.
 *		parclust : cluster number of immediate parent
 *		name     : name of DOS file, relative to immediate parent
 *		infp       : Unix input stream
 *		flag  	 : RAW if no <cr><lf> conversion necessary
 *		 	   MAP if <cr><lf> conversion always required
 *			   UNKNOWN if file should be checked for conversion
 */

cpover(parclust,name,infp,flag)
unsigned parclust;
char *name;
FILE *infp;
int flag;
{
	long size;
	time_t time();
	char dirent[DIRBYTES];
	int c, i, temp;
	unsigned clustno, previous, clustsize, start;

	size      = 0;
	start     = NOTFOUND;
	clustno   = FIRSTCLUST;
	clustsize = frmp->f_sectclust * Bps;

	if (flag == UNKNOWN)
		flag = (printable(infp) ? MAP : RAW);
	rewind(infp);

#ifdef DEBUG
	fprintf(stderr,"DEBUG cpover: flag = %d filename=[%s]\n",flag,name);
#endif

	if (flag == RAW){
		while (!feof(infp)){
			if ((size += fread(buffer,1,clustsize,infp)) == 0)
				break;

			previous = clustno;
			if ((clustno = clustalloc(previous)) == NOTFOUND)
				return(FALSE);

			if (start == NOTFOUND)		/* first cluster */
				start = clustno;
			else				/* other clusters */
				chain(previous,clustno);

			if (!writeclust(clustno,buffer))
				return(FALSE);
		}
	}
	else{	/* (flag == MAP) */
		c = temp = (char) NULL;

		while (c != EOF){
			if ((i = size % clustsize) == 0){
				previous = clustno;
				if ((clustno = clustalloc(previous))
								== NOTFOUND)
					return(FALSE);
								/*  first  */
				if (start == NOTFOUND)		/* cluster */
					start = clustno;
				else{				/* others */
					if (!writeclust(previous,buffer))
						return(FALSE);
					chain(previous,clustno);
				}
			}
			if (temp != (char) NULL){
				*(buffer + i) = temp;
				temp          = (char) NULL;
			}
			else
				switch(c = getc(infp)){

				case '\n':	temp          = '\n';
						*(buffer + i) = CR;
						break;
				case EOF:	*(buffer + i) = DOSEOF;
						break;
				default:	*(buffer + i) = c;
						break;
				}
			size++;
		}
		if (!writeclust(clustno,buffer))
			return(FALSE);
	}
	if (size == 0)				/* zero length file */
		start = 0;
	else
		marklast(clustno);

	forment(dirent,name,REGFILE,time((long *) 0),start,size);

	temp = TRUE;
	disable();
	if ( !dirfill(parclust,dirent) || !writefat(fat) )
		temp = FALSE;
	enable();
	return(temp);
}
