/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)s_string.h	11.1"
#ident	"@(#)s_string.h	11.1"
/* extensible strings */

#ifndef _string_h
#define _string_h
#include <string.h>

typedef struct string {
	char *base;	/* base of string */
	char *end;	/* end of allocated space+1 */
	char *ptr;	/* ptr into string */
} string;

#ifdef lint
# ifdef __STDC__
extern string *s_clone(string*);
extern int s_curlen(string*);
extern void s_delete(string*);
extern string *s_dup(string*);
extern int s_getc(string*);
extern int s_peek(string*);
extern int s_putc(string*, int);
extern string *s_reset(string*);
extern string *s_restart(string*);
extern void s_skipback(string*);
extern void s_skipc(string*);
extern void s_terminate(string*);
extern void s_truncate(string*, int);
extern char *s_to_c(string*);
extern char *s_ptr_to_c(string*);
# else
extern string *s_clone();
extern int s_curlen();
extern void s_delete();
extern string *s_dup();
extern int s_getc();
extern int s_peek();
extern int s_putc();
extern string *s_reset();
extern string *s_restart();
extern void s_skipback();
extern void s_skipc();
extern void s_terminate();
extern void s_truncate();
extern char *s_to_c();
extern char *s_ptr_to_c();
# endif
#else
# define s_clone(s)	s_copy((s)->ptr)
# define s_curlen(s)	((s)->ptr - (s)->base)
# define s_delete(s)	(s_free(s), (s) = 0)
# define s_dup(s)	s_copy_reserve((s)->base, s_curlen(s)+1, s_curlen(s)+1)
# define s_getc(s)	(*((s)->ptr++))
# define s_peek(s)	(*((s)->ptr))
# define s_putc(s,c)	(((s)->ptr<(s)->end) ? (*((s)->ptr)++ = (c)) : s_grow((s),(c)))
# define s_reset(s)	((s) ? (*((s)->ptr = (s)->base) = '\0' , (s)) : s_new())
# define s_restart(s)	((s)->ptr = (s)->base , (s))
# define s_skipback(s)	((s)->ptr--)
# define s_skipc(s)	((s)->ptr++)
# define s_space(s)	((s)->end - (s)->base)
# define s_terminate(s)	(((s)->ptr<(s)->end) ? (*(s)->ptr = '\0') : (s_grow((s),0), (s)->ptr--, 0))
# define s_truncate(s,i)	((void)((s)->base[i] = '\0'))
# define s_to_c(s)	((s)->base)
# define s_ptr_to_c(s)	((s)->ptr)
#endif /* lint */

#ifdef __STDC__
extern string *s_append(string *to, const char *from);
extern string *s_array(char *, int len);
extern string *s_copy(const char *);
extern string *s_copy_reserve(const char *, int slen, int alen);
extern string *s_etok(string*,char*);
extern string *s_etokc(string*,int);
extern void s_free(string*);
extern int s_grow(string *sp, int c);
extern string *s_nappend(string *to, const char *from, int n);
extern string *s_new(void);
extern string *s_parse(string *from, string *to);
extern char *s_read_line(FILE *fp, string *to);
extern int s_read_to_eof(FILE *fp, string *to);
extern string *s_seq_read(FILE *fp, string *to, int lineortoken);
extern void s_skiptoend(string*);
extern void s_skipwhite(string *from);
extern string *s_tok(string*,char*);
extern string *s_tokc(string*,int);
extern void s_tolower(string*);
extern void s_write(string *line, FILE *outfp);
extern string *s_xappend(string *to, const char *from1, const char *from2, ...);
#else
extern string *s_append();
extern string *s_array();
extern string *s_copy();
extern string *s_copy_reserve();
extern string *s_etok();
extern string *s_etokc();
extern void s_free();
extern int s_grow();
extern string *s_nappend();
extern string *s_new();
extern string *s_parse();
extern char *s_read_line();
extern int s_read_to_eof();
extern string *s_seq_read();
extern void s_skiptoend();
extern void s_skipwhite();
extern string *s_tok();
extern string *s_tokc();
extern void s_tolower();
extern void s_write();
extern string *s_xappend();
#endif

/* controlling the action of s_seq_read */
#define TOKEN	0	/* read the next whitespace delimited token */
#define LINE	1	/* read the next logical input line */
#define s_getline(fp,b)		s_seq_read(fp,b,LINE)
#define s_gettoken(fp,b)	s_seq_read(fp,b,TOKEN)

#endif
