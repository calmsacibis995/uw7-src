#ident	"@(#)ksh93:src/lib/libast/include/proc.h	1.1"
#pragma prototyped

/*
 * process library interface
 */

#ifndef _PROC_H
#define _PROC_H

#include <ast.h>

#define PROC_ARGMOD	(1<<0)	/* argv[-1],argv[0] can be modified	*/
#define PROC_BACKGROUND	(1<<1)	/* shell background (&) setup		*/
#define PROC_CLEANUP	(1<<2)	/* close parent redirect fds on error	*/
#define PROC_DAEMON	(1<<3)	/* daemon setup				*/
#define PROC_ENVCLEAR	(1<<4)	/* clear environment			*/
#define PROC_GID	(1<<5)	/* setgid(getgid())			*/
#define PROC_IGNORE	(1<<6)	/* ignore parent pipe errors		*/
#define PROC_OVERLAY	(1<<7)	/* overlay current process if possible	*/
#define PROC_PARANOID	(1<<8)	/* restrict everything			*/
#define PROC_PRIVELEGED	(1<<9)	/* setuid(0), setgid(getegid())		*/
#define PROC_READ	(1<<10)	/* proc pipe fd 1 returned		*/
#define PROC_SESSION	(1<<11)	/* session leader			*/
#define PROC_UID	(1<<12)	/* setuid(getuid())			*/
#define PROC_WRITE	(1<<13)	/* proc pipe fd 0 returned		*/

#define PROC_ARG_BIT	14	/* bits per op arg			*/
#define PROC_OP_BIT	4	/* bits per op				*/

#define PROC_ARG_NULL	((1<<PROC_ARG_BIT)-1)

#define PROC_fd_dup	0x4
#define PROC_FD_CHILD	0x1
#define PROC_FD_PARENT	0x2

#define PROC_sig_dfl	0x8
#define PROC_sig_ign	0x9

#define PROC_sys_pgrp	0xa
#define PROC_sys_umask	0xb

#define PROC_op1(o,a)	(((o)<<(2*PROC_ARG_BIT))|((a)&((PROC_ARG_NULL<<PROC_ARG_BIT)|PROC_ARG_NULL)))
#define PROC_op2(o,a,b)	(((o)<<(2*PROC_ARG_BIT))|(((b)&PROC_ARG_NULL)<<PROC_ARG_BIT)|((a)&PROC_ARG_NULL))

#define PROC_FD_CLOSE(p,f)	PROC_op2(PROC_fd_dup|(f),p,PROC_ARG_NULL)
#define PROC_FD_DUP(p,c,f)	PROC_op2(PROC_fd_dup|(f),p,c)
#define PROC_SIG_DFL(s)		PROC_op1(PROC_sig_dfl,s,0)
#define PROC_SIG_IGN(s)		PROC_op1(PROC_sig_ign,s,0)
#define PROC_SYS_PGRP(g)	PROC_op1(PROC_sys_pgrp,g)
#define PROC_SYS_UMASK(m)	PROC_op1(PROC_sys_umask,m,0)

#define PROC_OP(x)	(((x)>>(2*PROC_ARG_BIT))&((1<<PROC_OP_BIT)-1))
#define PROC_ARG(x,n)	((n)?(((x)>>(((n)-1)*PROC_ARG_BIT))&PROC_ARG_NULL):(((x)&~((1<<(2*PROC_ARG_BIT))-1))==~((1<<(2*PROC_ARG_BIT))-1))?(-1):((x)&~((1<<(2*PROC_ARG_BIT))-1)))

typedef struct
{
	pid_t	pid;			/* process id			*/
	pid_t	pgrp;			/* process group id		*/
	int	rfd;			/* read fd if applicable	*/
	int	wfd;			/* write fd if applicable	*/

#ifdef _PROC_PRIVATE_
_PROC_PRIVATE_
#endif

} Proc_t;

extern int	procclose(Proc_t*);
extern int	procfree(Proc_t*);
extern Proc_t*	procopen(const char*, char**, char**, long*, long);
extern int	procrun(const char*, char**);

#endif
