#ident	"@(#)pcintf:pkg_lmf/lmf.h	1.2"
/* SCCSID(@(#)lmf.h	7.4	LCC)	/* Modified: 15:07:05 2/11/91 */

/*
 *  Include file for the LMF (message file) library
 */

extern int lmf_message_length;
extern int lmf_errno;

/*
 *  Errors returned by lmf_push_domain()
 */

#define LMF_ERR_NOMEM		(-1)		/* No memory available */
#define LMF_ERR_NOFILE		(-2)		/* File not found */
#define LMF_ERR_BADFILE		(-3)		/* Bad file format */
#define LMF_ERR_NOMSGFILE	(-4)		/* No message file open */
#define LMF_ERR_NOTFOUND	(-5)		/* Domain/Message not found */
#define LMF_ERR_NOSPACE		(-6)		/* No space left in buffer */


#ifndef LMF_NO_EXTERNS
#if defined(__STDC__)
char *lmf_format_string(char *, int, char *, char *, ...);
#else
char *lmf_format_string();
#endif
#endif


/*
 *  Defines for default message installation/removal
 */

#ifdef LMF_NO_MESSAGE_FILE
#define lmf_open_file(a,b,c)	(0)
#define lmf_close_file(a)	(0)
#define lmf_push_domain(a)	(0)
#define lmf_pop_domain(a)
#define lmf_fast_domain(a)
#define lmf_get_message(a,b)	(b)
#define lmf_get_message_copy(a,b)	(b)
#endif

#ifdef LMF_NO_MESSAGE_DEFAULTS
#define lmf_get_message(a,b)	lmf_get_message_internal((a), 0)
#define lmf_get_message_copy(a,b)	lmf_get_message_internal((a), 1)
char *lmf_get_message_internal();
#endif

#ifndef lmf_open_file
#if defined(__STDC__)
int	lmf_open_file(char *, char *, char *);
int	lmf_close_file(int);
int	lmf_push_domain(char *);
void	lmf_pop_domain(void);
int	lmf_fast_domain(char *);
#else
int	lmf_open_file();
int	lmf_close_file();
int	lmf_push_domain();
void	lmf_pop_domain();
int	lmf_fast_domain();
#endif
#endif

#ifndef lmf_get_message
#if defined(__STDC__)
char *lmf_get_message(char *, char *);
char *lmf_get_message_copy(char *, char *);
#else
char *lmf_get_message();
char *lmf_get_message_copy();
#endif
#endif

#ifndef NULL
#if defined(MSDOS) && (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
#define NULL	0L
#else
#define NULL	0
#endif
#endif
