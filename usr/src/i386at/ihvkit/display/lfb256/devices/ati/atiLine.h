#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiLine.h	1.1"

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/


#define SETUP_BRES(x1, y1, x2, y2, __cmd)	\
						\
{						\
    register int dx, dy;			\
    register int min_d, max_d;			\
    register int i;				\
						\
    dx = x2 - x1;				\
    dy = y2 - y1;				\
						\
    if (dx > 0)					\
	__cmd |= 0x20;				\
    else {					\
	dx = -dx;				\
    }						\
						\
    if (dy > 0)					\
	__cmd |= 0x80;				\
    else					\
	dy = -dy;				\
						\
    if (dx >= dy) {				\
	min_d = dy;				\
	max_d = dx;				\
    }						\
    else {					\
	min_d = dx;				\
	max_d = dy;				\
	__cmd |= 0x40;				\
    }						\
						\
    ATI_NEED_FIFO(4);				\
						\
    i = 2*min_d - max_d;			\
    if (x1 < x2)				\
	i--;					\
    outw(ERR_TERM, i & 0x1fff);			\
						\
    i = 2 * (min_d - max_d);			\
    outw(DIASTP, i & 0x1fff);			\
						\
    i = 2 * min_d;				\
    outw(AXSTP, i & 0x1fff);			\
						\
    outw(MAJ_AXIS_PCNT, max_d);			\
}

#define DRAW_LINE(x1, y1, x2, y2, cmd)		\
						\
{						\
    ATI_NEED_FIFO(2);				\
						\
    outw(CUR_X, x1);				\
    outw(CUR_Y, y1);				\
    						\
    if (y1 == y2) {				\
	ATI_NEED_FIFO(2);			\
	if (x2 > x1) {				\
	    outw(MAJ_AXIS_PCNT, x2 - x1);	\
	    outw(CMD, cmd | 0x08);		\
	}					\
	else {					\
	    outw(MAJ_AXIS_PCNT, x1 - x2);	\
	    outw(CMD, cmd | 0x88);		\
	}					\
    }						\
    else if (x1 == x2) {			\
	ATI_NEED_FIFO(2);			\
	if (y2 > y1) {				\
	    outw(MAJ_AXIS_PCNT, y2 - y1);	\
	    outw(CMD, cmd | 0xc8);		\
	}					\
	else {					\
	    outw(MAJ_AXIS_PCNT, y1 - y2);	\
	    outw(CMD, cmd | 0x48);		\
	}					\
    }						\
    else {					\
	int _cmd = cmd;				\
						\
	SETUP_BRES(x1, y1, x2, y2, _cmd);	\
						\
	ATI_NEED_FIFO(1);			\
	outw(CMD, _cmd);			\
    }						\
}
