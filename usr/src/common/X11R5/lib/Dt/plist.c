/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:plist.c	1.12.1.2"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Dt/DesktopI.h>

static char *DtEscapeDblQuotes(char *p);

DtPropPtr
DtFindProperty(plistp, attrs)
DtPropListPtr plistp;
DtAttrs attrs;
{
	static DtPropListPtr _plistp = NULL;
	static DtPropPtr _pp;
	static int count;

	if (plistp) {
		_plistp = plistp;
		_pp     = plistp->ptr;
		count   = plistp->count;
	}

	if (_plistp) {
		while (count--) {
			if ((attrs == 0) || (attrs & _pp->attrs))
				return(_pp++);
			_pp++;
		}
	}
	return(NULL);
}

/*
 * DtGetProperty - Returns the string value associated with the property.
 * Skips properties that are marked as BACKUP - those are properties which
 * contains the original (English) values (i.e. not translated to a specific
 * locale).  The DT_ATTR_PROP_BACKUP bit is only used with properties in file
 * classes.
 */
char *
DtGetProperty(plistp, name, attrs)
DtPropListPtr plistp;
char *name;
DtAttrs *attrs;
{
	register DtPropPtr pp = plistp->ptr;
	register int cnt = plistp->count;

	/* check self */
	for (;cnt; cnt--, pp++) {
		if (!strcmp(name, pp->name) &&
			!(pp->attrs & DT_PROP_ATTR_BACKUP))
		{
			if (attrs)
				*attrs = pp->attrs;
			return(pp->value);
		}
	}
	return(NULL);
}

static char *
cache_name(name)
char *name;
{
	register char *p;
	int l = strlen(name) + 1;

	if (!(p = (char *)DtGetData(NULL, DT_CACHE_NAME, (void *)name, l))) {
		p = strdup(name);
		DtPutData(NULL, DT_CACHE_NAME, p, l, p);
	}
	return(p);
}

/*
 * DtSetProperty - Creates or update a property.  If prop_value is NULL, delete
 * named property.  Properties which are marked as BACKUP are skipped because
 * they are merely used to store the original English value of a property
 * which is required when writing out properties to disk.  The DT_ATTR_PROP_
 * BACKUP bit is only used with properties in file classes.
 */
char *
DtSetProperty(plistp, prop_name, prop_value, attrs)
DtPropListPtr plistp;
char *prop_name;
char *prop_value;
DtAttrs attrs;
{
	register int cnt = plistp->count;
	register DtPropPtr pp = plistp->ptr;

	for (; cnt; cnt--, pp++)
		if (!strcmp(pp->name, prop_name) &&
			!(pp->attrs & DT_PROP_ATTR_BACKUP))
		{
			/* replace current value */
			if (pp->value)
				free(pp->value);

			if (prop_value) {
				pp->value = strdup(prop_value);
				/* preserve DT_PROP_ATTR_TRANSLATED attribute */
				if (pp->attrs & DT_PROP_ATTR_TRANSLATED) {
					pp->attrs = attrs;
					pp->attrs |= DT_PROP_ATTR_TRANSLATED;
				} else {
					pp->attrs = attrs;
				}
				return(pp->value);
			}
			else {
				int n;

				/* remove entry pp */
				if (n = plistp->count-(int)(pp-plistp->ptr)-1) {
					memcpy((void *)pp, (void *)(pp + 1),
						n * sizeof(DtPropRec));
				}
				plistp->count--;
				return(NULL);
			}
		}

	if (prop_value)
		return(DtAddProperty(plistp, prop_name, prop_value, attrs));
	return(NULL);
}

char *
DtAddProperty(pp, name, value, attrs)
DtPropListPtr pp;
char *name;
char *value;
DtAttrs attrs;
{
	DtPropPtr newptr;

	if (!value)
		return(NULL);

	if (newptr = (DtPropPtr)realloc(pp->ptr,
				(pp->count + 1) * sizeof(DtPropRec))) {
		pp->ptr = newptr;
		newptr  = newptr + (pp->count)++;
		newptr->name = cache_name(name);
		newptr->value= strdup(value);
		newptr->attrs= attrs;
		return(newptr->value);
	}
	else
		return(NULL);
}

/*
 * Free space associated with property list.
 */
void
DtFreePropertyList(plistp)
DtPropListPtr plistp;
{
	if (plistp->ptr) {
		register int i;
		register DtPropPtr pp;

		for (i=plistp->count, pp=plistp->ptr; i ; i--, pp++) {
			if (pp->value)
				free(pp->value);
		}
		free(plistp->ptr);
	}
	plistp->ptr = NULL;
	plistp->count = 0;
}

/*
 * Copy a property list.
 */
DtPropListPtr
DtCopyPropertyList(dst, src)
DtPropListPtr dst;
DtPropListPtr src;
{
	dst->ptr = NULL;
	dst->count = 0;

	if (src->ptr) {
		register int i;
		register DtPropPtr src_pp;
		register DtPropPtr dst_pp;
		int cnt;

		/* first, count the # of properties to be copied */
		for (i=src->count, src_pp=src->ptr, cnt=0; i ; i--, src_pp++)
			if (!(src_pp->attrs & DT_PROP_ATTR_DONTCOPY))
				cnt++;

		if ((dst_pp = (DtPropPtr)malloc(cnt*sizeof(DtPropRec))) == NULL)
			return(NULL);
		dst->ptr = dst_pp;
		dst->count = cnt;

		/* do the copy */
		for (i=src->count, src_pp=src->ptr; i ; i--, src_pp++) {
			if (!(src_pp->attrs & DT_PROP_ATTR_DONTCOPY)) {
				dst_pp->name  =  src_pp->name; /* cache id */
				dst_pp->value =  strdup(src_pp->value);
				dst_pp->attrs =  src_pp->attrs;
				dst_pp++;
			}
		} /* for */
	} /* if (src->ptr) */

	return(dst);
}

int
Dt__StrToPropertyList(buff, plistp, func)
char *buff;
DtPropListPtr plistp;
int (*func)();
{
	register char *p = buff;
	register char *q;
	char *findchars;
	char *name, *value;
	int namelen, vallen;
	DtAttrs attrs;

		int i = 0;
		char buf[1024];

	/* must have the '{' */
	if (*p != '{')
		return(-1);

	p++; /* skip '{' */
	while (*p && (*p != '}')) {
		/* reset attributes */
		attrs = 0;

		/* get name */
		name = p;
		if (!(q = strpbrk(p, "(=")))
			return(-1);

		/* save name length */
		namelen = (int)(q - name);

		if (*q == '(') {
			/* get attributes */
			char *close_paren;

			if (!(close_paren = strchr(q, ')')))
				return(-1);

			if (sscanf(q + 1, "%lu", &attrs) != 1)
				return(-1);

			p = close_paren + 1; /* skip (attrs) */
			if (*p++ != '=') /* must have '=' after (attrs) */
				return(-1);
		}
		else
			p = q + 1; /* skip '=' */

		/* get value */
		/* allow both single quotes as well as double quotes */
		if (*p == '"') {
			findchars = "\"";
			p++; /* skip '"' */
		} else
		if (*p == '\'') {
			findchars = "'";
			p++; /* skip '\'' */
		}
		else
			findchars = ";}";
		value = p;
		if (!(p = strpbrk(p, findchars)))
			return(-1);

		p = value;
		while (*p) {
			if (*p == '\'' || *p == '"' || (*p == ';' && *(p+1)=='\0')
				|| *p == '}')
				break;
			else if (*p == '\\')
				p++;
			buf[i++] = *p;
			p++;
		}
		buf[i] = '\0';
		vallen = i;
		i = 0;

		if (*findchars == '"')
			p++;
		if (namelen) {
			name = Dt__strndup(name, namelen);
			value = vallen ? strdup(buf) : strdup("");
			(*func)(plistp, name, value, attrs);
			free(value);
			free(name);
		}
		else
			return(-1);

		if (*p == ';')
			p++; /* skip it */
	}
	return(p - buff);
}

int
DtStrToPropertyList(buff, plistp)
char *buff;
DtPropListPtr plistp;
{
	plistp->count = 0;
	plistp->ptr   = NULL;
	return(Dt__StrToPropertyList(buff, plistp, DtAddProperty));
}

int
DtMergeStrToPropertyList(buff, plistp)
char *buff;
DtPropListPtr plistp;
{
	return(Dt__StrToPropertyList(buff, plistp, DtSetProperty));
}

/*
 * This function estimates the length of a string to represent the property
 * list. It should be used in conjunction with DtPropertyListToString().
 */
int
DtPropertyListSize(plistp)
DtPropListPtr plistp;
{
	if (plistp->count) {
		register DtPropPtr pp = plistp->ptr;
		register int size = 2; /* '{' and '}' */
		register int cnt = plistp->count;

		for (; cnt; cnt--, pp++) {
			size += strlen(pp->name) +
				strlen(pp->value) +
				4; /* '=' + '"' + '"' + ';' */
			if (pp->attrs)
 				/* '(' + ')' + long int */
				size += ATTR_OVERHEAD + ATTR_LEN;
		}
		if (cnt == 1)
			size--; /* no ';' if only 1 property */
		return(size);
	} else
		return(0);
}

char *
Dt__AttrToStr(attrs)
DtAttrs attrs;
{
	/*
	 * WARNING: This buffer assumes attrs is a 32 bit integer.
	 */
	static char buffer[ATTR_LEN + ATTR_OVERHEAD + 1];

	if (attrs)
		sprintf(buffer, "(%lu)", attrs);
	else
		buffer[0] = '\0';
	return(buffer);
}

/*
 * This function converts a property list to a string (usually for
 * transmission). The input buffer must be big enough to hold the entire
 * string. The return value is a pointer to the end of the used portion
 * of the buffer.
 */
char *
DtPropertyListToStr(buff, plistp)
char *buff;
DtPropListPtr plistp;
{
	register DtPropPtr pp = plistp->ptr;
	register int cnt = plistp->count;

	*buff++ = '{';
	for (; cnt; cnt--, pp++) {
		buff += sprintf(buff, "%s%s=\"%s\";",
		    		pp->name, Dt__AttrToStr(pp->attrs),
				DtEscapeDblQuotes(pp->value));
	}
	*--buff = '}'; /* overwrite the last ';' */
	*++buff = '\0'; /* put a null terminator */
	return(buff);
}

char *
DtAttrToString(attrs, buffer)
DtAttrs attrs;
char *buffer;
{
	register char *p = buffer;

	*p = '\0';

	if (attrs & DT_PROP_ATTR_MENU) {
		strcpy(p, "MENU ");
		p += 5;
	}
	if (attrs & DT_PROP_ATTR_DONTCOPY) {
		strcpy(p, "DONTCOPY ");
		p +=  9;
	}
	if (attrs & DT_PROP_ATTR_INSTANCE) {
		strcpy(p, "INSTANCE ");
		p += 9;
	}

	if (attrs & DT_PROP_ATTR_LOCKED) {
		strcpy(p, "LOCKED   ");
		p += 9;
	}

	if (attrs & DT_PROP_ATTR_DONTCHANGE) {
		strcpy(p, "DONTCHG  ");
		p += 9;
	}

	/* mask system reserved bits */
	attrs &= ~DT_PROP_ATTR_SYS;

	if (attrs)
		sprintf(p, "(%lu) ", attrs);

	return(buffer);
}

#ifdef DEBUG
void
DtPrintPropList(plistp)
DtPropListPtr plistp;
{
	register int i;
	register DtPropPtr pp = plistp->ptr;

	for (i=0, pp=plistp->ptr; i < plistp->count; i++, pp++) {
		putchar('\t');
		if (pp->attrs & DT_PROP_ATTR_MENU)
			printf("MENU ");
		if (pp->attrs & DT_PROP_ATTR_DONTCOPY)
			printf("DONTCOPY ");
		if (pp->attrs & DT_PROP_ATTR_LOCKED)
			printf("LOCKED   ");
		if (pp->attrs & DT_PROP_ATTR_DONTCHANGE)
			printf("DONTCHG  ");
		printf("%s=%s\n", pp->name, pp->value);
	}
}
#endif

static char *
DtEscapeDblQuotes(p)
char *p;
{
	int i = 0;
	char c;
	char buf[1024];

	while (c = *p++) {
		if (c == '"' && (i && buf[i-1] != '\\'))
			buf[i++] = '\\';
		buf[i++] = c;
	}
	buf[i] = '\0';
	return(strdup(buf));

} /* end of DtEscapeDblQuotes */
