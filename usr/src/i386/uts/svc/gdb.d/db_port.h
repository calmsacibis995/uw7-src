#ident	"@(#)kern-i386:svc/gdb.d/db_port.h	1.1"
#ident	"$Header$"

#ifndef _DB_PORT_H
#define _DB_PORT_H

/*
   File: db_port.h

   Port structure, used by debugger router and debugger stub.
*/

#include "db_lock.h"

typedef struct db_port_ops {
  int (*po_read_ready) (/* struct db_port* */);
  int (*po_get_packet) (/* struct db_port* */);
  int (*po_put_packet) (/* struct db_port*, char *buf */);
} db_port_ops_t;

#define PBUFSIZ 400

typedef struct db_port {
  db_port_ops_t *p_ops;		/* port operations */
  char p_buf[PBUFSIZ];		/* port data */
  int p_last;			/* index of the last character in the buffer */
  int p_first;			/* index of the next char minus one */
  void *p_info;			/* port specific data (often owner id) */
  char p_lock;			/* port mutex */
} db_port_t;


/* clear port data and reset p_first and p_last to -1 */
extern void db_clear_port (/* db_port_t * */);

/* initialize a port */
extern void db_init_port (/* db_port_t *, db_port_ops_t *, void * */);

/* append character to the data in port buffer */
extern void db_port_append_char (/* db_port_t *, char */);

/* return next character from the port buffer and modify p_first, 
   return -1 if no more characters */
extern char db_port_next_char (/* db_port_t * */);

/* nonblocking, try lock, return 1 if succeeded locking, 0 otherwise */
#define db_try_lock_port(p) (db_try_lock(&(p)->p_lock))

/* unlock port (doesn't have to be locked */
#define db_unlock_port(p) (db_unlock(&(p)->p_lock))

/* spin until lock is acquired */
#define db_spin_lock_port(p) db_spin_lock(&(p)->p_lock)

/* any data available? */
#define db_port_read_ready(p) ((*p->p_ops->po_read_ready) (p))

/* returns 1 if packet is ready, 0 otherwise */
#define db_port_get_packet(p) ((*p->p_ops->po_get_packet) (p))

/* store data from buf into the port, 
   interrupt the port owner unless stopped */
#define db_port_put_packet(p,buf) ((*p->p_ops->po_put_packet) (p,buf))

/* port empty? */
#define db_port_empty(p) ((p)->p_last == -1)

/* port full? */
#define db_port_full(p) ((p)->p_last > 2 && (p)->p_buf[(p)->p_last-2] == '#')

#endif /* _DB_PORT_H */
