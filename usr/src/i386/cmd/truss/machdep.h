#ident	"@(#)truss:i386/cmd/truss/machdep.h	1.1.2.1"
#ident	"$Header$"

#define R_1	9	/* EDX */
#define R_0	11	/* EAX */
#define	R_PC	14	/* EIP */
#define	R_PS	16	/* EFL */
#define	R_SP	17	/* UESP */

#define SYSCALL_OFF	7

typedef	ulong_t	syscall_t;
#define	SYSCALL	(ulong_t)0x9a
#define	ERRBIT	0x1

#define PRT_SYS	prt_si86

#if	defined(__STDC__)
extern	CONST char *	si86name( int );
extern	int	prt_si86( int , int );
#else	/* defined(__STDC__) */
extern	CONST char *	si86name();
extern	int	prt_si86();
#endif
