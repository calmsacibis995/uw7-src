/*
 *	@(#) dfbProcs.h 12.2 95/06/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * dfbProcs.h
 *
 * routines for the "dfb"
 */

/* dfbCmap.c */

extern void
dfbInstallColormap(
	ColormapPtr cmap);

extern void
dfbUninstallColormap(
	ColormapPtr cmap);

extern int
dfbListInstalledColormaps(
	ScreenPtr pScreen,
	Colormap *pCmapList);

extern void
dfbStoreColors(
	ColormapPtr pmap,
	int ndef,
	xColorItem *pdefs);

/* dfbInit.c */

extern Bool dfbProbe();

extern Bool dfbInit(
        int index,
	ScreenPtr pScreen,
        int argc,
        char **argv);

extern void dfbFreeScreen(
	int index,
	ScreenPtr pScreen);

/* dfbScreen.c */

extern Bool
dfbSaveScreen(
        ScreenPtr pScreen,
        int on);

extern void dfbSetGraphics();

extern void dfbSetText();
