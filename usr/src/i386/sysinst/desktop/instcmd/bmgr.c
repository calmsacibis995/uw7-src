#ident	"@(#)bmgr.c	15.1"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

/*
 * The name of this command, bmgr, stands for "bit-map generator."  The input
 * file contains the bit maps to be used. They are groups of 14 or 16 lines,
 * separated by a blank line, with 8 0's or 1's that define the glyph.
 */

int
main(int argc, char **argv)
{
	register int i;
	register int j;
	int ofd;
	int bct = 0;

	char ofile[40];
	char buffer[200];
	char line[80];

	FILE *ifp;
	
	if (argc < 2)
	{
		printf("Usage: bmgr <input_file>\n");
		exit(1);
	}

	if ((ifp = fopen(argv[1], "r")) == NULL)
	{
		printf("Unable to open input file\n");
		exit(1);
	}

	sprintf(ofile, "%s.bm", argv[1]);

	if ((ofd = open(ofile, O_WRONLY | O_CREAT, 0666)) < 0)
	{
		printf("Unable to open file for output\n");
		exit(1);
	}

	for (i = 0; i < 11; i++)
	{
#ifdef DEBUG
printf("Character %d (bct = %d)\n", i, bct);
#endif
		while (fgets(line, 80, ifp) != NULL)
		{
			if (line[0] == '\n')
				break;
			
#ifdef DEBUG
printf("Processing %s", line);
#endif
			buffer[bct] = 0;

			for (j = 0; j < 8; j++)
			{
				if (line[j] == '1')
					buffer[bct] |= (1 << (7-j));
			}
#ifdef DEBUG
printf("buffer = %X\n", buffer[bct]);
#endif

			++bct;
		}
	}

	if (write(ofd, buffer, bct) < bct - 1)
		printf("failed to write all of buffer\n");

	close(ofd);
}
