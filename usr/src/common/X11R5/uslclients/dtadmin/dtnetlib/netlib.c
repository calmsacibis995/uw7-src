#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:netlib.c	1.9.1.1"
#endif

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
#include 	"util.h"

#define MAXALIASES	35
#define ETCHOSTS_PATH	"/etc/hosts"
#define NSLOOKUP_PATH   "/usr/sbin/nslookup"
#define VAR_TMP		"/var/tmp"
#define VAR_TMP_ERR	"/var/tmp/lookup.err"
#define VAR_TMP_DB	"/var/tmp/lookup.db"


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

/* should return 1 or 0 */
allocList(hostList *list)
{
	if (list == NULL)
		return;
	if (list->count % HOST_ALLOC_SIZE == 0) {
		list->list = (hostEntry *)
			realloc( (hostEntry *)list->list, 
			(list->count + HOST_ALLOC_SIZE)* sizeof (hostEntry));

		if(list->list == NULL){
			/*BUG:  need a separate shell for the error message*/
			printf("memory allocation probs..\n");
			return FALSE;
		}
	}
	return TRUE;
}

/* call from freeResolveConf() */
freeHostList(hostList *list)
{
	int i;

	if (list == NULL)
		return;
	for(i=0;i<list->count; i++){
		free(list->list[i].name);
		list->list[i].name = NULL;
	}
	free(list->list);
	list->list = NULL;
	list->count = 0;
}

/* debug purpose only */
printHostList(hostList list)
{
	int i;

	if(list.count <= 0)
		return;
	for(i=0; i<list.count; i++){
		printf("Nameserver %d %s\n",i, list.list[i].name);
	}
}

initEtcHostList(etcHostList **list)
{
	etcHostList *new;
	new = (etcHostList *) calloc(1, sizeof(etcHostList));
	if (new)
		*list = new;
	else
		printf("cannot init etchostlist\n");
}

freeEtcEntry(etcHostEntry entry)
{
	int i=0;

	if(entry.etcHost.name){
		free(entry.etcHost.name);
		entry.etcHost.name = NULL;
	}
	if(entry.comment){
		free(entry.comment);
		entry.comment = NULL;
	}
	while(entry.aliases[i] != NULL){
		free(entry.aliases[i]);
		entry.aliases[i] = NULL;
		i++;
	}
	if(entry.line){
		if (entry.line->line) {
			free(entry.line->line);
			entry.line->line = NULL;
		}
		free(entry.line);
		entry.line= NULL;
	}
}

freeEtcList(etcHostList *list)
{
	int i;
	etcLine	*nextPtr=NULL, *tmpPtr=NULL;

	if(list == NULL)
		return;
	for(i=0; i<list->count; i++){
		freeEtcEntry(list->list[i]);
	}
	free(list->list);
	list->list = NULL;
	list->count = 0;
	if (list->commentList) {
		if (list->commentList->line) free(list->commentList->line);
		nextPtr = list->commentList->next;
		free(list->commentList);
		list->commentList = NULL;
		i = 0;
	}
/* core dump when freeing the internal of etcLine
	while (nextPtr != NULL) {
		printf("i is %d line is %s\n", i, nextPtr->line);
		i++;	
		tmpPtr = nextPtr->next;
		if (nextPtr->line) free(nextPtr->line);
		nextPtr = tmpPtr;
	}
*/
}

etcLine * 
initCommentList(etcLine **list, char *line)
{
	etcLine *new;
	
	new = (etcLine *)calloc(1, sizeof(etcLine));
	new->next = new->prev = NULL;
	new->line = strdup(line);
	if (new) {
		*list = new;
		return new;
	}
	else {
		printf("error init comment list \n");
		return 0;
	}
}

etcLine * 
insertCommentList(etcLine *list, char *line)	
{
	etcLine *new;

	new = (etcLine *)calloc(1, sizeof(etcLine ));
	if (new) {
		new->next = NULL;
		list->next = new;
		new->prev = list;
		new->line = strdup(line);
		return(new);
	}
	else {
		printf("error inset comment list \n");
		return 0;
	}
}

void 
deleteCommentList(etcLine *list)	
{

	list->prev->next = list->next;
	list->next->prev = list->prev;
	free(list->line);
	list->line = NULL;
	free(list);
	list = NULL;
}

printCommentList(etcLine *list)
{
	etcLine *tmp;

	printf("LIST BEGIN %s\n",list->line);

	for(tmp=list; tmp; tmp = tmp->next){
		printf("LIST %s\n",tmp->line);
	}
}


char **
getAliases(char **aliases, int num_aliases)
{
	char **alist;
	int i;

	alist = (char **) calloc(num_aliases +1, sizeof(char *));
	for(i=0; i< num_aliases; i++)
		alist[i] = aliases[i];
	alist[i] = NULL;
	return alist;
}

extern readHostsReturn 
readEtcHosts(etcHostList *list)
{
	FILE          *hostFile;
	char          *token;
	char           buf [MEDIUMBUFSIZE];
	struct stat    statBuffer;
	static time_t  lastRead = 0;
	int            maxNameLen = 0;
	char		*aliases[MAXALIASES];
	int 		num_aliases;


	while(!(hostFile = fopen (ETCHOSTS_PATH, "r")) && errno == EINTR)
       	 /* try again */;

	if (!hostFile) {
		/* the caller will popup error message */
		fclose(hostFile);
		return Failure;
	}

	while (fstat(fileno(hostFile), &statBuffer) < 0 && errno == EINTR)
        	/* try again */;

	if (statBuffer.st_mtime > lastRead) {
		lastRead = statBuffer.st_mtime;
		freeEtcList(list);
	} else {
		fclose(hostFile);
		return SameList;
	}
	while (fgets (buf, MEDIUMBUFSIZE, hostFile)) {
		char *tmp,*comment= NULL;
		static etcLine *linePtr;

		/* preserve the line */
	        tmp = buf;
		if(list->commentList){
			linePtr = insertCommentList(linePtr, tmp);	
		}else{
			linePtr = initCommentList(&list->commentList, tmp);
		}

		/* get rid of the white space in the beginning of the line
		 * and continue if the first character is a # or else continue
		 */
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

				*token = EOS;	/* terminate the line at begin of comment */
		}
		/* the host name is the second token */
		token = strtok (tmp, WHITESPACE);
		if (token) {
			char host[MEDIUMBUFSIZE];	/* for the host name */
			char addr[MEDIUMBUFSIZE];	/* fot the host address */
		
			/* save the first token which is the inet address of host */
			strcpy(addr, token); /* save the inet address of host */

			/* BUG: MAKE provisions for a bad entry where 
			 * only address is shown and no name */

			/* get the hostname which is the second token*/
			token = strtok (NULL, WHITESPACE);

			/* if the host is localhost or me, get rid of it */
			if(!strcmp(token, "localhost") || !strcmp(token, "me") || !strcmp(token, "loopback")) 
				continue;

			/*valid hostname,  get it */
			strcpy(host,token);	

			/* collect the alias if any */
			num_aliases = 0;
			while ((token = strtok (NULL, WHITESPACE))) {
				aliases[num_aliases++] = strdup(token);
			}

			if (list->count % HOST_ALLOC_SIZE == 0) {
					list->list = (etcHostEntry *)
						realloc( (etcHostEntry *)list->list,
							(list->count + HOST_ALLOC_SIZE)* sizeof (etcHostEntry));
				if(list->list == NULL){
					/* caller should popup error */
					return Failure;
				}
			}

			list->list[list->count].etcHost.name = strdup(host);
			parseAddr(list->list[list->count].etcHost.addr, addr);
			if(num_aliases >0){
				list->list[list->count].aliases = getAliases( 
							aliases, num_aliases);
			}else
				list->list[list->count].aliases = NULL;
			if((int)strlen(host) > (int)maxNameLen)
				maxNameLen = strlen(host);
			/* if there is a comment attach it to the hostname */
			if(comment){
				list->list[list->count].comment= comment;
			}else
				list->list[list->count].comment= NULL;
			list->list[list->count].line = linePtr;
			list->count++;
		}
	}
	fclose (hostFile);

	if (list->count == 0)
		return Failure;

	return NewList;
}       /* End of ReadHosts () */


printEtclist(etcHostList *list)
{
	int i;
	int num_aliases;

	for(i = 0; i< list->count; i++){
		printf("%d.%d.%d.%d ",list->list[i].etcHost.addr[0],
					list->list[i].etcHost.addr[1],
					list->list[i].etcHost.addr[2],
					list->list[i].etcHost.addr[3]);
		printf("%s %s\n", list->list[i].etcHost.name, list->list[i].comment);
		num_aliases = 0;
		while(list->list[i].aliases[num_aliases] != NULL){
			printf( "      aliases %s\n",list->list[i].aliases[num_aliases++]);
		}
	}
}


initResolvConf(resolvConf **resolv)
{
	resolvConf *new;
	
	new = (resolvConf *)calloc(1, sizeof(resolvConf));
	if (new)
		*resolv = new;
	else {
		*resolv = NULL;
		printf("error init resolveConf\n");
	}
}

freeResolvConf(resolvConf* resolv)
{
	hostList *tmp;
	int i;

	if (resolv == NULL)
		return;
	if (resolv->domain) free(resolv->domain);
	freeHostList(&resolv->serverList);
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

readResolvConf(resolvConf *resolv)
{
	FILE	*file;
	char	buf [MEDIUMBUFSIZE], *tmp;
	char	*token;
	struct stat    statBuffer;
	static time_t  lastRead = 0;
	int	ret;

	while ((ret = stat(ETCCONF_PATH, &statBuffer)) < 0 && errno == EINTR)
		/* try again */;
	if (ret == -1 && errno == ENOENT) {
		return FALSE;
	}

	if (statBuffer.st_size <=0)
		return FALSE;

	if( (file = fopen(ETCCONF_PATH, "r")) == NULL ){
		/* caller will popup error */
		return FALSE;
	}

	if (statBuffer.st_mtime > lastRead) {
		lastRead = statBuffer.st_mtime;
		freeResolvConf(resolv);
	} else {
		fclose(file);
		return TRUE;
	}

	/* search for the string "domain" */
	while (fgets (buf, MEDIUMBUFSIZE, file)) {
		tmp = buf;
		while(isspace(*tmp)) tmp++;
		if(!(*tmp) || *tmp == '#')
			continue;
		if(strstr(buf,"domain")){
			token = strtok(buf,WHITESPACE);
			token = strtok(NULL,WHITESPACE);
			resolv->domain = strdup(token);
			break;
		}else{
			/* invalid first line in etc/resolv.conf */
			fclose(file);
			return FALSE;
		}
	}
	/* parser all the name servers for address and comments for names */
	while (fgets (buf, MEDIUMBUFSIZE, file)) {
		char *comment = NULL;
		char addr[MEDIUMBUFSIZE];       /* for the host address*/

		tmp = buf;
		while(isspace(*tmp)) tmp++;
		if(!(*tmp) || *tmp == '#')
			continue;
		/* collect the comments for name server name */
		token = strchr (tmp, '#');
		if (token){
			/* get rid of the new line at the end */
			if(comment = strchr(token,'\n'))
				*comment = EOS;
			comment = token;
			while (strpbrk(comment, "# \t"))
				++comment;
			/* get the comment into comment */
			comment = strdup(comment);  

			*token = EOS;   /* terminate the line at begin of commen */
		}
		if(strstr(buf,"nameserver")){

			token = strtok(buf,WHITESPACE);  /* keyword nameserver */
			token = strtok(NULL,WHITESPACE); /* inet addr */
			strcpy(addr, token); /* save the inet address of name server*/
		}else{
				/* invalid line in etc/resolv.conf */
				fclose(file);
				return FALSE;
		}
		/* allocate the space for nameserver list if necessary */
		allocList(&resolv->serverList);
		/* store the comment as the nameserver  */
		resolv->serverList.list[resolv->serverList.count].name = comment;
		/* parse the name server inet address */
		parseAddr(resolv->serverList.list[resolv->serverList.count].addr, addr);
		resolv->serverList.count++;
	}
	if (strcmp(resolv->domain,"") == 0 || resolv->serverList.count <=0 )
		return(FALSE);
	return(TRUE);
} 

char *
getTmpFile()
{
	char *file;

	(void)umask(0022);
	file = tempnam(VAR_TMP, NULL);
	if ( file == NULL ) {
		/* BUG:  should return error */
		file = strdup(VAR_TMP_ERR);
	}
	return(file);
}

isDuplicate(hostList list, char *entry)
{
	int i;

	if(list.count <= 0)
		return FALSE;
	for(i=list.count-1; i>=0; i--){
		if (!strcmp(list.list[i].name, entry))
			return TRUE;
	}
	return FALSE;
}

initDnsList(dnsList **dnsHosts)
{
	dnsList *new;
	
	new = (dnsList *)calloc(1, sizeof(dnsList));
	if (new)
		*dnsHosts = new;
	else {
		printf("error initDnslist\n");
		*dnsHosts = NULL;
	}
}

getNameServers(dnsList *dns, char *file)
{
	FILE	*serverFile;
	char	serverName[MEDIUMBUFSIZE];


	while(!(serverFile = fopen (file, "r")) && errno == EINTR)
		/* try again */;

	if (!serverFile)
	{
		/* caller will popup error message */
		return Failure;
	}
	while (fscanf(serverFile,"%s",serverName)!=EOF) {
		/* allocate the space for nameserver list if necessary */
		allocList(&dns->nameServerList);
		/* store the comment as the nameserver  */
		dns->nameServerList.list[dns->nameServerList.count].name = strdup(serverName);
		/* BUG? did not have the server address */
		dns->nameServerList.count++;
	}
	fclose(serverFile);
}

getDomainHosts(dnsList *dns, char *file)
{
	FILE	*hostFile;
	char	buf[MEDIUMBUFSIZE];
	char  entry[MEDIUMBUFSIZE];
	char  *tmp, *ptr1, *ptr2;
	char  var1[MEDIUMBUFSIZE], var2[MEDIUMBUFSIZE];
	char  *token;

	while(!(hostFile = fopen (file, "r")) && errno == EINTR)
        /* try again */;

	if (!hostFile) {
		return Failure;
	}
	
	/* Throw away the first lines which do not have any info */
	while (fgets (buf, MEDIUMBUFSIZE, hostFile)) {
		if(strchr(buf,'>')){
			fgets (buf, MEDIUMBUFSIZE, hostFile);
			break;
		}
	}

	while (fgets (buf, MEDIUMBUFSIZE, hostFile)) {

		tmp = buf;

		while(isspace(*tmp)) tmp++;
		if(*tmp == '>')
			break;
		if(*tmp == '#')
			continue;
		token = strtok (tmp, WHITESPACE);
		
		if (token) {
			/* allocate the space for systemList list if necessary */
			strcpy(entry,token);
			token = strtok (NULL, WHITESPACE);

			if(!strcmp(token, "NS") || !strcmp(token, "ns")){
				allocList(&dns->domainList);
				/* if the dns->domain.name is already exist,
				 * don't put it in 
				 */
				strcpy(var1, entry);
				strcpy(var2, dns->domain.name);
				ptr1 = &(var1[0]);
				ptr2 = &(var2[0]);
				while (*ptr1) 
					*ptr1++ = toupper(*ptr1);
				while (*ptr2) 
					*ptr2++ = toupper(*ptr2);

				/* if sub-domain name is not full qualified */
				if (!strstr(var1, var2)) {
					strcat(var1, ".");
					strcat(var1, var2);
					strcat(entry, ".");
					strcat(entry, dns->domain.name);
				}

				if (strncmp(var1, var2, strlen(var2)) && !isDuplicate(dns->domainList, entry))
				
					dns->domainList.list[dns->domainList.count++].name = strdup(entry);
			}
			else{
				if(!strcmp(token, "A") || !strcmp(token, "a")){
					allocList(&dns->systemList);
					if(!isDuplicate(dns->systemList, entry))
						dns->systemList.list[dns->systemList.count++].name = strdup(entry);
				}
			}
		}
	}
	fclose(hostFile);
	if (dns->systemList.count== 0)
		return Failure;

	return NewList;
}

Boolean
isInDomainList(int index, dnsList *pos)
{
	if (pos->domainList.count == 0 )
		return FALSE;
	if (index <= pos->domainList.count)
		return TRUE;
	else
		return FALSE;
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
