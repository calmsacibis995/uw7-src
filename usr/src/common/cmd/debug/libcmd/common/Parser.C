/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libcmd/common/Parser.C	1.46"

#include "Parser.h"
#include "Input.h"
#include "Interface.h"
#include "Location.h"
#include "utility.h"
#include "str.h"
#include "ProcObj.h"
#include "Machine.h"
#include "Event.h"
#include "Buffer.h"
#include "global.h"
#include "Link.h"
#include "Dbgvarsupp.h"
#include "Scanner.h"
#include "Keyword.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

// for DASH options
#define ERRORBIT 0x80000000
#define DUPBIT 0x40000000	

// local error codes
#define err_dashb_dashf	10000
#define err_dashn_dashr	10001
#define err_dashq_dashv	10002

static Location * parse_loc( register Token *);

void
check_collisions( const char *name, int classes )
{
	if ( classes & ALIASES ) {
		Token *p = find_alias( name );
		if ( p )
			printe(ERR_alias_name, E_WARNING, name);
	}
	if ( classes & COMMANDS ) {
		Keyword *p = keyword( name );
		if ( p )
			printe(ERR_cmd_name, E_WARNING, name);
	}
	if ( classes & MACROS ) {
		// MORE
	}
}

dvars vartab[] = {
	"%db_lang",	Db_lang_v,
	"%dis_mode",	Dis_mode_v,
#if EXCEPTION_HANDLING
	"%eh_object",	EH_object_v,
#endif
	"%file",	File_v,
	"%follow",	Follow_v,
	"%frame",	Frame_v,
	"%frame_numbers",Frame_num_v,
	"%func",	Func_v,
	"%global_path",	Glob_path_v,
	"%lang",	Lang_v,
	"%lastevent",	Lastevent_v,
	"%line",	Line_v,
	"%list_line",	List_line_v,
	"%list_file",	List_file_v,
	"%loc",		Loc_v,
	"%mode",	Mode_v,
	"%num_bytes",	Num_bytes_v,
	"%num_lines",	Num_lines_v,
	"%path",	Path_v,
	"%proc",	Proc_v,
	"%program",	Program_v,
	"%prompt",	Prompt_v,
	"%redir",	Redir_v,
	"%result",	Result_v,
	"%stack_bounds",	Stack_bounds_v,
	"%thread",	Thread_v,
	"%thread_change",Thread_change_v,
	"%thisevent",	Thisevent_v,
	"%verbose",	Verbose_v,
	"%wait",	Wait_v,
	0,		NoType_v,
};

dbg_vtype dbg_var( const char *word )
{
	static int init_done = 0;
	register dvars *p = vartab;
	register char *s = strlook(word);

	if ( !init_done ) {
		// initialize str() table
		for ( ; p->varname ; p++ )
			p->varname = str(p->varname);
		init_done = 1;
		p = vartab;
	}

	if (!s)
		return NoType_v;

	for ( ; p->varname ; p++ ) {
		if ( p->varname == s ) // since keywords are in str() table
			return p->vartype;
	}

	return NoType_v;
}

Ptrlist::Ptrlist( register int cnt )
{
	ptr = new void *[cnt+1];
	count = cnt;
	ptr[cnt] = 0;	// guarantee null term list
}

Ptrlist::~Ptrlist()
{
	delete ptr;
}

void *Ptrlist::operator[]( register int n )
{
	if ( n < 0 || n >= count )
		return 0;
	return ptr[n];
}

void Ptrlist::concat( register const Ptrlist *other )
{
	register void **nptr = (void **) new char *[count + other->count + 1];
	memcpy(nptr, ptr, count*sizeof (void *));
	memcpy(nptr+count, other->ptr, (other->count+1)*sizeof (void *));
	delete ptr;
	ptr = nptr;
	count += other->count;
}

static Buffer parse_buf;

void
Ptrlist::print(Buffer *buf, char sep)
{
	buf->clear();

	if ( !this )
		return;

	register int n = 0;

	for ( ; n < count ; n++ ) {
		if ( buf->size() ) buf->add(sep);
		buf->add((char *)ptr[n]);	// assumes pointers are char *
	}
	return;
}

void
Proclist::print(Buffer *buf, char sep)
{
	Ptrlist::print(buf, sep);
}

void
IntList::add(IntItem *ptr)
{
	if (!tail)
		head = ptr;
	else
		tail->next = ptr;
	tail = ptr;
}

IntList::~IntList()
{
	IntItem	*ptr;
	while(head)
	{
		ptr = head;
		head = head->next;
		delete ptr;
	}
}

void
IntList::print(Buffer *buf, char sep)
{
	buf->clear();
	IntItem	*ptr;

	for (ptr = head; ptr; ptr = ptr->next)
	{
		if ( buf->size() ) buf->add(sep);
		if (ptr->type == it_string)
			buf->add(ptr->val.cp);
		else
		{
			char	tbuf[MAX_INT_DIGITS + 1];
			sprintf(tbuf, "%d", ptr->val.i);
			buf->add(tbuf);
		}
	}
}

Node::Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2,
	OpType ot3, void *op3, OpType ot4, void *op4) 	       // (can't inline)
{
	clear();
	op = o;
	setopnd(1, ot1, op1);
	setopnd(2, ot2, op2);
	setopnd(3, ot3, op3);
	setopnd(4, ot4, op4);
}

Node::Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2,
	OpType ot3, void *op3,
	OpType ot4, void *op4, OpType ot5, void *op5) 	       // (can't inline)
{
	clear();
	op = o;
	setopnd(1, ot1, op1);
	setopnd(2, ot2, op2);
	setopnd(3, ot3, op3);
	setopnd(4, ot4, op4);
	setopnd(5, ot5, op5);
}

#define deleteopnd(n, which)	\
	if ( optype[n] == NODE && which.np->op != SAVE )\
		delete which.np;\
	else if ( optype[n] == PROCLIST )\
		delete which.procs;\
	else if ( optype[n] == INTLIST )\
		delete which.ilist;\
	else if ( optype[n] == LOC )\
		delete which.loc;\
	else if ( optype[n] == EXPLIST )\
		delete which.exp;\
	else if ( optype[n] == TOKEN )\
		delete which.tl;

Node::~Node()
{
	if ( this ) {
		if (first.np)
			deleteopnd(0, first);
		if (second.np)
			deleteopnd(1, second);
		if (third.np)
			deleteopnd(2, third);
		if (fourth.np)
			deleteopnd(3, fourth);
		if (fifth.np)
			deleteopnd(4, fifth);
	}
}

#if DEBUG
#define printopnd(n, which)	\
	if ( optype[n] == NONE )\
		sprintf(buf, "%s %10s", buf, "--");\
	else if ( optype[n] == INT )\
		sprintf(buf, "%s %#10x", buf, which.i);\
	else if ( which.i == 0 )\
		sprintf(buf, "%s %10s", buf, "(nil)");\
	else\
		sprintf(buf, "%s ->%08x", buf, which.i)

char *Node::print()
{
	static char	buf[128];
	register char	*name;
	Keyword		*k;

	if ( !this )
		return "(Node *) 0";

	switch ( op ) {
	case REDIR:		name = "REDIR";		break;
	case CMDLIST:		name = "CMDLIST";	break;
	case SHELL:		name = "SHELL";		break;
	case SAVE:		name = "SAVE";		break;
	case NoOp:		name = "NoOp";		break;
	default:
		name = (char *)keyword(op)->str;
		if ((k = keyword(op)) != 0)
			name = (char *)k->str;
		else
		{
			sprintf(buf, "<%d>", op);
			name = buf;
		}
		break;
	}

	sprintf(buf, "%-10s ", name);

	printopnd(0, first);
	printopnd(1, second);
	printopnd(2, third);
	printopnd(3, fourth);
	printopnd(4, fifth);

	return buf;
}

void Node::dump()
{
	if ( !this ) {
		printf("(Node *) 0\n");
	} else {
		char *p = print();
		printf("->%08x:	%s\n", this, p);

		dumpopnd(0);
		dumpopnd(1);
		dumpopnd(2);
		dumpopnd(3);
		dumpopnd(4);
	}
}

void Node::dumpopnd( register int i )
{
	register OpType ot = (enum OpType)optype[i];
	register Opnd *opnd = &first + i;
	Buffer	*buf = buf_pool.get();

	buf->clear();

	if ( ot == CHARSTAR || ot == STRING_OPND || ot == INT_VAR )
		printf("->%08x: %s\n", opnd->i, fmtstring(opnd->cp));
	else if ( ot == PROCLIST )
	{
		opnd->procs->print(buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == INTLIST )
	{
		opnd->ilist->print(buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == LOC )
	{
		opnd->loc->print(buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == EXPLIST )
	{
		opnd->exp->print(buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == FILELIST )
	{
		opnd->files->print(buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == TOKEN )
	{
		print_tok_list(opnd->tl, buf);
		printf("->%08x: %s\n", opnd->i, (char *)*buf);
	}
	else if ( ot == NODE && opnd->np )
	{
		buf_pool.put(buf);
		opnd->np->dump();
		return;
	}
	buf_pool.put(buf);
}
#endif

static Scanner scanner;
static Token *nexttok;

static inline int
isredir( TokType op )
{
	return ( op == GREATER || op == BAR );
}

static inline int
isvarname( TokType op )
{
	return ( op == DEBUG_NAME || op == USER_NAME );
}

static Token *
skipWS( register Token *t)
{
	while ( t && t->op() == WHITESPACE )
		t = t->next();

	return (Token*)t;
}

static char *
get_token( register Token *t )
{
	t = skipWS(t);
	parse_buf.clear();

	while ( t && t->op() != WHITESPACE &&
		     t->op() != NL &&
		     t->op() != SEMI &&
		     t->op() != LCURLY &&
		     t->op() != RCURLY &&
		     !isredir(t->op()) ) {
		parse_buf.add(t->print());
		t = t->next();
	}

	nexttok = (Token*)t;

	return (parse_buf.size() > 1) ? (char *)parse_buf : 0;
}


static char *
parse_opts( Opts *mask, const char *optstring, Token *t, int *arg_num )
{
	static char bad[2];
	Opts nmask;
	char *p;

	nexttok = t;
	*arg_num = 0;

	t = t->next();
	if (!t || t->op() != NAME || !(p = t->str()))
	{
		nexttok = t;
		*mask |= ERRORBIT;
		return 0;
	}

	while (*p) 
	{
		char *q;
		if ( (q = strchr(optstring, *p)) != 0 ) 
		{
			nmask = DASH(*p++);
			if ( (nmask & *mask) )
			{
				*mask |= ERRORBIT|DUPBIT;
				nexttok = t;
				bad[0] = *(p-1);
				return bad;
			}
			else
			{
				*mask |= nmask;
			}

			if ( *++q == ':' )
			{
				if ( !*p )
				{
					t = t->next();
				}
				else
				{
					char *p2 = p;
					if (isdigit(*p2))
					{
						p2++;
						if (*p2 == 'x' || *p2 == 'X')
							p2++;
						while (isxdigit(*p2))
							p2++;
					}
					// if operand is a number, convert
					// token from name to number
					if (!*p2)
						t->makenum(p);
					else
						t->set_str(p);
				}
				*arg_num = *++q - '0';
				nexttok = t;
				return 0;
			}
		} 
		else 
		{
			*mask |= ERRORBIT;	// not a valid option
			nexttok = t;
			bad[0] = *p;
			return bad;
		}
	}
	nexttok = t->next();
	return 0;
}

static char *
get_expr( register Token *t, int break_at_paren )
{
	// if break_at_paren is set, we break out of expression
	// parsing at the outermost right paren/curly/bracket,
	// otherwise we keep going

	int bnest = 0;
	int pnest = 0;
	int cnest = 0;
	int anest = 0;
	register int bracket = 0;

	if (!t)
		return 0;

	parse_buf.clear();
	if ( t->op() == LPAREN || t->op() == LBRACK || t->op() == LCURLY ) {
		bracket = t->op();
		switch ( bracket ) {
		case LPAREN:
			++pnest;
			break;
		case LBRACK:
			++bnest;
			break;
		case LCURLY:
			++cnest;
			break;
		}
		DPRINT(DBG_PARSER, ("get_expr() got left bracket == %s\n", t->dump()))
		parse_buf.add(t->print());
		t = t->next();
	}
	while ( t ) {
		switch ( t->op() ) {
		case AMPERSAND:
			if (t->next() && t->next()->op() == t->op())
			{
				if ( !bnest && !pnest && !cnest && !anest )
					goto out;
			}
			break;
		case BAR:
		case NL:
		case SEMI:
		case POUND:
			if ( !bnest && !pnest && !cnest ) {
				goto out;
			}
			break;
		case COMMA:
			if ( !bnest && !pnest && !cnest && !anest ) {
				t = t->next();
				goto out;
			}
			break;
		case LPAREN:
			++pnest;
			break;
		case RPAREN:
			if (!pnest && !bnest && !cnest)
				goto out;
			--pnest;
			if ( !pnest && bracket == LPAREN && break_at_paren)
			{
				parse_buf.add(t->print());
				t = t->next();
				goto out;
			}
			break;
		case LBRACK:
			++bnest;
			break;
		case RBRACK:
			if (!bnest && !pnest && !cnest)
				goto out;
			--bnest;
			if ( !bnest && bracket == LBRACK && break_at_paren)
			{
				parse_buf.add(t->print());
				t = t->next();
				goto out;
			}
			break;
		case LCURLY:
			// left curlies delimit command blocks 
			if (!pnest && !bnest && !cnest)
				goto out;
			++cnest;
			break;
		case RCURLY:
			if (!cnest && !pnest && !bnest)
				goto out;
			--cnest;
			if ( !cnest && bracket == LCURLY && break_at_paren)
			{
				parse_buf.add(t->print());
				t = t->next();
				goto out;
			}
			break;
		case LESS:
			++anest;
			break;
		case GREATER:
			if (!pnest && !bnest && !cnest && !anest)
				goto out;
			--anest;
			break;
		}
		parse_buf.add(t->print());
		DPRINT(DBG_PARSER, ("added %s\n", t->dump()))
		t = t->next();
	}
out:	nexttok = t;
	DPRINT(DBG_PARSER,("nexttok == %s\n", t->dump()))
	if ( bnest ) {
		printe(ERR_unmatch_brack, E_ERROR);
		return 0;
	}

	if ( pnest ) {
		printe(ERR_unmatch_paren, E_ERROR);
		return 0;
	}

	if ( cnest ) {
		printe(ERR_unmatch_brace, E_ERROR);
		return 0;
	}
	if (parse_buf.size() <= 1)
		return 0;

	// strip trailing whitespace
	int	i = parse_buf.size() - 2;
	char	*q = &(((char *)parse_buf)[i]);
	for(; i >= 0; i--, q--)
	{
		if (!isspace(*q))
			break;
	}
	*(q + 1) = 0;
	return str((char *)parse_buf);
}

static char *
get_shell_cmd( register Token *t )
{
	int	nest = 0;

	// shell cmds are delimited by newline, #, or semi-colon,
	// except where the # or semi-colon is escaped
	// an unmatched right curly will also delimit the cmd
	// (assume we are within a cmd block)
	parse_buf.clear();
	while ( t && t->op() != NL ) 
	{
		if (t->op() == BACKSLASH)
		{
			if (t->next())
			{
				switch(t->next()->op())
				{
					case SEMI:
					case POUND:
					case LCURLY:
					case RCURLY:
						t = t->next();
						break;
					default:
						break;
				}
			}
		}
		else if (t->op() == SEMI || t->op() == POUND)
			break;
		else if (t->op() == LCURLY)
			nest++;
		else if (t->op() == RCURLY)
		{
			if (!nest)
				break;
			else
				nest--;
		}
		// allow bad numbers
		if (t->op() >= BADTOKEN)
			return 0;
		parse_buf.add(t->print());
		t = t->next();
	}

	nexttok = t;

	return str((char *)parse_buf);
}

static char *
get_filename( register Token *t )
{
	parse_buf.clear();
	while ( t && t->op() != WHITESPACE && 
		     t->op() != NL &&
		     t->op() != SEMI &&
		     t->op() != POUND &&
		     t->op() != COLON &&
		     t->op() != COMMA &&
		     t->op() != LCURLY &&
		     t->op() != RCURLY ) 
	{
		// allow bad numbers
		if (t->op() >= BADTOKEN)
			return 0;
		parse_buf.add((t->op() == STRING) ? t->raw() : t->print());
		t = t->next();
	}

	nexttok = t;

	return (parse_buf.size() > 1 ? str((char *)parse_buf) : 0);
}

static Proclist *
get_proclist( register char *s )
{
	Proclist *p = 0;
	int count = 0;
	char *save[100];

	save[count] = s;

	while ( s && *s ) {
		switch( *s ) {
		case ',':		// COMMA
			save[count] = strn(save[count], s-save[count]);
			save[++count] = ++s;
			break;
		case '\n':		// NL
		case ' ':
		case '\t':		// WHITESPACE
		case ';':		// SEMI
		case '{':		// LCURLY
		case '}':		// RCURLY
			printe(ERR_bad_plist, E_ERROR);
			return 0;
		default:
			++s;
			break;
		}
	}

	if ( s > save[count] ) {
		save[count] = strn(save[count], s-save[count]);
		count++;
	}

	if ( count ) {
		// MORE: expand user variables here?
		p = new Proclist(count);
		for ( register int i = 0 ; i < count ; i++ )
			p->ptr[i] = save[i];
	}

	return p;
}

static char *
get_regexp( register Token *t )
{
	int	 redir = 0;
	
	if (t->op() == QUESTION)
		redir = 1;
	
	t = t->next();
	parse_buf.clear();
	while ( t && t->op() != NL )
	{
		if ((t->op() == SLASH && !redir) ||
			(t->op() == QUESTION && redir))
		{
			t = t->next();
			break;
		}
		parse_buf.add(t->print());
		t = t->next();
	}

	nexttok = t;
	return str((char *)parse_buf);
}

extern const char *sigtable[];

const char *
signame( int signo )
{
	if (signo < 1 || signo > NUMBER_OF_SIGS)
		return 0;
	else
		return sigtable[signo - 1];
}
	
static int
getsig( register char *s )
{
	static int init_done = 0;

	if ( !init_done ) {
		for ( register const char **p = sigtable ; *p ; p++ )
			*p = str(*p);
		init_done = 1;
	}

	char buf[60];

	s = strncpy(buf, s, 59);
	buf[59] = 0;

	for ( register char *p = s ; *p ; p++ )
		if ( (*p >= 'A') && (*p <= 'Z') )
			*p = *p - 'A' + 'a';		// make lower case

	if ( s[0] == 's' && s[1] == 'i' && s[2] == 'g' )
		s += 3;		// skip "sig" prefix

	s = strlook(s);		// so can compare ptrs

	register const char **q;
	for ( q = sigtable ; *q ; q++ )
		if ( s == *q )
			break;

	if ( *q )
		return (1 + (q-sigtable));
	else
		return 0;
}

static StopEvent *
parse_event(Token *t)
{
	StopEvent	*current = 0, *p = 0;
	char		*expr;
	Location	*loc;
	int		op;

	if (!t)
		return 0;
	op = E_LEAF;
	while(t)
	{
		switch(t->op())
		{
		case STAR:	
			t = t->next();
			expr = get_expr(t, 0);
			if (!expr)
			{
				dispose_event(current);
				return 0;
			}
			current = new StopExpr(op|E_TRIG_ON_CHANGE, 
					expr);
			break;
		case LCURLY:
		case LPAREN:
		case LBRACK:
			expr = get_expr(t, 1);
			if (!expr)
			{
				dispose_event(current);
				return 0;
			}
			current = new StopExpr(op, expr);
			break;
		default:
			loc = parse_loc(t);
			if (!loc)
			{
				dispose_event(current);
				return 0;
			}
			current = new StopLoc(op, loc);
			break;
		}
		current->append(p);
		t = skipWS(nexttok);
		if (!t)
			break;
		if ((t->op() != BAR) && (t->op() != AMPERSAND))
			break;
		if (t->op() != t->next()->op())
			break;
		op = (t->op() == BAR ? E_OR : E_AND);
		p = current;
		t = skipWS(t->next()->next());
	}
	nexttok = t;
	return current;
}

// parse generic list of ints, names, debug vars - used
// for signals and system calls
// names can be separated by commas
static IntItem *
parse_intlist(register Token *t, intlist_type iltype)
{
	IntItem	*p = 0;
	int	val;
	if (isvarname(t->op()))
	{
		p = new IntItem(t->str());
	}
	else if (t->op() == NAME)
	{
		switch(iltype)
		{
		case il_events:
			return 0;
		case il_signals:
			if ((val = getsig(t->str())) == 0)
				return 0;
			break;
		case il_syscalls:
			if ((val = getsys(t->str())) == 0)
				return 0;
			break;
		}
		p = new IntItem(val);
	}
	else if (t->op() == DEC)
	{
		switch(iltype)
		{
		case il_events:
			break;
		case il_signals:
			if (t->val() < 1 || t->val() > NUMBER_OF_SIGS)
				return 0;
			break;
		case il_syscalls:
			if (t->val() < 1 || t->val() > (int)lastone)
				return 0;
			break;
		}
		p = new IntItem(t->val());
	}
	t = t->next();
	if ( p && t && t->op() == COMMA )
		t = t->next();

	nexttok = (Token*)t;
	return p;
}

// is a debugger or user var, evaluate, else just return string
char *
parse_str_var(ProcObj *p, Frame *f, char *str_text)
{
	if (!str_text)
		return 0;
	if (*str_text == '%' || *str_text == '$')
	{
		char	*actual_text;
		var_table.set_context(p, f, 1, 1, 1);
		if (!var_table.Find(str_text))
		{
			printe(ERR_unknown_debug_var, E_ERROR, str_text);
			return 0;
		}
		actual_text = var_table.Value();
		if (actual_text == 0) 
			printe(ERR_eval_fail_expr, E_ERROR, str_text);
		return actual_text;
	}
	return str_text;
}

// parse number - if a user or debugger variable, evaluate
int
parse_num_var(ProcObj *p, Frame *f, char *num_text, int &ret)
{
	char	*actual_text; 
	if ((actual_text = parse_str_var(p, f, num_text)) == 0)
		return 0;
	char	*end_char;
	ret = (int)strtol(actual_text, &end_char, 0);
	if (end_char == actual_text)
	{
		printe(ERR_numeric_var_expected, E_ERROR, num_text);
		return 0;
	}
	return 1;
}

// parse signal - could be a number, a signal name or
// a debugger or user var that evaluates to either
int
parse_sig(ProcObj *p, Frame *f, char *text)
{
	int	sig = 0;
	char	*actual_text;

	if ((actual_text = parse_str_var(p, f, text)) == 0)
		return 0;
	char	*end_char;
	sig = (int)strtol(actual_text, &end_char, 0);
	if (end_char != actual_text)
	{
		if ( sig > 0 && sig <= NUMBER_OF_SIGS )
			return sig;
	}
	else if ((sig = getsig(actual_text)) != 0)
			return sig;
	printe(ERR_invalid_signal, E_ERROR, actual_text);
	return 0;
}

// parse a list of signals, syscalls or events;
// each list item could be an event, syscall name or number,
// signal name or number or debug or user variable;
// if a user variable, item could be a list, itself
int
parse_list(ProcObj *p, Frame *f, IntList *list, 
	Vector *v, intlist_type itype)
{
	int	val, extra;
	char	*strval;
	IntItem	*ptr = list->first();

	v->clear();

	for(; ptr; ptr = ptr->get_next())
	{
		if (ptr->get_type() == it_int)
		{
			val = ptr->get_ival();
			v->add(&val, sizeof(int));
			if (itype == il_syscalls)
			{
				if ((extra = getsys_mach(val)) != 0)
				{
					v->add(&extra, sizeof(int));
				}
			}
			continue;
		}
		if ((strval = parse_str_var(p, f, ptr->get_sval()))
			== 0)
			return 0;
		char	*next = strtok(strval, " ,");
		while(next)
		{
			char	*end_char;
			if (*next == '%' || *next == '$')
			{
				printe(ERR_nested_variable, E_ERROR, 
					next, ptr->get_sval());
				return 0;
			}
			val = (int)strtol(next, &end_char, 0);
			switch(itype)
			{
			case il_events:
				if (end_char == next)
				{
					printe(ERR_numeric_var_expected,
						E_ERROR, next);
					return 0;
				}
				break;
			case il_signals:
				if (((end_char != next) &&
					(val < 0 || val > NUMBER_OF_SIGS)) ||
					((end_char == next) &&
					((val = getsig(next)) == 0)))
				{
					printe(ERR_invalid_signal, 
						E_ERROR, next);
					return 0;
				}
				break;
			case il_syscalls:
				if (((end_char != next) &&
					(val < 0 || val > (int)lastone))
					|| ((end_char == next) &&
					((val = getsys(next)) == 0)))
				{
					printe(ERR_bad_syscall, 
						E_ERROR, next);
					return 0;
				}
				if ((extra = getsys_mach(val)) != 0)
					v->add(&extra, sizeof(int));
				break;
			}
			v->add(&val, sizeof(int));
			next = strtok(0, " ,");
		}
	}
	if (itype == il_syscalls)
	{
		// syscalls expects list to be null-terminated
		val = 0;
		v->add(&val, sizeof(int));
	}
	return 1;
}

// Location syntax accepted is:
// 	[ProcObj_id@][object_name@]address[+-decimal]
//	[ProcObj_id@][object_name@][filename@][header_file@]funcname[+-constant]
//	[ProcObj_id@][object_name@][filename@][header_file@]line
//	[ProcObj_id@]obj.funcname
//	[ProcObj_id@]obj->funcname
// where
//	address: is a hex or octal constant, user-defined var or
//	built-in var
//	constant: is a decimal, hex or octal integer
//	ProcObj_id: is a program, process or ProcObj id in any accepted form
//	line: is a decimal constant
//	object_name is the name of an object containing a
// 	symbol table

// search tokens in a location description until we find a
// special delimiter character or the at_sign (@).  Return 0
// if we don't find @, else the token following the @.
//
static Token *
find_at_sign(register Token *t)
{
	while(t)
	{
		switch(t->op())
		{
			default:
				t = t->next();
				continue;
			case AT:
					return t->next();
			case AMPERSAND:
				if (t->next() && 
					t->next()->op() == t->op())
					return 0;
				t = t->next();
				continue;
			case BAR:
			case POUND:
			case GREATER:
			case NL:
			case WHITESPACE:
			case SEMI:
			case LCURLY:
				return 0;
		}
	}
	return 0;
}

// template parameters, i.e., <type, type, ...> - appends to parse_buf
static int
get_template_parms(Token *t)
{
	Token	*last;
	int	nest = 1;

	if (!t || t->op() != LESS)
		return 0;
	parse_buf.add('<');
	last = t;
	t = t->next();
	while(t)
	{
		switch(t->op())
		{
		case NL:
		case SEMI:
			nexttok = t;
			return 0;
		case LESS:
			// nested templates
			nest++;
			parse_buf.add('<');
			last = t;
			t = t->next();
			break;
		case GREATER:
			nest--;
			// demangler puts out consecutive ">"'s as "> >"
			if (((char *)parse_buf)[parse_buf.size()-2] == '>')
				parse_buf.add(' ');
			parse_buf.add('>');
			if (nest)
			{
				last = t;
				t = t->next();
				break;
			}
			else
			{
				nexttok = t->next();
				return 1;
			}
		case WHITESPACE:
			if (t->next() &&
				(t->next()->op() == DOT &&
					(last->op() == NAME ||
					last->op() == STAR))
				||
			        (t->next()->op() == NAME &&
					last->op() == NAME))
				parse_buf.add(' ');
			last = t;
			t = t->next();
			break;
		default:
			parse_buf.add(t->print());
			last = t;
			t = t->next();
			break;
		}
	}
	nexttok = t;
	return 0;
}

// argument list of prototype - adds to parse_buf
static int
get_arglist(Token *t)
{
	Token	*last;
	int	nest = 1;

	if (!t || t->op() != LPAREN)
		return 0;
	parse_buf.add('(');
	last = t;
	t = t->next();
	while(t)
	{
		switch(t->op())
		{
		case NL:
		case SEMI:
			nexttok = t;
			return 0;
		case LPAREN:
			// nested parens - pointer to fcn, for example
			nest++;
			parse_buf.add('(');
			last = t;
			t = t->next();
			break;
		case RPAREN:
			nest--;
			parse_buf.add(')');
			if (nest)
			{
				last = t;
				t = t->next();
				break;
			}
			else
			{
				nexttok = t->next();
				return 1;
			}
		case COMMA:
			parse_buf.add(',');
			if (t->next() && t->next()->op() != WHITESPACE)
				parse_buf.add(' ');
			last = t;
			t = t->next();
			break;
		case STAR:
		case AMPERSAND:
			if (last->op() == NAME)
				parse_buf.add(' ');
			// FALL-THROUGH
		default:
			parse_buf.add(t->print());
			last = t;
			t = t->next();
			break;
		}
	}
	nexttok = t;
	return 0;
}

// Get a function name (appended to parse_buf).  Allows:
// name
// name<....>
// ~name
// ~name<....>

static int
get_name(Token *t)
{
	int	tilde = 0;

	if (!t)
		return 0;
	if (t->op() == TILDE)
	{
		// C++ destructor
		tilde = 1;
		t = t->next();
		if (!t)
			return 0;
	}
	if (t->op() != NAME)
		return 0;
	if (tilde)
		parse_buf.add('~');
	parse_buf.add(t->str());
	nexttok = t = t->next();

	if (t && t->op() == LESS)
	{
		if (!get_template_parms(t))
			return 0;
	}
	return 1;
}

// Get special C++ operator names - appends to parse_buf
static int
get_special(Token *t)
{
	t = skipWS(t);
	switch(t->op())
	{
	default:
		break;
	case NAME:
	{
		Token	*last = 0;
		while(t)
		{
			switch(t->op())
			{
			case NL:
			case POUND:
			case LPAREN:
			case SEMI:
				goto sp_out;
			case WHITESPACE:
				parse_buf.add(' ');
				last = t;
				t = skipWS(t->next());
				continue;
			case AMPERSAND:
			case BAR:
				{
					Token	*t1 = t->next();
					if (t1->op() == t->op())
						goto sp_out;
				}
				// FALLTHROUGH
			default:
				if (last && last->op() == NAME)
					parse_buf.add(' ');
				parse_buf.add(t->print());
				break;
			}
			last = t;
			t = t->next();
		}
	}
		break;
	case EQUAL:
	case STAR:
	case PERCENT:
	case SLASH:
	case HAT:
	case BANG:
		parse_buf.add(t->print());
		t = t->next();
		if (t && (t->op() == EQUAL))
		{
			// ==, *=, %=, /=, ^=, !=
			parse_buf.add('=');
			t = t->next();
		}
		break;
	case PLUS:
	case BAR:
	case AMPERSAND:
		parse_buf.add(t->print());
		if (t->next() && (t->next()->op() == t->op())
					|| (t->next()->op() == EQUAL))
		{
			// ++, +=, ||, |=, &&, &=
			parse_buf.add(t->print());
			t = t->next()->next();
		}
		else
			t = t->next();
		break;
	case ARROW:
	case COMMA:
	case GREATEREQUAL:
	case TILDE:
		parse_buf.add(t->print());
		t = t->next();
		break;
	case LBRACK:
		// []
		if (t->next() && 
			t->next()->op() == RBRACK)
		{
			parse_buf.add("[]");
			t = t->next()->next();
		}
		break;
	case LPAREN:
		//  (...)
		parse_buf.add('(');
		t = t->next();
		while(t)
		{
			switch(t->op())
			{
			case NL:
			case SEMI:
				nexttok = t;
				return 0;
			case RPAREN:
				parse_buf.add(')');
				nexttok = t->next();
				return 1;
			default:
				parse_buf.add(t->print());
				t = t->next();
				break;
			}
		}
		break;
	case GREATER:
		parse_buf.add('>');
		t = t->next();
		if (t && ((t->op() == GREATER) || 
			(t->op() == GREATEREQUAL)))
		{
			// >>, >>=
			parse_buf.add(t->print());
			t = t->next();
		}
		break;
	case LESS:
		parse_buf.add('<');
		t = t->next();
		if (t && ((t->op() == LESS) || (t->op() == EQUAL)))
		{
			//  <<, <=
			if ((t->op() == LESS) && t->next() &&
				t->next()->op() == EQUAL)
			{
				//  <<=
				parse_buf.add("<=");
				t = t->next()->next();
			}
			else
				parse_buf.add(t->print());
		}
		break;
	case MINUS:
		parse_buf.add('-');
		if (t->next() && (t->op() == t->next()->op())
					|| (t->next()->op() == EQUAL)
					|| (t->next()->op() == GREATER))
		{
			// --, -=, ->
			t = t->next();
			parse_buf.add(t->print());
			if ((t->op() == GREATER) && t->next() &&
				t->next()->op() == STAR)
			{
				// ->*
				parse_buf.add('*');
				t = t->next()->next();
			}
			else
				t = t->next();
		}
		break;
	}
sp_out:
	if (!t || t->op() != LPAREN)
	{
		nexttok = t;
		return 1;
	}
	// loop to handle special case of conversion function returning
	// pointer to function
	// would look like operator type (*)(arg list)()
	while (t->op() == LPAREN)
	{
		if (!get_arglist(t))
			return 0;
		t = nexttok;
	}
	return 1;
}

// get const and/or volatile qualifiers at end of name
// qualifiers are canonicalized for USG demangler
static char *
get_cv_list(Token *t)
{
	int	cfound = 0;
	int	vfound = 0;
	char	*list = 0;
	while(t)
	{
		switch(t->op())
		{
		default:
			goto cv_out;
		case WHITESPACE:
			t = skipWS(t->next());
			break;
		case NAME:
			if (t->str() == str("const"))
				cfound++;
			else if (t->str() == str("volatile"))
				vfound++;
			else
				goto cv_out;
			t = t->next();
			break;
		}
	}
cv_out:
	if (cfound && vfound)
	{
		list = str(" const volatile");
		nexttok = t;
	}
	else if (cfound)
	{
		list = str(" const");
		nexttok = t;
	}
	else if (vfound)
	{
		list = str(" volatile");
		nexttok = t;
	}
	return list;
}

// Get function name 
// For C++, we handle the following forms:
// [::] [prefix]* fname cv_list [template_args]
// [::] [prefix]* ~fname [template_args]
// [::] [prefix]* operator op cv_list [template_args]
// where fname can be a full prototype, including template brackets,
// prefix is name::, name<....>::, name->, or name,
// and template args is anything within square brackets ("[ ... ]")
// qual_list is [const] [volatile]
// uses_object is set to 1 if the prefix includes -> or .

static char *
get_func_name(Token *t, int &uses_object)
{
	int	tilde_seen = 0;

	uses_object = 0;

	if (!t)
		return 0;

	parse_buf.clear();
	if (t->op() == COLON)
	{
		t = t->next();
		if (!t || t->op() != COLON)
		{
			nexttok = t;
			return 0;
		}
		t = t->next();
		if (!t || ((t->op() != NAME) && (t->op() != TILDE))) 
		{
			nexttok = t;
			return 0;
		}
		parse_buf.add("::");
	}

	while (t)
	{
		if (t->op() == NAME && t->str() == str("operator"))
		{
			char	*cv_list;
			parse_buf.add("operator ");
			if (!get_special(t->next()))
				return 0;
			t = nexttok;
			if ((cv_list = get_cv_list(t)) != 0)
				parse_buf.add(cv_list);
			goto handle_template;
		}

		tilde_seen = (t->op() == TILDE);
		if (!get_name(t))
			return 0;

		t = nexttok;
		if (!t)
			return str((char *)parse_buf);

		if ((t->op() == ARROW || t->op() == DOT) && !tilde_seen)
		{
			uses_object = 1;
			parse_buf.add(t->print());
			t = t->next();
			continue;
		}
		else if (t->op() == COLON && !tilde_seen)
		{
			t = t->next();
			if (t->op() != COLON)
			{
				nexttok = t;
				return 0;
			}
			parse_buf.add("::");
			t = t->next();
			continue;
		}
		else if (t->op() == LPAREN)
		{
			// not part of a prefix (name-> or name::), must
			// be the function name
			char	*cv_list;
			if (!get_arglist(t))
				return 0;
			t = nexttok;
			if ((cv_list = get_cv_list(t)) != 0)
				parse_buf.add(cv_list);
		}
		goto handle_template;
	}

	nexttok = t;
	return 0;

handle_template:
	char *fname = str((char *)parse_buf);
	t = skipWS(nexttok);
	if (t && t->op() == LBRACK)
	{
		char *tstr;
		if ((tstr = get_expr(t, 1)) == 0)
			return 0;
		parse_buf.clear();
		parse_buf.add(fname);
		parse_buf.add(' ');
		parse_buf.add(tstr);
		return str((char *)parse_buf);
	}
	else
		return fname;
}

// get base type of location entry
static int
get_base_type(Location *l, Token *t)
{
	int		minus = 0;
	int		ret = 1;
	char		*var;
	dbg_vtype	vtype;

	switch(t->op())
	{
	case DEC:
		l->set_line(t->val());
		t = t->next();
		goto base_out;
	case HEX:
	case OCT:	// address
		l->set_addr(t->val());
		t = t->next();
		break;
	case DEBUG_NAME:	// regname or builtin var
		var = t->str();
		vtype = dbg_var(var);
		switch(vtype)
		{
		case NoType_v:
			// assume register
			l->set_addr(var);
			break;
		case Line_v:
		case List_line_v:
			l->set_line(var);
			t = t->next();
			goto base_out;
		case Func_v:
			l->set_func(var);
			break;
		case  Loc_v:
			l->set_addr(var);
			break;
		default:
			printe(ERR_loc_debug_var, E_ERROR, var);
			ret = 0;
			goto base_out;
		}
		t = t->next();
		break;
	case USER_NAME:
		l->set_var(t->str());
		t = t->next();
		break;
	case NAME:
	case TILDE:
	case COLON:
		{
			// func name
			int uses_object = 0;
			var = get_func_name(t, uses_object);
			if (!var)
			{
				ret = 0;
				goto base_out;
			}
			l->set_func(var);
			if (uses_object)
				l->set_flag(L_HAS_OBJECT);
			t = nexttok;
			break;
		}
	}
	if (!t)
	{
		goto base_out;
	}
	if (t->op() == MINUS)
		minus = 1;
	else if (t->op() != PLUS)
	{
		goto base_out;
	}
	t = t->next();
	if (!t)
	{
		ret = 0;
		goto base_out;
	}
	switch(t->op())
	{
	case USER_NAME:
		l->set_offset(t->str());
		if (minus)
			l->set_flag(L_MINUS_OFF);
		else
			l->set_flag(L_PLUS_OFF);
		t = t->next();
		break;
	case HEX:
	case DEC:
	case OCT:
		{
			long	off = t->val();
			if (minus)
				off = -off;
			l->set_offset(off);
			t = t->next();
		}
		break;
	default:
		ret = 0;
		break;
	}
base_out:
	nexttok = t;
	return ret;
}

// read tokens until @ and paste together
// we can assume @ ends file name because of find_at_sign()
// If entire filename is quoted, string = 1
static char *
get_file(Token *t, int &string)
{
	int		count = 0;

	parse_buf.clear();
	string = 0;
	while ( t && t->op() != AT )
	{
		count++;
		if (t->op() == STRING)
		{
			if (count == 1)
				string = 1;
			parse_buf.add(t->raw());
		}
		else
			parse_buf.add(t->print());
		t = t->next();
	}
	if (t)
		t = t->next(); // point past @
	nexttok = t;
	if (string && count > 1)
		string = 0;
	return ((parse_buf.size() > 1) ? str((char *)parse_buf) : 0);
}

static Location *
parse_loc( register Token *t )
{
	Location	*l = new Location();
	int		string = 0;
	int		at_count = 0;

	if (!t)
	{
		delete l;
		return 0;
	}

	while (find_at_sign(t))
	{
		at_count++;
		switch (at_count)
		{
		case 1:
			l->set_pobj(get_file((Token *)t, string));
			t = nexttok;
			break;
		case 2:
			l->set_symtab(get_file((Token *)t, string));
			if (string)
				l->set_flag(L_SYM_STRING);
			t = nexttok;
			break;
		case 3:
			l->set_file(get_file((Token *)t, string));
			if (string)
				l->set_flag(L_FILE_STRING);
			t = nexttok;
			break;
		case 4:
			l->set_header(get_file((Token *)t, string));
			if (string)
				l->set_flag(L_HEADER_STRING);
			t = nexttok;
			break;
		}
	}
	// 4 @'s: procobj, object, file name, and header file
	// less that 4 could be any combination of those
	if (at_count > 0)
	{
		if (at_count < 3)
			l->set_flag(L_CHECK_types);
		else if (at_count > 4)
		{
			printe(ERR_loc_too_complex, E_ERROR);
			return 0;
		}
	}

	if (!get_base_type(l, (Token *)t))
	{
		delete l;
		return 0;
	}
	if (l->get_type() == lk_none)
	{
		delete l;
		return 0;
	}
	return l;
}

#define NEXT(t)			t = skipWS(t->next())

static int err = (int)ERR_internal;
static char *bad_option = 0;

// saw a '>' or '|'
static Node *
get_redir( register Token *t )
{
	Redir rtype;
	char *arg;

	if ( t->op() == GREATER )
	{
		if ( t->next() && t->next()->op() == GREATER )
		{
			rtype = RedirAppend;
			t = t->next();
		} else
			rtype = RedirFile;
		NEXT(t);
		if ((arg = get_filename(t)) == 0)
		{
			err = ERR_null_filename;
			return 0;
		}
	}
	else if ( t->op() == BAR )
	{
		NEXT(t);
		rtype = RedirPipe;
		if ((arg = get_shell_cmd(t)) == 0)
		{
			err = ERR_no_shell_cmd;
			return 0;
		}
	}
	return new Node(REDIR, NODE, 0, CHARSTAR, arg, INT, (void*)rtype);
}


static Node *
prune( register Node *root )	// remove empty CMDLIST nodes
{
	if ( root->op == CMDLIST)
	{
		if (root->first.np == 0 &&
			root->second.np == 0 ) 
		{
			delete root;
			root = 0;
		} 
		else 
			root->second.np = prune( root->second.np );
	}
	return root;
}

enum State {
	BEGIN,
	END,
	COMMENT,
	COMMAND,
	REDIRECT,
	EOL,
	ERROR,
	NESTED_ERROR,
};

#if DEBUG
static char *
statename( register State s )
{
	register char *name = "UNK";
	switch ( s ) {
	case BEGIN:	name = "BEGIN";		break;
	case END:	name = "END";		break;
	case COMMENT:	name = "COMMENT";	break;
	case COMMAND:	name = "COMMAND";	break;
	case REDIRECT:	name = "REDIRECT";	break;
	case EOL:	name = "EOL";		break;
	case ERROR:	name = "ERROR";		break;
	}
	return name;
}
#endif

static State parser( Token *, int, int, Node *& );
static int depth = 0;

#define	CMD_LIST	0
#define	ONE_ONLY	1

#define error(msg)		(err = (int)msg, state = ERROR)

// parse_cmd handles one command's options and arguments
// the keyword has already been seen when parse_cmd is called
// return value will be EOL or ERROR

static State
parse_cmd(register Token *t, register Node *cmd, Keyword *keyptr, Node *&redir)
{
	register State state = COMMAND;	// parse arguments as long as state
					// is COMMAND
	Opts opts = 0;			// options seen
	const char *optstring = 0;	// string giving option letters and
					// whether or not they take an argument
	long	*arg_type = 0;		// table listing expected arguments
	register Op op;		// command type
	int op_num;		// current argument
	int op_type;		// expected type of current argument
	char *arg = 0;		// argument handled as a string
	Node *n = 0;		// argument that is another command or cmd block
	int found = 0;		// got at least one argument in a list
	int quit = 0;		// end of list of arguments - hit something
				// that doesn't look like what is expected
	Opnd opnd;		// holds current argument or argument list
	OpType opnd_type = NONE;// current operand type

	opnd.i = 0;
	if (!keyptr)
	{
		error(ERR_parser_internal);
		return state;
	}

	optstring = keyptr->opts;
	arg_type = keyptr->arg_type;
	op = keyptr->op;

	// find the table entry for the first argument
	for ( op_num = 0; op_num < MAX_ARGS; ++op_num )
	{
		if (arg_type[op_num] && !(arg_type[op_num] & OP_OPT))
			break;
	}

	while (t && state == COMMAND)
	{
		t = skipWS(t);
		if (isredir(t->op()))
		{
			if (redir)
			{
				error(ERR_multiple_io);
				break;
			}
			else if ((redir = get_redir(t)) == 0)
			{
				state = ERROR;
				break;
			}
			t = skipWS(nexttok);
			continue;
		}

		// parse options, etc
		DPRINT(DBG_PARSER,("cmd = %#x, t = %s\n", cmd, t->dump()))

		if (optstring && t->op() == MINUS)
		{
			int arg_num = 0;
			bad_option = parse_opts(&opts, optstring, t, &arg_num);
			if ( !opts || (opts & ERRORBIT))
			{
				t = nexttok;
				if ( opts & DUPBIT )
					error(ERR_dup_option);
				else if ( bad_option )
					error(ERR_bad_option);
				else
					error(ERR_missing_option);
				continue;
			}
			if ((op == RUN || op == STEP) && 
				((opts & (DASH('b') | DASH('f'))) ==
					(DASH('b') | DASH('f'))))
			{
				t = nexttok;
				error(err_dashb_dashf);
				continue;
			}
			else if ((op == CREATE) &&
				((opts & (DASH('n') | DASH('r'))) ==
					(DASH('n') | DASH('r'))))
			{
				t = nexttok;
				error(err_dashn_dashr);
				continue;
			}
			else if ((op == CHANGE) &&
				((opts & (DASH('q') | DASH('v'))) ==
					(DASH('q') | DASH('v'))))
			{
				t = nexttok;
				error(err_dashq_dashv);
				continue;
			}

			t = skipWS(nexttok);
			cmd->opts = opts;

			// arg_num is set if option takes an operand
			if (arg_num)
			{
				if (arg_type[arg_num-1] & OP_PROCS)
				{
					Proclist *procs;
					if (((arg = get_token(t)) == 0) ||
						((procs = 
						get_proclist(arg)) == 0))
					{
						error(ERR_missing_proc);
						continue;
					}
					cmd->setopnd(arg_num, PROCLIST, procs);
				}
				else if (arg_type[arg_num-1] & OP_LOCATION)
				{
					Location *l;
					if ((l = parse_loc(t)) == 0)
					{
						error(ERR_missing_location);
						continue;
					}
					cmd->setopnd(arg_num, LOC, l);
				}
				else if (arg_type[arg_num-1] & OP_NUMBER)
				{
					if (isvarname(t->op()))
					{
						cmd->setopnd(arg_num, INT_VAR, t->str());
					}
					else if (t->op() == DEC || ((t->op() == OCT)
						&& ((int)t->val() == 0)))
						cmd->setopnd(arg_num, INT, (void*)t->val());
					else
					{
						error(ERR_missing_number);
						continue;
					}
					NEXT(t);
					nexttok = t;
				}
				else if (arg_type[arg_num-1] & OP_HEXNUM)
				{
					if (isvarname(t->op()))
						cmd->setopnd(arg_num, INT_VAR, t->str());
					else if (t->op() == HEX)
						cmd->setopnd(arg_num, INT, (void*)t->val());
					else
					{
						error(ERR_missing_hexnum);
						continue;
					}
					NEXT(t);
					nexttok = t;
				}
				else if (arg_type[arg_num-1] & OP_STRING)
				{
					if (t->op() == STRING)
						cmd->setopnd(arg_num, STRING_OPND, t->resolved_string());
					else if (isvarname(t->op()))
						cmd->setopnd(arg_num, CHARSTAR, t->str());
					else
					{
						error(ERR_missing_string);
						continue;
					}
					NEXT(t);
					nexttok = t;
				}
				else if (arg_type[arg_num-1] & OP_RAWSTRING)
				{
					if (t->op() == STRING)
						cmd->setopnd(arg_num, STRING_OPND, t->raw());
					else if (isvarname(t->op()))
						cmd->setopnd(arg_num, CHARSTAR, t->str());
					else
					{
						error(ERR_missing_string);
						continue;
					}
					NEXT(t);
					nexttok = t;
				}
				else
				{
					if ((arg = get_token(t)) == 0)
					{
						error(ERR_missing_arg);
						continue;
					}
					cmd->setopnd(arg_num,CHARSTAR,str(arg));
				}
				t = skipWS(nexttok);
			}
			continue;
		}

		// special case handling for if-else
		if (op == IF && op_num == 2)
		{
			Token	*t2 = 0;
			if (t->op() == SEMI) 
			{
				t2 = t;

				NEXT(t);
			}
			if ((t->op() == NAME) && (t->str() == else_str))
			{
				NEXT(t);
				if (!t || t->op() == NL)
				{
					error(ERR_missing_block);
					continue;
				}
			}
			else
			{
				if (t2)
					t = t2;
				state = END;
				continue;
			}
		}

		// no more arguments
		if (t->op() == NL || t->op() == SEMI || t->op() == RCURLY
			|| t->op() == POUND)
		{
			if (found && (arg_type[op_num] & OP_LIST))
			{
				cmd->setopnd(++op_num, opnd_type, opnd.cp);
				found = 0;
				continue;
			}

			if ((op_num < MAX_ARGS)
				&& (arg_type[op_num] & OP_REQ))
			{
				// not all cases are in the switch because
				// not all option types are used in commands
				// where an option is required
				switch( arg_type[op_num] & OP_TYPE_MASK )
				{
					case OP_FILE:
						err = (int)ERR_missing_file;
						break;
					case OP_COMMAND:
						err = (int)ERR_missing_cmd;
						break;
					case OP_LOCATION:
						err = (int)ERR_missing_location;
						break;
					case OP_SIGNAL:
						err = (int)ERR_missing_signal;
						break;
					case OP_EXPR:
					case OP_SET_EXPR:
						err = (int)ERR_missing_expr;
						break;
					case OP_CMD_BLOCK:
						err = (int)ERR_missing_block;
						break;
					case OP_EVENT:
						err = (int)ERR_missing_event;
						break;
					default:
						err = (int)ERR_missing_arg;
						break;
				}
				state = ERROR;
			}
			else
				state = EOL;
			continue;
		}
		else if (op_num == MAX_ARGS || !arg_type[op_num])
		{
			error(ERR_too_many_args);
			continue;
		}

		op_type = (int)arg_type[op_num] & OP_TYPE_MASK;

		// special case handling for LIST regular expression
		if ((op == LIST) && ((t->op() == SLASH) || 
			(t->op() == QUESTION)))
		{
			op_type = OP_REGEXP;
			cmd->opts |= (t->op() == SLASH) ? DASH('d'):DASH('u');
		}
		// special case for CHANGE command;
		// must know the type of the event being changed
		// so we know how to parse the event specification
		else if ((op == CHANGE) && (op_num == 3))
		{
			if (t->op() == LCURLY)
			{
				// either just a command block, or end of
				// a list of sigs, syscalls followed by cmd
				if (found && (arg_type[op_num] & OP_LIST))
				{
					cmd->setopnd(++op_num, opnd_type, opnd.cp);
					found = 0;
				}
				else
					++op_num;
				continue;
			}
			int		event;
			IntList		*il = cmd->third.ilist;
			Event		*eptr;
			IntItem		*it = il->first();

			if (it->get_type() == it_int)
				event = it->get_ival();
			else if (!parse_num_var(0, 0, it->get_sval(),
				event))
				event = 0;

			if (!event || ((eptr =
				m_event.find_event(event)) == 0))
			{
				error(ERR_active_event);
				continue;
			}
			switch(eptr->get_type())
			{
			case E_ONSTOP:
				// onstop doesn't take a specifier
					op_num++;
					if (cmd->optype[0] == INT ||
						cmd->optype[0] == INT_VAR)
						printe(ERR_change_dashc,
							E_WARNING);
					continue;
			case E_STOP:
					op_type = OP_STOP;
					break;
			case E_SIGNAL:	
					if (cmd->optype[0] == INT ||
						cmd->optype[0] == INT_VAR)
						printe(ERR_change_dashc,
							E_WARNING);
					op_type = OP_SIGNAL;
					break;
			case E_SCALL:	
					op_type = OP_SYSCALL;
					break;
#if EXCEPTION_HANDLING
			case E_EH_EVENT:
					op_type = OP_EXPR;
					break;
#endif
			}
		}

		switch( op_type )
		{
			case OP_NAME:
				// expects a single C-style identifier
				// first argument of alias and macro
				if (t->op() != NAME && 
					!isvarname(t->op()))
				{
					error(ERR_missing_id);
					continue;
				}
				opnd_type = CHARSTAR;
				opnd.cp = t->str();
				NEXT(t);
				break;

			case OP_TOKENS:
			{
				// save list of tokens for alias 
				Token *p = 0, *pp = 0;
				while (t && t->op() != NL)
				{
					if (t->op() >= BADNUMBER)
					{
						state = ERROR;
						break;
					}
					if (t->op() == POUND)
					{
						// allow $# - otherwise
						// # terminates alias
						if (!pp ||
							pp->op() !=
							DOLLAR)
						break;
					}

					Token *tl = new Token(t);
					if (p)
					{
						tl->append(pp);
						pp = tl;
					}
					else
						pp = p = tl;
					t = t->next();
				}
				opnd_type = TOKEN;
				opnd.tl = p;
			}
				break;

			case OP_FILE:
				// get a filename (the script command) or
				// a list of filenames (grab)
				Filelist *fl;

				if ((arg = get_filename(t)) == 0)
				{
					// must be at least one file name,
					// but if already have one, then
					// just stop looping without an error
					if (!found)
					{
						error(ERR_missing_file);
						continue;
					}
					else
					{
						quit = 1;
						break;
					}
				}
				t = skipWS(nexttok);

				// some commands take a list of file names,
				// others just one
				if (arg_type[op_num] & OP_LIST)
				{
					opnd_type = FILELIST;
					fl = new Filelist(arg);
					if (found)
						opnd.files->concat(fl);
					else
						opnd.files = fl;
				}
				else
				{
					opnd_type = CHARSTAR;
					opnd.cp = arg;
				}
				++found;
				break;
				
			case OP_COMMAND:
				// expects a shell-type command for create and !
				opnd_type = CHARSTAR;
				if ((opnd.cp = get_shell_cmd(t)) == 0)
				{
					error(ERR_no_shell_cmd);
					continue;
				}
				t = nexttok;
				break;

			case OP_LOCATION:
				Location *l;
				if ((l = parse_loc(t)) == 0)
				{
					error(ERR_loc_syntax);
					continue;
				}
				t = skipWS(nexttok);
				opnd.loc = l;
				opnd_type = LOC;
				++found;
				break;

			case OP_SIGNAL:
				IntItem *nsig;
				if ((nsig = parse_intlist(t, 
					il_signals)) == 0)
				{
					if (!found)
					{
						error(ERR_missing_signal);
						continue;
					}
					else
					{
						quit = 1;
						break;
					}
				}

				t = skipWS(nexttok);

				if (!found)
					opnd.ilist = new IntList;
				opnd.ilist->add(nsig);
				opnd_type = INTLIST;
				++found;
				break;

			case OP_NUMBER:
				if (isvarname(t->op()))
				{
					opnd.cp = t->str();
					opnd_type = INT_VAR;
				}
				else if (t->op() == DEC || ((t->op() == OCT)
					&& ((int)t->val() == 0)))
				{
					opnd.i = (int)t->val();
					opnd_type = INT;
				}
				else
				{
					error(ERR_missing_number);
					continue;
				}
				NEXT(t);
				break;

			case OP_HEXNUM:
				if (isvarname(t->op()))
				{
					opnd.cp = t->str();
					opnd_type = INT_VAR;
				}
				else if (t->op() == HEX)
				{
					opnd.i = (int)t->val();
					opnd_type = INT;
				}
				else
				{
					error(ERR_missing_hexnum);
					continue;
				}
				NEXT(t);
				break;

			case OP_EXPR:
				if (op == IF || op == WHILE)
				{
					if (t->op() != LPAREN
						&& t->op() != LBRACK
						&& t->op() != LCURLY)
					{
						error(ERR_missing_p_expr);
						continue;
					}
					arg = get_expr(t, 1);
				}
#if EXCEPTION_HANDLING
				// '{' indicates beginning of command block
				// for exception events
				else if (t->op() == LCURLY
					&& op == EXCEPTION)
					arg = 0;
#endif
				else
					arg = get_expr(t, 0);

				if (arg == 0)
				{
					if (!found)
					{
						error(ERR_missing_expr);
						continue;
					}
					else
					{
						quit = 1;
						break;
					}
				}
				t = skipWS(nexttok);

				if ((arg_type[op_num] & OP_LIST)
					&& (op != CHANGE))
				{
					opnd_type = EXPLIST;
					if (found)
						opnd.exp->concat(arg);
					else
						opnd.exp = new Exp(arg);
				}
				else
				{
					opnd_type = CHARSTAR;
					opnd.cp = arg;
				}
				++found;
				break;

			case OP_REGEXP:
				opnd_type = CHARSTAR;
				opnd.cp = get_regexp(t);
				t = skipWS(nexttok);
				break;

			case OP_STRING:
				if (t->op() == STRING)
				{
					opnd.cp = t->resolved_string();
					opnd_type = STRING_OPND;
				}
				else if (isvarname(t->op()))
				{
					opnd.cp = t->str();
					opnd_type = CHARSTAR;
				}
				else
				{
					error(ERR_missing_string);
					continue;
				}
				NEXT(t);
				nexttok = t;
				break;
			case OP_RAWSTRING:
				if (t->op() == STRING)
					opnd.cp = t->raw();
				else if (isvarname(t->op()))
					opnd.cp = t->str();
				else
				{
					error(ERR_missing_string);
					continue;
				}
				opnd_type = CHARSTAR;
				NEXT(t);
				nexttok = t;
				break;
			case OP_CHARSTAR:
				if ((arg = get_token(t)) != 0)
				{
					opnd.cp = str(arg);
					t = skipWS(nexttok);
				}
				else
				{
					error(ERR_missing_string);
					continue;
				}
				opnd_type = CHARSTAR;
				break;

			case OP_SYSCALL:
				IntItem *nsys;
				if ((nsys = parse_intlist(t, 
					il_syscalls)) == 0)
				{
					if (!found)
					{
						error(ERR_bad_syscall);
						continue;
					}
					else
					{
						quit = 1;
						break;
					}
				}
				t = skipWS(nexttok);

				if (!found)
					opnd.ilist = new IntList;
				opnd.ilist->add(nsys);
				opnd_type = INTLIST;
				++found;
				break;

			case OP_EVENT:
			{
				IntItem	*il;
				if ((il = parse_intlist(t, 
					il_events)) != 0)
				{
					if (opts & DASH('a'))
					{
						error(ERR_unexpected_arg);
						continue;
					}
					if (!found)
						opnd.ilist = new IntList;
					opnd.ilist->add(il);
					opnd_type = INTLIST;
					t = skipWS(nexttok);
					++found;
					break;
				}
				else if ((op == EVENTS) || (op == CHANGE))
				{
					error(ERR_missing_event_num);
					continue;
				}
				else if (t->op() == NAME)
				{
					Keyword	*key;

					if ((key = keyword(t->str())) != 0
						&& (key->op == STOP
						   || key->op == SYSCALL
						   || key->op == ONSTOP
#if EXCEPTION_HANDLING
						   || key->op == EXCEPTION
#endif
						   || key->op == SIGNAL))
					{
						opnd.np = new Node(key->op,
							NODE, n);
						opnd_type = NODE;
						found++;
						quit = 1;
						NEXT(t);
						break;
					}
				}
				error(ERR_missing_event);
				break;
			}
			case OP_CMD_BLOCK:
				// either one debugger command or a block
				// with multiple commands
				{
					State s = parser(t, ONE_ONLY, 0, n);
					t = skipWS(nexttok);
					if (s == ERROR || s == NESTED_ERROR)
					{
						state = NESTED_ERROR;
						continue;
					}
				}
				switch(op)
				{
				case CHANGE:
				case ONSTOP:
				case SIGNAL:
				case STOP:
				case SYSCALL:
#if EXCEPTION_HANDLING
				case EXCEPTION:
#endif
				// associated command
					opnd.np = new Node(SAVE, 
						NODE, n);
					break;
				default:
					opnd.np = new Node(CMDLIST, 
						NODE, n);
					break;
				}
				opnd_type = NODE;
				break;

			case OP_STOP:
				// stop expression
				if ((opnd.stop = parse_event(t)) == 0)
				{
					error(ERR_missing_event_exp);
					continue;
				}
				opnd_type = EVENT_EXPR;
				quit = 1;
				t = skipWS(nexttok);
				break;
			case OP_UNAME:
				// user-defined debug var - for export
				if (t->op() != USER_NAME)
				{
					error(ERR_debug_id_dollar);
					continue;
				}
				opnd.cp = t->str();
				opnd_type = CHARSTAR;
				quit = 1;
				NEXT(t);
				break;
			case OP_SET_EXPR:
				if ((t->op() != DEBUG_NAME) &&
					(t->op() != USER_NAME))
					break;
				opnd.cp = t->str();
				NEXT(t);
				if (t->op() == EQUAL)
					NEXT(t);
				opnd_type = CHARSTAR;
				quit = 1;
				break;

#if EXCEPTION_HANDLING
			case OP_EH_TYPE:
				if (t->op() == NAME)
				{
					int val = 0;
					if (strcmp(t->str(), "throw") == 0)
					{
						val = E_THROW;
						NEXT(t);
						if (t->op() == NAME
							&& strcmp(t->str(), "catch") == 0)
						{
							val |= E_CATCH;
							NEXT(t);
						}
					}
					else if (strcmp(t->str(), "catch") == 0)
					{
						val = E_CATCH;
						NEXT(t);
						if (t->op() == NAME
							&& strcmp(t->str(), "throw") == 0)
						{
							val |= E_THROW;
							NEXT(t);
						}
					}
					else
						break;
					opnd.i = val;
					opnd_type = INT;
					quit = 1;
				}
				break;
#endif
			default:	// shouldn't be able to get here
				error(ERR_parser_internal);
				continue;
		}

		// if the command takes a list, keep looping, unless quit is
		// set, which means it hit something that doesn't belong in
		// the list
		if (quit || !(arg_type[op_num] & OP_LIST))
		{
			cmd->setopnd(++op_num, opnd_type, opnd.cp);
			opnd_type = NONE;
			opnd.i = 0;
			quit = 0;
			found = 0;
		}
	}

	// end of a command
	if ((op == ALIAS) && (opts&DASH('r')) && !cmd->second.cp)
		error(ERR_missing_id);

	nexttok = t;
	return state;
}

// keep a stack of pointers to token lists, so we can
// delete them; we keep a single static unused
// entry as the head of the list
class Tokstack : public Link {
	Token	*toklist;
public:
		Tokstack(Token *t) { toklist = t; }
		Tokstack() { toklist = 0; }
		~Tokstack() { delete toklist; }
	int	is_empty() { return !next(); }
	void	subst(Token *o, Token *n) { if (toklist == o)
			toklist = n; }
			// only substitute of old matches
			// saved entry - only save head
			// of token list for a cmd
	void	push(Tokstack *p) { ((Link *)p)->append(this); }
	Tokstack *pop() { return (Tokstack *)Link::next()->unlink(); }
	Tokstack *next() { return (Tokstack *)Link::next(); }
};

static Tokstack	tokstack;

#define MAX_NEST	20	// maximum nesting level for aliases

// parser handles one or more commands on a line, and returns
// a linked list of command nodes
// cmd_list is normally set to CMD_LIST - to parse everything on one line -
// but is set to ONE_ONLY for commands - if and while - that take a
// command as an argument

static State
parser( register Token *t, int cmd_list, int inblock, Node *& n )
	// parse a command list or block
{
	register State state = BEGIN;

	Node *cmd = 0;		// current cmd node
	Node *root = new Node(CMDLIST);	// always root of tree
	Node *cur = root;	// current CMDLIST node
	Node *redir = 0;	// curent REDIR node

	Keyword *keyptr = 0;
	int errors = 0;

    while ( t || inblock )
    {

	// could be in a block that goes over a line
	if (!t)
	{
		error(ERR_unfinished_cmd);
		inblock = 0;
	}

	DPRINT(DBG_PARSER, ("%d: %-8s %s\n", 
		inblock, statename(state), t->print()))
	DPRINT(DBG_PARSER, ("redir = %#x, cmd = %#x\n", redir, cmd))

	switch ( state ) {

	case BEGIN:
		// expecting a command to start here
		t = skipWS(t);		// skip whitespace

		switch( t->op() ) {
		case POUND:
			NEXT(t);
			state = COMMENT;
			break;

		case NL:
			// skip newlines
			if ( redir && !cmd )
				error(ERR_missing_cmd);
			else
				NEXT(t);
			break;

		case BANG:
			// shell escape
			keyptr = keyword(str("!"));
			NEXT(t);
			state = COMMAND;
			break;

		case SEMI:
			// command separator
			if ( redir && !cmd )
				error(ERR_missing_cmd);
			else
				state = EOL;
			break;

		case NAME:
			int nest;
			nest = 0;
			while ( nest < MAX_NEST )
			{
				Token *tl;
				tl = t->alias();

				// substitute
				if ( tl )
				{
					Token	*oldt = t;
					Tokstack *tk;
					t = t->subst(tl);
					tk = tokstack.next();
					if (tk) // should always
						// be true
						tk->subst(oldt, t);
					delete oldt;
				}
				else
					break;
				++nest;
			}
			if ( nest >= MAX_NEST )
			{	// recursive?
				error(ERR_alias_recursion);
				continue;
			}

			// keyword?
			keyptr = 0;
			if ( t->op() != NAME )
			{
				// could get here through an alias
				state = BEGIN;
			}
			else if (keyptr = keyword(t->str()))
			{
#ifndef HAS_SYSCALL_TRACING
				if (keyptr->op == SYSCALL)
				{
					error(ERR_not_supported_platform);
					break;
				}
#endif
				NEXT(t);
				state = COMMAND;
			}
			else if ( t->str() == else_str )
				error(ERR_syntax_error);
			else
				error(ERR_bad_keyword);
			break;

		case LCURLY:
			// begin block
			NEXT(t);
			++depth;
			{
				State s = parser(t, CMD_LIST, 1, cmd);
				--depth;
				t = skipWS(nexttok);
				if (s == ERROR || s == NESTED_ERROR)
					state = NESTED_ERROR;
				else
					state = END;
			}
			break;

		case RCURLY:
			// end block
			if ( inblock ) {
				NEXT(t);
				goto out;
			} else if (depth)
				goto out;
			else
				error(ERR_extra_curly);
			break;

		case GREATER:
		case BAR:
			// I/O redirection
			state = REDIRECT;
			break;

		default:
			// bad punctuation
			error(ERR_syntax_error);
			break;
		}
		break;

	case COMMENT:
		// flush tokens til newline
		while (t && t->op() != NL)
			NEXT(t);
		if (!cmd)
		{
			state = BEGIN;
			continue;
		}
		state = END;
		// fallthrough

	case END:
		// have seen a command, add to the list and reset to get another
		if ( redir ) {
			redir->setopnd(1, NODE, cmd);
			cmd = redir;
			redir = 0;
		}
		cur->setopnd(1, NODE, cmd);
		cur->setopnd(2, NODE, new Node(CMDLIST));
		cur = cur->second.np;
		cmd = 0;

		if (!t || cmd_list == ONE_ONLY)
			goto out;
		else if (t->op() != NL && t->op() != SEMI
			&& t->str() != else_str)
		{
			if ( t->op() == RCURLY && inblock ) {
				NEXT(t);
				goto out;
			} else if (depth)
				goto out;
			state = BEGIN;
		}
		else
		{
			NEXT(t);
			state = BEGIN;
		}
		break;

	case COMMAND:
		cmd = new Node(keyptr->op);
		state = parse_cmd(t, cmd, keyptr, redir);
		t = nexttok;
		break;

	case REDIRECT:
		// hit a redirection operator - those can come anywhere,
		// but only one allowed
		if (redir)
		{
			error(ERR_multiple_io);
			break;
		}
		else if ((redir = get_redir(t)) == 0)
		{
			state = ERROR;
			break;
		}
		t = skipWS(nexttok);
		state = BEGIN;
		break;

	case EOL:
		// end of "line"
		switch (t->op()) {
		case NL:
		case SEMI:
			state = END;
			break;
		case POUND:
			NEXT(t);
			state = COMMENT;
			break;
		case GREATER:
		case BAR:
			state = REDIRECT;
			break;
		case RCURLY:
			if (inblock || depth)
				state = END;
			else if (!depth)
				error(ERR_syntax_error);
			break;
		case NAME:
			if ( t->str() == else_str ) {
				state = END;
				break;
			}
			// fall through
		default:
			error(ERR_syntax_error);
			break;
		}
		break;

	case ERROR:
		// print error message and clean up
		if (t)
		{
			char buf[BUFSIZ];
			(void) sprintf(buf, "%*s",
				(t->pos() > BUFSIZ) ? BUFSIZ : t->pos(), "^");
			printe(ERR_cmd_pointer, E_NONE, buf);

			// a bad token may be the underlying cause of a syntax error
			// these are handled here instead of in the scanner since
			// tokens that are not part of the debugger's command
			// language may be valid in a source language dependent
			// expression
			switch (t->op()) {
			case BADTOKEN:	err = (int)ERR_bad_token;	break;
			case BADSTRING:	err = (int)ERR_bad_string;	break;
			case BADNUMBER:	err = (int)ERR_bad_number;	break;
			case BADCHAR:	err = (int)ERR_bad_character;	break;
			default:					break;
			}
		}

		switch (err) {
		case err_dashb_dashf:
			printe(ERR_option_mix, E_ERROR, "b", "f");
			break;
		case err_dashn_dashr:
			printe(ERR_option_mix, E_ERROR, "n", "r");
			break;
		case err_dashq_dashv:
			printe(ERR_option_mix, E_ERROR, "q", "v");
			break;
		case ERR_extra_curly:
		case ERR_active_event:
		case ERR_unfinished_cmd:
		case ERR_not_supported_platform:
			printe((Msg_id)err, E_ERROR);
			break;
		case ERR_bad_option:
			printe((Msg_id)err, E_ERROR, bad_option, t->print());
			break;
		default:
			printe((Msg_id)err, E_ERROR, t->print());
			break;
		}
		/* FALLTHROUGH */
	case NESTED_ERROR:
		delete cmd;
		cmd = 0;
		delete redir;
		redir = 0;
		++errors;

		// flush tokens til synch point
		while (t && state != BEGIN)
		{
			if ( t->op() == NL || t->op() == SEMI)
			{
				NEXT(t);
				if (cmd_list == ONE_ONLY)
					goto out;
				else
				{
					state = BEGIN;
					continue;
				}
			}
			else if (t->op() == RCURLY)
			{
				NEXT(t);
				goto out;
			}
			NEXT(t);
		}
		break;

	}	// end of switch
    }	// end of while

out:
	nexttok = t;
	if ( errors ) {
		delete root;
		n = 0;
		return ERROR;
	}
	n = prune(root);
	return state;
}

void
parse_and_execute(const char *input)
{
	Node		*root = 0;
	Token		*save_ptr = 0;
	Tokstack 	*p;

	scanner.scan(input);
	nexttok = scanner.token();
	p = new Tokstack(nexttok);
	tokstack.push(p);


	for (; nexttok;)
	{
		last_error = 0;
		parser( nexttok, ONE_ONLY, 0, root );
		save_ptr = nexttok;
		if (root)
			execute(root);
		else if (last_error)
			// root may be null because of an error 
			// or because of normal processing.  
			// Only update cmd_result
			// if an error occurred.
			cmd_result = last_error;
		if (save_ptr && save_ptr->op() == SEMI)
			nexttok = save_ptr->next();
	}
	p = tokstack.pop();
	delete p;
}
