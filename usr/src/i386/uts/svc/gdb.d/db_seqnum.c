#ident	"@(#)kern-i386:svc/gdb.d/db_seqnum.c	1.1"
#ident	"$Header$"

/*
   File: db_seqnum.c
   Sequence numbers.
*/

#include "db_seqnum.h"

static db_seqnum_t sendseqnum;	/* next transmit sequence number */
static db_seqnum_t recvseqnum;	/* last receive sequence number */
static int firstseq;		/* first message flag */

void
db_init_seqnum()
{
  firstseq = 1;
  sendseqnum = 1;
  recvseqnum = 1;
}

db_seqnum_t
db_get_sendseqnum ()
{
  return sendseqnum;
}

void
db_inc_sendseqnum()
{
  sendseqnum++;
  if (sendseqnum == 0)
    sendseqnum = 1;
}

db_seqnum_t
db_get_recvseqnum ()
{
  return recvseqnum;
}

void
db_set_recvseqnum (seq)
     db_seqnum_t seq;
{
  recvseqnum = seq;
}

int
db_old_recvseqnum (seq)
     db_seqnum_t seq;
{
  if (firstseq)
    {
      firstseq = 0;
      recvseqnum = seq-1;
      return 0;
    }
  return seq == recvseqnum;
}
