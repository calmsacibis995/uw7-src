#ident	"@(#)sco:ar.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */

/* Enhanced Application Compatibility Support */
#ifndef _AR_H
#define _AR_H

#if defined( PORTAR )
/*		COMMON ARCHIVE FORMAT
*
*	ARCHIVE File Organization:
*	_______________________________________________
*	|__________ARCHIVE_MAGIC_STRING_______________|
*	|__________ARCHIVE_FILE_MEMBER_1______________|
*	|					      |
*	|	Archive File Header "ar_hdr"          |
*	|.............................................|
*	|	Member Contents			      |
*	|		1. External symbol directory  |
*	|		2. Text file		      |
*	|_____________________________________________|
*	|________ARCHIVE_FILE_MEMBER_2________________|
*	|		"ar_hdr"		      |
*	|.............................................|
*	|	Member Contents (.o or text file)     |
*	|_____________________________________________|
*	|	.		.		.     |
*	|	.		.		.     |
*	|	.		.		.     |
*	|_____________________________________________|
*	|________ARCHIVE_FILE_MEMBER_n________________|
*	|		"ar_hdr"		      |
*	|.............................................|
*	|		Member Contents 	      |
*	|_____________________________________________|
*
*/

#define ARMAG	"!<arch>\n"
#define SARMAG	8
#define ARFMAG	"`\n"

struct ar_hdr		/* archive file member header - printable ascii */
{
	char	ar_name[16];	/* file member name - `/' terminated */
	char	ar_date[12];	/* file member date - decimal */
	char	ar_uid[6];	/* file member user id - decimal */
	char	ar_gid[6];	/* file member group id - decimal */
	char	ar_mode[8];	/* file member mode - octal */
	char	ar_size[10];	/* file member size - decimal */
	char	ar_fmag[2];	/* ARFMAG - string to end header */
};

#elif defined( PORT5AR )

#define	ARMAG	"<ar>"
#define	SARMAG	4


struct	ar_hdr {			/* archive header */
	char	ar_magic[SARMAG];	/* magic number */
	char	ar_name[16];		/* archive name */
	char	ar_date[4];		/* date of last archive modification */
	char	ar_syms[4];		/* number of ar_sym entries */
};

struct	ar_sym {			/* archive symbol table entry */
	char	sym_name[8];		/* symbol name, recognized by ld */
	char	sym_ptr[4];		/* archive position of symbol */
};

struct	arf_hdr {			/* archive file member header */
	char	arf_name[16];		/* file member name */
	char	arf_date[4];		/* file member date */
	char	arf_uid[4];		/* file member user identification */
	char	arf_gid[4];		/* file member group identification */
	char	arf_mode[4];		/* file member mode */
	char	arf_size[4];		/* file member size */
};

#else

#ifdef M_VAX
#  define ARBASE(arp)	((arp)->ar_name)
#  define ARHSIZE	(sizeof(struct ar_hdr) - ARHPAD)
#  define ARHPAD	2
#else
#  define ARBASE(arp)	arp
#  define ARHSIZE	sizeof(struct ar_hdr)
#endif

/*#define ARMAG	0177545*/
#define ARMAG "\145\377"
#define SARMAG 2

#ifdef i386
#pragma pack(2)
#endif

struct ar_hdr
{
#ifdef M_VAX
	char	ar_pad[ARHPAD];
#endif
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	short	ar_mode;
	long	ar_size;
};

#ifdef i386
#pragma pack()
#endif

#endif
#endif	/* _AR_H */
/* End Enhanced Application Compatibility Support */
