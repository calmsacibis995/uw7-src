#pragma ident	"@(#)m1.2libs:Xm/FontObj.h	1.1"
/*** FontObj.h ***/

#ifndef _FontObj_h
#define _FontObj_h

#include <Xm/Xm.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WidgetClass  _xmFontObjectClass;

typedef struct _FontObjectClassRec *FontObjectClass;
typedef struct _FontObjectRec      *FontObject;

#define XmNsansSerifFamilyFontList    "sansSerifFamilyFontList"
#define XmCSansSerifFamilyFontList    "SansSerifFamilyFontList"
#define XmNserifFamilyFontList        "serifFamilyFontList"
#define XmCSerifFamilyFontList        "SerifFamilyFontList"
#define XmNmonospacedFamilyFontList   "monospacedFamilyFontList"
#define XmCMonospacedFamilyFontList   "MonospacedFamilyFontList"
#define XmNuseFontObject              "useFontObject"
#define XmCUseFontObject              "UseFontObject"
#define XmNdynamicFontCallback        "dynamicFontCallback"
#define XmCDynamicFontCallback        "DynamicFontCallback"

typedef struct{
	int reason;
	XEvent * event;
	XmFontList new_font;
	XmFontList current_font;
} XmFontObjectCallbackStruct;

#define XmCR_SERIF_FAMILY_CHANGED	1
#define XmCR_SANS_SERIF_FAMILY_CHANGED	2
#define XmCR_MONOSPACED_FAMILY_CHANGED	3

#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _FontObj_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
