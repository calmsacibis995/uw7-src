#ident	"@(#)kern-i386:svc/gdb.d/db_lock.c	1.1"
#ident	"$Header$"

/*
   File: db_lock.c
   Simple MUTEX locks implementation.
*/

#include "db_lock.h"

/*
   Init - initialize lock to 0 -- unlocked state.
*/
void
db_init_lock (lck)
     db_lock_t *lck;
{
  *lck = 0;
}

/*
   Lock - set lock to 0 and return previous value. If the
   value returned is 0, than the operation failed, otherwise, operation
   suceeded and lock is set.
*/

int
db_try_lock (lck)
     db_lock_t *lck;
{
  int prev;

  asm ("movl	$1,%eax");
  asm ("movl	8(%ebp),%ecx");
  asm ("xchg	(%ecx),%eax");
  asm ("movl	%eax,-4(%ebp)");

  return !prev;
}

/*
   Unlock - set lock to 0 and return previous value.
   Works with both locked and unlocked locks.
*/

int
db_unlock (lck)
     register db_lock_t *lck;
{
  int prev;

  asm ("movl	$0,%eax");
  asm ("movl	8(%ebp),%ecx");
  asm ("xchg	(%ecx),%eax");
  asm ("movl	%eax,-4(%ebp)");

  return prev;
}
