#ident	"@(#)debugger:libutil/common/dump_raw.C	1.13"

#include "Interface.h"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Parser.h"
#include "Proglist.h"
#include "Symbol.h"
#include "Expr.h"
#include "Itype.h"
#include "Rvalue.h"
#include "global.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#ifdef MULTIBYTE
#include <wctype.h>
#else
#include <ctype.h>
#endif

// dump file contents in hexadecimal and ASCII

#define LSZB	16		// bytes per line 
#define LSZW	(LSZB/sizeof(int)) // words per line
#define NCPW	(sizeof(int)*2)	   // chars per word printed as hex 
#define NCPB	(sizeof(char)*2)   // chars per byte printed as hex 

// determine target byte order
#define UNK 0
#define LSB 1
#define MSB 2

static int	byte_order;

enum
{
	W_L0, W_L1, W_L2, W_L3
};

enum
{
	W_M3, W_M2, W_M1, W_M0,
	W_sizeof
};

static int
get_order()
{
	union {
		unsigned long	w;
		unsigned char	c[W_sizeof];
	} u;
	u.w = 0x10203;
	if  ((((((((u.c)[W_L3]<<8)
		+(u.c)[W_L2])<<8)
		+(u.c)[W_L1])<<8)
		+(u.c)[W_L0]) == 0x10203)
		return LSB;
	else if ((((((((u.c)[W_M3]<<8)
		+(u.c)[W_M2])<<8)
		+(u.c)[W_M1])<<8)
		+(u.c)[W_M0]) == 0x10203)
		return MSB;
	else
		return UNK;
}

// put out one byte in hex in form xx, where each x
// is a hex digit
static char *
out_byte(char *cur, unsigned int byte)
{
	unsigned char	c;
	c = (byte >> 4) & 0xf;
	if (c < 0xa)
		*cur++ = '0' + c;
	else
		*cur++ = 'a' + (c - 10);
	c = byte & 0xf;
	if (c < 0xa)
		*cur++ = '0' + c;
	else
		*cur++ = 'a' + (c - 10);
	return cur;
}

static char *
out_word(char *cur, unsigned int word)
{
	union {
		unsigned int	w;
		unsigned char	c[sizeof(unsigned int)];
	} u;
	int	i;
	u.w = word;
	if (byte_order == LSB)
	{
		for(i = sizeof(unsigned int)-1; i >= 0; i--)
			cur = out_byte(cur, (unsigned int)u.c[i]);
	}
	else
	{
		for(i = 0; i < sizeof(unsigned int); i++)
			cur = out_byte(cur, (unsigned int)u.c[i]);
	}
	return cur;
}

// print line organized as words
static char *
word_line(unsigned char *cur_line, char *cur, int bcount, int diff)
{
	// # of words for this line
	int 		wcount = (bcount + sizeof(int)-1)/sizeof(int);
	unsigned int	*word_ptr = (unsigned int *)cur_line;
	int		n;

	if (diff)
	{
		// first line of output
		// if we do not start on a 16-byte
		// boundary, print ".", until the
		// starting point
		int	jfirst = 1;
		for (n = 0; n < diff; n++ )
		{
			if (!(n % sizeof(int)))
			{
				// word boundary
				*cur++ = ' ';
				if ((diff - n) >= sizeof(int))
				{
					// allow for
					// 0x prefix
					*cur++ = ' ';
					*cur++ = ' ';
				}
			}
			*cur++ = '.';
			*cur++ = '.';
		}
		int j = n % sizeof(int);
		// may be starting in middle of a word
		// if so, must pass printf the
		// correct address for current byte
		// order - low order byte if (little
		// endian), else high order byte
		if (j)
		{
			int	h = sizeof(int) - j;
			int	k;
			if (byte_order == LSB)
				// least sig byte
				k = n + h - 1;
			else 
				k = n;
			for (; j < sizeof(int); j++)
			{
				if (jfirst)
				{
					// only print
					// 0x prefix
					// for the 1st
					// byte
					jfirst = 0;
					*cur++ = '0';
					*cur++ = 'x';
				}
				cur = out_byte(cur, *(cur_line + k));
				if (byte_order == LSB)
				{
					k--;
				}
				else
				{
					k++;
				}
			}
			n += h;
		}
		n = n / sizeof(int);
	}
	else
	{
		n = 0;
	}
	for (; n < wcount; ++n)
	{
		*cur++ = ' ';
		*cur++ = '0';
		*cur++ = 'x';
		cur = out_word(cur, *(word_ptr + n));
	}
	for (; n < LSZW; ++n)
	{
		for(int s = 0; s < NCPW+3; s++)
			*cur++ = ' ';
	}
	return cur;
}

// print line organized as bytes
static char *
byte_line(unsigned char *cur_line, char *cur, int bcount, int diff)
{
	int	n;

	for(n = 0; n < diff; n++)
	{
		// first line of output
		// if we do not start on a 16-byte
		// boundary, print ".", until the
		// starting point
		*cur++ = ' ';
		*cur++ = '.';
		*cur++ = '.';
	}
	cur_line += n;
	for(; n < bcount; n++, cur_line++)
	{
		*cur++ = ' ';
		cur = out_byte(cur, *cur_line);
	}
	for(; n < LSZB; n++)
	{
		*cur++ = ' ';
		*cur++ = ' ';
		*cur++ = ' ';
	}
	return cur;
}

int
dump_raw(Proclist *procl, char *expr_str, int cnt, char *cnt_var, int byte)
{
	unsigned char	*save;
	unsigned char	*cur_line, *next_line;
	unsigned char	*cur_byte;
	int		bcount;
	int		count, diff;
	Iaddr		addr, oaddr;
	int 		single = 1;
	int		ret = 1;
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head = 0;
	char		*cur;
#ifdef MULTIBYTE
	wchar_t		wchar;
#endif
	char		buf[256];	// big enough


	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	if (byte_order == UNK)
	{
		if ((byte_order = get_order()) == UNK)
		{
			printe(ERR_byte_order, E_ERROR);
			if (list_head)
				proglist.free_plist(list_head);
			return 0;
		}
	}
	// possibly bigger than needed, but okay for 
	// max unaligned address
	save = new(unsigned char[cnt + 16]);

	sigrelse(SIGINT);
	do
	{
		Symbol	func;
		Process	*proc = pobj->process();

		if (prismember(&interrupt, SIGINT))
			break;
		if (!pobj->state_check(E_RUNNING|E_DEAD) ||
			!proc->stop_all())
		{
			ret = 0;
			continue;
		}
		if (cnt_var)
		{
			if (!parse_num_var(pobj, pobj->curframe(),
				cnt_var, count))
			{
				proc->restart_all();
				ret = 0;
				continue;
			}
		}
		else
			count = cnt;
		printm(MSG_raw_dump_header, pobj->obj_name(),
			pobj->prog_name());

		Expr	*expr = new_expr(expr_str, pobj);
		Rvalue	*rval;
		Itype	itype;
		Stype	stype;
		if (!expr->eval(pobj, ~0, 0) ||
			!expr->rvalue(rval))
		{
			delete expr;
			printe(ERR_eval_fail_expr, E_ERROR, expr_str);
			proc->restart_all();
			ret = 0;
			continue;
		}
		stype = rval->get_Itype(itype);
		switch(stype)
		{
		case Schar:	
			oaddr = (Iaddr)itype.ichar;
			break;
		case Sint1:	
			oaddr = (Iaddr)itype.iint1;
			break;
		case Sint2:	
			oaddr = (Iaddr)itype.iint2;
			break;
		case Sint4:	
			oaddr = (Iaddr)itype.iint4;
			break;
		case Suchar:	
			oaddr = (Iaddr)itype.iuchar;
			break;
		case Suint1:	
			oaddr = (Iaddr)itype.iuint1;
			break;
		case Suint2:	
			oaddr = (Iaddr)itype.iuint2;
			break;
		case Suint4:	
			oaddr = (Iaddr)itype.iuint4;
			break;
		case Saddr:	
			oaddr = itype.iaddr;
			break;
		case Sdebugaddr:	
			oaddr = (Iaddr)itype.idebugaddr;
			break;
		case Sbase:	
			oaddr = (Iaddr)itype.ibase;
			break;
		case Soffset:	
			oaddr = (Iaddr)itype.ioffset;
			break;
		case Ssfloat:	
		case Sdfloat:	
		case Sxfloat:	
		case SINVALID:
		default:
			delete expr;
			printe(ERR_expr_not_integral, E_ERROR, expr_str);
			proc->restart_all();
			ret = 0;
			continue;
		}
		delete expr;

		// truncate addr to 16 byte boundary
		addr = oaddr >> 4;
		addr <<= 4;
		diff = (int)(oaddr - addr);
		cur_line = save;
		for (int i = 0; i < diff; i++)
			*cur_line++ = '\0';
		if ( pobj->read(oaddr, count, (char *)cur_line) == 0)
		{
			printe(ERR_proc_read, E_ERROR, pobj->obj_name(),
				oaddr);
			proc->restart_all();
			ret = 0;
			continue;
		}
		count += diff;
		next_line = save;

		while (count > 0) 
		{
			if (prismember(&interrupt, SIGINT))
				break;
			cur_line = next_line;
			cur = buf;
			// # of bytes for this line
			if (count < LSZB)
				bcount = count;
			else
				bcount = LSZB;

			next_line += bcount;
			count -= bcount;
			cur += sprintf(cur, "%#8lx:", 
				diff ? oaddr : addr);
			addr += bcount;
			if (byte)
				cur = byte_line(cur_line, cur, bcount, 
					 diff);
			else
				cur = word_line(cur_line, cur, bcount, 
					diff);
			diff = 0;
			*cur++ = ' ';
			*cur++ = ' ';
			cur_byte = cur_line;
#ifdef MULTIBYTE
			// handle multibyte characters:
			while(cur_byte < next_line)
			{
				int	clen;
				clen = mbtowc(&wchar, 
					(const char *)cur_byte, 
					MB_CUR_MAX);
				if (clen <= 0)
				{
					// NULL or not multibyte char
					*cur++ = '.';
					cur_byte++;
				}
				else if (iswprint(wchar) == 0)
				{
					// non printing multibyte
					cur_byte += clen;
					for(; clen; clen--)
					{
						*cur++ = '.';
					}
				}
				else
				{
					// printable multibyte
					for(; clen; clen--)
						*cur++ = *cur_byte++;
				}
			}
#else
			// print out ASCII
			for (n = 0; n < bcount; ++n, ++cur_byte)
				if (isprint(*cur_byte))
					*cur++ = *cur_byte;
				else
					*cur++ = '.';
#endif
			*cur = 0;
			printm(MSG_raw_dump, buf);
		}
		if (!single)
			printm(MSG_newline);
		if (!proc->restart_all())
			ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
	if (list_head)
		proglist.free_plist(list_head);
	delete save;
	return ret;
}
