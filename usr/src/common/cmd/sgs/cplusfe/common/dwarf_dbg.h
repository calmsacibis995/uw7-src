#ifndef DWARF_DBG_H
#define DWARF_DBG_H
#ident	"@(#)cplusfe:common/dwarf_dbg.h	1.6"

/************************************************************************
 *									*
 *	Declarations for generation of DWARF (Dwarf1 or Dwarf2)		*
 *	debugging information.						*
 *									*
 ************************************************************************/

#if GENERATING_DEBUG_INFO

#include <stdio.h>
#include "il.h"

EXTERN a_boolean	dwarf_generate_symbol_info /* = FALSE */;
EXTERN char		*dwarf_debug_file; /* name of the intermediate file */
EXTERN FILE		*dwarf_dbout;	/* intermediate file */
EXTERN a_boolean	dwarf_comments_in_debug /* = FALSE */;
EXTERN a_boolean	dwarf_gen_abbreviations /* = FALSE */;

EXTERN a_boolean	dwarf_name_lookup /* = FALSE */;
				/* Dwarf 2 - name lookup section generation
					   - off by default (for now) */

EXTERN a_boolean	dwarf_addr_ranges /* = FALSE */;
				/* Dwarf 2 - address range section generation
					   - off by default (for now) */
EXTERN Dwarf_level	dwarf_level
#if VAR_INITIALIZERS
				= Dwarf2
#endif /* VAR_INITIALIZERS */
				;


extern void	dwarf_debug_init(void);
extern void	dwarf_gen_debug_info_for_scope(a_scope_ptr);
extern void	dwarf_fixup_for_return_value_pointer_variable(a_variable_ptr);
extern void	dwarf_debug_cleanup(void);

#endif /* GENERATING_DEBUG_INFO */
#endif /* DWARF_DBG_H */
