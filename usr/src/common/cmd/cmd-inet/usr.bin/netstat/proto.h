#ident "@(#)proto.h	1.4"
#ident "$Header$"

/* forward declarations */
struct ifreq_all;
struct ifstats;
struct nlist;
struct var;
struct rtentry;
struct ortentry;
struct radix_node;
struct rt_msghdr;

/* host.c */
int hostpr(off_t);

/* if.c */
int get_ifcnt(void);
int get_if_all(struct ifreq_all *);
int intpr(int);
int sidewaysintpr(unsigned);
void catchalarm(void);
int getifstats(char *, struct ifstats *);
int dlpi_get_stats(char *, struct ifstats *);

/* inet.c */
int protopr(char *);
int tcp_stats(char *);
int udp_stats(char *);
int ip_stats(char *);
int icmp_stats(char *);
int igmp_stats(char *);
int inetprint(struct in_addr *, u_short, char *);
char *inetname(struct in_addr);
void lockstats(void);

/* main.c */
int usage(void);
char *plural(int);
char *waswere(int);
char *pluraly(int);
int getnlist(char *, struct nlist *);
int vaddrinit(char *, char *, int);
int seekmem(off_t, int, int);
int readmem(off_t, int, int, char *, unsigned, char *);
int error(char *, ...);
int readata(char *);
int writedata(void );

/* route.c */
int routepr(off_t);
int treestuff(off_t);
struct sockaddr *kgetsa(struct sockaddr *);
int p_tree(struct radix_node *);
int p_rtnode(void);
int ntreestuff(void);
int np_rtentry(struct rt_msghdr *);
int get_if_name_for_route(char *, int);
int p_sockaddr(struct sockaddr *, struct sockaddr *, int, int);
int p_flags(int f, char *);
int p_rtentry(struct rtentry *);
int p_ortentry(struct ortentry *);
char *routename(struct in_addr);
char *netname(struct in_addr, u_long, int);
int rt_stats(void);

/* unix.c */
int pr_ux_stats(void);
