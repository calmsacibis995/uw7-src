#ident	"@(#)debugger:libdbgen/common/str.C	1.8"

// str() looks up the given string, and returns a pointer to the
// (unique) saved copy; if not found, it saves (with new) a copy
// of the string and returns it.  sf() is an sprintf() plus a str()
// of the result.  strn() looks up only the first "n" chars of its
// argument, and returns a pointer to a (unique) copy of them.
// strlook() finds the given string in the table of saved
// strings and returns its address, but does not save it if it is
// not there already.
//
// Strings saved with str() must be considered read only!  They
// may be compared for equality by comparing their pointers.

// makestr() returns allocates new space for a string and copies
// the string into that space;  makesf() does the same with an sprintf
// inteface.  The difference between these and the s* functions
// is that they do not use the hash table.  They therefore do
// not have the lookup overhead, but also do not guarentee unique
// strings.

#include "str.h"
#include "UIutil.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define SF_HASH 823
struct SF_CELL {
	SF_CELL *link;
	char     buf[1];		// should be [0] - cfront bug
};

static SF_CELL *Table[SF_HASH];

char *
str(const char *x)
{
#if DEBUG
	static int Calls, Worst, Strings, Bytes, Strlen;
	unsigned int i = 0;
#endif

	register const char	*p;
	register struct SF_CELL *s;
	register unsigned long	len, h;

#if DEBUG
	if( !x ){
		static char report[128];
		sprintf( report, "strings=%d calls=%d worst=%d bytes=%d strlen=%d",
				 Strings,   Calls,   Worst,    Bytes, Strlen );
		return report;
	}
	++Calls;	
#endif
	
	h = 0;
	for( len = 0, p = x; *p; )
		h += (*p++) << (++len%4);
	h %= SF_HASH;

	for(s=Table[h]; s; s=s->link)
	{
#if DEBUG
		i++;
#endif
		if(!strcmp(x,s->buf))
			return s->buf;			// found it
	}

#if DEBUG
	++Strings;
	if (i > Worst ) Worst = i;
#endif

	// alocate enough space for the string itself and the pointer
	// to the next entry, which together make up an SF_CELL.
	// make sure that it is rounded up to 4 bytes for structure
	// alignment
#if DEBUG
	Strlen += len;
#endif
	len = (len+4+sizeof(SF_CELL*)) & ~03;
	s = (SF_CELL*) new char [len];

#if DEBUG
	Bytes += len;
#endif

	s->link = Table[h];			// link in at head
	Table[h] = s;
	strcpy( s->buf, x );			// copy bytes

	return s->buf;
}

char *
strn(const char *s, int n)
{
	char *buf = new char[ n + 1 ];
	strncpy( buf, s, n );
	buf[n] = '\0';

	char *result = str( buf );
	delete buf;
	return result;
}

char *
strlook(const char *x)
{
	register const char *p;
	unsigned long len, h;
	register struct SF_CELL *s;

	h = 0;
	for( len = 0, p = x; *p; )
		h += (*p++) << (++len%4);
	h %= SF_HASH;

	for( s=Table[h]; s; s = s->link )
		if (!strcmp(x, s->buf))
			return s->buf;			// found it
	return 0;
}

char *
makestr(const char *x)
{
	char *p = new char[strlen(x) + 1];
	strcpy(p, x);
	return p;
}

char *
makestrn(const char *x, size_t len)(
{
	char *p = new char[len+1];
	strncpy(p, x, len);
	p[len] = 0;
	return p;
}

char *
makesf(size_t len, const char *f ... )	
{
	va_list ap;
	char *x = new char[len+1];

	va_start(ap, f);
	if (vsprintf(x, f, ap) > len)
	{
		interface_error("makesf", __LINE__, 1);
		return 0;
	}
	va_end(ap);
	return x;
}

char *
sf(size_t len, const char *f ... )	
{
	va_list ap;
	char *x = new char[len+1];

	va_start(ap, f);
	if (vsprintf(x, f, ap) > len)
	{
		interface_error("sf", __LINE__, 1);
		return 0;
	}
	va_end(ap);

	char *result = str(x);
	delete x;
	return result;
}
