#ifndef NOIDENT
#ident	"@(#)memutil:memutil.h	1.13"
#endif

/*
 * memutil.h
 *
 */

#ifndef _memutil_h
#define _memutil_h

#ifdef MEMUTIL
#include <stdlib.h>
#endif /* MEMUTIL */

#include <string.h>

#ifdef MEMUTIL
#   define REPORT_NULL_FREE 1
#   define DONT_REPORT_NULL_FREE 0

#   define REGISTER_MALLOC(p) _MURegisterMalloc((p), __FILE__, __LINE__)
#   define FREE(p)            _MUFree((p), __FILE__, __LINE__,REPORT_NULL_FREE)
#   define CALLOC(n,s)        _MUCalloc((n), (s), __FILE__, __LINE__)
#   define MALLOC(p)          _MUMalloc((p), __FILE__, __LINE__)
#   define REALLOC(p,s)       _MURealloc((p), (s), __FILE__, __LINE__)
#   define STRDUP(p)          _MUStrdup((p), __FILE__, __LINE__)

#   define free(p)         FREE((p))
#   define calloc(n,s)     CALLOC((n),(s))
#   define malloc(p)       MALLOC((p))
#   define realloc(p,s)    REALLOC((p),(s))
#   define strdup(p)       STRDUP((p))

#   define Xfree(p)        FREE((p))
#   define Xcalloc(n,s)    CALLOC((n),(s))
#   define Xmalloc(p)      MALLOC((p))
#   define Xrealloc(p,s)   REALLOC((p),(s))

#   define XtFree(p)       _MUFree((p), __FILE__, __LINE__, DONT_REPORT_NULL_FREE)
#   define XtCalloc(n,s)   CALLOC((n),(s))
#   define XtMalloc(p)     MALLOC((p))
#   define XtRealloc(p,s)  REALLOC((p),(s))

extern void      _MUFree();
extern void *    _MUCalloc();
extern void *    _MUMalloc();
extern void *    _MURealloc();
extern char *	 _MUStrdup();
extern void	 InitializeMemutil();

#else /* MEMUTIL is not defined */

#   define REGISTER_MALLOC(p) (p)
#   define FREE(p)         free((p))
#   define CALLOC(n,s)     calloc((n), (s))
#   define MALLOC(p)       malloc((p))
#   define REALLOC(p,s)    realloc((p), (s))
#   define STRDUP(p)       strdup((p) ? (p) : "")

#endif /* end of ifdef MEMUTIL */

#endif /* end of ifndef _memutil_h */
