/*
 * multihome API
 */

#ifndef _MULTIHOME_H_
#define _MULTIHOME_H_
#ident "@(#)multihome.h	11.1"

/* first call opens the database file and keeps it open */
char *mhome_user_map(char *username, char *fqdn);

/* if you want to force a close of the database file */
void mhome_user_map_close();

/* find a virtual domain's ip addr (returned as a string x.x.x.x) */
char *mhome_virtual_domain_ip(char *domain);

/* force closure of virtual domain map */
void mhome_virtual_domain_close();

/* find the virtual domain name for an ip */
char *mhome_ip_virtual_name(char *ip);

#define MHOMEPATH	"/var/internet/ip"
#define MHOMEDB		"/var/internet/ip/127.0.0.1/virtusers.db"
#define MDOMAINDB	"/var/internet/ip/127.0.0.1/mail/virtdomains.db"

#endif
