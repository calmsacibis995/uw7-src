#ident	"@(#)cvtomf:sgs.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */

/* Enhanced Application Compatibility Support */
#define	SGS	""

/*	The symbol I386MAGIC is defined in filehdr.h	*/

#define MAGIC	I386MAGIC
#define TVMAGIC (MAGIC+1)

#define ISMAGIC(x)	(x ==  MAGIC)


#ifdef ARTYPE
#define	ISARCHIVE(x)	( x ==  ARTYPE)
#define BADMAGIC(x)	((((x) >> 8) < 7) && !ISMAGIC(x) && !ISARCHIVE(x))
#endif


/*
 *	When a UNIX aout header is to be built in the optional header,
 *	the following magic numbers can appear in that header:
 *
 *		AOUT1MAGIC : default
 *		PAGEMAGIC  : configured for paging
 */

#define AOUT1MAGIC 0407
#define AOUT2MAGIC 0410
#define PAGEMAGIC  0413
#define LIBMAGIC   0443



/*
 * MAXINT defines the maximum integer for printf.c
 */

#define MAXINT	2147483647

#define	SGSNAME	""
#define	RELEASE	"C Software Development System 4.1.5 5/27/88"
/* End Enhanced Application Compatibility Support */
