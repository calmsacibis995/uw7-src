#ifndef _PPP_TYPE_H
#define _PPP_TYPE_H

#ident	"@(#)ppp_type.h	1.3"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/ppp_pcid.h>
#include <synch.h>

#include "ppp_param.h"

/*
 * When used for incoming messages, the data pointer moves along the
 * message, length being decreased, as fields are consumed.
 *
 * For messages that are being generated for transmission, the data pointer
 * doesn't move, the place to write to is pp_data + pp_len, (we use 1500 byte
 * chunks of memory for outgoing messages).
 */
typedef struct db_s {
	unsigned char	*db_base;	/* Start of memory chunk */
	unsigned char	*db_lim;	/* Addr of byte following this chunk */
	unsigned char	*db_rptr;	/* Read position */
	unsigned char	*db_wptr;	/* Write position */
	uint_t		db_ref;		/* Reference count */
	int		db_band;	/* Q-Band for sent messages */
	struct ppp_inf_ctl_s db_ctl;	/* Control info */
} db_t;

#define db_func		db_ctl.ctl_func
#define db_cmd db_ctl.ctl_cmd
#define db_error db_ctl.ctl_error
#define db_proto db_ctl.ctl_proto
#define db_lindex	db_ctl.ctl_lindex
#define db_bindex	db_ctl.ctl_bindex

/*
 * This structure is used to represent atomic integers
 * See util.c for functions used to manipulate them.
 */
struct atomic_int_s {
	mutex_t lock;
	int ref;
};

typedef struct atomic_int_s atomic_int_t;

#endif /* _PPP_TYPE_H */
