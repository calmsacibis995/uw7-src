#ident	"@(#)Space.c	1.12"
#ident	"$Header$"


#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/immu.h>
#include <sys/proc.h>
#include <sys/kd.h>
#include <sys/kd_cgi.h>


stridx_t	kdstrmap;	/* Indices into string buffer */

/*
 * use this array to add configurable array of io port addresses 
 */
ushort	kdconfiotab[MKDCONFADDR];

/*
 * set "kdioaddrcnt" to count of new io port array elements being added
 * NOTE - new io address count CANNOT exceed MKDCONFADDR 
 */
int	kdioaddrcnt = 0;

/*
 * use this array to add configurable video memory array of
 * start and end address pairs. Note, array count CANNOT exceed MKDCONFADDR
 * address pairs
 */
struct kd_range kdvmemtab[MKDCONFADDR];

/*
 * set "kdvmemcnt" to count of new video memory array elements being added
 * NOTE - new video memory array count CANNOT exceed MKDCONFADDR
 */
#ifdef	EVC
int	kdvmemcnt = 1;
#else
int	kdvmemcnt = 0;
#endif

/*
 * This variable causes kd to assume that the graphics board is
 * a specific type.  Used to override the special port testing
 * for board identification.
 */
int AssumeVDCType = 0;	/* edited by install scripts -- do not change format */

struct portrange vidc_HGAports[] = {
	{0x3b4, 2}, {0x3b8, 8}, {0,0},
};
struct portrange vidc_CGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_EGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_VGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_MCGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_SVGAports[] = {
	{0x3bf, 1}, {0x3c0, 3}, {0x3c4, 3},
	{0x3ca, 1}, {0x3cc, 4}, {0x3d4, 2},
	{0x3d8, 1}, {0x3da, 1}, {0,0},
};
struct portrange vidc_ATIVGAports[] = {
	{0x1ce, 2},
	{0x3c0, 3}, {0x3c4, 2}, {0x3ca, 1},
	{0x3cc, 1}, {0x3ce, 2}, {0x3d4, 2},
	{0x3da, 1}, {0,0},
};
struct portrange vidc_VEGAports[] = {
	{0x3c0, 3}, {0x3c4, 2}, {0x3c8, 2},
	{0x3ca, 1}, {0x3cc, 1}, {0x3ce, 2},
	{0x3d4, 2}, {0x3da, 1}, {0,0},
};
struct portrange vidc_PLASMA386ports[] = {
	{0x3d4, 2}, {0x3d8, 2}, {0,0},		/* {0x23c6, 1},	*/
};
struct portrange vidc_GRXports[] = {
	{0x2b0, 2}, {0x2b2, 2}, {0x2b4, 2}, {0x2b6, 2},
	{0x2b8, 1}, {0x2b9, 1}, {0x2ba, 1}, {0x2bb, 1},
        {0,0},	
};
struct portrange vidc_AG1024ports[] = {
	{0x290, 2}, {0x292, 2}, {0x294, 2}, {0x296, 2},
	{0x298, 1}, {0x299, 1}, {0x29a, 1}, {0x29b, 1},
        {0,0},	
};

/*
 * The Metagraphic's Metawindow library expects the name field   
 * (first element) of each vidclass structure to be in UPPERCASE.
 */
struct cgi_class cgi_classlist[] = {
	{	"HGA",	"HGA",
		0xb0000, 0x10000,
		vidc_HGAports,
	},
	{	"CGA",	"CGA",
		0xb8000, 0x8000,
		vidc_CGAports,
	},
	{	"EGA",	"EGA",
		0xa0000, 0x10000,
		vidc_EGAports,
	},
	{	"VGA",	"VGA",
		0xa0000, 0x10000,
		vidc_VGAports,
	},
	{	"MCGA",	"MCGA",
		0xa0000, 0x10000,
		vidc_MCGAports,
	},
	{	"SVGA",	"Super VGA",
		0xa0000, 0x20000,
		vidc_SVGAports,
	},
	{	"ATIVGA", "ATI VGA Wonder",
		0xa0000, 0x10000,
		vidc_ATIVGAports,
	},
	{	"VEGA",	"Video-7 VEGA",
		0xa0000, 0x10000,
		vidc_VEGAports,
	},
	{	"PLASMA386",	"Compaq 386 Plasma",
		0xa0000, 0x10000,
		vidc_VEGAports,
	},
	{	"HP82328",	"HP 82328 IGC",
		0xcc000, 0x2000,
		vidc_VEGAports,
	},
	{	"AG1024",	"Compaq AG1024",
		0xa0000, 0x10000,
		vidc_AG1024ports,
	},
	{	"GRX",		"Renaissance GRX Rendition II",
		0xa0000, 0x10000,
		vidc_GRXports,
	},
	{ 0 }
};

unchar		kd_typematic = TYPE_VALS;
boolean_t	kd_no_activate = B_TRUE;

