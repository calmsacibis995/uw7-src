#ifndef ELFDEBUG_H
#define ELFDEBUG_H
#ident	"@(#)cplusbe:common/elfdebug.h	1.3"

/* interfaces between cplusbe-specific elfdebug.c and the rest of cplusbe */

typedef enum {
	db_il_block,
	db_il_try_block,
	db_il_catch_block,
	db_il_symbol,
	db_il_function
} db_il_type;

/* Linked list of items - either variables, labels, or blocks - whose debugging
** entries need updating by the back end.  Used for the children of normal functions
** and for abstract inline instances.
*/
typedef struct update_list
{
	struct update_list	*next;
	long			offset;		/* offset in intermediate file */
	db_il_type		item_type;
	union {
		struct {
			SX	sid;		/* symbol whose location needs updating */
			char	addr_only;
			char	return_val_opt;
		} symbol;
		struct {
			long			scope_id;
			struct update_list	*child;
		} scope;
	} variant;
} update_list;

/* region_updates is an array of pointers to debug_update structures, one for each
** memory region (each function plus file scope).  A debug_update structure is created
** only if the compiler is generating code for the function (unreferenced inlines
** currently are ignored).  The structure contains the begin
** and end offsets of the debugging entries for that region within the intermediate file,
** the offset where the high and low pc attributes for the function are to be
** inserted, the offset where any new entries for inlined functions are to be
** inserted, a list of any children that need to be updated, and the debug_id
** for the function definition - needed to allow concrete instances of inline function
** to refer to the abstract entries.
*/
typedef struct 
{
	long		first_offset;
	long		last_offset;
	long		routine_offset;
	long		inline_insert;
	update_list	*updates;
	long		debug_id;
	char		has_been_inlined;
	char		out_of_line_instance;
} debug_updates;

extern const char	*db_symbols_file;	/* intermediate file, input from c++fe */
extern debug_updates	**region_updates;
extern int		debug_can_handle_inlines;

void	db_copy_inline(long start, long end);
void	db_begfile(long prologue_length);	/* set up for generating debugging info */

#undef DB_S_BLOCK
#undef DB_E_BLOCK
#define DB_S_BLOCK(i)	cg_q_int(db_s_block, i)
#define DB_E_BLOCK(i)	cg_q_int(db_e_block, i)

#endif /* ELFDEBUG_H */
