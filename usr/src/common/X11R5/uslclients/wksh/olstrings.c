
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olstrings.c	1.3"


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/Gauge.h>
#include <Xol/MenuButton.h>
#include <Xol/Menu.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>

#include "wksh.h"
#include "olksh.h"


/* CONSTANT Character Strings */

const char str_XtROlChangeBarDefine[] = "OlChangeBarDefine";

#ifndef MOOLIT
const char str_XtRString[] = XtRString;
const char str_XtRCardinal[] = XtRCardinal;
const char str_XtRWidgetList[] = XtRWidgetList;
const char str_XtRPointer[] = XtRPointer;
const char str_XtRBitmap[] = XtRBitmap;
const char str_XtRPixmap[] = XtRPixmap;
const char str_XtRWidget[] = XtRWidget;
const char str_XtRInt[] = XtRInt;
const char str_XtRBoolean[] = XtRBoolean;
const char str_XtRPosition[] = XtRPosition;
const char str_XtRBool[] = XtRBool;
const char str_XtRDimension[] = XtRDimension;
const char str_XtRPixel[] = XtRPixel;
const char str_XtRFontStruct[] = XtRFontStruct;
const char str_XtRCallback[] = XtRCallback;
#endif

const char str_Function[] = "Function";
const char str_XtROlDefine[] = "OlDefine";
const char str_XtROlDefineInt[] = "OlDefineInt";
const char str_XtROlVirtualName[] = "OlVirtualName";
