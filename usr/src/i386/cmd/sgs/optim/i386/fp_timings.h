#ident	"@(#)optim:i386/fp_timings.h	1.3"
#ifndef FP_TIMINGS_H
#define FP_TIMINGS_H
#include "optim.h"
typedef struct times_s {
			int t_387;
			int t_486;
			int t_ptm;
		} times_t;

typedef struct opop_s {
		unsigned int op;
		char *op_code;
		} opopcode;

extern int gain_by_mem2st();
extern opopcode mem2st();
#endif
