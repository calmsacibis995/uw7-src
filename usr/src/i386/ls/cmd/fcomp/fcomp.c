#ident	"@(#)fcomp.c	1.2"
#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>

#define COMMA ','
#define TRUE (1 == 1)
#define FALSE (1 == 0)
#define BACK_SLASH '\\'
#define EVER ;;
#define MAX_VAL 255
#define LINE_LEN 80
#define MAX_SYM_LEN 20
#define UNDER_SCORE '_'
#define COMMENT '#'
#define RETURN '\n'
#define ERROR 1
#define BINARY 0
#define OCTAL 1
#define HEX 2
#define MACHINE 3
#define DISPLAY 4

int num_bytes[3] = {8, 14, 16};

main(argc, argv)
int argc;
char *argv[];
{
	register int i;		/*  Loop variable  */
	register int j;
	int bit_pattern;	/*  Bit pattern read from file  */
	int byte_num;		/*  Number of byte in character  */
	int first;			/*  Is this the first flag  */
	int fn;				/*  Font size number  */
	int line_num;		/*  Line number in input file  */
	int lpos;			/*  Position in line  */
	int op_mode;		/*  Output mode  */
	int space;			/*  Space in name  flag  */

	char inp_line[LINE_LEN];	/*  Line read from input file  */
	char old_name[LINE_LEN];	/*  Name of last character  */
	char tmp_str[LINE_LEN];		/*  Temporary string  */

	FILE *ifp;			/*  Input file pointer  */
	FILE *ofp;			/*  Output file pointer  */

	setlocale(LC_ALL, "");
	setcat("uxels");
	setlabel("UX:fcomp");

	/*  Check that there are enough arguments  */
	if (argc < 5 || argv[1][0] != '-')
	{
		pfmt(stderr, MM_ERROR, ":5:Usage: fcomp [-b -o -h -m -d] <font_size> <input_file> <output_file>\n");
		exit(ERROR);
	}

	/*  Check first argument, which specifies the output mode  */
	switch (argv[1][1]) {
		case 'b':
			op_mode = BINARY;
			break;

		case 'o':
			op_mode = OCTAL;
			break;

		case 'h':
			op_mode = HEX;
			break;

		case 'm':
			op_mode = MACHINE;
			break;

		case 'd':
			op_mode = DISPLAY;
			break;

		default:
			pfmt(stderr, MM_ERROR, ":5:Usage: fcomp [-b -o -h -m -d] <font_size> <input_file> <output_file>\n");
			exit(ERROR);
	}

	/*  Determine the size for this file  */
	if (strcmp(argv[2], "8x8") == 0)
		fn = 0;
	else if (strcmp(argv[2], "8x14") == 0)
		fn = 1;
	else if (strcmp(argv[2], "8x16") == 0 || strcmp(argv[2], "9x16") == 0)
		fn = 2;
	else
	{
		pfmt(stderr, MM_ERROR, ":7:fcomp: Invalid font size <%s>\n", argv[2]);
		exit(ERROR);
	}

	/*  Check that the names are different  */
	if (strcmp(argv[3], argv[4]) == 0)
	{
		pfmt(stderr, MM_ERROR, ":8:fcomp: input and output files must have different names\n");
		exit(ERROR);
	}

	/*  Open the input file for reading  */
	if ((ifp = fopen(argv[3], "r")) == NULL)
	{
		pfmt(stderr, MM_ERROR, ":9:fcomp: Unable to open '%s' for reading\n", argv[3]);
		exit(ERROR);
	}

	/*  Open the output file for writing  */
	if ((ofp = fopen(argv[4], "w")) == NULL)
	{
		pfmt(stderr, MM_ERROR, ":10:fcomp: Unable to open '%s' for writing\n", argv[4]);
		exit(ERROR);
	}

	line_num = 0;		/*  Initialise various varying variables  */
	byte_num = 0;
	first = TRUE;

	/*  Process each line of the file  */
	while (fgets(inp_line, LINE_LEN - 1, ifp) != NULL)
	{
		/*  Increment the line count  */
		++line_num;

		/*  Ignore blank lines and comments  */
		if (inp_line[0] == COMMENT ||
		    inp_line[0] == RETURN)
		{
			/*  For user readable forms we echo these as well  */
			if (op_mode != MACHINE)
				fprintf(ofp, "%s", inp_line);
			continue;
		}

		/*  Strip off comments from the end of the line, this is
		 *  simple because the # character is not used elsewhere,
		 *  so we don't need to be context sensitive.
		 */
		for (i = 0; i < strlen(inp_line); i++)
		{
			if (inp_line[i] == COMMENT)
			{
				inp_line[i] = NULL;
				break;
			}
		}

		/*  Symbolic names must start with a lower or upper case
		 *  letter.  If the first character isn't and it's not a 
		 *  digit then it must be an error.
		 */
		if (isalpha(inp_line[0]))
		{
			space = FALSE;
			j = strlen(inp_line);

			/*  To avoid problems we have to strip off any spaces or
			 *  a newline at the end of the line.
			 */
			for (i = 0; i < j; i++)
			{
				if (inp_line[i] == RETURN)
				{
					inp_line[i] = NULL;
					break;
				}

				/*  This bit is a little sneaky to trap symbolic
				 *  names that the user has tried to use which contain
				 *  spaces
				 */
				if (isspace(inp_line[i]))
				{
					space = TRUE;
					inp_line[i] = NULL;		/*  But we don't stop here  */
				}
				else
				{
					if (space == TRUE)	/*  A character after a space  */
					{
						pfmt(stderr, MM_ERROR, ":11:fcomp: Syntax error on line %d\n",
							line_num);
						exit(ERROR);
					}
				}
			}

			/*  Check to see if the last symbolic name had the 
			 *  right number of bytes specified with it.
			 *  This does not apply to the first.
			 */
			if (first == TRUE)
				first = FALSE;
			else
			{
				if (byte_num != num_bytes[fn])
				{
					pfmt(stderr, MM_ERROR, ":12:fcomp: Symbolic name <%s> definition incomplete\n",
						old_name);
					exit(ERROR);
				}
			}

			/*  Underscores are valid when not at the start of
			 *  symbolic name.  Here we check the symbolic name
			 *  for validity, and calculate the hashing value.
			 */
			for (i = 1; i < strlen(inp_line); i++)
			{
				if (!isalpha(inp_line[i]) &&
				    inp_line[i] != UNDER_SCORE)
				{
					pfmt(stderr, MM_ERROR, ":13:fcomp: Invalid symbolic name <%s> on line %d\n",
						inp_line, line_num);
					exit(ERROR);
				}
			}

			/*  Output the name to the file  */
			fprintf(ofp, "%s", inp_line);
	
			/*  Pad out with NULLs (Machine readable only  */
			if (op_mode == MACHINE)
				for (j = strlen(inp_line); j < MAX_SYM_LEN; j++)
					fprintf(ofp, "%c", 0);
			else
					fprintf(ofp, "\n");
			
			byte_num = 0;
			/*  Remember the name for use in an error message  */
			strcpy(old_name, inp_line);
			continue;
		}

		/*  If there is anything other than a digit or a \ at the
		 *  start of this line we have a syntax error.
		 */
		if (!isdigit(inp_line[0]) && inp_line[0] != BACK_SLASH)
		{
			pfmt(stderr, MM_ERROR, ":11:fcomp: Syntax error on line %d\n",
				line_num);
			exit(ERROR);
		}

		lpos = 0;

		/*  Repeat the next loop until the universe achieves thermal
		 *  equilibrium or we reach the end of the input line.
		 *  Whichever happens sooner.
		 */
		for(EVER)
		{
			j = 0;

			/*  Extract the substring which contains the bit 
			 *  pattern for this line
			 */
			while (inp_line[lpos] != NULL &&
			       inp_line[lpos] != COMMA &&
			       !isspace(inp_line[lpos]))
				tmp_str[j++] = inp_line[lpos++];

			tmp_str[j] = NULL;

			++byte_num;

			/*  Check for too much data  */
			if (byte_num > num_bytes[fn])
			{
				pfmt(stderr, MM_ERROR, ":14:fcomp: Character <%s> definition contains too many numbers\n",
					old_name);
				exit(ERROR);
			}

			/*  If the substring starts with a \ it is an octal number
			 */
			if (tmp_str[0] == BACK_SLASH)
			{
				/*  Check that all the characters following are
				 *  valid octal digits
				 */
				for (i = 1; i < strlen(tmp_str); i++)
				{
					if (tmp_str[i] < '0' || tmp_str[i] > '7')
					{
						pfmt(stderr, MM_ERROR, ":15:fcomp: Invalid octal number on line %d\n",
							line_num);
						exit(ERROR);
					}
				}

				/*  Extract the number  */
				sscanf(&tmp_str[1], "%o", &bit_pattern);

				if (bit_pattern > MAX_VAL)
				{
					pfmt(stderr, MM_ERROR, ":16:fcomp: Octal number too large on line %d\n",
						line_num);
					exit(ERROR);
				}
			}
			/*  A hexadecimal number  */
			else if (tmp_str[0] == '0' &&
			         (tmp_str[1] == 'x' || tmp_str[1] == 'X'))
			{
				for (i = 2; i < strlen(tmp_str); i++)
				{
					/*  Validate digits  */
					if ((tmp_str[i] >= '0' && tmp_str[i] <= '9') ||
					    (tmp_str[i] >= 'a' && tmp_str[i] <= 'f') ||
					    (tmp_str[i] >= 'A' && tmp_str[i] <= 'F'))
						continue;

					/*  Must be an invalid character  */
					pfmt(stderr, MM_ERROR, ":17:fcomp: Invalid hexadecimal number on line %d\n",
						line_num);
					exit(ERROR);
				}

				sscanf(&tmp_str[2], "%x", &bit_pattern);

				if (bit_pattern > MAX_VAL)
				{
					pfmt(stderr, MM_ERROR, ":18:fcomp: Hexadecimal number too large on line %d\n",
						line_num);
					exit(ERROR);
				}
			}
			/*  Must be a binary  */
			else
			{
				bit_pattern = 0;		/*  Make sure the value is zero  */

				for (i = 0; i < strlen(tmp_str); i++)
				{
					/*  Check for non-binary digit  */
					if (tmp_str[i] != '0' && tmp_str[i] != '1')
					{
						pfmt(stderr, MM_ERROR, ":19:fcomp: Invalid binary digit on line %d\n",
							line_num);
						exit(ERROR);
					}
					
					/*  Only need to add if it's a 1  */
					if (tmp_str[i] == '0')
						continue;
					else
						bit_pattern += (1 << (strlen(tmp_str) - i - 1));
				}

				/*  Check that the number is not too large  */
				if (bit_pattern > MAX_VAL)
				{
					pfmt(stderr, MM_ERROR, ":20:fcomp: Binary number too large on line %d\n",
						line_num);
					exit(ERROR);
				}
			}

			/*  Output the data in the right format  */
			switch (op_mode) {
				case BINARY:
					for (i = 0; i < 8; i++)
						if (bit_pattern & (1 << (7 - i)))
							fprintf(ofp, "1");
						else
							fprintf(ofp, "0");
					fprintf(ofp, "\n");
					break;

				case DISPLAY:
					for (i = 0; i < 8; i++)
						if (bit_pattern & (1 << (7 - i)))
							fprintf(ofp, "*");
						else
							fprintf(ofp, " ");
					fprintf(ofp, "\n");
					break;

				case OCTAL:
					fprintf(ofp, "\\%o ", bit_pattern);

					if (byte_num == num_bytes[fn] ||
					    (byte_num % 8 == 0 && byte_num != 0))
						fprintf(ofp, "\n");

					break;

				case HEX:
					fprintf(ofp, "0x%X ", bit_pattern);

					if (byte_num == num_bytes[fn] ||
					    (byte_num % 8 == 0 && byte_num != 0))
						fprintf(ofp, "\n");

					break;

				case MACHINE:
					fprintf(ofp, "%c", bit_pattern);
					break;

				default:
					pfmt(stderr, MM_ERROR, ":21:Internal error: Invalid Output Mode\n");
			}

			/*  Skip the guff between the real data.  If there are
			 *  any bum characters then report a syntax error.
			 */
			while (!isdigit(inp_line[lpos]) &&
			       inp_line[lpos] != BACK_SLASH &&
			       inp_line[lpos] != NULL)
			{
				if (!isspace(inp_line[lpos]) && inp_line[lpos] != COMMA)
				{
					pfmt(stderr, MM_ERROR, ":22:fcomp: line %d, syntax error\n",
						line_num);
					exit(ERROR);
				}

				++lpos;
			}

			/*  Have we reached the end of the line  */
			if (inp_line[lpos] == NULL)
				break;
		}
	}

	/*  This check needs to be duplicated here to catch an incomplete
	 *  definition for the last character in the file.
	 */
	if (first == TRUE)
		first = FALSE;
	else
	{
		if (byte_num != num_bytes[fn])
		{
			pfmt(stderr, MM_ERROR, ":12:fcomp: Symbolic name <%s> definition incomplete\n",
				old_name);
			exit(ERROR);
		}
	}

	fclose(ifp);
	fclose(ofp);
}
