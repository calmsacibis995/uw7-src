#ident	"@(#)kern-i386:util/kdb/scodb/val.h	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1992 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#define		MXARGS		8

#define		T_CHAR_P	(T_CHAR|(DT_PTR<<N_BTSHFT))	/* char * */
#define		T_INT_F		(T_INT|(DT_FCN<<N_BTSHFT))	/* int () */
#define		T_INT_P		(T_INT|(DT_PTR<<N_BTSHFT))	/* int * */

#define		INC()		(++*s)
#define		INCBY(l)	(*s += l)
#define		WHITES(s)	{ while (white(**s)) INC() ; }

#define		IS_ARY(td)	(((td) & 03) == DT_ARY)
#define		IS_PTR(td)	(((td) & 03) == DT_PTR)
#define		IS_FCN(td)	(((td) & 03) == DT_FCN)
#define		IS_STUN(tb)	((tb) == T_STRUCT || (tb) == T_UNION)

#define		V_VALUE		0x01
#define		V_ADDRESS	0x02
#define		V_TEXT		0x10
#define		V_DATA		0x20

struct value {
	short		va_flags;
	unsigned long	va_seg;
	unsigned long	va_value;
	unsigned long	va_address;
	struct cvari	va_cvari;
	char		va_name[NAMEL];
};

#define		FF_VALID	0x01

struct func {
	int		 fu_flags;
	struct cvari	 fu_cvari;		/* declaration cvari */
	struct ilin	*fu_args;		/* arguments */
	struct ilin	*fu_cmds;		/* command lines */
};

/*
*	must be a value
*/
#define		MUST_VALUE(v)	{				\
			if (((v)->va_flags & V_VALUE) == 0) {	\
				scodb_error = e_not_val;	\
				return 0;			\
			}					\
		}

/*
*	stack checking stuff
*	stack doesn't need to be checked in a function if the function
*	doesn't call another function.
*		STACK_CLOSE	how close to the end of the stack
*				we can get
*
*		CHECK_STACK	if too close, fail.
*
*/
#ifdef STACKCHECK
# define	STACK_CLOSE	0x200
# define	CHECK_STACK(er)	{					\
			extern char *g_ebp();				\
			extern char scodb_stk[];			\
									\
			if (g_ebp() < (scodb_stk + STACK_CLOSE)) {	\
				extern char *scodb_error;		\
				extern char *e_stack_ovr;		\
									\
				scodb_error = e_stack_ovr;		\
				return (er);				\
			}						\
		}
#else
# define	CHECK_STACK(er)
#endif
