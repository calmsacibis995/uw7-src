#ident	"@(#)pcintf:bridge/pw_gecos.h	1.1.1.2"
#ifndef	PW_GECOS_H
#define	PW_GECOS_H
/* SCCSID(@(#)pw_gecos.h	6.1	LCC);	/* Modified: 10/15/90 14:53:36 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#ifdef	LOCUS
/* 
 * Struct for the sub-fields of the pw_gecos structure in struct passwd. 
 * The format of the new pw_gecos field is:
 *	UserName/FileLimit;siteInfo;siteAccessPermission
 */

typedef struct pw_gecos {
	char	*userName;		/* User Full name */
	long	fileLimit;		/* File limit */
	char	*siteInfo;		/* Site infromation */
	char	*siteAccessPerm;	/* Site Access Permission */
} pw_gecos;

/* Function declaration */
extern	struct pw_gecos *parseGecos();

#endif	/* LOCUS */
#endif	/* PW_GECOS_H */
