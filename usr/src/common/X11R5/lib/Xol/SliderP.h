#ifndef	NOIDENT
#ident	"@(#)slider:SliderP.h	1.16"
#endif

/* 
 * Slider.h - Private definitions for Slider widget
 * 
 */

#ifndef _SliderP_h
#define _SliderP_h

/***********************************************************************
 *
 * Slider Widget Private Data
 *
 ***********************************************************************/

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Slider.h>

#include <Xol/Olg.h>

/* Types of operation */
#define	ANCHOR	       1
#define	DIR_INC	       2
#define	PAGE	       4
#define	ELEV_OP		8
#define	DRAG_OP		16
#define KBD_OP		32	/* mouseless operation indicator */
#define	ANCHOR_TOP     (ANCHOR)
#define	ANCHOR_BOT     (ANCHOR | DIR_INC)
#define	PAGE_DEC       (PAGE)
#define	PAGE_INC       (PAGE | DIR_INC)
#define GRAN_DEC	(ELEV_OP)
#define GRAN_INC	(ELEV_OP | DIR_INC)
#define	DRAG_ELEV      (ELEV_OP	| DRAG_OP)
#define	NOOP		255

/* Types of slider */
#define	SB_REGULAR	3
#define	SB_MINREG	7

/* New fields for the Slider widget class record */
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} SliderClassPart;

/* Full class record declaration */
typedef struct _SliderClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	SliderClassPart		slider_class;
} SliderClassRec;

extern SliderClassRec sliderClassRec;

/* 
 * New fields for the Slider widget record.
 * This struct must be the same as the Gauge widget except the extra members
 * for the slider widgets at the end.
 */

typedef struct {
	/* Public */
        int	sliderMin; 
        int	sliderMax; 
        int	sliderValue; 
	int	scale;
	int	ticks;
	char	*minLabel;
	char	*maxLabel;
	Dimension	span;
	Dimension	leftMargin;	/* user-specified leftMargin */
	Dimension	rightMargin;	/* user-specified rightMargin */
        OlDefine orientation; 
        OlDefine tickUnit; 
        OlDefine stoppos; 
        OlDefine dragtype; 
	Boolean	endBoxes;
	Boolean	recompute_size;
        XtCallbackList	sliderMoved; 
	Boolean showValue;	/* Moolit: Motif mode only */

	/* Private */
	GC	labelGC;
	OlgAttrs *pAttrs;
	int	numticks;
	XtIntervalId	timerid;
	Position	sliderPValue;
	Position	minTickPos;	/* Open Look:  position of minTick;
					 * Motif: first_position */ 
	Position	maxTickPos;	/* Open Look: position of maxTick.
					 * Motif: last_position */
	Position	*ticklist;	/* list of tickmark positions */
	Dimension	leftPad;	/* copy from leftMargin, actual val */
	Dimension	rightPad;	/* copy from rightMargin, actual val */
	Position	elev_offset;
	unsigned char	type;
	unsigned char	opcode;
	unsigned char	anchlen;
	unsigned char	anchwidth;
	unsigned char	elevwidth;
	unsigned char	elevheight;
	char		*display_value; /* if showValue = True */

	/* stuffs used only in slider widgets */
	Boolean		warp_pointer;
	Position	dragbase;	/* lengthwise pos. of mouse ptr */
	Position	absx; /* Motif: position_x */
	Position	absy; /* Motif: position_y */
        int	granularity; 
	int	repeatRate;
	int	initialDelay;
	
	/* new */
	Dimension	topPad;
	Dimension	bottomPad;
} SliderPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _SliderRec {
	CorePart	core;
	PrimitivePart	primitive;
	SliderPart	slider;
} SliderRec;

/* GUI specific functions...	*/
extern void _OloSliderRecalc OL_ARGS((SliderWidget));
extern void _OlmSliderRecalc OL_ARGS((SliderWidget));

extern Dimension _OloSlidercalc_leftMargin OL_ARGS((SliderWidget));
extern Dimension _OlmSlidercalc_leftMargin OL_ARGS((SliderWidget));

extern Dimension _OloSlidercalc_rightMargin OL_ARGS((SliderWidget));
extern Dimension _OlmSlidercalc_rightMargin OL_ARGS((SliderWidget));

extern Dimension _OloSlidercalc_bottomMargin OL_ARGS((SliderWidget));
extern Dimension _OlmSlidercalc_bottomMargin OL_ARGS((SliderWidget));

extern Dimension _OloSlidercalc_topMargin OL_ARGS((SliderWidget));
extern Dimension _OlmSlidercalc_topMargin OL_ARGS((SliderWidget));

#endif /* _SliderP_h */
