#ident	"@(#)kern-i386:svc/gdb.d/db_router.c	1.1"
#ident	"$Header$"

/*
 * File: db_router.c
 * Route messages between debugging device port and processor ports.
 */

#include <proc/cg.h>
#include "db_port.h"
#include "db_seqnum.h"
#include "db_router.h"

#if 0
#define TRACE printf("File: %s, Line: %d\n", __FILE__, __LINE__)
#else
#define TRACE 1
#endif

int db_current_processor;	/* processor to which messages go */

extern void db_ultohex (char*, int, unsigned long); /* in db_stub.c */

static db_lock_t db_router_lock; /* don't execute router code in parallel */

/*
 * Initialize router.
 */

void
db_router_init ()
{
  db_init_seqnum ();
  db_current_processor = 0;
  db_init_lock (&db_router_lock);
}

/*
 * Poll all ports and route packets when ready.
 */

#define db_poll_port(p)						\
do {								\
  if (db_try_lock_port (p))					\
    {								\
      if (db_port_read_ready (p) && db_port_get_packet (p))	\
	db_route_packet (p);					\
      db_unlock_port (p);					\
    }								\
} while (0)
  
void
db_router_poll ()
{
  int i;
  db_port_t *p;

  if (db_try_lock (&db_router_lock))
    {
      p = &gdb_out;
      db_poll_port(p);		/* gdb_in/out port lock is not necessary,
				 * but is here for simetry.
				 */
      
      DB_FOR_EACH_PROC (i)
	{
	  p = &db_proc_out[i];
	  db_poll_port(p);
	}
      
      db_unlock (&db_router_lock);
    }
}

#define db_lock_put_unlock(p,buf)		\
do {						\
  db_spin_lock_port ((p));			\
						\
  if (db_port_empty((p)))			\
    {						\
      db_port_put_packet ((p),(buf));		\
    }						\
						\
  db_unlock_port ((p));				\
} while (0)

/*
   Continue processor I.
*/

void
db_cont_proc (i)
     int i;
{
  db_port_t *p = &db_proc_in[i];
  char buf[PBUFSIZ];
  
  strcpy (buf, "$c#..");
  db_ultohex (buf+3, 2, 'c');
  
  db_lock_put_unlock(p,buf);
}

/*
   Compute checksum for data in BUF.
*/

unsigned char
db_checksum (buf)
     char *buf;
{
  unsigned char csum;

  for (csum = 0, buf++; *buf != '#'; buf++)
    csum += *buf;
  
  return csum;
}

#ifdef NUMA
/* sequence number of command for which reply is awaited by router 
   e.g. when waiting for replies to B command */
static db_seqnum_t router_awaited_seqnum;

/* bitmask of engine numbers for which replies are awaited */
static int router_awaiting_replys = 0;

/* if any of responders to a command for which router sends only one */
/* reply returns error, this variable is set to non-zero */
static int awaited_reply_is_error = 0;
#endif /* NUMA */

/*
   Packet in P has arrived. Pass it to GDB device if it came from a processor
   port, or to db_current_processor if it came from GDB.

   Handle commands: A (continue all processor), Qproc= (set current processor),
   and T (processor stopped), properly.
*/

void
db_route_packet (p)
     db_port_t *p;
{
  extern void db_stop_all_proc ();
  db_port_t *to;
  int i;

  to = (p == &gdb_out) ? &db_proc_in[db_current_processor] : &gdb_in;

  if (p == &gdb_out)
    {
      switch (p->p_buf[4])
	{
	  /*
	   * Interscept command A and continue all processors.
	   */
	case 'A':
	  if (p->p_buf[5] == '#')
	    DB_FOR_EACH_ONLINE_PROC(i)
	      if (&db_proc_in[i] != to)
		db_cont_proc (i);
	  /* turn command into a continue command for the 
	     current processor */
	  p->p_buf[4] = 'c';
	  db_ultohex (p->p_buf+6, 2, db_checksum (p->p_buf));
	  break;

	  /*
	   * Interscept command Qproc=NN and set db_current_processor to NN
	   * and QDR[^S]=* and pass them to all processors.
	   */
	case 'Q':
	  if (strncmp (&p->p_buf[5], "proc=", (sizeof "proc=") - 1) == 0)
	    {
	      int newproc;
	      extern long db_hextol(char*);

	      to = &gdb_in;
	      newproc = db_hextol (&p->p_buf[10]);
	      if (newproc < GDB_MAXPROC && newproc >= 0)
		{
		  db_current_processor = newproc;
		  strcpy (p->p_buf, "$??.OK#??");
		}
	      else
		{
		  strcpy (p->p_buf, "$??.E0#??");
		}
	      db_ultohex (p->p_buf+1, 2, db_get_recvseqnum());
	      db_ultohex (p->p_buf+7, 2, db_checksum (p->p_buf));
	      
	      p->p_last = 8;
	    }
	  else if (strncmp (&p->p_buf[5], "DR", 2) == 0
		   && p->p_buf[8] == '='
		   /* don't want to overwrite debug status register of each 
		      processor */
		   && p->p_buf[7] != 'S')
	    {
	      DB_FOR_EACH_ONLINE_PROC(i)
		{
		  /*
		   * Copy the packet to the destination port db_proc_in[i].
		   */
		  if (to != &db_proc_in[i])
		    db_lock_put_unlock(&db_proc_in[i],p->p_buf);
		}
	    }
	  break;

#ifdef NUMA
	  /*
	   * Breakpoint command. Send it to each PM leader engine.
	   */
	case 'B':
	  {
	    int to_is_leader = 0;
	    int cg;

	    router_awaiting_replys = 0;
	    router_awaited_seqnum = db_get_recvseqnum ();

	    TRACE;

	    DB_FOR_EACH_ONLINE_PM(cg)
	      {
		i = cg_array[cg].cg_cpuid[0];
		/*
		 * Copy the packet to the destination port db_proc_in[i].
		 */
		if (to != &db_proc_in[i])
		  {
		    TRACE;
		    db_lock_put_unlock(&db_proc_in[i],p->p_buf);
		  }
		else
		  {
		    TRACE;
		    to_is_leader = 1;
		  }
		router_awaiting_replys |= (1 << i);
	      }
	    if (!to_is_leader)
	      {
		TRACE;
		goto leave;
	      }
	    TRACE;
	    break;
	  }
#endif /* NUMA */

	}
    }
  else
    {
      /* 
       * Interscept command TAA... and stop all other processors
       * Assuming command T is sent by processor when it stops.
       */
      if (p->p_buf[1] == 'T')
	db_stop_all_other_proc ();

#ifdef NUMA
      /* see if we got a reply we were waiting for */
      else if (router_awaiting_replys &&
	       db_hextol (&p->p_buf[1]) == router_awaited_seqnum)
	{
	  TRACE;
	  router_awaiting_replys &= ~(1 << (int)p->p_info);
	  
	  if (p->p_buf[4] == 'E')
	    awaited_reply_is_error = 1;

	  /* if we got all the replys, then send it back to host */
	  if (router_awaiting_replys == 0)
	    {
	      TRACE;
	      if (awaited_reply_is_error)
		{
		  TRACE;
		  to = &gdb_in;
		  strcpy (p->p_buf, "$??.E3#??");
		  db_ultohex (p->p_buf+1, 2, router_awaited_seqnum);
		  db_ultohex (p->p_buf+7, 2, db_checksum (p->p_buf));
		  p->p_last = 8;
		}
	      router_awaited_seqnum = 0;
	      awaited_reply_is_error = 0;
	    }
	  else
	    {
	      TRACE;
	      goto leave;
	    }
	  TRACE;
	}
#endif /* NUMA */
    }

  /*
   * Finally copy the packet to the destination port TO.
   */
  db_lock_put_unlock(to,p->p_buf);

 leave:
  /*
   * The source port is available again.
   */
  db_clear_port(p);
}
