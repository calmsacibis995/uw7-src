#ident	"@(#)lprof:hdr/hidelibc.h	1.3"

#ifdef __STDC__

#define	access	_access
#define	chmod	_chmod
#define	chown	_chown
#define	close	_close
#define	creat	_creat
#define	getpid	_getpid
#define	link	_link
#define	open	_open
#define	read	_read
#define	tempnam	_tempnam
#define	unlink	_unlink
#define	write	_write

#endif
