#ident	"@(#)kern-i386:svc/gdb.d/db_port.c	1.1"
#ident	"$Header$"

/*
   File: db_port.c
   Debugger interprocessor communication port implementation.
*/

#include <stdio.h>
#include "db_port.h"

/*
   Initialize all fields of a port.
*/

void
db_init_port (p, ops, info)
     db_port_t *p;
     db_port_ops_t *ops;
     void *info;
{
  p->p_ops = ops;
  db_init_lock (&p->p_lock);
  p->p_info = info;
  db_clear_port(p);
}

/*
   Clear port data.
*/

void
db_clear_port (p)
     db_port_t *p;
{
  p->p_buf[0] = 0;
  p->p_last = -1;
  p->p_first = -1;
}

/*
   Append a character to port data.
*/

void
db_port_append_char (p, c)
     db_port_t *p;
     char c;
{
  p->p_buf[++p->p_last] = c;
  p->p_buf[p->p_last+1] = 0;
}

/*
   Return next character from the port buffer.
*/

char
db_port_next_char (p)
     db_port_t *p;
{
  if (p->p_first >= p->p_last)
    {
      
      return -1;
    }
  p->p_first++;
  return p->p_buf[p->p_first];
}
