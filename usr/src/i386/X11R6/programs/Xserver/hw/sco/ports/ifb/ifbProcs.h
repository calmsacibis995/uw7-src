/*
 *	@(#) ifbProcs.h 12.2 95/06/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * ifbProcs.h
 *
 * routines for the "ifb"
 */

/* ifbGC.c */
extern void
ifbValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* ifbImage1.c */
extern void
ifbReadImage1(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
ifbDrawImage1(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* ifbImage8.c */
extern void
ifbReadImage8(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
ifbDrawImage8(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* ifbImage16.c */
extern void
ifbReadImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
ifbDrawImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* ifbImage32.c */
extern void
ifbReadImage32(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
ifbDrawImage32(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* ifbInit.c */
extern Bool ifbProbe();

extern Bool ifbInit(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv);

extern void
ifbCloseScreen(
        int index,
        ScreenPtr pScreen);

/* ifbWin.c */
extern void ifbValidateWindowPriv(
        struct _Window * pWin);

