#pragma	ident	"@(#)Dt:msgutil.c	1.10"

#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include "DesktopI.h"

#define INITIAL_BUFFER_SIZE     1024
#define INC_BUFFER_SIZE         1024


/* extra characters in each string field: '=' and '\0' */
#define EXTRA_CHARS             2


#define MAX(A, B)       (((A) > (B)) ? (A) : (B))
#define EATSPACE(P)             while (*(P) == ' ') (P)++
#define FINDCHAR(P,C)   while (*P && (*P != (C))) P++

static char	*buffer = NULL;
static int	buffer_size = 0;

#ifndef MEMUTIL
extern char	*malloc();
extern char	*realloc();
#endif /* MEMUTIL */

static void	
InitBuffer()
{
	buffer_size = INITIAL_BUFFER_SIZE;
	buffer = malloc(buffer_size);
	*buffer = '\0';
}

static void	
expand_buffer(hint)
int	hint;       /* minimum amount of extra space wanted */
{
	char	*p;


	buffer_size += MAX(INC_BUFFER_SIZE, hint);
	if (p = realloc(buffer, buffer_size))
		buffer = p;
	else {
		buffer_size -= MAX(INC_BUFFER_SIZE, hint);
	}
}

static int	
data_size(base, map)
char *base;
DtStrToStructMapping const *map;
{
	char	*format = NULL;
	char	*dst = base + map->offset;
	int	len = strlen(map->name) + EXTRA_CHARS;

	switch (map->type) {
	case DT_MTYPE_SHORT:
		format = "%hd";
		/* FALLS THROUGH */
	case DT_MTYPE_LONG:
		if (!format)
			format = "%ld";
		/* FALLS THROUGH */
	case DT_MTYPE_USHORT:
		if (!format)
			format = "%hu";
		/* FALLS THROUGH */
	case DT_MTYPE_ULONG:
		if (!format)
			format = "%lu";
	case DT_MTYPE_FLOAT:
		if (!format)
			format = "%g";
		/* FALLS THROUGH */
	case DT_MTYPE_DOUBLE:
		if (!format)
			format = "%lg";
		 {
			char buff[32];

			return(len + sprintf(buff, format, dst));
		}
	case DT_MTYPE_STRING:
		 {
			char	*p;


			p = *(char **)dst;
			if (p) {
				/* reserve 2 chars for double quotes */
				return(len + strlen(p) + 2);
			}
			return(0);
		}
	case DT_MTYPE_BOOLEAN:
	case DT_MTYPE_CHAR:
		return(len + 1);
	case DT_MTYPE_PLIST:
		return(DtPropertyListSize((DtPropListPtr)dst));
	}
}

static char *
data_to_string(base, map, buff, len)
char *base;
DtStrToStructMapping *map;
char *buff;
int *len;
{
	register int size;
	char *dst = base + map->offset;
	char *save = buff;

	size = sprintf(buff, "%s=", map->name);
	buff += size;
	
	switch (map->type) {
	case DT_MTYPE_SHORT:
		size = sprintf(buff, "%hd", *(short *)dst);
		break;
	case DT_MTYPE_LONG:
		size = sprintf(buff, "%ld", *(long *)dst);
		break;
	case DT_MTYPE_USHORT:
		size = sprintf(buff, "%hu", *(short *)dst);
		break;
	case DT_MTYPE_ULONG:
		size = sprintf(buff, "%lu", *(long *)dst);
		break;
	case DT_MTYPE_FLOAT:
		size = sprintf(buff, "%g", *(float *)dst);
		break;
	case DT_MTYPE_DOUBLE:
		size = sprintf(buff, "%lg", *(double *)dst);
		break;
	case DT_MTYPE_STRING:
		/* escape embedded double qoutes here to s.t. they will be
		 * interpreted correctly in string_to_data().
		 */
	{
		int i = 0;
		char c;
		char buf[1024];
		char *s = *(char **)dst;

		while (c = *s++) {
			if (c == '"')
				buf[i++] = '\\';
			buf[i++] = c;
		}
		buf[i] = '\0';
		size = sprintf(buff, "\"%s\"", buf);
	}
		break;
	case DT_MTYPE_BOOLEAN:
		*buff = (*(Bool * )dst == True) ? 'T' : 'F';
		buff[1] = '\0';
		size = 1;
		break;
	case DT_MTYPE_PLIST:
		{
			char *save_buff = buff;

			buff = DtPropertyListToStr(buff, (DtPropListPtr)dst);
			size += buff - save_buff;
			*len += size;
			return(buff);
		}
	case DT_MTYPE_CHAR:
		*buff = *dst;
		size = 1;
		break;
	}

	*len -= buff - save;
	return(buff + size);
}

static char *
Dt__EncodeToString(base, prefix, info, user_buffer, user_buffer_size)
char	*base;
char	*prefix;
DtMsgInfo const *info;
char	*user_buffer;
int	*user_buffer_size;
{
	register char	*p;
	DtStrToStructMapping const *map;
	char	*buff;
	int	len;                        /* unused buffer len */
	int	i;
	int	l;
	int	first;

	if (user_buffer) {
		buff = user_buffer;
		len = *user_buffer_size;
	} else {
		if (buffer == NULL)
			InitBuffer();
		buff = buffer;
		len = buffer_size;
	}

	/* add prefix to buffer first */
	if (prefix) {
		int prefix_len = strlen(prefix);

		if (len < prefix_len) {
			if (user_buffer)
				return(NULL);
			expand_buffer(prefix_len + 2);
			len = buffer_size;
			if (len < prefix_len)
				return(NULL); /* try again later */
			buff = buffer;
		}

		strcpy(buff, prefix);
		len -= prefix_len;
		p = buff + prefix_len;
	}
	else
		p = buff;

	for (map=info->mapping, i=info->count, first=1; i; i--, map++) {
		l = data_size(base, map);
		if (len < (l + 1)) { /* 1 is for field separator ' ' */
			/* not enough space */
			if (user_buffer)
				return(NULL);
			else {
				/* expand static buffer */
				char	*save = buffer;
				int	savelen = buffer_size;

				expand_buffer(l - len + 1);
				len = buffer_size + (len - savelen);
				if (len < l)
					/* can't get space */
					return(NULL);
				buff = buffer;
				p = buffer + (p - save);/* ptr may have moved */
			}
		}

		if (l) {
			if (first)
				first = 0;
			else {
				*p++ = ' ';
				len--;
			}
			p = data_to_string(base, map, p, &len);
		}
	}

	*p = '\0';
	if (user_buffer_size)
		*user_buffer_size = p - buffer + 1; /* including NULL char */
	return(buff);
}


static char *
string_to_data(base, map, buff)
char *base;
DtStrToStructMapping const *map;
char *buff;
{
	register char *p = buff;
	register int ret;
	char *dst = base + map->offset;

	FINDCHAR(p, ' '); /* find the end of field */

	switch (map->type) {
	case DT_MTYPE_SHORT:
		 {
			short val;

			if ((ret = sscanf(buff, "%hd", &val)) == 1) {
				*(short *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_LONG:
		 {
			long	val;

			if ((ret = sscanf(buff, "%ld", &val)) == 1) {
				*(long *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_USHORT:
		 {
			unsigned short	val;

			if ((ret = sscanf(buff, "%hu", &val)) == 1) {
				*(unsigned short *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_ULONG:
		 {
			unsigned long	val;

			if ((ret = sscanf(buff, "%lu", &val)) == 1) {
				*(unsigned long *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_FLOAT:
		 {
			float	val;

			if ((ret = sscanf(buff, "%g", &val)) == 1) {
				*(float *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_DOUBLE:
		 {
			double	val;

			if ((ret = sscanf(buff, "%lg", &val)) == 1) {
				*(double *)dst = val;
				return(p);
			}
			return(NULL);
		}
	case DT_MTYPE_STRING:
		p = buff;
		if ((*p == '"') || (*p == '\'')) {
			int i = 0;
			char buf[1024];
			char c = *p;

			buff++;
			p++;
			while (*p) {
				if (*p == c)
					break;
				else if (*p == '\\')
					p++;
				buf[i++] = *p;
				p++;
			}
			buf[i] = '\0';
			*(char **)dst = strdup(buf);
		}
		else {
			FINDCHAR(p, ' ');
			*(char **)dst = Dt__strndup(buff, p - buff);
		}
		if (*p == '"')
			p++;
		return(p);
	case DT_MTYPE_BOOLEAN:
		*(Boolean * )dst = ((*buff == 'T') || (*buff == 't')) ? 
		    True : False;
		return(++buff);
	case DT_MTYPE_PLIST:
		if ((ret=DtStrToPropertyList(buff,(DtPropListPtr)dst)) == -1) {
			DtFreePropertyList((DtPropListPtr)dst);
			return(NULL);
		}
		return(buff + ret);
	case DT_MTYPE_CHAR:
		*(char *)dst = *buff;
		return(++buff);
	}
} /* string_to_data() */


/*
 * This function assumes fields in the string are in the same order as
 * the info list. Thus the info list will be scanned only once. When
 * the end of the info list is reached, decoding will stop and a pointer
 * to the remaining string is returned.
 */
int
Dt__DecodeFromString(base, info, str, endptr)
char	*base;
DtMsgInfo const *info;
char	*str;
char	**endptr;      /* ptr to the first unscanned character */
{
	register char	*p;
	register char	*q;
	register int	i;
	DtStrToStructMapping const *map;
	char	*save;
	int	n_matches = 0;

	for (p = str, map = info->mapping, i = info->count; i; i--, map++) {
		EATSPACE(p); /* skip leading spaces */
		save = p;
		q = (char *)(map->name);
		for (; *p && *q && (*p == *q); p++, q++);
		if ((*q == '\0') && (*p == '=')) {
			/* found it */
			n_matches++;
			p++; /* skip '=' */
			if ((p = string_to_data(base, map, p)) == NULL)
				/* parsing error */
				return(-1);
		} else
			p = save;
	}
	EATSPACE(p); /* skip remaining space */


	if (endptr)
		*endptr = ++p;
	return(n_matches);
}

char *
Dt__StructToString(request, len, mp, mlen)
DtRequest *request;
int *len;
DtMsgInfo const *mp;
int mlen;
{
	register int i;
	char prefix[36];

	for (i=mlen; i; i--, mp++)
		if (mp->type == request->header.rqtype)
			break;
	if (i == 0)
		return(NULL);

	/* request name has a limit of 32 characters */
	sprintf(prefix, "@%.32s: ", mp->name);
	return(Dt__EncodeToString((char *)request, prefix, mp, NULL, len));
}

void
Dt__EnqueueCharProperty(dpy, w, atom, data, len)
Display *dpy;
Window w;
Atom atom;
char *data;
int len;
{
	XChangeProperty(dpy, w, atom, XA_STRING, 8, PropModeReplace,
			(unsigned char *) data, len);

	XFlush(dpy);
} /* end of Dt__EnqueueCharProperty */

/*
 * Dt__GetCharProperty (generic routine to get a char type property)
 */

char *
Dt__GetCharProperty(dpy, w, property, length)
Display *dpy;
Window w;
Atom property;
unsigned long *length;
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		bytes_remaining;
	unsigned char *         buffer = NULL;

	(void)XGetWindowProperty(dpy, w, property, 0L, 1000000,
		True, XA_STRING, &actual_type, &actual_format, length,
		&bytes_remaining, &buffer);

	return((char *)buffer);
} /* end of Dt__GetCharProperty */

char *
Dt__strndup(str, len)
char *str;
int len;
{
	char *p;

	if (p = malloc(len + 1)) {
		memcpy(p, str, len);
		p[len] = '\0';
	}

	return(p);
}

