#ifndef NOIDENT
#ident	"@(#)olmisc:buffutil.h	1.9"
#endif

/*
 * buffutil.h
 *
 */

#ifndef _buffutil_h
#define _buffutil_h

#include <stdio.h>
#ifdef I18N
#ifndef MEMUTIL
#include <stdlib.h>             /* for wchar_t data type */
#else	

	/* The following 4 lines are questionable.
	 * I didn't change them because they are
	 * protected by MEMUTIL
	 */
#ifndef _WCHAR_T
#define _WCHAR_T
typedef long wchar_t;
#endif /* _WCHAR_T */
#endif /* MEMUTIL */

#ifndef sun	/* or other porting that doesn't care I18N */
#include <widec.h>
#endif

#endif /* I18N */

	/* shouldn't check for SEEK_SET because it defined in unistd.h  */
	/* in SVR3.2 and X11/Xos.h will include this header file if USG */
#ifndef __STDC__
#ifndef __cplusplus
#ifndef c_plusplus
#define memmove(dest, src, n)  bcopy((char*)src, (char*)dest, (int)n)
#endif
#endif
#endif

#ifdef I18N
typedef wchar_t BufferElement;
#else
typedef char BufferElement;
#endif


#define Bufferof(type) \
   struct \
      { \
      int    size; \
      int    used; \
      int    esize; \
      type * p; \
      }

typedef struct _Buffer
   {
   int    size;
   int    used;
   int    esize;
   BufferElement * p;
   } Buffer;

#define LNMIN       8
#define LNINCRE    24

#define BufferFilled(b)  (b-> size == b-> used)
#define BufferLeft(b)    (b-> size - b-> used)
#define BufferEmpty(b)   (b-> used == 0)

	/* Can't use OL_ARGS marco here because this header may
	 * be included before the inclusion of OpenLook.h.
	 */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern Buffer * AllocateBuffer(int element_size, int initial_size);
extern void     GrowBuffer(Buffer *, int increment);
extern Buffer * CopyBuffer(Buffer *);
extern void     FreeBuffer(Buffer *);
extern int      InsertIntoBuffer(Buffer * target, Buffer * source, int offset);
extern int      ReadFileIntoBuffer(FILE * fp, Buffer * buffer);
extern int      ReadStringIntoBuffer(Buffer * sp, Buffer * buffer);
extern Buffer * stropen(char *);
extern Buffer * wcstropen(BufferElement * string);
extern int      strgetc(Buffer *);
extern void     strclose(Buffer *);

#ifdef I18N
extern int _mbstrlen(char * mbstring);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#else /* defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) */

extern Buffer * AllocateBuffer();
extern void     GrowBuffer();
extern Buffer * CopyBuffer();
extern void     FreeBuffer();
extern int      InsertIntoBuffer();
extern int      ReadFileIntoBuffer();
extern int	ReadStringIntoBuffer();
extern Buffer * stropen();
extern Buffer * wcstropen();
extern int      strgetc();
extern void     strclose();

#ifdef I18N
extern int _mbstrlen();
#endif
#endif /* defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) */

#endif
