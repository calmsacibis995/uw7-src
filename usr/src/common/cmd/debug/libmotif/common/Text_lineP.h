#ifndef _TEXT_LINEP_H
#define	_TEXT_LINEP_H
#ident	"@(#)debugger:libmotif/common/Text_lineP.h	1.3"

// toolkit specific members of the Text_line class
// included by ../../gui.d/common/Text_line.h

#define TEXT_LINE_TOOLKIT_SPECIFICS	\
private:				\
	char            *string;	\
	Boolean         editable;

#endif	// _TEXT_LINEP_H
