#ident	"@(#)localedef:time.c	1.1.1.1"
#include <limits.h>
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include "_colldata.h"
#include "_localedef.h"

/*  The output format of the LC_TIME files is a set of required (but possibly 
    empty strings followed by some optional tagged strings (for era stuff and
	alt_digits.)  The #defines below give indexes into a table to save the 
	strings for output.  For the ordered ones, they are in the order required
	by the file format leaving spaces for multiple strings where that is 
	required (e.g., there are 12 slots for ABMON before the index for MON).
*/

#define ABMON 0
#define MON 12
#define ABDAY 24
#define DAY 31
#define T_FMT 38
#define D_FMT 39
#define D_T_FMT 40
#define AM_PM 41
#define DATECMD_FMT 43
#define T_FMT_AMPM 44
#define SEPARATOR 45
#define ALT_DIGITS 46
#define ERA 47
#define ERA_T_FMT 48
#define ERA_D_T_FMT 49
#define ERA_D_FMT 50
#define MAX_TIME_TAB 51

static unsigned char *time_tab[MAX_TIME_TAB];


/* Buffer management is done using dynamic allocation to allow maximum 
   flexibility in the size of the strings.  The value 2*strlen(line) seems a 
   safe max for the variable part of the string since the string is either 
   composed of characters from  the portable codeset (1 byte per) or symbolic
   characters (which have 3 bytes minimum on input <x>) or numeric constants 
   (which have a 7 byte minimum on input for multibyte characters \xff\xf).
   Given we only support 6 byte long multibyte characters, the output buffer 
   will never be exhausted.
*/
static unsigned char *buf = NULL;
static unsigned int buflen = 0;

void
kf_time_ext_func(unsigned char *line, void **args)
{
	unsigned int i = 0;
	unsigned char *bufptr;
	unsigned int num = (unsigned int) args[2];
	unsigned int index = (unsigned int) args[0];
	unsigned char *prefix = (unsigned char *) args[1];

	if(time_tab[index] != NULL) {
		diag(ERROR, TRUE, ":1:Attempt to redefine LC_TIME subcategory\n");
		return;
	}
	if(strlen((char *) prefix) + 1 + strlen((char *) line) * 2 > buflen) {
		buflen = strlen((char *) prefix) + 1 + strlen((char *) line) * 2;
		buf = (unsigned char *) getmem(buf,buflen,TRUE,TRUE);
	}
	(void) strcpy((char *) buf,(char *) prefix);
	bufptr = buf + strlen((char *) prefix);
	*bufptr++ = ' ';
	do {
		*bufptr++ = '"';  /* in anticiaption of valid string */
		if((bufptr = getstring(&line,bufptr,0,TRUE)) == NULL)
			return;
		*bufptr++ = '"';  
		line = skipspace(line);
		if(*line != ';') {
			break;
		}
		*bufptr++ = *line++;	/* put in the ';' */
	} while(i++ < num);

	/* test works because all keywords requiring exact number of args
	   require exactly one argument */
	if(i >= num) {
		diag(ERROR,TRUE,":54:Expected at most %d values; found %d values\n",num,i+1);
		return;
	}
	if(!EOL(line))
		diag(WARN, TRUE, extra_char_warn);
	*bufptr = '\0';
	if(strchr((char *) buf,'\n') != NULL) {
		diag(LIMITS,TRUE,":105:Newlines ('\n') not permitted in string\n");
		return;
	}
	time_tab[index] = (unsigned char *)
			strcpy(getmem( NULL,strlen((char *)buf)+1,FALSE,TRUE),(char *) buf);
}

void
kf_time_func(unsigned char *line, void **args)
{
	unsigned int i = 0;
	unsigned char *bufptr;
	unsigned int num = (unsigned int) args[1];
	unsigned int index = (unsigned int) args[0];

	if(time_tab[index] != NULL) {
		diag(ERROR, TRUE, ":1:Attempt to redefine LC_TIME subcategory\n");
		return;
	}

	if(strlen((char *) line) * 2 > buflen) {
		buflen = strlen((char *) line) * 2;
		buf = (unsigned char *) getmem(buf,buflen,TRUE,TRUE);
	}

	do {
		bufptr = buf;
		if((bufptr = getstring(&line,bufptr,0,TRUE)) == NULL)
			return;
		*bufptr = '\0';
		if(strchr((char *) buf,'\n') != NULL) {
			diag(LIMITS,TRUE,":105:Newlines ('\n') not permitted in string\n");
			return;
		}

		time_tab[index+i] = (unsigned char *)
			  strcpy(getmem((char *)NULL,strlen((char *)buf)+1,FALSE,TRUE),(char *) buf);
		line = skipspace(line);
		if(*line != ';')
			break;
		line++;
	} while(i++ < num);

	if (i != num - 1) {
		diag(ERROR,TRUE,":55:Expected %d values; found %d values\n",num,i+1);
		return;
	}

	if(!EOL(line))
		diag(WARN, TRUE, extra_char_warn);

}
			
void
write_time(FILE *file)
{
	unsigned int i;

	time_tab[SEPARATOR] = (unsigned char *) "%";

	for(i = 0; i < MAX_TIME_TAB; i++) {
		if(time_tab[i] == NULL) {
			/* DATECMD_FMT is an extension to the std.  It is a required 
			   field in our file format.  It seems reasonable to default it
			   to the date time format if the extension is not used.
			*/
			if(i == DATECMD_FMT && time_tab[D_T_FMT] != NULL) {
					fprintf(file,"%s\n",time_tab[D_T_FMT]);
					continue;
			}
			/* only need to put out blank lines for strings required by file 
			   format 
			*/
			if(i < ALT_DIGITS)
				putc('\n', file);
		}
		else {
			fprintf(file,"%s\n",time_tab[i]);
			if(i == SEPARATOR)
				continue;
			if(i != D_T_FMT)
				(void) free(time_tab[i]);
			if(i == DATECMD_FMT)
				(void) free(time_tab[D_T_FMT]);
		}
	}
	(void) free(buf);
	
}

extern void *kf_time_list[];
void * kf_abday_list[] = { (void *) ABDAY, (void *) 7 };
void * kf_abmon_list[] = { (void *) ABMON, (void *) 12 };
void * kf_am_pm_list[] = { (void *) AM_PM, (void *) 2 };
void * kf_d_fmt_list[] = { (void *) D_FMT, (void *) 1 };
void * kf_d_t_fmt_list[] = { (void *) D_T_FMT, (void *) 1 };
void * kf_datecmd_fmt_list[] = { (void *) DATECMD_FMT, (void *) 1 };
void * kf_day_list[] = { (void *) DAY, (void *) 7 };
void * kf_mon_list[] = { (void *) MON, (void *) 12 };
void * kf_t_fmt_list[] = { (void *) T_FMT, (void *) 1 };
void * kf_t_fmt_ampm_list[] = { (void *) T_FMT_AMPM, (void *) 1 };
void * kf_era_list[] = { (void *) ERA, (void *) "era", (void *) UINT_MAX };
void * kf_era_d_fmt_list[] = { (void *) ERA_D_FMT, (void *) "era_d_fmt", (void *) 1 };
void * kf_era_t_fmt_list[] = { (void *) ERA_T_FMT, (void *) "era_t_fmt", (void *) 1 };
void * kf_era_d_t_fmt_list[] = { (void *) ERA_D_T_FMT, (void *) "era_d_t_fmt", (void *) 1 };
void * kf_alt_digits_list[] = { (void *) ALT_DIGITS, (void *) "alt_digits", (void *) 100 };

struct keyword_func kf_time_tab[] = {
	{ "END", kf_END_func, kf_time_list },
	{ "abday", kf_time_func, kf_abday_list},
	{ "abmon", kf_time_func, kf_abmon_list},
	{ "alt_digits", kf_time_ext_func, kf_alt_digits_list},
	{ "am_pm", kf_time_func, kf_am_pm_list},
	{ "copy", kf_copy_func, kf_time_list},
	{ "d_fmt", kf_time_func, kf_d_fmt_list},
	{ "d_t_fmt", kf_time_func, kf_d_t_fmt_list},
	{ "datecmd_fmt", kf_time_func, kf_datecmd_fmt_list},
	{ "day", kf_time_func, kf_day_list},
	{ "era", kf_time_ext_func, kf_era_list},
	{ "era_d_fmt", kf_time_ext_func, kf_era_d_fmt_list},
	{ "era_d_t_fmt", kf_time_ext_func, kf_era_d_t_fmt_list},
	{ "era_t_fmt", kf_time_ext_func, kf_era_t_fmt_list},
	{ "mon", kf_time_func, kf_mon_list},
	{ "t_fmt", kf_time_func, kf_t_fmt_list},
	{ "t_fmt_ampm", kf_time_func, kf_t_fmt_ampm_list}
};

struct kf_hdr kfh_time = {
	17,
	kf_time_tab
};

void *kf_time_list[] = { (void *) "LC_TIME", (void *) &kfh_time, (void *) LC_TIME, (void *) "LC_TIME", (void *) write_time };
