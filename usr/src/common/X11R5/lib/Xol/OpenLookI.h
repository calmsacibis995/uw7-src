#ifndef NOIDENT
#ident	"@(#)olmisc:OpenLookI.h	1.13"
#endif

#ifndef __ApplicI_h__
#define __ApplicI_h__

#include "Xol/OpenLookP.h"

/*
 *  Description:
 *	This file contains all private tables for Applic.c.
 */

				/* Define a structure that holds the
				 * internal attributes of this OPEN LOOK
				 * application.  New internal attributes
				 * should be added to this structure.	*/
typedef struct
__OlAppAttributes {
	Cardinal	mouse_damping_factor;	/* in points		*/
	Cardinal	multi_click_timeout;	/* in milliseconds	*/
	int		beep_volume;		/* Beep volumn percentage*/
	OlDefine	beep;			/* Beep for which levels*/
	Boolean		select_does_preview;	/* Does select preview?	*/
	Boolean		grab_pointer;		/* can we grab the pointer*/
	Boolean		grab_server;		/* can we grab the server*/
	Cardinal	multi_object_count;
	OlBitMask	dont_care;		/* should use Modifiers,
						   but can't find a
						   repesentation type */
	Boolean		three_d;		/* use 3-D visuals?	*/
	char *		scale_map_file;		/* name of scale to screen
						 * resolution map file */
	char *		help_directory;		/* Where to look for help*/

			/*
			 * Specify Colors Global to the application
			 */
	Pixel		input_focus_color;
	Pixel		text_font_color;
	Pixel		text_background;
	Pixel		control_background;
	Pixel		control_foreground;
	Pixel		control_font_color;
	Pixel		generic_background;	/* bg for everything else*/

			/*
			 * Resources that control accelerators
			 * and mnemonics:
			 */
	Modifiers	mnemonic_modifiers;
	OlDefine	show_mnemonics;
	OlDefine	show_accelerators;
	String		shift_name;
	String		lock_name;
	String		control_name;
	String		mod1_name;
	String		mod2_name;
	String		mod3_name;
	String		mod4_name;
	String		mod5_name;

	OlDefine	help_model;
	Boolean		mouse_status;

	Dimension	drag_right_distance;
	Dimension	menu_mark_region;
	Cardinal	key_remap_timeout;	/* in seconds	*/

			/* resources for internationalization */

	char *		xnllanguage;
	char *		input_lang;
	char *		display_lang;
	char *		tdformat;
	char *		numeric;
	char *		input_method;
	Boolean		im_status;
	char *		font_group;
	char *		font_group_def;
	String		frontend_im_string;	/* for frontend input methods */	
	Atom		frontend_im_atom;	/* for frontend input methods*/

	Boolean		short_olwinattr;
	OlBitMask	key_dont_care;
	OlColorTupleList *	color_tuple_list;
	Boolean			use_color_tuple_list;
} _OlAppAttributes;

/* declare a ol_app_attributes variable here, so that Converters.c can
   use it in the Converter OlCvtFontGroupToFontStructList(). Note, this
   used to be static in Applic.c
*/

extern _OlAppAttributes ol_app_attributes;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

extern _OlAppAttributes *
_OlGetAppAttributesRef OL_ARGS(( Widget ));

OLEndFunctionPrototypeBlock

#endif /* __ApplicI_h__ */
