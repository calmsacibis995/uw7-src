#ident	"@(#)localedef:numeric.c	1.1"
#include <stdio.h>
#include <locale.h>
#include "_colldata.h"
#include "_localedef.h"

static unsigned char tsep = '\0';
static unsigned char dpoint = '\0';
static char *grouping = NULL;
static char *gptr;

/* File format is:

		byte for decimal point
		byte for thousands separator
		byte(s) for grouping specification terminated by CHAR_MAX or null
*/
void
write_numeric(FILE *file)
{
	if(dpoint == '\0') {
		diag(ERROR,FALSE,":104:Must specify decimal point\n");
		return;
	}
	/* Library expects thousands separator byte to be there.  If no grouping
	   is specified, it does not matter to the library what the value is, so we
	   can leave it as a null byte.  If grouping is specified though and the 
	   thousands separator is the null byte, it will prematurely terminate
	   strings, so disallow it.
	*/
	if(grouping != NULL && tsep == '\0') {
		diag(ERROR,FALSE,":60:If grouping is specified, must also give thousands separator\n");
		return;
	}
	if(putc(dpoint,file) == EOF)
		goto writeerr;
	if(putc(tsep,file) == EOF)
		goto writeerr;
	if(grouping != NULL) {
		if(fputs(grouping,file) == EOF)
			goto writeerr;
		if(putc('\0',file) == EOF)
			goto writeerr;
		(void) free(grouping);
	}
	return;
writeerr:
	diag(ERROR,FALSE,":52:Error writing LC_NUMERIC data\n");
	return;
}

void
kf_numeric_func(unsigned char *line, void **args)
{
	unsigned char *dest = (unsigned char *) args[0];

	if((dest = getstring(&line,dest,1,FALSE)) == NULL) {
		return;
	}
	line = skipspace(line);
	if(!EOL(line))
		diag(WARN,TRUE,extra_char_warn);
	
}

/* Note that this function is also used in processing mon_grouping from
   LC_MONETARY. */
void
kf_grouping_func(unsigned char *line, void **args)
{
	int res;
	int n;
	mboolean_t endseen;
	char **grp;

	grp = (char **) args[0];
	*grp = (char *) getmem(NULL,strlen((char *) line),TRUE,TRUE);

	gptr = *grp;
	do {
		if(sscanf((char *)line, "%d %n",&res,&n) != 1) {
			diag(ERROR,TRUE,":77:Invalid grouping specifier; Must be numeric and must be at least one of them\n");
			return;
		}
		if(res < -1 || res > CHAR_MAX) {
			diag(ERROR,TRUE,":76:Invalid grouping specifier; must be greater than -1 and less than %d\n",CHAR_MAX+1);
			return;
		}
		if(res == -1)
			*gptr = CHAR_MAX;
		else
			*gptr = (char) res;
		gptr++;
		line +=n;
	} while(*line++ == ';');
	line--;
	endseen = FALSE;
	while(--gptr >= *grp) {
		if(endseen && (*gptr == '\0' || *gptr == CHAR_MAX)) {
			diag(ERROR,TRUE,":59:Grouping specifier must not contain 0x%x, -1, or 0 except as terminator\n", CHAR_MAX);
			return;
		}
		if(*gptr != '\0')
			endseen = TRUE;
	}
	if(!EOL(line))
		diag(WARN,TRUE,extra_char_warn);

}



void *kf_num_group_list[] = { (void *) &grouping };
void *kf_tsep_list[] = { (void *) &tsep };
void *kf_dpoint_list[] = { (void *) &dpoint };

extern void *kf_numeric_list[];

struct keyword_func kf_numeric_tab[] = {
	{ "END", kf_END_func, kf_numeric_list },
	{ "copy", kf_copy_func, kf_numeric_list },
	{ "decimal_point", kf_numeric_func, kf_dpoint_list },
	{ "grouping", kf_grouping_func, kf_num_group_list },
	{ "thousands_sep", kf_numeric_func, kf_tsep_list }
};

struct kf_hdr kfh_numeric = {
	5,
	kf_numeric_tab
};
void *kf_numeric_list[] = { (void *) "LC_NUMERIC", (void *) &kfh_numeric, (void *)LC_NUMERIC, (void *) "LC_NUMERIC", (void *) write_numeric };
