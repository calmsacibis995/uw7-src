/*		copyright	"%c%" 	*/
#ident	"@(#)eac:i386/eaccmd/dosutil/pattern.c	1.7"
#ident  "$Header$"
/*
 * pattern.c - pattern matching routines used by dos cmds
 *
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<libgen.h>
#include	"dosutil.h"

extern int Bps;

extern struct format	 disk;		/* details of current DOS disk */
extern struct format	*frmp;
extern struct dosseg	 seg;
extern struct dosseg	*segp;
extern char	 errbuf[BUFMAX];	/* error message string	*/
extern char	*fat;			/* FAT of current DOS disk */
extern int	fd;			/* file descr of current DOS disk */
char	*do_malloc();		/* get memory and handle malloc failure */
short	target;			/* set when target pathname is matched */
short	made_target;		/* flag set to indicate that a target pathname
				   was made when the location matched but file
				   was non-existent. */
short   LOWER=1;		/* unix files names will be lowercase unless
				 * pattern has upper case letter(s) and no lower
				 * case letters */

/*	in->dos is expanded by matching patterns. The expanded
 *	names are returned in dos component of out.
 *	p_idx is a running index into the array of expanded name list
 *	This routine returns the number of expanded names.
 */
match_pattern(in, match_info, tpath)
struct file	*in;
struct match_info	*match_info;
int		tpath;
{
	int 		i=0, totmatch=0;

	target = tpath;		/* target == 1 when match_pattern is called
				   for target pathname. */
	if ( *in->dos ) {	/* do matching for non null dos component */

		/* expand() will return in match_info pointers for pathnames, 
	 	 * which match the pattern  in in->dos */

		totmatch = expand(in->unx, in->dos, match_info);

		if ( LOWER )
		   for ( i == 0; i < totmatch; i++ )
		       lowshift(match_info->name[i],strlen(match_info->name[i]));
		match_info->name[totmatch] = (char *) NULL;
		match_info->type[totmatch] = NULL;
	}
	else { /* no matching needed for null dos component; return 1 */ 

		match_info->name[totmatch] = (char *) NULL;
		match_info->type[totmatch++] = NULL;
	}	

	return (totmatch);
}

/*
 * expand() scans the string in 'pattern'. For each substring ending with
 * '/', if copies the pointer to substring in 'subpattern[n]'.
 * It saves the depth of pathname (number of directories to travese) in subdirs.
 * It calls chk_pathnames() to match pathnames with pattern.
 * Array 'path', is the pointer list of the names that matched.
 * Array file_dir contains 'F' or 'D' for matched pathname being file or directory.
 *
 *
 */

expand(dev,pattern,match_info)
char	*dev, *pattern;
struct match_info	*match_info;
{
	short	more = 1, i, subdirs=0, totmatch;
	short	decided = 0, lower = 0, upper = 0; 
	char	*c, *c1, *subpattern[NFILES], *tmp_pattern;

	/* save pattern in tmp_pattern */

	tmp_pattern=do_malloc(strlen(pattern),"expand");
	strcpy(tmp_pattern, pattern);

	c1 = c = tmp_pattern;
	subpattern[0] = (char * ) NULL;

	/* save subdirectory patterns in subpatterns. save depth in subdirs.  */

	while ( more ) {
	   /* Matched pattern will be shifted to lower case, unless all letters
            * in the pattern are uppercase. Global flag LOWER indicates this.*/

	    if ( ! decided  && isalpha(*c) ) {
		if ( isupper(*c) ) {
			upper++;
		}
		if ( islower(*c) && upper ) {
			upper=0;
			decided++; 
		}
	    }
	    switch (*c) {
		case 0:
			more = 0;
			if ( c1 == c ) /* last char is '/' */
				break;
		case '/':
			*c=NULL;
			if ( c-c1 > 0 ) {
			    subpattern[subdirs]=do_malloc(c-c1,"expand");
			    strcpy(subpattern[subdirs],c1);
			    subdirs++;
			}
			c1=c+1;
			break;
		case '\\':
			c++;
		default:
			break;
	    }
	    c++;
	}
	if (upper) LOWER--;
	free(tmp_pattern);

	/* dos commands are case insensitive. change patterns to upper case */
	for (i=0; i < subdirs; i++ ) {
		upshift(subpattern[i], strlen(subpattern[i]));
	}

	if ((totmatch=chk_pathnames(dev, subdirs, subpattern, match_info))<0){
		sprintf(errbuf,"aborting due to a failure on %s",dev);
		fatal(errbuf,1);
	}

	/* we don't need subpattern[] anymore. */
	for (i=0; i < subdirs; i++ ) {
		if ( subpattern[i] )
			free(subpattern[i]);
	}
	return(totmatch);
}

chk_pathnames(dev, subdirs, subpattern, match_info)
char *dev, *subpattern[];
int subdirs;
struct match_info	*match_info;
{
	unsigned	dirclust;
	int		i, match_idx, totmatch, no_names;
	char		dirent[DIRBYTES], *j, fdflag, fname[30];

	match_idx = totmatch = no_names = 0;

	match_info->name[match_idx] = (char *) NULL;
	match_info->type[match_idx] = NULL;
	made_target=0;

	zero(dirent,DIRBYTES);
	if ( setup_dosmedium(dev, O_RDONLY, 1) < 0 ) 
		return (-1);

	/* get names in the root directory and match with subpattern[0] */

	for (i = 0; (i < frmp->f_dirsect) && (no_names >= 0); i++){
		readsect(segp->s_dir + i,buffer);
		for (j = buffer; j < buffer + Bps; j += DIRBYTES){
			if ((no_names = get_dosname(j, fname, &fdflag)) < 0)
				break;
			if (no_names == 0 )
				continue;

			/* for patterns spanning subdirectories, skip files */
			if ( subdirs > 1 && fdflag == 'F' )
				continue;

			if ( ! gmatch(fname, subpattern[0]) )
				continue;

			/* store  matching name in match_list[k] */
			match_info->name[match_idx]=do_malloc(strlen(fname),"chk_pathnames");
			strcpy(match_info->name[match_idx], fname);
			match_info->type[match_idx]=fdflag;

			/* for deeper pathname, dive into the matched subdir */
			if ( fdflag == 'D' && subdirs > 1 ) {
				no_names = chk_subdir(ROOTDIR,subdirs,0,subpattern,match_info,&match_idx);
				/*no_names=0, if no match in subdirectory */
				if ( ! no_names ) { 
			    		free(match_info->name[match_idx]);
			    		match_info->name[match_idx]=(char *)NULL;
				}

				/* buffer is global, refresh it */
				readsect(segp->s_dir + i,buffer);
			}
			else {
			/* match_info->name[match_idx] points to matching file 
			   or directory. Let's increment match_idx. */

				if ( ++match_idx >= NFILES ) {
				   sprintf(errbuf,"malloc pathnames on DOS medium exceed %d\n",NFILES);
				   fatal(errbuf,1);
			        }
			}

			/* totmatch counts names matched */
			totmatch += no_names;
		}
	}

	/* if pattern is matched for target pathname, and no match occured
	 * and target pattern is not a subdirectory, use the pattern
	 * as the target pathname */

	if (  target && totmatch == 0 && subdirs == 1 ) {
		match_info->name[match_idx] = subpattern[0];
		/* we don't want to free subpattern[0] */
		subpattern[0] = (char *) NULL;
		/* target type is 'U'nknown. It can be a File or Directory */
		match_info->type[match_idx++]='U';
		totmatch++;
	}
	match_info->name[match_idx] =  (char *) NULL;
	match_info->type[match_idx]=NULL;

	close_dosmedium();
	return (totmatch);
}

/*	chk_subdir()  --  starting at the current directory, travel along 
 *		the DOS path  and list all files and directories within it.
 *
 *		clustno :  starting cluster of the current DOS directory.
 *		subdirs : number of directories in the pattern to be matched.
 *		subdir_no: depth of currect subdirectory, depth of root being 0.
 *		subpattern: pointer array to sub patterns for each subdirectory.
 *		match_info: structure pointer to pointer list of matching DOS 
 *			    names and pathname types (file or directory).
 *		match_idx: idx of the last match_list item.
 */

chk_subdir(clustno,subdirs,subdir_no,subpattern,match_info,match_idx)
unsigned clustno;
char *subpattern[];
int *match_idx, subdirs, subdir_no;
struct match_info	*match_info;
{
	unsigned	dirclust, newclustno;
	int 		k, totmatch = 0, no_names = 0;
	char		*bufend, *j, *base_d;
	char		dirent[DIRBYTES], fdflag, fname[30];

	subdir_no++;
	k=*match_idx;	/* k is index into the pointer array match_list */
	base_d = match_info->name[k];

	if ((newclustno=search(clustno,base_d,dirent)) == NOTFOUND) {
		sprintf(errbuf,"(chk_subdir):%s not found",base_d);
		fatal(errbuf,1);
	}
	if (dirent[ATTRIB] & SUBDIR){		/* sub-directory */
		dirclust = word(&dirent[CLUST]);
		bufend   = buffer+ (frmp->f_sectclust * Bps);

 
	/* get names in the sub-directory. Match with subpattern[subdir_no] */

		while (goodclust(dirclust) && (no_names >= 0)){
			if (!readclust(dirclust,buffer)){
			       sprintf(errbuf,"chk_subdir:can't read cluster %u",dirclust);
			       fatal(errbuf,1);
			}
			for (j = buffer; j < bufend; j += DIRBYTES){
				if ((no_names = get_dosname(j,fname, &fdflag)) < 0)
					break;
				if (no_names == 0 )
					continue;

				/* for deeper pathname, skip files */
				if ( subdirs > subdir_no+1 && fdflag == 'F' )
					continue;

				if ( ! gmatch(fname, subpattern[subdir_no]) )
					continue;
				/* store  matching name in match_list[k] */
				match_info->name[k]=do_malloc(strlen(fname)+strlen(base_d)+1,"chk_subdir");
				strcpy(match_info->name[k], base_d);
				strcat(match_info->name[k], "/");
				strcat(match_info->name[k], fname);
				match_info->type[k]=fdflag;

				if ( fdflag == 'D' && subdirs > subdir_no+1 ) {
					/* for deeper pathname, dive into 
					 * the matched subdir */

					no_names = chk_subdir(clustno,subdirs,subdir_no,subpattern,match_info,&k);
					if ( ! no_names )  { /* no match */
						free(match_info->name[k]);
						match_info->name[k]=(char *)NULL;
					}

					/* buffer is global, refresh it */
					readclust(dirclust,buffer);
				}
				else {

				   /* match_info->name[k] points to matching 
				      file or directory.  If a target name was 
				      made for contigency, match_info->name[0]
				      points to it.  Reset match_info->name[0]
				      to match_info->name[k]  */

					if ( made_target ) {
				   		free(match_info->name[0]);
				   		match_info->name[0]=match_info->name[k];
					}
					else	if ( ++k >= NFILES ) {
				   			sprintf(errbuf,"malloc pathnames on DOS medium exceed %d\n",NFILES);
				   			fatal(errbuf,1);
				     		}
				}
				totmatch += no_names;
			}
			dirclust = nextclust(dirclust);
		}
	}

	/* if pattern is matched for target pathname, and no match occured
	 * and this is as deep subdirectory as the pattern specifies,
	 * use the pattern as the target pathname */

	if ( target && totmatch == 0 && k == 0 && subdir_no+1 == subdirs ) {
		/* build a target file name for contingency */
		match_info->name[k]=do_malloc(strlen(subpattern[subdir_no])+strlen(base_d)+1,"chk_subdir");
		strcpy(match_info->name[k], base_d);
		strcat(match_info->name[k], "/");
		strcat(match_info->name[k], subpattern[subdir_no]);
		match_info->type[k++]='F';
		totmatch++;
		made_target++;
	}

	if ( totmatch )
		free(base_d);
	*match_idx=k;
	return (totmatch);
}

/* get_dosname returns dos pathnames in filename. fdflag indicates,
whether the path is a file or directory. */

get_dosname(entry,filename,fdflag)
char	entry[], *filename, *fdflag;
{
	*fdflag=NULL;
	switch(entry[0] & 0xff){
		case DIREND:				/* end of directory */
				return(-1);
		case WASUSED:				/* erased file */
				return(0);
		default:
				return( ls(entry,filename,fdflag) );
	}
}

/* ls(entry, str, fdflag)-- put the directory entry "ls" style in str, and
 *			mark fdflag 'F'(ile) or 'D'(irectory) accordingly.
 *			Return the number of entries listed.
 */

ls(entry, str, fdflag)
char	*entry, *str, *fdflag;
{
	int i;
	char attrib;

	attrib = entry[ATTRIB];
	if (attrib & (HIDDEN | SYSTEM | VOLUME)){
		return(0);
	}
	switch( *(entry+NAME)){			/* status of filename */

		case 0x05:	*str++=0xe5;	/*  encoded   */
				break;		/* first byte */
		case ' ':
		case DIRECT:	return(0);

		default:	*str++ = (*(entry+NAME));
	}
	for (i = NAME + 1; (i < NAME + NAMEBYTES) && (*(entry+i) != ' '); i++)
		*str++ = (*(entry+i));

	if (entry[EXT] != ' '){
		*str++ = ('.');
		for (i = EXT; (i < EXT + EXTBYTES) && (*(entry+i) != ' '); i++)
			*str++ = (*(entry+i));
	}
	*str++ = NULL;
	if ( attrib & SUBDIR )
		*fdflag = 'D';
	else
		*fdflag = 'F';
	return(1);
}

/*	do_malloc(str_size, debug_str) -- mallocs str_size+1 bytes of memory.
			Upon failure, it displays the failure msg including
			debug_str, and does a fatal exit.
 */

char *
do_malloc(str_size, debug_str)
char  *debug_str;
int str_size;
{
	char *p;
	if((p = (char *)malloc(str_size+1)) == (char *)NULL) {
		sprintf(errbuf,"malloc failed for %s - try again",debug_str);
		fatal(errbuf,1);
	}
	return(p);
}

close_dosmedium ()
{
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
#ifdef DEBUG
	fprintf(stderr,"DEBUG exit close_dosmedium()\n" );
#endif
}

setup_dosmedium(dev, devflag, fatalflag)
char *dev;
int devflag, fatalflag;
{
#ifdef DEBUG
	fprintf(stderr,"DEBUG entered setup_dosmedium()\n" );
#endif
	if (!(dev = setup(dev,devflag)))
		return(-1);

	if (!seize(fd)){
		sprintf(errbuf,"can't seize %s",dev);
		fatal(errbuf,0);
		close(fd);
		return(-1);
	}
	if ((fat=(char *)malloc(frmp->f_fatsect*MAX_BPS))==(char *)NULL ||
	    (buffer=(char *)malloc(frmp->f_sectclust*MAX_BPS))==(char *)NULL){
		release(fd);
		fatal("no memory for buffer",1);
	}
	for( ; Bps <= MAX_BPS; Bps += 512 ) {
		if (readfat(fat)){
			break;
		}
	}

	if( Bps > MAX_BPS ) {
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		close_dosmedium();
		fatal(errbuf,fatalflag);
		return(-1);
	}
	return(0);
}

/*
 *	lowshift(string,nchar)  --  convert string to lowercase completely.
 */

lowshift(string,nchar)
char *string;
unsigned nchar;
{
	for (; nchar > 0; nchar--)
		*(string++) = tolower(*string);
}
