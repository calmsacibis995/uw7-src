#ident	"@(#)r5fonts:clients/bdftosnf/bdftosnf.h	1.2"
/*copyright	"%c%"*/

#if defined(SYSV) || defined(SVR4)
#include <memory.h>
#define bzero(b,length) memset(b,0,length)
/* these are not strictly equivalent, but suffice for uses here */
#define bcopy(b1,b2,length) memcpy(b2,b1,length)
#endif /* SYSV */

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

typedef struct _GlyphMap {
    char	*bits;
    int		h;
    int		w;
    int		widthBytes;
} GlyphMap;

/*
 * a structure to hold all the pointers to make it easy to pass them all
 * around. Much like the FONT structure in the server.
 */

typedef struct _TempFont {
    FontInfoPtr pFI;
    CharInfoPtr pCI;
    unsigned char *pGlyphs;
    FontPropPtr pFP;
    CharInfoPtr pInkCI;
    CharInfoPtr pInkMin;
    CharInfoPtr pInkMax;
} TempFont; /* not called font since collides with type in X.h */

#ifdef vax

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#else
# ifdef sun

#  if (sun386 || sun5)
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER LSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  else
#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#  endif

# else
#  ifdef apollo

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#  else
#   ifdef ibm032

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#   else
#    ifdef hpux

#	define DEFAULTGLPAD 	2		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#    else
#     ifdef pegasus

#	define DEFAULTGLPAD 	4		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst	/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#     else
#      ifdef macII

#       define DEFAULTGLPAD     4               /* default padding for glyphs */
#       define DEFAULTBITORDER  MSBFirst        /* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */

#	else
#	ifdef i386

#	define DEFAULTGLPAD	4
#	define DEFAULTBITORDER	MSBFirst
#	define DEFAULTBYTEORDER MSBFirst
#	define DEFAULTSCANUNIT	1


#	else
#       ifdef mips
#        ifdef MIPSEL

#         define DEFAULTGLPAD     4             /* default padding for glyphs */#         define DEFAULTBITORDER  LSBFirst      /* default bitmap bit order */
#         define DEFAULTBYTEORDER LSBFirst      /* default bitmap byte order */
#         define DEFAULTSCANUNIT  1             /* default bitmap scan unit */

#        else
#         define DEFAULTGLPAD     4             /* default padding for glyphs */#         define DEFAULTBITORDER  MSBFirst      /* default bitmap bit order */
#         define DEFAULTBYTEORDER MSBFirst      /* default bitmap byte order */
#         define DEFAULTSCANUNIT  1             /* default bitmap scan unit */
#        endif
#       else

#	define DEFAULTGLPAD 	1		/* default padding for glyphs */
#	define DEFAULTBITORDER MSBFirst		/* default bitmap bit order */
#	define DEFAULTBYTEORDER MSBFirst	/* default bitmap byte order */
#	define DEFAULTSCANUNIT	1		/* default bitmap scan unit */
#	  endif
#	endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#ifdef USL
#	define DEFAULTGLPAD	4
#	define DEFAULTBITORDER	MSBFirst
#	define DEFAULTBYTEORDER MSBFirst
#	define DEFAULTSCANUNIT	1
#endif

#define GLWIDTHBYTESPADDED(bits,nbytes) \
	((nbytes) == 1 ? (((bits)+7)>>3)	/* pad to 1 byte */ \
	:(nbytes) == 2 ? ((((bits)+15)>>3)&~1)	/* pad to 2 bytes */ \
	:(nbytes) == 4 ? ((((bits)+31)>>3)&~3)	/* pad to 4 bytes */ \
	:(nbytes) == 8 ? ((((bits)+63)>>3)&~7)	/* pad to 8 bytes */ \
	: 0)

