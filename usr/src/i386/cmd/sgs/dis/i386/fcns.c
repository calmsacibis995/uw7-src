#ident	"@(#)dis:i386/fcns.c	1.5"

#include	<stdio.h>

#include	"dis.h"
#include	"sgs.h"
#include	"structs.h"
#include	"libelf.h"
#include	"ccstypes.h"

extern int Silent_mode;

#ifdef __STDC__
#include <stdlib.h>
#endif

#define TOOL	"dis"


/*
 * The following #define is necessary so that the disassembler
 * will run on UNIX 370 3.0 and UNIX 370 4.0 (printf was changed).
 *
 */

#define		BADADDR	-1L	/* used by the resynchronization	*/
				/* function to indicate that a restart	*/
				/* candidate has not been found		*/

void		convert();

FUNCLIST	*currfunc;

/*
 *      void getbyte ()
 *
 *      read a byte, mask it, then return the result in 'curbyte'.
 *      The getting of all single bytes is done here.  The 'getbyte[s]'
 *      routines are the only place where the global variable 'loc'
 *      is incremented.
 */
void
getbyte()
{
	extern unsigned short curbyte;
        extern  long    loc;            /* from _extn.c */
        extern  char    object[];       /* from _extn.c */
        char    temp[NCPS+1];
	extern unsigned char *p_data;
	extern	int	trace;
        unsigned char *p = (unsigned char *)&curbyte;

	*p = *p_data; ++p_data;
        loc++;
        curbyte = *p & 0377;
        convert(curbyte, temp, NOLEADSHORT, 0);
#ifndef FUR_ORIG
	if (!Silent_mode) {
			(void)sprintf(object,"%s%s ",object,temp);
		if (trace > 1)
		{
			(void)printf("\nin getbyte object <%s>\n", object);
		}
	}
#endif /* ifndef FUR */
	return;
}



/*
 *	void convert (num, temp, flag, extend_flag)
 *
 *	Convert the passed number to either hex or octal, depending on
 *	the oflag, leaving the result in the supplied string array.
 *	If  LEAD  is specified, preceed the number with '0' or '0x' to
 *	indicate the base (used for information going to the mnemonic
 *	printeut).  NOLEAD  will be used for all other printing (for
 *	printing the offset, object code, and the second byte in two
 *	byte immediates, displacements, etc.) and will assure that
 *	there are leading zeros.
 *
 *      extend_flag will tell us whether or not to print single byte
 *      numbers as sign extended
 */

void
convert(num,temp,flag, extend_flag)
unsigned	num;
char	temp[];
int	flag;
int	extend_flag;
{
	extern	int	oflag;		/* in _extn.c */

#ifndef FUR_ORIG
	if (!Silent_mode) {
		if (extend_flag && (num & 0x80)) /* if it is negative! */
			num |= ~0xffL;		/* single byte sign extend */

		if (flag == NOLEAD)
			(oflag) ?	(void)sprintf(temp,"%06o",num):
					(void)sprintf(temp,"%04x",num);
		if (flag == LEAD)
			(oflag) ?	(void)sprintf(temp,"0%o",num):
					(void)sprintf(temp,"0x%x",num);
			if (flag == NOLEADSHORT)
					(oflag) ?       (void)sprintf(temp,"%03o",num):
									(void)sprintf(temp,"%02x",num);
	}
#endif /* ifndef FUR_ORIG */
	return;
}


/*
 *	dis_data ()
 *
 *	the routine to disassemble a data section,
 *	which consists of just dumping it with byte offsets
 */

#ifndef FUR
void
dis_data( shdr )
Elf32_Shdr *shdr;
{
	extern	short	aflag;	 /* from _extn.c */
	extern	long	loc;	 /* from _extn.c */
	extern	char	mneu[];	 /* from _extn.c */

	static 	void	get2bytes();
#ifdef AR32WR
	static	void	getswapb2();
#endif

	void		printline(),
			prt_offset();

	short		count;
	long		last_addr;

	/* Blank out mneu so the printline routine won't print extraneous
	 * garbage.
	 */

	(void)sprintf(mneu,"");

	for (loc = aflag? shdr->sh_addr: 0, last_addr = loc + shdr->sh_size;
	    loc < last_addr; printline()) 
	{
		/* if -da flag specified, actual adress will be printed
		 if -d flag specified, offset within section will be printed */

		(void)printf("\t");
		prt_offset();
                for (count=0; (count<6) && (loc<last_addr); count+=2)
                        get2bytes();
	}
	return;
}
#endif /* ifndef FUR */



/*
 *	get2bytes()
 *
 *	This routine will get 2 bytes, print them in the object file
 *	and place the result in 'cur2bytes'.
 */

static void
get2bytes()
{
	extern  unsigned char *p_data;
	extern	long	loc;		/* from _extn.c */

	extern	unsigned short cur2bytes; /* from _extn.c */
	extern unsigned char *p_data;
	unsigned	char 	*p = (unsigned char *)&cur2bytes;

	extern	char	object[];	/* from _extn.c */
	char	temp[NCPS+1];
	extern	int	trace;

	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data;
	loc += 2;
	convert( (cur2bytes & 0xffff), temp, NOLEAD, 0);
	(void)sprintf(object,"%s%s ",object, temp);
	if (trace > 1)
	(void)	printf("\nin get2bytes object<%s>\n",object);
	return;
}


#ifdef AR32WR 
/*
 *	static void getswapb2()
 *
 *	This routine is used only for m32, m32a and vax. It will get and
 *	swap 2 bytes, print them in the object file and place the
 *	result in 'cur2bytes'.
 */

static void
getswapb2()
{
	extern	long	loc;		/* from _extn.c */
	extern	unsigned short cur2bytes; /* from _extn.c */
	extern	char	object[];	/* from _extn.c */
	char	temp[NCPS+1];
	extern	int	trace;
	extern unsigned char *p_data;

	unsigned char *p = (unsigned char *)&cur2bytes;

	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data;
	loc += 2;
	/* swap the 2 bytes contained in 'cur2bytes' */
	cur2bytes = ((cur2bytes>>8) & (unsigned short)0x00ff) |
		((cur2bytes<<8) & (unsigned short)0xff00);
	convert( (cur2bytes & 0xffff), temp, NOLEAD, 0);
	(void)sprintf(object,"%s%s ",object, temp);
	if (trace > 1)
		(void)printf("\nin getswapb2 object<%s>\n",object);
	return;
}
#endif


/*
 *	printline ()
 *
 *	Print the disassembled line, consisting of the object code
 *	and the mnemonics.  The breakpointable line number, if any,
 *	has already been printed, and 'object' contains the offset
 *	within the section for the instruction.
 */

void
printline()
{
	extern	int	oflag;		/* in _extn.c */
	extern	int	sflag;		/* in _extn.c */
	extern  char	object[];
	extern	char	mneu[];
	extern	char	symrep[];

        if (oflag > 0)
                (void)printf(sflag?"%-36s%s\n%20c[%s]\n":"%-36s%s\n",
                                object,mneu,' ',symrep); /* to print octal */
        else
                (void)printf(sflag?"%-30s%s\n%20c[%s]\n":"%-30s%s\n",
                                object,mneu,' ',symrep); /* to print hex */

	return;
}

/*
 *	printerrline ()
 *
 *	Print the disassembled line to stderr in the case where the 
 *	opcode is determined to be "bad."
 */

void
printerrline()
{
	extern	int	oflag;		/* in _extn.c */
	extern	int	sflag;		/* in _extn.c */
	extern  char	object[];
	extern	char	mneu[];
	extern	char	symrep[];

        if (oflag > 0)
                (void)fprintf(stderr, sflag?"%-36s%s\n%20c[%s]\n":"%-36s%s\n",
                                object,mneu,' ',symrep); /* to print octal */
        else
                (void)fprintf(stderr, sflag?"%-30s%s\n%20c[%s]\n":"%-30s%s\n",
                                object,mneu,' ',symrep); /* to print hex */

	return;
}

#ifdef FUR
/* This code is duplicated for fur from common/utls.c
/*
 *      void lookbyte ()
 *
 *      read a byte, mask it, then return the result in 'curbyte'.
 *      The byte is not immediately placed into the string object[].
 *      is incremented.
 */

void
lookbyte()
{
	extern unsigned short curbyte;
        extern  long    loc;            /* from _extn.c */
	extern unsigned char *p_data;
	unsigned char *p = (unsigned char *)&curbyte;

	*p = *p_data; ++p_data;
        loc++;
        curbyte = *p & 0377;
}
#endif /* ifndef FUR */
