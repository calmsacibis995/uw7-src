#ifndef	NOIDENT
#ident	"@(#)flat:ColorChip.h	1.1"
#endif

#ifndef _OL_COLORCHIP_H
#define _OL_COLORCHIP_H

/*
 * Special types:
 */

typedef struct _OlColorChipLabel {
	Pixel			pixel;
	Boolean			insensitive;
}			_OlColorChipLabel;

/*
 * External routines:
 */

extern void		_OlCreateColorChip OL_ARGS((
	Widget			w
));
extern void		_OlDestroyColorChip OL_ARGS((
	Widget			w
));
extern void		_OlDrawColorChip OL_ARGS((
	Screen *		screen,
	Window			window,
	OlgAttrs *		attrs,	/*NOTUSED*/
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	XtPointer		label
));

#endif /* _OL_COLORCHIP_H */
