#ident	"@(#)pdi.cmds:script.c	1.7.1.1"
#ident	"$Header$"

/*	copyright	"%c%"	*/
/*  "put_string()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_NOSTD>.
 *
 *  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "warning()" has been internationalized. The string to be output
 *   must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/stat.h>

#include 	<pfmt.h>
#include 	<errno.h>

#ifndef i386
#include	<sys/pdi.h>
#endif /* i386 */

#include	<sys/sdi_edt.h>
#include	<sys/scsi.h>
#include	"tokens.h"

#define TRUE		1
#define FALSE		0
#define ERREXIT		1
#if u3b2
#define	SCSI_DIR	"/usr/lib/scsi/"
#define SCSI_DIR2	"/usr/lib/scsi/format.d/"
#elif i386
#define	SCSI_DIR	"/etc/scsi/"
#define SCSI_DIR2	"/etc/scsi/format.d/"
#else /* 3B4000 */
#define	SCSI_DIR	"/etc/scsi.d/"
#define SCSI_DIR2	"/etc/scsi.d/format.d/"
#endif

extern int		Show;
extern struct ident	Inquiry_data;
typedef struct stat	STAT;
static char		TC_err[] = ":294:invalid format of the target controller index file : %s\n";

static char		Cmdname[64];

TOKENS_T	Tokens[] = {
	"UNKNOWN",		UNKNOWN,
	"BLOCK NUMBER",		BLOCK,
	"BYTES FROM INDEX",	BYTES,
	"DISK",			DISK,
	"DISK INFO",		DISKINFO,
	"FORMAT",		FORMAT,
	"FORMAT WITH DEFECTS",	FORMAT_DEFECTS,
	"MODE SELECT",		MDSELECT,
	"MODE SENSE",		MDSENSE,
	"PHYSICAL SECTOR",	PHYSICAL,
	"READ DEFECT DATA",	RDDEFECT,
	"READ",			READ,
	"READ CAPACITY",	READCAP,
	"REASSIGN BLOCK",	REASSIGN,
	"VERIFY",		VERIFY,
	"WRITE",		WRITE,
	"TCINQ",		TCINQ,
	"MKDEV",		MKDEV,
	"#",			COMMENT,
	"GENERIC",		GENERIC,
	"TCLEN",		TCLEN,
};


/*
 * File_Exists() - checks for the existence of a file; returns 1 if the file exists,
 * returns 0 if the file does not exist, exits for error.
 */

int
File_Exists(path)
char *path;
{
   STAT  buf;

   if(stat(path, &buf) < 0) {
	/*
	 * errno == ENOENT if file does not exist.
	 * Otherwise exit, because something else is wrong.
	 */
	if (errno != ENOENT )
		error(":295:Stat failed for %s\n", path);
	else /* file does not exist */
		return(FALSE);
	}
	/* file exists */
	return(TRUE);
}


/* The following is a new function added for "merged" tc.index files for
 * both "mkdev" and "format" called by DISK subutility
 */

FILE *
scriptfile_open(indexf_name)
char	*indexf_name;
{
	register int	tctype_match;
	register int	scriptf_found;
	register int	tokenindex;

	FILE	*indexfp;
	FILE	*scriptfp;
	int	end, tcinqlen;
	int	i;

	char	indexfile[MAX_LINE];
	char	scriptfile[MAX_LINE];
	char	product_id[MAX_LINE];
	char	tctypetoken[MAX_LINE];
	char	tcinqstring[MAX_LINE];

	indexfile[0] = '\0';
	scriptfile[0] = '\0';
	product_id[0] = '\0';
	tctypetoken[0]  = '\0';
	tcinqstring[0]  = '\0';

	tctype_match = FALSE;
	scriptf_found = FALSE;


	(void) strcpy(indexfile, indexf_name);

	/* obtain Device ID from Inquiry_data structure */

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":296:Device ID: %s\n", Inquiry_data.id_prod);

	strncpy(tcinqstring, Inquiry_data.id_prod, 16);
	tcinqstring[16]  = '\0';

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":296:Device ID: %s\n", tcinqstring);

	/* check to see if the index file exists */
	if (!File_Exists(indexfile)) {
		return(NULL);
	}
	if (Show)
		(void) pfmt(stderr, MM_INFO,
			":289:Opening %s\n", indexfile);

	/* open the index file */
	if ((indexfp = fopen(indexfile, "r")) == NULL) {
		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":275:%s open failed\n", indexfile);
		return(NULL);
	}

	tcinqlen = PID_LEN;

	/* start at the beginning of the index file */
	rewind(indexfp);
	while (!scriptf_found) {
		tokenindex = get_token(indexfp);

		if (Show)
			(void) pfmt(stdout, MM_NOSTD,
			    ":297:token = %s\n", Tokens[tokenindex].string);
		switch (tokenindex) {

#ifndef i386
		case TCTYPE:
			if (fscanf(indexfp, " %[^\n]\n", tctypetoken) == EOF) {
				errno = 0;
				warning(TC_err, indexfile);
				fclose(indexfp);
				return(NULL);
			}
			strncpy(product_id, &tctypetoken[8], 16);
			product_id[16]  = '\0';

			if (Show) 
				(void) pfmt(stderr, MM_INFO,
				    ":296:Device ID: %s\n", product_id);

			if (strcmp(product_id, tcinqstring) == 0)

				tctype_match = TRUE;
			break;

#endif /* i386 */

		/* A new GENERIC token was added to allow the an intelligent
		 * attempt at completing disksetup in case the TC's inquiry 
		 * string does NOT match an entry in the tc.index file.
		 */

		case GENERIC:

			if (fscanf(indexfp, " %[^\n]\n", tctypetoken) == EOF) {
				errno = 0;
				warning(TC_err, indexfile);
				fclose(indexfp);
				return(NULL);
			}

			if (Show) 
				(void) pfmt(stderr, MM_INFO,
					":296:Device ID: %s\n", tctypetoken);

			if (strncmp(tctypetoken, "RANDOM", 6) == 0)
			{
				tctype_match = TRUE;
			}

			break;

		/* A new TC INQuiry token was added to allow the TC's inquiry 
		 * string to specify the devices template file.
		 */
		case TCINQ:
			if (fscanf(indexfp, " %[^\n]\n", tctypetoken) == EOF) {
				errno = 0;
				warning(TC_err, indexfile);
				fclose(indexfp);
				return(NULL);
			}
			strncpy(product_id, &tctypetoken[VID_LEN], PID_LEN);
			product_id[PID_LEN]  = '\0';

			if (Show) 
				(void) pfmt(stderr, MM_INFO,
					":296:Device ID: %s\n", product_id);

			if (strncmp(product_id, tcinqstring, tcinqlen) == 0)
			{
				tctype_match = TRUE;
			}
			tcinqlen = PID_LEN;
			break;


		case TCLEN:
			if (fscanf(indexfp, " %d\n", &tcinqlen) == EOF) {
				errno = 0;
				warning(TC_err, indexfile);
				fclose(indexfp);
				return(NULL);
			}
			tcinqlen -= VID_LEN;
			if ( tcinqlen < 1 )
				tcinqlen = PID_LEN;
			break;


		case FORMAT:
			if (tctype_match) {
				if (fscanf(indexfp," %[^\n]\n", scriptfile) == EOF) {
					errno = 0;
					warning(TC_err, indexfile);
					fclose(indexfp);
					return(NULL);
				}
				scriptf_found = TRUE;
			}
			/* read the remainder of the input line */
			fscanf(indexfp, "%*[^\n]%*[\n]");
			break;

		case EOF:
			errno = 0;
			warning(":298:TC entry not found in %s.\n",
				indexfile);
			fclose(indexfp);
			return(NULL);

		case MKDEV:
		case COMMENT:
		case UNTOKEN:
		case UNKNOWN:
		default:
			/* read the remainder of the input line */
			fscanf(indexfp, "%*[^\n]%*[\n]");
			break;
		}
	}

	/* close index file */
	fclose(indexfp);

	if (Show)
		(void) pfmt(stdout, MM_ERROR,
			":299:scriptfile     = %s\n", scriptfile);

	/* Check to see that the script file exists. */
	if (!File_Exists(scriptfile)) {
		warning(":300:%s does not exist\n", scriptfile);
		return(NULL);
	}

	/* open the target controller script file. */
	if ((scriptfp = fopen(scriptfile,"r")) == NULL) 
		warning(":301:Could not open %s\n", scriptfile);
	
	return(scriptfp);
}

/* get_token() - reads the SCSI script file and returns the token found */

int
get_token(scriptfp)
FILE *scriptfp;	/* File pointer for the script file */
{
	int	ch;
	char	*c;
	char	token[MAX_LINE];
	int	curtoken;

	/* Skip over white space */
	while (isspace(ch = getc(scriptfp)));

	/* Put the last character read back in the stream */
	if (ungetc(ch, scriptfp) == EOF)
		return(EOF);

	/* Read the next token from the script file */
	switch (fscanf(scriptfp, "%[A-Z ] : ", token)) {
	case EOF :
		return(EOF);
	case 1 :
		break;
	default :
		return(UNKNOWN);
		break;
	}

	c = &token[strlen(token) - 1];
	while (isspace(*c))
		*c-- = '\0';

	/* Determine which token */
	for (curtoken = 0; curtoken < NUMTOKENS; curtoken++) {
		if (strcmp(Tokens[curtoken].string, token) == 0)
			return(Tokens[curtoken].token);
	}

	/* Token not found */
	return(NUMTOKENS);
}	/* get_token() */

/* get_string() - reads the SCSI script file and returns the remaining line */

int
get_string(scriptfp, string)
FILE *scriptfp;	/* File pointer for the script file */
char *string;	/* Location to place string */
{
	int	ch;

	/* Skip over white space */
	while (isspace(ch = getc(scriptfp)));

	/* Put the last character read back in the stream */
	if (ungetc(ch, scriptfp) == EOF)
		return(EOF);

	return(fscanf(scriptfp, "%[^\n]\n", string));
}	/* get_string() */

/* get_data() - reads the SCSI script file and returns len char's of
 * ascii hexidecimal data in the data pointer
 */

int
get_data(scriptfp, data, len)
FILE *scriptfp;	/* File pointer for the script file */
char *data;	/* Location to place data */
int len;	/* Length of data */
{
	int	ch;
	int	digitseen = 0;

	while (--len >= 0) {
		/* Skip over white space */
		while (isspace(ch = getc(scriptfp)));
		if (isxdigit(ch)) {
			int digit1 = ch - (isdigit(ch) ? '0' :
				     isupper(ch) ? 'A' - 10 : 'a' - 10);

			/* Skip over white space */
			while (isspace(ch = getc(scriptfp)));
			if (isxdigit(ch)) {
				int digit2 = ch - (isdigit(ch) ? '0' :
					     isupper(ch) ? 'A' - 10 : 'a' - 10);

				*data++ = (char) 16 * digit1 + digit2;
				digitseen++;
			}
		}
	}

	/* Try to put the next character read back in the stream */
	if (ungetc(ch = getc(scriptfp), scriptfp) == EOF)
		return(EOF);

	return(digitseen);	/* Successful match if non-zero */
}	/* get_data() */



/* put_token() - writes the token to the output file */

void
put_token(outputfp, token)
FILE *outputfp;	/* File pointer for the output file */
int token;	/* Token to be written */
{
	int	curtoken;

	/* Determine which token */
	for (curtoken = 0; curtoken < NUMTOKENS; curtoken++) {
		if (token == Tokens[curtoken].token) {
			(void) fprintf(outputfp, "%s : ", Tokens[curtoken].string);
			break;
		}
	}

	if (curtoken == NUMTOKENS)
		(void)pfmt(outputfp, MM_ERROR,
			":302:UNKNOWN 0x%X : ", token);
}	/* put_token() */


char *
upgettxt(char *string)
{
  char	msgid[128], *mp = msgid;
  char	*defmsg;

  /* copy catalog name */
  for (defmsg = string; *defmsg != ':' && *defmsg != '\0';
	*mp++ = *defmsg++);

  if (*defmsg == '\0')
    return string;

  /* copy message number */
  for (*mp++ = *defmsg++; *defmsg != ':' && *defmsg != '\0';
       *mp++ = *defmsg++);
  *mp = '\0';

  return (char *)gettxt(msgid, ++defmsg);
}

/* put_string() - writes the string to the output file */

void
put_string(outputfp, string)
FILE *outputfp;	/* File pointer for the output file */
char *string;	/* String to be written */
{
	(void) pfmt(outputfp, MM_NOSTD|MM_NOGET,
			"%s\n", upgettxt(string));
}	/* put_string() */

/* put_data() - writes len char's of ascii hexidecimal data in the data
 * pointer to the output file
 */

void
put_data(outputfp, data, len)
FILE *outputfp;	/* File pointer for the output file */
unsigned char *data;	/* Data to be written */
int len;	/* Length of data */
{
	int cur;

	for (cur = 1; cur <= len; cur++) {
		(void) fprintf(outputfp, "%.2X", *data++);
		if ((cur == len) || ((cur % 32) == 0)) {
			(void) fprintf(outputfp, "\n");
		} else if ((cur % 4) == 0) {
			(void) fprintf(outputfp, " ");
		}
	}
}	/* put_data() */
