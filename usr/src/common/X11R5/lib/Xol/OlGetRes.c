/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:OlGetRes.c	1.17"
#endif

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/*
 *************************************************************************
 * 
 ****************************procedure*header*****************************
 */
/* () {}
 *	The Above template should be located at the top of each file
 * to be easily accessable by the file programmer.  If this is included
 * at the top of the file, this comment should follow since formatting
 * and editing shell scripts look for special delimiters.		*/

/*
 *************************************************************************
 *
 * Date:	November 1988
 *
 * Description:
 *
 *	This file defines the "OlGetResolution()" routine.
 * 
 *	This routine generates a single letter that identifies the
 *	resolution of a screen. It refers to the .Xdefaults file to
 *	get a user specified mapping of resolution to character.
 *	If no entry is given in .Xdefaults, this routine uses a default,
 *	hard-coded mapping.
 *
 *	A modified BNF description of the syntax for specifing mapping
 *	follows.
 *
 *		mapping := "+" map-list
 *		mapping := map-list
 *
 *		map-list := map
 *		map-list := map-list "," map
 *
 *		map := resolution "=" character
 *
 *		resolution := integer "x" integer
 *		resolution := integer "x" integer "c"
 *
 *	An example is
 *
 *		"60x60=a,60x72=b"
 *
 *	A leading "+" means add the mapping to the built-in default;
 *	without the leading "+" the mapping replaces the default.
 *	A resolution is by default specified in dots per inch, with
 *	the horizontal value first. Appending a "c" to a resolution
 *	means dots per centimeter.
 *
 *	On lookup, the "OlGetResolution()" routine will interpolate
 *	to find the closest match. It will never fail to return a
 *	match, although (1) the match may be poor for unexpected screens,
 *	and (2) bad values of the "screen" parameter may cause problems,
 *	including a core dump.
 *
 *	The character returned is saved internally, so that a subsequent
 *	call to "OlGetResolution()" with the same screen argument can
 *	return quickly. To clear the saved character so that a complete
 *	lookup is performed again, call "OlGetResolution()" with a
 *	zero argument; nothing interesting is returned, but the NEXT
 *	call with a valid screen argument will work.
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>

#define OL_RESOLUTION_MAP		"ResolutionMap"
#define OL_DEFAULT_RESOLUTION_MAP	"\
76x55=a,\
76x63=b,\
76x76=c,\
95x95=d,\
66x48=e,\
66x55=f,\
66x66=g,\
83x83=h"

#define STREQU(A,B)	(strcmp((A), (B)) == 0)
#ifndef MEMUTIL
#define STRDUP(A)	(strcpy(XtMalloc(strlen((A)) + 1), (A)))
#endif /* MEMUTIL */

typedef struct _ResolutionItem {
	struct _ResolutionItem	*next;
	int			horz,
				vert;
	char			character;
}			ResolutionItem;

static ResolutionItem	*parse_resolution_map();
static ResolutionItem	*search_map();

/**
 ** OlGetResolution()
 **/

char
OlGetResolution (screen)
	Screen			*screen;
{
	char			*s_resolution_map;

	ResolutionItem		resolution,
				*p_item,
				*map;

	static Screen		*last_screen	= 0;

	static char		last_character	= 0;


	/*
	 * See if we've seen this screen argument before.
	 * If so, we know what to return without doing any more
	 * work. A zero argument clears what we've saved to force
	 * a lookup next time.
	 */
	if (screen == last_screen)
		return (last_character);
	last_screen = screen;

	if (!screen)
		return (0);


	s_resolution_map = XGetDefault(
		DisplayOfScreen(screen),
		XrmQuarkToString(_OlApplicationName),
		OL_RESOLUTION_MAP
	);
	if (!s_resolution_map || !*s_resolution_map)
		s_resolution_map = OL_DEFAULT_RESOLUTION_MAP;


	if (!(map = parse_resolution_map(screen, s_resolution_map))) {
		/*
		 * Either the user gave an empty map, or one full of
		 * errors, or our default map is bad. Try parsing just
		 * the default map to see which problem we have.
		 */
		if (!(map = parse_resolution_map(screen, OL_DEFAULT_RESOLUTION_MAP)))
		  OlVaDisplayErrorMsg((Display *)NULL,
				      OleNfileOlGetRes,
				      OleTmsg9,
				      OleCOlToolkitError,
				      OleMfileOlGetRes_msg9);
		else
		  OlVaDisplayWarningMsg((Display *)NULL,
				      OleNfileOlGetRes,
				      OleTmsg8,
				      OleCOlToolkitWarning,
				      OleMfileOlGetRes_msg8);
	}


#define INCHES(X)	((X) / (double)25.4)
	resolution.horz = WidthOfScreen(screen)
			/ INCHES(WidthMMOfScreen(screen));
	resolution.vert = HeightOfScreen(screen)
			/ INCHES(HeightMMOfScreen(screen));


	if (!(p_item = search_map(map, resolution)))
	  OlVaDisplayErrorMsg((Display *)NULL,
			      OleNfileOlGetRes,
			      OleTmsg7,
			      OleCOlToolkitError,
			      OleMfileOlGetRes_msg7);


	return (last_character = p_item->character);
}

/**
 ** parse_resolution_map()
 **/

static char *
strip (str)
	register char		*str;
{
	while (*str && isspace(*str))
		str++;
	return (str);
}

#ifdef RES_COMPLAIN
static void
res_complain (reason, str, offset)
	char			*reason,
				*str;
	int			offset;
{
	char			*msg;

	static char		*prefix	= "Resolution map error";


	msg = XtMalloc(
		  strlen(prefix)
		+ 2	/* --   */
		+ strlen(reason)
		+ 1	/* sp   */
		+ 1	/* sp   */
		+ 1	/* [    */
		+ strlen(str)
		+ 1	/* ]    */
		+ 1	/* nl   */
		+ 1	/* null */
	);

	sprintf (
		msg,
		"%s--%s %.*s [%s]\n",
		prefix,
		reason,
		offset,
		str,
		str + offset
	);
	OlWarning (msg);
	XtFree (msg);
}
#endif

static ResolutionItem *
parse_resolution_map (screen, str)
	Screen			*screen;
	char			*str;
{
	char			*orig_str,
				*copy_str,
				*pair,
				*p;

	ResolutionItem		*ret		= 0;


	if (!str)
	  OlVaDisplayErrorMsg((Display *)NULL,
			      OleNfileOlGetRes,
			      OleTmsg6,
			      OleCOlToolkitError,
			      OleMfileOlGetRes_msg6);

	/*
	 * Need a copy of the string, since "strtok()" is destructive.
	 * The copy is needed only for reporting errors; however, use
	 * "strtok()" on the copy, and leave the original alone, so that
	 * the original can be kept in read-only storage.
	 */
	p = copy_str = STRDUP(orig_str = strip(str));

	if (*p == '+') {
		char			*def = OL_DEFAULT_RESOLUTION_MAP;


		p++;

		/*
		 * Get the list of default resolution maps.
		 * To avoid endless loops, make sure there are
		 * no leading "+" characters!
		 */
		while (*def == '+')
			def++;
		ret = parse_resolution_map(screen, def);
	}

	/*
	 * Step through each resolution ``pair'' (HxV).
	 */
	while ((pair = strtok(p, ","))) {
		char			*x,
					*rest;

		ResolutionItem		*p_item;


		p = 0;	/* for subsequent calls to "strtok()" */

		if (!(x = strchr(pair, 'x'))) {
		  OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileOlGetRes,
					OleTmsg5,
					OleCOlToolkitWarning,
					OleMfileOlGetRes_msg5,
					pair - copy_str,
					orig_str,
					orig_str + (pair - copy_str));
			continue;
		}
		*x++ = 0;

		p_item = XtNew(ResolutionItem);

		p_item->horz = strtol(pair, &rest, 10);
		if (*rest)
		  OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileOlGetRes,
					OleTmsg4,
					OleCOlToolkitWarning,
					OleMfileOlGetRes_msg4,
					rest - copy_str,
					orig_str,
					orig_str + (rest - copy_str));

		p_item->vert = strtol(x, &rest, 10);
		if (*rest == 'c') {
			p_item->horz *= 2.54;
			p_item->vert *= 2.54;
			rest++;
		}
		if (*rest != '=') {
		  OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileOlGetRes,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileOlGetRes_msg3,
					rest - copy_str,
					orig_str,
					orig_str + (rest - copy_str));
			continue;
		}

		if (!(p_item->character = *++rest)) {
		  OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileOlGetRes,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileOlGetRes_msg2,
					rest - copy_str,
					orig_str,
					orig_str + (rest - copy_str));
			continue;
		}
		if (rest[1])
		  OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileOlGetRes,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileOlGetRes_msg1,
					rest + 1 - copy_str,
					orig_str,
					orig_str + (rest + 1 - copy_str));
		/*
		 * Link the resolution map into a list.
		 * The list is assumed to be small and order-independent.
		 * Thus we take it easy and link the new item at the
		 * head--this produces a backwards list.
		 */
		if (!ret)
			(ret = p_item)->next = 0;
		else {
			p_item->next = ret->next;
			ret->next = p_item;
		}
	}

	XtFree(copy_str);

	return (ret);
}

/*
 * search_map() - SEARCH RESOLUTION MAP FOR CLOSEST RESOLUTION
 */

static long
dist (a, b)
	register ResolutionItem *a, *b;
{
	register long		dhorz	= (a->horz - b->horz),
				dvert	= (a->vert - b->vert);


	return (dhorz * dhorz + dvert * dvert);
}

static ResolutionItem *
search_map (list, item)
	ResolutionItem		*list, item;
{
	long			best_distance,
				distance;

	ResolutionItem		*best_item;


	for (
		best_distance = dist((best_item = list), &item),
			list = list->next;
		list;
		list = list->next
	) {
		distance = dist(list, &item);
		if (distance < best_distance) {
			best_item = list;
			best_distance = distance;
		}
	}

	return (best_item);
}

