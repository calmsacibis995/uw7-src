#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/lookup.c	1.7.1.1"
#endif
/*
 * Module:	lookupTable  non-gui code for the lookup table
 * File:	lookup.c       get remote system names
 */

#include        <stdio.h>
#include        <string.h>
#include        <stdlib.h>
#include        <search.h>
#include        <errno.h>
#include        <ctype.h>
#include        <sys/types.h>
#include        <sys/stat.h>
#include        <sys/wait.h>
#include        <netdb.h>               /* for struct hostent */
#include        <sys/socket.h>          /* for AF_INET */
#include        <netinet/in.h>          /* for struct in_addr */
#include        <arpa/inet.h>           /* for inet_ntoa() */
#include	"lookup.h"
#include 	"common.h"
#include 	"text.h"

#define MAXALIASES	   35

/* defined in this file but accessed from elsewhere */
extern void	FreeHosts (HostList *);
extern readHostsReturn	ReadHosts (HostList **);
extern Boolean verifyHost(char * );

/* defined here and used only in this file */
static void alignHosts (HostList *, int );
static int  HostCmp ();
static char *getErrMsg(char *, char *,int *);
static Boolean removeWhitespace(char * );
static Boolean verifyHost2(char * );
static char *GetI18nText(char *);

/* 
 * verify if the host is a valid one by first checking the etc hosts file
 * and if not in the list, query the nslookup if the host is in the default
 * domain. Return the appropriate status.
 */

static Boolean 
verifyHost2(char * host)
{
 	register struct hostent *hostptr;

    if (strpbrk(host, WHITESPACE) != NULL)
    {
	fprintf(stderr,GetI18nText(TXT_MultipleHosts));
	return INVALID;
    }
	if ( (hostptr = gethostbyname(host)) == NULL) {
		return INVALID;
	}
	return VALID;
}

/* Verify if a valid host after getting rid of white space */

extern Boolean 
verifyHost(char * host )
{

    (void) removeWhitespace(host);
    if (host == NULL || *host == EOS)
    {
	fprintf(stderr,GetI18nText(TXT_HostRequired));
	return INVALID;
    }
    return verifyHost2(host);
}

/* ReadHosts
 *
 * Read /etc/hosts file.  Return True if there is no problem.
 */
extern readHostsReturn
ReadHosts (HostList **hostlist)
{
    static HostList	hosts = { NULL, 0, 0 };
    FILE	  *hostFile;
    char	  *token;
    char	   buf [MEDIUMBUFSIZE];
    struct stat    statBuffer;
    static time_t  lastRead = 0;
    int 	   maxNameLen = 0;

    while(!(hostFile = fopen (ETCHOSTS_PATH, "r")) && errno == EINTR)
	/* try again */;
    if (!hostFile)
    {
	fprintf(stderr,GetI18nText(TXT_EtcHostOpenFailed));
	return Failure;
    }
    while (fstat(fileno(hostFile), &statBuffer) < 0 && errno == EINTR)
	/* try again */;
    
    *hostlist = &hosts;
    if (statBuffer.st_mtime > lastRead)
    {
	FreeHosts (&hosts);
	lastRead = statBuffer.st_mtime;
    }
    else
    {
	fclose(hostFile);
	return SameList;
    }

    while (fgets (buf, MEDIUMBUFSIZE, hostFile))
    {
	char *tmp,*comment= NULL;

	 /* get rid of the white space in the beginning of the line
	  * and continue if the first character is a # or else continue
	  */
	tmp = buf;

	while(isspace(*tmp)) tmp++;
	if(*tmp == '#') 
		continue;
	/* collect the comments */
	token = strchr (tmp, '#');
	if (token){

	    /* get rid of the new line at the end */
	    if(comment = strchr(token,'\n'))
		*comment = EOS;

	    /* get the comment into comment */		
	    comment = strdup(strchr (tmp, '#'));
	    *token = EOS;
	}
	/* the host name is the second token */
	token = strtok (tmp, WHITESPACE);
	if (token)
	{
	    char host[MEDIUMBUFSIZE];

	    while ((token = strtok (NULL, WHITESPACE)))
	    {
		int i;
		Boolean duplicate = False;

		/* if the host is localhost or me, get rid of it */
		if(!strcmp(token, "localhost") || !strcmp(token, "me") ||
			!strcmp(token, "loopback"))
			continue;

		/* go ahead if not a duplicate */
		if (hosts.cnt >= hosts.allocated)
		{
		    hosts.allocated += HOST_ALLOC_SIZE;
		    hosts.list = (FormatData *)
			realloc((char *) hosts.list,
				   hosts.allocated * sizeof (FormatData));
		}
		if(strlen(token) > maxNameLen)
			maxNameLen = strlen(token);
		strcpy(host,token);
		/* if there is a comment attach it to the hostname */
		if(comment){
			/* tab not supported */
			strcat(host," "); 
			strcat(host,comment);
		}
		hosts.list [hosts.cnt++].name = (char *) strdup (host);
	    }
	}
	if(comment)
		free(comment);
    }
    fclose (hostFile);

    if (hosts.cnt == 0)
	return Failure;

    alignHosts (&hosts, maxNameLen);
    /* Sort the host list */
    qsort ((char *) hosts.list, hosts.cnt, sizeof (FormatData), HostCmp);

    return NewList;
}	/* End of ReadHosts () */


static void appendDomain(char *entry,char *domainName)
{
	char *entry_dup = strdup(entry);
	char *domain_dup = strdup(domainName);
	char *tmp;
	char *dom = NULL;

	tmp = entry_dup;
	while(*tmp) *tmp++ = toupper(*tmp);
	tmp = domain_dup;
	while(*tmp) *tmp++ = toupper(*tmp);
	tmp = domain_dup;
	if(strchr(entry_dup, '.')){
		while(*tmp){
		   if((dom = strstr( entry_dup, tmp)) == NULL){
			tmp = strchr(tmp, '.');
			if(tmp) tmp++;
       		    } else 	
			break;
		}
	}
	if(!dom){
              strcat(entry,".");
              strcat(entry,domainName);
        }
	free(entry_dup);
	free(domain_dup);
}

/* FreeHosts
 *
 * Free all old host data.  The list itself is kept around to reduce mallocing
 * on later calls.
 */
extern void
FreeHosts (HostList *hosts)
{
    int i;

    if (hosts == NULL || hosts->cnt == 0)
	return;

    for (i=hosts->cnt; --i>=0; )
    {
	if (hosts->list [i].name != NULL)
	    free(hosts->list [i].name);
    }
    free(hosts->list);
    hosts->list = NULL;
    hosts->cnt = 0;
}	/* End of FreeHosts () */

static void
alignHosts (HostList *hosts, int maxlen)
{
    int i, token_len, len;
    char *host, *comment;
    char           buf [MEDIUMBUFSIZE];
    char           name[MEDIUMBUFSIZE];
    char *tmp;

    if (hosts == NULL)
        return;

    for (i=hosts->cnt; --i>=0; )
    {
        if (hosts->list [i].name != NULL){
		strcpy(name, hosts->list [i].name);
		host = strtok (name, WHITESPACE);
		token_len = strlen(host);
		if(token_len < maxlen){
			strcpy(buf,host);
			for(len = 0; len <= maxlen - token_len; len++){ 
				buf[token_len+len] = ' ';
			}
		buf[token_len+len] = '\0';
		if(comment = strchr(hosts->list [i].name, '#'))
			strcat(buf, comment);

	        free(hosts->list [i].name);
		hosts->list [i].name = (char *) strdup (buf);
		}
	}
    }

}       /* End of aignHosts () */


/* HostCmp
 *
 * Comparison function for Host List sorter.
 */

static int
HostCmp (FormatData *h1, FormatData *h2)
{
    return (strcoll (h1->name, h2->name));
}	/* End of HostCmp () */


/*
 *  Get the error message by reading the error file and set the error number
 *  for display in the error dialog.
 */
static char *getErrMsg(char *errfile, char *domainName,int *errnum)
{

    FILE           *errFile;
    char           buf [MEDIUMBUFSIZE];
    char           tmp[MEDIUMBUFSIZE];

    while(!(errFile = fopen (errfile, "r")) && errno == EINTR)
		;
    while (fgets (buf, MEDIUMBUFSIZE, errFile))
    {
	sprintf(tmp,GetI18nText(TXT_cantList),domainName);
	if(strstr(buf,tmp)){
		if(strstr(buf,GetI18nText(TXT_noInfo)))
		     strcat(tmp,GetI18nText(TXT_noInfo));
		if(strstr(buf,GetI18nText(TXT_nonExist)))
		     strcat(tmp,GetI18nText(TXT_nonExist));
		*errnum = none;
		return(strdup(tmp));
	}
	
    }
    return NULL;
}

    /* Remove the white space */
static Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
        return False;

    while (isspace(*ptr))
    {
        ptr++;
        changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
        *string = EOS;
        return changed;
    }
    if (changed)
        (void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
                                                           /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
        ptr--;
        changed = True;
    }
    *(++ptr) = EOS;

    return changed;
}

#define UC(b)   (((int)b)&0xff)
parseAddr(ADDR inetAddr, char *addr)
{
        register char * p;
        u_long theaddr;

        theaddr = inet_addr(addr);
        p = (char *) &theaddr;
	inetAddr[0] = UC(p[0]);
	inetAddr[1] = UC(p[1]);
	inetAddr[2] = UC(p[2]);
	inetAddr[3] = UC(p[3]);
}
allocList(hostList *list)
{
	if (list->count % HOST_ALLOC_SIZE == 0) {
		list->list = (hostEntry *)
			realloc( (hostEntry *)list->list, 
			(list->count + HOST_ALLOC_SIZE)* sizeof (hostEntry));

		if(list->list == NULL){
			return 0;
		}
	}
	return(1);
}

freeHostList(hostList *list)
{
	int i;

	if(!list || list->count == 0)
		return;
	for(i=0;i<list->count; i++){
		free(list->list[i].name);
		list->list[i].name = NULL;
	}
	free(list->list);
	list->list = NULL;
	list->count = 0;
}

initResolvConf(resolvConf **resolv)
{
	resolvConf *new;
	
	new = (resolvConf *)malloc(sizeof(resolvConf));
	memset(new, '\0', sizeof(resolvConf));
	*resolv = new;
}

freeResolvConf(resolvConf* resolv)
{
	hostList *tmp;
	int i;

	if(!resolv || resolv->domain == NULL)
		return;
	free(resolv->domain);
	freeHostList(&resolv->serverList);
}

readResolvConf(resolvConf *resolv)
{
	FILE	*file;
	char	buf [MEDIUMBUFSIZE], *tmp;
	char	*token;
	struct stat    statBuffer;
	static time_t  lastRead = 0;

	while(!(file = fopen(ETCCONF_PATH, "r")) && errno == EINTR)
		;
	if (!file) {
		fprintf(stderr,GetI18nText(ERR_cantOpenEtcHost));
		return 0;
	}

	while (fstat(fileno(file), &statBuffer) < 0 && errno == EINTR)
                /* try again */;

	if (statBuffer.st_mtime > lastRead) {
		lastRead = statBuffer.st_mtime;
		freeResolvConf(resolv);
	} else {
		fclose(file);
		return 0;
	}

	/* search for the string "domain" */
	while (fgets (buf, MEDIUMBUFSIZE, file)) {
		tmp = buf;
		while(isspace(*tmp)) tmp++;
		if(*tmp == '#')
			continue;
		if(strstr(buf,"domain")){
			token = strtok(buf,WHITESPACE);
			token = strtok(NULL,WHITESPACE);
			resolv->domain = strdup(token);
			break;
		}else{
			/* invalid first line in etc/resolv.conf */
			return 0;
		}
	}
	/* parser all the name servers for address and comments for names */
	while (fgets (buf, MEDIUMBUFSIZE, file)) {
		char *comment = NULL;
		char addr[MEDIUMBUFSIZE];       /* for the host address*/

		tmp = buf;
		while(isspace(*tmp)) tmp++;
		if(*tmp == '#')
			continue;
		/* collect the comments for name server name */
		token = strchr (tmp, '#');
		if (token){
			/* get rid of the new line at the end */
			if(comment = strchr(token,'\n'))
				*comment = EOS;

				/* get the comment into comment */
			comment = strdup(token);  

			*token = EOS;   /* terminate the line at begin of commen */
		}
		if(strstr(buf,"nameserver")){

			token = strtok(buf,WHITESPACE);  /* keyword nameserver */
			token = strtok(NULL,WHITESPACE); /* inet addr */
			strcpy(addr, token); /* save the inet address of name server*/
		}else{
				/* invalid line in etc/resolv.conf */
				return 0;
		}
		/* allocate the space for nameserver list if necessary */
		/*if (resolv.serverList &&*/
		allocList(&resolv->serverList);
		/* store the comment as the nameserver  */
		resolv->serverList.list[resolv->serverList.count].name = comment;
		/* parse the name server inet address */
		parseAddr(resolv->serverList.list[resolv->serverList.count].addr, addr);
		resolv->serverList.count++;
	}
	if (strcmp(resolv->domain, "") == 0 || resolv->serverList.count <=0)
		return(0);
	return(1);
} 

char *getTmpFile()
{
	char *file;

	(void)umask(0022);
   file = tempnam(VAR_TMP, NULL);
   if ( file == NULL ) {
   /* fprintf(stderr,GetI18nText(TXT_fileCreateErr));*/
      file = strdup(VAR_TMP_ERR);
   }
   return(file);


}

isDuplicate(hostList list, char *entry)
{
	int i;

	if(list.count <= 0)
		return False;
	for(i=list.count-1; i>=0; i--){
		if (!strcmp(list.list[i].name, entry))
			return True;
	}
	return False;
}

initDnsList(dnsList **dnsHosts)
{
	dnsList *new;
	
	new = (dnsList *)malloc(sizeof(dnsList));
	memset(new, '\0', sizeof(dnsList));
	new->next = new->prev = NULL;
	*dnsHosts = new;
}

Boolean
isInDomainList(int index, dnsList *pos)
{
        if (index +1 <= pos->domainList.count )
		return 1;
                /*return TRUE;*/
        else
		return 0;
                /*return FALSE;*/
}

addDnsItem(dnsInfo *dns, dnsList *new)
{
	dnsList *tmp;

	if(dns->cur_pos == NULL){
		dns->cur_pos = new;
		dns->totalDnsItems = 1;
	}else{
		for(tmp = dns->cur_pos; tmp->next != NULL; tmp = tmp->next);
		new->prev = tmp;
		tmp->next = new;
		dns->totalDnsItems++;
	}
}
pruneDnsList(dnsList **dnsHosts, int *total)
{
	dnsList *tmp, *cur;

        tmp = *dnsHosts;
        if(tmp == NULL)
                return;
        while(tmp != NULL){
                cur = tmp;
                tmp = tmp->next;

                free(cur->domain.name);
                freeHostList(&cur->nameServerList);
                freeHostList(&cur->domainList);
                freeHostList(&cur->systemList);
                cur->next = cur->prev = NULL;
                (*total)--;
        }
        *dnsHosts = NULL;


}
static char *
GetI18nText(char * label)
{
        char    *p;
        char    c;

        if (label == NULL)
                return(INVALID);
        for (p = label; *p; p++)
                if (*p == '\001') {
                        c = *p;
                        *p++ = 0;
                        label = (char *)gettxt(label, p);
                        *--p = c;
                        break;
                }

        return(label);
}

printHostList(hostList list)
{
	int i;

	if(list.count <= 0)
		return;
	for(i=0; i<list.count; i++){
		printf("DNS list %d %s %d.%d.%d.%d\n",i, list.list[i].name,
			list.list[i].addr[0],
			list.list[i].addr[1],
			list.list[i].addr[2],
			list.list[i].addr[3]
		);
	}
}

printDnsList(dnsInfo *dns)
{
	dnsList *tmp;

	for(tmp = dns->cur_pos; tmp != NULL; tmp = tmp->next){
		
		printHostList(tmp->nameServerList);
		printHostList(tmp->systemList);
		printHostList(tmp->domainList);
	
	}
}

printResolvConf(resolvConf *resolv)
{
	hostList tmp;
	int i;

	tmp = resolv->serverList;
	for(i=0; i<tmp.count; i++){
		printf("Name Server %d %s %d.%d.%d.%d\n",i, tmp.list[i].name,
			tmp.list[i].addr[0],
			tmp.list[i].addr[1],
			tmp.list[i].addr[2],
			tmp.list[i].addr[3]
		);
	}
}

