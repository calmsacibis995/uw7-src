#ifndef _PRED1_CFG_H
#define _PRED1_CFG_H

#ident	"@(#)cfg.h	1.2"

/*
 * Predictor 1 Compression specific options
 */
struct cfg_pred1 {
	struct cfg_hdr 	pred1_ch;
	uint_t          pred1_name; /* The protocol name */
	char pred1_var[1]; /*Strings*/
};

#endif /*_PRED1_CFG_H */
