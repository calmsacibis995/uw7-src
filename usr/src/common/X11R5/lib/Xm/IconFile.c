#pragma ident	"@(#)m1.2libs:Xm/IconFile.c	1.4"
/************************************************************************* 
 **  (c) Copyright 1993, 1994 Hewlett-Packard Company
 **  (c) Copyright 1993, 1994 International Business Machines Corp.
 **  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 **  (c) Copyright 1993, 1994 Novell, Inc.
 *************************************************************************/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$"
#endif /* lint */
#endif /* REV_INFO */
/******************************************************************************
*******************************************************************************
*
*  (c) Copyright 1989, 1990, 1991 OPEN SOFTWARE FOUNDATION, INC.
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS.
*  (c) Copyright 1987, 1988, 1989, 1990, 1991 HEWLETT-PACKARD COMPANY
*  ALL RIGHTS RESERVED
*  
*  	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED
*  AND COPIED ONLY IN ACCORDANCE WITH THE TERMS OF SUCH LICENSE AND
*  WITH THE INCLUSION OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE OR
*  ANY OTHER COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE
*  AVAILABLE TO ANY OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF THE
*  SOFTWARE IS HEREBY TRANSFERRED.
*  
*  	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
*  NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY OPEN SOFTWARE
*  FOUNDATION, INC. OR ITS THIRD PARTY SUPPLIERS  
*  
*  	OPEN SOFTWARE FOUNDATION, INC. AND ITS THIRD PARTY SUPPLIERS,
*  ASSUME NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE ANY OF ITS
*  SOFTWARE .   OSF SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
*  KIND, AND OSF EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES, INCLUDING
*  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
*  FITNESS FOR A PARTICULAR PURPOSE.
*  
*  Notice:  Notwithstanding any other lease or license that may pertain to,
*  or accompany the delivery of, this computer software, the rights of the
*  Government regarding its use, reproduction and disclosure are as set
*  forth in Section 52.227-19 of the FARS Computer Software-Restricted
*  Rights clause.
*  
*  (c) Copyright 1989, 1990, 1991 Open Software Foundation, Inc.  Unpublished - all
*  rights reserved under the Copyright laws of the United States.
*  
*  RESTRICTED RIGHTS NOTICE:  Use, duplication, or disclosure by the
*  Government is subject to the restrictions as set forth in subparagraph
*  (c)(1)(ii) of the Rights in Technical Data and Computer Software clause
*  at DFARS 52.227-7013.
*  
*  Open Software Foundation, Inc.
*  11 Cambridge Center
*  Cambridge, MA   02142
*  (617)621-8700
*  
*  RESTRICTED RIGHTS LEGEND:  This computer software is submitted with
*  "restricted rights."  Use, duplication or disclosure is subject to the
*  restrictions as set forth in NASA FAR SUP 18-52.227-79 (April 1985)
*  "Commercial Computer Software- Restricted Rights (April 1985)."  Open
*  Software Foundation, Inc., 11 Cambridge Center, Cambridge, MA  02142.  If
*  the contract contains the Clause at 18-52.227-74 "Rights in Data General"
*  then the "Alternate III" clause applies.
*  
*  (c) Copyright 1989, 1990, 1991 Open Software Foundation, Inc.
*  ALL RIGHTS RESERVED 
*  
*  
* Open Software Foundation is a trademark of The Open Software Foundation, Inc.
* OSF is a trademark of Open Software Foundation, Inc.
* OSF/Motif is a trademark of Open Software Foundation, Inc.
* Motif is a trademark of Open Software Foundation, Inc.
* DEC is a registered trademark of Digital Equipment Corporation
* DIGITAL is a registered trademark of Digital Equipment Corporation
* X Window System is a trademark of the Massachusetts Institute of Technology
*
*******************************************************************************
******************************************************************************/

#include <stdio.h>
#include <locale.h>
#include <X11/Xos.h>
#include "IconFileP.h"
#include "_DtHashP.h"

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif

#include <unistd.h>

#include <errno.h>		/* to get EINVAL errno from getdents()*/

#include <limits.h>		/* to get _POSIX_PATH_MAX */

#include <dirent.h>

#include <sys/types.h>
#include <fcntl.h>

#include <sys/stat.h>
#ifndef MCCABE
#include <pwd.h>
#endif

#ifdef USE_GETWD
#include <sys/param.h>
#define MAX_DIR_PATH_LEN    MAXPATHLEN
#else
#define MAX_DIR_PATH_LEN    1024
#endif
#define MAX_USER_NAME_LEN   256

#ifndef __STDC__
#define const
#endif

/**************** end of vendor dependant defaults ********/

typedef union _DtCachedDirStruct *DtCachedDir;

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean AbsolutePathName() ;
static DtCachedDir MakeCachedDirEntry() ;
static int CheckDirCache() ;

#else

static Boolean AbsolutePathName( 
                        String path,
                        String *pathRtn,
                        String buf) ;
static DtCachedDir MakeCachedDirEntry( 
                        String dirName) ;
static int CheckDirCache( 
                        String path);


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/************************************<+>*************************************
 *
 *   Path code, used in Mrm, Mwm and Xm.
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
__DtOSGetHomeDirName(outptr) String outptr;
#else
__DtOSGetHomeDirName(String outptr)
#endif
/* outptr is allocated outside this function, just filled here. */
/* this solution leads to less change in the current (mwm) code */
{
    int uid;
    struct passwd *pw;
    static char *ptr = NULL;

    if (ptr == NULL) {
	if((ptr = (char *)getenv("HOME")) == NULL) {
	    if((ptr = (char *)getenv("USER")) != NULL) 
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
    }

    if (ptr) strcpy(outptr, ptr);
    else outptr[0] = '\0' ;
}

/*
 * this routine returns pointers to the beginning of the filename and
 * suffix in the path string.
 */
static void
#ifdef _NO_PROTO
_DtFindPathParts(path, filenameRtn, suffixRtn)
    String	path;
    String	*filenameRtn;
    String	*suffixRtn;
#else
_DtFindPathParts(
    String	path,
    String	*filenameRtn,
    String	*suffixRtn)
#endif
{
#define FILESEP '/'
#define SUFFIXSEP '.'

    String	filename = path, suffix = NULL;
    String	s;
/*
 * maybe a problem for I18N !!!
 */
    for (s = path; *s; ) {
	if (*s == FILESEP) {
	    filename = s++;
	}
	else if (*s == SUFFIXSEP) {
	    suffix = s++;
	}
	else
	  s++;
    }
    if (suffix < filename)
      suffix = NULL;
    if (*filenameRtn = filename)
      if (filename != path)
	(*filenameRtn)++;
    if (*suffixRtn = suffix)
      (*suffixRtn)++;
    
#ifdef notdef
    filename = strrchr(path, FILESEP);
    if (filename) {
	filename++;
	if (suffix = strchr(filename, SUFFIXSEP))
	  suffix++;
    }
    else {
	suffix = NULL;
	filename = path;
    }
#endif
}


/*
 * buf must be of length MAX_DIR_PATH_LEN
 */
static Boolean
#ifdef _NO_PROTO
AbsolutePathName(path, pathRtn, buf)
    String	path;
    String 	*pathRtn;
    String 	buf;
#else
AbsolutePathName(String path, String *pathRtn, String buf)
#endif
{
    Boolean 	doubleDot = False;
    
    *pathRtn = path;

    if (path[0] == '/')
      return True;

    if (path[0] == '.') {
	if (path[1] == '/') 
	  doubleDot = False;
	else if ((path[1] == '.') &&
		 (path[2] == '/'))
	  doubleDot = True;
	if (buf = getcwd(buf, MAX_DIR_PATH_LEN)) {
	    if (doubleDot) {
		String filePart, suffixPart;
		_DtFindPathParts(buf, &filePart, &suffixPart);
		(void) strcpy(filePart, &path[2]);
	    }
	    else {
		(void) strcat(buf, &path[1]);
	    }
	    *pathRtn = buf;
	    return True;
	}
	else {
	    XtWarning("invalid path name\n");
	    return True;
	}
    }
    return False;
}

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
/usr/lib/X11/%%L/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%l/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%L/%%T/%%P\
%%S:\
/usr/lib/X11/%%l/%%T/%%P\
%%S:\
/usr/lib/X11/%%T/%%P\
%%S:\
/usr/include/X11/%%T/%%P\
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
/usr/lib/X11/%%L/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%l/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%T/%%N/%%P\
%%S:\
/usr/lib/X11/%%L/%%T/%%P\
%%S:\
/usr/lib/X11/%%l/%%T/%%P\
%%S:\
/usr/lib/X11/%%T/%%P\
%%S:\
/usr/include/X11/%%T/%%P\
%%S";

static char ABSOLUTE_PATH[] = "\
%P\
%S";

static String
#ifdef _NO_PROTO
__DtOSInitPath(file_name, env_pathname, user_path)
        String	file_name ;
        String	env_pathname ;
        Boolean * user_path ;
#else
__DtOSInitPath(
        String	file_name,
        String	env_pathname,
	Boolean * user_path)
#endif
{
  String path;
  String old_path;
  char homedir[MAX_DIR_PATH_LEN];
  char buf[MAX_DIR_PATH_LEN];
  String local_path;

  *user_path = False ;

  if (file_name && AbsolutePathName(file_name, &file_name, homedir)) {
      path = XtMalloc(strlen(ABSOLUTE_PATH) + 1);
      strcpy (path, ABSOLUTE_PATH);
  } else {
      local_path = (char *)getenv (env_pathname);
      if (local_path  == NULL) {
	  __DtOSGetHomeDirName(homedir);
	  old_path = (char *)getenv ("XAPPLRESDIR");
	  if (old_path == NULL) {
	      path = XtCalloc(1, 7*strlen(homedir) + strlen(PATH_DEFAULT));
	      sprintf(path, PATH_DEFAULT, homedir, homedir, homedir,
		      homedir, homedir, homedir, homedir);
	  } else {
              AbsolutePathName(old_path, &old_path, buf);
	      path = XtCalloc(1, 6*strlen(old_path) + 2*strlen(homedir) 
			      + strlen(XAPPLRES_DEFAULT));
	      sprintf(path, XAPPLRES_DEFAULT, 
		      old_path, old_path, old_path, old_path, old_path, 
		      old_path, homedir, homedir);
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
_DtGenerateMaskName(imageName, maskNameBuf)
    String	imageName;
    String	maskNameBuf;
#else
_DtGenerateMaskName(
    String	imageName,
    String	maskNameBuf)
#endif
{
    String 	file, suffix;
    int		len;

    _DtFindPathParts(imageName, &file, &suffix);

    if (suffix) {
	len = (int)(suffix - imageName) - 1;
	/* point before the '.' */
	suffix--;
    }
    else
      len = strlen(imageName);

    strncpy(maskNameBuf, imageName, len);
    maskNameBuf += len;
    strcpy(maskNameBuf, "_m");
    if (suffix) 
      strcpy(maskNameBuf+2, suffix);
    else
      maskNameBuf[2] = '\0';
}

typedef struct _DtCommonCachedDirStruct{
    int			cachedDirType;
    int			dirNameLen;
    String		dirName;
}DtCommonCachedDirStruct, *DtCommonCachedDir;

typedef struct _DtValidCachedDirStruct{     
    int			cachedDirType;
    int			dirNameLen;
    String		dirName;
    int			numFiles;
    /*
     * we allocate both the offsets array and the names themselves in
     * a heap hanging off the end of this struct
     */
    short		nameOffsets[1];
    /*
    String		names
    */
}DtValidCachedDirStruct, *DtValidCachedDir;

#define DtVALID_CACHED_DIR	0
#define DtINVALID_CACHED_DIR	1
#define DtUNCACHED_DIR		2

#define MAX_CACHE_DIR_SIZE	(1L << 16)

typedef union _DtCachedDirStruct{
    DtCommonCachedDirStruct	common;
    DtValidCachedDirStruct 	valid;
}DtCachedDirStruct;

typedef struct _DtCachedDirListStruct{
    int			numDirs;
    int			maxDirs;
    DtCachedDir	*dirs;
}DtCachedDirListStruct;

#if defined(HP_MOTIF) || defined(IBM_MOTIF) || defined(NOT_SUPPORT_READDIR_R)
/* Implementation of a local version of readdir_r() if the system doesn't own
 * one. But this version of readdir_r() is not thread safe.
 */
static struct dirent *
#ifdef _NO_PROTO
readdir_r(dir, entry)
DIR *dir; 
struct dirent *entry;
#else
readdir_r(DIR *dir, struct dirent *entry)
#endif
{
	struct dirent *tmpDirect;

	tmpDirect = readdir(dir);
	if (tmpDirect)
		memcpy(entry, tmpDirect, tmpDirect->d_reclen);
	return tmpDirect;
}
#endif /* HP_MOTIF || IBM_MOTIF */

static DtCachedDir 
#ifdef _NO_PROTO
  MakeCachedDirEntry(dirName)
String	dirName;
#else
MakeCachedDirEntry(String dirName)
#endif
{
    DIR 		*dirDesc;
    struct dirent	*currDirect;
    DtCachedDir 	cachedDir;
    int			cachedDirType;
    int	max_dir_cache_len;

    max_dir_cache_len = MAX_CACHE_DIR_SIZE - ( sizeof(struct dirent) +
	(_POSIX_PATH_MAX > MAXNAMLEN ? _POSIX_PATH_MAX : MAXNAMLEN) );

    if ( (dirDesc=opendir(dirName)) == NULL )  {
	/* invalid directory */
	cachedDirType = DtINVALID_CACHED_DIR;
    }
    else {
	int	bufLen = 0;
	char	stackBuf[MAX_CACHE_DIR_SIZE];
	long	base = 0;
	int	numFiles = 0;
	int	nameHeapSize = 0;

	errno=0;
        cachedDirType = DtUNCACHED_DIR;
	/*
	 * Allow some 1024 bytes slop in the buffer
	 */
	while ( bufLen < max_dir_cache_len ) {
		if ( (currDirect = readdir_r(dirDesc, 
					(struct dirent *)(stackBuf+bufLen)))
			== NULL ) {
			if (!errno)	/* the end of the directory stream */
                	{
				/* fall through on eof */
				cachedDirType = DtVALID_CACHED_DIR;
				break;
			}
			else	/* real error; stop reading */
			{
				cachedDirType = DtINVALID_CACHED_DIR;
				break;		
			}
		}

		bufLen += currDirect->d_reclen;
		if ( bufLen >= MAX_CACHE_DIR_SIZE ) {
			/* almost impossible to be here */
			cachedDirType = DtUNCACHED_DIR;
			break;
		}
		numFiles ++;
		nameHeapSize += strlen(currDirect->d_name);
	}

        if( cachedDirType == DtVALID_CACHED_DIR)
        {
	    DtValidCachedDir	validDir;
	    String		nameHeap;
	    Cardinal		i;

	    /*
	     * we allocate an extra nameOffset to track the length of
	     * the last name
	     */
	    validDir = (DtValidCachedDir) 
	      XtMalloc((sizeof(DtValidCachedDirStruct)) +
		       (sizeof(short) * numFiles) +
		       (nameHeapSize));

	    validDir->dirNameLen = strlen(dirName);
	    validDir->dirName = dirName;
	    validDir->numFiles = numFiles;
	    cachedDirType = 
	      validDir->cachedDirType = 
		DtVALID_CACHED_DIR;
	    validDir->nameOffsets[0] = 0;
	    nameHeap = (String)
	      &(validDir->nameOffsets[numFiles + 1]);

	    for (i = 0, currDirect = (struct dirent *)stackBuf;
		 i < validDir->numFiles;
		 i++,
		 currDirect = (struct dirent *) 
		 (((char *)currDirect) + currDirect->d_reclen)) {
		validDir->nameOffsets[i + 1] = 
		  validDir->nameOffsets[i] + strlen (currDirect->d_name);
		memcpy(&(nameHeap[validDir->nameOffsets[i]]),
		       &(currDirect->d_name[0]),
		       strlen (currDirect->d_name));
	    }
          cachedDir = (DtCachedDir)validDir ;
	}
    }
    switch (cachedDirType) {
      case DtINVALID_CACHED_DIR:
      case DtUNCACHED_DIR:
	cachedDir = (DtCachedDir)
	  XtMalloc(sizeof(DtCommonCachedDirStruct));
	cachedDir->common.cachedDirType =
	  cachedDirType;
	cachedDir->common.dirNameLen = strlen(dirName);
	cachedDir->common.dirName = dirName;
	break;
      case DtVALID_CACHED_DIR:
	break;
    }
    if ((cachedDirType != DtINVALID_CACHED_DIR) &&
	(closedir(dirDesc) != 0)) {
	XtWarning("IconFileCache: unable to close the opened directory\n");
    }

    return cachedDir;
}

static 	DtCachedDirListStruct cacheList;

void
#ifdef _NO_PROTO
  XmeFlushIconFileCache(path)
String	path;
#else
  XmeFlushIconFileCache(String	path)
#endif
{
    Cardinal	dirNameLen;
    Cardinal 	i;

    /*
     * loop thru the dir list. if no path was specified then flush the
     * entire cache.
     */
    if (path) 
      dirNameLen = strlen(path);
    else
      dirNameLen = 0;
    for (i = 0; i < cacheList.numDirs; i++) {
	String			currName;
	int			currNameLen;
	String			nameHeap;
	DtValidCachedDir	currDir;

	currDir = (DtValidCachedDir)cacheList.dirs[i];
	if (!path ||
	    ((currDir->dirNameLen == dirNameLen) &&
	     (strncmp(currDir->dirName, path, dirNameLen) == 0))) {
	    XtFree(currDir->dirName);
	    XtFree((char *)currDir);

	    if (path) {
		/* ripple down the dir array */
		for (; i < cacheList.numDirs - 1; i++)
		  cacheList.dirs[i] = cacheList.dirs[i+1];
		cacheList.numDirs--;
		return;
	    }
	}
    }
    if (path && (i == cacheList.numDirs)) {
#ifdef notdef
	XtWarning("_DtFlushIconFileCache:directory not found\n");
#endif
	return;
    }
    cacheList.numDirs = 0;
    /* don't free the dirList itself */
}

static String __dirName;
static String __leafName;

static int
#ifdef _NO_PROTO
  CheckDirCache(path)
String	path;
#else
CheckDirCache(String	path)

#endif
{
    String	dirPtr, dirName;
    String	filePtr, fileName;
    String	suffixPtr, suffix;
    int		numDirs, dirNameLen, fileNameLen, suffixLen;
    Cardinal	i, j;
    char   	stackString[MAX_DIR_PATH_LEN];
    
    (void) AbsolutePathName(path, &path, stackString);
    _DtFindPathParts(path, &filePtr, &suffixPtr);
    
    if (path == filePtr) {
	dirNameLen = 0;
	fileNameLen = strlen(path);
    }
    else {
	/* take the slash into account */
	dirNameLen = filePtr - path - 1;
	fileNameLen = strlen(path) - dirNameLen - 1;
    }

    /*
     * set global variable for later use
     */
    __leafName = filePtr;

    if (dirNameLen == 0) {
	return DtINVALID_CACHED_DIR;
    }

    /*
     * loop thru the dir list. on the last pass create the new cached
     * dir and process it.  
     */
    numDirs = cacheList.numDirs;
    for (i = 0; i <= numDirs; i++) {
	String			currName;
	int			currNameLen;
	String			nameHeap;
	DtValidCachedDir	currDir;
	
	if (i == cacheList.numDirs) {

	    /*
	     * we didn't get a hit on the directory list so create a new one
	     */
	    if (cacheList.numDirs == cacheList.maxDirs) {
		cacheList.maxDirs += 16;
		cacheList.dirs = (DtCachedDir *)
		  XtRealloc((char *)cacheList.dirs,
			    cacheList.maxDirs * sizeof (DtCachedDir));
	    }
	    dirName = strncpy(XtMalloc(dirNameLen+1), path, dirNameLen);
	    dirName[dirNameLen] = '\0';
	    cacheList.dirs[cacheList.numDirs++] = MakeCachedDirEntry(dirName);
	}
	currDir = (DtValidCachedDir)cacheList.dirs[i];

	/*
	 * set global variable
	 */
	__dirName = currDir->dirName;

	if ((currDir->dirNameLen == dirNameLen) &&
	    (strncmp(currDir->dirName, path, dirNameLen) == 0)) {

	    switch(currDir->cachedDirType) {
	      case DtINVALID_CACHED_DIR:
	      case DtUNCACHED_DIR:
		return currDir->cachedDirType;
		break;
	      case DtVALID_CACHED_DIR:
		nameHeap = (String)
		  &(currDir->nameOffsets[currDir->numFiles + 1]);
		for (j = 0; j < currDir->numFiles; j++) {
		    /*
		     * nameOffsets has an extra offset to indicate the
		     * end of the last name (to handle border condition
		     */
		    currNameLen = (currDir->nameOffsets[j + 1] -
				   currDir->nameOffsets[j]);
		    if (currNameLen == fileNameLen) {
			currName =  &(nameHeap[currDir->nameOffsets[j]]);
			if (strncmp(currName, filePtr, currNameLen) == 0) {
			    return DtVALID_CACHED_DIR;
			}
		    }
		}
		return DtINVALID_CACHED_DIR;
		break;
	    }
	}
    }
    return DtINVALID_CACHED_DIR;
}

static String
#ifdef _NO_PROTO
find_slash(str)
String str ;
#else
find_slash(String str)
#endif
{
  int n;
  if (MB_CUR_MAX == 1) {
        return strchr(str, '/');
  } else {
      while ((n = mblen(str, MB_CUR_MAX)) >0) {
          if (n == 1 && *str == '/')
              return str;
          str += n;
      }
      return NULL;
  }
}

static Boolean
#ifdef _NO_PROTO
_DtTestIconFile(path)
    String	path;
#else
_DtTestIconFile(String	path)
#endif
{
    struct stat status;
    int	dirCacheType;

    if (!path || !*path)
      return False;

    if (!find_slash(path)) {
    dirCacheType = DtUNCACHED_DIR;
    __leafName = path;
    __dirName = ".";
    } else
    dirCacheType = CheckDirCache(path);

    switch(dirCacheType) {
      case DtVALID_CACHED_DIR:
	return True;
	break;
      case DtINVALID_CACHED_DIR:
	return False;
	break;
      case DtUNCACHED_DIR:
	return (access(path, R_OK) == 0 &&		/* exists and is readable */
		stat(path, &status) == 0 &&		/* get the status */
#ifndef X_NOT_POSIX
		S_ISDIR(status.st_mode) == 0	/* not a directory */
#else
		(status.st_mode & S_IFDIR) == 0	/* not a directory */
#endif /* X_NOT_POSIX else */
		);
	break;
    }
}

static DtHashKey
#ifdef _NO_PROTO
GetEmbeddedKey( entry, clientData)
  DtHashEntry	entry;
  XtPointer	clientData;
#else
GetEmbeddedKey(
  DtHashEntry	entry,
  XtPointer	clientData)
#endif /* _NO_PROTO */
{
    return (DtHashKey)(((char *)entry) + (int)clientData);
}

typedef struct _DtIconNameEntryPartRec{
    String	dirName;
    String	leafName;
    char	name[1];
}DtIconNameEntryPartRec, *DtIconNameEntryPart;

typedef struct _DtIconNameEntryRec{
    DtHashEntryPartRec		hash;
    DtIconNameEntryPartRec	iconName;
}DtIconNameEntryRec, *DtIconNameEntry;

DtHashEntryTypeRec dtIconNameEntryTypeRec = {
    {
	sizeof(DtIconNameEntryRec),
	(DtGetHashKeyFunc)GetEmbeddedKey,
	(XtPointer)XtOffset(DtIconNameEntry, iconName.name[0]),
	NULL,
    },
};
DtHashEntryType dtIconNameEntryTypes[] =  {
    (DtHashEntryType)&dtIconNameEntryTypeRec,
};

char ABSOLUTE_IPATH[] = {'%', 'H', '%', 'B', '\0'}; 

#define NUM_HOME_DIR_SPECS 6
/** NOTE: if NUM_HOME_DIR_SPECS changes, search for "NOTE:" **/
/**       to see a corresponding change which must be made  **/

#define B_SUB	0
#define P_SUB	1
#define M_SUB	2
#define H_SUB	3

static SubstitutionRec iconSubs[] = {
    {'B', NULL},	/* bitmap name */
    {'P', NULL},	/* alternate bitmap name BC */
    {'M', NULL},	/* magnitude */
    {'H', NULL},	/* host prefix */
};

String
#ifdef _NO_PROTO
XmGetIconFileName(screen, imageInstanceName, imageClassName, hostPrefix, size)
    Screen	*screen;
    String	imageInstanceName;
    String	imageClassName;
    String	hostPrefix;
    unsigned int size;
#else
XmGetIconFileName(
    Screen	*screen,		     
    String	imageInstanceName,
    String	imageClassName,
    String	hostPrefix,
    unsigned int size)
#endif
{
    Display		*display = DisplayOfScreen(screen);
    String		fileName = NULL;
    String		names[2];
    String		names_w_size[2];
    String		bPath, iPath;
    Cardinal		i, j, k, pathLen;
    Boolean		useColor;
    Boolean		useMask;
    Boolean		useIconFileCache;
    Boolean		absolute;
    char 		stackString[MAX_DIR_PATH_LEN];
    Boolean		(*testFileFunc)();

    static Boolean	firstTime = True;
    static String	iconPath = NULL;
    static String	bmPath = NULL;
    static DtHashTable iconNameCache;

    /* generate the icon path once pre application */
    if (firstTime) {
	String          tmpPath;
	Boolean		junkBoolean;

	iconNameCache = 
	  _XmAllocHashTable(dtIconNameEntryTypes,
			     XtNumber(dtIconNameEntryTypes),
			     True /* keyIsString */);
    
	firstTime = False;
	cacheList.numDirs =
	  cacheList.maxDirs = 0;
	cacheList.dirs = NULL;

        (void)_XmGetIconControlInfo(screen, 
				&useMask,
				&useColor,
				&useIconFileCache);

	if (useColor) {
	    tmpPath = getenv("XMICONSEARCHPATH");
	}
	else {
	    tmpPath = getenv("XMICONBMSEARCHPATH");
	}
	if (tmpPath) {
	    pathLen = strlen(tmpPath);
	    iconPath = XtCalloc(1, pathLen+1);
	    strcpy(iconPath, tmpPath);
	}
	else
	    iconPath = NULL;

	bmPath = __DtOSInitPath(NULL, "XBMLANGPATH", &junkBoolean);

    }

    (void)_XmGetIconControlInfo(screen, 
			        &useMask,
			        &useColor,
			        &useIconFileCache);

#ifdef SIZE_SUPPORT
    /*
     * calculate the size
     */
    if (size == XmUNSPECIFIED_ICON_SIZE)
      size = DtGetScreenSize(screen);
#endif

    switch (size) {
      case XmTINY_ICON_SIZE:
	iconSubs[M_SUB].substitution = ".t";
	break;
      case XmSMALL_ICON_SIZE:
	iconSubs[M_SUB].substitution = ".s";
	break;
      case XmMEDIUM_ICON_SIZE:
	iconSubs[M_SUB].substitution = ".m";
	break;
      case XmLARGE_ICON_SIZE:
	iconSubs[M_SUB].substitution = ".l";
	break;
      case XmNONE:
	iconSubs[M_SUB].substitution = NULL;
	break;
    }

    iconSubs[H_SUB].substitution = hostPrefix;

    if (useIconFileCache)
      testFileFunc = _DtTestIconFile;
    else
      testFileFunc = NULL;

    names[0] 	    = imageInstanceName;
    names[1] 	    = imageClassName;


    for (i = 0; i < 2; i++) {
	if (names[i] == NULL)
	  continue;

	if (absolute = AbsolutePathName(names[i], &names[i], stackString)) {
	    iPath = ABSOLUTE_IPATH;
	    bPath = ABSOLUTE_PATH;
	}
	else {
	    iPath = iconPath;
	    bPath = bmPath;
	}
	iconSubs[B_SUB].substitution = names[i];
	iconSubs[P_SUB].substitution = names[i];

       /* need to add size suffix if size is XmUNSPECIFIED_ICON_SIZE */
        if (size != XmUNSPECIFIED_ICON_SIZE && size != XmNONE) {
           int basenameLen = strlen(names[i]);
           int sizeLen = strlen(iconSubs[M_SUB].substitution);
	   char * ext_name = XtMalloc(basenameLen + sizeLen + 1);
#ifndef	SVR4
	   bcopy(names[i], &ext_name[0], basenameLen);
	   bcopy(iconSubs[M_SUB].substitution,
		 &ext_name[basenameLen], sizeLen);
#else
	   memmove(&ext_name[0], names[i], basenameLen);
	   memmove(&ext_name[basenameLen],
	           iconSubs[M_SUB].substitution, sizeLen);
#endif /* SVR4 */
	   ext_name[basenameLen + sizeLen] = '\0';

           names_w_size[i] = ext_name;

        } else
           names_w_size[i] = NULL;

       /*
        * try to see if its already in the image cache
	*/
	if (_XmInImageCache(names[i]))
	  fileName = XtNewString(names[i]);


	/*
	 * optimization to check all expansions in cache
	 */
	if (!fileName) {
	    DtIconNameEntry iNameEntry;

            if (names_w_size[i] != NULL)
	       iNameEntry =  (DtIconNameEntry) 
	         _XmKeyToHashEntry(iconNameCache,(DtHashKey)names_w_size[i]);
            else
	       iNameEntry =  (DtIconNameEntry) 
	         _XmKeyToHashEntry(iconNameCache,(DtHashKey)names[i]);

	    if (iNameEntry) {
#ifdef USE_SLOW_SPRINTF		
		sprintf(fileName, "%s/%s", 
			iNameEntry->iconName.dirName,
			iNameEntry->iconName.leafName);
#else
		int dirLen, leafLen;

		dirLen = strlen(iNameEntry->iconName.dirName);
		leafLen = strlen(iNameEntry->iconName.leafName);
		fileName = XtMalloc(dirLen + leafLen + 2);

#ifndef	SVR4
		bcopy(iNameEntry->iconName.dirName, 
		      &fileName[0],
		      dirLen);
		fileName[dirLen] = '/';
		bcopy(iNameEntry->iconName.leafName, 
		      &fileName[dirLen + 1],
		      leafLen);
#else
		memmove(&fileName[0],
	      		iNameEntry->iconName.dirName,
			dirLen);
		fileName[dirLen] = '/';
		memmove(&fileName[dirLen + 1],
	      		iNameEntry->iconName.leafName,
			leafLen);
#endif
		fileName[dirLen + leafLen + 1] = '\0';
#endif
	    }
	}

	if (fileName)
	  return fileName;

	/*
	 * first try DTPM and then XBM
	 */
	if (iPath)
	  fileName = 
	    XtResolvePathname(display, "icons", NULL,
			    NULL, iPath, iconSubs, 
			    XtNumber(iconSubs),
			    testFileFunc);
	
#ifdef ICON_FILE_DEBUG
	printf("DTPM XtResolve called with %s result = %s\n", names[i], fileName);
#endif
	if (fileName == NULL) {
	    fileName = 
	      XtResolvePathname(display, "bitmaps", NULL,
				NULL, bPath, iconSubs, 
				XtNumber(iconSubs),
				testFileFunc);
#ifdef ICON_FILE_DEBUG
	printf("XBM XtResolve called with %s result = %s\n", names[i], fileName);
#endif
	}
#ifdef ICON_FILE_DEBUG
	printf("\n");
#endif
	if (fileName)
	  break;
    }
    if (fileName && !absolute) {
	/* register it in name cache */
	DtIconNameEntry 	iNameEntry;
	int			nameLen;

        if (names_w_size[i] != NULL)
        {
	   nameLen = strlen(names_w_size[i]);
	   iNameEntry = (DtIconNameEntry)
	        XtCalloc(1, dtIconNameEntryTypeRec.hash.entrySize + nameLen);

	   strcpy(((char *)&(iNameEntry->iconName.name[0])),
	          names_w_size[i]);
        }
        else
        {
	   nameLen = strlen(names[i]);
	   iNameEntry = (DtIconNameEntry)
	        XtCalloc(1, dtIconNameEntryTypeRec.hash.entrySize + nameLen);

	   strcpy(((char *)&(iNameEntry->iconName.name[0])),
	          names[i]);
        }

        if (useIconFileCache)
        {
	   iNameEntry->hash.type = 0;
	   iNameEntry->iconName.dirName = XtNewString(__dirName);
	   iNameEntry->iconName.leafName = XtNewString(__leafName);
        }
        else
        {
           String	dirName;
           String	filePtr;
           String 	suffixPtr;
           int	dirNameLen;

           _DtFindPathParts(fileName, &filePtr, &suffixPtr);

           if (fileName == filePtr)
	       dirNameLen = 0;
           else {
	       /* take the slash into account */
	       dirNameLen = filePtr - fileName - 1;
           }

           dirName = (String)XtMalloc(dirNameLen + 1);
	   strncpy(dirName, fileName, dirNameLen);
	   dirName[dirNameLen] = '\0';

	   iNameEntry->hash.type = 0;
	   iNameEntry->iconName.dirName = dirName;
	   iNameEntry->iconName.leafName = XtNewString(filePtr);
        }
        if (names_w_size[i] != NULL)
	   _XmRegisterHashEntry(iconNameCache, names_w_size[i], 
			        (DtHashEntry)iNameEntry);
        else
	   _XmRegisterHashEntry(iconNameCache, names[i], 
			        (DtHashEntry)iNameEntry);
    }
    return fileName;
}
    
      





