#ident	"@(#)kern-i386:svc/gdb.d/db_seqnum.h	1.1"
#ident	"$Header$"

#ifndef _DB_SEQNUM_H
#define _DB_SEQNUM_H

/*
   File: db_seqnum.h
   Sequence numbers.
*/

typedef unsigned char db_seqnum_t;

/* initialize sequence numbers */
extern void db_init_seqnum();

/* next transmission sequnce number */
extern db_seqnum_t  db_get_sendseqnum ();

/* increment transmission sequnce number */
extern void db_inc_sendseqnum();

/* last sequnce number received */
extern db_seqnum_t  db_get_recvseqnum ();

/* set last sequnce number received */
extern void  db_set_recvseqnum (/* db_seqnum_t */);

/* have we already seen this sequnce number ? */
extern int  db_old_recvseqnum (/* db_seqnum_t */);

#endif /* _DB_SEQNUM_H */
