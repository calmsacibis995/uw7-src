#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/common.h	1.2"
#endif

	/*
	 * File common.h
	 * to be included by the gui and the non-gui part of the library i.e.
	 * lookup.c and prop.c
	 */

#define WHITESPACE        " \t\n"

typedef enum _readHostsReturn { NewList, SameList, Failure } readHostsReturn;

typedef enum _errorList {
        none,
        invalidHost,
        noEntries,
        cantList,
        noDomain
} errorList;

typedef struct {
    char *name;
} FormatData;

typedef struct {
    FormatData  *list;
    unsigned    cnt;
    unsigned    allocated;
} HostList;

typedef unsigned char ADDR[4];

typedef struct _hostEntry {
	char	*name;
	ADDR	addr;
} hostEntry;

typedef struct _hostList {
	hostEntry	*list;
	int		count;
} hostList;

typedef struct _etcLine{
	char *line;
	struct _etcLine *next;
	struct _etcLine *prev;
} etcLine;

typedef struct _etcHostEntry {
	hostEntry	etcHost;
	char		*comment;
	char		**aliases;
	etcLine		*line;		/*pointer to line */
} etcHostEntry;

typedef struct _etcHostList {
	etcHostEntry		*list;
	int			count;
	etcLine			*commentList;
} etcHostList;

typedef struct _dnsList {
	hostEntry	domain;
	hostList	domainList;
	hostList	systemList;
	hostList	nameServerList;
	struct _dnsList *prev;
	struct _dnsList *next;
} dnsList;

typedef struct _resolvConf {
	char			*domain;
	hostList		serverList; /*name field is used as comment */
} resolvConf;

typedef enum {
	etcHost,
	DNS
} viewType;

typedef enum {
	DNSL1,
	DNSL2,
	DNSL3,
	ETCL
} dnsListNum;

typedef enum {
        NONE,
        GET_ADDRESS,
        NAME_SERVERS,
        HOSTS
} nsQueryType;
typedef enum {
        START,
        SHOWDOMAIN,
        DOUBLECLICK,
} nsQueryFrom;

typedef struct _dnsInfo {
        Boolean dnsExists; /* is DNS configured */
        resolvConf      *resolv;
        dnsList         *dnsHosts;
        dnsListNum      cur_wid_pos;    /* position in the list widget array */
        dnsList         *cur_pos;       /* position in the linked list */
        int     totalDnsItems;          /* total number of linked list items */
        int     pid;    /* process id for the current ns query process */
        char    *outfile; /* parse output of the nsquery from this file */
        nsQueryType query;/* which is the current query */
        nsQueryFrom queryFrom;  /*from where the query get call: double click,
                                show domain button or initialization */
        Boolean hostQuerySuccess; /* is the last query to dns successful */
        int serverIndex;        /* to query the current server in nameservers
                                list, asynchronous querying  */
        int     dnsIndex[3];       /* current index into the List widget */
        char    *dnsSelection[3];  /* current selection from List widget */
} dnsInfo;


