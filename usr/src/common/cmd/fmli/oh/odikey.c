/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/odikey.c	1.6.3.5"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "typetab.h"
#include "var_arrays.h"
#include "moremacros.h"
#include "sizes.h"

#define ODISIZ (2*PATHSIZ)

int
odi_putkey(entry, key, value)
struct ott_entry *entry;
char *key, *value;
{
	register length;
	register char *p, *q;
	char valbuf[PATHSIZ], odibuf[ODISIZ];
	char *copy_to_key(), *skip_to_key();

	if ((length = (value?(int)strlen(value):0) + (int)strlen(key)) >= sizeof(valbuf))
		return(O_FAIL);
	if (entry->odi && ((int)strlen(entry->odi) + length >= ODISIZ))
		return(O_FAIL);

	strcpy(valbuf, key);
	strcat(valbuf, "=");
	q = value;
	for (p=valbuf+(int)strlen(valbuf); q && (*q!='\0') && (p < valbuf+PATHSIZ); p++,q++) {
		switch (*q) {
		case ';':
		case '=':
		case '\\':
			*p++ = '\\';
			/* no break */
		default:
			*p = *q;
		}
	}
	*p = '\0';

	if (entry->odi == NULL) {		/* no odi, just add it */
		entry->odi = strsave(valbuf);
		return(O_OK);
	}

	/* copy the new value onto beginning of odibuf, then copy all of
	 * the old odibuf onto the end, leaving out the original key if
	 * it exists.
	 */
	strcpy(odibuf, valbuf);

	strcpy(valbuf, key);
	strcat(valbuf, "=");
	length = strlen(valbuf);
	q = entry->odi;
	for (p = &odibuf[(int)strlen(odibuf)]; *q; ) {
		if (strncmp(q, valbuf, length) == 0)
			q = skip_to_key(q);
		else {
			*p++ = ';';
			q = copy_to_key(p, q, sizeof(odibuf) - (p-odibuf), FALSE);
			p = p + (int)strlen(p);
		}
	}
	*p = '\0';

	free(entry->odi);
	entry->odi = strsave(odibuf);
	return(O_OK);
}

char *
odi_getkey(entry, key)
struct ott_entry *entry;
char *key;
{
	register int length;
	register char *p;
	static char keybuf[PATHSIZ];

	char *copy_to_key(), *skip_to_key();

	strcpy(keybuf, key);
	strcat(keybuf, "=");
	length = strlen(keybuf);

	for (p = entry->odi; p && *p; p = skip_to_key(p)) {
		if (strncmp(keybuf, p, length) == 0) {
			copy_to_key(keybuf, p+length, sizeof(keybuf), TRUE);
			break;
		}
	}
	if (p && *p)
		return(keybuf);
	else
		return(NULL);
}

/* copy from src to dst one keyword's value, of maximum size sizedst.
 * If unquote is TRUE, then the copy should also remove a level of backslashes.
 */

static char *
copy_to_key(dst, src, sizedst, unquote)
char *dst, *src;
int sizedst;
bool unquote;
{
	register char *p = dst;
	register bool done = FALSE;

	while (!done && src && *src && dst-p < sizedst-1 ) {
		switch (*src) {
		case ';':
			done = TRUE;
			break;
		case '\\':
			if (src[1]) {
				if (unquote == FALSE)
					*dst++ = *src;
				src++;
			}
			/* no break! continue with next case */
		default:
			*dst++ = *src++;
			break;
		}
	}
	*dst = '\0';
	return(done?++src:src);		/* skip the ";" */
}

static char *
skip_to_key(src)
char *src;
{
	char dst[PATHSIZ];

	return(copy_to_key(dst, src, sizeof(dst), TRUE));
}
