#ident	"@(#)pppu_proto.h	1.2"
#ident	"$Header$"
#ident "@(#) pppu_proto.h,v 1.3 1995/08/25 20:47:45 neil Exp"
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

/* gtphostent.c */
FILE *setppphostent P(());
int endppphostent P((FILE *fp));
int getppphostent P((FILE *fp , struct ppphostent *hp));
int papgetpwd P((char pid [], char pwd []));
void mystrcpy P((char *s , char *t ));
int getaddress P((char tag [], struct sockaddr_in *address));
int getaddr P((char *s , struct sockaddr_in *sin));
int unique P((void ));

/* gtphstnamadr.c */
struct ppphostent *getppphostbyname P((char *nam ));
struct ppphostent *getppphostbyaddr P((char *addr , int length , int type ));
struct ppphostent *getppphostbyattach P((char *attach ));

/* ppp.c */
int notify_daemon P((int sig ));
int sig_hup P((int sig ));

/* pppd.c */
void ntfy_child P((int sig ));
void remove_child P((int sig ));
void sig_rm_conn P((int sig ));
void sig_poll P((int sig ));
void config P((void ));
void dofilter P((int fd , char *file, ulong netmask, char *key ));
int main P((int argc , char **argv ));
struct conn_made_s *ppp_add_conn P((int fd , struct ppphostent *hp));
int ppp_rm_conn P((struct conn_made_s *cm ));
int ppp_close_conn P((struct conn_made_s *cm ));
struct ip_made_s  *ppp_add_inf P((struct sockaddr_in dst, struct sockaddr_in src, struct sockaddr_in mask, char **name, char *file, int perm, int debug));
int ppp_rm_inf P((struct ip_made_s *im ));
int pppd_sockread P((int s ));
int ppcid_msg P((int ppcidfd ));
int ppp_syslog P((int pri , char *fmt, ...));
int pppdial P((struct ppphostent hostent, int s_attach ));
char *MD5String P((char *string, int len));


/* pppstat.c */
int sig_alm P((void ));
int print_conn_stat P((int fd)); 


/* sock_supt.c */
int pppd_sockinit P((int maxpend ));
int ppp_sockinit P((void ));

/* util.c */
int rm_route P((struct sockaddr *dst));
int getbaud P((char *s ));
int getbaudrate P((int b ));
char * getifname P((int fd ));
int dlbind P((int fd , int sap ));
int setarp P((int cmd , ulong host , char *a ));
int rtmsg P((int cmd ));
int sioctl P((int fd , int cmd , caddr_t dp , int len ));
char *pppfgets P((char *line , int n , FILE *fd ));
int getdtablesize P(());
pppd_mod_list_t *pop_modules P((int fd));
int restore_popped_mods P((int fd, pppd_mod_list_t *lp));
int control_hwd_flow P((int fd, int enable));
/* uucpspt.c */
void assert P((char *s1 , char *s2 , int i1 , char *s3 , int i2 ));
void logent P((char *s1 , char *s2 ));
void cleanup P((int Cn ));
char *ppp_getifname P(());

/* proxy.c */
int proxyadd P((int fd, ulong dst, ulong src));
int proxy_arp_set P((ulong inet_addr, char *mac_addr, int len));
int deletearp P((ulong inet_addr));
int get_physaddr_from_name P((char *name, u_char *eaddr, int len));
int get_macaddr P((char *name, char *addr, int len));
char *mac_sprintf P((u_char *addr, int len));

/* list.c */
void *element_add P((list_header_t **list,int size));
int element_free(list_header_t **list, list_header_t *el);

#undef P



