#ident "@(#)refclock_ptbacts.c	1.2"

/*
 * crude hack to avoid hard links in distribution
 * and keep only one ACTS type source for different
 * ACTS refclocks
 */
#if defined(REFCLOCK) && defined(PTBACTS)
#define KEEPPTBACTS
#undef ACTS
#include "refclock_acts.c"
#endif
