/*
 *	@(#) config.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 */
   
/*
   (c) Copyright 1988 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

#ifndef _XSCOCONFIG_H_
#define _XSCOCONFIG_H_

/*
   config.h - Xsco configuration data
*/

typedef struct ConfigRec {
    short headerSize;			/* size of ConfigRec */
    short buttonOffset;			/* offset to pointer button map */
    short buttonSize;			/* size of pointer button map */
    short modmapOffset;			/* offset to modifier map */
    short modmapSize;			/* size of modifier map */
    short keymapOffset;			/* offset to keymap record */
    short keymapSize;			/* size of keymap in bytes */
    short keymapWidth;			/* entries per key in keymap */
    short scanBias;			/* add this to hardware scan codes */
    short minScan;			/* lowest scan code (include bias) */
    short maxScan;			/* highest scan code (include bias) */
    short translateOffset;		/* offset to scan translate table */
    short translateSize;		/* size of scan translate table */
    short screenWidth;			/* screen width in millimeters */
    short screenHeight;			/* screen height in millimeters */
    short pixelOffset;			/* offset to pixel values */
    short pixelSize;			/* size of pixel values */
    short keyctrlOffset;		/* offset to key control table */
    short keyctrlSize;			/* size of key control table */
#ifdef XKB
    short xkbNamesOffset;		/* offset to XKB component names table */
    short xkbNamesSize;			/* size of XKB component names table */
#endif
    short pad;				/* to doubleword boundary */
} ConfigRec;

typedef struct ScanTranslation {
    unsigned short oldScan;
    unsigned short newScan;
} ScanTranslation;

#define PV_TempPixel	0
#define PV_StaticPixel	1
#define PV_WhitePixel	2
#define PV_BlackPixel	3

typedef struct PixelValue {
    unsigned long pixel;
    unsigned short red, green, blue;
    unsigned short type;
} PixelValue;

/*
   Definitions for key flags.  These definitions must correspond to
   those in xpseg.inc and xpseg.h.
   
   Note that the bits for CAPS_LIGHT, SCROLL_LIGHT and NUM_LIGHT
   correspond to the BIOS bits in KB_FLAGS1.
*/
#define	KF_TOGGLE_KEY		0x80
#define KF_CAPS_LIGHT		0x40
#define KF_NUM_LIGHT		0x20
#define KF_SCROLL_LIGHT		0x10
#define KF_SKIP_BIOS		0x08
#define KF_IGNORE_KEY		0x04
#define KF_BLOCKED		0x02
#define KF_LOCKED		0x01

#define KF_LIGHTS	(KF_CAPS_LIGHT | KF_NUM_LIGHT | KF_SCROLL_LIGHT)
#define KF_TOGGLES		(KF_LIGHTS | KF_TOGGLE_KEY)
#define KF_STATE		(KF_BLOCKED | KF_LOCKED)

#define KF_E0_OFFSET		0x80
#define KF_E1_OFFSET		0x100

#define KEY_CTRL_SIZE		0x180

#endif /* _XSCOCONFIG_H_ */
