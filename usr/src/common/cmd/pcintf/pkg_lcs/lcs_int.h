#ident	"@(#)pcintf:pkg_lcs/lcs_int.h	1.3"
/* SCCSID(@(#)lcs_int.h	7.2	LCC)	* Modified: 16:31:08 3/9/92 */
/*
 *  This file defines the LCS table formats
 *
 *  The file is organized as follows:
 *
 *	file_header
 *	input_header * lf_num_input_hdrs
 *	output_header * lf_num_output_hdrs
 *	input and output tables as needed
 *	multibyte table if needed
 */

struct file_header {
	char	fh_magic[4];		/* "LCS" */
	short	fh_num_input_hdrs;	/* Number of input headers */
	short	fh_num_output_hdrs;	/* Number of output headers */
	long	fh_multi_offset;	/* Offset of multibyte table */
	short	fh_multi_length;	/* Length of multibyte table */
	short	fh_default;		/* default character */
};

#define LCS_MAGIC	"LCS"		/* Magic token */


struct input_header {
	union {
		struct input_header *ihu_next;	/* Next input table */
		unsigned long ihu_offset;	/* Offset in file */
	} ih_un;
	unsigned short	ih_flags;	/* Flags (see below) */
	unsigned char	ih_start_code;	/* First code handled by this table */
	unsigned char	ih_end_code;	/* Last code handled by this table */
	union {
		struct {
			unsigned char	ihs_db_start;	/* db start code */
			unsigned char	ihs_db_end;	/* db end code */
		} ihu_st;
		short	ihu_length;	/* Length of dead char table */
	} ih_un2;
	unsigned short	ih_char_bias;	/* Bias for direct translations */
	lcs_char	ih_table[2];	/* Table contents (memory) */
};

#define ih_next		ih_un.ihu_next
#define ih_offset	ih_un.ihu_offset
#define ih_db_start	ih_un2.ihu_st.ihs_db_start
#define ih_db_end	ih_un2.ihu_st.ihs_db_end
#define ih_length	ih_un2.ihu_length

/* Flag definitions for ih_flags */
#define IH_DIRECT	0x0001	/* Input the character directly */
#define IH_DOUBLE_BYTE	0x0002	/* This table handles double byte input */
#define IH_DEAD_CHAR	0x0004	/* This table is a dead char exception table */


struct input_dead {
	unsigned char	id_code;	/* Code of composite character */
	unsigned char	id_fill;
	lcs_char	id_value;	/* Internal code of character */
};


struct output_header {
	union {
		struct output_header *ohu_next;	/* Next output table */
		unsigned long ohu_offset;	/* Offset in file */
	} oh_un;
	unsigned short	oh_flags;	/* Flags (see below) */
	unsigned short	oh_start_code;	/* First code handled by this table */
	unsigned short	oh_end_code;	/* Last code handled by this table */
	unsigned short	oh_char_bias;	/* Bias for direct translations */
	char		oh_table[4];	/* Table contents (memory) */
};

#define oh_next		oh_un.ohu_next
#define oh_offset	oh_un.ohu_offset

/* Flag definitions for oh_flags */
#define OH_DIRECT_CELL	0x0001	/* Output the cell directly */
#define OH_DIRECT_ROW	0x0002	/* Output the row directly */
#define OH_TABLE_4B	0x0004	/* The table entries are 4 bytes long */
#define OH_NO_LOWER	0x0008	/* Lower cell half codes are not selected */
#define OH_NO_UPPER	0x0010	/* Upper cell half codes are not selected */


struct output_table {
	unsigned char ot_flags;
	unsigned char ot_char;
	unsigned char ot_2chars[2];
};

/* Flag definitions for ot_flags */
#define OT_HAS_2B	0x01	/* This character has a 2 byte form */
#define OT_HAS_MULTI	0x02	/* This character has a multi-byte form */
#define OT_NOT_EXACT	0x04	/* This character is not an exact translation */


struct multi_byte {
	unsigned short	mb_code;	/* Code of multibyte character */
	unsigned char	mb_len;		/* Length of multibyte entry */
	unsigned char	mb_length;	/* Length of multibyte string */
	unsigned char	mb_text[1];	/* Multibyte text */
};


/*
 *  Table header for loaded table access
 */

struct table_header {
	struct input_header	*th_input;	/* First input table */
	struct output_header	*th_output;	/* First output table */
	unsigned char		*th_multi_byte;	/* Multibyte strings */
	int			 th_multi_length;
	short			 th_default;	/* default character */
	char			 th_magic[4];	/* Magic string "LCS" */
};


#ifndef O_BINARY
#define O_BINARY	0		/* DOS compatibility for opens */
#endif

#define lcs_tbl	struct table_header *

extern lcs_tbl	lcs_input_table;	/* input table for translations */
extern lcs_tbl	lcs_output_table;	/* output table for translations */
extern short	lcs_mode;		/* translation mode */
extern lcs_char	lcs_user_char;		/* user default character */
extern short	lcs_country;		/* country number */

/*
 * Flip macros for machines with big-endian architectures
 */
#define sflip(x) x = (((x >> 8) & 0xff) | ((x & 0xff) << 8))
#define lflip(x) x = (((x >> 24) & 0xff) | (((x >> 16) & 0xff) << 8) | \
		      (((x >> 8) & 0xff) << 16) | ((x & 0xff) << 24))
