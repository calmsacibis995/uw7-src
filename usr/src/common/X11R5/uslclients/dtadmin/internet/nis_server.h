#ifndef	NOIDENT
#pragma	ident	"@(#)dtadmin:internet/nis_server.h	1.3"
#endif

#ifndef _DTADMIN_INTERNET_NIS_SERVER_H
#define _DTADMIN_INTERNET_NIS_SERVER_H

#define YPSERVERS	"/var/yp/binding/%s/ypservers"
#define YPDOMCMD	"/usr/bin/domainname"
#define YPINITCMD	"/usr/sbin/ypinit -c 2>&1 > /dev/null"

typedef struct _nis_server_list {
	char *				server;
	struct _nis_server_list *	next;
} NisServerList_t;

typedef struct _nis_values {
	Boolean			firstTime;
	char *			domain;
	NisServerList_t *	serverList;
	int			selectedItem;
} NisValues_t;

enum { ADD, MODIFY, DELETE };

typedef struct _nis_server {
        Widget                  nisPopup;
        Widget                  nisDomain;
        Widget                  nisServer;
	Widget			nisButton[3];
        Widget                  nisList;
        Widget                  nisStatus;
} NisServer_t;

#endif /* _DTADMIN_INTERNET_NIS_SERVER_H */
