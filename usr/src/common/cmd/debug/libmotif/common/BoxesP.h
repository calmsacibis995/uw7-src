#ifndef	BOXESP_H
#define	BOXESP_H
#ident	"@(#)debugger:libmotif/common/BoxesP.h	1.1"

// toolkit specific members of the Box class and derived classes
// included by ../../gui.d/common/Boxes.h

#define BOX_TOOLKIT_SPECIFICS		\
protected:				\
	Orientation	orientation;

#define PACKBOX_TOOLKIT_SPECIFICS	\
private:				\
	void		add_to_form(Widget, Widget);

#define	DIVIDEDBOX_TOOLKIT_SPECIFICS

#define	EXPANSIONBOX_TOOLKIT_SPECIFICS	\
	void		add_to_form(Widget, Widget, Boolean);	\
	Boolean		elastic_added;

#endif	// BOXESP_H
