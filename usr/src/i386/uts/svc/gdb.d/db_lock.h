#ident	"@(#)kern-i386:svc/gdb.d/db_lock.h	1.1"
#ident	"$Header$"

#ifndef _DB_LOCK_H
#define _DB_LOCK_H

/*
   File: db_lock.h
   Simple MUTEX locks - interface.
*/

typedef int db_lock_t;

/* initialize lock. value of 1 means locked, 0 means unlocked */
extern void db_init_lock (/* db_lock_t *lck */);

/* try locking, but don't block. return 1 if succeeded locking, 0 otherwise */
extern int  db_try_lock (/* db_lock_t *lck */);

/* unlock the lock. if not locked, do nothing */
extern int  db_unlock (/* db_lock_t *lck */);

/* spin until the lock is acquired */
#define db_spin_lock(lck)			\
  do {						\
    while (*(lck)) 				\
      ;						\
  } while (!db_try_lock((lck)))

#endif /* _DB_LOCK_H */
