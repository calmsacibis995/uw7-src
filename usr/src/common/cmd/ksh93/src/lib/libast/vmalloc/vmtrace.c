#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmtrace.c	1.1"
#include	"vmhdr.h"

/*	Turn on tracing for regions
**
**	Written by (Kiem-)Phong Vo, kpv@research.att.com, 01/16/94.
*/

static int	Trfile = -1;
static char	Trbuf[128];

#if __STD_C
static char* trstrcpy(char* to, char* from, int endc)
#else
static char* trstrcpy(to, from, endc)
char*	to;
char*	from;
int	endc;
#endif
{	reg int	n;

	n = strlen(from);
	memcpy(to,from,n);
	to += n;
	if((*to = endc) )
		to += 1;
	return to;
}

/* convert a long value to an ascii representation */
#if __STD_C
static char* tritoa(ulong v, int type)
#else
static char* tritoa(v, type)
ulong	v;	/* value to convert					*/
int	type;	/* =0 base-16, >0: unsigned base-10, <0: signed base-10	*/
#endif
{
	char*	s;

	s = &Trbuf[sizeof(Trbuf) - 1];
	*s-- = '\0';

	if(type == 0)		/* base-16 */
	{	reg char*	digit = "0123456789abcdef";
		do
		{	*s-- = digit[v&0xf];
			v >>= 4;
		} while(v);
		*s-- = 'x';
		*s-- = '0';
	}
	else if(type > 0)	/* unsigned base-10 */
	{	do
		{	*s-- = (char)('0' + (v%10));
			v /= 10;
		} while(v);
	}
	else			/* signed base-10 */
	{	int	sign = ((long)v < 0);
		if(sign)
			v = (ulong)(-((long)v));
		do
		{	*s-- = (char)('0' + (v%10));
			v /= 10;
		} while(v);
		if(sign)
			*s-- = '-';
	}

	return s+1;
}

/* generate a trace of some call */
#if __STD_C
static void trtrace(Vmalloc_t* vm, uchar* oldaddr, uchar* newaddr, size_t size )
#else
static void trtrace(vm, oldaddr, newaddr, size)
Vmalloc_t*	vm;		/* region call was made from	*/
uchar*		oldaddr;	/* old data address		*/
uchar*		newaddr;	/* new data address		*/
size_t		size;		/* size of piece		*/
#endif
{
	char		buf[1024], *bufp, *endbuf;
	reg Vmdata_t*	vd = vm->data;
	reg char*	file;
	reg int		line;
#define SLOP	32

	VMFILELINE(file,line);

	if(Trfile < 0)
		return;

	bufp = buf; endbuf = buf+sizeof(buf);
	bufp = trstrcpy(bufp, tritoa(oldaddr ? ULONG(oldaddr) : 0L, 0), ':');
	bufp = trstrcpy(bufp, tritoa(newaddr ? ULONG(newaddr) : 0L, 0), ':');
	bufp = trstrcpy(bufp, tritoa((ulong)size, 1), ':');
	bufp = trstrcpy(bufp, tritoa(ULONG(vm), 0), ':');
	if(vd->mode&VM_MTBEST)
		bufp = trstrcpy(bufp, "best", ':');
	else if(vd->mode&VM_MTLAST)
		bufp = trstrcpy(bufp, "last", ':');
	else if(vd->mode&VM_MTPOOL)
		bufp = trstrcpy(bufp, "pool", ':');
	else if(vd->mode&VM_MTPROFILE)
		bufp = trstrcpy(bufp, "profile", ':');
	else	bufp = trstrcpy(bufp, "debug", ':');
	if(file && file[0] && line > 0 && (bufp + strlen(file) + SLOP) < endbuf)
	{	bufp = trstrcpy(bufp, file, ',');
		bufp = trstrcpy(bufp, tritoa((ulong)line,1), ':');
	}
	*bufp++ = '\n';
	*bufp = '\0';

	write(Trfile,buf,(bufp-buf));
}

#if __STD_C
vmtrace(int file)
#else
vmtrace(file)
int	file;
#endif
{
	int	fd;

	_Vmstrcpy = trstrcpy;
	_Vmitoa = tritoa;
	_Vmtrace = trtrace;

	fd = Trfile;
	Trfile = file;
	return fd;
}
