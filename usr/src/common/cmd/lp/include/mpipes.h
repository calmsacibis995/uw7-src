/*		copyright	"%c%" 	*/


#ifndef	MPIPES_H
#define	MPIPES_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)mpipes.h	1.2"
#ident	"$Header$"

#ifdef	__STDC__

int	MountPipe (int *, char *);
int	UnmountPipe (int *, char *);

#else

int	MountPipe ();
int	UnmountPipe ();

#endif
/*==================================================================*/
#endif
