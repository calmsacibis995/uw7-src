#ifndef _RESOURCESP_H
#define _RESOURCESP_H

#ident	"@(#)debugger:libmotif/common/ResourcesP.h	1.2"

// toolkit specific members of the Resources class
// included by ../../gui.d/common/Resources.h

#define RESOURCE_TOOLKIT_SPECIFICS		\
public:						\
	XrmOptionDescRec	*get_options(int &noptions);

#endif
