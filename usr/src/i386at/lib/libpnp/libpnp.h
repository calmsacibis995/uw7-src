
/*
 *	@(#)libpnp.h	7.1	10/22/97	12:21:23
 * libpnp.h
 */


/* EISA vendor string conversion routines */
const char *PnP_idStr(unsigned long vendor);
unsigned long PnP_idVal(const char *idStr);

