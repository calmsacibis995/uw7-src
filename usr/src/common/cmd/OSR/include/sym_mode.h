#ident	"@(#)OSRcmds:include/sym_mode.h	1.1"
#pragma comment(exestr, "@(#) sym_mode.h 25.1 92/08/12 ")
/*
 *	Copyright (C) 1992 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 *
 *	sym_mode.h - header file for symbolic mode processing
 *
 * MODIFICATION HISTORY
 *
 *	20 Feb 1992	scol!markhe	creation
 *	11 Aug 1992	scol!markhe
 *		- Changed the type of the first argument in the comp_mode
 *		  prototype (from register char *).
 */

typedef struct action_t {
	unsigned short	bits;
	unsigned short	who_bits;
	unsigned short	flags;
} action_atom;

#define		MODE_NOERROR	0
#define		MODE_INVALID	1
#define		MODE_MEMORY	2

action_atom	*comp_mode(const char *, mode_t);
unsigned int	exec_mode(int, action_atom *);

extern int mode_err;
