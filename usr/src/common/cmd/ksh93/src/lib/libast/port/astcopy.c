#ident	"@(#)ksh93:src/lib/libast/port/astcopy.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * copy from rfd to wfd (with conditional mmap hacks)
 */

#include <ast.h>

#if __sun__ || sun

#if _lib_mmap && (_hdr_mman || _sys_mman)

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide mmap munmap
#else
#define mmap		______mmap
#define munmap		______munmap
#endif

#include <ls.h>
#if _hdr_mman
#include <mman.h>
#else
#include <sys/mman.h>
#endif

#define MAPSIZE		(1024*256)

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide mmap munmap
#else
#undef	mmap
#undef	munmap
#endif

extern int		munmap(caddr_t, size_t);
extern caddr_t		mmap(caddr_t, size_t, int, int, int, off_t);

#endif

#endif

#define BUFSIZ		4096

/*
 * copy n bytes from rfd to wfd
 * actual byte count returned
 * if n<=0 then ``good'' size is used
 */

off_t
astcopy(int rfd, int wfd, off_t n)
{
	register off_t	c;
#ifdef MAPSIZE
	off_t		pos;
	off_t		mapsize;
	char*		mapbuf;
	struct stat	st;
#endif

	static int	bufsiz;
	static char*	buf;

	if (n <= 0 || n >= BUFSIZ * 2)
	{
#if MAPSIZE
		if (!fstat(rfd, &st) && S_ISREG(st.st_mode) && (pos = lseek(rfd, (off_t)0, 1)) != ((off_t)-1))
		{
			if (pos >= st.st_size) return(0);
			mapsize = st.st_size - pos;
			if (mapsize > MAPSIZE) mapsize = (mapsize > n && n > 0) ? n : MAPSIZE;
			if (mapsize >= BUFSIZ * 2 && (mapbuf = (char*)mmap(NiL, mapsize, PROT_READ, MAP_SHARED, rfd, pos)) != ((caddr_t)-1))
			{
				if (write(wfd, mapbuf, mapsize) != mapsize || lseek(rfd, mapsize, 1) == ((off_t)-1)) return(-1);
				munmap((caddr_t)mapbuf, mapsize);
				return(mapsize);
			}
		}
#endif
		if (n <= 0) n = BUFSIZ;
	}
	if (n > bufsiz)
	{
		if (buf) free(buf);
		bufsiz = roundof(n, BUFSIZ);
		if (!(buf = newof(0, char, bufsiz, 0))) return(-1);
	}
	if ((c = read(rfd, buf, (size_t)n)) > 0 && write(wfd, buf, (size_t)c) != c) c = -1;
	return(c);
}
