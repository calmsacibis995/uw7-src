#ifndef _Path_h
#define _Path_h

#ident	"@(#)debugger:inc/common/Path.h	1.3"

extern char	*current_dir;	// current working directory

extern int 	init_cwd();	// setup current_dir

extern char	*pathcanon(const char *); // canonicalized path name
#endif
