#ident	"@(#)pcintf:bridge/p_search.c	1.1.1.5"
#include	<sccs.h>
SCCSID(@(#)p_search.c	6.14	LCC);	/* Modified: 15:25:06 4/16/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include <pwd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "table.h"
#include "xdir.h"

#ifdef FAST_FIND
#	include "flip.h"
#	include "version.h"
#endif	/* FAST_FIND */

#ifdef 	XENIX 	/* to avoid hard to find run-time barfs */
#define sio 	output
#endif	/* XENIX */

#define	sDbg(dbgArg)

LOCAL char		*getdir_entry		PROTO((char *, DIR *, char *, char *, int, int, int, struct stat *));
LOCAL int		format_response		PROTO((char *, char *, int, unsigned short, char *, ino_t));
LOCAL void		fill_sio_buffer		PROTO((struct sio *, DIR *, int, char *, char *, int));
LOCAL struct sio	*stat_n_statahead	PROTO((struct sio *, DIR *, int, char *, char *, int));
LOCAL struct sio	*fast_path		PROTO((struct sio *, char *, char *, int, int, int));

#if defined(FAST_FIND)

LOCAL struct sio	*fast_find_fill		PROTO((struct sio *, DIR *, int, char *, char *, int));

#endif	/* FAST_FIND */

extern unsigned short	btime	PROTO((struct tm *));
extern unsigned short	bdate	PROTO((struct tm *));


/*			    External Variables				*/

extern unsigned char	brg_seqnum;         /* bridge frame sequence number */
extern unsigned char	request;            /* bridge frame request type */
extern  int swap_how;           /* How to swap output packets */
extern int flipBytes;		/* Byte ordering flag */
extern  int outputframelength;  /* Length of last output frame */


extern  struct output *optr;             /* Pointer to last output frame sent */


extern  struct ni2 ndata;       /* Ethernet header */


extern  struct output out1;     /* Output frame buffers */

#ifdef	JANUS
int  doingfunny = FALSE;        /* boolean: dealing with funny name or not */
#endif	/* JANUS */

#ifdef FAST_FIND
struct find_ctx {
	unsigned short	fc_length;	/* Length of this context */
	short	fc_umode;	/* Unix File Mode */
	long	fc_offset;	/* Directory offset of next entry */
	short	fc_ftime;	/* File Time */
	short	fc_fdate;	/* File Date */
	long	fc_fsize;	/* File Size */
	char	fc_fattr;	/* File Attributes */
	char	fc_fname[3];	/* File Name (Extended name) */
};
#endif	/* FAST_FIND */


LOCAL char *
getdir_entry(responseptr, dirptr, pathname, pattern, hiddenflg,
     subdirflg, mode, filstatptr)
    char        *responseptr;       /* Pointer to response buffer */
    DIR         *dirptr;            /* Pointer to opendir() structure */
    char        *pathname;     /* Pathname component of srch pattern */
    char        *pattern;     /* Filename component of srch pattern */
    int         hiddenflg;          /* Match on hidden files */
    int         subdirflg;          /* Match on subdirectorys */
    int         mode;               /* Match on Mapped/Unmapped filenames */
    struct stat *filstatptr;        /* Pointer to stat() structure */
{

    register struct dirent *direntryptr;   /* Pointer to opendir() entry */
    char mappedname[MAX_FN_COMP + 1];	/* mapped candidate directory entries */
    char statname[MAX_FN_TOTAL];         /* Name of file to stat() */

    while ((direntryptr = readdir(dirptr)) != NULL)
    {
	if (!hiddenflg && direntryptr->d_name[0] == '.'
	  &&  (!is_dot(direntryptr->d_name))
	  && (!is_dot_dot(direntryptr->d_name)) )
	    continue;

	strcpy(mappedname,direntryptr->d_name);
	if (
#ifdef	JANUS
            !doingfunny &&
#endif
	    mode == MAPPED)
	    mapdirent(pathname, mappedname, (ino_t)direntryptr->d_ino);
	if (match(mappedname, pattern, mode) == FALSE)
	    continue;

    /* Construct pathname for stat() */
	strcpy(statname, pathname);
	if (strcmp(statname, "/"))
		strcat(statname, "/");
	strcat(statname, direntryptr->d_name);

	if ((stat(statname, filstatptr)) < 0)
	    continue;
	if ((!subdirflg) && ((filstatptr->st_mode & S_IFMT) == S_IFDIR))
	    continue;

	strcpy(responseptr, direntryptr->d_name);
	return responseptr;
    }
    return NULL;
}



/*
 *	format_response() -	Returns a filename in MAPPED or UNMAPPED form.
 */

#if defined(__STDC__)
LOCAL int
format_response(char *directory, char *filename, int mode,
		unsigned short uid, char *response, ino_t inode)
#else
LOCAL int
format_response(directory, filename, mode, uid, response, inode)
    char 	*directory;		/* Directory of file */
    char 	*filename;		/* Filename to map */
    int		mode;			/* Indicates Mapped/Unmapped format */
    unsigned short uid;			/* User id */
    char	*response;		/* location for response */
    ino_t	inode;
#endif
{
    register int        stringlen;      /* Length of filename to map */
    register struct passwd *passptr;    /* Pointer to password struct */
    char *stringptr,
	 *userptr;


    sDbg(("format_response:in(dir=%s filename=%s mode=%d. uid=%d inode=%ld.)\n",
	directory, filename, mode, uid, (long)inode));
/* Send response in either Mapped or Unmapped or Funny form */
    cvt_fname_to_dos(mode, (unsigned char *)directory,
	(unsigned char *)filename, (unsigned char *)response, inode);
    stringlen = strlen(response);
    if (mode == UNMAPPED) {
	if (stringlen > MAX_FN_COMP)
	    stringlen = MAX_FN_COMP;
	response[stringlen] = '\0';
	stringptr = userptr = response + stringlen + 1;
	if ((passptr = getpwuid((int) uid)) != NULL &&
	    (passptr->pw_name && (*passptr->pw_name)))
		strcpy(stringptr, passptr->pw_name);
	else
		sprintf(stringptr,"%d",uid);

	stringlen += strlen(stringptr) + 1;
	response[stringlen] = '\0';
	stringptr = response + stringlen + 1;

	cvt_fname_to_dos(MAPPED, (unsigned char *)directory,
	    (unsigned char *)filename, (unsigned char *)stringptr, inode);
	stringlen += strlen(stringptr) + 1;

	sDbg(("format_resp: filename %s, username %s, mappedname %s\n",
		filename, userptr, stringptr));
	sDbg(("format_resp: stringlen %d\n",stringlen));
		
    }
    sDbg(("format_response:out(dir=%s filename=%s mode=%d. uid=%d.)\n",
	directory, filename, mode, uid));
    return(stringlen);
}


LOCAL void
fill_sio_buffer(saddr, dirptr, search_id, pathname, pattern, mode)
struct sio	*saddr;                 /* Points to read-ahead output buff */
DIR		*dirptr;		/* Pointer to opendir() structure */
int		search_id;		/* Logical search identifier */
char		*pathname;		/* Pathname component */
char		*pattern;		/* Filename cmpnt of search pattern */
int		mode;			/* Mode is Mapped or Unmapped */
{
    register int        hflg;           /* Hidden attributes */
    register int        sflg;           /* Hidden and subdir attributes */
    register int        stringlen;      /* Length of response string */
    register int	vdesc;		/* virtual desc for given open file */
    struct tm           *timeptr;       /* Pointer to localtime() struct */
    struct stat		filstat;	/* Stat structure */
    char		filename[MAX_FN_COMP];

    hflg  = ((char)get_attr(search_id) & HIDDEN) ? TRUE : FALSE;
    sflg  = ((char)get_attr(search_id) & SUB_DIRECTORY) ? TRUE : FALSE;

    memset(&saddr->hdr, 0, sizeof(struct header));

/* Move next matching directory entry into output buffer */
    if (getdir_entry(filename, dirptr, pathname, pattern,
	hflg, sflg, mode, &filstat) == NULL) {
	sDbg(("fill_sio_buffer: NO_MORE_FILES\n"));
	saddr->hdr.res  = NO_MORE_FILES;
	saddr->hdr.stat = NEW;
	saddr->pre.select = BRIDGE;
	saddr->hdr.fdsc = (unsigned short)search_id;
	return;
    }

/* Format output according to mode: Mapped or Unmapped */
    stringlen = format_response(pathname, filename, mode,
				filstat.st_uid, saddr->text, filstat.st_ino);
#ifdef	JANUS
    if (doingfunny) {
	toofunny(saddr->text);
	stringlen = strlen(saddr->text);
	doingfunny = FALSE;
    }
#endif	/* JANUS */

    snapshot(search_id);		/* Save the current dir pointer */

/* Fill-in response frame */
    saddr->hdr.res = SUCCESS;
    saddr->hdr.req = request;
    saddr->hdr.seq = brg_seqnum;
    saddr->pre.select = BRIDGE;
    saddr->hdr.stat = 0;
    saddr->hdr.fdsc = (unsigned short)search_id;
    saddr->hdr.t_cnt = (unsigned short)(stringlen + 1);
    saddr->hdr.offset = telldir(dirptr);
    saddr->hdr.mode = filstat.st_mode;
    saddr->hdr.f_size = filstat.st_size;
    saddr->hdr.attr = attribute(&filstat, filename);

    /*
     * Return time last modified to reflect the correct time if someone else
     * has written and closed the file.
     */
    timeptr = localtime (&(filstat.st_mtime));	/* UNIX time stamp */
    saddr->hdr.date = bdate(timeptr);
    saddr->hdr.time = btime(timeptr);
}


LOCAL struct sio *
stat_n_statahead(saddr, dirptr, search_id, pathname, pattern, mode)
    struct sio  *saddr;                 /* Points to read-ahead output buff */
    DIR		*dirptr;		/* Pointer to opendir() structure */
    int		search_id;		/* Logical search identifier */
    char	*pathname;		/* Pathname component */
    char        *pattern;		/* Filename cmpnt of search pattern */
    int		mode;			/* Mode is Mapped or Unmapped */
{

/* Move next matching directory entry into output buffer */
    fill_sio_buffer(saddr, dirptr, search_id, pathname, pattern, mode);
    if (saddr->hdr.res != SUCCESS) {
	del_dir(search_id);
	return saddr;
    }

    optr = (struct output *)saddr;

    outputframelength = xmtPacket(optr, &ndata, swap_how);
    sDbg(("about to do statahead search\n"));
    saddr = getbufaddr(search_id);

/* Move next matching directory entry into stat ahead buffer */
    fill_sio_buffer(saddr, dirptr, search_id, pathname, pattern, mode);
    if (saddr->hdr.res != SUCCESS)
	del_dir(search_id);
    return NULL;
}


/* 
 * If the search pattern for a search first call does not contain any
 * wildcard characters we take a fast path and just stat the specified     
 * file.  No search context is created, and a subsequent search next     
 * will fail.  (As it would have anyway.)     
 *     
 */     
LOCAL struct sio *
fast_path(saddr, pathcomp, namepat, mode, attr, request)
struct sio *saddr;
char *pathcomp, *namepat;
int mode, attr;
int request;
{
    char fulltemp[MAX_FN_TOTAL];
    char *nameptr;
    struct stat statbuf;
    int stringlen;
    int vdesc;			/* virtual descriptor for given open file */
    struct tm *timeptr;
    
    strcpy(fulltemp,pathcomp);
    strcat(fulltemp,"/");
    nameptr = &fulltemp[strlen(fulltemp)];
    if (mode == MAPPED) {
	sDbg(("fastpath: mode = MAPPED\n"));
	set_tables(U2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0, country);
	lcs_translate_string(nameptr, MAX_FN_TOTAL, namepat);
	unmapfilename(CurDir, nameptr);
    } else {
	sDbg(("fastpath: mode = %d\n", mode));
	strcpy(nameptr, namepat);
    }
    sDbg(("fast_path: filename: %s\n",fulltemp));
    if (stat(fulltemp,&statbuf)) {
	*strrchr(fulltemp,'/') = 0;
	if (fulltemp[0] && stat(fulltemp,&statbuf))
	    saddr->hdr.res = PATH_NOT_FOUND;
	else
	    saddr->hdr.res = NO_MORE_FILES;
	sDbg(("fast_path: Can't stat %s (%d)\n",fulltemp,errno));
	return saddr;
    }
    else {
	/* if directory then 
	      if (attr not set) then 
		  rtn NO_MORE_FILES 
        */
	if (statbuf.st_mode & S_IFDIR)
	   if (!(attr & SUB_DIRECTORY)){
		saddr->hdr.res = NO_MORE_FILES;
		return saddr;
           }
	}

/* Fill-in response frame */

#ifdef FAST_FIND
    if (request != SEARCH && (bridge_ver_flags & V_FAST_FIND)) {
	struct find_ctx *fctx;		/* File Context structure */

	short tmpShort;			/* Temporary for flipping */
	long  tmpLong;			/* Temporary for flipping */

	fctx = (struct find_ctx *)saddr->text;
	stringlen = format_response(pathcomp, nameptr, mode,
	    (unsigned short) statbuf.st_uid, fctx->fc_fname, statbuf.st_ino);
	saddr->hdr.res = SUCCESS;
	saddr->hdr.t_cnt = fctx->fc_length = (unsigned short)stringlen + 1 +
							sizeof(struct find_ctx);
	saddr->hdr.fdsc = -1;
	fctx->fc_offset = 0;
	fctx->fc_umode = statbuf.st_mode;
	fctx->fc_fsize = statbuf.st_size;
	fctx->fc_fattr = attribute(&statbuf, nameptr);

	/*
	 * Return time last modified to reflect the correct time if someone else
	 * has written and closed the file.
	 */
	timeptr = localtime (&(statbuf.st_mtime));	/* UNIX time */
	fctx->fc_fdate = bdate(timeptr);
	fctx->fc_ftime = btime(timeptr);

	if (flipBytes) {
	    dosflipm(fctx->fc_length, tmpShort);
	    dosflipm(fctx->fc_ftime, tmpShort);
	    dosflipm(fctx->fc_fdate, tmpShort);
	    lsflipm(fctx->fc_fsize, tmpLong);
	    dosflipm(fctx->fc_umode, tmpShort);
	}
	return saddr;
    }
#endif	/* FAST_FIND */

    stringlen = format_response(pathcomp, nameptr, mode,
		(unsigned short) statbuf.st_uid, saddr->text, statbuf.st_ino);
    saddr->hdr.res = SUCCESS;
    saddr->hdr.t_cnt = (unsigned short)stringlen + 1;
    saddr->hdr.fdsc = -1;
    saddr->hdr.offset = (request == SEARCH) ? 65535L : 0;
    saddr->hdr.mode = statbuf.st_mode;
    saddr->hdr.f_size = statbuf.st_size;
    saddr->hdr.attr = attribute(&statbuf, nameptr);

   /*
    * Return time last modified to reflect the correct time if someone else
    * has written and closed the file.
    */
    timeptr = localtime (&(statbuf.st_mtime));	/* UNIX time stamp */
    saddr->hdr.date = bdate(timeptr);
    saddr->hdr.time = btime(timeptr);
    return saddr;
}


#ifdef FAST_FIND

/*
 *  The fast_find_fill procedure fills the entire text area of the output
 *  packet with directory search results.
 */

struct sio *
fast_find_fill(saddr, dirptr, search_id, pathname, pattern, mode)
struct sio	*saddr;                 /* Points to read-ahead output buff */
DIR		*dirptr;		/* Pointer to opendir() structure */
int		search_id;		/* Logical search identifier */
char		*pathname;		/* Pathname component */
char		*pattern;		/* Filename cmpnt of search pattern */
int		mode;			/* Mode is Mapped or Unmapped */
{
	int		i;		/* number of buffer slots to use */
	struct sio	sio1;		/* Structure to put next entry in */
	unsigned short	pktlen;		/* Length remaining in packet */
	unsigned short	ctxlen;		/* Length of the current context */
	struct find_ctx *fctx;		/* Find context structure */

	short tmpShort;			/* Temporary for flipping */
	long  tmpLong;			/* Temporary for flipping */

	pktlen = MAX_OUTPUT;
	saddr->hdr.t_cnt = 0;

	for (i = 7; i >= 0; i--) {
		fill_sio_buffer(&sio1, dirptr, search_id,pathname,pattern,mode);
		if (sio1.hdr.res != SUCCESS) {
			if (saddr->hdr.t_cnt > 0)	/* Already have some */
				break;
			/* Return no more files */
			sDbg(("fast_find_fill: NO_MORE_FILES\n"));
			saddr->hdr.res  = NO_MORE_FILES;
			saddr->hdr.stat = NEW;
			saddr->pre.select = BRIDGE;
			saddr->hdr.fdsc = (unsigned short)search_id;
			del_dir(search_id);
			return saddr;
		}

		/* If this entry won't fit in the current packet, break */
		ctxlen = sio1.hdr.t_cnt + sizeof(struct find_ctx);
		ctxlen &= ~3;		/* Make sure the entry is padded */
		if (ctxlen > pktlen)
			break;

		/* Place entry into packet */
		fctx = (struct find_ctx *)&saddr->text[saddr->hdr.t_cnt];
		fctx->fc_length = ctxlen;
		fctx->fc_offset = sio1.hdr.offset;
		fctx->fc_ftime = sio1.hdr.time;
		fctx->fc_fdate = sio1.hdr.date;
		fctx->fc_fsize = sio1.hdr.f_size;
		fctx->fc_umode = sio1.hdr.mode;
		fctx->fc_fattr = sio1.hdr.attr;
		(void)memcpy(fctx->fc_fname, sio1.text, sio1.hdr.t_cnt);

		if (flipBytes) {
		    dosflipm(fctx->fc_length, tmpShort);
		    lsflipm(fctx->fc_offset, tmpLong);
		    dosflipm(fctx->fc_ftime, tmpShort);
		    dosflipm(fctx->fc_fdate, tmpShort);
		    lsflipm(fctx->fc_fsize, tmpLong);
		    dosflipm(fctx->fc_umode, tmpShort);
		}
		pktlen -= ctxlen;
		saddr->hdr.t_cnt += ctxlen;
	}

	/* Fill-in response frame */
	saddr->hdr.res = SUCCESS;
	saddr->hdr.req = request;
	saddr->hdr.seq = brg_seqnum;
	saddr->pre.select = BRIDGE;
	saddr->hdr.stat = 0;
	saddr->hdr.fdsc = (unsigned short)search_id;

	return saddr;
}

#endif	/* FAST_FIND */


/*
 * The MS-DOS Search First/ Search Next & Find-First/Find-Next commands
 * use a "read ahead" algorithm which causes the transaction response of
 * the next Find-Next/Search-Next to be built before the request arrives.
 * Since the UNIX server must support simultaneous search contexts
 * the frame to follow is built and stored in the search context state vector.
 */

struct	sio	*
pci_sfirst(dos_fname, request, mode, attr, pid)
    char	*dos_fname;		/* Search pattern */
    int		request;		/* Distinguishes SEARCH/FIND */
    int		mode;			/* Mode of search (Mapped/Unmapped) */
    int		attr;			/* MS-DOS file attribute */
    int         pid;                    /* Id of DOS process calling srch */
{
    register int        search_id;
    register struct sio *saddr;         /* Points to read-ahead output buff */
    register char       *slashptr;      /* Pointer to last slash in path */
    DIR                 *dirptr;        /* Pointer to opendir() structure */
    struct stat         filstat;        /* Stat structure */
    char		filename[MAX_FN_TOTAL];
    char                pathcomp[MAX_FN_TOTAL];
    char                *namepat;

/* Translate MS-DOS search pattern to UNIX */
    saddr  = (struct sio *)&out1;

    if (cvt_fname_to_unix(UNMAPPED, (unsigned char *)dos_fname,
	(unsigned char *)filename)) {
	saddr->hdr.res = PATH_NOT_FOUND;
	sDbg(("pci_sfirst: could not unmap: %s\n",pathcomp));
	return saddr;
    }
#ifdef	JANUS
    doingfunny = fromfunny(filename); /* this translates funny names, as */
#endif  /* JANUS            well as autoexec.bat, con, ibmbio.com, ibmdos.com  */

    sDbg(("pci_sfirst:filename=%s\n", filename));
/* Return uname() as MS-DOS local volume */
    if (attr == VOLUME_LABEL) {
#ifdef FAST_FIND
      if (request != SEARCH && (bridge_ver_flags & V_FAST_FIND)) {
	struct find_ctx *fctx;		/* File Context structure */

	short tmpShort;			/* Temporary for flipping */
	long  tmpLong;			/* Temporary for flipping */

	fctx = (struct find_ctx *)saddr->text;
	cvt_to_dos((unsigned char *)myhostname(),
	    (unsigned char *)fctx->fc_fname);
	saddr->hdr.res = SUCCESS;
	saddr->hdr.stat = NEW;
	saddr->hdr.attr = VOLUME_LABEL;

	saddr->hdr.t_cnt = fctx->fc_length =
			   (unsigned short)strlen(fctx->fc_fname) + 1 +
			   sizeof(struct find_ctx);
	saddr->hdr.fdsc = -1;
	fctx->fc_fattr = VOLUME_LABEL;
	fctx->fc_offset = 0;
	fctx->fc_umode = 0;
	fctx->fc_fsize = 0;
	fctx->fc_fdate = 0;
	fctx->fc_ftime = 0;

	if (flipBytes)
	    dosflipm(fctx->fc_length, tmpShort);
	sDbg(("pci_sfirst:Volume=%s\n", fctx->fc_fname));
	return saddr;
      }
#endif	/* FAST_FIND */
	cvt_to_dos((unsigned char *)myhostname(), (unsigned char *)saddr->text);
	saddr->hdr.res = SUCCESS;
	saddr->hdr.stat = NEW;
	saddr->hdr.t_cnt = (unsigned short)strlen(saddr->text) + 1;
	saddr->hdr.attr = VOLUME_LABEL;
	sDbg(("pci_sfirst:Volume=%s\n", saddr->text));
	return saddr;
    }

/* Split search pattern into directory component and filename component */
    if ((slashptr = strrchr(filename, '/')) == NULL) {
	namepat = filename;
	strcpy(pathcomp, CurDir);
    } else {
	namepat = slashptr + 1;
	if (slashptr == filename)
	    strcpy(pathcomp, "/");
	else {
	    *slashptr = '\0';
	    if (mode == MAPPED) {
		set_tables(U2U);
		lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0, country);
		lcs_translate_string(pathcomp, MAX_FN_TOTAL, filename);
		unmapfilename(CurDir, pathcomp);
	    } else
		strcpy(pathcomp, filename);
	    *slashptr = '/';
	}
    }
    sDbg(("pci_sfirst:p=%s n=%s\n", pathcomp, namepat));

    if (
#ifdef JANUS
	 !doingfunny &&
#endif
	 !wildcard(namepat,strlen(namepat))
       )
        return fast_path(saddr, pathcomp, namepat, mode, attr, request);

    /* check for read and execute (search) permission of directory */
    if (access(pathcomp, 05) != 0)
    {
	saddr->hdr.res = PATH_NOT_FOUND; /* means invalid path */
	sDbg(("pci_sfirst: No access to directory.\n"));
	return saddr;
    }

/* Is it really a directory? */
    stat(pathcomp, &filstat);
    if ((filstat.st_mode & S_IFMT) != S_IFDIR)
    {
	saddr->hdr.res = PATH_NOT_FOUND; /* means invalid path */
	sDbg(("pci_sfirst: Directory component isn't a directory\n"));
	return saddr;
    }

/*
 * There used to be a notice of SPAM here. The notice said that there
 * was no way to tell from a DOS command line when a context should be
 * removed. I differ. Contexts are removed when (a) a process exits or
 * (b) NO_MORE_FILES is returned on a given context.
 *
 * As a result, all search/find firsts will create new contexts where
 * before they would have re-used a context if the directory was the same.
 * The only problem one can run into is some program performing many
 * search/find firsts without completing the searchs on every context
 * and never exiting.
 */

    if ((search_id = add_dir(pathcomp, namepat, mode,
    	attr, pid)) < 0)
    {
    	saddr->hdr.res = PATH_NOT_FOUND;
	sDbg(("pci_sfirst:could not add_dir :%d\n", search_id));
	return saddr;
    }

    if ((dirptr = swapin_dir(search_id)) == NULL)
    {
	saddr->hdr.res = FAILURE;
	del_dir(search_id);		/* delete context */
	sDbg(("pci_sfirst:could not swapin_dir(%d)\n", search_id));
	return saddr;
    }

#ifdef FAST_FIND
    if (request != SEARCH && (bridge_ver_flags & V_FAST_FIND))
	return fast_find_fill(saddr, dirptr, search_id, pathcomp, namepat,mode);
#endif	/* FAST_FIND */

    sDbg(("pci_sfirst:about to statahead\n"));
/* Send first matching filename and then build next response frame */
    return stat_n_statahead(saddr, dirptr, search_id, pathcomp, namepat, mode);
}



/*
 * The "search next" command follows the "search" command utilizing: 
 * path, state, addr, optr, and toggle as state variables.
 * Search next commands are issued by MS-DOS until a NO_MORE_FILES
 * response is sent.
 */

struct	sio	*
pci_snext(dos_fname, stringlen, request, search_id, offset, attr)
    char        *dos_fname;      /* Search pattern */
    int         stringlen;      /* Length of filename string */
    int         request;        /* Distinguishes between SEARCH/FIND */
    int         search_id;      /* Logical search context identifier */
    long        offset;         /* Offset into directory */
    int         attr;           /* MS-DOS filename search attribute */
{
    int             mode;	    /* Mode of a search (MAPPED/UNMAPPED) */
    char            *slashptr;
    char            *pathname;      /* Pointer to search pathname string */
    char            *pattern;       /* Pointer to search pattern string */
    char	    filename[MAX_FN_TOTAL];
    char            pathcmp[MAX_FN_TOTAL];
    char            *nameptt;    /* file name/pattern for matching */
    unsigned char   save_resp;      /* To remember response */
    register struct sio *saddr;     /* Points to read-ahead output buffer */
    register DIR    *dirptr;        /* Pointer to opendir() structure */

    saddr = (struct sio *)&out1;

/* Is there an existing search context? */
    if ((dirptr = swapin_dir(search_id)) == NULL)
    {
	saddr->hdr.res = NO_MORE_FILES;
	if (offset == 65535L) {
		saddr->hdr.fdsc = (unsigned short)search_id;
		saddr->hdr.offset = 65535L;
	}
	sDbg(("pci_snext:could not swapin_dir(%d)\n", search_id));
	return saddr;
    }

/* Search-Next calls can update a search context */

    if (request == NEXT_SEARCH) {

	mode = get_mode(search_id);

	/*
	 * If text count indicates there is data, examine input text and form a
	 * pathname for search directory and filename/search pattern.
	 */
	if (stringlen) {
	    if (cvt_fname_to_unix(UNMAPPED, (unsigned char *)dos_fname,
		(unsigned char *)filename)) {
		saddr->hdr.res = PATH_NOT_FOUND;
		return saddr;
	    }
	    if ((slashptr = strrchr(filename, '/')) != NULL) {
		nameptt = slashptr + 1;
		if (slashptr == filename)
		    strcpy(pathcmp, "/");
		else {
		    *slashptr = '\0';
		    if (mode == MAPPED) {
			set_tables(U2U);
			lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE,
					0, country);
			lcs_translate_string(pathcmp, MAX_FN_TOTAL, filename);
			unmapfilename(CurDir, pathcmp);
		    } else
			strcpy(pathcmp, filename);
		    *slashptr = '/';
		}
	    } else {
		nameptt = filename;
		strcpy(pathcmp, getpname(search_id));
	    }
	    stringlen = strlen(nameptt);
	    sDbg(("pci_snext:pc=%s nc=%s\n", pathcmp, nameptt));
	}

	if (!same_context(search_id, stringlen ? nameptt : "", attr)) {
	    if (stringlen)
		add_pattern(search_id, nameptt);
	    add_offset(search_id);
	    add_attribute(search_id, attr);
	    mode = get_mode(search_id);
	    sDbg(("pci_snext:call stata\n"));

	    return stat_n_statahead(saddr, dirptr, search_id, pathcmp, nameptt,
				    mode);
	}
#ifdef FAST_FIND
    } else if (bridge_ver_flags & V_FAST_FIND) {
	seekdir(dirptr, offset);	/* Seek to the location desired */
	mode = get_mode(search_id);
	pattern = get_pattern(search_id);
	pathname = getpname(search_id);

	return fast_find_fill(saddr, dirptr, search_id, pathname, pattern,mode);
#endif	/* FAST FIND */
    }

/* The request == NEXT_FIND, or is a same context NEXT_SEARCH */


/* Transfer read-ahead frame into an output buffer and send */

    saddr = getbufaddr(search_id);
#if !defined(UDP42) && !defined(UDP41C)
    (void) memcpy((char *)&out1, (char *)saddr,
		  (int)(saddr->hdr.t_cnt + HEADER));
#else   
    (void) memcpy((char *)&out1.pre, (char *)&saddr->pre,
		  (int)(saddr->hdr.t_cnt + HEADER));
#endif  /* !UDP42 && !UDP41C */
    out1.hdr.seq = (char)brg_seqnum;
    out1.hdr.req = (char)request;
    optr = &out1;
    save_resp = saddr->hdr.res;

    outputframelength = xmtPacket(optr, &ndata, swap_how);

/* When did not have a successful find, no use continuing search */
    if (save_resp != SUCCESS) {
	/* 
	No longer using the above code (when OLD_WAY is defined):
	Now instead, the directory search context is deleted.
	The server was found to be growing very large since the del_dir's
	were not done until a process exit occured.  If the process did a 
	lot of searches in different contexts it allocated lots of contexts
	including the packet buffers for stat aheads.  Now the contexts
	are reused as soon as nothing is found in a search.  We don't need
	near as many of them.  Gregc  03-mar-86
	*/
	/*
	I believe that the above code was to fix a badly written
	tree program that continued to do searching after a NO_MORE_FILES.
	With this code removed, now instead of getting more "NO_MORE_FILES"
	on subsequent searches, the response will be "FAILURE". The previous
	way was how DOS treated the situation, but it was extremly wasteful.
	A better way for remembering that a search context is at the end,
	and returning NO_MORE_FILES, is needed. (on the DOS side ?)
	In the meantime, this deleting of search contexts should work ok for
	most programs.   Peet 04-mar-86
	*/

	del_dir(search_id);
	return NULL;
    }

/* Get search context variables */
    mode = get_mode(search_id);
    pattern = get_pattern(search_id);
    pathname = getpname(search_id);

    fill_sio_buffer(saddr, dirptr, search_id, pathname, pattern, mode);
    if (saddr->hdr.res != SUCCESS)
	del_dir(search_id);
    return NULL;
}
