#ident	"@(#)fsba:i386/cmd/fsba/fsba.h	1.1"
#ident	"$Header$"

#define DEF_BLOCKSIZE	1024
#define	SECTSIZE	512	/* size of sector (physical block) */
#define	SECTPERLOG(b)	(b/SECTSIZE)
