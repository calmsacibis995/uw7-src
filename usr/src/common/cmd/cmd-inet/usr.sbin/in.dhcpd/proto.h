#ident "@(#)proto.h	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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
 */

#ifdef __STDC__
# define P(s) s
#else
# define P(s) ()
#endif

/* options.c */
OptionDesc *lookup_option P((char *name));
void init_option_defaults P((void));

/* database.c */
int init_database P((void));
void reset_database P((void));
void free_options P((OptionSetting *));
Client *lookup_client P((u_char id_type, u_char *id, int id_len));
UserClass *lookup_user_class P((char *class, int class_len));
VendorClass *lookup_vendor_class P((u_char *class, int class_len));
Subnet *lookup_subnet P((struct in_addr *addrp));
int add_client P((Client *client));
int add_user_class P((UserClass *uclass));
int add_vendor_class P((VendorClass *vclass));
int add_subnet P((Subnet *subnet));

/* parse.c */
void configure P((void));
int config_file_changed P((void));

/* proto.c */
void proto P((int timeout));

/* setarp.c */
void setarp P((struct in_addr *ia, u_char *ha, int len));

/* endpt.c */
int get_endpt P((u_short port));
int set_endpt_opt P((int endpt, int level, int option, void *value, int len));
int send_packet P((int endpt, struct sockaddr_in *dest, void *buf, int len));
int recv_packet P((int endpt, void *buf, int len, struct sockaddr_in *from,
	u_long *ifindex));
int is_endpt P((int fd));

/* util.c */
void report P((int pri, char *fmt, ...));
void malloc_error P((char *where));
char *binary2string P((u_char *buf, int len));
char *t_strerror P((int err));

/* ping.c */
int ping P((struct in_addr *addr));

/* main.c */
void done P((int exit_code));

#undef P
