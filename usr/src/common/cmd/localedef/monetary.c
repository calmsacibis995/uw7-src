#ident	"@(#)localedef:monetary.c	1.1"
#include <limits.h>
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include "_colldata.h"
#include "_localedef.h"

extern void kf_grouping_func(unsigned char *line, void **args);

/* File format is:
	struct lconv (with all unsigned char * as index to string table)
	string table
*/

/* initialize lconv with default values */
static struct lconv p = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CHAR_MAX, CHAR_MAX, 
CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX};

static unsigned char strbuf[BUFSIZ];
/* index of zero means empty string */
static unsigned char *strptr = strbuf + 1;

static char *mon_grp = NULL;

void
write_monetary(FILE *file)
{
	/* could have been multibyte characters so check here */
	if(strlen((char *) strbuf[(int) p.decimal_point]) > 1 ||
		strlen((char *) strbuf[(int) p.thousands_sep]) > 1) {
		diag(LIMITS,FALSE, ":96:Monetary decimal point and thousands separator may be one single byte character each\n");
		return;
	}

	/* put monetary grouping at end since need to copy from buffer - 
	   not directly in strbuf as convenience to use numeric grouping func */
	if(mon_grp != NULL) 
		p.mon_grouping = (char *) (strptr - strbuf);

	if(fwrite(&p, sizeof(struct lconv), 1, file) != 1) {
		goto writeerr;
	}
	if(fwrite(strbuf,sizeof(unsigned char),strptr - strbuf,file) != strptr - strbuf)
		goto writeerr;

	if(mon_grp != NULL) {
		if(fputs(mon_grp,file) == EOF)
			goto writeerr;
		if(putc('\0',file) == EOF)
			goto writeerr;
		(void) free(mon_grp);
	}
	return;
writeerr:
	diag(ERROR, FALSE, ":51:Error writing LC_MONETARY data: %s\n",
			strerror(errno));
}



void
kf_monet_int_func(unsigned char *line, void **args)
{
	int tdest;
	int n;
	unsigned char *dest = (unsigned char *) args[0];
	int low = (int) args[1];
	int high = (int) args[2];

	if(sscanf((char *)line, "%d %n", &tdest, &n) == 1 &&
	   tdest >= low && tdest <= high) {
		*dest = tdest;
		line += n;
		if(!EOL(line))
			diag(WARN, TRUE, extra_char_warn);
		return;
	}

	diag(ERROR, TRUE, ":65:Integer argument between %d and %d required\n", low, high);
}

void
kf_monet_str_func(unsigned char *line, void **args)
{
	unsigned char *tdest;

	/* see time.c for rationale behind 2 *strlen(line) */
	if(BUFSIZ - (strptr -strbuf) < 2 * strlen((char *) line)) {
		diag(LIMITS,TRUE,":72:Overflow of buffer.\n");
		return;
	}
	if((tdest = getstring(&line,strptr,(int) args[1],TRUE)) == NULL)
		return;
	if(!EOL(line)) 
		diag(WARN,TRUE, extra_char_warn);
	*tdest++ = '\0';
	*((unsigned char **) args[0]) = (unsigned char *) (strptr - strbuf);
	strptr = tdest;
}

void *kf_currency_list[] = { (void *) &p.currency_symbol, (void *) 0};
void *kf_frac_list[] = { (void *) &p.frac_digits, (void *) 0, (void *) (CHAR_MAX - 1)};
void *kf_int_curr_list[] = { (void *) &p.int_curr_symbol, (void *) 4};
void *kf_int_frac_list[] = { (void *) &p.int_frac_digits, (void *) 0, (void *) (CHAR_MAX -1)};
void *kf_mon_decimal_list[] = { (void *) &p.mon_decimal_point, (void *) 1};
void *kf_mon_thou_list[] = { (void *) &p.mon_thousands_sep, (void *) 1};
void *kf_n_cs_precedes_list[] = { (void *) &p.n_cs_precedes, (void *) 0, (void *) 1};
void *kf_n_sep_space_list[] = { (void *) &p.n_sep_by_space, (void *) 0, (void *) 1};
void *kf_n_sign_posn_list[] = { (void *) &p.n_sign_posn, (void *) 0, (void *) 4};
void *kf_negative_sign_list[] = { (void *) &p.negative_sign, (void *) 0};
void *kf_p_cs_precedes_list[] = { (void *) &p.p_cs_precedes, (void *) 0, (void *) 1};
void *kf_p_sep_space_list[] = { (void *) &p.p_sep_by_space, (void *) 0, (void *) 1};
void *kf_p_sign_posn_list[] = { (void *) &p.p_sign_posn, (void *) 0, (void *) 4};
void *kf_postive_sign_list[] = { (void *) &p.positive_sign, (void *) 0};

void *kf_mon_group_list[] = { (void *) &mon_grp };

extern void *kf_monetary_list[];
struct keyword_func kf_monetary_tab[] = {
	{ "END", kf_END_func, kf_monetary_list },
	{ "copy", kf_copy_func, kf_monetary_list},
	{ "currency_symbol", kf_monet_str_func,  kf_currency_list},
	{ "frac_digits", kf_monet_int_func,  kf_frac_list},
	{ "int_curr_symbol", kf_monet_str_func,  kf_int_curr_list},
	{ "int_frac_digits", kf_monet_int_func,  kf_int_frac_list},
	{ "mon_decimal_point", kf_monet_str_func,  kf_mon_decimal_list},
	{ "mon_grouping", kf_grouping_func, kf_mon_group_list },
	{ "mon_thousands_sep", kf_monet_str_func,  kf_mon_thou_list},
	{ "n_cs_precedes", kf_monet_int_func,  kf_n_cs_precedes_list},
	{ "n_sep_by_space", kf_monet_int_func,  kf_n_sep_space_list},
	{ "n_sign_posn", kf_monet_int_func,  kf_n_sign_posn_list},
	{ "negative_sign", kf_monet_str_func,  kf_negative_sign_list},
	{ "p_cs_precedes", kf_monet_int_func,  kf_p_cs_precedes_list},
	{ "p_sep_by_space", kf_monet_int_func,  kf_p_sep_space_list},
	{ "p_sign_posn", kf_monet_int_func,  kf_p_sign_posn_list},
	{ "positive_sign", kf_monet_str_func, kf_postive_sign_list}
};

struct kf_hdr kfh_monetary = {
	17,
	kf_monetary_tab
};

void *kf_monetary_list[] = { (void *) "LC_MONETARY", (void *) &kfh_monetary, (void *) LC_MONETARY, (void *) "LC_MONETARY", (void *) write_monetary };
