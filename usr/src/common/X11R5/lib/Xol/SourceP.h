#ifndef	NOIDENT
#ident	"@(#)oldtext:SourceP.h	1.7"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        SourceP.h
 **
 **   Project:     X Widgets
 **
 **   Description: private include file for TextPane widget sources
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 *****************************************************************************
 *************************************<+>*************************************/

#ifndef _OlSourceP_h
#define _OlSourceP_h

#define applySource(method) (*(self->text.source->method))

#define OlEstringSrc "stringsrc"
#define OlEdiskSrc   "disksrc"
#define MAGICVALUE -1

typedef struct _StringSourceData {
    OlDefine       editMode;
    unsigned char  *buffer;
    unsigned char  *initial_string;
    OlTextPosition length,          /* current data size of buffer */
                   buffer_size,     /* storage size of buffer */
                   max_size;        /* user specified buffer limit */
    int            max_size_flag;   /* flag to test max_size set */
} StringSourceData, *StringSourcePtr;

  
#endif
/* DON'T ADD STUFF AFTER THIS #endif */
