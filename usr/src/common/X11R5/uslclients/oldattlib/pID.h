#ifndef	NOIDENT
#ident	"@(#)oldattlib:pID.h	1.1"
#endif
/*
 pID.h (C header file)
	Acc: 575322459 Fri Mar 25 14:47:39 1988
	Mod: 575321571 Fri Mar 25 14:32:51 1988
	Sta: 575321571 Fri Mar 25 14:32:51 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 10/20/87
************************************************************************/

#ifndef _PID_H
#define _PID_H

typedef struct
{
	char *		name;
	unsigned long	type;
}
	ID;

extern ID _events[];
extern ID _eventmasks[];
extern ID _mousekeystate[];
extern ID _mousebutton[];
extern ID _notifymode[];
extern ID _notifydetail[];
extern ID _visibilitystate[];
extern ID _configuredetail[];
extern ID _configuremask[];
extern ID _circulateplace[];
extern ID _propertystate[];
extern ID _colormapstate[];
extern ID _mappingrequest[];
extern ID _notifyhint[];
extern ID _boolean[];
extern ID _class[];
extern ID _bit_gravity[];
extern ID _win_gravity[];
extern ID _backing_store[];
extern ID _map_state[];
extern ID _GXfunction[];
extern ID _line_style[];
extern ID _cap_style[];
extern ID _join_style[];
extern ID _fill_style[];
extern ID _fill_rule[];
extern ID _arc_mode[];
extern ID _subwindow_mode[];
extern ID _visualclass[];
extern ID _fontdirection[];
extern ID _bytebitorder[];
extern ID _knownproperties[];

extern void	fprint_match ();
extern void	fprint_mask ();
extern char **	mask_ID ();
extern char *	match_ID ();

/*
	standard formats
*/
#define STRING		"%s"
#define INT		"%d"
#define LONG		"%ld"
#define UINT		"%u"
#define ULONG		"%lu"
#define HEX		"0x%x"
#define LONGHEX		"0x%lx"
#define POINTER		LONGHEX

#endif
