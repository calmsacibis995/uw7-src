#ident	"@(#)libmail.h	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)libmail.h	1.23 'attmail mail(1) command'"

#ifndef __LIBMAIL_H
# define __LIBMAIL_H

/*
 * Declarations for users of libmail.a
 */

#include "stdc.h"
#include "maillock.h"
#include "s_string.h"

/* argument to isyesno() */
typedef enum Default_Answer {
	Default_False = 0, Default_True = 1
} Default_Answer;

typedef enum		/* Content-Type: */
{
	C_Text,		/*     7-bit ASCII */
	C_GText,	/*     Printable in current locale: generic-text */
	C_Binary	/*     has non-printable characters */
} t_Content;

extern	string *abspath ARGS((const char *path, const char *dot, string *to));
extern	int bang_collapse ARGS((char *s));
extern	char *basename ARGS((const char *path));
extern	int cascmp ARGS((const char *s1, const char *s2));
extern	int casncmp ARGS((const char *s1, const char *s2, int n));
extern	void closeallfiles ARGS((int firstfd));
extern	int copystream ARGS((FILE *infp, FILE *outfp));
extern	int copynstream ARGS((FILE *infp, FILE *outfp, int numtocopy));
extern	int delempty ARGS((mode_t m, const char *mailname));
extern	long encode_file_to_base64 ARGS((FILE *in, FILE *out, int portablenewline));
extern	long encode_file_to_quoted_printable ARGS((FILE *in, FILE *out));
extern	long encode_string_to_quoted_printable ARGS((string *in, string *out, int qp_for_hdr));
extern	void errexit ARGS((int exitval, int sverrno, char *fmt, ...));
extern	int expand_argvec ARGS((char ***pargvec, int chunksize, char **_argvec, int *pargcnt));
extern	string *getmessageid ARGS((int add_angle_bracket));
extern	int islocal ARGS((const char *user, uid_t *puid));
extern	t_Content	istext ARGS((unsigned char *s, int size, t_Content cur));
extern	int isyesno ARGS((const char *str, Default_Answer def));
extern	string *long_to_string_format ARGS((long l, const char *format));
extern	string *long_to_string ARGS((long l));
extern	pid_t loopfork ARGS((void));
extern	char *maildomain ARGS((void));
extern	const char *mail_get_charset ARGS((void));
extern	char *mailsystem ARGS((int));
extern	char *Mgetenv ARGS((const char *env));
extern	char *mgetenv ARGS((const char *env));
extern	int newer ARGS((const char *file1, const char *file2));
extern	void notify ARGS((char *user, char *msg, int check_mesg_y, char *etcdir));
extern	int parse_execarg ARGS((char *p, int i, int *pargcnt, char ***argvec, int chunksize, char **_argvec));
extern	int pclosevp ARGS((FILE *fp));
extern	FILE *popenvp ARGS((char *file, char **argv, char *mode, int resetid));
extern	int posix_chown ARGS((const char *arg));
extern	char **setup_exec ARGS((char *s));
extern	const char *skipspace ARGS((const char *p));
extern	const char *skiptospace ARGS((const char *p));
extern	int sortafile ARGS((char *infile, char *outfile));
extern	const char *Strerror ARGS((int errno));
extern	void strmove ARGS((char *to, const char *from));
extern	int substr ARGS((const char *string1, const char *string2));
extern	int systemvp ARGS((const char *file, const char **argv, int resetid));
extern	void trimnl ARGS((char *s));
extern	char *Xgetenv ARGS((const char *env));
extern	char *xgetenv ARGS((const char *env));
extern	int xsetenv ARGS((char *file));
extern	FILE *xtmpfile ARGS((void));
extern  FILE    *Gettmpfile ARGS((int *fd));

/*
   The following pointer must be defined in the
   main program and given an appropriate value
*/
extern const char *progname;

#include "config.h"

#ifndef	MFMODE
# define	MFMODE		0660		/* create mode for `/var/mail' files */
#endif

/* versions of ctype.h macros that first guarantee that the character is unsigned */
#define Isalnum(x)  isalnum((unsigned char)(x))
#define Isalpha(x)  isalpha((unsigned char)(x))
#define Iscntrl(x)  iscntrl((unsigned char)(x))
#define Isdigit(x)  isdigit((unsigned char)(x))
#define Isxdigit(x) isxdigit((unsigned char)(x))
#define Isgraph(x)  isgraph((unsigned char)(x))
#define Islower(x)  islower((unsigned char)(x))
#define Isprint(x)  isprint((unsigned char)(x))
#define Isspace(x)  isspace((unsigned char)(x))
#define Isupper(x)  isupper((unsigned char)(x))

/* allocate a structure of the given size */
#define New(T) ((T*)malloc(sizeof(T)))

/* peek ahead to see what the next character is on the stream */
#define peekc(infp) (ungetc(getc(infp), infp))

/* how many elements are in an array? */
#define nelements(x) (sizeof(x) / sizeof(x[0]))

#endif /* __LIBMAIL_H */
