#pragma ident	"@(#)olmisc:ColorObjP.h	1.1"
/*** ColorObjP.h ***/

#ifndef _ColorObjP_h
#define _ColorObjP_h

#include <Xol/ColorObj.h>
#include <Xol/VendorI.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


#define XmNuseIconFileCache	"useIconFileCache"
#define XmCUseIconFileCache	"UseIconFileCache"

/* this is from XmP.h */
#define XmBACKGROUND     ((unsigned char) (1<<0))
#define XmFOREGROUND     ((unsigned char) (1<<1))
#define XmTOP_SHADOW     ((unsigned char) (1<<2))
#define XmBOTTOM_SHADOW  ((unsigned char) (1<<3))
#define XmSELECT         ((unsigned char) (1<<4))
 
/*  Structure used to hold color schemes  */
typedef struct _XmColorData
{  Screen * screen;
   Colormap color_map;
   unsigned char allocated;
   XColor background;
   XColor foreground;
   XColor top_shadow;
   XColor bottom_shadow;
   XColor select;
} XmColorData;
/* end of stuff from XmP.h */

typedef PixelSet Colors[NUM_COLORS];

typedef struct _ColorObjPart {
    XtArgsProc          RowColInitHook;
    PixelSet       	*myColors;     /* colors for my (application) screen */
    int             	myScreen;
    Display             *display;      /* display connection for "pseudo-app" */
    Colors         	*colors;       /* colors per screen for workspace mgr */
    int             	numScreens;    /*               for workspace manager */
    Atom           	*atoms;        /* to identify colorsrv screen numbers */
    Boolean         	colorIsRunning;    /* used for any color problem      */
    Boolean         	done;
    int            	*colorUse;
    int             	primary;
    int             	secondary;
    int             	active;
    int             	inactive;
    Boolean         	useColorObj;  /* read only resource variable */
    
    Boolean		useMask;
    Boolean		useMultiColorIcons;
    Boolean		useIconFileCache;

} ColorObjPart;


typedef struct _ColorObjRec {
    CorePart 		core;
    CompositePart 	composite;
    ShellPart 		shell;
    WMShellPart		wm;
    ColorObjPart	color_obj;
} ColorObjRec;

typedef struct _ColorObjClassPart {
    XtPointer        extension;
} ColorObjClassPart;

/* 
 * we make it a appShell subclass so it can have it's own instance
 * hierarchy
 */
typedef struct _ColorObjClassRec{
    CoreClassPart      		core_class;
    CompositeClassPart 		composite_class;
    ShellClassPart  		shell_class;
    WMShellClassPart   		wm_shell_class;
    ColorObjClassPart		color_obj_class;
} ColorObjClassRec;


extern ColorObjClassRec _xmColorObjClassRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmColorObjCreate() ;
extern Boolean _XmGetPixelData() ;
extern Boolean _XmGetIconControlInfo() ;
extern Boolean _XmUseColorObj() ;

#else

extern void _XmColorObjCreate( 
                        Widget w,
                        ArgList al,
                        Cardinal *acPtr) ;
extern Boolean _XmGetPixelData( 
                        int screen,
                        int *colorUse,
                        PixelSet *pixelSet,
                        short *a,
                        short *i,
                        short *p,
                        short *s) ;
extern Boolean _XmGetIconControlInfo( 
                        Screen *screen,
                        Boolean *useMaskRtn,
                        Boolean *useMultiColorIconsRtn,
                        Boolean *useIconFileCacheRtn) ;
extern Boolean _XmUseColorObj( void ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _ColorObjP_h */

