#ident	"@(#)ihvkit:display/lfb256/lfbCompat.c	1.1"

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <lfb.h>
#include <stdlib.h>

static SIBool (*oneBitLine)(), (*oneBitSegment)(), (*fillRect)();

SIBool lfbOldInit(int fd,
		  SIConfigP cfgP,
		  SIInfoP infoP,
		  ScreenInterface **funcs);
SIBool lfbOldOneBitLine(SIint32 cnt, SIPointP ptsIn);
SIBool lfbOldOneBitSegment(SIint32 cnt, SIPointP ptsIn);
SIBool lfbOldFillRect(SIint32 cnt, SIRectP pRects);

static ScreenInterface oldLfbFuncs= {
    lfbOldInit,			/* This is the only entry we need */
};
ScreenInterface *DisplayFuncs = &oldLfbFuncs;

SIBool lfbOldInit(int fd,
		  SIConfigP cfgP,
		  SIInfoP infoP,
		  ScreenInterface **funcs)

{
    SIScreenRec screen;
    SIConfig cfg;
    SIInfo info = {0};
    char chipset[25] = "", wgt[10] = "";
    float mon_w, mon_h;
    int w, h, depth, freq;

    cfg.resource = cfgP->resource;
    cfg.class = cfgP->class;
    cfg.visual_type = cfgP->visual_type;
    cfg.info = cfgP->info;
    cfg.display = cfgP->display;
    cfg.displaynum = cfgP->displaynum;
    cfg.screen = cfgP->screen;
    cfg.device = cfgP->device;

    sscanf(cfg.info, "%s %dx%d %d %d %fx%f %s",
	   &chipset, &w, &h, &depth, &freq, &mon_w, &mon_h, &wgt);

    cfg.chipset = chipset;
    cfg.videoRam = 0;
    cfg.model = NULL;
    cfg.vendor_lib = NULL;
    cfg.virt_w = cfg.disp_w = w;
    cfg.virt_h = cfg.disp_h = h;
    cfg.depth = depth;
    cfg.monitor_info.model = NULL;
    cfg.monitor_info.width = mon_w;
    cfg.monitor_info.height = mon_h;
    cfg.monitor_info.hfreq = 0.0;
    cfg.monitor_info.vfreq = freq;
    cfg.monitor_info.otherinfo = NULL;
    cfg.info2vendorlib = wgt;
    cfg.IdentString = NULL;
    cfg.priv = NULL;

    *funcs = DisplayFuncs;

    screen.cfgPtr = &cfg;
    screen.flagsPtr = &info;
    screen.funcsPtr = DisplayFuncs;

    if (!DM_InitFunction(fd, &screen))
	return(SI_FAIL);

    infoP->SIserver_version = info.SIserver_version;
    infoP->SIdm_version = info.SIdm_version;
    infoP->SIlinelen = info.SIlinelen;
    infoP->SIlinecnt = info.SIlinecnt;
    infoP->SIxppin = info.SIxppin;
    infoP->SIyppin = info.SIyppin;
    infoP->SIstatecnt = info.SIstatecnt;
    infoP->SIavail_bitblt = info.SIavail_bitblt;
    infoP->SIavail_stplblt = info.SIavail_stplblt;
    infoP->SIavail_fpoly = info.SIavail_fpoly;
    infoP->SIavail_point = info.SIavail_point;
    infoP->SIavail_line = info.SIavail_line;
    infoP->SIavail_drawarc = info.SIavail_drawarc;
    infoP->SIavail_fillarc = info.SIavail_fillarc;
    infoP->SIavail_font = info.SIavail_font;
    infoP->SIavail_spans = info.SIavail_spans;
    infoP->SIavail_memcache = info.SIavail_memcache;
    infoP->SIcursortype = info.SIcursortype;
    infoP->SIcurscnt = info.SIcurscnt;
    infoP->SIcurswidth = info.SIcurswidth;
    infoP->SIcursheight = info.SIcursheight;
    infoP->SIcursmask = info.SIcursmask;
    infoP->SItilewidth = info.SItilewidth;
    infoP->SItileheight = info.SItileheight;
    infoP->SIstipplewidth = info.SIstipplewidth;
    infoP->SIstippleheight = info.SIstippleheight;
    infoP->SIfontcnt = info.SIfontcnt;
    infoP->SIvisuals = info.SIvisuals;
    infoP->SIvisualCNT = info.SIvisualCNT;
    infoP->SIavail_exten = info.SIavail_exten;

    oneBitLine = (*funcs)->si_line_onebitline;
    (*funcs)->si_line_onebitline = lfbOldOneBitLine;

    oneBitSegment = (*funcs)->si_line_onebitseg;
    (*funcs)->si_line_onebitseg = lfbOldOneBitSegment;

    fillRect = (*funcs)->si_poly_fillrect;
    (*funcs)->si_poly_fillrect = lfbOldFillRect;

    return(SI_SUCCEED);
}

SIBool lfbOldOneBitLine(SIint32 cnt, SIPointP ptsIn)

{
    static SIPointP pts = NULL;
    static ptsCnt = 0;
    register int i;

    if (! oneBitLine)
	return(SI_FAIL);

    /* 
     * SIPoint and SIPointRec are the same thing.  SIPointRec is
     * declared in terms of short and SIPoint is declared in terms of
     * SIint16, which is really a short.
     *
     * Unfortunately, there is no guarantee that the two types will 
     * remain the same size, so we must test.  Fortuantely, a half 
     * decent compiler can optimize out this test.
     *
     */

    if (sizeof(SIPoint) != sizeof(SIPoint)) {
	if (ptsCnt < cnt) {
	    ptsCnt = 2 * cnt;
	    pts = realloc(pts, ptsCnt * sizeof(SIPoint));
	    if (! pts) {
		ptsCnt = 0;
		return(SI_FAIL);
	    }
	}

	for (i = 0; i < cnt; i++) {
	    pts[i].x = ptsIn[i].x;
	    pts[i].y = ptsIn[i].y;
	}
    }
    else
	pts = (SIPointP)ptsIn;

    return((*oneBitLine)(0, 0, cnt, pts, SI_FALSE, SICoordModeOrigin));
}

SIBool lfbOldOneBitSegment(SIint32 cnt, SIPointP ptsIn)

{
    static SISegmentP segs = NULL;
    static segsCnt = 0;
    register int i, c = cnt >> 1;

    if (! oneBitSegment)
	return(SI_FAIL);

    if (cnt & 1) {
	fprintf(stderr, "oldOneBitSegment: Odd number of endpoints\n");
	return(SI_FAIL);
    }

    if (segsCnt < c) {
	segsCnt = cnt * c;
	segs = realloc(segs, segsCnt * sizeof(SISegment));
	if (! segs) {
	    segsCnt = 0;
	    return(SI_FAIL);
	}
    }

    for (i = 0; i < c; i++) {
	segs[i].x1 = ptsIn[i+i].x;
	segs[i].y1 = ptsIn[i+i].y;
	segs[i].x2 = ptsIn[i+i+1].x;
	segs[i].y2 = ptsIn[i+i+1].y;
    }

    return((*oneBitSegment)(0, 0, c, segs, SI_FALSE));
}

SIBool lfbOldFillRect(SIint32 cnt, SIRectP pRects)

{
    static SIRectOutlineP rcts = NULL;
    static rctsCnt = 0;
    register int i;

    if (! fillRect)
	return(SI_FAIL);

    if (rctsCnt < cnt) {
	rctsCnt = 2 * cnt;
	rcts = realloc(rcts, rctsCnt * sizeof(SIRectOutline));
	if (! rcts) {
	    rctsCnt = 0;
	    return(SI_FAIL);
	}
    }

    for (i = 0; i < cnt; i++) {
	if (pRects[i].ul.x < pRects[i].lr.x) {
	    rcts[i].x = pRects[i].ul.x;
	    rcts[i].width = pRects[i].lr.x - pRects[i].ul.x;
	}
	else {
	    rcts[i].x = pRects[i].lr.x;
	    rcts[i].width = pRects[i].ul.x - pRects[i].lr.x;
	}

	if (pRects[i].ul.y < pRects[i].lr.y) {
	    rcts[i].y = pRects[i].ul.y;
	    rcts[i].height = pRects[i].lr.y - pRects[i].ul.y;
	}
	else {
	    rcts[i].y = pRects[i].lr.y;
	    rcts[i].height = pRects[i].ul.y - pRects[i].lr.y;
	}
    }

    return((*fillRect)(0, 0, cnt, rcts));
}
