#pragma ident	"@(#)m1.2libs:Xm/VirtKeys.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmVirtKeys_h
#define _XmVirtKeys_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _OSF_Keysyms
#define _OSF_Keysyms

#define osfXK_BackSpace	0x1004FF08
#define osfXK_Insert	0x1004FF63
#define osfXK_Delete	0x1004FFFF
#define osfXK_Copy	0x1004FF02
#define osfXK_Cut	0x1004FF03
#define osfXK_Paste	0x1004FF04

#define osfXK_AddMode		0x1004FF31
#define osfXK_PrimaryPaste	0x1004FF32
#define osfXK_QuickPaste	0x1004FF33

#define osfXK_PageLeft	0x1004FF40
#define osfXK_PageUp	0x1004FF41
#define osfXK_PageDown	0x1004FF42
#define osfXK_PageRight	0x1004FF43

#define osfXK_EndLine	0x1004FF57
#define osfXK_BeginLine	0x1004FF58

#define osfXK_Activate	0x1004FF44

#define osfXK_MenuBar	0x1004FF45

#define osfXK_Clear	0x1004FF0B
#define osfXK_Cancel	0x1004FF69
#define osfXK_Help	0x1004FF6A
#define osfXK_Menu	0x1004FF67
#define osfXK_Select	0x1004FF60
#define osfXK_Undo	0x1004FF65

#define osfXK_Left	0x1004FF51
#define osfXK_Up	0x1004FF52
#define osfXK_Right	0x1004FF53
#define osfXK_Down	0x1004FF54

#endif  /* OSF_Keysyms */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmTranslateKey() ;

#else

extern void XmTranslateKey( 
                        Display *dpy,
#if NeedWidePrototypes
                        unsigned int keycode,
#else
                        KeyCode keycode,
#endif /* NeedWidePrototypes */
                        Modifiers modifiers,
                        Modifiers *modifiers_return,
                        KeySym *keysym_return) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmVirtKeys_h */
