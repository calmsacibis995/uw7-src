#ident	"@(#)OSRcmds:lib/libos/sym_mode.c	1.1"
#pragma comment(exestr, "@(#) sym_mode.c 25.1 92/08/12 ")
/*
 *	Copyright (C) 1992 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 *	sym_mode.c - symbolic mode processing routines.
 */
/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level
 * SCO_DEVSYS 		SCO Development System Extension
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension
 * SCO_REALTIME 	SCO Realtime Extension
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)
 * SCO_TOOLS 		SCO System Administration Tools
 * SCO_FS 		Alternate File Systems
 * SCO_GAMES 		SCO Games
 */

/* BEGIN SCO_BASE */

/* MODIFICATION HISTORY
 *
 *	20 Feb 1992	scol!markhe	created
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "../../include/sym_mode.h"
#include "../../include/osr.h"

#define	USER_BITS	(S_IRWXU | S_ISUID | S_ISVTX)
#define	GROUP_BITS	(S_IRWXG | S_ISGID)
#define	OTHER_BITS	 S_IRWXO
#define	READ_BITS	(S_IRUSR | S_IRGRP | S_IROTH)
#define	WRITE_BITS	(S_IWUSR | S_IWGRP | S_IWOTH)
#define	EXEC_BITS	(S_IXUSR | S_IXGRP | S_IXOTH)
#define	SETID_BITS	(S_ISUID | S_ISGID)
#define	STICKY_BITS	 S_ISVTX
#define	LOCK_BITS	 S_ISGID

/* values and masks for `flags' member in action_atom */
#define COPY_FROM_BIT		0001
#define	COPY_FROM_MASK		0007
#define	COPY_FROM_OWNER		0001
#define	COPY_FROM_GROUP		0003
#define	COPY_FROM_OTHER		0005
#define	X_PERM			0010
#define	LOCKING			0020
#define PLUS_OP			0100
#define MINUS_OP		0200
#define EQUAL_OP		0300
#define	OP_MASK			0700
#define END_OP			0777

/* used to return error status */
int mode_err;

/* prototypes */
static const char *who(register const char *, unsigned short *, mode_t);
static int atooctal(const char *);


/*
 * compile a symbolic_mode into a link list of actions
 */
action_atom *
comp_mode(register const char *string, mode_t usr_mask)
{
	register action_atom *action_current;
	action_atom *action;
	unsigned short who_bits;
	const char *start;
	int flag;
	int count;

	mode_err = MODE_NOERROR;

	/* try to change to octal */
	if((flag = atooctal(string)) != -1) {
		if (flag > 07777) {
			mode_err = MODE_INVALID;
			return((action_atom *) 0);
		}
		/* add value to first, and only, atom in list */
		if ((action = (action_atom *) malloc(sizeof(action_atom)*2)) == (action_atom *) 0) {
			mode_err = MODE_MEMORY;
			return((action_atom *) 0);
		}
		action[0].bits = flag;
		action[0].flags = EQUAL_OP;
		action[0].who_bits = 07777;
		action[1].flags = END_OP;

		return(action);
	}

	/* calculate the number of actions in the string */
	for( start = string, count = 0; *string; ++string)
		if (*string == '-' || *string == '+' || *string == '=')
			count++;
	count++;	/* allow for END_OP */
	string = start;

	if ((action = (action_atom *) malloc((sizeof(action_atom)*count))) == (action_atom *) 0) {
		mode_err = MODE_MEMORY;
		return((action_atom *) 0);
	}
	count = 0;

	/*
	 * loop for each clause in the symbolic mode string
	 */
	do {

		string = who(string, &who_bits, usr_mask);

		while (*string == '=' || *string == '+' || *string == '-') {
			action_current = &action[count];
			count++;
			action_current->flags = (*string == '-' ? MINUS_OP : ( *string == '+' ? PLUS_OP : EQUAL_OP));
			action_current->who_bits = who_bits;
			action_current->bits = 0;

			for (flag=1; flag;) {
				switch(*++string) {
					case 'r':
						action_current->bits |= (READ_BITS & who_bits);
						break;
					case 'w':
						action_current->bits |= (WRITE_BITS & who_bits);
						break;
					case 'x':
						action_current->bits |= (EXEC_BITS & who_bits);
						break;
					case 's':
						action_current->bits |= (SETID_BITS & who_bits);
						break;
					case 't':
						action_current->bits |= (STICKY_BITS & who_bits);
						break;
					case 'u':
						if (action_current->flags & COPY_FROM_MASK) {
							mode_err = MODE_INVALID;
							return((action_atom *) 0);
						}
						action_current->bits = who_bits;
						action_current->flags |= COPY_FROM_OWNER;
						break;
					case 'g':
						if (action_current->flags & COPY_FROM_MASK) {
							mode_err = MODE_INVALID;
							return((action_atom *) 0);
						}
						action_current->bits = who_bits;
						action_current->flags |= COPY_FROM_GROUP;
						break;
					case 'o':
						if (action_current->flags & COPY_FROM_MASK) {
							mode_err = MODE_INVALID;
							return((action_atom *) 0);
						}
						action_current->bits = who_bits;
						action_current->flags |= COPY_FROM_OTHER;
						break;
					case 'l':
						action_current->bits = LOCK_BITS;;
						action_current->flags |= LOCKING;
						break;

					case 'X':
						action_current->flags |= X_PERM;
						action_current->bits |= (EXEC_BITS & who_bits);
						break;
					default:
						flag = 0;
						break;
				}/*switch*/
			}/*for*/
		}/*while*/
	} while (*string == ',' && string++);


	if (*string != '\0') {
		mode_err = MODE_INVALID;
		return((action_atom *) 0);
	}

	action[count].flags = END_OP;

	return(action);
}


/*
 * execute a link list of actions on a give mode
 */
unsigned int
exec_mode(int old_mode, action_atom *action)
{
	register action_atom *action_current;
	register unsigned short new_mode;
	register unsigned short bits;
	int count;

	new_mode = old_mode & 07777;

	for (count = 0; action[count].flags != END_OP; count++) {
		action_current = &action[count];
		if (action_current->flags & COPY_FROM_BIT) {
			bits = action_current->who_bits;
			switch(action_current->flags & COPY_FROM_MASK) {
				case COPY_FROM_OWNER:
					bits &= ((new_mode & 0700) >> 3 | (new_mode & 0700) >> 6 | (new_mode & 0700));
					break;
				case COPY_FROM_GROUP:
					bits &= ((new_mode & 0070) << 3 | (new_mode & 0070) >> 3 | (new_mode & 0070));
					break;
				default:	/* COPY_FROM_OTHER */
					bits &= ((new_mode & 0007) << 6 | (new_mode & 0007) << 3 | (new_mode & 0007));
					break;
			}/*switch*/
		} else
			bits = action_current->bits;

		/* the locking is a bit of a kludge */
		if (action_current->flags & LOCKING)
			switch(action_current->flags & OP_MASK) {
				case PLUS_OP:
				case EQUAL_OP:
					if (!(new_mode & S_IXGRP)) {
						new_mode |= LOCK_BITS;
					}
					break;
				case MINUS_OP:
					if ((new_mode & S_ISGID) && !(new_mode & S_IXGRP)) {
						new_mode &= ~LOCK_BITS;
					}
					break;
			}

		if ((action_current->flags & X_PERM) && !S_ISDIR(old_mode) &&
		    !(old_mode & EXEC_BITS))
			bits &= ~EXEC_BITS;

		switch(action_current->flags & OP_MASK) {
			case PLUS_OP:
				new_mode |= bits;
				break;
			case MINUS_OP:
				new_mode &= ~bits;
				break;
			case EQUAL_OP:
				new_mode = (new_mode & ~action_current->who_bits) | bits;
				break;
		}
	}/*for*/

	return(new_mode);
}


static const char *
who(register const char *string, register unsigned short *who_bits, mode_t usr_mask)
{
	
	*who_bits = 0;

	--string;
	while (1)
		switch (*++string) {
			case 'u':
				*who_bits |= USER_BITS;
				break;
			case 'g':
				*who_bits |= GROUP_BITS;
				break;
			case 'o':
				*who_bits |= OTHER_BITS;
				break;
			case 'a':
				*who_bits |= USER_BITS | GROUP_BITS | OTHER_BITS;
				break;
			default:
				if (*who_bits == 0)
					*who_bits = (USER_BITS | GROUP_BITS | OTHER_BITS) & ~usr_mask;

				return(string);
		}
}



static int
atooctal(register const char *string)
{
	register int value = 0;

	for (; *string; string++) {
		if (strchr("01234567", *string) == (char *) 0)
			return(-1);
		value = value * 8 + toint(*string);
	}

	return(value);
}
/* END SCO_BASE */
