#ident	"@(#)optim:i386/debug.h	1.4"
#ifndef DEBUG_H
#define DEBUG_H

/*
**	This header defines the interface for the module, defined
**	in debug.c, which processes elf-style debugging information.
*/

/*
**	1st pass routines, called from pass1() in local.c
*/

void parse_debug(); /* called from yylex() in local.c */

void mod_debug(); /* called after register allocation has taken place */

void print_debug(); /* called from wrapup: print all .debug stuff */

/* routines to handle .line section */
extern init_line_flag;
void save_text_begin_label();
void init_line_section(); /* Call when we see .line info in .s file */
void exit_line_section();	/* from wrapup() */
void print_line_info(); /* called when intruction has uniqid info */
void print_FS_line_info(); /* called to emit line entry for 1st source
			      line of function */

extern enum Section section, prev_section;
	/* Control sections, defined in local.c. */
extern aflag;  /* True if we have to save fns in temp file */

#endif
