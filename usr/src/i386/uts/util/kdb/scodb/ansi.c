#ident	"@(#)kern-i386:util/kdb/scodb/ansi.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

char	*c_move_up	=	"\033[A";
char	*c_move_down	=	"\033[B";
char	*c_move_right	=	"\033[C";
char	*c_move_left	=	"\b"	;
char	*c_delete_line	=	"\033[M";
char	*c_delete_char	=	"\033[P";
char	*c_insert_char	=	"\033[@";
char	*c_insert_line	=	"\033[L";
char	*c_clear_to_eol	=	"\033[K";
char	*c_startinsert	=	0;
char	*c_endinsert	=	0;
char	*c_reverse	=	"\033[7m";
char	*c_normal	=	"\033[m";
char	*c_move_cursor	=	"\033[%d;%dH";

int scodb_nlpp = 20;
int scodb_nlines = 25;
int scodb_ncols = 80;
