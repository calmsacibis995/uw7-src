
mfbClipLine(){}

#if defined(gemini)
ScaleHideMessage(){}
SetSWSequence(){}
ScalePopMessage(){}
ReadInputEvents(){}
XTestGenerateEvent(){}
LoadRuntimeExtensions(){}

#include <ctype.h>

/*
 * A portable hack at implementing strcasecmp()
 */
int strcasecmp(s1, s2)
char *s1, *s2;
{
	char c1, c2;

	if (*s1 == 0)
		if (*s2 == 0)
			return(0);
		else
			return(1);

	c1 = (isupper(*s1) ? tolower(*s1) : *s1);
	c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	while (c1 == c2)
	{
		if (c1 == '\0')
			return(0);
		s1++; s2++;
		c1 = (isupper(*s1) ? tolower(*s1) : *s1);
		c2 = (isupper(*s2) ? tolower(*s2) : *s2);
	}
	return(c1 - c2);
}

char *index(const char *s, int c) {
	extern char *strchr(const char *s, int c);
	return strchr( s, c );
}

#endif	/*	 gemini	*/
