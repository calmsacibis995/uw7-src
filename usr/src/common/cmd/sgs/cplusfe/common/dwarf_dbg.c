#ident	"@(#)cplusfe:common/dwarf_dbg.c	1.16"

#include "basics.h"
#include "host_envir.h"
#include "targ_def.h"
#include "target.h"
#include "error.h"
#include "types.h"
#include "il.h"
#include "il_alloc.h"
#include "lower_il.h"
#include "dwarf_dbg.h"
#include "dwarf.h"
#include "dwarf2.h"
#include "libdwarf2.h"
#include "abbrev.h"
#include "sgs.h"
#include "mem_manage.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/utsname.h>

/* definitions used by dwarf_gen.h */
#define DB_STACK_INCR	20
#define DB_CURINFOLAB	(db_stack[db_stack_depth].infolab)
#define DB_CURSIBLING	(db_stack[db_stack_depth].sibling)

/* debug_stack is used to maintain a stack of entries that holds
** the label numbers for the current debugging information entry
** and its sibling pointer at each level
*/
typedef struct
{
    int infolab;		/* label of current info entry */
    int sibling;		/* sibling of current entry */
} debug_stack;
static debug_stack *db_stack;
static size_t	db_stack_size;	/* total size of the debug stack */
static size_t	db_stack_depth;	/* current entry on the debug stack */

static const char	*db_dbg_sec_name;
static const char	*db_line_sec_name;
static int		dw2_prev_line_label = 0; /* unused by front end */
static int		dw2_curr_line_label = 0; /* unused by front end */

#define IS_DWARF_2()	(dwarf_level == Dwarf2)
#define IS_DWARF_1()	(dwarf_level == Dwarf1)
#define COMMENTS_IN_DEBUG	(dwarf_comments_in_debug)
#define ERROR_FUNCTION		internal_error
#define ADDR_SIZE		targ_sizeof_pointer
#define DBOUT			dwarf_dbout

#include <dwarf_gen.h>

#define DB_DW2_END	0
#define DB_DW2_VERSION	2
#define DB_GENLAB()	(++dw_labno)
static a_debug_entry_id	dw_labno = 0;	/* current debug label number */

/* dw_mark_stack is used to maintain a stack of a_debug_mark entries at
** each scope level.  This roughly parallels the debug_stack, but the
** stacks are pushed and popped at different places, and the dw_mark_stack
** is pushed just for functions and blocks, not for enumerations, class
** definitions, etc. where the debug_stack is pushed
*/
typedef struct
{
	a_debug_mark_list	*mark_list;
	a_debug_mark		*tail;
} dw_mark_stack;
static dw_mark_stack	*mark_stack;
static dw_mark_stack	*mark_stack_ptr;	/* current entry in the mark stack */
static size_t		mark_stack_size;	/* total size of mark stack */
static size_t		mark_stack_depth;	/* current entry in the mark stack */
static a_debug_mark_ptr	return_value_ptr_mark;
static a_debug_entry_id	return_value_ptr_id;

/* A linked list of static data members is created during processing of class
** definitions, and the list is walked at file scope to generate debugging
** entries for the definitions.  Processing of the definitions has to delayed
** since it can't simply be handled in the parent scope, which might just be
** another class.
*/
typedef struct a_static_member
{
	struct a_static_member	*next;
	a_variable_ptr		variable;
} a_static_member;

static a_static_member	*static_member_head;
static a_static_member	*static_member_tail;

/* A linked list of vtbl entries is created during processing of class definitions.
** Processing the list has to delayed until after IL lowering since it is the
** lowering process that determines whether a vtbl entry will be generated.
*/
typedef struct a_vtbl_link
{
	struct a_vtbl_link		*next;
	a_variable_ptr			vtbl;
	a_virtual_function_number	nslots;
} a_vtbl_link;
static a_vtbl_link	*vtbl_head;
static a_vtbl_link	*vtbl_tail;

static const Dwarf2_Abbreviation *dw_abbrev_table;
static a_boolean		curr_is_inline;

/* octl is needed for generating the .pubnames section */
static an_il_to_str_output_control_block	octl;



/* This array gives tags for member function declarations (children of
** the class definition) and is indexed by boolean values:
** [is compiler generated?][has a return type?][is virtual?]
** If it is compiler generated, then the entry will have a
**	DW_AT_artificial attribute and no DW_AT_decl_line or DW_AT_decl_file
** If it does not have a return type, the entry will not have a DW_AT_type attribute
** If it is virtual, the entry will have DW_AT_virtuality and
**	DW_AT_vtable_elem_location attributes
*/
static Abbrev_index member_function_tags[2][2][2] =
{
	/* compiler generated */
	{
		/* return type */
		{
			/* virtual */
			DW2_void_function_decl,
			DW2_void_virtual_member,
		},
		{
			DW2_non_void_function_decl,
			DW2_non_void_virtual_member,
		},
	},
	{
		{
			DW2_artif_void_member,
			DW2_artif_void_virtual_member,
		},
		{
			DW2_artif_non_void_member,
			DW2_artif_non_void_virtual_member,
		},
	},
};

/* This array gives tags for member function definitions (file scope)
** and is indexed by boolean values:
** [is compiler generated?][is inline?]
** If it is compiler generated, then the entry will have a
**	DW_AT_artificial attribute and no DW_AT_decl_line or DW_AT_decl_file
** If it is inline, the entry will have DW_AT_inline but no
**	DW_AT_high_pc or DW_AT_low_pc attributes
*/
static Abbrev_index member_function_defn_tags[2][2] =
{
	/* compiler generated */
	{
		/* inline */
		DW2_member_defn,
		DW2_member_inline,
	},
	{
		DW2_artif_member_defn,
		DW2_artif_member_inline,
	},
};

/* This array is gives the tags for non-member functions
** and is indexed by boolean values:
** [is compiler generated?][has a return type?][is inline?]
** If it is compiler generated, then the entry will have a
**	DW_AT_artificial attribute and no DW_AT_decl_line or DW_AT_decl_file
** If it does not have a return type, the entry will not have a DW_AT_type attribute
** If it is virtual, the entry will have DW_AT_virtuality and
**	DW_AT_vtable_elem_location attributes
** If it is inline, the entry will have DW_AT_inline but no
**	DW_AT_high_pc or DW_AT_low_pc attributes
*/
static Abbrev_index function_tags[2][2][2] = 
{
	/* compiler generated */
	{
		/* return type */
		{
			/* inline */
			DW2_void_subr,
			DW2_void_subr_inline,
		},
		{
			DW2_non_void_subr,
			DW2_non_void_subr_inline,
		}
	},
	{
		{
			DW2_artif_void_subr,
			DW2_artif_void_subr_inline,
		},
		{
			DW2_artif_non_void_subr,
			DW2_artif_non_void_subr_inline,
		}
	},
};

/* This array is indexed by boolean values:
** [is compiler generated?][is parameter?][is in inline function?]
** If it is compiler generated, then the entry will have a
**	DW_AT_artificial attribute and no DW_AT_decl_line or DW_AT_decl_file
** If in an inline function, will not have a location
*/
static Abbrev_index variable_tags[2][2][2] = 
{
	/* compiler generated */
	{
		/* is parameter */
		{
			/* inline */
			DW2_variable,
			DW2_variable_abstract,
		},
		{
			DW2_formal_param,
			DW2_formal_param_abstract,
		}
	},
	{
		{
			DW2_artif_variable,
			DW2_artif_variable_abstract,
		},
		{
			DW2_artif_formal_param,
			DW2_artif_formal_param_abstract,
		}
	}
};

/* translate between dwarf2 abbreviation table entries and dwarf1 tags */
static int dwarf1_tags[] =
{
	TAG_padding,		/* DW2_none */
	TAG_compile_unit,	/* DW2_compile_unit */
	TAG_compile_unit,	/* DW2_compile_no_line_info */
	TAG_padding,		/* DW2_base_type */
	TAG_pointer_type,	/* DW2_pointer_type */
	TAG_reference_type,	/* DW2_reference_type */
	TAG_padding,		/* DW2_const_type */
	TAG_padding,		/* DW2_volatile_type */
	TAG_typedef,		/* DW2_typedef */
	TAG_class_type,		/* DW2_class */
	TAG_class_type,		/* DW2_incomplete_class */
	TAG_structure_type,	/* DW2_struct */
	TAG_structure_type,	/* DW2_incomplete_struct */
	TAG_union_type,		/* DW2_union */
	TAG_union_type,		/* DW2_incomplete_union */
	TAG_member,		/* DW2_su_member */
	TAG_member,		/* DW2_csu_member */
	TAG_member,		/* DW2_bit_field */
	TAG_enumeration_type,	/* DW2_enumeration */
	TAG_enumeration_type,	/* DW2_incomplete_enumeration */
	TAG_member,		/* DW2_enumerator */
	TAG_array_type,		/* DW2_array */
	TAG_padding,		/* DW2_subrange */
	TAG_subroutine_type,	/* DW2_non_void_subr_type */
	TAG_subroutine_type,	/* DW2_void_subr_type */
	TAG_formal_parameter,	/* DW2_formal_param_type */
	TAG_formal_parameter,	/* DW2_artif_formal_param_type */
	TAG_unspecified_parameters,	/* DW2_unspecified_params */
	TAG_local_variable,	/* DW2_variable */
	TAG_local_variable,	/* DW2_variable_no_decl */
	TAG_local_variable,	/* DW2_artif_variable */
	TAG_formal_parameter,	/* DW2_formal_param */
	TAG_formal_parameter,	/* DW2_formal_param_no_decl */
	TAG_formal_parameter,	/* DW2_artif_formal_param */
	TAG_local_variable,	/* DW2_variable_abstract */
	TAG_local_variable,	/* DW2_artif_variable_abstract */
	TAG_formal_parameter,	/* DW2_formal_param_abstract */
	TAG_formal_parameter,	/* DW2_artif_formal_param_abstract */
	TAG_local_variable,	/* DW2_variable_concrete */
	TAG_formal_parameter,	/* DW2_formal_param_concrete */
	TAG_label,		/* DW2_label */
	TAG_label,		/* DW2_label_abstract */
	TAG_label,		/* DW2_label_concrete */
	TAG_lexical_block,	/* DW2_lexical_block */
	TAG_lexical_block,	/* DW2_lexical_block_abstract */
	TAG_lexical_block,	/* DW2_lexical_block_concrete */
	TAG_global_subroutine,	/* DW2_void_subr */
	TAG_global_subroutine,	/* DW2_non_void_subr */
	TAG_global_subroutine,	/* DW2_artif_void_subr */
	TAG_global_subroutine,	/* DW2_artif_non_void_subr */
	TAG_global_subroutine,	/* DW2_member_defn */
	TAG_global_subroutine,	/* DW2_artif_member_defn */
	TAG_global_subroutine,	/* DW2_non_void_subr_inline */
	TAG_global_subroutine,	/* DW2_void_subr_inline */
	TAG_global_subroutine,	/* DW2_artif_non_void_subr_inline */
	TAG_global_subroutine,	/* DW2_artif_void_subr_inline */
	TAG_global_subroutine,	/* DW2_member_inline */
	TAG_global_subroutine,	/* DW2_artif_member_inline */
	TAG_global_subroutine,	/* DW2_non_void_function_decl */
	TAG_global_subroutine,	/* DW2_void_function_decl */
	TAG_global_subroutine,	/* DW2_non_void_virtual_member */
	TAG_global_subroutine,	/* DW2_void_virtual_member */
	TAG_global_subroutine,	/* DW2_artif_non_void_member */
	TAG_global_subroutine,	/* DW2_artif_void_member */
	TAG_global_subroutine,	/* DW2_artif_non_void_virtual_member */
	TAG_global_subroutine,	/* DW2_artif_void_virtual_member */
	TAG_inheritance,	/* DW2_inheritance */
	TAG_global_variable,	/* DW2_static_member */
	TAG_global_variable,	/* DW2_static_member_decl */
	TAG_ptr_to_member_type,	/* DW2_ptr_to_member */
	TAG_ptr_to_member_type,	/* DW2_ptr_to_function_member */
	TAG_local_variable,	/* DW2_unrefed_variable */
	TAG_local_variable,	/* DW2_variable_decl */
	TAG_lexical_block,	/* DW2_try_block */
	TAG_lexical_block,	/* DW2_try_block_abstract */
	TAG_lexical_block,	/* DW2_try_block_concrete */
	TAG_lexical_block,	/* DW2_catch_block */
	TAG_lexical_block,	/* DW2_catch_block_abstract */
	TAG_lexical_block,	/* DW2_catch_block_concrete */
	TAG_padding,		/* DW2_thrown_type */
	TAG_padding,		/* DW2_template_type_param */
	TAG_padding,		/* DW2_template_value_param */
	TAG_padding,		/* DW2_namespace */
	TAG_padding,		/* DW2_namespace_alias */
	TAG_padding,		/* DW2_using_directive */
	TAG_padding,		/* DW2_using_declaration */
	TAG_inlined_subroutine,	/* DW2_inlined_subroutine */
	TAG_subroutine,		/* DW2_out_of_line_instance */
	TAG_padding,		/* DW2_last */
};

/* forward declarations */
static void dw_type_list(a_type_ptr);
static void dw_gen_based_types(a_type_ptr);
static void dw_variable(a_variable_ptr var, a_scope_ptr scope, a_boolean gen_type);
static a_boolean dw_walktype(a_type_ptr type);
static void dw_typedef(a_type_ptr type);
static void dw_gen_array(a_type_ptr type, int nelements);
static void dw_mark_variable(a_variable_ptr var, a_boolean addr_only);
static void dw_mark_routine(a_routine_ptr rp);
static void dw_beg_function(Abbrev_index tag, a_routine_ptr routine,
				a_boolean is_definition);
static a_debug_mark_ptr dw_new_mark(an_il_entry_kind kind);

static void dw_s_entry_common(a_source_correspondence *scp)
/* generate a label for the start of a debugging entry and assign a debug id */
{
	a_debug_entry_id	id;

	if ((id = DB_CURSIBLING) == 0)
		id = DB_GENLAB();
	DB_CURINFOLAB = id;
	DB_CURSIBLING = DB_GENLAB();

	dwarf_out(".d", id);
	if (scp) {
		/* If the entity had already been assign a temporary id (a forward
		** reference), equate the temporary id with the real label.
		*/
		if (scp->debug_id_is_temporary) {
			dwarf_out("=", (int)scp->debug_id);
			scp->debug_id_is_temporary = FALSE;
			scp->debug_id = DB_CURINFOLAB;
		} else if (!scp->debug_id) {
			scp->debug_id_is_temporary = FALSE;
			scp->debug_id = DB_CURINFOLAB;
		} /* if */
	} /* if */
} /* dw_s_entry_common */

static void dw1_s_entry(int tag, a_source_correspondence *scp, int scope,
			const char *comment)
/* Dwarf1-specific - always generates a sibling attribute */
/* scope is zero for everything but a function definition */
{
	if (tag == TAG_padding)
		internal_error("dw1_s_entry(): invalid tag");
	dw_s_entry_common(scp);
	dwarf_out("\tW\tLC-Lc\n");	/* size of entry */
	dwarf_out("\tH\txC", tag, comment);
	/* Output sibling attribute - all dwarf1 entries have one */
	/* Siblings for functions are generated with the function entry
	** based on the memory region number, since some reordering of groups of
	** entries may occur in the back end due to inlining.
	** So that the debugging information does not depend on ordering
	** between function, the block of debugging entries for a function is
	** made into a self-contained unit, with the sibling label (of the
	** form ..S#) being moved along with the function entries.  In
	** the resultant .s file, the offset of that label will be the same
	** as the address of the next debugging entry, whatever that might
	** happen to be.
	*/
	if (scope)
		dwarf_out("\tH\tx; W\tLsC", AT_sibling, scope, "AT_sibling");
	else
		dwarf_out("\tH\tx; W\tLdC", AT_sibling, DB_CURSIBLING, "AT_sibling");
} /* dw1_s_entry */

static void dw_s_entry(Abbrev_index index, a_source_correspondence *scp,
			const char *comment)
/* Start new debug information entry with tag index.  Set up the current
** stack appropriately.  "index" denotes the abbreviation table number.
** For Dwarf1 the index is translated into a TAG value.
*/
{
	if (IS_DWARF_1()) {
		dw1_s_entry(dwarf1_tags[index], scp, /*scope=*/ 0, comment);
	} else {
		dw_s_entry_common(scp);
		dwarf_out("\tluC", index, comment);
	} /* if */
} /* dw_s_entry */

static void dw_e_entry(void)
/* End debug information entry. Generate end label (for Dwarf 1). */
{
	if (IS_DWARF_1())
		dwarf_out(".C");
} /* dw_e_entry */

#define NAME_BUF_INCR	BUFSIZ
static char	*dw_name_buf;
static size_t	dw_name_buf_len;
static size_t	dw_name_buf_used;

static void write_string(char *s)
/* Output function for form_symbol_name, called while creating full names for
** .debug_pubnames section
*/
{
	size_t	len = strlen(s);
	size_t	needed;
	if (!dw_name_buf)
	{
		needed = (len < (size_t)NAME_BUF_INCR) ? NAME_BUF_INCR : len;
		dw_name_buf = alloc_general(needed);
		dw_name_buf_len = needed;
	} else if (dw_name_buf_used + len >= dw_name_buf_len) {
		needed = (len < (size_t)NAME_BUF_INCR) ? NAME_BUF_INCR : len;
		dw_name_buf = realloc_general(dw_name_buf, dw_name_buf_len,
						dw_name_buf_len + needed);
		dw_name_buf_len += needed;
	}
	strcpy(&dw_name_buf[dw_name_buf_used], s);
	dw_name_buf_used += len;		
} /* write_string */

static a_debug_entry_id dw_debug_id(a_source_correspondence *scp)
/* Return the id for the given entity.  If the entity has not yet been
** generated, assign a temporary id.
*/
{
	static a_debug_entry_id temp_id;

	if (!scp->debug_id) {
		scp->debug_id = ++temp_id;
		scp->debug_id_is_temporary = TRUE;
	} /* if */
	return scp->debug_id;
} /* dw_debug_id */


static void dw_push(void)
/* Start new level on debug stack.  New sibling label number is 0. */
{
	if (db_stack_depth+1 == db_stack_size) {
		size_t new_size = db_stack_size + DB_STACK_INCR;
		db_stack = (debug_stack *)realloc_general((char *)db_stack,
				(size_t)(db_stack_size * sizeof(debug_stack)),
				(size_t)(new_size * sizeof(debug_stack)));
		db_stack_size = new_size;
	} /* if */
	++db_stack_depth;
	DB_CURSIBLING = 0;			/* no sibling yet */
} /* dw_push */

static long dw_pop(void)
/* Exit current debug stack level.  Tie off any dangling sibling
** pointer.  Return the offset in the output file of the point between
** the label and the null entry.  This offset is stored in a debug_mark
** for a function or block entry, and is used by the back end as the
** insertion point for any inline function entries needed at that level.
*/
{
	long	insert_point;
	/* Output a null entry to tie off any non-existent children for
	** this entry.  For Dwarf1, the phony entry has no content other
	** than the length.  For Dwarf2, it is a zero entry.
	*/
	if (DB_CURSIBLING)
		dwarf_out(".d", DB_CURSIBLING);
	insert_point = ftell(dwarf_dbout);
	if (IS_DWARF_1()) {
		dwarf_out("\tW\td\n", 4);
	} else {
		dwarf_out("\tB\txC", DB_DW2_END, "end sibling chain");
	}  /* if */
	if (--db_stack_depth < 0)
		internal_error("debug stack underflow");
	return insert_point;
} /* dw_pop */

static void dw_at_name(const char *s)
/* Output name attribute.  It may be null for unamed types or artificial
** (compiler-generated) entities
*/
{
	if (s == 0)
		s = "";
	if (IS_DWARF_1()) {
		/* Dwarf 1 */
		dwarf_out("\tH\txC", AT_name, "AT_name");
		dwarf_out("b\n", s);
	} else {
		/* Dwarf 2 */
		dwarf_out("bC", s, "DW_AT_name");
	}  /* if */
} /* dw_at_name */

static void dw_get_file_and_line(a_seq_number seq, int *file_index,
				a_line_number *line)
/* Translate sequence number into line number and file index */
{
	a_source_file_ptr	file;
	a_boolean		at_eos;
	unsigned long		depth;

	if (seq) {
		file = source_file_for_seq(seq, line, &at_eos, &depth,
			/*physical_line=*/FALSE);
		if (!file)
			internal_error("dw_get_file_and_line(): invalid seq");
		*file_index = file->index;
	} else {
		*file_index = 0;
		*line = 0;
		return;
	} /* if */
} /* dw_get_file_and_line */

static void dw_at_reference(int attr, a_source_correspondence *scp,
			    const char *comment)
/* Put out the DW_AT entry, which is a reference to an
** entry elsewhere in the .debug_info section.
*/
{
	a_debug_entry_id id = dw_debug_id(scp);
	const char	*fmt = (scp->debug_id_is_temporary) ? "\tW\tLt" : "\tW\tLd";

	/* The test below is special-case handling for Dwarf1 type information.
	** The only place this function is called with attr == 0 is from dw_at_type,
	** where there may be some number of modifiers that have to be
	** generated between the attribute name and the reference to another
	** type entry.
	*/
	if (IS_DWARF_1() && attr)
		dwarf_out("\tH\tx;", attr);
	dwarf_out(fmt, id);
	if (IS_DWARF_2())
		dwarf_out("-s", LAB_INFOBEGIN);
	dwarf_out("C", comment);
}  /* dw_at_reference */

static void dw_at_type(a_type_ptr type)
/* Dwarf 1
** Output type attribute.  By this time all sub-types that
** would result in a new debug info entry being generated
** must already have been flushed.
**
** Dwarf 2
** Put out the DW_AT_type entry, which is a reference to a
** type entry elsewhere in the .debug_info section.
*/
{
	a_type_ptr	base_type;
	int		modifiers;
	int		attr;
	const char	*attr_name;
	a_boolean	usertype;

	if (IS_DWARF_2()) {
		dw_at_reference(DW_AT_type, &type->source_corresp, "DW_AT_type");
		return;
	} /* if */

	/* Dwarf 1 */
	/* Count the number of type modifiers */
	base_type = type;
	modifiers = 0;
	for (;;) {
		if (base_type->kind == tk_typeref
			&& typeref_is_qualified(base_type)) {
			if (typeref_is_const_qualified(base_type))
				++modifiers;
			if (typeref_is_volatile_qualified(base_type))
				++modifiers;
			base_type = base_type->variant.typeref.type;
		} else {
			break;
		} /* if */
	} /* for */

	/* determine type attribute */
	if (base_type->kind == tk_void || base_type->kind == tk_float
		|| (base_type->kind == tk_pointer
			&& is_void_type(type_pointed_to(base_type)))
		|| (base_type->kind == tk_integer
			&& !base_type->variant.integer.enum_type)) {
		usertype = FALSE;
		if (modifiers) {
			attr = AT_mod_fund_type;
			attr_name = "AT_mod_fund_type";
		} else {
			attr = AT_fund_type;
			attr_name = "AT_fund_type";
		} /* if */
	} else {
		usertype = TRUE;
		if (modifiers) {
			attr = AT_mod_u_d_type;
			attr_name = "AT_mod_u_d_type";
		} else {
			attr = AT_user_def_type;
			attr_name = "AT_user_def_type";
		} /* if */
	} /* if */
	dwarf_out("\tH\txC", attr, attr_name);

	if (modifiers) {
		/* Generate length of whole attribute, modifiers. */
		dwarf_out("\tH\td\n", modifiers + (usertype ? 4 : 2));
		base_type = type;
		for (;;) {
			if (base_type->kind == tk_typeref
				&& typeref_is_qualified(base_type)) {
				if (typeref_is_const_qualified(base_type))
					dwarf_out("\tB\tx\n", MOD_const);
				if (typeref_is_volatile_qualified(base_type))
					dwarf_out("\tB\tx\n", MOD_volatile);
				base_type = base_type->variant.typeref.type;
			} else {
				break;
			} /* if */
		} /* for */
	} /* if */

	if (usertype) {
		/* Attribute name already generated, don't generate again */
		dw_at_reference(/*attr=*/0, &base_type->source_corresp, "user type");
	} else if (base_type->kind == tk_void) {
		dwarf_out("\tH\txC", FT_void, "void");
	} else if (base_type->kind == tk_pointer) {
		dwarf_out("\tH\txC", FT_pointer, "void *");
	} else if (base_type->kind == tk_float) {
		switch (base_type->variant.float_kind) {
		case fk_float:
			dwarf_out("\tH\txC", FT_float, "float");
			break;
		case fk_double:
			dwarf_out("\tH\txC", FT_dbl_prec_float, "double");
			break;
		case fk_long_double:
			dwarf_out("\tH\txC", FT_ext_prec_float, "long double");
			break;
		} /* switch */
	} else if (base_type->kind != tk_integer) {
		internal_error("dw_at_type: invalid type");
	} else if (base_type->variant.integer.bool_type) {
		dwarf_out("\tH\txC", FT_boolean, "bool");
	} else {
		int		elftype = FT_none;
		const char	*ts = "";

		switch (base_type->variant.integer.int_kind) {
		case ik_char:
			elftype = FT_char; ts = "char"; break;
		case ik_signed_char:
			elftype = FT_signed_char; ts = "signed char"; break;
		case ik_unsigned_char:
			elftype = FT_unsigned_char; ts = "unsigned char"; break;
		case ik_short:
			if (base_type->variant.integer.explicitly_signed) {
				elftype = FT_signed_short; ts = "signed short";
			} else {
				elftype = FT_short; ts = "short";
			} /* if */
			break;
		case ik_unsigned_short:
			elftype = FT_unsigned_short; ts = "unsigned short"; break;
		case ik_int:
			if (base_type->variant.integer.explicitly_signed) {
				elftype = FT_signed_integer; ts = "signed int";
			} else {
				elftype = FT_integer; ts = "int";
			} /* if */
			break;
		case ik_unsigned_int:
			elftype = FT_unsigned_integer; ts = "unsigned int"; break;
		case ik_long:
			if (base_type->variant.integer.explicitly_signed) {
				elftype = FT_signed_long; ts = "signed long";
			} else {
				elftype = FT_long; ts = "long";
			} /* if */
			break;
		case ik_unsigned_long:
			elftype = FT_unsigned_long; ts = "unsigned long"; break;
#if LONG_LONG_ALLOWED
#ifndef FT_long_long	/* not in older (<= UnixWare 2.1?) versions of dwarf.h */
#define FT_long_long		(FT_label+1)
#define FT_signed_long_long	(FT_label+2)
#define FT_unsigned_long_long	(FT_label+3)
#endif
		case ik_long_long:
			if (base_type->variant.integer.explicitly_signed) {
				elftype = FT_signed_long_long;
				ts = "signed long long";
			} else {
				elftype = FT_long_long; ts = "long long";
			} /* if */
			break;
		case ik_unsigned_long_long:
			elftype = FT_unsigned_long_long;
			ts = "unsigned long long";
			break;
#endif /* LONG_LONG_ALLOWED */
		} /* switch */
		dwarf_out("\tH\txC", elftype, ts);
	} /* if */
}  /* dw_at_type */

static void dw_at_byte_size(a_targ_size_t size)
/* Output the size attribute for an object whose size is "size" bytes. */
{
	if (IS_DWARF_1()) {
		/* Dwarf 1 */
		dwarf_out("\tH\tx; W\tdC", AT_byte_size, size, "AT_byte_size");
	} else {
		/* Dwarf 2 */
		dwarf_out("\tluC", size, "DW_AT_byte_size");
	}  /* if */
} /* dw_at_byte_size */

/* generate one base-type entry */
static void dwarf2_base_type(a_type_ptr type, const char *name,
			     int base_type_encoding)
/* generate a Dwarf2 base type entry */
{
	dw_s_entry(DW2_base_type, &type->source_corresp, "base type");
	dw_at_name(name);
	dwarf_out("\tB\txC", base_type_encoding, "DW_AT_encoding");
	dw_at_byte_size(type->size);
	dw_e_entry();
} /* dwarf2_base_type */

static void dwarf2_fund_types(void)
/* Generate the fundamental types for C and C++

   NOTE: These fundamental type entries are ideal candidates to move to 
         a common .debug_info section to be shared by all .o's when
	 compiled with debugging.
*/
{
	dwarf2_base_type(integer_type(ik_char), "char", DW_ATE_signed_char);
	dwarf2_base_type(integer_type(ik_unsigned_char), "unsigned char",
			DW_ATE_unsigned_char);
	dwarf2_base_type(integer_type(ik_signed_char), "signed char",
			DW_ATE_signed_char);

	dwarf2_base_type(integer_type(ik_short), "short", DW_ATE_signed);
	dwarf2_base_type(integer_type(ik_unsigned_short), "unsigned short",
			DW_ATE_unsigned);
	dwarf2_base_type(signed_integer_type(ik_short), "signed short",
			DW_ATE_signed);

	dwarf2_base_type(integer_type(ik_int), "int", DW_ATE_signed);
	dwarf2_base_type(integer_type(ik_unsigned_int), "unsigned int",
			DW_ATE_unsigned);
	dwarf2_base_type(signed_integer_type(ik_int), "signed int", DW_ATE_signed);

	dwarf2_base_type(integer_type(ik_long), "long", DW_ATE_signed);
	dwarf2_base_type(integer_type(ik_unsigned_long), "unsigned long",
			DW_ATE_unsigned);
	dwarf2_base_type(signed_integer_type(ik_long), "signed long", DW_ATE_signed);
#if LONG_LONG_ALLOWED
	dwarf2_base_type(integer_type(ik_long_long), "long long", DW_ATE_signed);
	dwarf2_base_type(integer_type(ik_unsigned_long_long), "unsigned long long",
				DW_ATE_unsigned);
	dwarf2_base_type(signed_integer_type(ik_long_long), "signed long long",
				DW_ATE_signed);
#endif /* LONG_LONG_ALLOWED */

	dwarf2_base_type(float_type(fk_float), "float", DW_ATE_float);
	dwarf2_base_type(float_type(fk_double), "double", DW_ATE_float);
	dwarf2_base_type(float_type(fk_long_double), "long double", DW_ATE_float);

	dwarf2_base_type(wchar_t_type(), "wchar_t", DW_ATE_signed);
	dwarf2_base_type(bool_type(), "bool", DW_ATE_boolean);
	dwarf2_base_type(void_type(), "void", DW_ATE_unsigned);
	dwarf2_base_type(make_pointer_type(void_type()), "void *", DW_ATE_address);
}  /* dwarf2_fund_types */

static void dwarf2_fund_types_2(void)
/* Generate all the types based off the fundamental types (char *, etc.)
** This is delayed until the end of the compilation unit, when all the types
** used are known.
*/
{
	dw_gen_based_types(integer_type(ik_char));
	dw_gen_based_types(integer_type(ik_unsigned_char));
	dw_gen_based_types(integer_type(ik_signed_char));
	dw_gen_based_types(integer_type(ik_short));
	dw_gen_based_types(integer_type(ik_unsigned_short));
	dw_gen_based_types(signed_integer_type(ik_short));
	dw_gen_based_types(integer_type(ik_int));
	dw_gen_based_types(integer_type(ik_unsigned_int));
	dw_gen_based_types(signed_integer_type(ik_int));
	dw_gen_based_types(integer_type(ik_long));
	dw_gen_based_types(integer_type(ik_unsigned_long));
	dw_gen_based_types(signed_integer_type(ik_long));
#if LONG_LONG_ALLOWED
	dw_gen_based_types(integer_type(ik_long_long));
	dw_gen_based_types(integer_type(ik_unsigned_long_long));
	dw_gen_based_types(signed_integer_type(ik_long_long));
#endif /* LONG_LONG_ALLOWED */

	dw_gen_based_types(float_type(fk_float));
	dw_gen_based_types(float_type(fk_double));
	dw_gen_based_types(float_type(fk_long_double));
	dw_gen_based_types(wchar_t_type());
	dw_gen_based_types(bool_type());
	dw_gen_based_types(void_type());
}  /* dwarf2_fund_types_2 */

static void dwarf2_enum_list(a_constant_ptr cptr)
/* Output a list of enumerator entries */
{
	for (; cptr; cptr = cptr->next) {
		if (cptr->kind != ck_integer) {
			/* error already reported */
			continue;
		} /* if */
		dw_s_entry(DW2_enumerator, &cptr->source_corresp, "enumerator");
		dwarf_out("\tlsC", (signed int)cptr->variant.integer_value,
			  "DW_AT_const_value");
		dw_at_name(cptr->source_corresp.name);
		dw_e_entry();
	}  /* for */
}  /* dwarf2_enum_list */

static void dwarf1_enum_list(a_constant_ptr cptr)
/* Generate value, then string, for each enumerator. In Dwarf 1,
** enumeration constants are put out in reverse order from the way
** they appear in the source. */
{
	if (cptr->kind != ck_integer) {
		/* error already reported */
		return;
	} /* if */
	if (cptr->next)
		dwarf1_enum_list(cptr->next);
	dwarf_out("\tW\td\nb\n", (signed int)cptr->variant.integer_value,
		cptr->source_corresp.name);
} /* dwarf1_enum_list */

static void dw_enum_def(a_type_ptr type)
/* create definition for an enum type and each of its literals */
/* sptr->defined may be true even if there are no constants (enum E {};) */
{
	a_constant_ptr	cptr = type->variant.integer.enum_info.constant_list;
	a_symbol_ptr	sptr = (a_symbol_ptr)type->source_corresp.assoc_info;
	a_boolean	definition = (cptr || sptr->defined);

	if (definition) {
		dw_s_entry(DW2_enumeration, &type->source_corresp, "enumeration");
		if (IS_DWARF_2())
			dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN,
				"DW_AT_sibling");
		dw_at_name(type->source_corresp.name);
		dw_at_byte_size(type->size);
	} else {
		dw_s_entry(DW2_incomplete_enumeration, &type->source_corresp,
				"incomplete enumeration");
		dw_at_name(type->source_corresp.name);
		if (IS_DWARF_2())
			dwarf_out("\tB\txC", /*TRUE*/ 1, "DW_AT_declaration");
	}  /* if */
	if (IS_DWARF_1()) {
		if (cptr) {
			/* Dwarf 1 - enumerators are an attribute of the enumeration
			** type.  Generate attribute, length, label for stuff. */
			int lab = DB_GENLAB();
			dwarf_out("\tH\txC\tW\tLD-Ld\n.d", AT_element_list,
				"AT_element_list", lab, lab, lab);
			/* Generate each enumerator. */
			dwarf1_enum_list(cptr);
			/* Generate end label */
			dwarf_out(".D", lab);
		} /* if */
		dw_e_entry();
	} else {
		/* Dwarf 2 */
		dw_e_entry();
		if (definition) {
			dw_push();		/* new level for members */
			dwarf2_enum_list(cptr);
			(void)dw_pop();
		} /* if */
	}  /* if */
} /* dw_enum_def */

static void dw_gen_member(a_field_ptr field)
/* Output debug information for each data member of a c/s/u. */
{
	Abbrev_index	tag = field->is_bit_field ? DW2_bit_field : DW2_csu_member;
	a_type_ptr	type = field->type;
	int		i;
	const Dwarf2_Attribute	*ab_entry;

	if (field->is_bit_field) {
		type = skip_typerefs(type);
		if (targ_plain_int_bit_field_is_unsigned
			&& !type->variant.integer.bool_type) {
			switch (type->variant.integer.int_kind) {
			case ik_char:
				type = integer_type(ik_unsigned_char);
				break;
			case ik_short:
				type = integer_type(ik_unsigned_short);
				break;
			case ik_int:
				type = integer_type(ik_unsigned_int);
				break;
			case ik_long:
				type = integer_type(ik_unsigned_long);
				break;
#if LONG_LONG_ALLOWED
			case ik_long_long:
				type = integer_type(ik_unsigned_long_long);
				break;
#endif /* LONG_LONG_ALLOWED */
			} /* switch */
		} /* if */
	} /* if */

	/* Start new entry. */
	dw_s_entry(tag, &field->source_corresp, "data member");
	ab_entry = dw_abbrev_table[tag].attributes;
	for (i = dw_abbrev_table[tag].nattr; i; --i, ++ab_entry) {
		switch (ab_entry->name) {
		case DW_AT_name:
			dw_at_name(field->source_corresp.name);
			break;
		case DW_AT_byte_size:
			dw_at_byte_size(type->size);
			break;

		case DW_AT_bit_size:
			if (IS_DWARF_1())
				dwarf_out("\tH\tx; W\tdC", AT_bit_size,
					field->bit_size,"AT_bit_size");
			else
				dwarf_out("\tluC", field->bit_size, "DW_AT_bit_size");
			break;
		case DW_AT_bit_offset:
		{
			a_targ_size_t offset;
			if (targ_little_endian)
				offset = (type->size * targ_char_bit)
					- (field->offset_bit_remainder
						+ field->bit_size);
			else
				offset = field->offset_bit_remainder;
			if (IS_DWARF_1())
				dwarf_out("\tH\tx; H\tdC", AT_bit_offset, offset,
					"AT_bit_offset");
			else
				dwarf_out("\tluC", offset, "DW_AT_bit_offset");
			break;
		}
		case DW_AT_type:
			dw_at_type(type);
			break;
		case DW_AT_SCO_mutable:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", field->is_mutable,
					"DW_AT_SCO_mutable");
			break;
		case DW_AT_artificial:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC",
					(field->source_corresp.decl_position.seq == 0),
					"DW_AT_artificial");
			break;
		case DW_AT_data_member_location:
			/* location attribute; address of start of s/u is implicit. */
			if (IS_DWARF_1()) {
				/* Dwarf 1 */
				dwarf_out("\tH\tx; H d; B x; W d; B xC",
					AT_location, 1+4+1, OP_CONST,
					field->offset, OP_ADD,
					"AT_location: OP_CONST val OP_ADD");
			} else {
				/* Dwarf 2 */
				dw2_out_loc_desc("DW_AT_data_member_location",
						DW_FORM_block1, DW_OP_plus_uconst,
						field->offset, DB_DW2_END);
			} /* if */
			break;
		default:
			internal_error("dw_gen_member(): bad attribute");
			break;
		} /* switch */
	} /* for */
	dw_e_entry();
} /* dw_gen_member */

static void dw_gen_vtbl(a_variable_ptr vtbl_var, a_virtual_function_number nslots)
/* Generate debugging entry for virtual function table */
{
	a_type_ptr	type = vtbl_var->type;
	a_type_ptr	element_type = type->variant.array.element_type;
	a_boolean	external = vtbl_var->storage_class != sc_static;

	/* generate the struct type now, since it may not be on the file scope
	** type list, if no vtbls are being generated in this compilation unit.
	** The struct type is needed for the "vptr" class member, even if the
	** vtbl itself is not generated
	*/
	dw_typedef(element_type);

	/* IL lowering determines whether or not a vtbl should be generated,
	** and sets the storage class appropriately.
	*/
	if (vtbl_var->storage_class == sc_extern
		|| (vtbl_var->storage_class == sc_static
			&& !vtbl_var->source_corresp.referenced))
		return;

	/* Generate array type entry */
	dw_gen_array(type, nslots);

	/* Start new entry for the variable. */
	if (IS_DWARF_1()) {
		dw1_s_entry(external ? TAG_global_variable : TAG_local_variable,
				&vtbl_var->source_corresp, /*scope=*/0,
				"vtbl entry");
	} else {
		dw_s_entry(DW2_artif_variable, &vtbl_var->source_corresp,
				"vtbl entry");
	} /* if */
	dw_at_name(vtbl_var->source_corresp.name);
	dw_at_type(type);
	if (IS_DWARF_2()) {
		dwarf_out("\tB\txC", 1 /* TRUE */, "DW_AT_artificial");
		dwarf_out("\tB\txC", external, "DW_AT_external");
	} /* if */
	dw_mark_variable(vtbl_var, FALSE);
	dw_e_entry();

	if (dwarf_name_lookup && external) {
		dw_name_buf_used = 0;
		form_symbol_name((a_symbol_ptr)vtbl_var->source_corresp.assoc_info,
				&octl);
		/* Output the .debug_pubnames entry for this variable. */
		dwarf_out("Sn\tW\tLd-sCb\nSp", DB_CURINFOLAB, LAB_INFOBEGIN,
			".debug pubnames variable entry", dw_name_buf);
	}  /* if */
} /* dw_gen_vtbl */

static void dw_gen_parameters(a_scope_ptr scope, a_type_ptr return_type,
				a_routine_type_supplement_ptr extra,
				a_boolean gen_exception_spec, a_boolean gen_types)
/* Generate formal parameters (for a definition) or just the types (for a
** function declaration).  For member functions, gen_types will be TRUE for the
** declaration (in the class definition) and FALSE for the function definition
** (at file scope level), so that any types that might be generated on the fly
** (arrays, function pointers, etc.) will be generated in the declaration and
** not in the definition.  This is necessary since the back end will delete
** entries for unreferenced inline functions, and that would leave dangling
** type references in the function declaration.
*/
{
	/* Debug info entries for formals are children of the subroutine entry.
	** Note: Any types introduced in the parameter list such as array types
	**	will appear in the children list, intermixed with formal parameters
	*/
	a_param_type_ptr	ptype = extra->param_type_list;
	if (scope) {
		/* real function definition - put out formal parameters */
		a_variable_ptr  var;
		if ((var = scope->variant.routine.this_param_variable) != 0)
			dw_variable(var, scope, gen_types);
		if (extra->value_returned_by_cctor) {
			/* There is an implicit parameter for the return value address*/
			a_type_ptr		ret_type = make_pointer_type(return_type);
			if (gen_types)
				dw_walktype(ret_type);
			dw_s_entry(DW2_artif_formal_param,
				/*source_corresp=*/0,
				"artificial formal parameter - return value");
			return_value_ptr_id = DB_CURINFOLAB;
			dw_at_name("");
			dw_at_type(ret_type);
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1 /*TRUE*/, "DW_AT_artificial");
			dwarf_out("\tC", "location - generated by c++be");
			return_value_ptr_mark = dw_new_mark(iek_variable);
			return_value_ptr_mark->offset = ftell(dwarf_dbout);
			return_value_ptr_mark->return_value_ptr = TRUE;
			dw_e_entry();
		}  /* if */
		for (var = scope->variant.routine.parameters; var;
			var = var->next, ptype = ptype->next) {
			if (ptype->passed_via_copy_constructor) {
				/* space is reserved in the calling function
				** and the address passed on the stack, making
				** this look like a reference object
				*/
				a_type_ptr save_type = var->type;
				var->type = make_reference_type(save_type);
				dw_variable(var, scope, gen_types);
				var->type = save_type;
			} else {
				dw_variable(var, scope, gen_types);
			} /* if */
		} /* for */
	} else {
		a_type_ptr		this_param_type;

		/* declaration - types only available */
		if ((this_param_type = extra->implicit_this_param_type) != 0) {
			dw_walktype(this_param_type);
			dw_s_entry(DW2_artif_formal_param_type,
				/*source_corresp=*/0,
				"artificial formal parameter - this");
			dw_at_type(this_param_type);
			if (IS_DWARF_2()) {
				dwarf_out("\tB\txC", 1 /*TRUE*/, "DW_AT_artificial");
			} else {
				/* Dwarf 1 */
				dw_at_name("this");
			} /* if */
			dw_e_entry();
		} /* if */
 		if (extra->value_returned_by_cctor) {
  			/* There is an implicit parameter for the return value address*/
			a_type_ptr ret_type = make_pointer_type(return_type);
			dw_walktype(ret_type);
			dw_s_entry(DW2_artif_formal_param_type,
				/*source_corresp=*/0,
				"artificial formal parameter - return value");
			dw_at_type(ret_type);
			if (IS_DWARF_2()) {
				dwarf_out("\tB\txC", 1 /*TRUE*/, "DW_AT_artificial");
			} /* if */
			dw_e_entry();
		}  /* if */
	
		for (; ptype; ptype = ptype->next) {
			a_type_ptr paramt = ptype->type;
			if (ptype->passed_via_copy_constructor)
				paramt = make_reference_type(paramt);
			dw_walktype(paramt);
			dw_s_entry(DW2_formal_param_type,
				/*source_corresp=*/0,
				"formal parameter type");
			dw_at_type(paramt);
			dw_e_entry();
		} /* for */
	} /* if */

	/* Represent "...". */
	if (extra->has_ellipsis) {
		dw_s_entry(DW2_unspecified_params, /*source_corresp=*/0,
				"unspecified parameters");
		dw_e_entry();
	} /* if */

	if (IS_DWARF_2() && gen_exception_spec && extra->exception_specification) {
		/* not supported by Dwarf 1 */
		an_exception_specification_type_ptr type_list
				 = extra->exception_specification
						->exception_specification_type_list;
		for (; type_list; type_list = type_list->next) {
			if (type_list->redundant)
				continue;
			if (gen_types)
				dw_walktype(type_list->type);
			dw_s_entry(DW2_thrown_type, /*source_corresp=*/0,
				"exception specification");
			dw_at_type(type_list->type);
			dw_e_entry();
		} /* for */
	} /* if */
} /* dw_gen_parameters */

void dwarf_fixup_for_return_value_pointer_variable(a_variable_ptr var)
{
	if (return_value_ptr_mark)
		return_value_ptr_mark->variant.variable = var;
	var->source_corresp.debug_id = return_value_ptr_id;
} /* dwarf_fixup_for_return_value_pointer_variable */

static void dw_member_function(a_routine_ptr routine)
/* Generate debugging info for a member function declaration */
{
	Abbrev_index		tag;
	a_type_ptr		rettype = routine->type->variant.routine.return_type;

	if (rettype && rettype->kind == tk_void)
		rettype = 0;
	tag = member_function_tags[routine->compiler_generated]
					[rettype != 0][routine->is_virtual];
	dw_beg_function(tag, routine, /*is_definition=*/ FALSE);

	/* Put out the parameter types */
	dw_push();
	dw_gen_parameters(/*scope=*/0, rettype,
			routine->type->variant.routine.extra_info,
			/*gen_exception_spec=*/ TRUE, /*gen_types=*/ TRUE);
	(void)dw_pop();
} /* dw_member_function */

static void dw_using_directives(a_using_decl_ptr uptr)
/* Generate entries for using directives and using declarations - Dwarf 2 only */
{
	for (; uptr; uptr = uptr->next) {
		if (uptr->is_using_directive) {
			dw_s_entry(DW2_using_directive, /*source_corresp=*/0,
					"using directive");
			dw_at_reference(DW_AT_SCO_namespace,
					(a_source_correspondence *)uptr->entity.ptr,
					"DW_AT_SCO_namespace");
		} else {
			dw_s_entry(DW2_using_declaration, /*source_corresp=*/0,
					"using declaration");
			dw_at_reference(DW_AT_SCO_using,
					(a_source_correspondence *)uptr->entity.ptr,
					"DW_AT_SCO_using");
		} /* if */
		dw_e_entry();
	} /* for */
} /* dw_using_directives */

static void dw_gen_constant(a_constant_ptr cptr)
/* Generate a constant - Dwarf2 only.  This is only needed to generated
** information about template value parameters.  The FORM in the abbreviation
** table is FORM_indirect, allowing this function to generate the FORM,
** dependent upon the type of constant, as part of the attribute itself.
*/
{
	switch (cptr->kind) {
	case ck_integer:
		dwarf_out("\tluC", DW_FORM_sdata, "DW_FORM_sdata");
#if MAX_INTEGER_VALUE > LONG_MAX
		dwarf_out("\tlSC",
			cptr->variant.integer_value,
			"DW_AT_const_value: integer");
#else
		dwarf_out("\tlsC",
			(int)cptr->variant.integer_value,
			"DW_AT_const_value: integer");
#endif /* MAX_INTEGER > LONG_MAX */
		break;
	case ck_address:
		dwarf_out("\tluC", DW_FORM_addr, "DW_FORM_addr");
		switch (cptr->variant.address.kind) {
		case abk_routine:
			dw_mark_routine(cptr->variant.address
					.variant.routine);
			break;
		case abk_variable:
			dw_mark_variable(cptr->variant.address
					.variant.variable, TRUE);
			break;
		default:
			internal_error("dw_gen_constant: unknown address constant");
		} /* end switch */
		break;
	case ck_ptr_to_member:
		if (cptr->variant.ptr_to_member.is_function_ptr) {
			a_targ_ptrdiff_t delta;
			a_targ_ptrdiff_t index;
			a_routine_ptr    func = 0;
			a_targ_ptrdiff_t offset;
			repr_for_ptr_to_member_function_constant(cptr,
					&delta, &index, &func, &offset);
			dwarf_out("\tlu;\tB xC", DW_FORM_block1,
					make_mptr_type()->size,
					"DW_FORM_block1: ptr-to-function-member");
			if (TARG_DELTA_INT_KIND == ik_short)
				dwarf_out("\tH x;\tH x\n", (unsigned short)delta,
					(unsigned short)index);
			else
				dwarf_out("\tW x;\tH x,x\n", (unsigned long)delta,
					(unsigned short)index, /*padding*/ 0);
			if (func)
				dw_mark_routine(func);
			else
				dwarf_out("\tW x\n", offset);
		} else {
			a_field_ptr field = cptr->variant.ptr_to_member
						.variant.field;
			dwarf_out("\tluC", DW_FORM_udata, "DW_FORM_udata");
			dwarf_out("\tluC",
				field ? (field->offset + 1) : 0,
				"DW_AT_const_value: ptr-to-data-member");
		} /* if */
		break;
	default:
		internal_error("dw_gen_constant: bad constant kind\n");
	} /* switch */
} /* dw_gen_constant */

static void dw_gen_template_args(a_template_arg_ptr arg, a_template_param_ptr formal)
/* Generate entries for template parameters - Dwarf2 only */
{
	for (; arg && formal; arg = arg->next, formal = formal->next) {
		if (arg->is_type) {
			dw_walktype(arg->variant.type);
			dw_s_entry(DW2_template_type_param, /*source_corresp=*/ 0,
					"template type argument");
			dw_at_name(formal->param_symbol->header->identifier);
			dw_at_type(arg->variant.type);
			dw_e_entry();
		} else {
			a_constant_ptr	cptr = arg->variant.constant;
			dw_walktype(cptr->type);
			dw_s_entry(DW2_template_value_param, /*source_corresp=*/ 0,
					"template value param");
			dw_at_name(formal->param_symbol->header->identifier);
			dw_at_type(cptr->type);
			dw_gen_constant(cptr);
			dw_e_entry();
		} /* if */
	} /* for */
} /* dw_gen_template_args */

static void dw_gen_base_class(a_base_class_ptr base)
/* Generate an inheritance entry for a base class */
{
	a_targ_size_t	offset;
	int		access;

	if (base->is_virtual) {
		offset = base->pointer_offset;
		if (base->pointer_base_class) {
			offset += base->pointer_base_class->offset;
		}
	} else {
		offset = base->offset;
	} /* if */
	dw_s_entry(DW2_inheritance, /*source_corresp=*/ 0, "inheritance");
	if (IS_DWARF_1()) {
		dw_at_reference(AT_user_def_type, &base->type->source_corresp,
				"base_type");
		if (base->derivation->access == as_public)
			access = AT_public;
		else if (base->derivation->access == as_protected)
			access = AT_protected;
		else
			access = AT_private;
		/* The existence of the attribute is sufficient, data is unused */
		dwarf_out("\tH\tx; B\txC", access, 0, "accessibility");
		if (base->is_virtual) {
			dwarf_out("\tH\tx; B\txC", AT_virtual, 0, "AT_virtual");
			dwarf_out("\tH\tx; H d; B x; W d; B x,xC",
				AT_location, 1+4+1+1, OP_CONST, offset,
				OP_ADD, OP_DEREF, 
				"AT_location: OP_CONST(val) OP_ADD OP_DEREF");
		} else {
			dwarf_out("\tH\tx; H d; B x; W d; B xC",
				AT_location, 1+4+1, OP_CONST, offset,
				OP_ADD, "AT_location: OP_CONST(val) OP_ADD");
		} /* if */
	} else {
		/* Dwarf 2 */
		dw_at_type(base->type);
		if (base->derivation->access == as_public)
			access = DW_ACCESS_public;
		else if (base->derivation->access == as_protected)
			access = DW_ACCESS_protected;
		else
			access = DW_ACCESS_private;
		dwarf_out("\tB\txC", access, "DW_AT_accessibility");
		if (base->is_virtual) {
			dwarf_out("\tB\txC", DW_VIRTUALITY_virtual,"DW_AT_virtuality");
			dw2_out_loc_desc("DW_AT_data_member_location", DW_FORM_block1,
				DW_OP_plus_uconst, offset, DW_OP_deref, DB_DW2_END);
		} else {
			dwarf_out("\tB\txC", DW_VIRTUALITY_none, "DW_AT_virtuality");
			dw2_out_loc_desc("DW_AT_data_member_location", DW_FORM_block1,
				DW_OP_plus_uconst, offset, DB_DW2_END);
		} /* if */
	} /* if */
	dw_e_entry();
} /* dw_gen_base_class */

static void dw_struct_def(a_type_ptr type)
/* Output member information for c/s/u .
*/
{
	a_field_ptr			field;
	a_scope_ptr			scope;
	a_base_class_ptr		base;
	a_routine_ptr			routine;
	a_class_type_supplement_ptr	suppl;
	a_variable_ptr			var;
	Abbrev_index			tagno;

	if (type->source_corresp.debug_id
		&& !type->source_corresp.debug_id_is_temporary)
		return;

	/* do the partial lowering to get vtbl information */
	il_lowering_underway = TRUE;
	prelower_class_type(type);
	il_lowering_underway = FALSE;

	suppl = type->variant.class_struct_union.extra_info;
	/* Add entry for virtual function table, if any, to list to be put out later */
	if (suppl && suppl->virtual_function_table_var) {
		a_vtbl_link *link
			= (a_vtbl_link *)alloc_fe(sizeof(a_vtbl_link));
		link->next = 0;
		link->vtbl = suppl->virtual_function_table_var;
		link->nslots = suppl->highest_virtual_function_number + 1;
		if (vtbl_tail)
			vtbl_tail->next = link;
		else
			vtbl_head = link;
		vtbl_tail = link;
	} /* if */

	/* Begin entry. */
	if (type->size) {
		switch (type->kind) {
		case tk_class:	tagno = DW2_class; break;
		case tk_struct:	tagno = DW2_struct; break;
		case tk_union:	tagno = DW2_union; break;
		}  /* switch */
		dw_s_entry(tagno, &type->source_corresp, "class, struct, or union");
		if (IS_DWARF_2())
			dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN,
				"DW_AT_sibling");
	} else {
		switch (type->kind) {
		case tk_class:	tagno = DW2_incomplete_class; break;
		case tk_struct:	tagno = DW2_incomplete_struct; break;
		case tk_union:	tagno = DW2_incomplete_union; break;
		}  /* switch */
		dw_s_entry(tagno, &type->source_corresp,
				"incomplete class, struct, or union");
	}  /* if */

	dw_at_name(type->source_corresp.name);
	if (type->size) {
		dw_at_byte_size(type->size);
	} else if (IS_DWARF_2()) {
		/* generate an AT_declaration. */
		dwarf_out("\tB\txC", 1, "DW_AT_declaration");
	}  /* if */
	dw_e_entry();
	if (!type->size)
		return;

	scope = suppl ? suppl->assoc_scope : 0;

	dw_push();	/* new level for members */

	/* put out template information */
	if (IS_DWARF_2()) {
		a_symbol_ptr	sym = (a_symbol_ptr)type->source_corresp.assoc_info;
		a_class_symbol_supplement_ptr sym_suppl = sym
					? sym->variant.class_struct_union.extra_info
					: 0;
		if (sym_suppl && sym_suppl->is_instance) {
			a_template_symbol_supplement_ptr tptr
				= sym_suppl->class_template->variant.template_info;
			dw_gen_template_args(suppl->template_arg_list,
					tptr->cache.decl_info->parameters);
		} /* if */
	} /* if */

	/* put out derivation information */
	base = suppl ? suppl->base_classes : 0;
	for(; base; base = base->next) {
		if (base->direct) {
			/* only put out direct base classes */
			dw_gen_base_class(base);
		} /* if */	
	} /* for */

	if (scope) {
		dw_type_list(scope->types);
		/* put out any using declarations for base class members */
		if (scope->using_decls && IS_DWARF_2())
			dw_using_directives(scope->using_decls);
	} /* if */

	/* get non-static data members */
	for (field = type->variant.class_struct_union.field_list;
		field; field = field->next) {
		if (!field->source_corresp.assoc_info
			&& field->source_corresp.name
			&& strncmp(field->source_corresp.name, "__b_",
					strlen("__b_")) == 0) {
			/* created during lowering phase */
			continue;
		} /* if */
		dw_walktype(field->type);
		dw_gen_member(field);
	} /* for */

	if (scope) {
		/* generate static data members */
		for (var = scope->variables; var; var = var->next)
			dw_variable(var, scope, /*gen_type=*/TRUE);

		/* put out declarations of member functions - definitions
		** are generated by memory region
		*/
		for (routine = scope->routines; routine; routine = routine->next)
			dw_member_function(routine);
	} /* if */

	(void)dw_pop();
} /* dw_struct_def */

static void dwarf_outfunction(a_type_ptr type)
/* Output debug information entry for a function type. */
{
	a_routine_type_supplement_ptr suppl = type->variant.routine.extra_info;
	a_type_ptr		ret_type = type->variant.routine.return_type;
	Abbrev_index		tag;

	if (ret_type && ret_type->kind != tk_void) {
		/* output return sub-type stuff */
		dw_walktype(type->variant.routine.return_type);
		tag = DW2_non_void_subr_type;
	} else {
		tag = DW2_void_subr_type;
	} /* if */

	dw_s_entry(tag, &type->source_corresp, "subroutine type");
	if (IS_DWARF_2())
		dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN, "DW_AT_sibling");
	if (ret_type && ret_type->kind != tk_void)
		dw_at_type(ret_type);		/* describe the returned type */
	if (IS_DWARF_2())
		dwarf_out("\tB\txC", suppl->prototyped, "DW_AT_prototyped");
	dw_e_entry();

	/* Debug info entries for formals are children of the subroutine entry.
	** Put out types only */
	dw_push();
	dw_gen_parameters(/*scope=*/0, ret_type, suppl, /*gen_exception_spec=*/TRUE,
		/*gen_types=*/TRUE);
	(void)dw_pop();
}  /* dwarf_outfunction */

static void dw_typeref(a_type_ptr type)
/* Generate a typedef entry.  Also, for Dwarf2, generate const and volatile
** type entries.  For Dwarf1, const and volatile do not get separate entries,
** and are handled in dw_at_type.
*/
{
	if (!typeref_is_qualified(type)) {
		if (type->variant.typeref.is_placeholder_for_class_instantiation
			|| type->variant.typeref.is_placeholder_for_namespace_type
			|| type->variant.typeref.is_placeholder_for_nested_class_def) {
			if (type->source_corresp.debug_id_is_temporary) {
				/* type is referenced - shouldn't happen */
				internal_error("dw_typeref: placeholder is referenced");
			} /* if */
			return;
		} /* if */
		dw_s_entry(DW2_typedef, &type->source_corresp, "typedef");
		dw_at_name(type->source_corresp.name);
		dw_at_type(type->variant.typeref.type);
	} else if (IS_DWARF_1()) {
		/* Dwarf1 does not allow for separate entries for cv-qualifed types */
		return;
	} else if (typeref_is_const_qualified(type)
			&& typeref_is_volatile_qualified(type)) {
		/* One IL type structure translates into two separate dwarf2 entries */
		dw_s_entry(DW2_const_type, &type->source_corresp, "const type");
		dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN, "DW_AT_type");
		dw_e_entry();
		dw_s_entry(DW2_volatile_type, /*source_corresp=*/ 0, "volatile type");
		dw_at_type(type->variant.typeref.type);
	} else if (typeref_is_const_qualified(type)) {
		dw_s_entry(DW2_const_type, &type->source_corresp, "const type");
		dw_at_type(type->variant.typeref.type);
	} else if (typeref_is_volatile_qualified(type)) {
		dw_s_entry(DW2_volatile_type, &type->source_corresp, "volatile type");
		dw_at_type(type->variant.typeref.type);
	} /* if */
	dw_e_entry();
} /* dw_typeref */

static void dw_gen_array(a_type_ptr type, int nelements)
/* Generate an array type entry.  nelements is set only for vtbl entries,
** otherwise, the size is taken from the type itself.
*/
{
	/* array types are put out when seen */
	a_type_ptr	subrange_type;
	a_type_ptr	element_type = type->variant.array.element_type;
	/* Put out the element type if necessary.  Skip the subranges for now,
	** they aren't put out as individual types */
	while (element_type->kind == tk_array)
		element_type = element_type->variant.array.element_type;
	dw_walktype(element_type);

	/* Output a debug information entry for an array. */
	dw_s_entry(DW2_array, &type->source_corresp, "array");
	if (IS_DWARF_2()) {
		dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN, "DW_AT_sibling");
		dw_at_type(element_type);
		if (!type->size && nelements)
			dw_at_byte_size(nelements * element_type->size);
		else
			dw_at_byte_size(type->size);
		dw_e_entry();
	
		/* Subscripting information is expressed in terms of
		** children subranges which specify the number of elements
		** in that dimension.  If the upper bound is unknown, the
		** count is specified as 0.
		*/
		dw_push();
		if (nelements) {
			/* generating a vtbl type, only one dimension */
			dw_s_entry(DW2_subrange, /*source_corresp=*/ 0, "subrange");
			dwarf_out("\tluC", nelements, "DW_AT_count");
			dw_e_entry();
		} else {
			subrange_type = type;
			while (subrange_type->kind == tk_array) {
				dw_s_entry(DW2_subrange, /*source_corresp=*/ 0,
					"subrange");
				dwarf_out("\tluC", subrange_type->variant.array
							.variant.number_of_elements,
					"DW_AT_count");
				subrange_type = subrange_type->variant.array
							.element_type;
				dw_e_entry();
			} /* while */
		} /* if */
		(void)dw_pop();
	} else {
		/* Dwarf 1 - Subscript information looks like this for C:
		**		1	format (FMT_FT_C_C)
		**		2	type (int)
		**		4	low bound (0)
		**		4	high bound (number of elements - 1)
		**	       --
		**
		** If the upper bound is unknown, the format is FMT_FT_C_X, with
		** an empty high bound expression.
		**
		** Following the subscript stuff comes FMT_ET, the element type,
		** which is an embedded at_type.
		*/
		int		dimlab;	/* label number for start of dimensions */
		a_targ_size_t	nelem;

		dwarf_out("\tH\tx; H\txC", AT_ordering, ORD_row_major,
			"AT_ordering:  row major");
		dwarf_out("\tH\txC", AT_subscr_data, "AT_subscr_data");
		dimlab = DB_GENLAB();
		/* length of subscript data */
		dwarf_out("\tH\tLD-Ld\n", dimlab, dimlab);
		dwarf_out(".d", dimlab);  /* remember start of subscript data */
		if (nelements) {
			/* creating a vtbl type, only one dimension */
			dwarf_out("\tB\tx; H\tx; W\tsd\n",
				FMT_FT_C_C, FT_signed_integer,
					"0,", nelements-1);
		} else {
			subrange_type = type;
			while (subrange_type->kind == tk_array) {
				nelem = subrange_type->variant.array.variant
							.number_of_elements;
				if (nelem == 0)
					dwarf_out("\tB\tx; H x; W s; H sC",
						FMT_FT_C_X, FT_signed_integer,
						"0", "0", "no bound");
				else
					dwarf_out("\tB\tx; H\tx; W\tsd\n",
						FMT_FT_C_C, FT_signed_integer,
						"0,", nelem-1);
				subrange_type = subrange_type->variant.array
							.element_type;
			} /* while */
		} /* if */

		dwarf_out("\tB\txC", FMT_ET, "FMT_ET");	/* output element type */
		dw_at_type(element_type);
		dwarf_out(".D", dimlab);	/* end of subscript data */
		dw_e_entry();
	} /* if */
} /* dw_gen_array */

static void dw_ptr_to_member(a_type_ptr type)
/* Generate an entry for a pointer-to-member type */
{
	static a_debug_entry_id	func_loc_desc = 0;

	a_type_ptr base_type = type->variant.ptr_to_member.type;
	a_type_ptr class_type = type->variant.ptr_to_member.class_of_which_a_member;
	Abbrev_index tag;

	dw_walktype(class_type);

	/* DW2_ptr_to_member and DW2_ptr_to_function_member are identical except
	** for the location attribute.  In DW2_ptr_to_function_member it has
	** form DW_FORM_ref4, and is a reference to another entry where the
	** full location description has already been generated
	*/
	if (is_function_type(base_type)
		&& class_type->variant.class_struct_union.any_virtual_functions
		&& func_loc_desc)
		tag = DW2_ptr_to_function_member;
	else
		tag = DW2_ptr_to_member;
	dw_s_entry(tag, &type->source_corresp, "ptr to member");
	dw_at_type(base_type);
	dw_at_reference(AT_containing_type, &class_type->source_corresp,
				"containing_type");
	dw_at_byte_size(type->size);

	if (IS_DWARF_1()) {
		dw_e_entry();
		return;
	} /* if */

	/* Assumes location stack for a pointer-to-function member looks like this:
	** top 	  [ object address ]
	**	  [ function pointer or vtbl offset ]
	** bottom [ short delta/short vtbl index ]
	** or
	** top 	  [ object address ]
	**	  [ function pointer or vtbl offset ]
	**        [ (short) vtbl index/padding ]
	** bottom [ delta ]
	**
	** stack should be left looking like this:
	** top	[ function address ]
	** 	[ this pointer ]
	**
	** For a pointer-to-data members, the stack will look like this (before):
	** top	  [ object address ]
	** bottom [ ptr-to-mem value ]
	**
	** and after:
	** top	[ data address ]
	*/
	class_type = skip_typerefs(class_type);
	if (!is_function_type(base_type)) {
		dw2_out_loc_desc("DW_AT_use_location",
			DW_FORM_block1,
			DW_OP_swap,	/* ptr-to-mem value on top of stack */
			DW_OP_const1u, 1,
			DW_OP_minus,
			DW_OP_plus,	/* object + (p-t-m-value - 1) */
			DB_DW2_END);
	} else if (!class_type->variant.class_struct_union.any_virtual_functions) {
		if (TARG_DELTA_INT_KIND == ik_short) {
			dw2_out_loc_desc("DW_AT_use_location",
				DW_FORM_block1,
				DW_OP_rot,	/* func. address on top */
				DW_OP_rot,	/* delta/vtbl index on top */
				DW_OP_const1u, 16,	/* push literal 16 */
				DW_OP_shr,	/* shift to get delta value */
				DW_OP_plus,	/* address + delta => this pointer */
				DB_DW2_END);
		} else {
			dw2_out_loc_desc("DW_AT_use_location",
				DW_FORM_block1,
				DW_OP_pick, 3,	/* copy delta to top */
				DW_OP_plus,	/* address + delta => this pointer */
				DW_OP_swap,	/* function address on top */
				DB_DW2_END);
		}
	} else if (func_loc_desc) {
		/* Location description already generated.  Since the algorithm
		   is the same regardless of class type, reuse rather than making
		   another copy */
		dwarf_out("\tW\tLd-sC", func_loc_desc, LAB_INFOBEGIN,
				"DW_AT_use_location");
	} else if (TARG_DELTA_INT_KIND == ik_long) {
		func_loc_desc = DB_GENLAB();
		dwarf_out(".d", func_loc_desc);
		dw2_out_loc_desc("DW_AT_use_location",
			DW_FORM_block1,
			DW_OP_pick, 3,	/* 1 - copy delta to top */
			DW_OP_plus,	/* 2 - pushes this pointer on top */
			DW_OP_rot,	/* 3 - func. address on top */
			DW_OP_rot,	/* 4 - vtbl index/padding on top */
			DW_OP_const1u, 16,	/* 5 - push literal 16 */
			DW_OP_shr,	/* 6 - shift to get index value */
			DW_OP_dup,	/* 7 */
			DW_OP_const1u, 0,
			DW_OP_lt,	/* 8 - if negative vtbl index, non-virtual case */
			DW_OP_bra, 4,	/* 9 - skip to virtual case */
				/* non-virtual case */
			DW_OP_drop,	/* 10 - drop unneeded vtbl index */
			DW_OP_swap,	/* 11 - put function address on top */
			DW_OP_skip, 20,	/* 12 - done with non-virtual case */
				/* virtual case */
			DW_OP_const1u, 12, /* 13 - size of vtbl entries */
			DW_OP_mul,	/* 14 - vtbl index => slot offset */
			DW_OP_rot,	/* 15 - this pointer on top */
			DW_OP_dup,	/* 16 - make a copy of this pointer and */
			DW_OP_rot,	/* 17 - store this pointer on bottom */
			DW_OP_plus,	/* 18 - "this" + vtbl offset => addr of vtbl ptr */
			DW_OP_deref,	/* 19 - push vtbl address on stack */
			DW_OP_pick, 2,	/* 20 - put slot offset on top */
			DW_OP_plus,	/* 21 - vtbl + slot offset => address of slot */
			DW_OP_dup,	/* 22 */
			DW_OP_rot,	/* 23 */
			DW_OP_deref,	/* 24 - read first word of vtbl slot */
			DW_OP_plus,	/* 25 - this + delta => this adjusted for subobj. */
			DW_OP_swap,	/* 26 - move slot address to top */
			DW_OP_const1u, 8,	/* 27 - adjust address of slot */
			DW_OP_plus,	/* 28 */
			DW_OP_deref,	/* 29 - push function pointer on stack */
			DW_OP_nop,
			DB_DW2_END);
	} else {
		func_loc_desc = DB_GENLAB();
		dwarf_out(".d", func_loc_desc);
		dw2_out_loc_desc("DW_AT_use_location",
			DW_FORM_block1,
			DW_OP_pick, 2,	/* 1 - copy delta/vtbl index to top */
			DW_OP_const1u, 16,	/* 2 - push shift value */
			DW_OP_shr,	/* 3 -shift to get delta value */
			DW_OP_plus,	/* 4 -pushes this pointer on top */
			DW_OP_rot,	/* 5 -func. address on top */
			DW_OP_rot,	/* 6 -delta/vtbl index on top */
			DW_OP_constu, 0xffff,
			DW_OP_and,	/* 7 - pushes vtbl index on top */
			DW_OP_dup,	/* 8 */
			DW_OP_const1u, 0,
			DW_OP_lt,	/* 9 -if negative vtbl index, non-virtual case */
			DW_OP_bra, 4,	/* 10 - skip to virtual case */
				/* non-virtual case */
			DW_OP_drop,	/* 11 - drop unneeded vtbl index */
			DW_OP_swap,	/* 121 - put function address on top */
			DW_OP_skip, 23,	/* 13 - done with non-virtual case */
				/* virtual case */
			DW_OP_const1u, 8, /* 14 - size of vtbl entries */
			DW_OP_mul,	/* 15 - 8 * vtbl index => slot offset */
			DW_OP_rot,	/* 16 - this pointer on top */
			DW_OP_dup,	/* 17 - make a copy of this pointer and */
			DW_OP_rot,	/* 18 - move off of top of stack */
			DW_OP_plus,	/* 19 - "this" + vtbl offset => addr of vtbl ptr */
			DW_OP_deref,	/* 20 - push vtbl address on stack */
			DW_OP_rot,	/* 21 - move slot offset to top */
			DW_OP_rot,	/* 22 */
			DW_OP_plus,	/* 23 - vtbl + slot offset => address of slot */
			DW_OP_dup,	/* 24 */
			DW_OP_rot,	/* 25 */
			DW_OP_deref,	/* 26 - read first word of vtbl slot */
			DW_OP_constu, 16,
			DW_OP_shr,	/* 27 - shift to get delta value */
			DW_OP_plus,	/* 28 - this + delta => this adjusted for subobj. */
			DW_OP_swap,	/* 29 - move slot address to top */
			DW_OP_const1u, 4,	/* 30 - adjust address of slot */
			DW_OP_plus,	/* 31 */
			DW_OP_deref,	/* 32 - push function pointer on stack */
			DW_OP_nop,
			DB_DW2_END);
	} /* if */
	dw_e_entry();
} /* dw_ptr_to_member */

static a_boolean type_in_based_type_list(a_type_ptr type, a_type_ptr base_type)
/* returns TRUE if type is in the list of based types for base_type */
{
	a_based_type_list_member_ptr list = base_type->based_types;
	for (; list; list = list->next) {
		if (list->based_type == type)
			return TRUE;
	} /* if */
	return FALSE;
} /* type_in_based_type_list */

/* Assume we're in a state where it's okay to write new debug info entries.
** Walk the type structure and assign id's.  arrays, function types, and
** pointers to functions are output when seen.  All other types are generated
** by walking the list of named types for the scope where they are defined.
** dw_walktype returns TRUE if the type is generated now.
*/
static a_boolean dw_walktype(a_type_ptr type)
{
	if (type->source_corresp.debug_id
		&& !type->source_corresp.debug_id_is_temporary)
		return FALSE;	/* already generated */

	switch (type->kind)
	{
	case tk_typeref:
		if (dw_walktype(type->variant.typeref.type)
			&& typeref_is_qualified(type)) {
			/* cv-qualified unnamed type - not in typelist */
			dw_typeref(type);
			return TRUE;
		} /* if */
		break;
		
	case tk_pointer:
	{
		/* 3 cases where a pointer type must be generated here:
		** 1) If base type is generated here, pointer type must be also
		**	since base type won't be encountered again
		** 2) The base type has already been generated, and it isn't
		**	one of the basic types, so it also won't be handled again.
		** 3) The pointer type is not in the list of types based off of
		**	its based type.  This happens for some declarations like
		**	int (*(*f)(...)) where the full type for the thing pointed
		**	to isn't know at the time the pointer type is created.
		*/
		a_type_ptr	base_type = type->variant.pointer.type;
		if (dw_walktype(base_type) /*1*/
		  /*2*/ || (base_type->source_corresp.debug_id
				&& !base_type->source_corresp.debug_id_is_temporary
				&& !(base_type->kind == tk_integer
					|| base_type->kind == tk_float))
		  /*3*/ || !type_in_based_type_list(type, base_type)) {
			if (type->variant.pointer.is_reference)
				dw_s_entry(DW2_reference_type, &type->source_corresp,
						"reference type");
			else
				dw_s_entry(DW2_pointer_type, &type->source_corresp,
						"pointer type");
			dw_at_type(base_type);
			dw_e_entry();
			return TRUE;
		} /* if */
	}
		break;

	case tk_routine:
		dwarf_outfunction(type); /* output function type description */
		return TRUE;

	case tk_array:
		/* array types are put out when seen */
		/* nelements is set for vtbl arrays only - in other cases it
		   gets that information from the type itself */
		dw_gen_array(type, /*nelements=*/ 0);
		return TRUE;

	case tk_ptr_to_member:
		if (dw_walktype(type->variant.ptr_to_member.type)
			|| !type_in_based_type_list(type,
					type->variant.ptr_to_member.type)) {
			dw_ptr_to_member(type);
			return TRUE;
		} /* if */
		break;

	default:
		/* all other cases are base types (already generated) or
		   are in typelist */
		break;
	} /* switch */

	dw_debug_id(&type->source_corresp);
	return FALSE;
} /* dw_walktype */

static void dw_typedef(a_type_ptr type)
/* create definition for a type */
{
	if (type->source_corresp.debug_id
		&& !type->source_corresp.debug_id_is_temporary)
		return;

	switch(type->kind)
	{
	case tk_integer:
		if (type->variant.integer.enum_type) {
			dw_enum_def(type);
			break;
		} /* if */
		/*FALLTHROUGH*/
	case tk_float:
	case tk_void:
		internal_error("dw_typedef: base type not output in init");
		break;

	case tk_typeref:
		dw_walktype(type->variant.typeref.type);
		dw_typeref(type);
		break;

	case tk_class:
	case tk_struct:
	case tk_union:
		dw_struct_def(type);
		break;

	case tk_pointer:
		if (type->variant.pointer.is_reference)
			dw_s_entry(DW2_reference_type, &type->source_corresp,
					"reference_type");
		else
			dw_s_entry(DW2_pointer_type, &type->source_corresp,
					"pointer type");
		dw_at_type(type->variant.pointer.type);
		dw_e_entry();
		break;

	case tk_ptr_to_member:
		dw_walktype(type->variant.ptr_to_member.type);
		dw_ptr_to_member(type);
		break;

	case tk_array:
		/* nelements is set for vtbl arrays only - in other cases it
		   gets that information from the type itself */
		dw_gen_array(type, /*nelements=*/ 0);
		break;

	case tk_routine:
		dwarf_outfunction(type);
		break;

	case tk_template_param:
		/* should not ever get here - the list of template parameters
		   is walked before the typelist for a scope */
		break;

	default:
		break;
	}
	dw_gen_based_types(type);
} /* dw_typedef */

static void dw_gen_based_types(a_type_ptr type)
/* generate all types based off of this type so that they are all defined
   in the same scope */
{
	a_based_type_list_member_ptr	based_types = type->based_types;
	for (; based_types; based_types = based_types->next) {
		dw_typedef(based_types->based_type);
		dw_gen_based_types(based_types->based_type);
	} /* for */
} /* dw_gen_based_types */

static void dw_type_list(a_type_ptr type)
/* Walk the list of named types in a scope */
{
	for(; type; type = type->next)
	{
		if (!type->source_corresp.debug_id
			|| type->source_corresp.debug_id_is_temporary)
			dw_typedef(type);
		dw_gen_based_types(type);
	}
} /* dw_type_list */

static a_debug_mark_ptr dw_new_mark(an_il_entry_kind kind)
/* Create a new debug_mark and link it onto the end of the list for the
** current scope. */
{
	a_debug_mark_ptr	mark = alloc_debug_mark(kind);

	if (!mark_stack_ptr->mark_list->marks)
		mark_stack_ptr->mark_list->marks = mark;
	else
		mark_stack_ptr->tail->next = mark;
	mark_stack_ptr->tail = mark;
	return mark;
} /* dw_new_mark */

static void dw_mark_variable(a_variable_ptr var, a_boolean addr_only)
/* Create a debug_mark for a variable */
{
	a_debug_mark_ptr	mark = dw_new_mark(iek_variable);
	mark->offset = ftell(dwarf_dbout);
	mark->address_only = addr_only;
	mark->variant.variable = var;
} /* dw_mark_variable */

static void dw_mark_routine(a_routine_ptr rp)
/* Create a debug_mark for a routine - used only for creating an entry for
** a constant in a template parameter, so address_only is always TRUE */
{
	a_debug_mark_ptr	mark = dw_new_mark(iek_routine);
	mark->offset = ftell(dwarf_dbout);
	mark->address_only = TRUE;
	mark->variant.routine = rp;
} /* dw_mark_variable */

static void dw_variable(a_variable_ptr var, a_scope_ptr scope, a_boolean gen_type)
/* Generate a debugging entry for a variable.  Since there are several abbreviation
** entries with different attributes for different classes of variables, generation
** of the attributes is table driven.
*/
{
	Abbrev_index			tag;
	int				external = 0;
	int				artificial = FALSE;
	enum a_storage_class_tag	sc = var->storage_class;
	const Dwarf2_Attribute		*ab_entry;
	a_line_number			line;
	int				file_index;
	a_symbol_ptr			symbol;
	a_boolean			has_location = FALSE;
	int				i;

	if (!var->source_corresp.decl_position.seq) {
		if (var->source_corresp.name
			&& strncmp(var->source_corresp.name, "__vtbl_",
							 strlen("__vtbl_")) == 0) {
			/* vtbls are handled separately in dwarf_debug_cleanup */
			return;
		} /* if */
		artificial = TRUE;
	} else if (var->is_anonymous_parent_object || var->is_this_parameter) {
		artificial = TRUE;
	} else {
		dw_get_file_and_line(var->source_corresp.decl_position.seq,
			&file_index, &line);
	} /* if */

	symbol = (a_symbol_ptr)var->source_corresp.assoc_info;
	switch(sc)
	{
	case sc_extern:
	case sc_unspecified:
		if (var->source_corresp.is_class_member) {
			tag = DW2_static_member_decl;
			if (symbol && symbol->defined) {
				/* Definitions of static data members are delayed
				** and generated at file scope
				*/
				a_static_member *link
					= (a_static_member *)alloc_fe(
							sizeof(a_static_member));
				link->next = 0;
				link->variable = var;
				if (static_member_tail)
					static_member_tail->next = link;
				else
					static_member_head = link;
				static_member_tail = link;
			} /* if */
		} else if (symbol && symbol->defined) {
			tag = variable_tags[artificial][FALSE][FALSE];
		} else if ((symbol && symbol->referenced)
			|| var->source_corresp.parent.namespace_ptr != 0
			|| var->source_corresp.debug_id_is_temporary) {
			tag = DW2_variable_decl;
		} else {
			/* Declaration (not definition) of unreferenced
			** external object.
			*/
			return;
		} /* if */
		external++;
		break;

	case sc_static:
		if (var->promoted_local_static)
			/* already covered in function scope */
			return;
		else if (!artificial && !var->source_corresp.referenced)
			/* No location - compiler doesn't allocate space for
			   unreferenced statics */
			tag = DW2_unrefed_variable;
		else
			tag = variable_tags[artificial][FALSE][curr_is_inline];
		break;

	default:	/* local variable or parameter */
		if (!var->source_corresp.referenced && !var->is_parameter)
			tag = DW2_unrefed_variable;
		else
			tag = variable_tags[artificial]
					[var->is_parameter || var->is_handler_param]
					[curr_is_inline];
		break;
	}

	/* gen_type will always be true except when it is in the body of a inline
	** member function definition.  In that case, any types generated in the
	** parameter list should be generated with the function declaration, not
	** the definition, so that if the back end does not copy the definition
	** through, there will not be any dangling references from the declaration.
	*/
	if (gen_type)
		dw_walktype(var->type);		/* walk type structure */

	/* Start new entry. */
	if (IS_DWARF_1()) {
		/* Abbreviation table entries do not translate easily into
		   dwarf1 tags for variables - can't go through the dwarf1_tag table */
		int dw1_tag;
		if (sc == sc_extern || sc == sc_unspecified)
			dw1_tag = TAG_global_variable;
		else if (var->is_parameter)
			dw1_tag = TAG_formal_parameter;
		else
			dw1_tag = TAG_local_variable;
		dw1_s_entry(dw1_tag, &var->source_corresp, /*scope=*/0, "variable");
	} else
		dw_s_entry(tag, &var->source_corresp, "variable");
	ab_entry = dw_abbrev_table[tag].attributes;
	for (i = dw_abbrev_table[tag].nattr; i; ++ab_entry, --i) {
		switch (ab_entry->name) {
		case DW_AT_name:
			if (var->is_this_parameter)
				dw_at_name("this");
			else
				dw_at_name(var->source_corresp.name);
			break;
		case DW_AT_type:
			dw_at_type(var->type);
			break;
		case DW_AT_artificial:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1 /*TRUE*/, "DW_AT_artificial");
			break;
		case DW_AT_declaration:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1 /* TRUE */, "DW_AT_declaration");
			break;
		case DW_AT_decl_line:
			if (IS_DWARF_2())
				dwarf_out("\tluC", line, "DW_AT_decl_line");
			break;
		case DW_AT_decl_file:
			if (IS_DWARF_2())
				dwarf_out("\tluC", file_index, "DW_AT_decl_file");
			break;
		case DW_AT_external:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", external, "DW_AT_external");
			break;
		case DW_AT_location:
			/* filled in by back end */
			dwarf_out("\tC", "location - generated by c++be");
			has_location = TRUE;
			break;
		default:
			internal_error("dw_variable(): bad attribute");
			break;
		} /* switch */
	} /* for */
	if (has_location || curr_is_inline)
		dw_mark_variable(var, FALSE);
	dw_e_entry();

	if (dwarf_name_lookup && external) {
		dw_name_buf_used = 0;
		form_symbol_name(symbol, &octl);
		/* Output the .debug_pubnames entry for this variable. */
		dwarf_out("Sn\tW\tLd-sCb\nSp", DB_CURINFOLAB, LAB_INFOBEGIN,
			".debug pubnames variable entry", dw_name_buf);
	}  /* if */
} /* dw_variable */

static void dw_label_list(a_label_ptr label)
/* Generate debugging entries for user-defined labels in a function */
{
	a_debug_mark_ptr	mark;

	for (; label; label = label->next) {
		if (!label->source_corresp.name)
			continue;
		/* The back end handles generating the address.  No address
		** attribute is generated in an abstract tree, but the debug mark
		** is still needed to force the back end to create an entry in
		** a concrete instance.
		*/
		dw_s_entry(curr_is_inline ? DW2_label_abstract : DW2_label,
				&label->source_corresp, "label");
		dw_at_name(label->source_corresp.name);
		mark = dw_new_mark(iek_label);
		mark->offset = ftell(dwarf_dbout);
		mark->variant.label = label;
		dw_e_entry();
	} /* for */
} /* dw_label_list */

static void dw_beg_function(Abbrev_index tag, a_routine_ptr routine,
				a_boolean is_definition)
/* Do debug stuff at start of function definition. */
{
	const Dwarf2_Attribute	*ab_entry;
	int			external = (routine->storage_class != sc_static);
	a_line_number		line;
	int			file_index;
	a_type_ptr	rettype = routine->type->variant.routine.return_type;
	a_debug_entry_id	save_id;
	a_boolean		save_id_is_temporary;
	a_boolean		reset_id = FALSE;
	int			i;
	
	if (is_definition && (routine->source_corresp.is_class_member
				|| routine->source_corresp.parent.namespace_ptr)) {
		/* a member function definition has to get its own debug id
		   (which is refered to only by inline function instance) without
		   writing over the debug id in the routine structure, which refers
		   to the entry in the class definition, and which may be 
		   referred to more than once (specification entries for
		   the function definition, using directives, etc.) */
		/* All type information is handled on the member function
		   declaration, not the definition */
		save_id = routine->source_corresp.debug_id;
		save_id_is_temporary = routine->source_corresp.debug_id_is_temporary;
		routine->source_corresp.debug_id = 0;
		routine->source_corresp.debug_id_is_temporary = FALSE;
		reset_id = TRUE;
	} else {
		if (rettype) {
			/* force out lower-level type info */
			dw_walktype(rettype);
		} /* if */
	} /* if */

	if (routine->source_corresp.is_class_member
		&& routine->source_corresp.parent.class_type
				->source_corresp.is_class_member) {
		/* nested class member */
		external = 0;
	}

	/* Always generate a new entry */
	if (IS_DWARF_1())
		dw1_s_entry(external ? TAG_global_subroutine : TAG_subroutine,
				&routine->source_corresp,
				is_definition ? routine->assoc_scope : 0,
				"subroutine");
	else
		dw_s_entry(tag, &routine->source_corresp, "subroutine");

	mark_stack_ptr->mark_list->definition_id = routine->source_corresp.debug_id;
	if (reset_id) {
		routine->source_corresp.debug_id = save_id;
		routine->source_corresp.debug_id_is_temporary = save_id_is_temporary;
	} /* if */

	/* Sibling already generated in dw_s_entry for Dwarf1 */
	if (IS_DWARF_2()) {
		/* Siblings for function definition are generated with the function
		** entry, based on the scope number, since some reordering of groups
		** of entries may occur in the back end due to inlining
		*/
		if (is_definition)
			dwarf_out("\tW\tLs-sC", routine->assoc_scope, LAB_INFOBEGIN,
				"DW_AT_sibling");
		else
			dwarf_out("\tW\tLd-sC", DB_CURSIBLING, LAB_INFOBEGIN,
				"DW_AT_sibling");
		if (!routine->compiler_generated)
			dw_get_file_and_line(routine->source_corresp.decl_position.seq,
				&file_index, &line);
	} /* if */

	ab_entry = dw_abbrev_table[tag].attributes;
	for (i = dw_abbrev_table[tag].nattr; i; --i, ++ab_entry) {
		switch (ab_entry->name) {
		case DW_AT_sibling:	/* already generated */
			break;
		case DW_AT_name:
			dw_at_name(routine->source_corresp.name);
			break;
		case DW_AT_type:
			dw_at_type(rettype);	/* describe the returned type */
			break;
		case DW_AT_prototyped:	/* always true for C++ */
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1, "DW_AT_prototyped");
			break;
		case DW_AT_external:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", external, "DW_AT_external");
			break;
		case DW_AT_artificial:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1 /* true */, "DW_AT_artificial");
			break;
		case DW_AT_declaration:
			if (IS_DWARF_2())
				dwarf_out("\tB\txC", 1 /* true */, "DW_AT_declaration");
			break;
		case DW_AT_decl_file:
			if (IS_DWARF_2())
				dwarf_out("\tluC", file_index, "DW_AT_decl_file");
			break;
		case DW_AT_decl_line:
			if (IS_DWARF_2())
				dwarf_out("\tluC", line, "DW_AT_decl_line");
			break;
		case DW_AT_virtuality:
			if (IS_DWARF_2()) {
				dwarf_out("\tB\txC", routine->pure_virtual
						? DW_VIRTUALITY_pure_virtual
						: DW_VIRTUALITY_virtual,
					"DW_AT_virtuality");
			} else {
				/* Dwarf 1 */
				if (routine->pure_virtual)
					dwarf_out("\tH\tx; B\txC", AT_pure_virtual,
						0, "AT_pure_virtual");
				else
					dwarf_out("\tH\tx; B\txC", AT_virtual, 0,
						"AT_virtual");
			} /* if */
			break;
		case DW_AT_vtable_elem_location:
		{
			/* Output location attribute; address of start of object
			** is implicit.
			*/
			a_type_ptr class_type = routine->source_corresp.parent
								.class_type;
			a_type_ptr mptr_type = make_mptr_type();
			if (IS_DWARF_2())
			{
				dw2_out_loc_desc("DW_AT_vtable_elem_location",
					DW_FORM_block1,
					DW_OP_plus_uconst,
					class_type->variant.class_struct_union
						.extra_info
						->virtual_function_info_offset,
					DW_OP_deref, DW_OP_plus_uconst,
					routine->virtual_function_number
						* mptr_type->size,
					DB_DW2_END);
			} else {
				/* Dwarf 1 */
				dwarf_out("\tH\tx; H d; B x; W d; B x; B x; B x; W d; B xC",
					AT_location, 1+4+1+1+1+4+1, OP_CONST,
					class_type->variant.class_struct_union
						.extra_info
						->virtual_function_info_offset,
					OP_ADD, OP_DEREF, OP_CONST,
					routine->virtual_function_number
						* mptr_type->size,
					OP_ADD,
					"AT_location: OP_CONST(val) OP_ADD OP_DEREF OP_CONST(val) OP_ADD");
			} /* if */
		}
			break;
		case DW_AT_specification:
			if (IS_DWARF_2()) {
				dw_at_reference(DW_AT_specification,
						&routine->source_corresp,
						"DW_AT_specification");
			} /* if */
			break;
		case DW_AT_inline:
			/* DW_AT_inline attribute supplied by the back end */
			dwarf_out("\tC", "inline attribute - generated by c++be");
			mark_stack_ptr->mark_list->routine_offset = 0;
			mark_stack_ptr->mark_list->inline_insert = ftell(dwarf_dbout);
			break;
		case DW_AT_high_pc:
			dwarf_out("\tC", "high_pc - generated by c++be");
			mark_stack_ptr->mark_list->routine_offset = ftell(dwarf_dbout);
			break;
		case DW_AT_low_pc:
			dwarf_out("\tC", "low_pc - generated by c++be");
			break;
		default:
			internal_error("dw_beg_function(): bad case");
		} /* switch */
	} /* if */
	if (IS_DWARF_1() && is_definition && routine->source_corresp.is_class_member) {
		dw_at_reference(AT_member,
			&routine->source_corresp.parent.class_type->source_corresp,
			"AT_member");
	} /* if */
	dw_e_entry();

	if (dwarf_name_lookup && external) {
		dw_name_buf_used = 0;
		form_symbol_name((a_symbol_ptr)routine->source_corresp.assoc_info,
				&octl);
		/* Output the .debug_pubnames entry for this function. */
		dwarf_out("Sn\tW\tLd-sCb\nSp", DB_CURINFOLAB, LAB_INFOBEGIN,
			".debug pubnames function entry", dw_name_buf);
	}  /* if */
}  /* dw_beg_function */

static void dw_namespaces(a_namespace_ptr nmsp)
{
	for (; nmsp; nmsp = nmsp->next) {
		if (nmsp->is_namespace_alias) {
			/* Not supported by Dwarf 1 */
			if (IS_DWARF_2()) {
				dw_s_entry(DW2_namespace_alias,
					&nmsp->source_corresp,
					"namespace alias");
				dw_at_name(nmsp->source_corresp.name);
				dw_at_reference(DW_AT_SCO_namespace,
					&nmsp->variant.assoc_namespace
							->source_corresp,
					"DW_AT_SCO_namespace");
				dw_e_entry();
			} /* if */
		} else {
			if (IS_DWARF_2()) {
				a_routine_ptr rp;
				dw_s_entry(DW2_namespace, &nmsp->source_corresp,
					"namespace");
				dwarf_out("\tW\tLd-sC", DB_CURSIBLING,
					LAB_INFOBEGIN, "DW_AT_sibling");
				dw_at_name(nmsp->source_corresp.name);
				dw_e_entry();

				dw_push();

				dwarf_gen_debug_info_for_scope(
						nmsp->variant.assoc_scope);
				/* generate function declarations */
				for (rp = nmsp->variant.assoc_scope->routines;
					rp; rp = rp->next)
					dw_member_function(rp);

				(void)dw_pop();
			} else {
				/* Dwarf 1 - put out namespaces entries without
				   creating a new scope */
				dwarf_gen_debug_info_for_scope(
						nmsp->variant.assoc_scope);
			} /* if */
		} /* if */
	} /* for */
} /* dw_namespaces */

static void dw_push_mark_stack(void)
/* Start a new level on the mark stack -- called when starting a file,
** function, or block scope */
{
	if (mark_stack_depth+1 == mark_stack_size) {
		size_t new_size = mark_stack_size + DB_STACK_INCR;
		mark_stack = (dw_mark_stack *)realloc_general((char *)mark_stack,
				(size_t)(mark_stack_size * sizeof(dw_mark_stack)),
				(size_t)(new_size * sizeof(dw_mark_stack)));
		mark_stack_size = new_size;
	} /* if */
	++mark_stack_depth;
	mark_stack_ptr = &mark_stack[mark_stack_depth];
	mark_stack_ptr->mark_list = alloc_debug_mark_list();
	mark_stack_ptr->mark_list->first_offset = ftell(dwarf_dbout);
	mark_stack_ptr->tail = 0;
} /* dw_push */

static a_debug_mark_list_ptr dw_pop_mark_stack(void)
/* Exiting a scope, pop a level in the mark stack.  Return value (head of the list)
** is stored in the parent scope.
*/
{
	a_debug_mark_list_ptr	tmp = mark_stack_ptr->mark_list;
	if (--mark_stack_depth < 0)
		internal_error("debug stack underflow");
	--mark_stack_ptr;
	return tmp;
} /* dw_pop */

static void dw_gen_common_scope_info(a_scope_ptr scope)
{
	a_variable_ptr	var;
	a_scope_ptr	sptr;

	dw_namespaces(scope->namespaces);
	if (IS_DWARF_2())
		dw_using_directives(scope->using_decls);
	dw_type_list(scope->types);
	for (var = scope->variables; var; var = var->next)
		dw_variable(var, scope, TRUE);
	for (var = scope->nonstatic_variables; var; var = var->next)
		dw_variable(var, scope, TRUE);
	for (sptr = scope->scopes; sptr; sptr = sptr->next)
		dwarf_gen_debug_info_for_scope(sptr);
} /* dw_gen_common_scope_info */	

static void dw_gen_info_for_file(a_scope_ptr scope)
{
	a_static_member	*link;
	a_routine_ptr	rp;

	curr_is_inline = FALSE;
	dw_push_mark_stack();
	dw_gen_common_scope_info(scope);

	for (rp = scope->routines; rp; rp = rp->next) {
		if (rp->source_corresp.debug_id_is_temporary) {
			/* Some other debugging entry (a namespace member,
			** for example, referenced an undefined function.
			** Put out a function declaration entry
			*/
			a_type_ptr rettype = rp->type->variant.routine.return_type;
			if (rettype && rettype->kind == tk_void)
				rettype = 0;
			dw_beg_function(rettype ? DW2_non_void_function_decl
						: DW2_void_function_decl,
					rp, /*is_definition=*/ FALSE);
			/* Put out the parameter types */
			dw_push();
			dw_gen_parameters(/*scope=*/0, rettype,
				rp->type->variant.routine.extra_info,
				/*gen_exception_spec=*/ TRUE, /*gen_types=*/ TRUE);
			(void)dw_pop();
		} /* if */
	} /* for */

	/* generate entries for static data members defined in this
	   compilation unit */
	for (link = static_member_head; link; link = link->next) {
		a_variable_ptr	var = link->variable;

		if (!var->source_corresp.referenced) {
			/* true only for some template specializations */
			continue;
		}

		dw_s_entry(DW2_static_member, &var->source_corresp, "static member");
		if (IS_DWARF_1()) {
			dw_at_name(var->source_corresp.name);
			dw_at_reference(AT_member,
					&var->source_corresp.parent.class_type
						->source_corresp,
					"AT_member");
		} else {
			a_line_number	line;
			int		file_index = 1;

			dw_get_file_and_line(var->source_corresp.decl_position.seq,
				&file_index, &line);
			dwarf_out("\tluC\tluC", file_index, "DW_AT_decl_file",
		     		line, "DW_AT_decl_line");
			dw_at_reference(DW_AT_specification, &var->source_corresp,
				"DW_AT_specification");
		} /* if */
		/* location filled in by back end */
		dwarf_out("\tC", "location - generated by c++be");
		dw_mark_variable(var, FALSE);
		dw_e_entry();
	} /* for */

	/* generation of types based off of the fundamental types has been
	** delayed until the end (but before lowering the file scope
	** memory region) to ensure that we get everything
	*/
	dwarf2_fund_types_2();
} /* dw_gen_info_for_file */

static void dw_gen_info_for_func(a_scope_ptr scope)
{
	a_routine_ptr	rp = scope->variant.routine.ptr;
	a_type_ptr	rettype = rp->type->variant.routine.return_type;
	Abbrev_index	tag;

	if (rp->is_trivial_default_constructor
		|| (IS_DWARF_1() && rp->is_inline)) {
		/* no IL generated and function is not linked into
		   class definition */
		return;
	}

	dw_push_mark_stack();
	mark_stack_ptr->mark_list->routine = rp;
	if (rettype && rettype->kind == tk_void)
		rettype = 0;

	curr_is_inline = rp->is_inline;
	if (IS_DWARF_2() && (rp->source_corresp.is_class_member
			|| rp->source_corresp.parent.namespace_ptr)) {
		tag = member_function_defn_tags[rp->compiler_generated]
					[rp->is_inline];
	} else {
		tag = function_tags[rp->compiler_generated]
					[rettype != 0][rp->is_inline];
	} /* if */
	dw_beg_function(tag, rp, /*is_definition=*/ TRUE);

	dw_push();
	if (rp->is_template_function && IS_DWARF_2()) {
		a_symbol_ptr rsym = (a_symbol_ptr)rp->source_corresp.assoc_info;
		a_template_symbol_supplement_ptr tptr
			= rsym->variant.routine.instance_ptr->template_sym
				->variant.template_info;
		dw_gen_template_args(rp->template_arg_list,
					tptr->cache.decl_info->parameters);
	} /* if */
	/* Put out parameters. Also, put out exception specification list for
	** non-member functions. For member functions, the exception specification
	** list will be put out with the function declaration from dw_struct_def
	*/
	dw_gen_parameters(scope, rettype, rp->type->variant.routine.extra_info,
			/*gen_exception_spec=*/ !rp->source_corresp.is_class_member,
			/*gen_types=*/ !(rp->is_inline && rp->source_corresp
							.is_class_member));
	dw_gen_common_scope_info(scope);
	dw_label_list(scope->labels);
	if (rp->is_inline)
		(void)dw_pop();
	else
		mark_stack_ptr->mark_list->inline_insert = dw_pop();

	dwarf_out("Ls:C", rp->assoc_scope, "routine entry sibling");
	mark_stack_ptr->mark_list->last_offset = ftell(dwarf_dbout);
	scope->debug_marks = dw_pop_mark_stack();
} /* dw_gen_info_for_func */

static void dw_gen_info_for_block(a_scope_ptr scope)
{
	a_boolean	generate_block = FALSE;
	Abbrev_index	tag;
	a_debug_mark_ptr mark;

	if (scope->variant.assoc_handler && IS_DWARF_2()) {
		tag = curr_is_inline ? DW2_catch_block_abstract
				     : DW2_catch_block;
		generate_block = TRUE;
	} else if (scope->next && scope->next->kind == sck_block
		&& scope->next->variant.assoc_handler && IS_DWARF_2()) {
		tag = curr_is_inline ? DW2_try_block_abstract
				     : DW2_try_block;
		generate_block = TRUE;
	} else if (scope->variables || scope->nonstatic_variables
		|| scope->labels) {
		/* generate a lexical block entry only if needed */
		tag = curr_is_inline ? DW2_lexical_block_abstract
				     : DW2_lexical_block;
		generate_block = TRUE;
	} /* if */
	if (generate_block == TRUE) {
		if (curr_is_inline) {
			/* generate a label for the debug entry based on the unique
			** scope number.  This is needed to give the 
			** DW_AT_specification attribute in the concrete
			** instance tree generated by the
			** back end something to refer to.  ..B<scope#> is used
			** instead of ..D<debug_id> since a block (scope) does
			** not have a source_correspondence field
			*/
			dwarf_out("LS:C", scope->number, "block entry label");
		} /* if */
		dw_s_entry(tag, /*source_corresp=*/ 0, "block");
		if (IS_DWARF_2())
			dwarf_out("\tW\tLd-sC", DB_CURSIBLING,
					LAB_INFOBEGIN, "DW_AT_sibling");
		if (!curr_is_inline) {
			/* Generate low and hi pc attributes for blocks */
			/* The labels are based on the current (unique) scope id */
			if (IS_DWARF_1())
				dwarf_out("\tH\tx; W\tLlC\tH\tx; W\tLhC",
					AT_low_pc, scope->number, "AT_low_pc",
					AT_high_pc, scope->number, "AT_high_pc");
			else
				dwarf_out("\tW\tLlC\tW\tLhC",
					scope->number, "DW_AT_low_pc",
					scope->number, "DW_AT_high_pc");
		} /* if */
		dw_e_entry();
		dw_push();	/* things in the block owned by this entry */
		mark = dw_new_mark(iek_scope);
		mark->variant.scope = scope;
		dw_push_mark_stack();
	} /* if */
	if (tag == DW2_catch_block && !scope->variant.assoc_handler->parameter) {
		dw_s_entry(DW2_unspecified_params, /*source_corresp=*/ 0,
			"unspecified parameters");
		dw_e_entry();
	} /* if */
	dw_gen_common_scope_info(scope);
	if (generate_block) {
		mark->offset = dw_pop();
		scope->debug_marks = dw_pop_mark_stack();
	}
} /* dw_gen_info_for_block */

void dwarf_gen_debug_info_for_scope(a_scope_ptr scope)
/* Main routine that drives most debugging information generation.
** Called from pop_scope in scope_stk.c at the end of processing functions and
** the file scope.  Also called recursively to handle lexical blocks
*/
{
	static a_boolean	first_time = TRUE;

	if (!dwarf_generate_symbol_info || !dwarf_dbout || !scope
		|| total_errors || total_catastrophes)
		return;

	if (first_time) {
		if (IS_DWARF_2())
			dwarf2_fund_types();
		il_header.dbg_prologue_length = ftell(dwarf_dbout);
		first_time = FALSE;
	} /* if */

	switch (scope->kind) {
	case sck_file:		dw_gen_info_for_file(scope);		break;
	case sck_function:	dw_gen_info_for_func(scope);		break;
	case sck_block:		dw_gen_info_for_block(scope);		break;
	default:		dw_gen_common_scope_info(scope);	break;
	} /* switch */
} /* dwarf_gen_debug_info_for_scope */

static void dwarf2_gen_abbreviations(void)
/* Generate a separate .debug_abbrev section containing
** a "standard" abbreviation table.
*/
{
	const Dwarf2_Abbreviation	*entry = dw_abbrev_table;
	int	i, j;
	/* create abbreviation table  section */
	fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, DB_DOTDBG_ABBR);
	/* output Abbreviation table lable */
	dwarf_out("s:\tC", ABBREV_VERSION_ID, "Dwarf 2 - Abbreviation Table");
	/* create the entries */
	for (i = 1, ++entry; i < DW2_last; ++i, ++entry) {
		const Dwarf2_Attribute *attrs = entry->attributes;
		dwarf_out("\tluC", i, "start abbrev entry");
		/* first entry is tag and children flag */
		dwarf_out("\tlu;\tlu", entry->tag, entry->children);
		if (dwarf_comments_in_debug)
			fprintf(dwarf_dbout, "\t%s %s %s", COMMENTSTR,
				dwarf2_tag_name(entry->tag),
				entry->children ? "DW_CHILDREN_yes"
						: "DW_CHILDREN_no");
		putc('\n', dwarf_dbout);
		/* generate attributes */
		for (j = entry->nattr; j; --j, ++attrs) {
			dwarf_out("\tlu;\tlu", attrs->name, attrs->form);
			if (dwarf_comments_in_debug)
				fprintf(dwarf_dbout, "\t%s %s %s", COMMENTSTR,
					dwarf2_attribute_name(attrs->name),
					dwarf2_form_name(attrs->form));
			putc('\n', dwarf_dbout);
		} /* for */
		dwarf_out("\tlu;\tluC", DB_DW2_END, DB_DW2_END, "end entry");
	} /* for */

	dwarf_out("\tluC", DB_DW2_END, "end section");
} /* dwarf2_gen_abbreviations */

void dwarf_debug_init()
{
	a_boolean cannot_open;
	a_boolean bad_name;

	if (!dwarf_generate_symbol_info || total_errors || total_catastrophes) {
		il_header.dbg_prologue_length = 0;
		il_header.dbg_level = No_debug_info;
		return;
	} /* if */

	dwarf_dbout = open_output_file(dwarf_debug_file, /*binary_file=*/FALSE,
					/*update_mode=*/FALSE,
					&cannot_open, &bad_name);
	if (bad_name || cannot_open) {
		str_command_line_error(ec_cannot_open_debug_file, dwarf_debug_file);
		return;
	}  /* if */
	il_header.dbg_level = dwarf_level;

	db_stack_size = DB_STACK_INCR;
	db_stack = (debug_stack *)alloc_general(db_stack_size * sizeof(debug_stack));
	DB_CURSIBLING = DB_CURINFOLAB = 0;
	mark_stack_size = DB_STACK_INCR;
	mark_stack = (dw_mark_stack *)alloc_general(mark_stack_size
							* sizeof(dw_mark_stack));

	/* Set up the table of abbreviation entries - used by both
	   dwarf versions to determine which attributes to generate */
	if ((dw_abbrev_table = dwarf2_gen_abbrev_table()) == 0)
		internal_error("cannot create abbreviation table");

	if (IS_DWARF_1()) {
		/* Dwarf 1 */
		db_dbg_sec_name = DB_DOTDEBUG;
		db_line_sec_name = DB_DOTLINE;
	} else {
		/* Dwarf 2 */
		db_dbg_sec_name = DB_DOTDBG_INFO;
		db_line_sec_name = DB_DOTDBG_LINE;
	
	} /* if */
	/* Initialize sections with all their attributes. */
	/* Right now, there are no attributes for these sections. */
	fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, db_dbg_sec_name);
	fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, db_line_sec_name);

	if (IS_DWARF_1()) {
		dwarf_out("Sd");
		dw_s_entry(DW2_compile_unit, /*source_corresp=*/ 0, "compile unit");
		dw_at_name(primary_source_file_name);
		dwarf_out("\tH\tx; W\txC", AT_language,
			(C_dialect == C_dialect_cplusplus)
				? LANG_C_PLUS_PLUS : LANG_C89,
			"AT_language");
		dwarf_out("\tH\tx; W\tLBC\tH\tx; W\tLEC",
			AT_low_pc, "AT_low_pc", AT_high_pc, "AT_high_pc");
		dwarf_out("\tH\tx; W\tLbC", AT_stmt_list, "AT_stmt_list");
		dw_e_entry();
	} else {
		char		buffer[SYS_NMLN+PATH_MAX+2];
		struct utsname	unm;
		size_t		len;

		fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, DB_DOTDBG_ABBR);
		/* The following 2 sections can only be flagged for output if
		   generating Dwarf 2; this is controlled currently when
		   the command line options are processed in cmd_line.c. */
		/* NOTE - there is no option yet to turn on dwarf_addr_ranges */
		if (dwarf_name_lookup) {
			fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, DB_DOTDBG_NAME);
		}  /* if */
		if (dwarf_addr_ranges) {
			fprintf(dwarf_dbout, "\t%s\t%s\n", SECTION, DB_DOTDBG_ADDR);
		}  /* if */
		if (dwarf_gen_abbreviations)
			dwarf2_gen_abbreviations();

		/* Generate the .debug_pubnames section header. */
		if (dwarf_name_lookup) {
			dwarf_out("Sn\tW\ts-sC", LAB_NAMEEND, LAB_NAMEBEGIN,
				"Dwarf 2 - .debug_pubnames section header");
			dwarf_out("s:\tC", LAB_NAMEBEGIN,
				"Dwarf 2 - .debug_pubnames section start");
			dwarf_out("\tH\txC", DB_DW2_VERSION /*version 2*/,
					"version 2");
			dwarf_out("\tW\tsC", LAB_INFOBEGIN, ".debug_info offset");
			dwarf_out("\tW\ts-sC", LAB_INFOEND, LAB_INFOBEGIN,
				"length of .debug_info section (including 4 byte len.)");
			clear_il_to_str_output_control_block(&octl);
			octl.output_str = write_string;
		}  /* if */

		/* Generate the .debug_info header. */
		dwarf_out("Sds:\tC", LAB_INFOBEGIN, "Dwarf 2 - .debug_info section header");
		dwarf_out("\tW\ts-s-4C", LAB_INFOEND, LAB_INFOBEGIN,
			".debug_info section length less these 4 bytes");
		dwarf_out("\tH\txC", DB_DW2_VERSION /*version 2*/, "version 2");
		dwarf_out("\tW\tsC", ABBREV_VERSION_ID, "abbreviation table offset");
		dwarf_out("\tB\txC", targ_sizeof_pointer, "size of an address");

		/* create the string "host:compilation_dir" for DW_AT_comp_dir */
		if (uname(&unm) < 0) {
			len = 0;
		} else {
			len = strlen(unm.nodename);
			strncpy(buffer, unm.nodename, SYS_NMLN);
		} /* fi */
		buffer[len] = ':';
		if (!getcwd(&buffer[len+1], PATH_MAX+1))
			buffer[len+1] = '\0';

		/* Generate the compilation unit header. */
		dwarf_out("\tluC", DW2_compile_unit, "DW_TAG_compile_unit");
		dwarf_out("bC", buffer, "DW_AT_comp_dir");
		dw_at_name(primary_source_file_name);
		dwarf_out("\tB\txC", (C_dialect == C_dialect_cplusplus)
					? DW_LANG_C_plus_plus : DW_LANG_C89,
			"DW_AT_language");
	
		sprintf(buffer, "SCO %s", CPLUS_PKG);
		dwarf_out("bC", buffer, "DW_AT_producer");
		dwarf_out("\tW\tLBC\tW\tLEC", "DW_AT_low_pc", "DW_AT_high_pc");
		dwarf_out("\tW\tLbC", "DW_AT_stmt_list");
	
	} /* if */
	/* file entry owns all the others */
	dw_push();
} /* dwarf_debug_init */

void dwarf_debug_cleanup()
/* Do what needs to be done for debug information at the end of the
   primary source file. */
{
	a_vtbl_link	*vlink;

	if (!dwarf_generate_symbol_info || !dwarf_dbout
		|| total_errors || total_catastrophes)
		return;

	/* generate entries for vtbls defined in this compilation unit
	** This has to be done at the very end (after IL lowering, but before
	** writing the file scope memory region to the file) because IL lowering
	** determines whether or not a vtbl object will actually be generated
	*/
	for (vlink = vtbl_head; vlink; vlink = vlink->next) {
		dw_gen_vtbl(vlink->vtbl, vlink->nslots);
	} /* for */

	il_header.primary_scope->debug_marks = dw_pop_mark_stack();
	(void)dw_pop();	/* unwind from push in dwarf_debug_init */
	if (db_stack_depth != 0)
		internal_error("debug stack not zero");

	if (IS_DWARF_1()) {
		/* Dwarf 1 */
		/* Force out dummy entry to align to 4-byte boundary.
		** This is a hack to cope with assemblers that insist on
		** aligning everything, whether asked to or not.
		*/
		int lab = DB_GENLAB();
		dwarf_out(".d\tW\tLD-Ld\n\ts\t4\n.D.d",
			lab, lab, lab, DB_DOTALIGN, lab, DB_CURSIBLING);
	} else {
		/* Put out the label for the end of the .debug_info section. */
		dwarf_out("\ts\t4\ns:\tC", DB_DOTALIGN, LAB_INFOEND,
			"Dwarf 2 - .debug_info section end");
	} /* if */

	/* Terminate the .debug_pubnames section header. */
	if (dwarf_name_lookup) {
		dwarf_out("Sn\tB\tx\ns:\tC", DB_DW2_END, LAB_NAMEEND,
			"end of .debug_pubnames section");
	}  /* if */

	il_header.primary_scope->debug_marks->last_offset = ftell(dwarf_dbout);
	fclose(dwarf_dbout);
} /* dwarf_debug_cleanup */

