#ifndef	_MENUP_H
#define	_MENUP_H
#ident	"@(#)debugger:libmotif/common/MenuP.h	1.3"

// toolkit specific members of the Menu class
// included by ../../gui.d/common/Menu.h

class Button;

#define	MENU_TOOLKIT_SPECIFICS		\
private:				\
	Widget		*list;	\
	Widget		create_button(Button *);


#endif	// _MENUP_H
