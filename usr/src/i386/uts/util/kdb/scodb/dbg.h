#ident	"@(#)kern-i386:util/kdb/scodb/dbg.h	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1994 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#ifdef _KERNEL
#include <util/types.h>
#else
#include <sys/types.h>
#endif /* _KERNEL */

#define KCSSEL	0x100
#define KDSSEL	0x108
#define T_ESP	(-4)	/* TBD */

#define		MAXSEP		0xFFFFF
#define		MAXCOL		80
#define		NSPECREG	9	/* number of special registers */
#define		VLOFF		48	/* value offset on line for structs */

#define		NSYSREG		19	/* number of registers */
#define		NREGL		6

/*
*	any changes to FMXA must be reflected in
*		cmd.c
*			usage message for c_funccall (4 args...)
*		c_func.c
*			function call to the user's offset
*/
#define		FMXA		4

#define		NMEL(ar)	(sizeof(ar)/sizeof((ar)[0]))
#define		MAX(a, b)	((a) > (b) ? (a) : (b))
#define		MIN(a, b)	((a) < (b) ? (a) : (b))

/*
*	return codes from the functions
*/
#define		DB_RETURN	1
#define		DB_CONTINUE	2
#define		DB_ERROR	3
#define		DB_USAGE	4
#define		DB_FUNCRET	5

/*
*	debugger run modes
*/
#define		KERNEL		1
#define		FROMBP		2
#define		SINGLESTEP	3
#define		TRACEFUNCE	5
#define		GOTORET		6
#define		DBFUNC		7
#define		QUIT		8

#ifndef NULL
# define	NULL	0
#endif

#define		MD_BYTE		1
#define		MD_SHORT	2
#define		MD_LONG		4
#define		MD_SYM		5

#define DESC_BASE(a)	( ((long)(a)->a_base2431 << 24) |	\
			  ((long)(a)->a_base1623 << 16) |	\
				(a)->a_base0015 )

#define	LDT_DSCR	4

#define		LDTR		0
#define		GDTR		1
#define		IDTR		2
#define		TR		3
#define		PROC		4
#define		CR0		5
#define		CR1		6
#define		CR2		7
#define		CR3		8
#define			CR(x)	(CR0+(x))

#define		quit(c)		((c) == 0177 || (c) == 'q' || (c) == 'Q')

#define		U_OK		0
#define		U_NONE		1
#define		U_NOTINCORE	2


/*
*	how to access:
*/
#define		CSYM		'&'	/* symbol addresses	*/
#define		CREG		'%'	/* register values	*/
#define		CBKP		'#'	/* breakpoint addresses	*/
#define		CARG		'@'	/* argument values	*/
#define		CVAR		'$'	/* variable values	*/

#define		CMOD		'{'	/*}*/

#define		CSFLD		'.'	/* field name for sym	*/
#define		CSARB		'['
#define		CSARE		']'

#define		ISYM(c)		(c) == CSYM ||
#define		IREG(c)		(c) == CREG ||
#define		IBKP(c)		(c) == CBKP ||
#define		IARG(c)		(c) == CARG ||
#define		IVAR(c)		(c) == CVAR ||
#define		IMOD(c)		(c) == CMOD ||



#define		alphl(c)	((c) >= 'a' && (c) <= 'z')
#define		alphu(c)	((c) >= 'A' && (c) <= 'Z')
#define		hexcl(c)	((c) >= 'a' && (c) <= 'f')
#define		hexcu(c)	((c) >= 'A' && (c) <= 'F')
#define		white(c)	((c) == ' ' || (c) == '\t')
#define		numer(c)	((c) >= '0' && (c) <= '9')

/*
*	alpha		character is	alphabetic or _
*	numer				numeric
*	alnum				alpha or numeric
*	hexdi				hex digit
*/
#define		alpha(c)	(alphl(c) || alphu(c) || (c) == '_')
#define		alnum(c)	(alpha(c) || numer(c))
#define		octal(c)	((c) >= '0' && (c) <= '7')
#define		hexdi(c)	(numer(c) || hexcl(c) || hexcu(c))
#define		addrc(c)	(ISYM(c) IREG(c) IBKP(c) IARG(c) IVAR(c) IMOD(c) 0)
#define		calcc(c)	(addrc(c) || (c) == '*' || (c) == '(' || (c) == ')' || (c) == '-')

#define		ascii(c)	(((c) >= '!' && (c) <= '~') ? (c) : '.')




#define		DF_MXN		8
#define		AR_INF		-1

/*
*	if there is more than one dc_name then only the first is
*	printed, but no command matches will be made on it.
*/
struct dbcmd {
	char	*dc_name[DF_MXN];	/* names command is known by */
	char	dc_minargs;		/* min # arguments */
	char	dc_maxargs;		/* max # arguments */
	int	(*dc_func)();		/* func to call */
};

/*
*	when given choices...
*/
#define		k_accept(c)	((c) == 'y' || (c) == '\n')
#define		k_backup(c)	((c) == 'k' || (c) == '-')
#define		k_next(c)	((c) == 'j' || (c) == ' ' || (c) == 'n' || (c) == '+')


#define		VLOFF		48	/* value offset from pinfo() */

#define		PI_STEL		0x01
#define		PI_PSYM		0x02
#define		PI_DELTA	0x04

#define		DM_NO		0
#define		DM_YES		0x1
#define		DM_NONL		0x2
#define		DM_QUIT		0x4
#define		DM_LAST		0x8


/*
 * Bit fields in scodb_option variable.  Each bit defines an aspect of
 * SCODB functionality whose default behaviour can be changed.
 */

#define OPT_STACKSYM	1	/* enables symbolic dumping of function
				 * arguments during stack backtraces.
			 	 */
#define OPT_ORIGPS	2	/* original ps() output format */
#define OPT_NUM_WCHAN	4	/* output ps() waitchan field numerically */

#if !defined(STDALONE) && !defined(USER_LEVEL)
# if SCODB_STRINGS == 1
#  define	strcmp		scodb_strcmp
#  define	strcpy		scodb_strcpy
#  define	strlen		scodb_strlen
#  define	strncmp		scodb_strncmp
#  define	strncpy		scodb_strncpy
# endif
#endif

#ifdef USER_LEVEL

extern int	scodb_nlpp;

#define NLPP	scodb_nlpp

/*
 * This structure defines the table which contains the list of emulated
 * kernel functions which can be called from user level scodb (eg: ps()).
 */

struct func_table {
	char		*name;
	int		(*uaddr)();
};

/*
 * Definitions used to emulate the accessing of the proc structure
 */

extern struct proc *retrieve_proc(), *retrieve_proc_slot();
extern struct eproc *retrieve_eproc();
extern struct pregion *retrieve_pregion();
extern struct region *retrieve_region();
extern unsigned k_v_proc, kaddr_proc;

#define V_PROC			k_v_proc		/* v.v_proc */
#define PROC_SLOT(n)		((struct proc *) retrieve_proc_slot(n))
#define PROC_SLOT_KADDR(n)	((struct proc *) kaddr_proc + n)
#define PROC_PTR(p)		((struct proc *) retrieve_proc(p))
#define EPROC_PTR(ep)		((struct eproc *) retrieve_eproc(ep))
#define PREGION_PTR(prp)	((struct pregion *) retrieve_pregion(prp))
#define REGION_PTR(rp)		((struct region *) retrieve_region(rp))

/*
 * Definitions used to emulate accessing of the U-page
 */

extern unsigned kaddr_u;
extern struct user upage;

#define		U_KADDR		((struct user *)kaddr_u)

#define putchar(c)	check_putchar(c)
#define printf		check_printf


#else /* !USER_LEVEL */

#define		NLPP		20

/*
 * Conversion between an index in the regnames[] array and a register
 * trap offset value (T_* in svc/reg.h).
 */

#define T_REGNAME(x)	(T_SS - x)

#define USYM_MAGIC	0x5C0DB001

struct scodb_usym_info {
	unsigned	magic;		/* 0x5C0DB001 (offset 0) */
	int		stun_offset;
	int		stun_size;
	int		vari_offset;
	int		vari_size;
	int		lineno_offset;
	int		lineno_size;
	int		lsym_offset;
	int		lsym_size;
};

#ifdef NEVER

#define V_PROC			v.v_proc
#define PROC_SLOT(n)		((struct proc *)&proc[n])
#define PROC_SLOT_KADDR(n)	((struct proc *)&proc[n])
#define PROC_PTR(p)		(p)
#define EPROC_PTR(ep)		(ep)
#define PREGION_PTR(prp)	(prp)
#define REGION_PTR(rp)		(rp)

/*
 * Definitions used to emulate accessing of the U-page
 */

#define U_KADDR			&u

#endif /* NEVER */


#endif

