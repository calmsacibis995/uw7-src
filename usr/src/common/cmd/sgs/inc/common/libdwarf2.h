#ifndef LIB_DWARF2_H
#define LIB_DWARF2_H
#ident	"@(#)sgs-inc:common/libdwarf2.h	1.6"

#include <time.h>

typedef unsigned char byte;
typedef unsigned long address_t;

typedef struct Dwarf2_Attribute
{
	unsigned long	name;
	unsigned long	form;
} Dwarf2_Attribute;

typedef struct Dwarf2_Abbreviation
{
	unsigned long		tag;
	int			nattr;
	int			children;
	const Dwarf2_Attribute	*attributes;
} Dwarf2_Abbreviation;

typedef struct Dwarf2_Line_entry
{
	address_t	address;
	unsigned long	line;
	unsigned long	file_index;
} Dwarf2_Line_entry;

typedef struct Dwarf2_File_entry
{
	const char	*name;
	unsigned int	dir_index;	/* index into array returned by dwarf2_get_include_table */
	time_t		time;
} Dwarf2_File_entry;

#ifdef __cplusplus
extern "C" {
#endif

extern const char *dwarf2_tag_name(unsigned int);
extern const char *dwarf2_attribute_name(unsigned int);
extern const char *dwarf2_form_name(unsigned int);
extern const char *dwarf2_language_name(unsigned int);
extern const char *dwarf2_location_op_name(unsigned int);
extern const char *dwarf2_base_type_encoding_name(unsigned int);

extern int dwarf2_encode_signed(long, byte *);
extern int dwarf2_encode_unsigned(unsigned long, byte *);
extern int dwarf2_decode_signed(long *, byte *);
extern int dwarf2_decode_unsigned(unsigned long *, byte *);

/* dwarf2_get_abbreviation_table reads an abbreviation table from
 * an object file and allocates space for
 * the abbreviation table and lists of attributes.
 * It returns 0 for failure.
 * dwarf2_delete_abbreviation_table should be called to free
 * the value returned by dwarf2_get_abbreviation_table
 *
 * dwarf2_gen_abbreviation_table creates a standard abbreviation table
 * to be shared by the tools that create debugging information
 * (acomp, cplusfe, and cplusbe) or that need to read the debugging
 * information in .s or .o files without a .debug_abbrev section
 * (dump, basicblk, etc.)
 */

extern Dwarf2_Abbreviation *dwarf2_get_abbreviation_table(unsigned char *,
					unsigned int max_size,
					unsigned int *bytes_used);
extern void dwarf2_delete_abbreviation_table(Dwarf2_Abbreviation *);

extern const Dwarf2_Abbreviation *dwarf2_gen_abbrev_table(void);

/* The next few functions are used to read line number information.
 * They operate on a .o level, and would have to be used in a loop
 * to read multiple line number tables in one section in an executable.
 *
 * dwarf2_read_line_header must be called before any of the other functions.
 * The other functions may be called in any order. dwarf2_read_line_header
 * returns 1 on success, 0 on failure.  It also returns, in the second argument,
 * the number of bytes in the current line number table.
 *
 * dwarf2_get_file_table, dwarf2_get_line_table, dwarf2_get_include_table,
 * and dwarf2_get_opcode_length_table return pointers
 * to an array of entries.  The number of entries in the array is returned
 * in the argument passed in. 
 * These functions all return  NULL on failure.
 *
 * The first entries of the arrays returned by dwarf2_get_include_table
 * and dwarf2_get_file_table are both zero.  In the include table, the
 * zero element represents the compilation directory (that information
 * is available in the DW_TAG_compile_unit entry in the .debug_info section.)
 *
 * The tables returned by dwarf2_get_file_table, dwarf2_get_include_table,
 * and dwarf2_get_opcode_length_table will be overwritten on subsequent
 * calls to the functions, so it is the responsibility of the calling
 * function to make copies of the data if needed.
 *
 * The table returned by dwarf2_get_line_table is a special case.
 * On each call to dwarf2_get_line_table, the function by default appends
 * line number entries to the end of the table, so that the calling program
 * can create a list of all the line number entries in the executable
 * if necessary.  The line number table may be reset between calls (so that
 * it returns only those line number entries within a single .o) by calling
 * dwarf2_reset_line_table.  The argument to dwarf2_reset_line_table specifies
 * whether to free the allocated space (free_space == 1, typically done
 * after processing all compilation units), or to reuse the space already
 * allocated (free_space == 0).  The data in table returned by
 * dwarf2_get_line_table is ordered as it is in the file.
 * It is the calling function's responsibility to sort it if necessary.
 */
extern int			dwarf2_read_line_header(unsigned char *, unsigned long *);
extern const Dwarf2_Line_entry	*dwarf2_get_line_table(unsigned long *nentries);
extern const Dwarf2_File_entry	*dwarf2_get_file_table(unsigned long *nentries);
extern const char		**dwarf2_get_include_table(unsigned long *nentries);
extern int			*dwarf2_get_opcode_length_table(unsigned long *nentries);
extern void			dwarf2_reset_line_table(int free_space);

#ifdef __cplusplus
}
#endif

#endif /* LIB_DWARF2_H */
