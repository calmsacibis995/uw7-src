#pragma ident	"@(#)m1.2libs:Xm/XmosP.h	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmosP_h
#define _XmosP_h

#include <Xm/XmP.h>

/*
 * Fix for 5222 - if NO_MEMMOVE is defined, some systems will still
 *                require stdlib.h.
 */
#ifdef NO_MEMMOVE
#define memmove( p1, p2, p3 )   bcopy( p2, p1, p3 )
#endif

#ifndef X_NOT_STDC_ENV
#include <stdlib.h> /* Needed for MB_CUR_MAX, mbtowc, mbstowcs and mblen */
#endif

/* On Sun systems, mblen is broken. It doesn't return 0 when the
   string is empty. Here's a patch. NOTE: On Sun systems, mblen
   is a macro wrapper around mbtowc. Hence the implementation below. */
#if defined(sun)
#undef  mblen
#define mblen(ptr, size) \
  ((ptr && *(ptr) == '\0') ? 0 : mbtowc((wchar_t *)0, (ptr), (size)))
#endif

#ifdef BOGUS_MB_MAX  /* some systems don't properly set MB_[CUR|LEN]_MAX */
#undef MB_LEN_MAX
#define MB_LEN_MAX 1 /* temp fix for ultrix */
#undef MB_CUR_MAX
#define MB_CUR_MAX 1 /* temp fix for ultrix */
#endif /* BOGUS_MB_MAX */

/**********************************************************************/
/* here we duplicate Xtos.h, since we can't include this private file */

#ifdef INCLUDE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef CRAY
#define WORD64
#endif

/* stolen from server/include/os.h */
#ifndef NO_ALLOCA
/*
 * os-dependent definition of local allocation and deallocation
 * If you want something other than XtMalloc/XtFree for ALLOCATE/DEALLOCATE
 * LOCAL then you add that in here.
 */
#if defined(__HIGHC__)

#if HCVERSION < 21003
#define ALLOCATE_LOCAL(size)	alloca(size)
pragma on(alloca);
#else /* HCVERSION >= 21003 */
#define	ALLOCATE_LOCAL(size)	_Alloca(size)
#endif /* HCVERSION < 21003 */

#define DEALLOCATE_LOCAL(ptr)  /* as nothing */

#endif /* defined(__HIGHC__) */


#ifdef __GNUC__

#ifndef alloca /* gnu itself might have done that already */
#define alloca __builtin_alloca
#endif

#define ALLOCATE_LOCAL(size) alloca(size)
#define DEALLOCATE_LOCAL(ptr)  /* as nothing */
#else /* ! __GNUC__ */
/*
 * warning: mips alloca is unsuitable, do not use.
 */
#if defined(vax) || defined(sun) || defined(apollo) || defined(stellar)
/*
 * Some System V boxes define alloca in libPW; if you
 * decide that you don't want to use alloca, you might want to fix it here.
 */
char *alloca();
#define ALLOCATE_LOCAL(size) alloca(size)
#define DEALLOCATE_LOCAL(ptr)  /* as nothing */
#endif /* who does alloca */
#endif /* __GNUC__ */

#endif /* NO_ALLOCA */

#ifndef ALLOCATE_LOCAL
#define ALLOCATE_LOCAL(size) XtMalloc(size)
#define DEALLOCATE_LOCAL(ptr) XtFree(ptr)
#endif /* ALLOCATE_LOCAL */

/* End of Xtos.h */
/*****************/


#ifdef __cplusplus
extern "C" {
#endif


#define MATCH_CHAR 'P'  /* referenced in InitPath strings and in the files 
			 that uses it (ImageCache.c and Mrmhier.c) */

/* OS-dependent file info for VirtKeys */

#define XMBINDDIR "XMBINDDIR"
#ifndef XMBINDDIR_FALLBACK
#define XMBINDDIR_FALLBACK "/usr/lib/Xm/bindings"
#endif
#define XMBINDFILE "xmbind.alias"
#define MOTIFBIND ".motifbind"

/* Vendor dependent macro for XmCvtXmStringToCT */
/* Sample implementation treats unmapped charsets as locale encoded text. */
#define _XmOSProcessUnmappedCharsetAndText(tag, ctext, sep, outc, outl, prev) \
	processCharsetAndText(XmFONTLIST_DEFAULT_TAG, ctext, sep, outc, outl, prev)

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern String _XmOSFindPatternPart() ;
extern void _XmOSQualifyFileSpec() ;
extern void _XmOSGetDirEntries() ;
extern void _XmOSBuildFileList() ;
extern int _XmOSFileCompare() ;
extern String _XmOSGetHomeDirName() ;
extern String _XmOSInitPath() ;
extern void _XmSleep() ;
extern int _XmMicroSleep() ;
extern String _XmOSSetLocale() ;
extern XmString _XmOSGetLocalizedString() ;
extern String _XmOSBuildFileName() ;
extern int _XmOSPutenv() ;
#else

extern String _XmOSFindPatternPart( 
                        String fileSpec) ;
extern void _XmOSQualifyFileSpec( 
                        String dirSpec,
                        String filterSpec,
                        String *pQualifiedDir,
                        String *pQualifiedPattern) ;
extern void _XmOSGetDirEntries( 
                        String qualifiedDir,
                        String matchPattern,
#if NeedWidePrototypes
                        unsigned int fileType,
                        int matchDotsLiterally,
                        int listWithFullPath,
#else
                        unsigned char fileType,
                        Boolean matchDotsLiterally,
                        Boolean listWithFullPath,
#endif /* NeedWidePrototypes */
                        String **pEntries,
                        unsigned int *pNumEntries,
                        unsigned int *pNumAlloc) ;
extern void _XmOSBuildFileList( 
                        String dirPath,
                        String pattern,
#if NeedWidePrototypes
                        unsigned int typeMask,
#else
                        unsigned char typeMask,
#endif /* NeedWidePrototypes */
                        String **pEntries,
                        unsigned int *pNumEntries,
                        unsigned int *pNumAlloc) ;
extern int _XmOSFileCompare( 
                        XmConst void *sp1,
                        XmConst void *sp2) ;
extern String _XmOSGetHomeDirName() ;
extern String _XmOSInitPath( 
                        String file_name,
                        String env_pathname,
                        Boolean *user_path) ;
extern void _XmSleep( 
                        unsigned int secs) ;
extern int _XmMicroSleep( 
                        long secs) ;
extern String _XmOSSetLocale( 
                        String locale) ;
extern XmString _XmOSGetLocalizedString( 
                        char *reserved,
                        Widget widget,
                        char *resource,
                        String string) ;
extern String _XmOSBuildFileName(
			String file,
			String path) ;
extern int _XmOSPutenv(
		       char *string);
#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmosP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
