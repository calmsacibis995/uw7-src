/*		copyright	"%c%" 	*/

#ident	"@(#)limits.c	1.2"
#ident "$Header$"
#include "uucp.h"

/* add sitedep() and sysmatch() to list when ifdefs are removed below */
static int get_tokens(), siteindep();

/*  field array indexes for LIMIT parameters */

#define U_SERVICE	0
#define U_MAX		1
#define U_SYSTEM	2
#define U_MODE		3

static char * _Lwords[] = {"service", "max", "system", "mode"};

#define NUMFLDS		4

struct name_value
{
	char *name;
	char *value;
};

/*
 * manage limits file.
 */

/*
 * scan the Limits file and get the limit for the given service.
 * return SUCCESS if the limit was found, else return FAIL.
 */
int
scanlimit(service, limitval)
char *service;
struct limits *limitval;
{
	char buf[BUFSIZ];
	char *tokens[NUMFLDS];	/* fields found in LIMITS */
	char msgbuf[BUFSIZ];	/* place to build messages */
	FILE *Fp = NULL;	/* file pointer for LIMITS */
	int SIflag, SDflag;

	if ((Fp = fopen(LIMITS, "r")) == NULL) {
	    DEBUG(5, "cannot open %s\n", LIMITS);
	    sprintf(msgbuf, "fopen of %s failed with errno=%%d\n", LIMITS);
	    DEBUG(5, msgbuf, errno);
	    return(FAIL);
	}

	SIflag = FALSE;
	SDflag = TRUE;

	while ((getuline(Fp, buf) > 0) && ((SIflag && SDflag) == FALSE)) {
	    if (get_tokens(buf, tokens) == FAIL)
		continue;

	    if (SIflag == FALSE) {
		if (siteindep(tokens, service, limitval) == SUCCESS)
		    SIflag = TRUE;
	    }

	}

	fclose(Fp);
	if ((SIflag && SDflag) == TRUE)
	    return(SUCCESS);
	else return(FAIL);
}

/*
 * parse a line in LIMITS and return a vector
 * of fields (flds)
 *
 * return:
 *	SUCCESS - token pairs name match with the key words
 */
static int
get_tokens (line,flds)
char *line;
char *flds[];
{
	register i;
	register char *p;
	struct name_value pair;

	/* initialize defaults  in case parameter is not specified */
	for (i=0;i<NUMFLDS;i++)
		flds[i] = CNULL;

	for (p=line;p && *p;) {
		p = next_token (p, &pair);

		for (i=0; i<NUMFLDS; i++) {
			if (EQUALS(pair.name, _Lwords[i])) {
				flds[i] = pair.value;
				break;
			}
			if (i == NUMFLDS-1) /* pair.name is not key */
				return FAIL;
		}
	}
	return(SUCCESS);
}
/*
 * this function can only handle the following format
 *
 *	service=uucico max=5
 *
 * return:
 *	SUCCESS - OK
 *	FAIL - service's value does not match or wrong format
 */
static int
siteindep(flds, service, limitval)
char *flds[];
char *service;
struct limits *limitval;
{

	if (!EQUALS(flds[U_SERVICE], service) || (flds[U_MAX] == CNULL))
		return(FAIL);
	if (sscanf(flds[U_MAX],"%d",&limitval->totalmax)==0)
		limitval->totalmax = -1; /* type conflict*/
	return(SUCCESS);
}

