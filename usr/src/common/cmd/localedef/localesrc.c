#ident	"@(#)localedef:localesrc.c	1.1"
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include "_colldata.h"
#include "_localedef.h"


void
kf_comesc_func(unsigned char *line_remain, void **args)
{
	line_remain = skipspace(line_remain);
	if(*line_remain == escape_char)
		line_remain++;
	if(isgraph(*line_remain)) {
		*((unsigned char *) args[0]) = *line_remain++;
		line_remain = skipspace(line_remain);
		if(!EOL(line_remain))
			diag(WARN, TRUE, extra_char_warn);
		return;
	}
	diag(ERROR,TRUE,":74:Invalid comment or escape character\n");
}

static struct kf_hdr *prev_hdr; /* keeps track of where to go back to */

/* track if been here and error information on per category basis 
   mechanism assumes that no errors in default file - otherwise it will
   look like section already processed by the time we get to user data */
int been_here[LC_ALL+1];

/* tracks number of keywords processed between START and END of categories
	used only for error processing with copy directive */
int kyseen;

/* used to indicate that have processed copy directive within current 
   category */
static mboolean_t copyflg = FALSE;

void
kf_END_func(unsigned char *line_remain, void **args)
{

	line_remain = skipspace(line_remain);
	if(strncmp((char *) args[0],(char *) line_remain,strlen((char *) args[0])) != 0)  {
		diag(ERROR, TRUE, ":87:Malformed END statement at end of %s section\n",(char *) args[0]);
	}
	line_remain +=strlen((char *) args[0]);
	line_remain = skipspace(line_remain);
	if(!EOL(line_remain)) {
		diag(WARN, TRUE, extra_char_warn);
	}

	/* put back context */
	cur_hdr = prev_hdr;

	if(copyflg) {
		if(kyseen != 1)
			diag(ERROR,TRUE,":114:Other statements in section %s after \"copy\"\n",args[0]);
		return;
	}

	if(softdefn)
		return;

	output((unsigned char *)args[3],(int (*)())args[4]);

	if(been_here[curindex] == 0) {
		pfmt(stdout,MM_INFO,":128:Successfully processed %s section\n", args[0]);
		been_here[curindex]++;
	}


	return;
}


void
kf_START_func(unsigned char *line_remain, void **args)
{

	if(been_here[(int) (args[2])] != 0) {
		diag(FATAL,TRUE,":2:Attempt to redefine %s section\n",(char *) args[0]);
	}
	copyflg = FALSE;
	kyseen = 0;
	line_remain = skipspace(line_remain);
	if(!EOL(line_remain)) 
		diag(WARN,TRUE,extra_char_warn);
	
	/* Save old environment and install new one so parse will now 
	   operate on new environment */
	prev_hdr = cur_hdr;
	curindex = (unsigned int) args[2];
	cur_hdr = (struct kf_hdr *) args[1];
	return;
}

void
kf_SKIP_func(unsigned char *line, void **args)
{
	while((line = getline()) != NULL) {
		if(strncmp("END ",(char *) line,4) == 0 &&
			strncmp(args[0], (char *) line+4, strlen(args[0])) == 0 &&
			  !isgraph(*(line + strlen(args[0]) + 5))) 
				return;
	
	}
}

/* handle copy directive for all categories */
void
kf_copy_func(unsigned char *line, void **args)
{

	unsigned char *fname, *oldname, oldcomment, oldescape, *dest;
	struct kf_hdr *oldkfh, *oldprevkfh;
	int oldline;
	FILE *oldFILE;
	int i;
	mboolean_t topflg = FALSE;

	line = skipspace(line);
	if(*line == '"') {
		line++;
		fname = line;
		dest = line;
		while(*line && *line != '"') {
			if(*line == escape_char)
				line++;
			*dest++ = *line++;
		}
		i= dest-fname;
		if(*line++ != '"') {
			diag(ERROR,TRUE,":86:Malformed \"copy\" statement: Missing closing quotation mark\n");
			return;
		}
	}
	else {
		fname = line;
		dest = line;
		while(isgraph(*line)) {
			if(*line == escape_char)
				line++;
			*dest++ = *line++;
		}
		i=dest-fname;
	}
	fname = (unsigned char *)
		 strncpy(getmem(NULL,i+1,TRUE,TRUE),(char *)fname,i);
	line = skipspace(line);
	if(!EOL(line))
		diag(WARN,TRUE,extra_char_warn);

	if(kyseen != 1) 
		diag(ERROR,TRUE,":115:Other statements in section %s besides \"copy\"\n",args[0]);

	oldFILE = curFILE;
	oldname = curfile;
	oldline = curline;
	oldkfh = cur_hdr;
	oldprevkfh = prev_hdr;
	oldcomment = comment_char;
	oldescape = escape_char;
	for(i=0;i < kfh_localesrc.kfh_numkf;i++) {
		if(kfh_localesrc.kfh_arraykf[i].kf_func == kf_START_func &&
		   kfh_localesrc.kfh_arraykf[i].kf_list[1] != cur_hdr) {
			kfh_localesrc.kfh_arraykf[i].kf_func = &kf_SKIP_func;
			topflg = TRUE;
		}
	}
	process(fname,&kfh_localesrc,softdefn);

	if(topflg)
		for(i=0;i < kfh_localesrc.kfh_numkf;i++) {
			if(kfh_localesrc.kfh_arraykf[i].kf_func == kf_SKIP_func)
				kfh_localesrc.kfh_arraykf[i].kf_func = &kf_START_func;
		}
	kyseen = 0;
	curFILE = oldFILE;
	curfile = oldname;
	curline = oldline;
	cur_hdr = oldkfh;
	prev_hdr = oldprevkfh;
	comment_char = oldcomment;
	escape_char = oldescape;
	(void) free(fname);
	copyflg = TRUE;
	if(been_here[curindex] == 0) {
		diag(WARN,TRUE,":109:No section %s found\n",args[0]);
		been_here[curindex]++;
	}
}

extern void *kf_collate_list[], *kf_numeric_list[], *kf_time_list[], *kf_monetary_list[], *kf_ctype_list[],*kf_messages_list[];

void *kf_comment_list[] = { (void *) &comment_char };
void *kf_escape_list[] = { (void *) &escape_char };

struct keyword_func kf_localesrc_tab[] =  {
  { "LC_COLLATE", kf_START_func, kf_collate_list },
  { "LC_CTYPE", kf_START_func, kf_ctype_list },
  { "LC_MESSAGES", kf_START_func, kf_messages_list },
  { "LC_MONETARY", kf_START_func, kf_monetary_list },
  { "LC_NUMERIC", kf_START_func, kf_numeric_list },
  { "LC_TIME", kf_START_func, kf_time_list },
  { "comment_char", kf_comesc_func, kf_comment_list },
  { "escape_char", kf_comesc_func, kf_escape_list }
};

struct kf_hdr kfh_localesrc = {
	8,
	kf_localesrc_tab
};

