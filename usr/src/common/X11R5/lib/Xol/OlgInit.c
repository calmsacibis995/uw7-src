#ifndef NOIDENT
#ident	"@(#)olg:OlgInit.c	1.23"
#endif

/* Initialize device structures */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>

#define DPI(dots,mm)	(((dots)*254+5)/((mm)*10))

#define DFLT_MAP	"/usr/X/lib/Xol/scale.map"
#define DFLT_H_DPI	82
#define DFLT_V_DPI	82
#define DFLT_SCALE	12

extern char *_OlGetScaleMap ();

unsigned	_olgIs3d = 1;

static _OlgDevice	*_OlgDeviceHead;


/* Busy pattern */
#define busy_width 8
#define busy_height 4

static char busy_bits[] = {
   0x11, 0x00, 0x44, 0x00,
};

/* Inactive pattern */

#define inactive_width 8
#define inactive_height 2

static unsigned char inactive_bits[] = {
   0x55, 0xaa,
};

/* Default values for arcs.  These are correct for 'h' type screens at
 * the 12 pt. scale.
 */

static unsigned char dfltOblongB2ULData [] = {
    7, 11,	5, 6,	4, 4,	3, 3,	2, 2,	1, 1,
    1, 1,	0, 0,	0, 0,	0, 0,	0, 0,
};

static _OlgDesc	dfltOblongB2UL = {
    12, 11, dfltOblongB2ULData,
};

static unsigned char dfltOblongB2URData [] = {
    0, 2,	3, 4,	5, 5,	6, 6,	7, 7,	8, 8,
    8, 9,	9, 9,	9, 10,	9, 10,	9, 10,
};

static _OlgDesc dfltOblongB2UR = {
    11, 11, dfltOblongB2URData,
};

static unsigned char dfltOblongB2LLData [] = {
    0, 0,	0, 0, 	0, 0, 	0, 0,	1, 1,	1, 1,
    2, 2,	3, 3,	4, 4,	5, 6,	6, 11,	8, 11,
};

static _OlgDesc dfltOblongB2LL = {
    12, 12, dfltOblongB2LLData,
};

static unsigned char dfltOblongB2LRData [] = {
    9, 10,	9, 10,	9, 10,	9, 10,	8, 10,	8, 9,
    7, 9,	6, 8,	5, 7,	3, 6,	0, 5,	0, 3,
};

static _OlgDesc dfltOblongB2LR = {
    11, 12, dfltOblongB2LRData,
};

static unsigned char dfltOblongB3ULData [] = {
    7, 11,	5, 6,	4, 4,	3, 3,	2, 2,	1, 1,
    1, 1,	0, 0,	0, 0,	0, 0,	0, 0,
};

static _OlgDesc	dfltOblongB3UL = {
    12, 11, dfltOblongB3ULData,
};

static unsigned char dfltOblongB3URData [] = {
    0, 2,	3, 4,	5, 5,	6, 6,	7, 7,	8, 8,
    8, 8,	9, 9,	9, 9,	9, 9,	9, 9,
};

static _OlgDesc dfltOblongB3UR = {
    10, 11, dfltOblongB3URData,
};

static unsigned char dfltOblongB3LLData [] = {
    0, 0,	0, 0,	0, 0,	0, 0,	1, 1,	1, 1,
    2, 2,	3, 3,	4, 4,	5, 6,	7, 11,
};

static _OlgDesc dfltOblongB3LL = {
    12, 11, dfltOblongB3LLData,
};

static unsigned char dfltOblongB3LRData [] = {
    9, 9,	9, 9,	9, 9,	9, 9,	8, 8,	8, 8,
    7, 7,	6, 6,	5, 5,	3, 4,	0, 2,
};

static _OlgDesc dfltOblongB3LR = {
    10, 11, dfltOblongB3LRData,
};

static unsigned char dfltOblongDefULData [] = {
    6, 9,	4, 5,	3, 3,	2, 2,	1, 1,	1, 1,
    0, 0,	0, 0,	0, 0,
};

static _OlgDesc dfltOblongDefUL = {
    10, 9, dfltOblongDefULData,
};

static unsigned char dfltOblongDefURData [] = {
    0, 1,	2, 3,	4, 4,	5, 5,	6, 6,	6, 6,
    7, 7,	7, 7,	7, 7,
};

static _OlgDesc dfltOblongDefUR = {
    8, 9, dfltOblongDefURData,
};

static unsigned char dfltOblongDefLLData [] = {
    0, 0,	0, 0,	0, 0,	1, 1,	1, 1,	2, 2,
    3, 3,	4, 5,	6, 9,
};

static _OlgDesc dfltOblongDefLL = {
    10, 9, dfltOblongDefLLData,
};

static unsigned char dfltOblongDefLRData [] = {
    7, 7,	7, 7,	7, 7,	6, 6,	6, 6,	5, 5,
    4, 4,	2, 3,	0, 1,
};

static _OlgDesc dfltOblongDefLR = {
    8, 9, dfltOblongDefLRData,
};

static unsigned char dfltRect2ULData [] = {
    1, 2,	0, 0,	0, 0,
};

static _OlgDesc dfltRect2UL = {
    3, 3, dfltRect2ULData,
};

static unsigned char dfltRect2URData [] = {
    0, 0,	1, 1,	1, 2,
};

static _OlgDesc dfltRect2UR = {
    3, 3, dfltRect2URData,
};

static unsigned char dfltRect2LLData [] = {
    0, 0,	1, 2,	2, 2,
};

static _OlgDesc dfltRect2LL = {
    3, 3, dfltRect2LLData,
};

static unsigned char dfltRect2LRData [] = {
    1, 2,	0, 2,	0, 1,
};

static _OlgDesc dfltRect2LR = {
    3, 3, dfltRect2LRData,
};

static unsigned char dfltRect3ULData [] = {
    1, 1,	0, 0,
};

static _OlgDesc dfltRect3UL = {
    2, 2, dfltRect3ULData,
};

static unsigned char dfltRect3URData [] = {
    0, 0,	1, 1,
};

static _OlgDesc dfltRect3UR = {
    2, 2, dfltRect3URData,
};

static unsigned char dfltRect3LLData [] = {
    0, 0,	1, 1,
};

static _OlgDesc dfltRect3LL = {
    2, 2, dfltRect3LLData,
};

static unsigned char dfltRect3LRData [] = {
    1, 1,	0, 0,
};

static _OlgDesc dfltRect3LR = {
    2, 2, dfltRect3LRData,
};

static unsigned char dfltSliderVULData [] = {
    1, 2,	0, 0,
};

static _OlgDesc dfltSliderVUL = {
    3, 2, dfltSliderVULData,
};

static unsigned char dfltSliderVURData [] = {
    0, 0,	1, 1,
};

static _OlgDesc dfltSliderVUR = {
    2, 2, dfltSliderVURData,
};

static unsigned char dfltSliderVLLData [] = {
    0, 0,	1, 2,
};

static _OlgDesc dfltSliderVLL = {
    3, 2, dfltSliderVLLData,
};

static unsigned char dfltSliderVLRData [] = {
    1, 1,	0, 0,
};

static _OlgDesc dfltSliderVLR = {
    2, 2, dfltSliderVLRData,
};

static unsigned char dfltSliderHULData [] = {
    1, 1,	0, 0,	0, 0,
};

static _OlgDesc dfltSliderHUL = {
    2, 3, dfltSliderHULData,
};

static unsigned char dfltSliderHURData [] = {
    0, 0,	1, 1,	1, 1,
};

static _OlgDesc dfltSliderHUR = {
    2, 3, dfltSliderHURData,
};

static unsigned char dfltSliderHLLData [] = {
    0, 0,	1, 1,
};

static _OlgDesc dfltSliderHLL = {
    2, 2, dfltSliderHLLData,
};

static unsigned char dfltSliderHLRData [] = {
    1, 1,	0, 0,
};

static _OlgDesc dfltSliderHLR = {
    2, 2, dfltSliderHLRData,
};


static unsigned char dfltArrowUpData [] = {
    4, 5,	3, 6,	2, 7,	1, 8,	0, 9,	0, 9,
};

static _OlgDesc dfltArrowUp = {
    10, 6, dfltArrowUpData,
};

static unsigned char dfltArrowDownData [] = {
    0, 9,	0, 9,	1, 8,	2, 7,	3, 6,	4, 5,
};

static _OlgDesc dfltArrowDown = {
    10, 6, dfltArrowDownData,
};

static unsigned char dfltArrowLeftData [] = {
    4, 5,	3, 5,	2, 5,	1, 5,	0, 5,	0, 5,
    1, 5,	2, 5,	3, 5,	4, 5,
};

static _OlgDesc dfltArrowLeft = {
    6, 10, dfltArrowLeftData,
};

static unsigned char dfltArrowRightData [] = {
    0, 1,	0, 2,	0, 3,	0, 4,	0, 5,	0, 5,
    0, 4,	0, 3,	0, 2,	0, 1,
};

static _OlgDesc dfltArrowRight = {
    6, 10, dfltArrowRightData,
};

static unsigned char dfltArrowTextData [] = {
    0, 0,	0, 1,	0, 2,	0, 3,	0, 2,	0, 1,
    0, 0,
};

static _OlgDesc dfltArrowText = {
    4, 7, dfltArrowTextData,
};


static unsigned char dfltShArrowUpCenterData [] = {
    1, 0,	1, 0,	1, 0,	5, 5,	5, 5,	4, 6,
    4, 6,	3, 7,	3, 7,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowUpCenter = {
    11, 11, dfltShArrowUpCenterData,
};

static unsigned char dfltShArrowUpDarkData [] = {
    1, 0,	6, 6,	6, 6,	6, 7,	6, 7,	7, 8,
    7, 8,	8, 9,	8, 9,	2, 10,	1, 10,
};

static _OlgDesc dfltShArrowUpDark = {
    11, 11, dfltShArrowUpDarkData,
};

static unsigned char dfltShArrowUpBrightData [] = {
    5, 5,	4, 5,	4, 5,	3, 4,	3, 4,	2, 3,
    2, 3,	1, 2,	1, 2,	0, 1,	0, 0,
};

static _OlgDesc dfltShArrowUpBright = {
    11, 11, dfltShArrowUpBrightData,
};

static unsigned char dfltShArrowDownCenterData [] = {
    1, 0,	1, 0,	3, 7,	3, 7,	4, 6,	4, 6,
    5, 5,	5, 5,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowDownCenter = {
    11, 11, dfltShArrowDownCenterData,
};

static unsigned char dfltShArrowDownDarkData [] = {
    1, 0,	9, 10,	8, 9,	8, 9,	7, 8,	7, 8,
    6, 7,	6, 7,	5, 6,	5, 6,	5, 5,
};

static _OlgDesc dfltShArrowDownDark = {
    11, 11, dfltShArrowDownDarkData,
};

static unsigned char dfltShArrowDownBrightData [] = {
    0, 10,	0, 8,	1, 2,	1, 2,	2, 3,	2, 3,
    3, 4,	3, 4,	4, 4,	4, 4,	1, 0,
};

static _OlgDesc dfltShArrowDownBright = {
    11, 11, dfltShArrowDownBrightData,
};

static unsigned char dfltShArrowRightCenterData [] = {
    1, 0,	1, 0,	1, 0,	2, 3,	2, 5,	2, 7,
    2, 5,	2, 3,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowRightCenter = {
    11, 11, dfltShArrowRightCenterData,
};

static unsigned char dfltShArrowRightDarkData [] = {
    1, 0,	1, 0,	1, 0,	1, 0,	1, 0,	8, 10,
    6, 9,	4, 7,	2, 5,	1, 3,	1, 1,
};

static _OlgDesc dfltShArrowRightDark = {
    11, 11, dfltShArrowRightDarkData,
};

static unsigned char dfltShArrowRightBrightData [] = {
    0, 1,	0, 3,	0, 5,	4, 7,	6, 9,	1, 0,
    1, 0,	1, 0,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowRightBright = {
    11, 11, dfltShArrowRightBrightData,
};

static unsigned char dfltShArrowRightVertData [] = {
    1, 0,	1, 0,	1, 0,	0, 1,	0, 1,	0, 1,
    0, 1,	0, 1,	0, 1,	0, 0,	0, 0,
};

static _OlgDesc dfltShArrowRightVert = {
    11, 11, dfltShArrowRightVertData,
};

static unsigned char dfltShArrowLeftCenterData [] = {
    1, 0,	1, 0,	1, 0,	7, 8,	5, 8,	3, 8,
    5, 8,	7, 8,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowLeftCenter = {
    11, 11, dfltShArrowLeftCenterData,
};

static unsigned char dfltShArrowLeftDarkData [] = {
    1, 0,	1, 0,	1, 0,	1, 0,	1, 0,	1, 0,
    1, 4,	3, 6,	5, 10,	7, 10,	9, 10,
};

static _OlgDesc dfltShArrowLeftDark = {
    11, 11, dfltShArrowLeftDarkData,
};

static unsigned char dfltShArrowLeftBrightData [] = {
    9, 10,	7, 9,	5, 8,	3, 6,	1, 4,	0, 2,
    1, 0,	1, 0,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowLeftBright = {
    11, 11, dfltShArrowLeftBrightData,
};

static unsigned char dfltShArrowLeftVertData [] = {
    1, 0,	10, 10,	9, 10,	9, 10,	9, 10,	9, 10,
    9, 10,	9, 10,	1, 0,	1, 0,	1, 0,
};

static _OlgDesc dfltShArrowLeftVert = {
    11, 11, dfltShArrowLeftVertData,
};

#define pushpin2d_width 26
#define pushpin2d_height 48
static unsigned char pushpin2d_bits[] = {
   0x00, 0x07, 0x00, 0x00, 0xe0, 0x18, 0x00, 0x00, 0x58, 0x10, 0x00, 0x00,
   0x24, 0x20, 0x00, 0x00, 0x24, 0x20, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00,
   0x62, 0x10, 0x00, 0x00, 0xc2, 0x18, 0x00, 0x00, 0xc6, 0x1f, 0x00, 0x00,
   0x04, 0x0f, 0x00, 0x00, 0x1c, 0x0e, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00,
   0xe6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x20, 0x81, 0x01,
   0x00, 0x20, 0x41, 0x02, 0x00, 0x20, 0x7f, 0x02, 0x00, 0x20, 0x41, 0x02,
   0xe0, 0x3f, 0x41, 0x02, 0xc0, 0x3f, 0x41, 0x02, 0x00, 0x20, 0x7f, 0x02,
   0x06, 0x20, 0xff, 0x03, 0x09, 0xe0, 0xc1, 0x03, 0x09, 0xe0, 0x81, 0x01,
   0x06, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00,
   0x00, 0x20, 0x81, 0x01, 0x00, 0x20, 0x41, 0x02, 0x00, 0x20, 0x7f, 0x02,
   0x00, 0x20, 0x41, 0x02, 0xe0, 0x3f, 0x41, 0x02, 0xc0, 0x3f, 0x41, 0x02,
   0x00, 0x20, 0x7f, 0x02, 0x3c, 0x20, 0xff, 0x03, 0x42, 0xe0, 0xc1, 0x03,
   0x99, 0xe0, 0x81, 0x01, 0xa5, 0xe0, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00,
   0x99, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00};

#define pushpin3d_width 78
#define pushpin3d_height 48
static unsigned char pushpin3d_bits[] = {
   0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x08,
   0x00, 0x00, 0x40, 0x00, 0x00, 0x70, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00,
   0x40, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x80, 0x00,
   0x80, 0xfd, 0x01, 0x00, 0x24, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0xfd,
   0x01, 0x00, 0x22, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0xfd, 0x01, 0x00,
   0x62, 0x00, 0x00, 0x00, 0x40, 0x00, 0xc0, 0xf9, 0x00, 0x00, 0x02, 0x00,
   0x00, 0x00, 0x63, 0x00, 0xc0, 0x73, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
   0x7f, 0x00, 0x80, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3c, 0x00,
   0x80, 0x0f, 0x00, 0x00, 0x04, 0x00, 0x00, 0x60, 0x38, 0x00, 0x00, 0x1e,
   0x00, 0x00, 0x02, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x98, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x20, 0x80, 0x01, 0x00, 0x04, 0x00, 0x00, 0x0c, 0x00,
   0x00, 0x20, 0x40, 0x00, 0x00, 0x04, 0x08, 0x00, 0x0c, 0x18, 0x00, 0x20,
   0x7e, 0x00, 0x00, 0x04, 0x08, 0x00, 0x0c, 0x18, 0x00, 0x20, 0x40, 0x00,
   0x00, 0x04, 0x08, 0x00, 0xec, 0x1b, 0xe0, 0x3f, 0x40, 0x00, 0x00, 0x04,
   0x08, 0x00, 0xec, 0x1b, 0x00, 0x20, 0x00, 0x00, 0x7f, 0x04, 0x09, 0x00,
   0xec, 0x1b, 0x00, 0x20, 0x00, 0x00, 0x00, 0xfc, 0x09, 0x00, 0x0c, 0x18,
   0x00, 0x20, 0x00, 0x18, 0x00, 0xfc, 0x0f, 0x00, 0x0c, 0x00, 0x08, 0x20,
   0x00, 0x04, 0x00, 0x07, 0x6f, 0x00, 0x00, 0x00, 0x08, 0x20, 0x00, 0x04,
   0x00, 0x07, 0x66, 0x00, 0x00, 0x00, 0x06, 0x20, 0x00, 0x00, 0x00, 0x03,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x01, 0x00, 0x04,
   0x00, 0x00, 0x0c, 0x00, 0x00, 0x20, 0x40, 0x00, 0x00, 0x04, 0x08, 0x00,
   0x0c, 0x18, 0x00, 0x20, 0x7e, 0x00, 0x00, 0x04, 0x08, 0x00, 0x0c, 0x18,
   0x00, 0x20, 0x40, 0x00, 0x00, 0x04, 0x08, 0x00, 0xec, 0x1b, 0xe0, 0x3f,
   0x40, 0x00, 0x00, 0x04, 0x08, 0x00, 0xec, 0x1b, 0x00, 0x20, 0x00, 0x00,
   0x7f, 0x04, 0x09, 0x00, 0xec, 0x1b, 0x00, 0x20, 0x00, 0x00, 0x00, 0xfc,
   0x09, 0x00, 0x0c, 0x18, 0x3c, 0x20, 0x00, 0x00, 0x00, 0xfc, 0x0f, 0x00,
   0x0c, 0x00, 0x02, 0x20, 0x00, 0x00, 0x01, 0x07, 0x0f, 0x00, 0x00, 0x00,
   0x01, 0x20, 0x00, 0x60, 0x02, 0x07, 0x06, 0x00, 0x00, 0x00, 0x21, 0x20,
   0x00, 0x10, 0x02, 0x03, 0x80, 0x01, 0x00, 0x00, 0x21, 0x00, 0x00, 0x10,
   0x02, 0x00, 0x80, 0x01, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x02, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define checks_width 28
#define checks_height 13
static unsigned char checks_bits[] = {
   0x00, 0x20, 0x00, 0x00, 0x00, 0x18, 0x80, 0x01, 0x00, 0xcc, 0xff, 0x00,
   0x00, 0xc6, 0x7f, 0x02, 0x00, 0xc3, 0x3f, 0x03, 0x8c, 0xc3, 0x1c, 0x01,
   0xde, 0x41, 0x88, 0x01, 0xff, 0x01, 0x80, 0x01, 0xfe, 0x40, 0xc0, 0x01,
   0xfc, 0xc0, 0xc0, 0x01, 0x78, 0xc0, 0xe1, 0x01, 0x70, 0xc0, 0xe3, 0x01,
   0x20, 0xc0, 0xf7, 0x01};

/* Calculate the points within the ring defining the maximum vertical and
 * horizontal extents of the interior.  This point is inset 1 point from
 * the ring.
 */

static void
interiorPosition (pDev, ul, lr, pOrigX, pOrigY, pCornerX, pCornerY)
    _OlgDevice	*pDev;
    _OlgDesc	*ul, *lr;
    char	*pOrigX, *pOrigY, *pCornerX, *pCornerY;
{
    int			yOffset;
    unsigned char	*pData;
    unsigned		edge;

    /* Find the lowest point in the top of the ring. */
    pData = ul->pData + (ul->height << 1) + 1;
    edge = ul->width - 1;
    for (yOffset = ul->height; --yOffset >= 0; )
    {
	pData -= 2;
	if (*pData == edge)
	    break;
    }
    *pOrigY = yOffset + pDev->verticalStroke + 1;
    if (*pOrigY >= (int) ul->height)
	*pOrigX = ul->pData [((ul->height - 1) << 1) + 1] +
	    pDev->horizontalStroke + 1;
    else
	*pOrigX = ul->pData [(*pOrigY << 1) + 1] + pDev->horizontalStroke + 1;

    /* Find the highest point in the bottom of the ring. */
    pData = lr->pData;
    for (yOffset = 0; yOffset < (int) lr->height; yOffset++, pData += 2)
    {
	if (*pData == 0)
	    break;
    }
    *pCornerY = yOffset - pDev->verticalStroke;
    if (*pCornerY <= 0)
	*pCornerX = lr->width - (lr->pData [0] - pDev->horizontalStroke);
    else
	*pCornerX = lr->width - (lr->pData [(*pCornerY - 1) << 1] -
				 pDev->horizontalStroke);
    *pCornerY = lr->height - *pCornerY;
}

/* Determine the normal width of a line for a particular scale */

static void
OlgGetStrokeWidth (pDev)
    register _OlgDevice	*pDev;
{
    /* The width of a stroke depends on the scale, but we are going to
     * be lazy here and use 1 pt.  This is the correct value for the
     * 12 point scale.
     */

    pDev->horizontalStroke = OlScreenPointToPixel(OL_HORIZONTAL, 1, pDev->scr);
    if (pDev->horizontalStroke < 1)
	pDev->horizontalStroke = 1;
    pDev->verticalStroke = OlScreenPointToPixel(OL_VERTICAL, 1, pDev->scr);
    if (pDev->verticalStroke < 1)
	pDev->verticalStroke = 1;
}

/* Determine the base name of the correct arc and bitmap files to use
 * for this screen and scale.  Extract the name from the scale mapping
 * file that correspondes to the best match of resolution and scale.
 */

static char *
readScaleMap (fileName, hRes, vRes, scale)
    char	*fileName;
    unsigned	hRes, vRes, scale;
{
    FILE	*mapFile;
    char	*bestMatch;
    unsigned long	error, newError;
    char	buf[81];

    if (!(mapFile = fopen (fileName, "r")))
    {
	OlVaDisplayWarningMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileOlgInit_msg1,
				fileName);
	return (char *) 0;
    }

    /* Read the file.  Use a least squares error to determine the best match */
    bestMatch = (char *) 0;
    error = ~0;
    while (True)
    {
	int	matched;
	unsigned	hR, vR, sc;

	matched = fscanf (mapFile, "%u%u%u%80s", &hR, &vR, &sc, buf);
	if (matched == EOF)
	    break;

	if (matched != 4)
	{
		OlVaDisplayWarningMsg(	(Display *) NULL,
					OleNfileOlgInit,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileOlgInit_msg2,
					fileName);
	    if (bestMatch)
		XtFree (bestMatch);
	    bestMatch = (char *) 0;
	    break;
	}

	newError = (hR - hRes) * (hR - hRes) + (vR - vRes) * (vR - vRes) +
	    (sc - scale) * (sc - scale);
	if (newError < error)
	{
	    error = newError;
	    if (bestMatch)
		XtFree (bestMatch);
	    bestMatch = (char *) XtMalloc (strlen (buf) + 1);
	    strcpy (bestMatch, buf);
	}
    }

    fclose (mapFile);

    /* Check for builtin keyword */
    if (bestMatch && strcmp (bestMatch, "builtin") == 0)
	bestMatch = (char *) 0;

    return bestMatch;
}


static void
badFile (name)
    char	*name;
{

	OlVaDisplayWarningMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg3,
				OleCOlToolkitWarning,
				OleMfileOlgInit_msg3,
				name);
}

/* Read a single description for the file.  Return True if hunky-dorey. */

static Boolean
readDesc (arcFile, name, pDesc)
    FILE	*arcFile;
    char	*name;
    _OlgDesc	*pDesc;
{
    register	i;
    unsigned	width, height;
    unsigned	start, end;
    unsigned char	*pData;

    if (fscanf (arcFile, "%u%u", &width, &height) != 2)
    {
	badFile (name);
	return False;
    }

    pDesc->width = width;
    pDesc->height = height;
    pDesc->pData = (unsigned char *) XtMalloc (height * 2);
    for (pData=pDesc->pData,i=0; i<height; i++)
    {
	if ((fscanf (arcFile, "%u%u", &start, &end) != 2) || end >= width)
	{
	    badFile (name);
	    fclose (arcFile);
	    return False;
	}

	*pData++ = start;
	*pData++ = end;
    }
    return True;
}

/* Read arc descriptions from file.  Return True if everything went OK. */

static Boolean
readArcs (pDev, fileName)
    register _OlgDevice	*pDev;
    char	*fileName;
{
    FILE	*arcFile;
    int		imajor, iminor;

    if (!(arcFile = fopen (fileName, "r")))
    {
	OlVaDisplayWarningMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg4,
				OleCOlToolkitWarning,
				OleMfileOlgInit_msg4,
				fileName);
	return False;
    }

    /* Read the version number.  For version 2.1 files, read the descriptions
     * for 2.1 objects and use defaults for the 4.1 objects.
     */
    if ((fscanf (arcFile, "%u%u", &imajor, &iminor) != 2) ||
	imajor != 2 || iminor != 1)
    {
	badFile (fileName);
	fclose (arcFile);
	return False;
    }

    /* Read the various arc descriptions */
    if (!readDesc (arcFile, fileName, &pDev->oblongB2UL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB2UR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB2LL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB2LR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB3UL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB3UR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB3LL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongB3LR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongDefUL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongDefUR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongDefLL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->oblongDefLR)) return False;

    if (!readDesc (arcFile, fileName, &pDev->rect2UL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect2UR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect2LL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect2LR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect3UL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect3UR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect3LL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->rect3LR)) return False;

    if (!readDesc (arcFile, fileName, &pDev->sliderVUL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderVUR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderVLL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderVLR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderHUL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderHUR)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderHLL)) return False;
    if (!readDesc (arcFile, fileName, &pDev->sliderHLR)) return False;

    if (!readDesc (arcFile, fileName, &pDev->arrowUp)) return False;
    if (!readDesc (arcFile, fileName, &pDev->arrowDown)) return False;
    if (!readDesc (arcFile, fileName, &pDev->arrowLeft)) return False;
    if (!readDesc (arcFile, fileName, &pDev->arrowRight)) return False;
    if (!readDesc (arcFile, fileName, &pDev->arrowText)) return False;

    /*  The next line should contain the next release number: 4.1  */
    if ((fscanf (arcFile, "%u%u", &imajor, &iminor) == 2) &&
	imajor == 4 && iminor == 1) {
    	/* Read the various arc descriptions */
    	if (!readDesc (arcFile, fileName, &pDev->arrowUpCenter)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowUpDark)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowUpBright)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowDownCenter)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowDownDark)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowDownBright)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowRightCenter)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowRightDark)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowRightBright)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowRightVert)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowLeftCenter)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowLeftDark)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowLeftBright)) return False;
    	if (!readDesc (arcFile, fileName, &pDev->arrowLeftVert)) return False;
    }
    else {
#define	ASSIGN_DFLT(mem,val)	((pDev->mem.pData ? \
				 XtFree ((char *)pDev->mem.pData), 0 : 0), \
				 pDev->mem = val)
	ASSIGN_DFLT (arrowUpCenter, dfltShArrowUpCenter); /* shadow arrows */
	ASSIGN_DFLT (arrowUpDark, dfltShArrowUpDark);
	ASSIGN_DFLT (arrowUpBright, dfltShArrowUpBright);
	ASSIGN_DFLT (arrowDownCenter, dfltShArrowDownCenter);
	ASSIGN_DFLT (arrowDownDark, dfltShArrowDownDark);
	ASSIGN_DFLT (arrowDownBright, dfltShArrowDownBright);
	ASSIGN_DFLT (arrowRightCenter, dfltShArrowRightCenter);
	ASSIGN_DFLT (arrowRightDark, dfltShArrowRightDark);
	ASSIGN_DFLT (arrowRightBright, dfltShArrowRightBright);
	ASSIGN_DFLT (arrowRightVert, dfltShArrowRightVert);
	ASSIGN_DFLT (arrowLeftCenter, dfltShArrowLeftCenter);
	ASSIGN_DFLT (arrowLeftDark, dfltShArrowLeftDark);
	ASSIGN_DFLT (arrowLeftBright, dfltShArrowLeftBright);
	ASSIGN_DFLT (arrowLeftVert, dfltShArrowLeftVert);
    }
    fclose (arcFile);
    return True;
}

/* Get the name of the file where the arc descriptions or bitmaps
 * live.  Return 0 if we should use the compiled-in versions; otherwise,
 * return a pointer to the path.  The path is derived by taking the device
 * resolution and user scale and looking up the best match in the scale
 * mapping file.  The base file name associated with the best match has
 * an extension added to get the name of the file containing the description.
 * The OlScaleMap resource specifies the name of the scale mapping file.
 * If null, a default file is used.
 */

static char *
getPath (pDev, ext)
    _OlgDevice	*pDev;
    char	*ext;
{
    char	*mapName;
    char	*fileName;
    unsigned	hRes, vRes;

    /* Check the defaults file to see if user wants to override the path */
    if (!pDev->descPath)
    {
	hRes = DPI (WidthOfScreen (pDev->scr), WidthMMOfScreen (pDev->scr));
	vRes = DPI (HeightOfScreen (pDev->scr), HeightMMOfScreen (pDev->scr));

	mapName = _OlGetScaleMap ();
	if (!mapName)
	{
	    /* no path has been specified.  If the resolution of the device
	     * is the same as the compiled in defaults, use them.
	     */
	    if (hRes == DFLT_H_DPI && vRes == DFLT_V_DPI &&
		pDev->scale == DFLT_SCALE)
		return (char *) 0;

	    mapName = DFLT_MAP;
	}

	pDev->descPath = readScaleMap (mapName, hRes, vRes, pDev->scale);
	if (!pDev->descPath)
	    return (char *) 0;
    }

    /* Make a copy of the description path and append the extension. */
    fileName = (char *) XtMalloc (strlen (pDev->descPath) + strlen (ext) + 1);
    strcpy (fileName, pDev->descPath);
    strcat (fileName, ext);
    return fileName;
}

/* Load the descriptions of curves used for a given screen and scale.  If
 * the resolution and scale correspondes to the compiled-in defaults, use
 * them. otherwise read the descriptions from a file.
 */

static void
OlgGetArcs (pDev)
    _OlgDevice	*pDev;
{
    char	*path;
    char	useDefaults;

    /* If we are using the defaults, and the scale and resolution happen
     * to correspond to the compiled in defaults, then simply use them.
     */
    if (!(path = getPath (pDev, ".arc")))
	useDefaults = True;
    else
    {
	/* Read the arcs from a file */
	if (readArcs (pDev, path))
	    useDefaults = False;
	else
	    useDefaults = True;		/* read failed */
	XtFree (path);
    }

    if (useDefaults)
    {
#define	ASSIGN_DFLT(mem,val)	((pDev->mem.pData ? \
				 XtFree ((char *)pDev->mem.pData), 0 : 0), \
				 pDev->mem = val)
	ASSIGN_DFLT (oblongB2UL, dfltOblongB2UL);
	ASSIGN_DFLT (oblongB2UR, dfltOblongB2UR);
	ASSIGN_DFLT (oblongB2LL, dfltOblongB2LL);
	ASSIGN_DFLT (oblongB2LR, dfltOblongB2LR);
	ASSIGN_DFLT (oblongB3UL, dfltOblongB3UL);	/* 3-D corners */
	ASSIGN_DFLT (oblongB3UR, dfltOblongB3UR);
	ASSIGN_DFLT (oblongB3LL, dfltOblongB3LL);
	ASSIGN_DFLT (oblongB3LR, dfltOblongB3LR);
	ASSIGN_DFLT (oblongDefUL, dfltOblongDefUL);	/* default ring */
	ASSIGN_DFLT (oblongDefUR, dfltOblongDefUR);	/* (2- and 3-D) */
	ASSIGN_DFLT (oblongDefLL, dfltOblongDefLL);
	ASSIGN_DFLT (oblongDefLR, dfltOblongDefLR);

	ASSIGN_DFLT (rect2UL, dfltRect2UL);	/* 2-D small radius corners */
	ASSIGN_DFLT (rect2UR, dfltRect2UR);
	ASSIGN_DFLT (rect2LL, dfltRect2LL);
	ASSIGN_DFLT (rect2LR, dfltRect2LR);
	ASSIGN_DFLT (rect3UL, dfltRect3UL);	/* 3-D small corners*/
	ASSIGN_DFLT (rect3UR, dfltRect3UR);
	ASSIGN_DFLT (rect3LL, dfltRect3LL);
	ASSIGN_DFLT (rect3LR, dfltRect3LR);

	ASSIGN_DFLT (sliderVUL, dfltSliderVUL);	/* vertical slider */
	ASSIGN_DFLT (sliderVUR, dfltSliderVUR);
	ASSIGN_DFLT (sliderVLL, dfltSliderVLL);
	ASSIGN_DFLT (sliderVLR, dfltSliderVLR);
	ASSIGN_DFLT (sliderHUL, dfltSliderHUL);	/* horizontal slider */
	ASSIGN_DFLT (sliderHUR, dfltSliderHUR);
	ASSIGN_DFLT (sliderHLL, dfltSliderHLL);
	ASSIGN_DFLT (sliderHLR, dfltSliderHLR);

	ASSIGN_DFLT (arrowUp, dfltArrowUp);	/* miscellaneous arrows */
	ASSIGN_DFLT (arrowDown, dfltArrowDown);
	ASSIGN_DFLT (arrowLeft, dfltArrowLeft);
	ASSIGN_DFLT (arrowRight, dfltArrowRight);
	ASSIGN_DFLT (arrowText, dfltArrowText);

	ASSIGN_DFLT (arrowUpCenter, dfltShArrowUpCenter); /* shadow arrows */
	ASSIGN_DFLT (arrowUpDark, dfltShArrowUpDark);
	ASSIGN_DFLT (arrowUpBright, dfltShArrowUpBright);
	ASSIGN_DFLT (arrowDownCenter, dfltShArrowDownCenter);
	ASSIGN_DFLT (arrowDownDark, dfltShArrowDownDark);
	ASSIGN_DFLT (arrowDownBright, dfltShArrowDownBright);
	ASSIGN_DFLT (arrowRightCenter, dfltShArrowRightCenter);
	ASSIGN_DFLT (arrowRightDark, dfltShArrowRightDark);
	ASSIGN_DFLT (arrowRightBright, dfltShArrowRightBright);
	ASSIGN_DFLT (arrowRightVert, dfltShArrowRightVert);
	ASSIGN_DFLT (arrowLeftCenter, dfltShArrowLeftCenter);
	ASSIGN_DFLT (arrowLeftDark, dfltShArrowLeftDark);
	ASSIGN_DFLT (arrowLeftBright, dfltShArrowLeftBright);
	ASSIGN_DFLT (arrowLeftVert, dfltShArrowLeftVert);
    }
}

/* Get Device Data
 *
 * Create a Device structure (if one for the requested resolution and scale
 * does not already exist) and initialize it.  The arc descriptions are
 * initialized immediately; bitmap initialization is deferred until needed.
 * Use compiled in versions where reasonable, otherwise, try to read the
 * arc descriptions from a file.  If the read fails, use the compiled-in
 * defaults.
 */

_OlgDevice *
_OlgGetDeviceData OLARGLIST((scr, scale))
    OLARG( Screen *,	scr)
    OLGRA( Dimension,	scale)
{
    register _OlgDevice	*pDev;		/* ptr to device structure */
    XGCValues	GCvalues;		/* initial gc values */
    XColor	grayColor;		/* medium gray */
    char	tmp;			/* garbage value */

    /* Check if the structure we need has already been allocated. */
    for (pDev=_OlgDeviceHead; pDev; pDev=pDev->next)
    {
	if (pDev->scr == scr && pDev->scale == scale)
	{
	    return pDev;
	}
    }

    /* Allocate new structure and put it at the head of the list. */
    pDev = (_OlgDevice *) XtCalloc (1, sizeof (_OlgDevice));
    pDev->next = _OlgDeviceHead;
    _OlgDeviceHead = pDev;

    /* Initialize all non-pixmap fields (well, OK, initialize
     * the stipples, too)
     */
    pDev->scr = scr;
    pDev->scale = scale;

    OlgGetStrokeWidth (pDev);

    pDev->busyStipple = XCreateBitmapFromData (DisplayOfScreen (scr),
	       RootWindowOfScreen (scr), busy_bits, busy_width, busy_height);

    pDev->inactiveStipple = XCreateBitmapFromData (DisplayOfScreen (scr),
	       RootWindowOfScreen (scr), (char *) inactive_bits,
	       inactive_width, inactive_height);

    if (!pDev->busyStipple || !pDev->inactiveStipple)
    {
	OlVaDisplayErrorMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg5,
				OleCOlToolkitError,
				OleMfileOlgInit_msg5);
    }

    grayColor.red = grayColor.green = grayColor.blue = 32768;
    grayColor.flags = DoRed | DoGreen | DoBlue;
    if (!XAllocColor (DisplayOfScreen (scr), DefaultColormapOfScreen (scr),
		      &grayColor))
    {
	OlVaDisplayErrorMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg6,
				OleCOlToolkitError,
				OleMfileOlgInit_msg6);
    }

    GCvalues.foreground = WhitePixelOfScreen (scr);
    pDev->whiteGC = XCreateGC (DisplayOfScreen (scr), RootWindowOfScreen (scr),
			       GCForeground, &GCvalues);

    GCvalues.stipple = pDev->inactiveStipple;
    GCvalues.fill_style = FillStippled;
    pDev->lightGrayGC = XCreateGC (DisplayOfScreen (scr),
				   RootWindowOfScreen (scr),
				   GCForeground|GCStipple|GCFillStyle,
				   &GCvalues);

    GCvalues.foreground = BlackPixelOfScreen (scr);
    pDev->dimGrayGC = XCreateGC (DisplayOfScreen (scr),
				 RootWindowOfScreen (scr),
				 GCForeground|GCStipple|GCFillStyle,
				 &GCvalues);

    GCvalues.stipple = pDev->busyStipple;
    pDev->busyGC = XCreateGC (DisplayOfScreen (scr), RootWindowOfScreen (scr),
			      GCForeground|GCStipple|GCFillStyle, &GCvalues);

    pDev->blackGC = XCreateGC (DisplayOfScreen (scr), RootWindowOfScreen (scr),
			       GCForeground, &GCvalues);

    GCvalues.foreground = grayColor.pixel;
    pDev->grayGC = XCreateGC (DisplayOfScreen (scr), RootWindowOfScreen (scr),
			      GCForeground, &GCvalues);

    pDev->scratchGC = XCreateGC (DisplayOfScreen (scr),
				 RootWindowOfScreen (scr),
				 0, &GCvalues);

    if (!pDev->whiteGC || !pDev->lightGrayGC || !pDev->dimGrayGC ||
	!pDev->busyGC || !pDev->blackGC || !pDev->grayGC || !pDev->scratchGC)
    {
	OlVaDisplayErrorMsg(	(Display *) NULL,
				OleNfileOlgInit,
				OleTmsg7,
				OleCOlToolkitError,
				OleMfileOlgInit_msg7);
    }

    OlgGetArcs (pDev);

    interiorPosition (pDev, &pDev->oblongDefUL, &pDev->oblongDefLR,
		      &tmp, &pDev->lblOrigY, &tmp, &pDev->lblCornerY);

    interiorPosition (pDev, &pDev->rect3UL, &pDev->rect3LR,
		      &pDev->rect3OrigX, &pDev->rect3OrigY,
		      &pDev->rect3CornerX, &pDev->rect3CornerY);

    interiorPosition (pDev, &pDev->rect2UL, &pDev->rect2LR,
		      &pDev->rect2OrigX, &pDev->rect2OrigY,
		      &pDev->rect2CornerX, &pDev->rect2CornerY);

    return pDev;
}

/* Get bitmaps
 *
 * Get bitmaps for things that are not drawn using line segments, like
 * the pushpins.  The bitmap read is one of the following:
 *
 *	OLG_PUSHPIN_2D
 *	OLG_PUSHPIN_3D
 *	OLG_CHECKS
 *
 * For a given type, all the variants of that type are stored in different
 * areas of the same bitmap.  For example, PUSHPIN_3D is a 3x3 matrix of
 * images stored in one bitmap.  The top row contains the pushed-in version
 * of the pin; the middle row contains the pulled-out version; and the bottom
 * row contains the default version.  For the three rows, the left image is
 * the light hightlights, the middle is the dark areas, and the right is the
 * interior.  The PUSHPIN_2D pushpin bitmap contains the 3 different pins:
 * in, out, and default.  The CHECKS bitmap contains a mask and the check.
 * Within a bitmap, each subimage is the same size.
 *
 * As with the arc descriptions, use compiled-in defaults where reasonable,
 * and read the bitmaps.
 */

void
_OlgGetBitmaps OLARGLIST((scr, pInfo, bitmapType))
    OLARG( Screen *,	scr)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlDefine,	bitmapType)
{
    register _OlgDevice	*pDev = pInfo->pDev;
    char	*path;
    char	*extension;
    Pixmap	*pBitmap;
    char	useDefaults;
    Dimension	*pWidth, *pHeight;
    unsigned	bmapWidth, bmapHeight;
    unsigned char	*defaultData;

    switch (bitmapType) {
    case OLG_PUSHPIN_2D:
	if (pDev->pushpin2D)
	    return;	/* we already loaded it */
	extension = ".p2";
	pBitmap = &pDev->pushpin2D;
	pWidth = &pDev->widthPushpin2D;
	pHeight = &pDev->heightPushpin2D;
	*pWidth = pushpin2d_width;
	*pHeight = pushpin2d_height;
	defaultData = pushpin2d_bits;
	break;

    case OLG_PUSHPIN_3D:
	if (pDev->pushpin3D)
	    return;	/* we already loaded it */
	extension = ".p3";
	pBitmap = &pDev->pushpin3D;
	pWidth = &pDev->widthPushpin3D;
	pHeight = &pDev->heightPushpin3D;
	*pWidth = pushpin3d_width;
	*pHeight = pushpin3d_height;
	defaultData = pushpin3d_bits;
	break;

    case OLG_CHECKS:
	if (pDev->checks)
	    return;	/* we already loaded it */
	extension = ".ck";
	pBitmap = &pDev->checks;
	pWidth = &pDev->widthChecks;
	pHeight = &pDev->heightChecks;
	*pWidth = checks_width;
	*pHeight = checks_height;
	defaultData = checks_bits;
	break;

    default:
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNbadFile,
					OleTbadBitmap,
					OleCOlToolkitError,
					OleMbadFile_badBitmap,
					"",
					"OlgInit",
					"_OlgGetBitmaps");
    }

    /* If we are using the defaults, and the scale and resolution happen
     * to correspond to the compiled in defaults, then simply use them.
     */
    if (!(path = getPath (pDev, extension)))
	useDefaults = True;
    else
    {
	/* Read the bitmap from a file.  First construct the complete name. */
	int	fake;

	if (XReadBitmapFile (DisplayOfScreen (scr), RootWindowOfScreen (scr),
			     path, &bmapWidth, &bmapHeight, pBitmap,
			     &fake, &fake) == BitmapSuccess)
	{
	    *pWidth = bmapWidth;
	    *pHeight = bmapHeight;
	    useDefaults = False;
	}
	else
	    useDefaults = True;		/* read failed */
	XtFree (path);
    }

    if (useDefaults)
	*pBitmap = XCreateBitmapFromData (DisplayOfScreen (scr),
					  RootWindowOfScreen (scr),
					  (char *) defaultData,
					  *pWidth, *pHeight);
}

void (*_olmOlgSizeScrollbarElevator) OL_ARGS((
	Widget, OlgAttrs *, OlDefine, Dimension *, Dimension *));

void (*_olmOlgSizeScrollbarAnchor) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension *));

void (*_olmOlgUpdateScrollbar) OL_ARGS((Widget, OlgAttrs *, OlBitMask));

void (*_olmOlgDrawScrollbar) OL_ARGS((Widget, OlgAttrs *));

void (*_olmOlgDrawSlider) OL_ARGS((Widget, OlgAttrs *));

void (*_olmOlgUpdateSlider) OL_ARGS((Widget,OlgAttrs *, OlBitMask));

void (*_olmOlgSizeSliderElevator) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));

void (*_olmOlgSizeSlider) OL_ARGS((
	Widget, OlgAttrs *, Dimension *, Dimension * ));

void (*_olmOlgDrawAbbrevMenuB) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));

void (*_olmOlgDrawCheckBox) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, OlBitMask));

void (*_olmOlgDrawOblongButton) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, XtPointer, OlgLabelProc, OlBitMask));

Dimension (*_olmOlgDrawMenuMark) OL_ARGS((
	Screen *, Drawable, OlgAttrs *, Position, Position, Dimension,
	Dimension, OlBitMask));

void
_OlResolveOlgGUISymbol OL_NO_ARGS()
{
	OLRESOLVESTART
	OLRESOLVE(OlgDrawCheckBox,		_olmOlgDrawCheckBox)
	OLRESOLVE(OlgDrawMenuMark,		_olmOlgDrawMenuMark)
	OLRESOLVE(OlgDrawOblongButton,		_olmOlgDrawOblongButton)
	OLRESOLVE(OlgSizeScrollbarElevator,	_olmOlgSizeScrollbarElevator)
	OLRESOLVE(OlgSizeScrollbarAnchor,	_olmOlgSizeScrollbarAnchor)
	OLRESOLVE(OlgUpdateScrollbar,		_olmOlgUpdateScrollbar)
	OLRESOLVE(OlgDrawScrollbar,		_olmOlgDrawScrollbar)
	OLRESOLVE(OlgDrawSlider,		_olmOlgDrawSlider)
	OLRESOLVE(OlgUpdateSlider,		_olmOlgUpdateSlider)
	OLRESOLVE(OlgSizeSliderElevator,	_olmOlgSizeSliderElevator)
	OLRESOLVE(OlgSizeSlider,		_olmOlgSizeSlider)
	OLRESOLVEEND(OlgDrawAbbrevMenuB,	_olmOlgDrawAbbrevMenuB)

}  /* end of _OlResolveOlgGUISymbol() */
