#ident	"@(#)ihvkit:display/vga256/v256spreq.h	1.1"

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

#ifndef _V256_SPREQ_H_

#define _V256_SPREQ_H_

#define MAX_VGA_PAGES 32 

/*
 *	VGA_PAGE_SIZE must be known at compile time
 */
#ifndef	VGA_PAGE_SIZE
#error	VGA_PAGE_SIZE unknown !
#endif	/* VGA_PAGE_SIZE */
/*
 *	The structure of a rectangular VGA region fits in ONE VGA page.
 */
typedef struct _Vga_Rect_Region
{

	int		x,y;
	int		width,height;
	BYTE 	*region_p;

} VgaRegion;

#define OFFSET(a,b) 	((a)+((b)*v256_slbytes))

/*
 *	Per page information
 */
struct	_v256_page_info 
{
    /*
     *	Starting screen coords of page
     */
	int	x_start;	
	int	y_start;
    /*
     *	offset of the page beginning from the beginning of vga memory
     */
	long 	page_offset;
};

/*
 *	prototype for the splitter setup
 */
extern int	
v256_setup_splitter(int x_resolution, int y_resolution, int page_size);

/*
 * prototype for the split request function
 */

extern 	int
v256_split_request( int	x_top_left, int	y_top_left, int	x_bottom_right,
	int	y_bottom_right, int	*n_rects_p, VgaRegion *rects_p);

#endif /* _V256_SPREQ_H_ */
