/*
 *
 * $Id$
 *
 * Program:	Mailbox Context Management
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 *
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 *
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 */

#ifndef _CONTEXT_INCLUDED
#define _CONTEXT_INCLUDED

extern char *current_context;
typedef void  (*find_return) PROTO(());

/*
 * Prototypes
 */
void	    context_mailbox PROTO((char *));
void	    context_bboard PROTO((char *));
void	    context_fqn_to_an PROTO((find_return, char *, char *));
int	    context_isambig PROTO((char *));
char	   *context_digest PROTO((char *, char *, char *, char *, char *));
void	    context_apply PROTO((char *, char *, char *));
void	    context_find PROTO((char *, MAILSTREAM *, char *));
void	    context_find_bboards PROTO((char *, MAILSTREAM *, char *));
void	    context_find_all PROTO((char *, MAILSTREAM *, char *));
void	    context_find_all_bboard  PROTO((char *, MAILSTREAM *, char *));
long	    context_create PROTO((char *, MAILSTREAM *, char *));
MAILSTREAM *context_open PROTO((char *, MAILSTREAM *, char *, long));
long	    context_rename PROTO((char *, MAILSTREAM *, char *, char *));
long	    context_delete PROTO((char *, MAILSTREAM *, char *));
long	    context_append PROTO((char *, MAILSTREAM *, char *, STRING *));
long	    context_copy PROTO((char *, MAILSTREAM *, char *, char *));
long	    context_append_full PROTO((char *, MAILSTREAM *, char *, char *, \
				       char *, STRING *));
long	    context_subscribe PROTO((char *, MAILSTREAM *, char *));
long	    context_unsubscribe PROTO((char *, MAILSTREAM *, char *));
MAILSTREAM *context_same_stream PROTO((char *, char *, MAILSTREAM *));
MAILSTREAM *context_same_driver PROTO((char *, char *, MAILSTREAM *));
#ifdef NEWBB
void	    context_find_new_bboard PROTO((char *, MAILSTREAM *, char *, \
					   char *));
#endif

#endif /* _CONTEXT_INCLUDED */
