#ident	"@(#)pool_proto.h	1.2"
#ident	"$Header$"
/*      @(#) pool_proto.h,v 1.4 1995/04/11 23:36:45 neil Exp        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 *
 */
/*      SCCS IDENTIFICATION        */
#ifdef __STDC__
# define	P(s) s
#else
# define P(s) ()
#endif

int pool_addr_alloc P((char *pool_tag, uint addr_type, void *addr_buf, int buf_sz));
int pool_addr_free P((char *pool_tag, uint addr_type, void *addr_buf, int buf_sz));
int pool_init P(());
char *poolfgets P((char *line, int n, FILE *fd));
void *pool_get_addr P((uint type, char *addr));
uint check_addr_type P((char *type));
struct sockaddr_in *ip_get_addr P((char *s));
char *type_to_string P((uint type));
char *addr_to_string P((void *addr, uint type));
addr_pool_t *find_pool P((char *tag));
int addr_size P((uint type));
addr_list_el_t *alloc_addr P((addr_pool_t *poolp));
int free_addr P((addr_pool_t *poolp, addr_list_el_t *addr_buf));
int alloc_from_old P((addr_pool_t *list_a, addr_pool_t *list_b));




