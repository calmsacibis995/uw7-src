/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/fileinfo.h	1.2"
#endif


#ifndef _FileInfo
#define _FileInfo


typedef struct _Filelist
{
   int dir_size;
   int dir_used;
   char ** dirs;
} Filelist;

typedef struct _FileInfo 
{
   char *      name;             /* current name   */
   char *      directory;        /* current directory                      */
   Filelist *  list;             /* contents of the current directory      */
   Widget      controlWidget;    /* control area       (returned)          */
   Widget      textFieldWidget;  /* text field         (returned)          */
   Widget      textFieldCaption; /* text field         (returned)          */
   Widget      curItemWidget;	/* static text        (returned)          */
   Widget      curPathWidget;	/* static text        (returned)          */
   Widget      curFolderWidget;	/* static text        (returned)          */
   Widget      subdirListWidget; /* subdirs list       (returned)          */
} FileInfo;

#endif /* _FileInfo_h */
