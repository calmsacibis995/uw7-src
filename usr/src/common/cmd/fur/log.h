#ident	"@(#)fur:common/cmd/fur/log.h	1.1.2.1"
#ifndef _LOG_H
#define _LOG_H

struct flowcount {
	unsigned long callcount;
	unsigned long firstcount;
	unsigned long secondcount;
};
struct blockcount {
	unsigned long callcount;
	unsigned long firstblock;
	unsigned long firstcount;
	unsigned long secondcount;
};
struct callcount {
	void *callee;
	void *caller;
	unsigned long count;
};
#endif
