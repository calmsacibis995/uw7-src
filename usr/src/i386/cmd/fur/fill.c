#ident	"@(#)fur:i386/cmd/fur/fill.c	1.1"

void
#ifdef __STDC__
filltext(char *start, char *end)
#else
filltext(start, end)
char *start;
char *end;
#endif
{
	while (start != end) {
		*(start) = 0x90; /* NOP */
		start++;
	}
}

