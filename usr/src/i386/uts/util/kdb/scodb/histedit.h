#ident	"@(#)kern-i386:util/kdb/scodb/histedit.h	1.1"
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

#define		DBIBFL		128	/* input buf length */
#define		DBNARG		16	/* # args to break up input */

#define		CTRL(c)		(c - 'A' + 1)

/*
*	in space.c:
*/
extern char	*c_move_up;
extern char	*c_move_down;
extern char	*c_move_right;
extern char	*c_move_left;
extern char	*c_delete_line;
extern char	*c_delete_char;
extern char	*c_startinsert;
extern char	*c_endinsert;
extern char	*c_insert_char;
extern char	*c_insert_line;
extern char	*c_clear_to_eol;
extern char	*c_reverse;
extern char	*c_normal;
extern char	*c_move_cursor;

#ifndef USER_LEVEL

/*
*	the cursor stuff returns 0 if not possible, 1 if done
*/
#define		_STR_PUT(x)	((c_ ## x) ? (printf(c_ ## x), 1) : 0)

#define		up()		_STR_PUT(move_up)
#define		down()		_STR_PUT(move_down)
#define		right()		_STR_PUT(move_right)
#define		delch()		_STR_PUT(delete_char)
#define		delline()	_STR_PUT(delete_line)
#define		startinsert()	_STR_PUT(startinsert)
#define		endinsert()	_STR_PUT(endinsert)
#define		insch()		_STR_PUT(insert_char)
#define		insline()	_STR_PUT(insert_line)
#define		clrtoeol()	_STR_PUT(clear_to_eol)
#define		backsp()	_STR_PUT(move_left)
#define		reverse()	_STR_PUT(reverse)
#define		normal()	_STR_PUT(normal)

#define		goto_col(col)	printf(c_move_cursor, p_row() + 1, col);

#endif

#define		goloc(n)	goto_col(prlen + n + 1);



/*
*	buffers are circular
*/
struct ilin {
	int		 il_narg;
	char		 il_ibuf[DBIBFL];
	char		*il_ivec[DBNARG];
	struct ilin	*il_next;
};

#define		freebuf(il)	((il)->il_narg = 0)

#define		MXLIBUF		128
#define		MXPRL		32
#define		ntob(l, n)	((n) % (l)->li_mod)
#define		dtoc(n)		((n) + '0')

#define		LF_CANERR	0x00000001
#define		LF_WRAPS	0x00000002
#define		LF_EDITING	0x00000004
#define			LF	0x0000000F
#define		LF_PLN		0x0000FFF0
#define			LFPLS	4
#define		LF_PLX(l,n)	((l)->li_flag |= (((n) << LFPLS) & LF_PLN))
#define		LF_PL(l)	(((l)->li_flag & LF_PLN) >> LFPLS)

#define		prlen		(list->li_pp - list->li_prompt)

#define		LB_SAVED	0x01
#define		LB_SEARCHED	0x02
struct lsbu {
	int		lb_flag;
	int		lb_hc;
	int		lb_vc;
	int		lb_nc;
	char		lb_save[DBIBFL];
	char		lb_search[DBIBFL];
};

struct scodb_list {
	int		  li_flag;
	int		  li_rflag;
	int		  li_mod;
	int		  li_minum;
	int		  li_mxnum;
	int		  li_curnum;	/* current line number */
	char		  li_prompt[MXPRL];
	char		 *li_pp;
	struct lsbu	  li_lsbuf;
	struct ilin	 *li_bufl;	/* list			*/
	struct ilin	**li_buffers;	/* array of pointers	*/
};

/*				      FFFF	taken by LF_*	*/
#define		ED_CANCEL	0x00000000
#define		ED_OK		0x00010000
#define		ED_EDIT		0x00020000
#define			ED	0x00070000

#define		OP_GL		0x00080000
#define		OP_DL		0x00100000
#define		OP_AL		0x00200000
#define		OP_CO		0x00400000
#define		OP_RT		0x00500000	/* return */
#define		OP_SR		0x00800000
#define		OP_SF		0x00900000	/* SR forw */
#define			OP	0x00F80000

#define		OP__LN		0xFF000000
#define		OP__LS		24		/* - enough?	*/
#define			LN(x)	(((x) & OP__LN) >> OP__LS)
#define			LS(n)	((n) << OP__LS)
