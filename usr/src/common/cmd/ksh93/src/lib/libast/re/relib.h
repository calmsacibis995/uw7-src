#ident	"@(#)ksh93:src/lib/libast/re/relib.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * regular expression library private definitions
 */

#ifndef _RELIB_H
#define _RELIB_H

#include <ast.h>

#define NCLASS		16		/* max # [...] expressions	*/
#define NMATCH		('9'-'0'+1)	/* max # (...) expressions	*/

typedef struct			/* sub-expression match			*/
{
	char*	sp;		/* start in source string		*/
	char*	ep;		/* end in source string			*/
} Match_t;

typedef struct			/* sub-expression match table		*/
{
	Match_t	m[NMATCH + 1];
} Subexp_t;

typedef struct			/* character class bit vector		*/
{
	char		map[UCHAR_MAX / CHAR_BIT + 1];
} Class_t;

typedef struct Inst		/* machine instruction			*/
{
	int		type;	/* <TOKEN ==> literal, otherwise action	*/
	union
	{

	int		sid;	/* sub-expression id for RBRA and LBRA	*/
	struct Inst*	other;	/* for right child			*/
	char*		cls;	/* CCLASS bit vector			*/

	} u;
	struct Inst*	left;	/* left child				*/
} Inst_t;

/*
 * NOTE: subexp must be the first element to match Re_program_t.match
 */

#define _RE_PROGRAM_PRIVATE_ \
	Subexp_t	subexp;		/* sub-expression matches	*/ \
	int		flags;		/* RE_* flags			*/ \
	Inst_t*		startinst;	/* start pc			*/ \
	Class_t		chrset[NCLASS];	/* .data			*/ \
	Inst_t		firstinst[5];	/* .text			*/

#include <re.h>

#define clrbit(set,bit)	(set[(bit)/CHAR_BIT]&=~(1<<((bit)%CHAR_BIT)))
#define setbit(set,bit)	(set[(bit)/CHAR_BIT]|=(1<<((bit)%CHAR_BIT)))
#define tstbit(set,bit)	((set[(bit)/CHAR_BIT]&(1<<((bit)%CHAR_BIT)))!=0)

#define	next	left
#define	subid	u.sid
#define right	u.other
#define cclass	u.cls

/*
 * tokens and actions
 *
 *	TOKEN<=x<OPERATOR are tokens, i.e. operands for operators
 *	>=OPERATOR are operators, value == precedence
 */

#define TOKEN		(UCHAR_MAX+1)
#define	ANY		(UCHAR_MAX+1)	/* `.' any character		*/
#define	NOP		(UCHAR_MAX+2)	/* no operation (internal)	*/
#define	BOL		(UCHAR_MAX+3)	/* `^' beginning of line	*/
#define	EOL		(UCHAR_MAX+4)	/* `$' end of line		*/
#define	BID		(UCHAR_MAX+5)	/* `\<' begin identifier	*/
#define	EID		(UCHAR_MAX+6)	/* `\>' end identifier		*/
#define	CCLASS		(UCHAR_MAX+7)	/* `[]' character class		*/
#define SUBEXPR		(UCHAR_MAX+8)	/* `\#' sub-expression		*/
#define	END		(UCHAR_MAX+9)	/* terminate: match found	*/

#define	OPERATOR	(UCHAR_MAX+11)
#define	START		(UCHAR_MAX+11)	/* start, stack marker		*/
#define	RBRA		(UCHAR_MAX+12)	/* `)' right bracket		*/
#define	LBRA		(UCHAR_MAX+13)	/* `(' left bracket		*/
#define	OR		(UCHAR_MAX+14)	/* `|' alternation		*/
#define	CAT		(UCHAR_MAX+15)	/* concatentation (implicit)	*/
#define	STAR		(UCHAR_MAX+16)	/* `*' closure			*/
#define	PLUS		(UCHAR_MAX+17)	/* a+ == aa*			*/
#define	QUEST		(UCHAR_MAX+18)	/* a? == 0 or 1 a's		*/

#endif
