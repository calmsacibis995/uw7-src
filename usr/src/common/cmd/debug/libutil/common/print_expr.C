/*
 * $Copyright: $
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

#ident	"@(#)debugger:libutil/common/print_expr.C	1.19"

#include "Proglist.h"
#include "Parser.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Interface.h"
#include "Buffer.h"
#include "global.h"
#include "NewHandle.h"
#include <signal.h>
#include <stdio.h>

class Format_string
//
// Scans and prints printf style string.
// The format string may consist of any string with embedded
// printf-style formats; each format has the following form:
// %[0|-|+|#|space][width][.[precision]][l|h|L][specifier]
// width and precision are decimal numbers < PRINT_SIZE
// specifier may be one of:d,i,o,u,x,X,f,e,E,g,G,c,s,p,%
// the specifier ends each format

// If _USL_PRINTF is defined, we also recognize a,A,b,B,C,S as
// specifiers.
//
{
private:
#ifdef __cplusplus
	static char	*default_format;
#endif
	char	*format;
	char	*formbuf;	// used to do sprintfs
	char	*next_char_to_print;
	char	*next_specifier;
	char	*next_format;
	char	*first_specifier;
	char	*first_format;

	int	Find_format();
	void	get_flags();
	int	get_decimal();
	void	get_conversion();
	int	get_specifier();
public:

		Format_string(char *format);

	int	First_format();
	char	Next_specifier(void) { return *next_specifier; }
	char	*Next_format(void);
	int	Print_to_next_format(Buffer *);
	void	Complete(Buffer *);
};

#ifdef __cplusplus
char	*Format_string::default_format = "%? ";
#else
static char	*default_format = "%? ";
#endif

// local new handler
static void
print_memory_handler()
{
	// restore old handler before printing message
	// in case it generates another memory error
	newhandler.restore_old_handler();
	printe(ERR_memory_allocation, E_ERROR);
	newhandler.return_to_saved_env(1);
}

int
print_expr(Proclist *procl, char *fmt, Exp *exp, int verbose, int brief)
//
// foreach proc do
//    first format, first expr
//    while format and expr do
//       eval expr (handle error)
//       print expr with format
//       next expr, next format
//    end
//    if format or expr then
//       error
//    end
// end
//
{
	plist	*list;
	plist	*list_head = 0;
	ProcObj  	*pobj;
	int	ret = 1;
	Buffer	*buf = buf_pool.get();
	Process	*proc = 0;
	
	if (procl)
	{
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		list = 0;
		pobj = proglist.current_object();
	}

	int multiple = list && list->p_pobj;

	newhandler.install_user_handler(print_memory_handler);
	if (setjmp(*newhandler.get_jmp_buf()) != 0)
	{
		// longjmp returns here
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do // execute once even with no pobj at all
	{
		buf->clear();
		if (multiple)
			printm(MSG_print_header, pobj->obj_name());
		Format_string format(fmt);
		if (!format.First_format())
		{
			ret = 0;
			break;
		}
		char          specifier = format.Next_specifier();
		char	      *format_str = format.Next_format();
		int           exp_index = 0;

		proc = 0;
		if (list)
		{
			// we only stop all if we have a thread -
			// the lower-level print routines catch whether
			// the current object needs to be stopped - for
			// threads we must also have entire process stopped
			if (pobj && (pobj->obj_type() == pobj_thread))
			{
				proc = pobj->process();
				if (! proc->stop_all())
				{
					ret = 0;
					continue;
				}
			}
		}
		while (specifier && (*exp)[exp_index])
		{
	    		// Evaluating the expressions may change 
			// current process. "print %proc="p2", %program
	    		if (!list) 
			{
				Process	*nproc;
				pobj = proglist.current_object();
				if (pobj && pobj->obj_type() == pobj_thread)
				{
					nproc = pobj->process();
					if (nproc != proc)
					{
						if ((proc &&
						!proc->restart_all())
						||!nproc->stop_all())
						{
							ret = 0;
							proc = 0;
							goto out;
						}
						proc = nproc;
					}
				}
			}

			// for C++, use_derived_type will change an expression of the
			// form p *base_class to p *derived_class, if applicable

			Expr	*expr = new_expr((*exp)[exp_index], pobj);
			Rvalue	*rvalue;
			if (expr->eval(pobj, ~0, 0, verbose)
				&& (expr->use_derived_type(), expr->rvalue(rvalue)))
			{
				// Print here because of messages from eval 
				// (especially function call)
				if (!format.Print_to_next_format(buf))
				{
					ret = 0;
					delete expr;
					goto out;
				}
				if (rvalue->isnull())
					printe(ERR_internal, E_ERROR, 
						"print_expr", __LINE__);
				else
				{
					ProcObj	*context = list ? pobj:
						proglist.current_object();

					if (brief)
						rvalue->print(buf, 
							context,
							DEBUGGER_BRIEF_FORMAT);
					else
						rvalue->print(buf,
							context,
							specifier, format_str, verbose);
				}
			}
			else
			{
				format.Print_to_next_format(buf);
				delete expr;
				ret = 0;
				goto loop;
			}

			delete expr;
			exp_index++;
			specifier = format.Next_specifier();
			format_str = format.Next_format();
		}
loop:
		format.Complete(buf);
		if (buf->size() != 0)
		{
			printm(MSG_print_val, (char *)*buf);
		}
		if (proc)
		{
			if (!proc->restart_all())
				ret = 0;
			proc = 0;
		}
		if (list)
			pobj = list++->p_pobj;
		else
			pobj = 0;
	} while (pobj && !prismember(&interrupt, SIGINT));
out:
	newhandler.restore_old_handler();
	if (proc)
	{
		// bombed out of middle of loop
		if (!proc->restart_all())
			ret = 0;
	}
	sighold(SIGINT);
	buf_pool.put(buf);
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}

Format_string::Format_string(char * format)
{
	if (format && *format) 
	{
		this->format = format;
	}
	else
	{
		default_format[1] = DEBUGGER_FORMAT;
		this->format = default_format;
	}
	formbuf = new char[strlen(this->format) + 1];
	next_char_to_print = this->format;
}

// invariants:
// next_specifier != 0
// first_specifier != 0
// next_format != 0
// first_format != 0
// next_char_to_print != 0
// *first_format == 0 or '%'
// *next_format == 0 or '%'
// next_format >= first_format
// next_specifier >= first_specifier
int
Format_string::First_format()
{
	if (!Find_format())
	{
		printe(ERR_print_format, E_ERROR, format);
		return 0;
	}
	first_specifier = next_specifier;
	first_format = next_format;
	return 1;
}

// find beginning of next format and the format specifier itself
int 
Format_string::Find_format()
{
	next_format = next_char_to_print;
	while(*next_format &&
		(*next_format != '%' || next_format[1] == '%'))
	{
		if (*next_format == '%')
			next_format += 2; // "%%"
		else
			next_format++;
	}
	if (!*next_format)
	{
		next_specifier = next_format;
		return 1;
	}
	next_specifier = next_format + 1;
	// parse format string
	if (!*next_specifier)
		return 0;
	get_flags();
	if (!*next_specifier)
		return 0;
	if (!get_decimal()) // width
		return 0;
	if (!*next_specifier)
		return 0;
	if (*next_specifier == '.')
		next_specifier++;
	if (!*next_specifier)
		return 0;
	if (!get_decimal())  // precision
		return 0;
	if (!*next_specifier)
		return 0;
	get_conversion();
	if (!*next_specifier)
		return 0;
	return(get_specifier());
}

void
Format_string::get_flags()
{
	switch(*next_specifier)
	{
	case '0':
	case '-':
	case '+':
	case '#':
	case ' ':
		next_specifier++;
		break;
	default:
		break;
	}
	return;
}

void
Format_string::get_conversion()
{
	switch(*next_specifier)
	{
	case 'l':
	case 'h':
	case 'L':
		next_specifier++;
		break;
	default:
		break;
	}
	return;
}

int 
Format_string::get_specifier()
{
	switch(*next_specifier)
	{
#ifdef _USL_PRINTF
	case 'a':
	case 'A':
	case 'b':
	case 'B':
	case 'C':
	case 'S':
#endif
	case 'E':
	case 'G':
	case 'X':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'i':
	case 'o':
	case 'p':
	case 's':
	case 'u':
	case 'x':
	case 'z':
		return 1;
	default:
		return 0;
	}
}

int
Format_string::get_decimal()
{
	int	val = 0;
	int	place = 1;

	if (*next_specifier == '0')
	{
		next_specifier++;
		return 1;
	}
	while((*next_specifier >= '0') && (*next_specifier <= '9'))
	{
		val += place * (*next_specifier - '0');
		place *= 10;
		next_specifier++;
	}
	if (val > PRINT_SIZE)
	{
		printe(ERR_print_width, E_ERROR, val);
		return 0;
	}
	return 1;
}

char *
Format_string::Next_format()
{
	int		len;
	static char	fbuf[14]; // sizeof("%-1024.1024ld")
	len = next_specifier - next_format + 1;
	if (len > 13)
	{
		printe(ERR_internal, E_ERROR, "Format_string::Next_format", __LINE__);
		len = 13;
	}
	strncpy(fbuf, next_format, len);
	fbuf[len] = 0;
	return fbuf;
}

// Print format string up to next beginning of next complete format
// and advance to following format.
// Assumes next_specifier is not null.
int 
Format_string::Print_to_next_format(Buffer *buf)
{
	if (next_char_to_print > next_format)
	{
		// Beyond last format
		// Print end of string before repeating beginning.
		if (*next_char_to_print)
		{
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			buf->add(formbuf);
		}
		next_char_to_print = format;
	}

	if (next_char_to_print != next_format)
	{
		*next_format = 0;
		// make sure we expand %% to %
		sprintf(formbuf, next_char_to_print);
		buf->add(formbuf);
		*next_format = '%';
	}
   	// Set up for next print.
	next_char_to_print = next_specifier+1; // specifier ends format

	// Set up for next format and specifier.
	if (!Find_format())
	{
		delete formbuf;
		formbuf = 0;
		printe(ERR_print_format, E_ERROR, format);
		return 0;
	}

	// Repeat format string if it has any specifier
	if (!*next_format)
	{
		next_format = first_format;
		next_specifier = first_specifier;
	}
	return 1;
}

void
Format_string::Complete(Buffer *buf)
{
	if (format == default_format)
		buf->add('\n');
	else if (next_char_to_print != next_format && 
		*next_char_to_print)
	// Print characters to next format or end
	{
		if (*next_format)
		{
			*next_format = 0;
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			buf->add(formbuf);
			*next_format = '%';
		}
		else
		{
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			buf->add(formbuf);
		}
	}
	delete formbuf;
	formbuf = 0;
}
