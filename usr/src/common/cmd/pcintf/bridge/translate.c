#ident	"@(#)pcintf:bridge/translate.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)translate.c	6.6	LCC);	/* Modified: 15:16:02 2/20/92 */

/*****************************************************************************

	Copyright (c) 1984, 1991 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <memory.h>
#include <ctype.h>
#include <string.h>

#include "pci_types.h"
#include "dossvr.h"
#include "table.h"
#include "log.h"

#define tDbg(x)

#ifdef	NO_MAP_UPPERCASE
int upper_case_fn_ok = 0;
#endif	/* NO_MAP_UPPERCASE */

/*
 *  cvt_to_unix(dos_string, unix_string)
 *
 *  Converts a string from the current dos format to the unix format.
 *  The strings are assumed to be MAX_FN_TOTAL long.
 */

cvt_to_unix(dos_string, unix_string)
char *dos_string;
char *unix_string;
{
	tDbg(("cvt_to_unix: %s\n", dos_string));
	set_tables(D2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE, default_char, country);
	return lcs_translate_string(unix_string, MAX_FN_TOTAL, dos_string);
}


/*
 *  cvt_to_dos(unix_string, dos_string)
 *
 *  Converts a string from the unix format to the current dos format.
 *  The strings are assumed to be MAX_FN_TOTAL long.
 */

cvt_to_dos(unix_string, dos_string)
unsigned char *unix_string;
unsigned char *dos_string;
{
	tDbg(("cvt_to_dos: %s\n", unix_string));
	set_tables(U2D);
	lcs_set_options(LCS_MODE_NO_MULTIPLE, default_char, country);
	return lcs_translate_string((char *)dos_string, MAX_FN_TOTAL, (char *)unix_string);
}


/*
 *  cvt_fname_to_unix(mode, dos_fname, unix_fname)
 *
 *  Convert a dos filename into a unix filename.
 *  mode specifies whether the filename should be unmapped or not.
 */

cvt_fname_to_unix(mode, dos_fname, unix_fname)
int mode;
unsigned char *dos_fname;
unsigned char *unix_fname;
{
	unsigned char *fp;
	int ret;

#ifdef SPACE_TRIM
	while (*dos_fname < 0x80 && isspace(*dos_fname))
		dos_fname++;
	for (fp = &dos_fname[strlen(dos_fname)];
	     fp > dos_fname && fp[-1] < 0x80 && isspace(fp[-1]);
	     fp--)
		;
	*fp = '\0';
#endif
	tDbg(("cvt_fname_to_unix: mode %d dos %s\n", mode, dos_fname));
	set_tables(D2U);
	lcs_set_options((short)(LCS_MODE_NO_MULTIPLE | LCS_MODE_STOP_XLAT |
			((mode == MAPPED) ? LCS_MODE_LOWERCASE : 0)),
			default_char, country);
	if ((ret = lcs_translate_string((char *)unix_fname, MAX_FN_TOTAL,
	    (char *)dos_fname)) < 0)
		return ret;
	for (fp = unix_fname; *fp; fp++) {
		if (*fp == '\\')
			*fp = '/';
	}
	tDbg(("cvt_fname_to_unix: mapped %s\n", unix_fname));
	if (mode == MAPPED)
		ret = (unmapfilename(CurDir, (char *)unix_fname) == FALSE);
	tDbg(("cvt_fname_to_unix: unix %s  ret %d\n", unix_fname, ret));
	return ret;
}


/*
 *  cvt_fname_to_dos(mode, path, unix_fname, dos_fname, inode)
 *
 *  Convert a unix filename into a dos filename.
 *  mode specifies whether the filename should be mapped or not.
 */

cvt_fname_to_dos(mode, path, unix_fname, dos_fname, inode)
int mode;
unsigned char *path;
unsigned char *unix_fname;
unsigned char *dos_fname;
ino_t inode;
{
	unsigned char *fp;
	int ret;
	unsigned char fname[MAX_FN_TOTAL];

#ifdef SPACE_TRIM
	while (*unix_fname < 0x80 && isspace(*unix_fname))
		unix_fname++;
#endif
	strcpy((char *)fname, (char *)unix_fname);
#ifdef SPACE_TRIM
	for (fp = &fname[strlen(fname)];
	     fp > fname && fp[-1] < 0x80 && isspace(fp[-1]);
	     fp--)
		;
	*fp = '\0';
#endif
 	tDbg(("cvt_fname_to_dos: mode %d unix %s inode %ld\n", mode, fname, (long)inode));
  	if (mode == MAPPED) {
 		if (inode)
 		    ret = mapdirent(path, fname, inode);
 		else
 		    ret = mapfilename(path, fname);
		if (ret != 0)
			return ret;
	}
	tDbg(("cvt_fname_to_dos: returned from mapfilename\n"));
	for (fp = fname; *fp; fp++) {
		if (*fp == '/')
			*fp = '\\';
	}
	ret = cvt_to_dos(fname, dos_fname);
	tDbg(("cvt_fname_to_dos: fname %s dos %s ret %d\n", fname, dos_fname, ret));
	return ret;
}


/*
 * scan_illegal() -	Returns true upon encountering an illegal character.
 */

scan_illegal(ptr, res)
register char *ptr;
char *res;
{
	int prev_period = FALSE;
	lcs_char comp[MAX_FN_TOTAL];
	lcs_char compx[MAX_FN_TOTAL];
	char temp[MAX_FN_TOTAL];
	register lcs_char *cp, *cpx;
	lcs_char *pp;
#ifdef	NO_MAP_UPPERCASE
	int have_upper;
	int have_lower;
#endif	/* NO_MAP_UPPERCASE */

	tDbg(("scan_illegal: %s\n", ptr));
#ifdef	NO_MAP_UPPERCASE
	have_upper = 0;
	have_lower = 0;
#endif	/* NO_MAP_UPPERCASE */
	/* filenames may not begin or end with a dot */
	if (*ptr == '.' || ptr[strlen(ptr)-1] == '.')
		return TRUE;

	set_tables(U2D);
	lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_STOP_XLAT, 0, country);
	lcs_errno = 0;
	lcs_convert_in(comp, MAX_FN_TOTAL, ptr, 0);
	if (lcs_errno != 0)
		return TRUE;
	for (cp = comp, cpx = compx; *cp != lcs_ascii('\0'); cp++)
		*cpx++ = lcs_toupper(*cp);
	*cpx = lcs_ascii('\0');
	lcs_convert_out(temp, MAX_FN_TOTAL, compx, 0);
	if (lcs_errno != 0)
		return TRUE;

	for (cp = comp; *cp != lcs_ascii('\0'); cp++) {
		if (*cp == lcs_ascii('.')) {
			if (prev_period ||
			    cp == comp ||
			    (cp - comp) > DOS_NAME)
				return TRUE;
			pp = cp + 1;
			prev_period = TRUE;
		} else if (
#ifndef	NO_MAP_UPPERCASE
			   lcs_isupper(*cp) ||
#else	/* NO_MAP_UPPERCASE */
			   (upper_case_fn_ok == 0 && lcs_isupper(*cp)) ||
#endif	/* NO_MAP_UPPERCASE */
			   *cp == lcs_ascii('\"') ||
			   (*cp >= lcs_ascii('*') && *cp <= lcs_ascii(',')) ||
			   *cp == lcs_ascii('/') ||
			   (*cp >= lcs_ascii(':') && *cp <= lcs_ascii('?')) ||
			   (*cp >= lcs_ascii('[') && *cp <= lcs_ascii(']')) ||
			   *cp == lcs_ascii('|') ||
			   *cp <= lcs_ascii(' ') || *cp == lcs_ascii(0x7f))
			return TRUE;
#ifdef	NO_MAP_UPPERCASE
		else if (upper_case_fn_ok == 1) {
			/* Must be all upper or all lower. */
			if (have_upper == 0)
				have_upper = lcs_isupper(*cp);
			if (have_lower == 0)
				have_lower = lcs_islower(*cp);
			if (have_upper && have_lower)
				return TRUE;
		}
#endif	/* NO_MAP_UPPERCASE */
	}
	if (cp == comp ||
            (prev_period && (cp - pp) > DOS_EXT) ||
	    (!prev_period && (cp - comp) > DOS_NAME))
		return TRUE;
	if (res == NULL)
		return FALSE;
	set_tables(U2U);
	lcs_convert_out(res, MAX_FN_TOTAL, compx, 0);
	return (lcs_errno != 0) ? TRUE : FALSE;
}


/*
 * cover_illegal() -    Changes illegal characters to underscores,
			Returns TRUE if it did so, and FALSE when
			no illegal characters were found.
			Also, replace uppercase characters with lowercase.
 */

int
cover_illegal(ptr)
register char *ptr;
{
	char *prev_ptr;
	int prev_period = FALSE;
	int changed = FALSE;

	if (*ptr == '.') {		/* filenames may not begin with dot */
		*ptr = '_';
		changed = TRUE;
	}
	for (; *ptr; ptr++) {
		if (*ptr == '.') {
			if (prev_period) {
				*prev_ptr = '_';
				changed = TRUE;
			}
			prev_ptr = ptr;
			prev_period = TRUE;
		} else if (*ptr == '\"' || *ptr == '\'' ||
			   (*ptr >= '*' && *ptr <= ',') ||
			   *ptr == '/' ||
			   (*ptr >= ':' && *ptr <= '?') ||
			   (*ptr >= '[' && *ptr <= ']') ||
			   *ptr == '|' ||
			   *ptr <= ' ' || (unsigned char)*ptr >= 0x7f) {
			*ptr = '_';
			changed = TRUE;
		} else if (islower(*ptr)) {
			*ptr = toupper(*ptr);
			changed = TRUE;
		}
	}
	if (*--ptr == '.') {	/* Check for a dot ending the filename	*/
		*ptr = '_';
		changed = TRUE;
	}
	return(changed);
}

#ifdef	NO_MAP_UPPERCASE
/*
 *  unix_uppercase(unix_string, out_string, out_size)
 *
 *  Converts a string from the current dos format to the unix format.
 */

unix_uppercase(unix_string, out_string, out_size)
char	*unix_string;
char	*out_string;
int	out_size;
{
	int	retval;

	tDbg(("unix_uppercase(%s)", unix_string));

	set_tables(U2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE | LCS_MODE_UPPERCASE,
		default_char, country);
	*out_string = '\0';
	retval = lcs_translate_string(out_string, out_size, unix_string);
	tDbg(("%s'%s'\n", (retval)?"error":"", out_string));
	return(retval);
}
#endif	/* NO_MAP_UPPERCASE */
