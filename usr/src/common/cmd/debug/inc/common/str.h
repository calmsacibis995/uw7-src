#ifndef STR_H
#define STR_H
#ident	"@(#)debugger:inc/common/str.h	1.4"

#include <stddef.h>

// The s* functions manage a table of unique character strings,
// looking up the incoming strings and not making a copy if
// the string already exists.  The saved strings may be compared for
// equality by comparing the pointers.

// str() looks up the string and makes a copy if not found
// strlook() looks up the string, but does not save a copy if it
//      isn't there
// strn() is the same as str(), but it only uses the first n bytes
//      of the string

extern char *str( const char * );
extern char *strlook( const char * );
extern char *strn( const char *, int );
extern char *sf(size_t, const char *, ... );

// The make* functions just save a copy without doing the lookup.
// makestr() saves a copy of the string
// makestrn() is the same as makestr, but uses only the first len bytes
// makesf is similar to makestr( sprintf( ... )) - the first argument
// is the amount of space to allocate

extern char *makestr( const char * );
extern char *makestrn( const char *, size_t len );
extern char *makesf(size_t, const char *, ... );

#endif /* STR_H */
