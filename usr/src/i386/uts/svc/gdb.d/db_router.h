#ident	"@(#)kern-i386:svc/gdb.d/db_router.h	1.1"
#ident	"$Header$"

#ifndef _DB_ROUTER_H
#define _DB_ROUTER_H

/*
   File: db_router.h
*/

#include <util/engine.h>
#include "db_port.h"

#ifndef GDB_MAXPROC
#error GDB_MAXPROC must be defined.
#endif

/* initialize router */
extern void db_router_init ();

/* poll each of _out ports and route packets as necessary */
extern void db_router_poll ();

/* route packet from source port to one of GDB _in port or current 
   processor _in port */
extern void db_route_packet (/* port_t * */);

/* compute data checksum - assumes buf[0]=='$' and buf[i]=='#', i>0
   makes sum of characters between '$' and '#', noninclusively */
extern unsigned char db_checksum (/* char *buf */);

/* processor input ports */
extern db_port_t db_proc_in[GDB_MAXPROC];

/* processor output ports */
extern db_port_t db_proc_out[GDB_MAXPROC];

/* debugger ports */
extern db_port_t gdb_in;
extern db_port_t gdb_out;

/* loop through all processor ids with VAR being current id in the body */
extern int Nengine;
#define DB_FOR_EACH_PROC(var)			\
 for (var = 0; var < Nengine; var++)

/* loop through online engines */
#ifdef UNIPROC
#define DB_FOR_EACH_ONLINE_PROC(var)		\
  DB_FOR_EACH_PROC(var)
#else
#define DB_FOR_EACH_ONLINE_PROC(var) 		\
  for (var = 0; var < Nengine; var++)		\
    if (!(engine[var].e_flags & E_NOWAY))
#endif

#ifdef NUMA
/* loop through all PMs */
#define DB_FOR_EACH_PM(var)			\
 for (var = 0; var < Ncg; var++)

/* loop through online PMs */
#define DB_FOR_EACH_ONLINE_PM(var)		\
 for (var = 0; var < Ncg; var++)		\
   if (IsCGOnline(var))
#endif /* NUMA */

#endif /* _DB_ROUTER_H */
