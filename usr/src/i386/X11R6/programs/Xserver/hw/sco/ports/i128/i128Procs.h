/*
 * @(#) i128Procs.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

/*
 * i128Procs.h
 *
 * Routines for the "i128" port
 */

#include "i128Defs.h"

/* i128Cmap.c */
extern void
i128SetColor(
             unsigned int cmap,
             unsigned int index,
             unsigned short r,
             unsigned short g,
             unsigned short b,
             ScreenPtr pScreen);

extern void
i128LoadColormap(
                 ColormapPtr pmap );

/* i128Cursor.c */
extern void
i128InstallCursor(
                  unsigned long int *image,
                  unsigned int hotx,
                  unsigned int hoty,
                  ScreenPtr pScreen);

extern void
i128SetCursorPos(
                 unsigned int x,
                 unsigned int y,
                 ScreenPtr pScreen);

extern void
i128CursorOn(
             int on,
             ScreenPtr pScreen);

extern void
i128SetCursorColor(
                   unsigned short fr,
                   unsigned short fg,
                   unsigned short fb,
                   unsigned short br,
                   unsigned short bg,
                   unsigned short bb,
                   ScreenPtr pScreen);

/* i128GC.c */
extern void
i128ValidateWindowGC(
                     GCPtr pGC,
                     Mask changes,
                     DrawablePtr pDraw);

/* i128Image.c */
extern void
i128ReadImage(
              BoxPtr pbox,
              void *imagearg,
              unsigned int stride,
              DrawablePtr pDraw );

extern void
i128DrawImage(
              BoxPtr pbox,
              void *imagearg,
              unsigned int stride,
              unsigned char alu,
              unsigned long planemask,
              DrawablePtr pDraw);

/* i128Init.c */
extern Bool
i128Setup();

extern Bool
i128Init(
         int index,
         struct _Screen * pScreen,
         int argc,
         char **argv );

extern void
i128CloseScreen(
                int index,
                ScreenPtr pScreen);

int
i128EnableMemory(
                 ScreenPtr pScreen);


/* i128Screen.c */
extern Bool
i128BlankScreen(
                int on,
                ScreenPtr pScreen);

extern void
i128SetGraphics(
                ScreenPtr pScreen );

extern void
i128SetText(
            ScreenPtr pScreen );

extern void
i128SaveGState(
               ScreenPtr pScreen );

extern void
i128RestoreGState(
                  ScreenPtr pScreen );

extern void
i128SetMode(
            ScreenPtr pScreen,
            struct Blackbird_Mode *mode,
            int flags);

/* i128Win.c */
extern void
i128ValidateWindowPriv(
                       struct _Window * pWin );


/* i128RectOps.c */
extern void
i128CopyRect(
             BoxPtr pdstBox,
             DDXPointPtr psrc,
             unsigned char alu,
             unsigned long planemask,
             DrawablePtr pDrawable );

extern void
i128DrawPoints(
               DDXPointPtr ppt,
               unsigned int npts,
               unsigned long fg,
               unsigned char alu,
               unsigned long planemask,
               DrawablePtr pDrawable );

extern void
i128DrawSolidRects(
                   BoxPtr pbox,
                   unsigned int nbox,
                   unsigned long fg,
                   unsigned char alu,
                   unsigned long planemask,
                   DrawablePtr pDrawable );

extern void
i128TileRects(
              BoxPtr pbox,
              unsigned int nbox,
              void *tile_data,
              unsigned int stride,
              unsigned int w,
              unsigned int h,
              DDXPointPtr patOrg,
              unsigned char alu,
              unsigned long planemask,
              DrawablePtr pDrawable );

/* i128Mono.c */
extern void
i128DrawMonoImage(
                  BoxPtr pbox,
                  void *image_data,
                  unsigned int startx,
                  unsigned int stride,
                  unsigned long fg,
                  unsigned char alu,
                  unsigned long planemask,
                  DrawablePtr pDrawable );

extern void
i128DrawOpaqueMonoImage(
                        BoxPtr pbox,
                        void *image_data,
                        unsigned int startx,
                        unsigned int stride,
                        unsigned long fg,
                        unsigned long bg,
                        unsigned char alu,
                        unsigned long planemask,
                        DrawablePtr pDrawable );


/* i128FillSp.c */
extern void
i128SolidFS(
            GCPtr pGC,
            DrawablePtr pDraw,
            DDXPointPtr ppt,
            unsigned int *pwidth,
            unsigned n ) ;

extern void
i128TiledFS(
            GCPtr pGC,
            DrawablePtr pDraw,
            DDXPointPtr ppt,
            unsigned int *pwidth,
            unsigned int n ) ;

extern void
i128StippledFS(
               GCPtr pGC,
               DrawablePtr pDraw,
               DDXPointPtr ppt,
               unsigned int *pwidth,
               unsigned int n ) ;

extern void
i128OpStippledFS(
                 GCPtr pGC,
                 DrawablePtr pDraw,
                 DDXPointPtr ppt,
                 unsigned int *pwidth,
                 unsigned n ) ;

/* i128Rect.c */
extern void
i128SolidFillRects(
                   GCPtr pGC,
                   DrawablePtr pDraw,
                   BoxPtr pbox,
                   unsigned int nbox ) ;

extern void
i128TiledFillRects(
                   GCPtr pGC,
                   DrawablePtr pDraw,
                   BoxPtr pbox,
                   unsigned int nbox ) ;

extern void
i128StippledFillRects(
                      GCPtr pGC,
                      DrawablePtr pDraw,
                      BoxPtr pbox,
                      unsigned int nbox ) ;

extern void
i128OpStippledFillRects(
                        GCPtr pGC,
                        DrawablePtr pDraw,
                        BoxPtr pbox,
                        unsigned int nbox ) ;

/* i128Line.c */
extern void
i128SolidZeroSegs(
                  GCPtr pGC,
                  DrawablePtr pDrawable,
                  BresLinePtr plines,
                  int nlines);

extern void
i128SolidZeroSeg(
                 GCPtr pGC,
                 DrawablePtr pDraw,
                 int signdx,
                 int signdy,
                 int axis,
                 int x1,
                 int y1,
                 int e,
                 int e1,
                 int e2,
                 int len ) ;


extern void
i128PolyZeroPtPtSegs (GC *gc,
                      DrawablePtr pDraw,
                      BoxRec *line_vals,
                      int nlines);


/* i128Glyph.c */

extern void i128DrawMonoGlyphs(
                               nfbGlyphInfo *glyph_info,
                               unsigned int nglyphs,
                               unsigned long fg,
                               unsigned char alu,
                               unsigned long planemask,
                               nfbFontPSPtr pPS,
                               DrawablePtr pDrawable );

/* i128Text.c */
extern void
i128DrawFontText(
                 BoxPtr pbox,
                 unsigned char *chars,
                 unsigned int count,
                 nfbFontPSPtr pPS,
                 unsigned long fg,
                 unsigned long bg,
                 unsigned char alu,
                 unsigned long planemask,
                 unsigned char transparent,
                 DrawablePtr pDraw );

extern void
i128DrawMonoGlyphs(
                   nfbGlyphInfo *glyph_info,
                   unsigned int nglyphs,
                   unsigned long fg,
                   unsigned char alu,
                   unsigned long planemask,
                   nfbFontPSPtr pPS,
                   DrawablePtr pDrawable);

/*  i128Clip.c */
extern void
i128SetClipRegions(
                   BoxRec *pbox,
                   int nbox,
                   DrawablePtr pDraw);


/* i128Pixmap.c */
extern PixmapPtr
i128CreatePixmap (
                  ScreenPtr pScreen,
                  unsigned int width,
                  unsigned int height,
                  unsigned int depth);

extern Bool
i128DestroyPixmap (PixmapPtr pPixmap);
