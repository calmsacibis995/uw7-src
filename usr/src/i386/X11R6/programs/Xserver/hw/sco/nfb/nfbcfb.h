/*
 *	@(#) nfbcfb.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Sun Oct 11 13:40:58 PDT 1992	mikep@sco.com
 *	- Created file
 *	S001	Sat Dec 05 06:09:02 PST 1992	buckm@sco.com
 *	- Add cfb{16,32}CopyArea.
 */


extern void cfb16ValidateGC();
extern void cfb16ZeroPolyArcSS8Copy(), cfb16ZeroPolyArcSS8Xor();
extern void cfb16ZeroPolyArcSS8General();
extern void cfb16LineSS(), cfb16LineSD(), cfb16SegmentSS(), cfb16SegmentSD();
extern void cfb168LineSS1Rect(), cfb168SegmentSS1Rect();
extern RegionPtr cfb16CopyPlane(), cfb16CopyArea();
extern void cfb16PolyFillArcSolidCopy(),cfb16PolyFillArcSolidXor();
extern void cfb16PolyFillArcSolidGeneral();
extern void cfb16FillPoly1RectCopy(), cfb16FillPoly1RectGeneral();

extern void cfb16SetSpans();
extern void cfb16GetSpans();
extern void cfb16SolidSpansCopy(), cfb16SolidSpansXor(), cfb16SolidSpansGeneral();
extern void cfb16UnnaturalTileFS();
extern void cfb16UnnaturalStippleFS();
extern void cfb16TileFSCopy(), cfb16TileFSGeneral();

extern void cfb16TEGlyphBlt();
extern void cfb16TEGlyphBlt8();
extern void cfb16PolyGlyphBlt8();
extern void cfb16PolyGlyphRop8();
extern void cfb16ImageGlyphBlt8();

extern void cfb16PutImage();
extern void cfb16GetImage();
extern void cfb16PolyPoint();

extern void cfb16CopyRotatePixmap();
extern void cfb16YRotatePixmap();
extern void cfb16XRotatePixmap();
extern void cfb16PadPixmap();

extern void cfb32ValidateGC();
extern void cfb32ZeroPolyArcSS8Copy(), cfb32ZeroPolyArcSS8Xor();
extern void cfb32ZeroPolyArcSS8General();
extern void cfb32LineSS(), cfb32LineSD(), cfb32SegmentSS(), cfb32SegmentSD();
extern RegionPtr cfb32CopyPlane(), cfb32CopyArea();
extern void cfb32PolyFillArcSolidCopy(),cfb32PolyFillArcSolidXor();
extern void cfb32PolyFillArcSolidGeneral();
extern void cfb32FillPoly1RectCopy(), cfb32FillPoly1RectGeneral();

extern void cfb32SetSpans();
extern void cfb32GetSpans();
extern void cfb32SolidSpansCopy(), cfb32SolidSpansXor(), cfb32SolidSpansGeneral();
extern void cfb32UnnaturalTileFS();
extern void cfb32UnnaturalStippleFS();
extern void cfb32TileFSCopy(), cfb32TileFSGeneral();

extern void cfb32TEGlyphBlt();
extern void cfb32TEGlyphBlt8();
extern void cfb32PolyGlyphBlt8();
extern void cfb32PolyGlyphRop8();
extern void cfb32ImageGlyphBlt8();

extern void cfb32PutImage();
extern void cfb32GetImage();
extern void cfb32PolyPoint();

extern void cfb32CopyRotatePixmap();
extern void cfb32YRotatePixmap();
extern void cfb32XRotatePixmap();
extern void cfb32PadPixmap();

