#ident	"@(#)localedef:parse.c	1.1"
#include <sys/types.h>
#include <stdio.h>
#include "_colldata.h"
#include "_localedef.h"

struct kf_hdr *cur_hdr;

void
parse(void)
{
	unsigned char *line_remain;
	struct keyword_func *kf;
	extern int kyseen;

	while((line_remain = getline())!= NULL) {
		if((kf = getkeyword(&line_remain, cur_hdr)) == NULL) {
			diag(ERROR,TRUE, ":144:Unknown keyword \"%s\"\n",line_remain);
			continue;
		}
		(*kf->kf_func)(line_remain, kf->kf_list);
		kyseen++;
	}
}
