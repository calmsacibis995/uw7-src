#ident	"@(#)dkcomp.c	15.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include "key_remap.h"
#include "defs.h"

char *malloc();

FILE *yyin;				/* the file lex reads */
char *map_file_name;	/* the name of the input file */
int line_num;			/* the line number in the input file */
int value;				/* the returned value from lex */

/*  Array to check for duplicate keys  */
static bool used_in[256];

/* linked lists of dead key sequences  */
static struct dc {
	byte seq[3];
	struct dc *next_dc;
} *first_dead;

static int ndead;		/*  Number of dead keys  */
static int ndcseq;		/*  Number of key sequences  */

/*
 * functions which return a non-int value
 */
bool dead();
bool add_seq();	    

/**
 *  Convert the human readable dead key file to a binary format suitable 
 *  for use on the boot floppies
 **/
main(argc, argv)
int argc;
char *argv[];
{
	register int i;		/*  Loop variables  */
	register int j;
	int ofd;			/*  Output file descriptor  */
	int prev = 0;		/*  Used to avoid duplicate dead keys  */

	struct dc *pd;		/*  dead key list working pointer  */
	struct dc *pd2;		/*  dead key list working pointer  */

	DK_HDR dk_hdr;		/*  Compiled file header  */
	DK_INFO dk_info;	/*  Info on dead key  */
	DK_COMBI dk_combi;	/*  key/charcater combination  */

	/*  Check that we have at least two arguments, the input file and
	 *  the output file
	 */
	if (argc < 3)
	{
		printf("Usage: dkcomp <input_file> <output_file>\n");
		exit(1);
	}

	map_file_name = argv[1];

	/*  Open the output file for writing  */
	if ((ofd = open(argv[2], O_WRONLY | O_CREAT, 0644)) < 0)
	{
		printf("Unable to open %s for writing\n", argv[1]);
		exit(1);
	}

	/*  Open the input file for reading  */
	if ((yyin = fopen(argv[1], "r")) == NULL)
	{
		printf("cannot open %s\n", argv[1]);
		exit(1);
	}

	/*  Initialise variables  */
	first_dead = NULL;
	ndead = 0;
	ndcseq = 0;
	line_num = 1;
	skip();     /* a call to get the first real token */

	while (value != LEXEOF) 
	{
		if (value == DEAD)
		{
			if (!dead())
			{
				printf("Error in processing dead keys\n");
				exit(1);
			}
		}
		else
			skip();
	}

	fclose(yyin);

	/*  First write the header to the file  */
	strcpy((char *)dk_hdr.dh_magic, DH_MAGIC);
	dk_hdr.dh_ndead = ndead;

	if (write(ofd, &dk_hdr, sizeof(DK_HDR)) < sizeof(DK_HDR))
	{
		printf("Failed to write header to file\n");
		exit(1);
	}

	/*  Process the dead keys into the required binary format  */
	for (pd = first_dead; pd; pd = pd->next_dc)
	{
		/*  Check for a new dead key  */
		if (pd->seq[0] != prev)
		{
			prev = pd->seq[0];	/*  Set prev to this value  */

			/*  Set up info structure  */
			dk_info.di_key = pd->seq[0];
			dk_info.di_ncombi = 0;

			/*  We need to scan down the list to determine the number
			 *  of combinations with this key
			 */
			for (pd2 = pd; pd2; pd2 = pd2->next_dc)
			{
				/*  End of values for this key  */
				if (pd2->seq[0] != pd->seq[0])
					break;
				++dk_info.di_ncombi;
			}

#ifdef DEBUG
			printf("dead key %X, number combos = %d\n", 
				dk_info.di_key, dk_info.di_ncombi);
#endif

			/*  Write out the info structure to the file  */
			if (write(ofd, &dk_info, sizeof(DK_INFO)) < sizeof(DK_INFO))
			{
				printf("Unable to write info structure to file\n");
				exit(1);
			}
		}

		dk_combi.dc_orig = pd->seq[1];
		dk_combi.dc_result = pd->seq[2];

		/*  Write the combi structure to the file  */
		if (write(ofd, &dk_combi, sizeof(DK_COMBI)) < sizeof(DK_COMBI))
		{
			printf("Unable to write combination structure to file\n");
			exit(1);
		}

#ifdef DEBUG
		printf("Combo is %X -> %X\n", pd->seq[1], pd->seq[2]);
#endif
	}

	close(ofd);
}

/*
 * report a syntax error in the file
 * giving the file name and line number.
 */
static
syntax(fmt, args)
char *fmt;
int args;
{
	fprintf(stderr, "%s: %d: ", map_file_name, line_num);
	vfprintf(stderr, fmt, &args);
	return(FALSE);
}

/*
 * the dead section
 */
static
bool
dead()
{
	int d, a, b;

	if ((d = token()) < 0)
		return(syntax("expected char value for dead key\n"));

	if (d == 0)
		return(syntax("dead key cannot be null\n"));

	if (used_in[d])
		return(syntax("value 0x%02x already used for input mapping\n", d));

	used_in[d] = TRUE;
	nl();
	/*
	 * are there any dead sequences following?
	 * if not, don't bump ndead.
	 */
	if (value >= 0)
		++ndead;
	while (value >= 0)
	{
		a = value;
		if (a == 0)
			return(syntax("cannot use null in dead key sequence\n"));
		if ((b = token()) < 0)
			return(syntax("expected char value\n"));
		if (!add_seq(&first_dead, d, a, b))
			return(syntax("duplicate dead key sequence: 0x%02x 0x%2x\n", d, a));
		nl();
	}
	return(TRUE);
}

/*
 * add the dead/compose key sequence to the specified list
 * keeping things sorted.
 */
static
bool
add_seq(pfirst, d, a, b)
struct dc **pfirst;
int d, a, b;
{
	int rc;
	struct dc *first;
	struct dc *p, *q, *prev;

	++ndcseq;
	p = (struct dc *) malloc(sizeof(struct dc));

	if (!p)
	{
		printf("out of memory\n");
		exit(1);
	}

	p->seq[0] = d;
	p->seq[1] = a;
	p->seq[2] = b;
	first = *pfirst;
	if (!first) {
		p->next_dc = NULL;
		*pfirst = p;
	} else if (seqcmp(p, first) < 0) {
		p->next_dc = first;
		*pfirst = p;
	} else {
		q = first;
		while ((rc = seqcmp(q, p)) < 0 && q->next_dc) {
			prev = q;
			q = q->next_dc;
		}
		if (rc == 0)
			return(FALSE);		/* duplicate sequence */
		else if (rc < 0) {		/* add p after q */
			q->next_dc = p;
			p->next_dc = NULL;
		} else {			/* add p before q */
			prev->next_dc = p;
			p->next_dc = q;
		}
	}
	return(TRUE);
}

/*
 * the compare function for the sort.
 * there are two fields the sort is done on.
 */
static
int
seqcmp(p, q)
struct dc *p, *q;
{
	if (p->seq[0] < q->seq[0])
		return(-1);
	if (p->seq[0] > q->seq[0])
		return(1);
	return(p->seq[1] - q->seq[1]);
}

/*
 * the next token had better be a newline.
 * then skip to the next non-newline.
 */
static
nl()
{
	if (token() != NEWLINE)
		return(syntax("NEWLINE expected\n"));
	++line_num;
	skip();
}

/*
 * skip to the next non-newline token.
 */
static
skip()
{
	while ((value = token()) == NEWLINE)
		++line_num;
}
