#include <stdio.h>
#include "./staticStub.h"

char *Prog;

static void
BailOut(
char *str,
int exit_val)
{
	fprintf(stderr, "%s: %s\n", Prog, str);
	exit(exit_val);
}

int
main(
int argc,
char *argv[])
{
	static char ch[] = StaticScreens;
	int i, len;

	Prog = argv[0];
	i = 0;
	len = strlen(ch);
	if (len <= 0)
		BailOut("define too short!", 1);
	while (i < len)
	{
		while (ch[i] == ' ' || ch[i] == '\t')
			++i;
		if (ch[i] == '-' && ch[i + 1] == 'D')
		{
			i += 2;
			printf("#define ");
			while (i < len && ch[i] != ' ' && ch[i] != '\t')
			{
				putchar(ch[i]);
				++i;
			}
			printf(" 1\n");
		}
		else
			++i;
	}

	return 0;
}
