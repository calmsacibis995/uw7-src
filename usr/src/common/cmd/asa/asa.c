#ident	"@(#)asa:asa.c	1.2"
#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <unistd.h>

/*
 * Process FORTRAN output files, converting FORTRAN style formatting
 * into line printer control codes.
 *
 * As per historical practice documented in POSIX 1003.2-1992 rationale:
 * 
 * Replace initial '1' with ASCII form feed, 
 *         initial '+' with ASCII carriage return, 
 *         initial '0' with ASCII new line.
 * and delete any other initial character.
 */

static int
asa(const char *file, FILE *fd){
char buf[BUFSIZ];
int first=1;	/* We're at the beginning of the file */
int nl=1;	/* We're at the beginning of a line */

  while (fgets(buf,sizeof(buf),fd) != NULL) {
	char *s = buf;
	int n;

	if (first) 
		switch(buf[0]) {
			case '0':	*s='\n'; break;
			case '1':	*s='\f'; break;

			/* 
			 * space, + and everything else treated the
			 * same on the first line.
			 */
			default : 	s++; break;
		}
	else if (nl)
		switch(buf[0]) {
					/* 
					 * Output nl from previous line
					 * unless first char is +
					 */
			case '0':	putchar('\n');  *s='\n'; break;
			case '1':	putchar('\n');  *s='\f'; break;
			case '+':			*s='\r'; break;
			default : 	putchar('\n');  s++; break;
		}


	n = strlen(s);

	/*
	 * Don't output nl now, in case next line has a +
	 */
	if (nl = (s[n-1] == '\n'))
		s[--n] = '\0';
				
	if (fputs(s,stdout) != n) {
		(void)pfmt(stderr,MM_ERROR,":1:Write error: %s\n", 
							strerror(errno));
		exit(1);
	}

	first=0;

  }

  if (ferror(fd)) {
	(void)pfmt(stderr,MM_ERROR, ":1082:Read error on file '%s': %s\n",
				file, strerror(errno));
	return 1;
  } else {
	putchar('\n');
  	return 0;
  }

}

main(int argc, char **argv){
FILE *fd;
int errors=0;

  (void)setlocale(LC_ALL, "");
  (void)setlabel("UX:asa");
  (void)setcat("uxcore.abi");

  /*
   * No options, but ignore '--' if it is the first operand
   */

  if (argc >= 2 && strcmp(argv[1],"--") == 0) {
		argv++;
		argc--;
  }

  if (argc < 2)
	exit(asa("stdin",stdin));
  else 
	for(argc--,argv++;argc;argc--,argv++)
		if ((fd = fopen(*argv,"r")) == NULL) {
			(void)pfmt(stderr,MM_ERROR,":4:Cannot open %s: %s\n",
					*argv, strerror(errno));
			errors++;
		} else {
			errors += asa(*argv,fd);
			(void)fclose(fd);
		}

  exit(errors);
}
