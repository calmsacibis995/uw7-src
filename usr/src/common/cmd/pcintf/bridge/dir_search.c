#ident	"@(#)pcintf:bridge/dir_search.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)dir_search.c	6.4	LCC);	/* Modified: 23:21:23 7/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "log.h"
#include "xdir.h"

/*		        External Functions & Variables		*/

#ifdef 	XENIX 	/* to avoid hard to find run-time barfs */
#define sio 	output
#endif	/* XENIX */

#define	dsDbg(dbgArg)	debug(0x20, dbgArg)

LOCAL struct dircontext	*find_dir	PROTO((int));
LOCAL int		match_dir	PROTO((char *, int));
LOCAL void		add_pid		PROTO((int, int));
LOCAL void		add_mode	PROTO((int, int));
LOCAL void		set_offset	PROTO((int, long));


/*			   Directory Context
 *
 * These structures support multiple simultaneous directory search contexts.
 */

struct	dircontext
	dir_tbl[MAXTBL],	        /* Primary search context table */
	*ext_tbl[MAXTBL] = {		/* Pointer to search context tables */
	    dir_tbl
	};

unsigned int
	entries = 0;			/* Current number of search contexts */

int
	current_dir = -1;		/* Identifier of swapped-in directory */

DIR
	*dirdp = NULL;			/* Pointer to opendir() structure */



/*
 *	These routines support multiple simultaneous directory search contexts.
 */


/*
 * find_dir() -		Returns a pointer to the named identifier within
 *			the directory search context table or NULL.
 */

struct dircontext *
find_dir(identifier)
register int identifier;
{ 
    register struct dircontext
	*entryptr,
	*baseptr;

    if (identifier < 0)
	return NULL;

    if (identifier < MAXTBL) {
	entryptr = &dir_tbl[identifier];
	return (entryptr->pathname != NULL) ? entryptr : NULL;
    }

    if (identifier > MAXTBL * MAXTBL)
	return NULL;

    if ((baseptr = ext_tbl[identifier/MAXTBL]) != NULL) {
	entryptr = &baseptr[identifier % MAXTBL];
	return (entryptr->pathname != NULL) ? entryptr : NULL;
    }

    return NULL;
}


/*
 * match_dir() -	Searches directory search context table for
 *			an existing entry.  Returns the logical
 *			search identifer if found otherwise -1.
 */

int
match_dir(pathnm, ppid)
    char *pathnm;               /* Pathname of directory to be searched */
    int  ppid;                  /* process pid to also match */
{
    register int
	i;			/* Loop counters for iterative search */

    register struct dircontext
	*entryptr;		/* Index pointer into dir search table */

    dsDbg(("match_dir(%s)\n", pathnm));

    for (i = 0; i < (MAXTBL * MAXTBL); i++)
    {
	if ((entryptr = find_dir(i)) != NULL
	&&  entryptr->pathname != NULL
	&&  strcmp(entryptr->pathname, pathnm) == 0
	&&  entryptr->pid == ppid)
	{
	    dsDbg(("match_dir returns %#x\n", i));
	    return(i);
	}
    }
    dsDbg(("match_dir returns -1\n"));
    return(-1);
}


/*
 * swapin_dir() -		Swaps in a directory search context.
 *                              Returns NULL if error.
 *                              Returns directory pointer.
 */

DIR *
swapin_dir(identifier)
    register int identifier;    /* Directory search context identifier */
{
    register struct dircontext
	*entryptr;

    dsDbg(("swapin_dir(id=%d)", identifier));

/* Is directory already swapped in? */
    if (current_dir == identifier && entries > 0)
    {
	dsDbg((" alreadyin\n"));
	return dirdp; 
    }

/* State consistency tests */
    if (dirdp == NULL && current_dir != -1)
	current_dir = -1;

    if (dirdp != NULL && current_dir == -1) {
	closedir(dirdp);
	dirdp = NULL;
    }

/* First, swap-out current context */
    if (dirdp) {
	if ((entryptr = find_dir(current_dir)) == NULL) {
	    closedir(dirdp);
	    dirdp = NULL;
	    current_dir = -1;
	} else {
	    /* Clean-up old context */
	    entryptr->rdwr_ptr = telldir(dirdp);
	    closedir(dirdp);
	    dirdp = NULL;
	    current_dir = -1;
	}
    }

/* Swap-in requested context */
    if ((entryptr = find_dir(identifier)) == NULL)
    {
	dsDbg(("id %d not found ret NULL\n", identifier));
	return NULL;

    }
    if ((dirdp = opendir(entryptr->pathname)) == NULL)
    {
	dsDbg(("could not open %s ret NULL\n",
	    entryptr->pathname));
	return NULL;
    }
    if (entryptr->rdwr_ptr > 0)
	seekdir(dirdp, entryptr->rdwr_ptr);
    current_dir = identifier;
    dsDbg((" now curdir is %s\n", entryptr->pathname));
    return dirdp;
}



/*
 * add_dir() -		Adds a new search context to directory search table.
 *			Returns	a logical directory search context identifier
 *			or -1 upon failure.
 */

int
add_dir(pathnm, pattrn, mode, attribute, pid)
    char *pathnm;
    char *pattrn;
int  
	mode,
	attribute,
	pid;
{
    register int i, j;

    register struct dircontext
	*entryptr;		/* Index pointer into search context table */

    int identifier = -1;        /* Status flag which terminates loop */

    char *pathptr;              /* Pointer to space for directory pathname */

    dsDbg(("add_dir(path=%s pat=%s mode=%#x att=%#x pid=%#x)\n",
	pathnm, pattrn, mode, attribute, pid));

/*
 * Find an empty position in dir search context table or extend it.
 */
    for (i = 0; i < MAXTBL && ext_tbl[i] != NULL; i++)
	for (j=0, entryptr=ext_tbl[i]; j < MAXTBL; j++, entryptr++)
	    if ((entryptr->pathname == NULL)
	    && (strlen(entryptr->pattern) == 0)) {
		identifier = (i * MAXTBL) + j;
		goto somewhere;
	    }

/*
 * If no available slots were found determine if we need to extend table.
 */
somewhere:
    if (i >= MAXTBL)
	return(-1);

    if (identifier == -1) {

	dsDbg(("add_dir: no slots available. making more\n"));

	ext_tbl[i] = (struct dircontext *)memory(sizeof dir_tbl);
	(void) memset(ext_tbl[i], 0, sizeof dir_tbl);
	entryptr = ext_tbl[i];
	identifier = i * MAXTBL;
    }

/* Create directory table entry */
    pathptr = memory(strlen(pathnm) + 1);
    strcpy(pathptr, pathnm);
    entryptr->rdwr_ptr = 0;
    entryptr->buf_ptr  = NULL;
    entryptr->pathname = pathptr;
    entryptr->pid      = pid;
    entryptr->attr     = attribute;
    entryptr->mode     = mode;

    strcpy(entryptr->pattern, pattrn);

/* Make sure it can be opened before updating state! */
    if ((dirdp = swapin_dir(identifier)) == NULL) {
	del_dir(identifier);
	return(-1);
    }

/* Update internal state */
    ++entries;
    current_dir = identifier;

    dsDbg(("add_dir: put in context %#x\n", identifier));

    return(identifier);
}



/*
 * del_dir() -			Deletes an entry from the search context table.
 *				Returns TRUE if entry is successfully deleted.
 */

int
del_dir(identifier)
    register int identifier;
{
    register struct dircontext
	*entryptr;		/* Pointer into dir search context table */

    dsDbg(("del_dir(context %#x)\n", identifier));

/* Locate entry within table */
    if ((entryptr = find_dir(identifier)) == NULL)
	return(FALSE);

/* Clear entry for future use */
    free(entryptr->pathname);
    entryptr->pathname = NULL;
    if (entryptr->buf_ptr != NULL)
	free((char *)entryptr->buf_ptr);
    entryptr->buf_ptr  = NULL;
    strcpy(entryptr->pattern, "");
    entryptr->rdwr_ptr = 0;
    entryptr->pid      = 0;
    entryptr->mode     = 0;
    entryptr->attr     = 0;
    entries--;

/* Close the open directory */
    if ((dirdp != NULL) && (current_dir == identifier)) {
	closedir(dirdp);
	dirdp = NULL;
	current_dir = -1;
    }
    return(TRUE);
}



/*
 * getbufaddr() -	Returns the address of the read-ahead output buffer.
 */

struct sio *
getbufaddr(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return (struct sio *)NULL;

	if (entryptr->buf_ptr == NULL)
	    entryptr->buf_ptr  = (struct sio *)memory(sizeof(struct sio));

	return entryptr->buf_ptr;
}



/*
 * getpname() -		Returns the pathname of a search context.
 */

char *
getpname(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return (char *)NULL;
	return(entryptr->pathname);
}



/*
 * get_pattern() -	Returns the search pattern of a dir search context.
 */

char *
get_pattern(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return (char *)NULL;
	return(entryptr->pattern);
}


/*
 * get_attr() -		Returns the MS-DOS search attribute.
 */

int
get_attr(identifier)
register int
	identifier;
{
struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return 0;
	return entryptr->attr;
}


/*
 * get_mode() -		Returns the MS-DOS search mode(MAPPED or UNMAPPED).
 */

int
get_mode(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return 0;
	return entryptr->mode;
}


/*
 * snapshot() -		Stores and returns the current offset into a directory.
 */

long
snapshot(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return 0L;
	return entryptr->rdwr_ptr = telldir(dirdp);
}


/*
 * add_pid() -		Stores process id in a directory search context.
 */

void
add_pid(identifier, pid)
int identifier;
int pid;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return;
	entryptr->pid = pid;
}


/*
 * add_mode() -		Stores search mode (MAPPED/UNMAPPED) in a directory
 *			search context.
 */

void
add_mode(identifier, mode)
int
	identifier,
	mode;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return;
	entryptr->mode = mode;
}



/*
 * add_pattern() -	Stores a new pattern in a directory search context.
 */

void
add_pattern(identifier, pattrn)
int
	identifier;
char
	*pattrn;
{
register struct dircontext
	*entryptr;

    if ((entryptr = find_dir(identifier)) == NULL)
	return;
    strcpy(entryptr->pattern, pattrn);
}



/*
 * add_offset() -	Stores and advances the read/write pointer in the 
 *			directory search context table.
 */

void
add_offset(identifier)
int
	identifier;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return;

#ifdef ALLOW_MODIFY_SEARCH_OFFSET
	/*
	   If this line is turned on then the DOS side will be allowed
	   to modify the current position in a directory search.  This
	   is not legal, but some very old programs did it by copying
	   the FCB entries.  Unfortunately the DOS side can only handle
	   16 bit positions inside directories.  With the NFS support
	   you now can get bits on in the high word of the long that is
	   returned by telldir, so this doesn't work too well anymore.
	   So we desupport these ancient DOS programs by no longer
	   allowing them to change the offset.
	*/
	entryptr->rdwr_ptr = offset;
#endif	/* ALLOW_MODIFY_SEARCH_OFFSET */
	seekdir(dirdp, entryptr->rdwr_ptr);
}



/*
 * add_attribute() -	Stores a new attribute in a directory search context.
 */

void
add_attribute(identifier, attribute)
int
	identifier;
int
	attribute;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return;
	entryptr->attr = attribute;
}



/*
 * same_context() - 	Determines if the requested search context is the
 *			same as what is stored in the state vector.  Returns 
 *			TRUE if the context is the same otherwise FALSE.
 */

int
same_context(identifier, pat, attribute)
register int
	identifier;
char
	*pat;
register int
	attribute;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return FALSE;

	if ((strcmp(pat, entryptr->pattern) == 0 || *pat == '\0')
#ifdef ALLOW_MODIFY_SEARCH_OFFSET
	/*
	   This line used to allow the DOS side to change the offset
	   within a directory during a search.  Since this is no longer
	   allowed, we need not check whether the DOS side has requested
	   a different value than we recorded from last time.  Indeed, the
	   values could be different even if the DOS side was not trying
	   to modify the position in the case where the telldir return
	   value does not fit within the 16 bit offset value that the
	   DOS side stores.
	*/
	&&  offset == entryptr->rdwr_ptr
#endif	/* ALLOW_MODIFY_SEARCH_OFFSET */
	&&  attribute == entryptr->attr)
		return TRUE;

	return FALSE;
}



/*
 * set_offset() -		Writes the new offset to the state vector.
 */

void
set_offset(identifier, offset)
int
	identifier;
long
	offset;
{
register struct dircontext
	*entryptr;

	if ((entryptr = find_dir(identifier)) == NULL)
		return;
	entryptr->rdwr_ptr = offset;
}


/*
 * deldir_pid()	-	Deletes all search contexts with a specific pid.
 */

void
deldir_pid(pid)
register int
	pid;
{
register int
	i;			/* For loop counter */
register struct dircontext
	*entryptr;

/* delete dir context if process id matches pid */
    for (i = 0; i < (MAXTBL * MAXTBL); i++)
	if ((entryptr = find_dir(i)) != NULL && entryptr->pid == pid)
		del_dir(i);
}


/*
 * dump_dir()	-	dumps all search contexts
 */

void
dump_dir()
{
	register int i;			/* For loop counter */
	register struct dircontext *ep;

	/* dump dir context */
    for (i = 0; i < (MAXTBL * MAXTBL); i++)
	if ((ep = find_dir(i)) != NULL) {
		log("Dir Context %3d: mode %d, pid %04x, pattern %s/%s\n",
			i, ep->mode, ep->pid, ep->pathname, ep->pattern);
	}
}
