#ident	"@(#)localedef:messages.c	1.1"
#include <limits.h>
#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include "_colldata.h"
#include "_localedef.h"


/* mkmsgs format header - # of strings followed by index for each one */
static int header[12] = {11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* indexes into header for message file */
#define YESSTR	4
#define NOSTR	5
#define	YESEXPR	6
#define	NOEXPR	7
#define QUITEXPR 10
#define QUITSTR 11

static unsigned char strbuf[BUFSIZ];
/* index into string table cannot be 0 - that indicates no message present */
static unsigned char *strptr = strbuf + 1;

void
write_messages(FILE *file)
{

	int i;
	for(i = 1; i < 12; i++)
		header[i] += sizeof(header);
	
	if(fwrite(&header,sizeof(header),1, file) != 1)
		goto writeerr;

	
	if(fwrite(strbuf,sizeof(unsigned char),strptr-strbuf, file) != (strptr-strbuf))
		goto writeerr;

	return;
writeerr:
	diag(ERROR, FALSE, ":50:Error writing LC_MESSAGES data: %s\n",
			strerror(errno));
}

		


void
kf_messages_str_func(unsigned char *line, void **args)
{
	unsigned char *tdest;

	/* see time.c for rationale behind 2 *strlen(line) */
	if(BUFSIZ - (strptr -strbuf) < 2 * strlen((char *) line)) {
		diag(LIMITS,TRUE,":118:Overflow of messages buffer.\n");
		return;
	}

	if((tdest = getstring(&line,strptr,0,TRUE)) == NULL) 
		return;
	line = skipspace(line);
	if(!EOL(line)) 
		diag(WARN,TRUE, extra_char_warn);
	*tdest++ = '\0';
	header[(int) args[0]]= strptr - &strbuf[0];
	strptr = tdest;
}

extern void *kf_messages_list[];
void * kf_noexpr_list[] = { (void *) NOEXPR };
void * kf_yesexpr_list[] = { (void *) YESEXPR };
void * kf_nostr_list[] = { (void *) NOSTR };
void * kf_yesstr_list[] = { (void *) YESSTR };
void * kf_quitstr_list[] = { (void *) QUITSTR };
void * kf_quitexpr_list[] = { (void *) QUITEXPR };

struct keyword_func kf_messages_tab[] = {
	{ "END", kf_END_func, kf_messages_list },
	{ "copy", kf_copy_func, kf_messages_list},
	{ "noexpr", kf_messages_str_func,  kf_noexpr_list},
	{ "nostr", kf_messages_str_func,  kf_nostr_list},
	{ "quitexpr", kf_messages_str_func, kf_quitexpr_list},
	{ "quitstr", kf_messages_str_func, kf_quitstr_list},
	{ "yesexpr", kf_messages_str_func,  kf_yesexpr_list},
	{ "yesstr", kf_messages_str_func,  kf_yesstr_list}
};

struct kf_hdr kfh_messages = {
	8,
	kf_messages_tab
};

void *kf_messages_list[] = { (void *) "LC_MESSAGES", (void *) &kfh_messages, (void *) LC_MESSAGES, (void *) "LC_MESSAGES/Xopen_info", (void *) write_messages };
