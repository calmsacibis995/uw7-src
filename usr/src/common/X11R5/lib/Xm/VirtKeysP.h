#pragma ident	"@(#)m1.2libs:Xm/VirtKeysP.h	1.2"
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
#ifndef _XmVirtKeysP_h
#define _XmVirtKeysP_h

#include <Xm/XmP.h>
#include <Xm/VirtKeys.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XmKEYCODE_TAG_SIZE 32

typedef struct _XmDefaultBindingStringRec{
    String	vendorName;
    String	defaults;
}XmDefaultBindingStringRec, *XmDefaultBindingString;

typedef	struct _XmKeyBindingRec{
    KeySym		keysym;
    unsigned int	modifiers;
}XmKeyBindingRec, *XmKeyBinding;

typedef	struct _XmVirtualKeysymRec{
    String		name;
    KeySym		keysym;
}XmVirtualKeysymRec, *XmVirtualKeysym;

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmVirtKeysInitialize() ;
extern void _XmVirtKeysDestroy() ;
extern void _XmVirtKeysHandler() ;
extern void _XmVirtualToActualKeysym() ;
extern void _XmVirtKeysStoreBindings() ;
extern Boolean _XmVirtKeysLoadFileBindings() ;
extern int _XmVirtKeysLoadFallbackBindings() ;

#else

extern void _XmVirtKeysInitialize( 
                        Widget widget) ;
extern void _XmVirtKeysDestroy( 
                        Widget widget) ;
extern void _XmVirtKeysHandler( 
                        Widget widget,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *dontSwallow) ;
extern void _XmVirtualToActualKeysym( 
                        Display *dpy,
                        KeySym virtKeysym,
                        KeySym *actualKeysymRtn,
                        Modifiers *modifiersRtn) ;
extern void _XmVirtKeysStoreBindings( 
                        Widget shell,
                        String binding) ;
extern Boolean _XmVirtKeysLoadFileBindings( 
                        char *fileName,
                        String *binding) ;
extern int _XmVirtKeysLoadFallbackBindings(
			Display *display,
			String *binding) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmVirtKeysP_h */
