#pragma ident	"@(#)m1.2libs:Xm/Xmos.c	1.6.1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <stdio.h>

#ifdef __cplusplus
extern "C" { /* some 'locale.h' do not have prototypes (sun) */
#endif
#include <X11/Xlocale.h>
#ifdef __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */

#include <X11/Xos.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#include <unistd.h>
#endif

#include <sys/time.h>  /* For declaration of timeval. */

#if defined(NO_REGCOMP) && !defined(NO_REGEX)
#ifdef __sgi
extern char *regcmp();
extern int regex();
#else
#ifdef SUN_MOTIF
#include "regexpI.h"
#else
#ifdef SVR4
#include <libgen.h>
#else
#ifdef SYSV
extern char *regcmp();
extern int regex();
#endif /* sysv */
#endif /* svr4 */
#endif /* SUN_MOTIF */
#endif /* __sgi */
#endif /* NO_REGEX */

#ifndef NO_REGCOMP
#include <regex.h>
#endif /* NO_REGCOMP */

#if defined (SVR4) && !defined (ARCHIVE) /* Note: this #define has been added */
#define select _abi_select               /* for ABI compliance. This #define  */
#endif /* SVR4 */                        /* appears in XlibInt.c, XConnDis.c  */
                                         /* NextEvent.c(libXt), Xmos.c(libXm) */
                                         /* fsio.c (libfont), and Berk.c      */
                                         /* (libX11.so.1)                     */

#ifdef SYS_DIR
#include <sys/dir.h>
#else
#ifdef NDIR
#include <ndir.h>
#else
#ifdef __apollo
#include <sys/dir.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif
#endif
#endif

#include <sys/stat.h>
#ifndef MCCABE
#include <pwd.h>
#endif

#include <Xm/XmosP.h>

#ifdef USE_GETWD
#include <sys/param.h>
#define MAX_DIR_PATH_LEN    MAXPATHLEN
#define getcwd( buf, len)   ((char *) getwd( buf))
#else
#define MAX_DIR_PATH_LEN    1024
#endif
#define MAX_USER_NAME_LEN   256

#ifndef S_ISDIR
#define S_ISDIR(m) ((m & S_IFMT)==S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m) ((m & S_IFMT)==S_IFREG)
#endif

#define FILE_LIST_BLOCK 64

typedef struct {   
     unsigned char type;
     mode_t file_mode;
     char * file_name;     /* Must be last entry in structure. */
    } XmDirCacheRec, **XmDirCache ;


/********
set defaults for resources that are implementation dependant
and may be modified.
********/ 

externaldef(xmos) char _XmSDEFAULT_FONT[] = "fixed";
externaldef(xmos) char _XmSDEFAULT_BACKGROUND[] = "#729FFF";

/**************** end of vendor dependant defaults ********/

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static String GetQualifiedDir() ;
static String GetFixedMatchPattern() ;
#ifdef OSF_v1_2_4
static void FreeDirCache() ;
static void ResetCache() ;
static void AddEntryToCache() ;
#endif /* OSF_v1_2_4 */

#else

static String GetQualifiedDir( 
                        String dirSpec) ;
static String GetFixedMatchPattern( 
                        String pattern) ;
#ifdef OSF_v1_2_4
static void FreeDirCache( void ) ;
static void ResetCache( 
                        char *qDirName) ;
static void AddEntryToCache( 
                        char *entryName,
                        unsigned entryNameLen) ;
#endif /* OSF_v1_2_4 */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static char *dirCacheName ; 
static unsigned dirCacheNameLen ; 
static time_t dirCacheMtime;
static XmDirCache dirCache ;
static unsigned numCacheAlloc ;
static unsigned numCacheEntries ;


/****************************************************************/
static String
#ifdef _NO_PROTO
GetQualifiedDir( dirSpec)
            String          dirSpec ;
#else
GetQualifiedDir(
            String          dirSpec)
#endif
/*************GENERAL:
 * dirSpec is a directory name, that can contain relative 
 *   as well as logical reference. This routine resolves all these
 *   references, so that dirSpec is now suitable for open().
 * The routine allocates memory for the result, which is guaranteed to be
 *   of length >= 1.  This memory should eventually be freed using XtFree().
 ****************/
/*************UNIX:
 * Builds directory name showing descriptive path components.  The result
 *   is a directory path beginning at the root directory and terminated
 *   with a '/'.  The path will not contain ".", "..", or "~" components.  
 ****************/
{
            int             dirSpecLen ;
            struct passwd * userPW ;
            char *          userDir ;
            int             userDirLen ;
            int             userNameLen ;
            char *          outputBuf ;
            char *          destPtr ;
            char *          srcPtr ;
            char *          scanPtr ;
            char            nameBuf[MAX_USER_NAME_LEN] ;
            char            dirbuf[MAX_DIR_PATH_LEN] ;

    dirSpecLen = strlen( dirSpec) ;
    outputBuf = NULL ;

    switch(    *dirSpec    )
    {   case '~':
        {   if(    !(dirSpec[1])  ||  (dirSpec[1] == '/')    )
	    {
                userDir = _XmOSGetHomeDirName() ;
                if(    *userDir    )
                {   
    		    userDirLen = strlen( userDir) ;
    		    outputBuf = XtMalloc( userDirLen + dirSpecLen + 2) ;
                    strcpy( outputBuf, userDir) ;
    		    strcpy( &outputBuf[userDirLen], (dirSpec + 1)) ;
                    }
    	        }
	    else
	    {
		destPtr = nameBuf ;
                userNameLen = 0 ;
		srcPtr = dirSpec + 1 ;
		while(    *srcPtr  &&  (*srcPtr != '/')
                       && (++userNameLen < MAX_USER_NAME_LEN)    )
		{   *destPtr++ = *srcPtr++ ;
                    } 
		*destPtr = '\0' ;

                userPW = (struct passwd *)getpwnam( nameBuf) ;
		if(    userPW    )
		{   
		    userDirLen = strlen( userPW->pw_dir) ;
		    dirSpecLen = strlen( srcPtr) ;
		    outputBuf = XtMalloc( userDirLen + dirSpecLen + 2) ;
                    strcpy( outputBuf, userPW->pw_dir) ;
                    strcpy( &outputBuf[userDirLen], srcPtr) ;
                    } 
		}
            break ;
            } 
        case '/':
        {   outputBuf = XtMalloc( dirSpecLen + 2) ;
	    strcpy( outputBuf, dirSpec) ;
            break ;
            } 
        default:
        {  
            if(     ((destPtr = getenv( "PWD")) != NULL)
                ||  ((destPtr = getcwd( dirbuf, MAX_DIR_PATH_LEN)) != NULL)   )
            {
                userDirLen = strlen( destPtr) ;
	        outputBuf = XtMalloc( userDirLen + dirSpecLen + 3) ;
                strcpy( outputBuf, destPtr) ;
	        outputBuf[userDirLen++] = '/';
                strcpy( &outputBuf[userDirLen], dirSpec) ;
                } 
            break ;
            } 
        } 
    if(    !outputBuf    )
    {   outputBuf = XtMalloc( 2) ;
        outputBuf[0] = '/' ;
        outputBuf[1] = '\0' ;
        } 
    else
    {   userDirLen = strlen( outputBuf) ;
        if(    outputBuf[userDirLen - 1]  !=  '/'    )
        {   outputBuf[userDirLen] = '/' ;
            outputBuf[++userDirLen] = '\0' ;
            } 
        /* The string in outputBuf is assumed to begin and end with a '/'.
        */
        scanPtr = outputBuf ;
        while(    *++scanPtr    )               /* Skip past '/'. */
        {   /* scanPtr now points to non-NULL character following '/'.
            */
            if(    scanPtr[0] == '.'    )
            {   
                if(    scanPtr[1] == '/'    )
                {   /* Have "./", so just erase (overwrite with shift).
                    */
                    destPtr = scanPtr ;
                    srcPtr = &scanPtr[2] ;
                    while(    (*destPtr++ = *srcPtr++) != '\0'    )
                    {   } 
                    --scanPtr ;     /* Leave scanPtr at preceding '/'. */
                    continue ;
                    } 
                else
                {   if(    (scanPtr[1] == '.')  &&  (scanPtr[2] == '/')    )
                    {   /* Have "../", so back up one directory.
                        */
                        srcPtr = &scanPtr[2] ;
                        --scanPtr ;      /* Move scanPtr to preceding '/'.*/
                        if(    scanPtr != outputBuf    )
                        {   while(    (*--scanPtr != '/')    )
                            {   }          /* Now move to previous '/'.*/
                            } 
                        destPtr = scanPtr ;
                        while(    (*++destPtr = *++srcPtr) != '\0'    )
                        {   }               /* Overwrite "../" with shift.*/
                        continue ;
                        } 
                    } 
                } 
            else
            {   /* Check for embedded "//".  Posix allows a leading double
                *   slash (and Apollos require it).
                */
                if(    *scanPtr == '/'    )
                {   
		    if(    (scanPtr > (outputBuf + 1))
                        || (scanPtr[1] == '/')    )
                    {
                        /* Have embedded "//" (other than root specification),
			 *   so erase with shift and reset scanPtr.
			 */
			srcPtr = scanPtr ;
			--scanPtr ;
			destPtr = scanPtr ;
			while(    (*++destPtr = *++srcPtr) != '\0'    )
			    {   } 
		    }
                    continue ;
		}
	    } 
            while(    *++scanPtr != '/'    )
		{   } 
	} 
    } 
	    return( outputBuf) ;
}

/****************************************************************/
String
#ifdef _NO_PROTO
_XmOSFindPatternPart(fileSpec)
String   fileSpec ;
#else    
_XmOSFindPatternPart(String  fileSpec)
#endif
/****************GENERAL:
 * fileSpec is made of a directory part and a pattern part.
 * Returns the pointer to the first character of the pattern part
 ****************/
/****************UNIX:
 * Returns the pointer to the character following the '/' of the name segment
 *   which contains a wildcard or which is not followed by a '/'.
 ****************/
{
    char *          lookAheadPtr = fileSpec ;
    char *          maskPtr ;
    Boolean         hasWildcards ;
    char            prevChar ;
    char            prev2Char  ;

    do {   /* Stop at final name segment or if wildcards were found.*/
	maskPtr = lookAheadPtr ;
        hasWildcards = FALSE ;
        prevChar = '\0' ;
        prev2Char = '\0' ;
        while((*lookAheadPtr != '/') && !hasWildcards && *lookAheadPtr) {   
	    switch (*lookAheadPtr) {   
	    case '*': case '?': case '[': 
                if((prevChar != '\\')  ||  (prev2Char == '\\')) {   
		    hasWildcards = TRUE ;
		    break ;
		} 
	    }
            prev2Char = prevChar ;
            prevChar = *lookAheadPtr ;
	    lookAheadPtr += MB_CUR_MAX > 1 ? 
	                    abs(mblen(lookAheadPtr, MB_CUR_MAX)) : 1;
	} 
    } while (!hasWildcards  &&  *lookAheadPtr++) ;

    if(*maskPtr == '/') ++maskPtr ;

    return(maskPtr) ;
}

/****************************************************************/
void
#ifdef _NO_PROTO
_XmOSQualifyFileSpec( dirSpec, filterSpec, pQualifiedDir, pQualifiedPattern)
            String          dirSpec ;
            String          filterSpec ;
            String *        pQualifiedDir ;     /* Cannot be NULL.*/
            String *        pQualifiedPattern ; /* Cannot be NULL.*/
#else
_XmOSQualifyFileSpec(
            String          dirSpec,
            String          filterSpec,
            String *        pQualifiedDir,      /* Cannot be NULL.*/
            String *        pQualifiedPattern)  /* Cannot be NULL.*/
#endif
/************GENERAL:
 * dirSpec, filterSpec can contain relative or logical reference.
 * dirSpec cannot contain pattern characters.
 * if filterSpec does not specify all for its last segment, a pattern 
 * for 'all' is added.
 * Use GetQualifiedDir() for dirSpec.
 ****************/
/************UNIX:
 * 'all' is '*' and '/' is the delimiter.
 ****************/
{
    int             filterLen ;
    int             dirLen ;
    char *          fSpec ;
    char *          remFSpec ;
    char *          maskPtr ;
    char *          dSpec ;
    char *          dPtr ;

    if(!dirSpec) dirSpec = "" ;
    if(!filterSpec) filterSpec = "" ;
        
    filterLen = strlen(filterSpec) ;

    /* Allocate extra for NULL character and for the appended '*' (as needed).
    */
    fSpec = XtMalloc( filterLen + 2) ;
    strcpy( fSpec, filterSpec) ;

    /* If fSpec ends with a '/' or is a null string, add '*' since this is
    *   the interpretation.
    */
    if(!filterLen  ||  (fSpec[filterLen - 1] == '/')){   
	fSpec[filterLen] = '*' ;
        fSpec[filterLen + 1] = '\0' ;
    } 

    /* Some parts of fSpec may be copied to dSpec, so allocate "filterLen" 
    *   extra, plus some for added literals.
    */
    dirLen = strlen(dirSpec) ;
    dSpec = XtMalloc(filterLen + dirLen + 4) ;
    strcpy(dSpec, dirSpec) ;
    dPtr = dSpec + dirLen ;

    /* Check for cases when the specified filter overrides anything
    *   in the dirSpec.
    */
    remFSpec = fSpec ;
    switch(*fSpec) {   
    case '/':
	dSpec[0] = '/' ;
	dSpec[1] = '\0' ;
	dPtr = dSpec + 1 ;
	++remFSpec ;
	break ;
    case '~':
        dPtr = dSpec ;
	while((*dPtr = *remFSpec)  &&  (*remFSpec++ != '/')) ++dPtr ;
	*dPtr = '\0' ;
	break ;
    } 

    /* If directory spec. is not null, then make sure that it has a
    *   trailing '/', to be prepared for appending from filter spec.
    */
    if(*dSpec  &&  (*(dPtr - 1) != '/')) {   
	*dPtr++ = '/' ;
        *dPtr = '\0' ;
    } 

    maskPtr = _XmOSFindPatternPart(remFSpec) ;

    if(maskPtr != remFSpec) {  
        do {   
	    *dPtr++ = *remFSpec++ ;
	} while(remFSpec != maskPtr) ;
        *dPtr = '\0' ;
    } 

    if(remFSpec != fSpec) {   
	/* Shift remaining filter spec. to the beginning of the buffer. */
        remFSpec = fSpec ;
        while ((*remFSpec++ = *maskPtr++) != '\0') ;
    } 

    *pQualifiedDir = GetQualifiedDir( dSpec) ;
    *pQualifiedPattern = fSpec ;
    XtFree(dSpec) ;
}

/****************************************************************/
static String
#ifdef _NO_PROTO
GetFixedMatchPattern( pattern)
            String         pattern ;
#else
GetFixedMatchPattern(
            String         pattern)
#endif
/**********GENERAL:
 * The pattern parameter is converted to the format required of the
 *   the regular expression library routines.
 * Memory is allocated and returned with the result.  This memory
 *   should eventually be freed by a call to XtFree().
 ****************/
/**********UNIX:
 * '/' is used as a delimiter for the pattern.
 ****************/
{
    register char *         bufPtr ;
    char *          outputBuf ;
    char lastchar = '\0' ;
    int len ;

    outputBuf = XtCalloc( 2, strlen( pattern) + 4) ;

    bufPtr = outputBuf ;
    *bufPtr++ = '^' ;

    while(    (len = mblen( pattern, MB_CUR_MAX)) != 0    )
    {
      if(    len <= 1    )
        {
          if(    *pattern == '/'    )
            {   
              break ;
            } 
          if(    lastchar == '\\'    )
          {   
            *bufPtr++ = *pattern ;
          }
          else
          {
            switch( *pattern )
            {   
              case '.':
                *bufPtr++ = '\\' ;
                *bufPtr++ = '.' ;
                break ;
              case '?':
                *bufPtr++ = '.' ;
                break;
              case '*':
                *bufPtr++ = '.' ;
                *bufPtr++ = '*' ;
                break ;
              default:
                *bufPtr++ = *pattern ;
                break ;
            } 
          }
          lastchar = *pattern ;
          ++pattern ;
        } 
      else
        {
          strncpy( bufPtr, pattern, len) ;
          bufPtr += len ;
          pattern += len ;
          lastchar = '\0' ;
        } 
    }
    *bufPtr++ = '$' ;
    *bufPtr = '\0' ;

    return( outputBuf) ;
}

static void
#ifdef _NO_PROTO
FreeDirCache()
#else
FreeDirCache()
#endif /* _NO_PROTO */
{   
  if(    dirCacheName != NULL    )
    {   
      XtFree( dirCacheName) ;
      dirCacheName = NULL ;
      dirCacheNameLen = 0 ;

      while(    numCacheEntries    )
        {   
           XtFree( (char *) dirCache[--numCacheEntries]->file_name) ;
           XtFree( (char *) dirCache[numCacheEntries]) ;
        } 
    } 
} 

static void
#ifdef _NO_PROTO
ResetCache( qDirName )
        char *qDirName ;
#else
ResetCache(
        char *qDirName)
#endif /* _NO_PROTO */
{   
  FreeDirCache() ;

  dirCacheNameLen = strlen( qDirName) ;
  dirCacheName = strcpy(XtMalloc(dirCacheNameLen+MAX_USER_NAME_LEN+1), qDirName);
  dirCacheMtime = 0;
} 

static void
#ifdef _NO_PROTO
AddEntryToCache( entryName, entryNameLen )
        char *entryName ;
        unsigned entryNameLen ;
#else
AddEntryToCache(
        char *entryName,
        unsigned entryNameLen)
#endif /* _NO_PROTO */
{   
  struct stat statBuf ;

  if(    numCacheEntries == numCacheAlloc    )
    {
      numCacheAlloc += FILE_LIST_BLOCK ;
      dirCache = (XmDirCache) XtRealloc( (char *) dirCache,
                                    numCacheAlloc * sizeof( XmDirCacheRec *)) ;
    } 
  dirCache[numCacheEntries] = (XmDirCacheRec *) XtMalloc(
                                       sizeof( XmDirCacheRec) + entryNameLen) ;
  dirCache[numCacheEntries]->file_name =
		strcpy(XtMalloc(strlen(entryName)+1), entryName) ;


  /* Use dirCacheName character array as temporary buffer for full file name.*/
  strcpy( &dirCacheName[dirCacheNameLen], entryName) ;

  if(    !stat( dirCacheName, &statBuf)    )
    {   
      dirCache[numCacheEntries]->file_mode=statBuf.st_mode;
      if(    S_ISREG( statBuf.st_mode)    )
        {   
          dirCache[numCacheEntries]->type = XmFILE_REGULAR ;
        } 
      else
        {   
          if(    S_ISDIR( statBuf.st_mode)    )
            {   
              dirCache[numCacheEntries]->type = XmFILE_DIRECTORY ;
            } 
          else
            {   
              dirCache[numCacheEntries]->type = 0 ;
            } 
        } 
    } 
  else
    {   
      dirCache[numCacheEntries]->type = 0 ;
      dirCache[numCacheEntries]->file_mode= 0;
    } 
  ++numCacheEntries ;
  dirCacheName[dirCacheNameLen] = '\0' ; /* Restore to dir path only. */
}

/****************************************************************/
void
#ifdef _NO_PROTO
_XmOSGetDirEntries(qualifiedDir, matchPattern, fileType, matchDotsLiterally,
	      listWithFullPath, pEntries, pNumEntries, pNumAlloc)
            String          qualifiedDir ;
            String          matchPattern ;
            unsigned char   fileType ;
            Boolean         matchDotsLiterally ;
            Boolean         listWithFullPath ;
            String * *      pEntries ;      /* Cannot be NULL. */
            unsigned int *  pNumEntries ;   /* Cannot be NULL. */
            unsigned int *  pNumAlloc ;     /* Cannot be NULL. */
#else
_XmOSGetDirEntries(
            String          qualifiedDir,
            String          matchPattern,
#if NeedWidePrototypes
	      unsigned int fileType,
	      int matchDotsLiterally,
	      int listWithFullPath,
#else
	      unsigned char fileType,
	      Boolean matchDotsLiterally,
	      Boolean listWithFullPath,
#endif /* NeedWidePrototypes */
            String * *      pEntries,       /* Cannot be NULL. */
            unsigned int *  pNumEntries,    /* Cannot be NULL. */
            unsigned int *  pNumAlloc)      /* Cannot be NULL. */
#endif
/***********GENERAL:
 * This routine opens the specified directory and builds a buffer containing
 * a series of strings containing the full path of each file in the directory 
 * The memory allocated should eventually be freed using XtFree.
 * The 'qualifiedDir' parameter must be a fully qualified directory path 
 * The matchPattern parameter must be in the proper form for a regular 
 * expression parsing.
 * If the location pointed to by pEntries is NULL, this routine allocates
 *   and returns a list to *pEntries, though the list may have no entries.
 *   pEntries, pEndIndex, pNumAlloc are updated as required for memory 
 *   management.
 ****************/
/***********UNIX:
 * Fully qualified directory means begins with '/', does not have 
 * embedded "." or "..", but does not need trailing '/'.
 * Regular expression parsing is regcmp or re_comp.
 * Directory entries are also Unix dependent.
 ****************/
 
{
  char *          fixedMatchPattern ;
  XmDirCacheRec   * entryPtr = NULL;
  DIR *           dirStream = NULL ;
  struct stat     dirCacheStatBuf;
  Boolean         entryTypeOK ;
  unsigned int    dirLen = strlen( qualifiedDir) ;
  Boolean         useCache = FALSE ;
  Boolean         loadCache = FALSE ;
  unsigned        readCacheIndex = 0;
  unsigned char   dirFileType ;
#ifndef NO_REGCOMP
  regex_t         preg ;
  int             comp_status ;
  Boolean         compiled_preg = False;
#else /* NO_REGCOMP */
#ifndef NO_REGEX
#ifdef	SUN_MOTIF
  _sun_regexp*	    compiledRE = NULL;
#else
  char *          compiledRE = NULL ;
#endif
#endif
#endif /* NO_REGCOMP */

  /****************/
  
  if(    !*pEntries    )
    {   *pNumEntries = 0 ;
        *pNumAlloc = FILE_LIST_BLOCK ;
        *pEntries = (String *) XtMalloc( FILE_LIST_BLOCK * sizeof( char *)) ;
      } 
  fixedMatchPattern = GetFixedMatchPattern( matchPattern) ;
  
  if(    fixedMatchPattern    )
    {   
      if(    !*fixedMatchPattern    )
        {   
	  XtFree( fixedMatchPattern) ;
	  fixedMatchPattern = NULL ;
	} 
      else
        {   
#ifndef NO_REGCOMP
	  compiled_preg = True;
	  comp_status = regcomp( &preg, fixedMatchPattern, REG_NOSUB) ;
	  if(    comp_status    )
#else /* NO_REGCOMP */
#  ifndef NO_REGEX
#ifdef SUN_MOTIF
	    compiledRE = _sun_regcomp(fixedMatchPattern);
#else
	    compiledRE = (char *)regcmp( fixedMatchPattern, (char *)NULL);
#endif
	  if(    !compiledRE    )
#  else
          if(    re_comp( fixedMatchPattern)    )
#  endif
#endif /* NO_REGCOMP */
	      {   XtFree( fixedMatchPattern) ;
		  fixedMatchPattern = NULL ;
              } 
	}
    }
  
  stat(qualifiedDir, &dirCacheStatBuf); /* errors taken care of elsewhere*/
  
  if ((dirCacheName != NULL) &&
      (!strcmp(qualifiedDir, dirCacheName)) &&
      (fileType != XmFILE_DIRECTORY || 
       /* only when fileType is XmFILE_DIRECTORY,
        * should we care about update directory cache */
       dirCacheMtime == dirCacheStatBuf.st_mtime) )
    {   
      useCache = TRUE ;
      readCacheIndex = 0 ;
    } 
  else
    {   
      if(    !strcmp( matchPattern, "*")
	 && (fileType == XmFILE_DIRECTORY)
	 && !matchDotsLiterally    )
	{   
	  /* This test is a incestual way of knowing that we are searching
	   * a directory to fill the directory list.  We can thereby conclude
	   * that a subsequent call will be made to search the same directory
	   * to fill the file list.  Since a "stat" of every file is very
	   * performance-expensive, we will cache the directory used for
	   * a directory list search and subsequently use the results for
	   * the file list search.
	   */
	  loadCache = TRUE ;
	}
      dirStream = opendir( qualifiedDir) ;
    }

  if(    dirStream  ||  useCache    )
    {   
      unsigned loopCount = 0 ;
      
      if(    loadCache    )
	{   
	  ResetCache( qualifiedDir) ;
	  dirCacheMtime = dirCacheStatBuf.st_mtime;
	} 
      /* The POSIX specification for the "readdir" routine makes
       *  it OPTIONAL to return the "." and ".." entries in a
       *  directory.  The algorithm used here depends on these
       *  entries being included in the directory list.  So, we
       *  will first handle "." and ".." explicitly, then ignore
       *  them later if they happen to be returned by "readdir".
       */
      while(    TRUE    )
        {   
	  char *dirName ;
	  unsigned dirNameLen ;
	  
	  if(    loopCount < 2 && !useCache   )
            {   
	      if(    loopCount == 0    )
                {   
		  dirName = "." ;             /* Do current directory */
		  dirNameLen = 1 ;            /*  first time through. */
		} 
	      else
                {   
		  dirName = ".." ;            /* Do parent directory */
		  dirNameLen = 2 ;            /*  second time through. */
		} 
	      ++loopCount ;
	    } 
	  else
            {   
	      struct dirent * dirEntry ;
	      
	      do
                {   
		  if(    useCache    )
		    {   
		      if(    readCacheIndex == numCacheEntries    )
			{   
			  dirName = NULL ; 
			  break ;
			} 
		      else
			{
			  dirFileType = dirCache[readCacheIndex]->type ;
			  dirName = dirCache[readCacheIndex]->file_name ;
			  /* if the cache stores the filenames in fullpath mode
			   * we have to truncate them to not contain the path,
			   * because the readdir() returns a relative or
			   * short filename and the subsequent regex operation
			   * also expects to deal with a relative dirName
			   *
			   */
			  if (dirName[0]=='/')
				dirName = &dirName[dirLen];
			  entryPtr = dirCache[readCacheIndex++];
			  dirNameLen = strlen( dirName) ;
			}
		    }
		  else
		    {   
		      if(    (dirEntry = readdir( dirStream)) == NULL    )
			{
			  dirName = NULL ;
			  break ;
			} 
		      dirName = dirEntry->d_name ;
		      dirNameLen = strlen( dirName) ;
		    }
		  /* Check to see if directory entry is "." or "..",
		   *  since these have already been processed.
		   *  So if/when readdir returns these directories,
		   *  we just ignore them.
		   */
		} while(!useCache && 
			( ( (dirNameLen == 1) && 
			    (dirName[0] == '.') ) ||
			  ( (dirNameLen == 2) && 
			    (dirName[0] == '.') && 
			    (dirName[1] == '.') ) ) );
	      
	      if(    dirName == NULL    )
                break ;             /* Exit from outer loop. */
	    }

          /* loadCache==True, create a new item through AddEntryToCache 
	   *	and assign this lastest cache item to entryPtr
           * !loadCache && useCache, uses existing entryPtr from dirCache
           * !loadCache && !useCache, has to create an entryPtr on fly
           */
	  if(    loadCache    )
	    { 
	      AddEntryToCache( dirName, dirNameLen) ;
	    } 
	  else if ( !useCache )
	    {
	      entryPtr = (XmDirCacheRec *) XtMalloc(
                             sizeof( XmDirCacheRec) + dirNameLen) ;
	      entryPtr->file_name = XtMalloc( dirNameLen + dirLen + 1) ;
              strcpy( entryPtr->file_name, qualifiedDir) ;
              strcpy( &(entryPtr->file_name[dirLen]), dirName) ;
	      if( !stat(entryPtr->file_name, &dirCacheStatBuf) )
		entryPtr->file_mode = dirCacheStatBuf.st_mode;
	      else
		entryPtr->file_mode = 0;
	    }

	  if(    fixedMatchPattern    )
	    {   
#ifndef NO_REGCOMP
	      if(    regexec( &preg, dirName, 0, NULL, 0)    )
#else /* NO_REGCOMP */
#ifndef NO_REGEX
#ifdef SUN_MOTIF
                if(    !_sun_regexec(compiledRE, dirName)    )
#else
		  if(    !regex( compiledRE, dirName)    )
#endif
#else /* NO_REGEX */
		    if(    !re_exec( dirName)    )
#endif
#endif /* NO_REGCOMP */
		      {
			continue ;
		      } 
	    } 
	  if(    matchDotsLiterally
	     && (dirName[0] == '.')
	     && (*matchPattern != '.')    )
	    {
	      continue ;
	    } 
	  if(    *pNumEntries == *pNumAlloc    )
	    {
	      *pNumAlloc += FILE_LIST_BLOCK ;
	      *pEntries = (String *) XtRealloc((char*) *pEntries, 
					       (*pNumAlloc* sizeof( char *))) ;
	    } 
	  
	  if (loadCache && dirCache) {
	    entryPtr = dirCache[numCacheEntries - 1];
	    
	    if (entryPtr->file_name)
	      XtFree(entryPtr->file_name);
	    
	    if (listWithFullPath) {
	      entryPtr->file_name =
		strcpy(XtMalloc(dirNameLen+dirLen+1) , qualifiedDir) ;
	      strcpy( &entryPtr->file_name[dirLen], dirName) ;
	    }
	    else
	      entryPtr->file_name = strcpy(XtMalloc(dirNameLen+1) , dirName);
	  }
	  /* Now screen entry according to type.
	   */
	  entryTypeOK = FALSE ;
	  
	  if (fileType == XmFILE_ANY_TYPE)
	    {
	      entryTypeOK = TRUE ;
	    }
	  else
	    {   
	      if(    useCache    )
		{   
		  if(    dirFileType == fileType    )
		    {   
		      entryTypeOK = TRUE ;
		    } 
		} 
	      else
		{   
		  switch(    fileType    )
		    {   
		    case XmFILE_REGULAR:
		      {   
			if(    S_ISREG( entryPtr->file_mode)    )
			  {   
			    entryTypeOK = TRUE ;
			  } 
			break ;
		      } 
		    case XmFILE_DIRECTORY:
		      {   
			if(    S_ISDIR( entryPtr->file_mode)    )
			  {   
			    entryTypeOK = TRUE ;
			  } 
			break ;
		      } 
		    }
		}
	    }
	  if(    entryTypeOK    )
            {   
	      if(    listWithFullPath    )
		{   
		  (*pEntries)[(*pNumEntries)++] =
		    strcpy(XtMalloc(strlen(entryPtr->file_name)+1),
			   entryPtr->file_name);
		} 
	      else
		{
		  (*pEntries)[(*pNumEntries)++] =
		    strcpy(XtMalloc(dirNameLen+1), dirName);
		} 
            } 
	  /* To free the entryPtr space */
	  if (!useCache && !loadCache && entryPtr)
	    {
		if (entryPtr->file_name)
		    XtFree(entryPtr->file_name);
		XtFree(entryPtr);
	        entryPtr = NULL;
	    }
        } /* end of while loop */
      if(    !useCache    )
	{   
	  closedir( dirStream) ;
	} 
    }
#ifndef NO_REGCOMP
  if(    compiled_preg    )
    {   regfree( &preg) ;
      } 
#else /* NO_REGCOMP */
#  ifndef NO_REGEX
  if(    compiledRE    )
    {   /* Use free instead of XtFree since malloc is inside of regex().
	 */
      free( compiledRE) ; 
    } 
#  endif
#endif /* NO_REGCOMP */
  XtFree( fixedMatchPattern) ;
  return ;
}

/****************************************************************/
void
#ifdef _NO_PROTO
_XmOSBuildFileList(dirPath, pattern, typeMask, pEntries, pNumEntries, pNumAlloc)
            String          dirPath ;
            String          pattern ;
            unsigned char   typeMask ;
            String * *      pEntries ;      /* Cannot be NULL. */
            unsigned int *  pNumEntries ;   /* Cannot be NULL. */
            unsigned int *  pNumAlloc ;     /* Cannot be NULL. */
#else
_XmOSBuildFileList(
	      String          dirPath,
	      String          pattern,
#if NeedWidePrototypes
	      unsigned int typeMask,
#else
	      unsigned char typeMask,
#endif /* NeedWidePrototypes */
	      String * *      pEntries,       /* Cannot be NULL. */
	      unsigned int *  pNumEntries,    /* Cannot be NULL. */
	      unsigned int *  pNumAlloc)      /* Cannot be NULL. */
#endif
/************GENERAL:
 * The 'dirPath' parameter must be a qualified directory path.
 * The 'pattern' parameter must be valid as a suffix to dirPath.
 * typeMask is an Xm constant coming from Xm.h.
 ****************/
/************UNIX:
 * Qualified directory path means no match characters, with '/' at end.
 ****************/
{  
    String          qualifiedDir ;
    String          nextPatternPtr ;
    String *        localEntries ;
    unsigned int    localNumEntries ;
    unsigned int    localNumAlloc ;
    unsigned int    entryIndex ;
/****************/

    qualifiedDir = GetQualifiedDir( dirPath) ;
    nextPatternPtr = pattern ;
    while(*nextPatternPtr  &&  (*nextPatternPtr != '/')) ++nextPatternPtr ;

    if(!*nextPatternPtr) {   
	/* At lowest level directory, so simply return matching entries.*/
        _XmOSGetDirEntries( qualifiedDir, pattern, typeMask, FALSE, TRUE, 
		      pEntries, pNumEntries, pNumAlloc) ;
    } else {   
	++nextPatternPtr ;               /* Move past '/' character.*/
        localEntries = NULL ;
        _XmOSGetDirEntries( qualifiedDir, pattern, XmFILE_DIRECTORY, TRUE, TRUE, 
		      &localEntries, &localNumEntries, &localNumAlloc) ;
        entryIndex = 0 ;
        while(entryIndex < localNumEntries) {   
	    _XmOSBuildFileList( localEntries[entryIndex], nextPatternPtr, 
			  typeMask, pEntries, pNumEntries, pNumAlloc) ;
            XtFree( localEntries[entryIndex]) ;
            ++entryIndex ;
	} 
        XtFree((char*)localEntries) ;
    }
    XtFree( qualifiedDir) ;
    return ;
}


/****************************************************************/
int
#ifdef _NO_PROTO
_XmOSFileCompare(sp1, sp2)
        XmConst void *sp1 ;
        XmConst void *sp2 ;
#else
_XmOSFileCompare(
        XmConst void *sp1,
        XmConst void *sp2)
#endif
/*********GENERAL:
 * The routine must return an integer less than, equal to, or greater than
 * 0 according as the first argument is to be considered less
 * than, equal to, or greater than the second.
 ****************/
{
  String s1 = *((String *) sp1) ;
  String s2 = *((String *) sp2) ;
  int caseCompare = 0 ;

  while(    TRUE    )
    {
      unsigned char c1 = *s1++ ;
      unsigned char c2 = *s2++ ;
      
      if(    c1 == '\0'    )
        {   
          if(    c2 == '\0'    )
            {   
              /* S1 and S2 are identical (length and characters, except
              *    possibly for case), so break to return the case comparison.
              */
              break ;
            } 
          /* S1 is shorter than S2.
          */
          return -1 ;
        } 
      else
        {   
          if(    c2 == '\0'    )
            {
              /* S1 is longer than S2.
              */
              return 1 ;
            } 
          else
            {   
              /* Neither c1 nor c2 is NULL.
              */
              if(    c1 != c2    )
                {   
                  unsigned char lc1 = tolower( c1) ;
                  unsigned char lc2 = tolower( c2) ;

                  if(    lc1 == lc2    )
                    {
                      if(    !caseCompare    )
                        {
                          /* No previous differences in case, and the c1 and c2
                          *    are different only by case.
                          */
                          caseCompare = (c1 < c2) ? -1 : 1 ;
                        } 
                    } 
                  else
                    {
                      return (lc1 < lc2) ? -1 : 1 ;
                    } 
                } 
            } 
        }
    }
  return caseCompare ;
}

/************************************<+>*************************************
 *
 *   Path code, used in Mwm and Xm.
 *   Returned pointer should not be freed!
 *
 *************************************<+>************************************/
String
#ifdef _NO_PROTO
_XmOSGetHomeDirName()
#else
_XmOSGetHomeDirName()
#endif
{
    uid_t uid;
    struct passwd *pw;
    char *ptr = NULL;
    static char empty = '\0';
    static char *homeDir = NULL;

    if (homeDir == NULL) {
        if((ptr = (char *)getenv("HOME")) == NULL) {
            if(    ((ptr = (char *)getenv("LOGNAME")) != NULL)
                || ((ptr = (char *)getenv("USER")) != NULL)    )
                pw = getpwnam(ptr);
            else {
                uid = getuid();
                pw = getpwuid(uid);
            }
            if (pw)
                ptr = pw->pw_dir;
            else
                ptr = NULL;
        }
	if (ptr != NULL) {
	    homeDir = XtMalloc (strlen(ptr) + 1);
	    strcpy (homeDir, ptr);
	}
	else {
	    homeDir = &empty;
	}
    }

    return (homeDir);
}


#ifndef LIBDIR
#define LIBDIR "/usr/lib/X11"
#endif
#ifndef INCDIR
#define INCDIR "/usr/include/X11"
#endif

static char libdir[] = LIBDIR;
static char incdir[] = INCDIR;

static char XAPPLRES_DEFAULT[] = "\
%%P\
%%S:\
%s/%%L/%%T/%%N/%%P\
%%S:\
%s/%%l/%%T/%%N/%%P\
%%S:\
%s/%%T/%%N/%%P\
%%S:\
%s/%%L/%%T/%%P\
%%S:\
%s/%%l/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S:\
%s/%%P\
%%S:\
%s/%%L/%%T/%%N/%%P\
%%S:\
%s/%%l/%%T/%%N/%%P\
%%S:\
%s/%%T/%%N/%%P\
%%S:\
%s/%%L/%%T/%%P\
%%S:\
%s/%%l/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S";

static char PATH_DEFAULT[] = "\
%%P\
%%S:\
%s/%%L/%%T/%%N/%%P\
%%S:\
%s/%%l/%%T/%%N/%%P\
%%S:\
%s/%%T/%%N/%%P\
%%S:\
%s/%%L/%%T/%%P\
%%S:\
%s/%%l/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S:\
%s/%%P\
%%S:\
%s/%%L/%%T/%%N/%%P\
%%S:\
%s/%%l/%%T/%%N/%%P\
%%S:\
%s/%%T/%%N/%%P\
%%S:\
%s/%%L/%%T/%%P\
%%S:\
%s/%%l/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S:\
%s/%%T/%%P\
%%S";

static char ABSOLUTE_PATH[] = "\
%P\
%S";

String
#ifdef _NO_PROTO
_XmOSInitPath(file_name, env_pathname, user_path)
        String	file_name ;
        String	env_pathname ;
        Boolean * user_path ;
#else
_XmOSInitPath(
        String	file_name,
        String	env_pathname,
	Boolean * user_path)
#endif
{
  String path;
  String old_path;
  char *homedir;
  String local_path;

  *user_path = False ;

  if (file_name[0] == '/') {
      path = XtMalloc(strlen(ABSOLUTE_PATH) + 1);
      strcpy (path, ABSOLUTE_PATH);
  } else {
      local_path = (char *)getenv (env_pathname);
      if (local_path  == NULL) {
	  homedir = _XmOSGetHomeDirName();
	  old_path = (char *)getenv ("XAPPLRESDIR");
	  if (old_path == NULL) {
	      path = XtCalloc(1, 7*strlen(homedir) + strlen(PATH_DEFAULT) 
			         + 6*strlen(libdir) + strlen(incdir) + 1);
	      sprintf(path, PATH_DEFAULT, homedir, homedir, homedir,
		      homedir, homedir, homedir, homedir,
		      libdir, libdir, libdir, libdir, libdir, libdir, incdir);
	  } else {
	      path = XtCalloc(1, 6*strlen(old_path) + 2*strlen(homedir) 
			      + strlen(XAPPLRES_DEFAULT) + 6*strlen (libdir)
			      +	strlen(incdir) + 1);
	      sprintf(path, XAPPLRES_DEFAULT, 
		      old_path, old_path, old_path, old_path, old_path, 
		      old_path, homedir, homedir,
		      libdir, libdir, libdir, libdir, libdir, libdir, incdir);
	  }
      } else {
	  path = XtMalloc(strlen(local_path) + 1);
	  strcpy (path, local_path);
	  *user_path = True ;
      }
  }
  return (path);
}

void
#ifdef _NO_PROTO
_XmSleep( secs )
        unsigned int secs ;
#else
_XmSleep(
        unsigned int secs)
#endif
{   sleep( secs) ;
    }


int
#ifdef _NO_PROTO
_XmMicroSleep( usecs )
        long    usecs ;
#else
_XmMicroSleep(
        long    usecs)
#endif
{
    struct timeval      timeoutVal;

    timeoutVal.tv_sec = 0;
    timeoutVal.tv_usec = usecs;

#ifdef OSF_v1_2_4
    /* There is no reliable way to include a prototype for select()? */
#endif /* OSF_v1_2_4 */
    return (select(0, NULL, NULL, NULL, &timeoutVal));
}

/************************************************************************
 *                                                                    *
 *    _XmOSSetLocale   wrapper so vendor can disable call to set       *
 *                    if locale is superset of "C".                   *
 *                                                                    *
 ************************************************************************/

String
#ifdef _NO_PROTO
_XmOSSetLocale(locale)
     String locale;
#else
_XmOSSetLocale(String locale)
#endif
{
  return(setlocale(LC_ALL, locale));
}

/************************************************************************
 *                                                                    *
 *	_XmOSGetLocalizedString	Map an X11 R5 XPCS string in a locale	*
 *				sensitive XmString.			*
 *                                                                    *
 *		reserved	Reserved for future use.		*
 *		widget		The widget id.				*
 *		resource	The resource name.			*
 *		string		The input 8859-1 value.			*
 *                                                                    *
 ************************************************************************/

XmString
#ifdef _NO_PROTO
_XmOSGetLocalizedString( reserved, widget, resource, string)
        char *reserved ;
        Widget widget ;
        char *resource ;
        String string ;
#else
_XmOSGetLocalizedString(
        char *reserved,
        Widget widget,
        char *resource,
        String string)
#endif
{
  return( XmStringCreateLocalized( string)) ;
}


/************************************************************************
 *									*
 *    _XmOSBuildFileName						*
 *									*
 *	Build an absolute file name from a directory and file.		*
 *	Handle case where 'file' is already absolute.
 *	Return value should be freed by XtFree()			*
 *									*
 ************************************************************************/

String
#ifdef _NO_PROTO
_XmOSBuildFileName( path, file)
    String path;
    String file;
#else
_XmOSBuildFileName(
    String path,
    String file)
#endif
{
    String fileName;

    if (file[0] == '/') {
	fileName = XtMalloc (strlen (file) + 1);
	strcpy (fileName, file);
    }
    else {
	fileName = XtMalloc (strlen(path) + strlen (file) + 2);
	strcpy (fileName, path);
	strcat (fileName, "/");
	strcat (fileName, file);
    }

    return (fileName);
}


/************************************************************************
 *									*
 *    _XmOSPutenv							*
 *									*
 *	Provide a standard interface to putenv (BSD) and setenv (SYSV)  *
 *      functions.                                                      *
 *									*
 ************************************************************************/

int
#ifdef _NO_PROTO
_XmOSPutenv( string)
    char *string;
#else
_XmOSPutenv(
    char *string)
#endif
{
#ifndef NO_PUTENV
    return (putenv(string));

#else
    char *value;

    if ( (value = strchr(string, '=')) != NULL)
      {
	char *name  = XtNewString(string);
	int result;

	name[value-string] = '\0';

	result = setenv(name, value+1, 1);
	XtFree(name);
	return result;
      }
    else
      return -1;
#endif
}
