#ident	"@(#)sum:sum.c	1.7.1.7"
#ident "$Header$"
/*
 * Sum bytes in file mod 2^16
 */


#define WDMSK 0177777L
#define BUFSIZE 512
#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define  BSZ 16384
struct part {
	short unsigned hi,lo;
};
static union hilo { /* this only works right in case short is 1/2 of long */
	struct part hl;
	unsigned int lg;
} tempa, suma;

static unsigned char str[BSZ];
main(argc,argv)
char **argv;
{
	register int loop,bytes_read;
	register unsigned int sum;
	register i, c;
	register FILE *f;
	register off_t nbytes;
	int	alg, errflg, longflg;
	int	operands;	/* 1 if any file operands given */
	unsigned int lsavhi,lsavlo;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:sum");

	alg = errflg = longflg = 0;
	i = 1;

	while ((c = getopt(argc,argv,"rl")) != -1)
		switch (c) {
			case 'r': alg=1;
				  break;
			case 'l': longflg = 1;
				  break;
			default:  (void)pfmt(stderr,MM_ERROR,
					":2:Incorrect usage\n");
				  (void)pfmt(stderr,MM_ACTION,
					":150:Usage: sum [-rl] [file ...]\n");
				  exit(1);
		}


	i = optind;
	operands = (optind != argc);

	do {
		if(operands) {
			if((f = fopen(argv[i], "r")) == NULL) {
				(void) pfmt(stderr, MM_ERROR, 
					":3:Cannot open %s: %s\n",
					argv[i], strerror(errno));
				errflg += 10;
				continue;
			}
		} else
			f = stdin;
		sum = 0;
		suma.lg = 0;
		nbytes = 0;
		if(alg == 1) {
			while(1) {
				if ( (bytes_read = fread(str,1,BSZ,f)) < 0) 
					break;
				nbytes += bytes_read;
				for (loop=0;loop<bytes_read;loop++) {
					if (!longflg) {
						if(sum & 01)
							sum = (sum >> 1) + 0x8000;
						else
							sum >>= 1;
		                        	sum += str[loop];
						sum &= 0xFFFF;
					} else {
						if(sum & 01)
							sum = (sum >> 1) + 0x80000000;
						else
							sum >>= 1;
		                        	sum += str[loop];
						sum &= 0xFFFFFFFF;
					}
				}
				if (bytes_read < BSZ)
					break;
			}
		} else {
			while(1) {
				if ( (bytes_read = fread(str,1,BSZ,f)) < 0) 
					break;
				nbytes += bytes_read;
				for (loop=0;loop<bytes_read;loop++) {
					suma.lg += str[loop] & WDMSK;
				}
				if (bytes_read < BSZ)
					break;
			}
		}

		if(ferror(f)) {
			errflg++;
			if (operands)
				(void) pfmt(stderr, MM_ERROR, 
					":59:Read error on %s: %s\n",
					argv[i], strerror(errno));
			else
				(void) pfmt(stderr, MM_ERROR,
					":60:Read error on stdin: %s\n",
					strerror(errno));
		}
		if (alg == 1) {
			if (!longflg)
				(void) printf("%.5u %5Ld", sum, (nbytes+BUFSIZE-1)/BUFSIZE);
			else
				(void) printf("%.10u %5Ld", sum, (nbytes+BUFSIZE-1)/BUFSIZE);
		} else {
			if (!longflg) {
				tempa.lg = (suma.hl.lo & WDMSK) + (suma.hl.hi & WDMSK);
				lsavhi = (unsigned) tempa.hl.hi;
				lsavlo = (unsigned) tempa.hl.lo;
				(void) printf("%u %Ld", (unsigned)(lsavhi + lsavlo), (nbytes+BUFSIZE-1)/BUFSIZE);
			} else {
				(void) printf("%u %Ld", suma.lg, (nbytes+BUFSIZE-1)/BUFSIZE);
			}
		}
		if (operands)
			(void) printf(" %s", argv[i]==(char *)0?"":argv[i]);
		(void) printf("\n");
		(void) fclose(f);
	} while(++i < argc);
	return(errflg);
}
