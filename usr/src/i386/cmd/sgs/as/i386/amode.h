#ident	"@(#)nas:i386/amode.h	1.3"
/*
* i386/amode.h - i386 operand extensions for addressing modes
*
* Only included from "common/expr.h"
*/

/* These flags get set in Operand's oper_flags field. */

#define Amode_Literal	0x01	/* $expr operand */
#define Amode_Segment	0x02	/* oper_amode.seg filled in */
#define	Amode_BIS	0x04	/* at least one of base/index/scale set */
#define Amode_Indirect	0x08	/* "bogus" * prefix */
#define	Amode_FPreg	0x10	/* %st(n):  oper_expr contains n */

/* This is the Amode structure that forms a part of an Operand. */
typedef struct	/* [seg:] and [([base][,index[,scale]])] */
{
	Expr	*seg;	/* must become ExpTy_Register */
	Expr	*base;	/* must become ExpTy_Register */
	Expr	*index;	/* must become ExpTy_Register */
	Expr	*scale;	/* must become ExpTy_Integer (1, 2, 4, or 8) */
} Amode;
