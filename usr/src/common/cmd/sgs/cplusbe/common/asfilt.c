#ident	"@(#)cplusbe:common/asfilt.c	1.3"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef char bool;
#define TRUE 1
#define FALSE 0

#define MAX_LINE 512
#define MAX_LABELS 32767

void define_exception_thrown(FILE* out_s)
{
	static bool defined = FALSE;
	if (! defined) {
		fputs("\t.data\n", out_s);
		fputs("\t.align\t4\n", out_s);
		fputs("__exception_thrown:\n", out_s);
		fputs("\t.zero\t4\n", out_s);
		fputs("\t.type\t__exception_thrown,\"object\"\n", out_s);
		fputs("\t.size\t__exception_thrown,4\n", out_s);
		fputs("\t.previous\n", out_s);
		defined = TRUE;
	}
}

int main(int argc, char *argv[])
{
	/* Read each line of .s file.

	   If is a ..ERnnn label, make note of that.

	   If is a .long ..ERnnn ... reference, if label has not been defined,
	   delete that line and the next.

	   If is a ..ECnnn label, make note of that.

	   If is a .long ..ECnnn ... reference, if label has not been defined,
	   change the reference to 0.

	   If is a cmpl $0,__exception_thrown, delete that line and comment
	   lines up until a jne instruction.  If see reference to 
	   __exception_thrown outside of that pattern,
	   add a definition of __exception_thrown, set to zero.

	   Otherwise, write .s file out.
	*/

	FILE* in_s;
	FILE* out_s;
	char line[MAX_LINE];
	bool ER_defined[MAX_LABELS];
	bool EC_defined[MAX_LABELS];
	int i;
	bool skip_next_line = FALSE;

	if (argc != 3) {
		fprintf(stderr, "Usage: asfilt in-s out-s\n");
		return EXIT_FAILURE;
	}

	in_s = fopen(argv[1], "r");
	if (in_s == NULL) {
		fprintf(stderr, "Cannot open input file %s\n", argv[1]);
		return EXIT_FAILURE;
	}
	out_s = fopen(argv[2], "w");
	if (out_s == NULL) {
		fprintf(stderr, "Cannot open output file %s\n", argv[2]);
		return EXIT_FAILURE;
	}

	for (i = 0; i < MAX_LABELS; i++)
		EC_defined[i] = ER_defined[i] = FALSE;

	/* first pass, collect label definitions */

	while (fgets(line, sizeof line, in_s) != NULL) {
		int label_no;

		if (strlen(line) >= 5 && strncmp(line, "..ER",4) == 0) {
			/* found a region label definition, mark it */
			sscanf(line, "..ER%d:", &label_no);
			ER_defined[label_no] = TRUE;
		}
		if (strlen(line) >= 5 && strncmp(line, "..EC",4) == 0) {
			/* found a catch handlers label definition, mark it */
			sscanf(line, "..EC%d:", &label_no);
			EC_defined[label_no] = TRUE;
		}
	}

	rewind(in_s);

	while (fgets(line, sizeof line, in_s) != NULL) {
		int label_no;
		char* idx;

#if 0
		if (skip_next_line
		    && strlen(line) >= 11
		    && strncmp(line,"/VOL_OPND 2",11) == 0)
			/* volatile marking for bogus branch,skip it and next */
			continue;

		if (skip_next_line) {
			skip_next_line = FALSE;
			continue;
		}
#endif

		/* TBD should use sscanf's directly */

		if (strncmp(line, "\t.long\t..ER", 11) == 0) {
			/* found a region label reference, check it */
			sscanf(line, "\t.long\t..ER%d-", &label_no);

			if (! ER_defined[label_no]) {
				/* label wasn't defined, skip this RRT entry
				   (this line and the next) */
				/* TBD could sanity check the next line,
				   but unlikely to be wrong	*/
				fgets(line, sizeof line, in_s);
				continue;
			}
		}

		if (strncmp(line, "\t.long\t..EC", 11) == 0) {
			/* found a catch handlers label reference, check it */
			sscanf(line, "\t.long\t..EC%d-", &label_no);

			if (! EC_defined[label_no]) {
				int j;
				/* label wasn't defined, replace with zero */
				line[7] = '0';
				for (j = 8; j < MAX_LINE && line[j] != '\t'; j++)
					line[j] = ' ';
			}
		}

		if (strncmp(line, "\tcmpl\t$0,__exception_thrown", 27) == 0) {
			char save_conditional[MAX_LINE];
			char save_volatile[MAX_LINE];
			int line_2_saved = FALSE;
			/* found a bogus conditional */
			strcpy(save_conditional, line);
			/* assume we won't hit EOF during look ahead */
			fgets(line, sizeof line, in_s);
			if (strncmp(line, "/VOL_OPND 2", 11) == 0) {
				/* found volatile comment after conditional */
				strcpy(save_volatile, line);
				line_2_saved = TRUE;
				fgets(line, sizeof line, in_s);
			}
			if (strncmp(line, "\tjne\t.L", 7) == 0)
				/* found right jump, drop everything */
				continue;
			else {
				/* not expected sequence, restore everything
				   and make definition for __exception_thrown */
				define_exception_thrown(out_s);
				fputs(save_conditional, out_s);
				if (line_2_saved)
					fputs(save_volatile, out_s);
			}
		} else if (strstr(line, "__exception_thrown") != NULL)
			/* some other usage we don't handle now, 
			   make definiton for __exception_thrown */
			define_exception_thrown(out_s);

#if 0
		if ((strlen(line) >= 8)
		    && (strncmp(line,"\t.size\t",7) == 0)
		    && (idx = strstr(line,".-")) != NULL) {
			/* found a function .size, also put out a .set */
			char name[MAX_LINE];
			char line2[MAX_LINE];
			strcpy(name,idx);
			name[strlen(name)-1] = '\0';	/* drop newline	*/
			sprintf(line2, "\t.set\t..EH.end.%s,.-%s\n", 
				name+2, name+2);	/* skip ".-"	*/
			fputs(line2, out_s);
		}
#endif

		fputs(line, out_s);
	}

	fclose(in_s);
	fclose(out_s);
 
	return EXIT_SUCCESS;
}
