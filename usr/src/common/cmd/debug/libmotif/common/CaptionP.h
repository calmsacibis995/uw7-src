#ifndef	_CAPTIONP_H
#define	_CAPTIONP_H
#ident	"@(#)debugger:libmotif/common/CaptionP.h	1.2"

// toolkit specific members of the Caption class
// included by ../../gui.d/common/Caption.h

// caption is a static text widget; the top level widget is a form
// The predefined caption widget is not being used because it doesn't
// allow for resizing the child widget

#define CAPTION_TOOLKIT_SPECIFICS	\
private:				\
	Widget		caption;	\
	Caption_position position;	\
					\
	void		align_y(Widget);

#endif	// _CAPTIONP_H
