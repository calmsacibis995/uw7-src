#ident	"@(#)ksh93:src/lib/libcmd/wclib.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * library interface for word count
 */

#include <cmdlib.h>
#include <wc.h>
#include <ctype.h>

#define endline(c)	(((signed char)-1)<0?(c)<0:(c)==((char)-1))

Wc_t *wc_init(char *space)
{
	register int  n;
	Wc_t *wp;

	if(!(wp = (Wc_t*)stakalloc(sizeof(Wc_t))))
		return(0);
	if(space)
		memcpy(wp->space, space, (1<<CHAR_BIT));
	else
	{
		for(n=(1<<CHAR_BIT);--n >=0;)
			wp->space[n] = (isspace(n)!=0);
		wp->space['\n'] = -1;
	}
	return(wp);
}

/*
 * compute the line, word, and character count for file <fd>
 */
wc_count(Wc_t *wp, Sfio_t *fd)
{
	register signed char	*space = wp->space;
	register unsigned char	*cp;
	register long		nwords;
	register int		c;
	register unsigned char	*endbuff;
	register int		lasttype = 1;
	unsigned int		lastchar;
	unsigned char		*buff;
	wp->lines = nwords = wp->chars = 0;
	sfset(fd,SF_WRITE,1);
	while(1)
	{
		/* fill next buffer and check for end-of-file */
		if (!(buff = (unsigned char*)sfreserve(fd, 0, 0)) || (c = sfslen()) <= 0)
			break;
		sfread(fd,(char*)(cp=buff),c);
		wp->chars += c;
		/* check to see whether first character terminates word */
		if(!lasttype && space[*cp])
			nwords++;
		lastchar = cp[--c];
		cp[c] = '\n';
		endbuff = cp+c;
		c = lasttype;
		/* process each buffer */
		while(1)
		{
			/* process spaces and new-lines */
			do
			{
				if(endline(c))
				{
					/* check for end of buffer */
					if(cp>endbuff)
						goto eob;
					wp->lines++;
				}
			}
			while(c=space[*cp++]);
			/* skip over word characters */
			while(!(c=space[*cp++]));
			nwords++;
		}
	eob:
		if((cp -= 2) >= buff)
			c = space[*cp];
		else
			c  = lasttype;
		lasttype = space[lastchar];
		/* see if was in word */
		if(!c && !lasttype)
			nwords--;
	}
	wp->words = nwords;
	if(endline(lasttype))
		wp->lines++;
	else if(!lasttype)
		wp->words++;
	return(0);
}
