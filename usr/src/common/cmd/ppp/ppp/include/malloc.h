#ifndef _MALLOC_H
#define _MALLOC_H

#ident	"@(#)malloc.h	1.2"

char *rrstrdup();

#define malloc(a) rrmalloc(a, __LINE__, __FILE__)

#define realloc(o, a) rrrealloc(o, a, __LINE__, __FILE__)

#define free(a) rrfree(a, __LINE__, __FILE__)

#define strdup(a) rrstrdup(a, __LINE__, __FILE__)

#define memcpy(a,b,c) rrmemcpy(a,b,c, __LINE__, __FILE__)


#endif /* _MALLOC_H */
