#ident	"@(#)OSRcmds:cmos/cmos.c	1.1"
#pragma comment(exestr, "@(#) cmos.c 25.2 94/08/03 ")

/*	@(#) cmos.c 25.2 94/08/03 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985-1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/***	cmos -- manipulate the Salmon CMOS configuration database
**
**	cmos [ [-s|-r file ] | [address [ value ]] ]
**
**	if no address or value is specified, will dump 
**		entire database contents
**	if address only is specified, will dump that address
**	if both address and value are specified, that
**		address is set to value
**
**	if -s file then read /dev/cram and save it to file 
**	if -r file then restore /dev/cram from file 
**
**	pfb 1/84
*/

/*
 *	MODIFICATION HISTORY
 *	M000	May 22/84	barrys
 *	Addresses and values can now be entered in hex and octal as
 *	well as in decimal.  Verification is also done to make sure
 *	that the numbers entered correspond to the base that is implied.
 *	M001	Feb 2  1987	sco!chapman
 *	Added checksum computation.  This allows cmos(C) to
 *	now be usefull.
 *	L002	June 30 1994	scol!dipakg
 *	Added -s and -r flags to save to file and restore from file
 *	to make this tool useful for boot/root emergency floppies.
 */

#include <sys/cram.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#define NBYTES 64
#define START_CKSM	0x10		/* first address to checksum */
#define END_CKSM	0x2d		/* last address to checksum */
#define	HB_CKSM		0x2e		/* CMOS addr of high byte of checksum */
#define	LB_CKSM		0x2f		/* CMOS addr of low byte of checksum */


#define LOWER(c)	(isupper(c)? _tolower(c) : (c))

#define BHEX	16		/* base multipliers */
#define BDEC	10
#define BOCT	8
#define BERR	-1

extern	long	lseek();

int	fd;	/* global handle for database file */
FILE	*fp;	/* L002 file stream for save/restore operation */

unsigned getbyte();


main(argc, argv)
int	argc;
char	**argv;
{
	int i;
	int j; 
	unsigned cksm;
	char buf[15]; /* L002 buffer to hold an entry in save/restore file */

	if ( argc > 3 ){
		fprintf(stderr, "usage: cmos [ [-s|-r file ] | [address [ value ]] ]\n"); /* L002 */
		exit(1);
	}

	if ( ( fd = open("/dev/cram", O_RDWR) ) < 0 )
		moan("can't open /dev/cmos");

	switch(argc) {
	case 1:
		for ( i = 0; i < NBYTES; i += 1 )
			printbyte(i);
		break;
	case 2:
		/* Begin M000 */
		if (isbase(argv[1]) == -1) {
			fprintf(stderr, "cmos: bad number %s\n", argv[1]);
			exit(1);
		}
		i = atobi(argv[1]);
		/* End M000 */
		if ( i < 0 || i >= NBYTES )
			moan("address out of range");
		printbyte(i);
		break;
	case 3:
		/* L002 begin */
		if ( argv[1][0] == '-' && argv[1][1] == 's' )
		{
			printf("Saving /dev/cmos to file %s\n",argv[2]);
			if ( (fp=fopen(argv[2],"w")) == NULL ){
				fprintf(stderr, "cmos: Cannot open %s for writing.\n", argv[2]);
				exit(1);
			}
		
			/* read each byte and save to file */
			for ( i = 0; i < NBYTES; i += 1 )
			{
				buf[0] ='\0';
				sprintf(buf,"0x%x 0x%x\n", (i & 0xff), ((int)getbyte(i) & 0xff));
				fwrite(buf,1,strlen(buf),fp);
			}
			fclose(fp);
			exit(0);
		}
		else if ( argv[1][0] == '-' && argv[1][1] == 'r' )
		{
			printf("Restoring /dev/cmos from file %s\n",argv[2]);
			if ( (fp=fopen(argv[2],"r")) == NULL ){
				fprintf(stderr, "cmos: Cannot open %s for reading.\n", argv[2]);
				exit(1);
			}
		
			/* read each byte from file and write to /dev/cmos */
			for ( i = 0; i < NBYTES; i += 1 )
			{
				buf[0] ='\0';
				fscanf(fp,"%x %x\n", &i, &j);
				setbyte(i,j);
			}
			/* Update the checksum. */

			cksm = 0;
			for (i = START_CKSM; i <= END_CKSM; i++)
		    		cksm += getbyte(i);
			setbyte(LB_CKSM, cksm & 0xff);
			setbyte(HB_CKSM, (cksm>>8) & 0xff);
			fclose(fp);
			exit(0);
		}
		/* L002 end */

		/* Begin M000 */
		if (isbase(argv[1]) == -1) {
			fprintf(stderr, "cmos: bad number %s\n", argv[1]);
			exit(1);
		}
		i = atobi(argv[1]);
		if (isbase(argv[2]) == -1) {
			fprintf(stderr, "cmos: bad number %s\n", argv[2]);
			exit(1);
		}
		j = atobi(argv[2]);
		/* End M000 */
		if ( i < 0 || i >= NBYTES )
			moan("address out of range");
		if ( j < 0 || j > 0xff )
			moan("value out of range");
		setbyte(i, j);
		printf("%x = %x\n", i, j);

		/* Update the checksum. */

		cksm = 0;
		for (i = START_CKSM; i <= END_CKSM; i++)
		    cksm += getbyte(i);
		setbyte(LB_CKSM, cksm & 0xff);
		setbyte(HB_CKSM, (cksm>>8) & 0xff);
		break;
	}
	exit(0);
}

unsigned
getbyte(a)
int a;
{
	struct {
		char offset;
		unsigned char val;
	} data;

	data.offset=a;
	if ( (int)ioctl(fd, CMOSREAD, &data) != 0 )
		moan("can't get CMOS data");
	return(data.val);
}

printbyte(a)
int a;
{
	printf("%x %x\n", (a & 0xff), ((int)getbyte(a) & 0xff));
}

setbyte(a, v)
int a;
int v;
{
	struct {
		char offset;
		unsigned char val;
	} data;

	data.offset = (unsigned char)a;
	data.val = (unsigned char)v;
#ifdef DEBUG
	printf("setbyte: val = %d, v = %d, a = %d\n", data.val, a, v);
#endif
	if ( (int)ioctl(fd, CMOSWRITE, &data) != 0 )
		moan("can't set CMOS data");
}
	
/*
 * Determine the base of the number and verify that all digits in
 * the number coincide with the particular base.
 */
isbase(s)
	register char	*s;
{
	register int	base;

	if (*s == '-') {
		s++;
	}
	if (*s == '0' && s[1] == 'x') {
		base = BHEX;
		s +=2;
	} else if (*s == '0') {
		base = BOCT;
		s++;
	} else
		base = BDEC;
	while (*s != ' ' && *s != '\t' && *s != '\0') {
		switch (base) {
			case BDEC:
				if (isdigit(*s)) 
					break;
				return(BERR);

			case BOCT:
				if (*s >= '0' && *s <= '7')
					break;
				return(BERR);

			case BHEX:
				if (isxdigit(*s)) 
					break;
				return(BERR);
		}
		s++;
	}
	return(base);
}


/*
 * atobi - atoi with variable base 
 */

atobi(s)
	register char	*s;
{
	register int digit;
	register int base = BDEC;
	register int sum;
	register int sign = 0;

	if (*s == '-') {
		s++;
		sign++;
	}
	if (*s == '0' && s[1] == 'x') {
		s += 2;
		base = BHEX;
	} else if (*s == '0') {
		base = BOCT;
	}
	for (sum = 0; isxdigit(digit = *s); s++) {
		if (isdigit(digit)) {
			if (digit > '7' && base == BOCT) {
				break;
			}
			sum = (sum * base) + digit - '0';
		} else if (base == BHEX) {
			sum = (sum * base) + LOWER(digit) + 10 - 'a';
		} else {
			break;
		}
	}
	return(sign ? -sum : sum);
}

moan(s)
char *s;
{
	fprintf(stderr, "cmos: %s.\n", s);
	exit(1);
}
