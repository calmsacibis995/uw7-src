/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dir.c	1.2.1.4"
#ident  "$Header$"
/*	@(#) dir.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 *	MODIFICATION hISTORY
 *	M000	ericc	Feb 13, 1985
 *	- The root directory may contain a volume label, which is a directory
 *	  entry with the attribute byte set to VOLUME.  Since findent() is
 *	  to only find files or directories, it skips over the volume label
 *	  when searching through the root directory.
 *	M001	sco!rr	4 Apr 1988
 *	- Issue a warning when truncating any part of the filename (sometimes
 *	  truncation is necessary when copying to DOS).
 *	- I guess dots (...) are not legal filename or extension characters
 *	  in DOS. So the algorithm for altering a Unix filename to a DOS
 *	  filename is now (in addition to possible truncation):
 *		a) strip leading dots
 *		b) find next dot - use that as end of basename
 *		c) strip leading dots off of extension
 *		d) use extension string up until next dot (if any) as suffix
 *	  This allows doscp of files like .profile and test...four and a.b.c
 *	  into legal dos files (even tho the name change may be rather drastic).
 *	- Strip filename of all illegal DOS filename characters.
 */

#include	<stdio.h>
#include	"dosutil.h"

extern int Bps;

/*	findent()  --  find the directory entry of a DOS file, and copy it
 *		into dirent.  The cluster containing this directory entry, or
 *		NOTFOUND if there is no such entry, is returned.  On entry,
 *		dirent contains the name and extension of the DOS file; on
 *		exit, dirent contains the whole directory entry.
 *
 *		dirclust :  starting cluster of the directory to examine
 *		dirent   :  buffer for the directory entry, as explained
 *
 *		NOTE:	Volume labels in the root directory are ignored.
 */

unsigned findent(dirclust,dirent)
unsigned dirclust;
char *dirent;
{
	unsigned i;
	char *bufend, *j, *name;

	name = &dirent[NAME];

	if (dirclust == ROOTDIR)
		for (i = 0; i < frmp->f_dirsect; i++){
			readsect(segp->s_dir + i,buffer);
			for (j = buffer; j < buffer + Bps; j += DIRBYTES){
#ifdef DEBUG
				fprintf(stderr,"DEBUG findent %.11s\n",j);
 
#endif
				if (*j == DIREND)
					return(NOTFOUND);
				if (!strncmp(name,&j[NAME],NAMEBYTES+EXTBYTES)&&
				    !(j[ATTRIB] & VOLUME)){
					movchar(dirent,j,DIRBYTES);
					return(dirclust);
				}
			}
		}
	else{
		bufend = buffer + (frmp->f_sectclust * Bps);

		while (goodclust(dirclust)){
			if (!readclust(dirclust,buffer)){
				sprintf(errbuf,
					"can't read cluster %u",dirclust);
				fatal(errbuf,1);
			}
			for (j = buffer; j < bufend; j += DIRBYTES){
#ifdef DEBUG
				fprintf(stderr,"DEBUG findent %.11s\n",j);
 
#endif
				if (*j == DIREND)
					return(NOTFOUND);
				if (!strncmp(name,&j[NAME],NAMEBYTES+EXTBYTES)){
					movchar(dirent,j,DIRBYTES);
					return(dirclust);
				}
			}
			dirclust = nextclust(dirclust);
		}
	}
	return(NOTFOUND);
}


/*	makename(path,name)  --  given a DOS path, fill in the name (including
 *		the extension) of the highest level component, ie. directory or
 *		file.  A pointer to the next component on the path is returned.
 *
 *	WARNINGS: If EXTSEP is a '.', then "." and ".." won't work as paths.
 */
int _warn=1;	/* M001 */
char invalid[] = "[]?\\=\",+*:; <>|";	/* M001 */

char *makename(path,name)
char *path,*name;
{
	int n, ill=0;
	char *c, *d, *e, *f, *g, *extension, *newpath, *oldpath;

	blank(name,NAMEBYTES + EXTBYTES);
	while (*path == DIRSEP)		/* ignore leading DIRSEPs */
		path++;
	while (*path == EXTSEP)		/* ignore leading EXTSEPs */
		path++;			/* M001 begin */
	newpath=malloc(strlen(path)+1);
	oldpath=path;
	while ((c = strpbrk(path,invalid)) != NULL) {
		strncat(newpath,path,c-path);
		path=++c;
		ill=1;
	}
	if (ill) {
		strcat(newpath,path);
		path = newpath;
		if ((strlen(path) == 0) && (_warn)) {
			printf("Error: %s is an illegal DOS filename\n",oldpath);
			return(NULL);
		}
	}				/* M001 end */
	if ((d = strchr(path,DIRSEP)) == NULL)
		d = strchr(path,NULL);

	if (((e = strchr(path,EXTSEP)) == NULL) || (e > d))
		f = e = d;
	else{				/* extension */
		f = e;			/* save 1st occurence of EXTSEP M001 */
		while(*e == EXTSEP)	/* ignore leading EXTSEP's in */
			e++;		/* extension  M001 */
		if ((g = strchr(e,EXTSEP)) == NULL || (g > d)) /* M001 */
			n = min( (int) (d - e), EXTBYTES );
		else
			n = min( (int) (g - e), EXTBYTES ); /* M001 */
		strncpy(name + NAMEBYTES, e, n);
	}
	n = min( (int) (f - path), NAMEBYTES );
	strncpy(name,path,n);
	upshift(name,NAMEBYTES + EXTBYTES);

	if (*name == 0xe5)					/*  encoded   */
		*name = 0x05;					/* first byte */
	if ((_warn)&&((ill)||((int)(f-path)>NAMEBYTES)||((int)(d-e)>EXTBYTES)))
		printf("Warning: renaming filename %s to %.8s.%.3s\n",
				oldpath,name,name+NAMEBYTES);	/* M001 */
	return(d);
}


/*	search()  --  starting at the current directory, travel along the DOS
 *		path until the directory entry of the last component is found.
 *		This directory entry is copied into dirent, and the cluster
 *		containing this directory entry returned.  If the path cannot
 *		be travelled, NOTFOUND is returned.
 *
 *		clustno :  starting cluster of the current DOS directory.
 *		path    :  DOS path name of the DOS file, relative to the
 *				current DOS directory.
 *		dirent  :  buffer for directory entry of the DOS file.
 */

unsigned search(clustno,path,dirent)
unsigned clustno;
char *path, *dirent;
{
	unsigned dirclust;
	char *makename(), *nextlevel;

	_warn=0;
	nextlevel = makename(path,&dirent[NAME]);
	_warn=1;

#ifdef DEBUG
	fprintf(stderr,"DEBUG search for name %.8s\textension %.3s\n",
			&dirent[NAME],&dirent[EXT]);
#endif

	if ((dirclust = findent(clustno,dirent)) == NOTFOUND)
		return(NOTFOUND);
	if (*nextlevel == (char ) NULL)				/* eureka ! */
		return(dirclust);
	else{							/* dig deeper */
		clustno = word(&dirent[CLUST]);
		return( search(clustno,nextlevel,dirent) );
	}
}
