#ifndef NOIDENT
#pragma ident	"@(#)filter.c	15.1"
#endif

#include <stdio.h>
#include <string.h>

char *
FilterExecutable(char *name, char *line, char *format)
{
	char	*ptr, *ptr2, *path, *modeptr;
	int	n = 0;
/*
 *	isolate filename and mode on input lines that match the pkg name
 *	custom packages have the format:
 *
 *		PKGNAME<space>C+MODE<space>OWN/GRP<space>LINKS<space>PATH  ...
 *
 *	(C+MODE being char (d/f/x/...) followed by mode; I only look for 'x')
 *	with arbitrary whitespace between the fields (in general, tabs or
 *	spaces up to the next standard tab column)
 *
 *	and SVR4 packages are:
 *
 *		PATH C CLASS MODE ... PKGNAME[:CLASS] ...
 *
 *	(with some variants ignored here, including links symbolic and hard;
 *	since these paths are presented only as a fall back when there are no
 *	icons defined, it is not critical that we find all linked variants of
 *	executables, as long as each gets represented.)
 */
	if (*line=='#' || (ptr=strpbrk(line, " \t")) == NULL)
		return NULL;
	*ptr++ = '\0';
	/*
	 *	parse the line according to format (custom, or SVR4)
	 *	eliminate first those that are not even possibly executables
	 *	then, eliminate those that are not in the right package(s)
	 *	and finally, break out the mode and pathnames, and validate
	 *	executability by "other" -- return NULL if any test fails.
	 */
	if (format[0] == 'C') {
		if (strcmp(name, line) != 0)
			return NULL;
		while (isspace(*ptr))
			ptr++;
		if (*ptr != 'x')
			return NULL;
		modeptr = ++ptr;
		/*
		 *	find pathname field
		 */
		strtok(ptr, " \t\n");
		strtok(NULL," \t\n");
		strtok(NULL," \t\n");
		path = strtok(NULL," \t\n");
		if (*path == '.')
			++path;
	}
	else {
		if (*ptr != 'f' && *ptr != 'v')
			return NULL;
		if ((ptr2 = strstr(ptr, name)) == NULL)
			return NULL;
		else {
			--ptr2;
			if (!isspace(*ptr2) && *ptr2 != ':')
				return NULL;
			else {
				ptr2 += strlen(name)+1;
				if (*ptr2 != '\0' && *ptr2 != ':')
					return NULL;
			}
		}
		while (isspace(*++ptr))		/* step to class field */
			;
		while (!isspace(*++ptr))	/* and step through that */
			;
		modeptr = ++ptr;
		path = line;
	}
	while(isdigit(*modeptr)) {
		n = *modeptr-'0'+ n*8;
		modeptr++;
	}
	return (n & 1? path: NULL);
}

char *
FilterIcon (char *line)
{
	if (strncmp(line,"ICON=",5) != 0)
		return NULL;
	return (strchr(line,'\t'));	/* step past icon filepath */
}

