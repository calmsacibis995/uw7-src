#ifndef _IO_STROPTS_F_H	/* wrapper symbol for kernel use */
#define _IO_STROPTS_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/stropts_f.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif


/* SCO XENIX Streams ioctl's */
#define X_STR			('S'<<8)
#define X_I_BASE		(X_STR|0200)
#define X_I_NREAD		(X_STR|0201)
#define X_I_PUSH		(X_STR|0202)
#define X_I_POP			(X_STR|0203)
#define X_I_LOOK		(X_STR|0204)
#define X_I_FLUSH		(X_STR|0205)
#define X_I_SRDOPT		(X_STR|0206)
#define X_I_GRDOPT		(X_STR|0207)
#define X_I_STR			(X_STR|0210)
#define X_I_SETSIG		(X_STR|0211)
#define X_I_GETSIG		(X_STR|0212)
#define X_I_FIND		(X_STR|0213)
#define X_I_LINK		(X_STR|0214)
#define X_I_UNLINK		(X_STR|0215)
#define X_I_PEEK		(X_STR|0217)
#define X_I_FDINSERT		(X_STR|0220)
#define X_I_SENDFD		(X_STR|0221)
#define X_I_RECVFD		(X_STR|0222)

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STROPTS_F_H */
