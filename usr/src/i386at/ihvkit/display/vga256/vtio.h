#ident	"@(#)ihvkit:display/vga256/vtio.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#include "vtdefs.h"

#define BYTE	unsigned char
#define	SUCCESS	1
#define	FAIL	0

/*
 * structure used to initialize the V256 registers
 */
struct v256_regs 
{
	unsigned char	seqtab[NSEQ];
	unsigned char	miscreg;
	struct egainit	egatab;
};


/*
 * General adapter information such as size of display, pixels per inch, etc.
 */
/*
 * structure common for all modes
 */
typedef struct _DispM {
	int	mode;		/* mode number for this entry */
	char	*entry;		/* entry */
	char	*monitor;	/* type of monitor */
	int	x;		/* X resolution */
	int	y;		/* Y resolution */
	struct v256_regs *regs;	/* data for std registers for this mode */
#ifdef NOTNOW
	int     Flags;
  	int	Clock; 		/* doclock */
	int     HDisplay;  	/* horizontal timing */
	int     HSyncStart;
	int     HSyncEnd;
	int     HTotal;
	int     VDisplay;	/* vertical timing */
	int     VSyncStart;
	int     VSyncEnd;
	int     VTotal;
#endif
} DisplayModeRec, *DisplayModePtr;

/*
 * the graphic device
 */
typedef struct {
	char	*vendor;	/* vendor - filled up by Init() */
	char 	*chipset;	/* chipset - filled up by Init() */
	int  	videoRam;  /* video RAM, default: 1MB - filled up by Init() */
	int	virtualX,virtualY;	/* virtual X, Y - filled up by Init() */
	int  	dispX, dispY;	/* display X, Y - filled up by Init() */
	int  	depth;		/* frame buffer depth */
	BYTE	*vt_buf;	/* virtual address of screen memory */
	int	map_size;	/* size of one plane of memory */
	int	ad_addr;	/* base register address for adapter */
	SIBool	is_color;	/* color / monochrome monitor */
	int  	width, height;  /* monitor width and height */
	int	(* Probe)();	/* checks for type of chip set and memory */
	int	(* SetMode)();	/* checks for valid mode and set the mode num */
	int	(* VtInit)();	/* vt init - kd specific; called only once */
	int	(* Init)();	/* mode specific init - also called at vt switch */
	int     (* Restore)();	/* restore from the previous mode */
	int	(* SelectReadPage)();	/* select read page */
	int	(* SelectWritePage)();	/* select write page */
	int	(* AdjustFrame)();	/* adjust the frame to display area */
	int	(* SwitchMode)();	/* switch from current mode to the requested mode */
	int	(* PrintIdent)();	/* print any vendor info */
	DisplayModePtr pCurrentMode; 	/* ptr to the current mode */
	DisplayModePtr pmodedata; 	/* ptr to the mode data array, ie: all modes */
	int (* HWStatus)();		/* status of HW accelerator, ie: pending or ready*/
} ScrInfoRec;


#define ErrorF printf
