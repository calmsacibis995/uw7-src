#ident	"@(#)pcintf:bridge/xdir.h	1.1.1.3"
/* SCCSID(@(#)xdir.h	6.2	LCC);	/* Modified: 20:31:11 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "pci_proto.h"

#if defined(NO_DIRENT_H)
#	include <sys/fs/s5dir.h>
#	define dirent direct
#else
#	include <dirent.h>
#endif

#if defined(NO_DIR_LIB)

#ifndef	BERK42FILE
#	define	BLKSIZ		1024
#endif

#ifndef MAXPATHLEN			/* be sure we use MAXPATHLEN from */
#	define MAXPATHLEN	80	/* sys/param.h if it exists */
#endif

/*
 * Definitions for access routines operating on directories.
 */


typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;

	/* the union nonsense below is so we can guarantee null-termination
	*  on names in directories. The 4.2 versions already provide this
	*/

	union {
		struct direct udd_dir;
		char udd_dirchars[sizeof (struct direct)+1];
	} dd_udir;

#define	dd_dir	dd_udir.udd_dir
#define	dd_dirchars	dd_udir.udd_dirchars

	char	dd_buf[BLKSIZ];
} DIR;


/*			External Functions Declarations 		*/

extern DIR		*opendir	PROTO((char *));
extern struct direct	*readdir	PROTO((DIR *));
extern long		telldir		PROTO((DIR *));
extern void		seekdir		PROTO((DIR *, long));
extern void		closedir	PROTO((DIR *));

#endif	/* NO_DIR_LIB */

extern DIR		*swapin_dir	PROTO((int));
extern int		add_dir		PROTO((char *, char *, int, int, int));
extern int		del_dir		PROTO((int));
extern char		*getpname	PROTO((int));
extern char		*get_pattern	PROTO((int));
extern int		get_attr	PROTO((int));
extern int		get_mode	PROTO((int));
extern long		snapshot	PROTO((int));
extern void		add_pattern	PROTO((int, char *));
extern void		add_offset	PROTO((int));
extern void		add_attribute	PROTO((int, int));
extern int		same_context	PROTO((int, char *, int));
extern void		dump_dir	PROTO((void));

#if defined(XENIX)
extern struct output	*getbufaddr	PROTO((int));
#else
extern struct sio	*getbufaddr	PROTO((int));
#endif
