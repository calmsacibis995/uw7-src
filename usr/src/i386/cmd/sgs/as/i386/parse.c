#ident	"@(#)nas:i386/parse.c	1.9"
/*
* i386/parse.c - i386 assembler parsing code
*
* Only included from "common/gram.c"
*/
#include "common/stmt.h"	/* for Inst */
#include "stmt386.h"
#include <unistd.h>

#define TOKEN_LPAREN	(TOKEN_AVAIL + 0)
#define TOKEN_RPAREN	(TOKEN_AVAIL + 1)
#define TOKEN_LBRACKET	(TOKEN_AVAIL + 2)
#define TOKEN_RBRACKET	(TOKEN_AVAIL + 3)
#define TOKEN_EQUAL	(TOKEN_AVAIL + 4)
#define TOKEN_INDIRECT	(TOKEN_AVAIL + 5)
#define TOKEN_DOLLAR	(TOKEN_AVAIL + 6)


#define	TOKEN_ASSIGN	TOKEN_EQUAL	/* Enable "sym=val" form of .set. */

static const struct token_info toktab[TOKEN_DOLLAR + 1] =
{
	/*ExpOp_LeafRegister*/	{"register",		TOKTY_BUFFER},
	/*ExpOp_LeafName*/	{"name",		TOKTY_BUFFER},
	/*ExpOp_LeafString*/	{"string constant",	TOKTY_BUFFER},
	/*ExpOp_LeafInteger*/	{"integer constant",	TOKTY_BUFFER},
	/*ExpOp_LeafFloat*/	{"float constant",	TOKTY_BUFFER},
	/*ExpOp_LeafCode*/	{0},

	/*ExpOp_Multiply*/	{"\\*",	TOKTY_BINARY},
	/*ExpOp_Divide*/	{"\\/",	TOKTY_BINARY},
	/*ExpOp_Remainder*/	{"\\%",	TOKTY_BINARY},
	/*ExpOp_Add*/		{"+",	TOKTY_BINARY | TOKTY_PREFIX},
	/*ExpOp_Subtract*/	{"-",	TOKTY_BINARY | TOKTY_PREFIX},
	/*ExpOp_And*/		{"&",	TOKTY_BINARY},
	/*ExpOp_Or*/		{"|",	TOKTY_BINARY},
	/*ExpOp_Xor*/		{"^",	TOKTY_BINARY},
	/*ExpOp_Nand*/		{"!",	TOKTY_BINARY},
	/*ExpOp_LeftShift*/	{"<<",	TOKTY_BINARY},
	/*ExpOp_RightShift*/	{">>",	TOKTY_BINARY},
	/*ExpOp_Maximum*/	{0},
	/*ExpOp_LCM*/		{0},

	/*ExpOp_Complement*/	{"~",	TOKTY_PREFIX},
	/*ExpOp_UnaryAdd*/	{0},		/* created by unaryexpr() */
	/*ExpOp_Negate*/	{0},		/* created by unaryexpr() */
	/*ExpOp_SegmentPart*/	{"<s>",	TOKTY_PREFIX},
	/*ExpOp_OffsetPart*/	{"<o>",	TOKTY_PREFIX},

	/*ExpOp_LowMask*/	{0},
	/*ExpOp_HighMask*/	{0},
	/*ExpOp_HighAdjustMask*/{0},

	/*ExpOp_Pic_PC*/	{0},
	/*ExpOp_Pic_GOT*/	{"@GOT",	TOKTY_IDPICOP},
	/*ExpOp_Pic_PLT*/	{"@PLT",	TOKTY_IDPICOP},
	/*ExpOp_Pic_GOTOFF*/	{"@GOTOFF",	TOKTY_POSTFIX},
	/*ExpOp_Pic_BASEOFF*/	{"@BASEOFF",	TOKTY_POSTFIX},

	/*TOKEN_EOF*/		{"end-of-file"},
	/*TOKEN_EOS*/		{"; or new-line"},
	/*TOKEN_COMMA*/		{","},
	/*TOKEN_COLON*/		{":"},

	/*TOKEN_LPAREN*/	{"("},
	/*TOKEN_RPAREN*/	{")"},
	/*TOKEN_LBRACKET*/	{"[",	TOKTY_LEFT},
	/*TOKEN_RBRACKET*/	{"]",	TOKTY_RIGHT},
	/*TOKEN_EQUAL*/		{"="},
	/*TOKEN_INDIRECT*/	{"*"},
	/*TOKEN_DOLLAR*/	{"$"},
};

static int regno;	/* register number returned by lookupreg() */

static Operand *
#ifdef __STDC__
operparse(void)	/* operand -> $ exp | pre re | pre (disp) | pre re (disp) */
#else		/* pre -> | * | reg : | * reg : */
operparse()	/* disp -> re | , re | re , re | , re , exp | re, re, exp */
#endif		/* re -> reg | exp */
{
	register Expr *ep;
	register Operand *op;
	register int ct;
	register int opflags = 0;
	Expr *seg = 0;

	if ((ct = curtok) == TOKEN_DOLLAR)	/* $ exp */
	{
		curtok = yylex();
		if ((ep = exprparse()) == 0)
			return 0;
		op = operand(ep);
		op->oper_flags = Amode_Literal;
		return op;
	}
	if (ct == TOKEN_INDIRECT)	/* "bogus" '*' prefix */
	{
		opflags = Amode_Indirect;
		curtok = ct = yylex();
	}
	if (ct != ExpOp_LeafRegister)	/* possibly empty expression */
	{
		if (ct == TOKEN_LPAREN)
			ep = 0;
		else
		{
			if ((ep = exprparse()) == 0)
				return 0;
			ct = curtok;
		}
	}
	else if (regno == Reg_st)	/* special 80x87 stack register */
	{
		/* Entire operand must be either "%st" or "%st(intexpr)". */
		if (opflags != 0)
			return 0;
		if ((ct = yylex()) != TOKEN_LPAREN)
		{
			ep = regexpr(Reg_st);
			curtok = ct;
		}
		else
		{
			curtok = yylex();
			if ((ep = exprparse()) == 0 || curtok != TOKEN_RPAREN)
				return 0;
			opflags |= Amode_FPreg;
			curtok = yylex();
		}
		op = operand(ep);
		op->oper_flags = opflags;
		return op;
	}
	else	/* regular register */
	{
		ep = regexpr(regno);
		ct = yylex();
	}
	if (ct == TOKEN_COLON)	/* ep is segment prefix */
	{
		if (ep->ex_op != ExpOp_LeafRegister
			&& ep->ex_op != ExpOp_LeafName)
		{
			curtok = ct;
			return 0;
		}
		seg = ep;
		opflags |= Amode_Segment;
		curtok = ct = yylex();
		ep = 0;
		switch (ct)
		{
		case ExpOp_LeafRegister:
			if (regno == Reg_st)
				return 0;	/* disallow %seg:%st */
			ep = regexpr(regno);
			curtok = ct = yylex();
			break;
		default:
			ep = exprparse();
			ct = curtok;
			break;
		case TOKEN_LPAREN:
			break;
		}
	}
	op = operand(ep);
	if ((ep = seg) != 0)	/* had segment register */
	{
		op->oper_amode.seg = ep;
		ep->ex_cont = Cont_Operand;
		ep->parent.ex_oper = op;
	}
	if (ct != TOKEN_LPAREN)
		curtok = ct;
	else			/* have base/index/scale */
	{
		opflags |= Amode_BIS;	/* assume base/index/scale set */
		if ((ct = yylex()) != TOKEN_COMMA)	/* base */
		{
			if (ct == ExpOp_LeafRegister)
				ep = regexpr(regno);
			else if (ct == ExpOp_LeafName)
				ep = idexpr(inbuf + token_beg, token_len);
			else
			{
				curtok = ct;
				return 0;
			}
			ct = yylex();
			op->oper_amode.base = ep;
			ep->ex_cont = Cont_Operand;
			ep->parent.ex_oper = op;
		}
		if (ct != TOKEN_RPAREN)		/* index */
		{
			if (ct != TOKEN_COMMA)
			{
				curtok = ct;
				return 0;
			}
			if ((ct = yylex()) == ExpOp_LeafRegister)
				ep = regexpr(regno);
			else if (ct == ExpOp_LeafName)
				ep = idexpr(inbuf + token_beg, token_len);
			else
			{
				curtok = ct;
				return 0;
			}
			ct = yylex();
			op->oper_amode.index = ep;
			ep->ex_cont = Cont_Operand;
			ep->parent.ex_oper = op;
			if (ct != TOKEN_RPAREN)		/* scale */
			{
				if (ct != TOKEN_COMMA)
				{
					curtok = ct;
					return 0;
				}
				curtok = yylex();
				if ((ep = exprparse()) == 0)
					return 0;
				op->oper_amode.scale = ep;
				ep->ex_cont = Cont_Operand;
				ep->parent.ex_oper = op;
				if (curtok != TOKEN_RPAREN)
					return 0;
			}
		}
		curtok = yylex();
	}
	op->oper_flags = opflags;
	return op;
}

static int
#ifdef __STDC__
attoken(void)	/* look up @ operator; massage old-style @{type,attr}'s */
#else
attoken()
#endif
{
	register Uchar *p = inbuf + current;

	/*
	* Allow nonoperator versions for compatibility.
	*	@0x...	  -> 0x...	(number)
	*	@note	  -> note	(string)
	*	@nobits	  -> nobits	(string)
	*	@object	  -> object	(string)
	*	@strtab	  -> strtab	(string)
	*	@symtab	  -> symtab	(string)
	*	@no_type  -> none	(string)
	*	@function -> function	(string)
	*	@progbits -> progbits	(string)
	*/
	if (!name())
	{
		if (chtab[*p] & CH_DIG)
		{
			if (numtoken())	/* floating constant */
			{
				error(gettxt(":945","invalid old-style @ token: @%s"),
					prtstr(p, token_len));
			}
			return ExpOp_LeafInteger;
		}
		error(gettxt(":946","invalid @ operator"));
	}
	else
	{
		/*
		* Hardcoded, hopefully fast operator recognition.
		*/
		switch (token_len)
		{
		case 3:
			if (p[2] == 'T')
			{
				if (p[1] == 'O')
				{
					if (p[0] == 'G')
						return ExpOp_Pic_GOT;
				}
				else if (p[1] == 'L' && p[0] == 'P')
					return ExpOp_Pic_PLT;
			}
			break;
		case 4:
			if (p[0] == 'n' && p[1] == 'o' && p[2] == 't'
				&& p[3] == 'e')
			{
				return ExpOp_LeafString;
			}
			break;
		case 6:
			if (p[0] == 'G')
			{
				if (p[1] == 'O' && p[2] == 'T' && p[3] == 'O'
					&& p[4] == 'F' && p[5] == 'F')
				{
					return ExpOp_Pic_GOTOFF;
				}
			}
			else if (p[0] == 'o')
			{
				if (p[1] == 'b' && p[2] == 'j' && p[3] == 'e'
					&& p[4] == 'c' && p[5] == 't')
				{
					return ExpOp_LeafString;
				}
			}
			else if (p[0] == 'n')
			{
				if (p[1] == 'o' && p[2] == 'b' && p[3] == 'i'
					&& p[4] == 't' && p[5] == 's')
				{
					return ExpOp_LeafString;
				}
			}
			else if (p[0] == 's' && p[3] == 't' && p[4] == 'a'
				&& p[5] == 'b')
			{
				if ((p[1] == 'y' && p[2] == 'm')
					|| (p[1] == 't' && p[2] == 'r'))
				{
					return ExpOp_LeafString;
				}
			}
			break;
		case 7:
			if (p[0] == 'n'
				&& strncmp("o_type", (const char *)&p[1],
				(size_t)6) == 0)
			{
				p[2] = 'n';
				p[3] = 'e';
				token_len = 4;
				return ExpOp_LeafString;
			}
			else if (p[0] == 'B' &&
				strncmp("ASEOFF", (const char *)&p[1],
					(size_t)6) == 0)

			{
					return ExpOp_Pic_BASEOFF;
			}
			break;
		case 8:
			if (p[0] == 'f')
			{
				if (strncmp("unction", (const char *)&p[1],
					(size_t)7) == 0)
				{
					return ExpOp_LeafString;
				}
			}
			else if (p[0] == 'p'
				&& strncmp("rogbits", (const char *)&p[1],
				(size_t)7) == 0)
			{
				return ExpOp_LeafString;
			}
			break;
		}
		error(gettxt(":947","invalid operator: @%s"), prtstr(p, token_len));
	}
	return ExpOp_Pic_GOTOFF;
}

static int
#ifdef __STDC__
lookupreg(void)	/* look up register name; return register number */
#else
lookupreg()
#endif
{
	register Uchar *p = inbuf + token_beg--;
	register int len = token_len++;

	/*
	* Hardcoded, hopefully fast register name recognition.
	*/
	if (len >= 2)
	{
		switch (p[0])
		{
		case 'a':
			if (len != 2)
				break;
			switch (p[1])
			{
			case 'h':
				return Reg_ah;
			case 'l':
				return Reg_al;
			case 'x':
				return Reg_ax;
			}
			break;
		case 'b':
			if (len != 2)
				break;
			switch (p[1])
			{
			case 'h':
				return Reg_bh;
			case 'l':
				return Reg_bl;
			case 'p':
				return Reg_bp;
			case 'x':
				return Reg_bx;
			}
			break;
		case 'c':
			if (len == 3)
			{
				if (p[1] != 'r')
					break;
				switch (p[2])
				{
				case '0':
					return Reg_cr0;
				case '2':
					return Reg_cr2;
				case '3':
					return Reg_cr3;
				case '4':
					return Reg_cr4;
				}
			}
			else if (len == 2)
			{
				switch (p[1])
				{
				case 'h':
					return Reg_ch;
				case 'l':
					return Reg_cl;
				case 's':
					return Reg_cs;
				case 'x':
					return Reg_cx;
				}
			}
			break;
		case 'd':
			if (len == 2)
			{
				switch (p[1])
				{
				case 'h':
					return Reg_dh;
				case 'i':
					return Reg_di;
				case 'l':
					return Reg_dl;
				case 's':
					return Reg_ds;
				case 'x':
					return Reg_dx;
				}
			}
			else if (len == 3)
			{
				switch (p[1])
				{
				case 'b':	/* %db[012367] synonym */
				case 'r':
					switch (p[2])
					{
					case '0':
						return Reg_dr0;
					case '1':
						return Reg_dr1;
					case '2':
						return Reg_dr2;
					case '3':
						return Reg_dr3;
					case '6':
						return Reg_dr6;
					case '7':
						return Reg_dr7;
					}
				}
			}
			break;
		case 'e':
			if (len == 3)
			{
				switch (p[1])
				{
				case 'a':
					if (p[2] == 'x')
						return Reg_eax;
					break;
				case 'b':
					if (p[2] == 'x')
						return Reg_ebx;
					else if (p[2] == 'p')
						return Reg_ebp;
					break;
				case 'c':
					if (p[2] == 'x')
						return Reg_ecx;
					break;
				case 'd':
					if (p[2] == 'x')
						return Reg_edx;
					else if (p[2] == 'i')
						return Reg_edi;
					break;
				case 's':
					if (p[2] == 'p')
						return Reg_esp;
					else if (p[2] == 'i')
						return Reg_esi;
					break;
				}
			}
			else if (len == 2)
			{
				if (p[1] == 's')
					return Reg_es;
			}
			break;
		case 'f':
			if (p[1] == 's' && len == 2)
				return Reg_fs;
			break;
		case 'g':
			if (p[1] == 's' && len == 2)
				return Reg_gs;
			break;
		case 's':
			if (len != 2) break;
			switch (p[1])
			{
			case 'i':
				return Reg_si;
			case 'p':
				return Reg_sp;
			case 's':
				return Reg_ss;
			case 't':
				return Reg_st;	/* rest in operparse() */
			}
			break;
		case 't':
			if (len == 3)
			{
				if (p[1] != 'r')
					break;
				switch (p[2])
				{
				case '3':
					return Reg_tr3;
				case '4':
					return Reg_tr4;
				case '5':
					return Reg_tr5;
				case '6':
					return Reg_tr6;
				case '7':
					return Reg_tr7;
				}
			}
			break;
		}
	}
	error(gettxt(":948","invalid register: %%%s"), prtstr(p, len));
	return Reg_eax;
}

static int bump_lineno;	/* marks pending beginning of new input line */

static int
#ifdef __STDC__
yylex(void)	/* return next input token */
#else
yylex()
#endif
{
	register Uchar *cur = inbuf + current++;

	for (;;) /* only loop for white space, new-line, and garbage chars */
	{
		/*
		* Handle all single character tokens.
		*/
		switch (*cur)
		{
		case '\0':
			/*
			* Either a garbage character or one of the
			* special marks left in the buffer for EOF
			* and just handled new-line.
			*/
			if (bump_lineno) /* first token after new-line */
			{
				/*
				* Safe to toss the previous line's
				* characters.
				*/
				bump_lineno = 0;
				if (newline())
					return TOKEN_EOF;
				cur = inbuf + current++;
				continue;
			}
			else if (current >= maxcur) /* EOF w/out new-line */
				return TOKEN_EOF;
			/*FALLTHROUGH*/
		default:
		invalid:;
			error(gettxt(":949","invalid input character: %s"),
				prtstr(cur, (size_t)1));
			current++;
			cur++;
			continue;
		case '\\':		/* escaped operator or new-line */
			current++;
			switch (*++cur)
			{
			default:
				error(gettxt(":950","invalid \\ operator: \\%s"),
					prtstr(cur, (size_t)1));
				continue;
			case '\n':	/* \new-line taken as white space */
				break;
			case '*':
				return ExpOp_Multiply;
			case '/':
				return ExpOp_Divide;
			case '%':
				return ExpOp_Remainder;
			case '$':
				goto identifier;
			}
			/*
			* Grow the buffer if we're at the end or if there
			* are no more new-lines to be found.
			*/
			cur++;
			if (++current >= maxcur || memchr((void *)cur,
				'\n', maxcur - current) == 0)
			{
				(void)fillbuf();
			}
			curlineno++;
			continue;
		case ' ':
		case '\t':
			current++;	/* white space */
			cur++;
			continue;
		case '>':
			if (*++cur != '>')
				error(gettxt(":951","invalid right shift operator token"));
			else
				current++;
			return ExpOp_RightShift;
		case '<':
			current++;
			if (*++cur == '<')
				return ExpOp_LeftShift;
			switch (*cur)
			{
			default:
				error(gettxt(":952","invalid operator token: <%s>"),
					prtstr(cur, (size_t)1));
				continue;
			case 's':
				if (*++cur != '>')
					error(gettxt(":953","missing > for <s> operator"));
				else
					current++;
				return ExpOp_SegmentPart;
			case 'o':
				if (*++cur != '>')
					error(gettxt(":954","missing > for <o> operator"));
				else
					current++;
				return ExpOp_OffsetPart;
			}
		case '~':
			return ExpOp_Complement;
		case '!':
			return ExpOp_Nand;
		case '&':
			return ExpOp_And;
		case '|':
			return ExpOp_Or;
		case '^':
			return ExpOp_Xor;
		case '+':
			return ExpOp_Add;
		case '-':
			return ExpOp_Subtract;
		case '(':
			return TOKEN_LPAREN;
		case ')':
			return TOKEN_RPAREN;
		case '*':
			return TOKEN_INDIRECT;
		case ',':
			return TOKEN_COMMA;
		case ':':
			return TOKEN_COLON;
		case '=':
			return TOKEN_EQUAL;
		case '$':
			return TOKEN_DOLLAR;
		case '[':
			return TOKEN_LBRACKET;
		case ']':
			return TOKEN_RBRACKET;
		case '#':
		case '/':
			if ((cur = comment()) == 0)	/* at EOF */
				return TOKEN_EOS;
			goto eostmt;
		case '\n':
			current--;
		eostmt:;
			*cur = '\0';	/* see case '\0' above */
			bump_lineno = 1;
			/*FALLTHROUGH*/
		case ';':
			return TOKEN_EOS;
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z': case '.':
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z': case '_':
			/*
			* Inline version of name() since we know
			* that this must be a name.
			*/
identifier:
			while (chtab[*++cur] & (CH_LET | CH_DIG))
				;
			token_beg = current - 1;
			current = cur - inbuf;
			token_len = current - token_beg;
			return ExpOp_LeafName;
		case '%':
			if (!name())
			{
				error(gettxt(":955","invalid register token"));
				current++;
				cur++;
				continue;
			}
			regno = lookupreg();
			return ExpOp_LeafRegister;
		case '@':
			return attoken();
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (numtoken())
				return ExpOp_LeafFloat;
			return ExpOp_LeafInteger;
		case '"':
			strtoken();
			return ExpOp_LeafString;
		}
	}
}

void
#ifdef __STDC__
setinput(FILE *stream)	/* initialize parsing for stream */
#else
setinput(stream)FILE *stream;
#endif
{
	instream = stream;
	bump_lineno = 0;
	current = 0;
	maxcur = 0;
	(void)newline();
}
