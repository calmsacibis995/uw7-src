#ifndef	_DBG_EDIT_H
#define	_DBG_EDIT_H

#ident	"@(#)debugger:inc/common/dbg_edit.h	1.1"

/* debugger interfaces to ksh editing 
 * this file is included by C language files, so must
 * not use C++ comments or constructs
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int	kread(int, char *, int);
extern int	set_mode(char *);
extern char	*get_mode();
extern void	ksh_init();
extern int	debug_read(int, void *, unsigned int);
extern void	edit_setup_prompt(const char *);

#ifdef __cplusplus
}
#endif

#endif
