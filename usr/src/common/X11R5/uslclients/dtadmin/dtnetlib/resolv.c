#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:resolv.c	1.6"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>
#include "lookup.h"

extern int h_errno;  /* for resolver errors */

int findNameServers();    
int queryNameServers();   /* grab zone records from servers */
int  skipToData();         /* skip to the resource record data */
int  skipName();           /* skip a compressed name */
int strip_domain(); /* formatting returned record */
int  listInfo();	   /* list subdomains and hosts in a zone */
int  catch_alm();
int  ncmp();

/* Maximum number of name servers we will check */
#define ALIMIT 10000
#define UC(b) (((int)b)&0xff)
#define TIMEOUT 10

int	aNum = 0;
int	count = 0;
int	err_code=0;

/****************************************************************
 * findNameServers -- find all of the name servers for the      *
 *     given domain and store their names in nsList.  nsNum is  *
 *     the number of servers in the nsList array.               *
 ****************************************************************/
int
findNameServers(char *domain, hostList *nsList)
{
    union {
        HEADER hdr;           /* defined in resolv.h */
        u_char buf[PACKETSZ]; /* defined in arpa/nameser.h */
    } response;               /* response buffers */
    int responseLen;          /* buffer length */

    u_char  *cp;       /* character pointer to parse DNS packet */
    u_char  *endOfMsg; /* need to know the end of the message */
    u_short class;     /* classes defined in arpa/nameser.h */
    u_short type;      /* types defined in arpa/nameser.h */
    u_long  ttl;       /* resource record time to live */
    u_short dlen;      /* size of resource record data */
    struct hostent *host;
    struct in_addr *ptr;
    u_long theaddr;
    unsigned char *p;
    struct {ADDR addr;
    } tmp_addr[10];
    int addr_count;
    int n;

    int i, count, dup; /* misc variables */

    /* 
     * Look up the NS records for the given domain name.
     * We expect the domain to be a fully qualified name, so
     * we use res_query().  If we wanted the resolver search 
     * algorithm, we would have used res_search() instead.
     */
    res_init();
    if((responseLen = 
           res_query(domain,      /* the domain we care about   */
                     C_IN,        /* Internet class records     */
                     T_NS,        /* Look up name server records*/
                     (u_char *)&response,      /*response buffer*/
                     sizeof(response)))        /*buffer size    */
                                        < 0){  /*If negative    */
	switch (h_errno){
		case HOST_NOT_FOUND: 
			err_code = 10;
			break;
		case NO_DATA:
			err_code = 11;
			break;
		case TRY_AGAIN:
			err_code = 12;
			break;
		default:
			err_code = 13;
			break;
	}		
        return(err_code);
    }

    /*
     * Keep track of the end of the message so we don't 
     * pass it while parsing the response.  responseLen is 
     * the value returned by res_query.
     */
    endOfMsg = response.buf + responseLen;

    /*
     * Set a pointer to the start of the question section, 
     * which begins immediately AFTER the header.
     */
    cp = response.buf + sizeof(HEADER);

    if ((n = skipName(cp, endOfMsg)) < 0) 
	return(err_code);
    cp += n + QFIXEDSZ;

    count = ntohs(response.hdr.ancount) + 
            ntohs(response.hdr.nscount);
    while (    (--count >= 0)        /* still more records     */
            && (cp < endOfMsg)) {      /* still inside the packet*/
            

        /* Skip to the data portion of the resource record */
	if ((n = skipToData(cp, &type, &class, &ttl, &dlen, endOfMsg)) < 0)
		return(err_code);
        cp += n;

        if (type == T_NS) {
            if (!allocList(nsList)) 
		return(1);

	    nsList->list[nsList->count].name=(char *)malloc(MAXDNAME);
	    if (nsList->list[nsList->count].name==NULL) 
		return(1);
            /* Expand the name server's name */
            if (dn_expand(response.buf, /* Start of the packet   */
                          endOfMsg,     /* End of the packet     */
                          cp,           /* Position in the packet*/
                          (u_char *)nsList->list[nsList->count].name, /* Result    */
                          MAXDNAME)     /* size of nsList buffer */
                                    < 0) { /* Negative: error    */
		return(2);
            }

            /*
             * Check the name we've just unpacked and add it to 
             * the list of servers if it is not a duplicate.
             * If it is a duplicate, just ignore it.
             */
            for(i = 0, dup=0; (i < nsList->count) && !dup; i++)
                dup = !strcasecmp(nsList->list[i].name, nsList->list[nsList->count].name);
            if(dup) 
               free((char *)nsList->list[nsList->count].name);
            else {
		addr_count = 0;
		if ((host = gethostbyname(nsList->list[nsList->count].name)) != NULL) {
			while ((ptr = (struct in_addr *) *host->h_addr_list)
				!= NULL) {
				theaddr = inet_addr(inet_ntoa(*ptr));
				p = (unsigned char *)&theaddr;
				tmp_addr[addr_count].addr[0]  = UC(p[0]);
				tmp_addr[addr_count].addr[1]  = UC(p[1]);
				tmp_addr[addr_count].addr[2]  = UC(p[2]);
				tmp_addr[addr_count].addr[3]  = UC(p[3]);
				*host->h_addr_list++;
				addr_count++;
			}
		}
		else return(3);
		addr_count--;
		if (addr_count > 0) {
			for (i=nsList->count+1; i<=nsList->count+addr_count; i++) 
				nsList->list[i].name=strdup(nsList->list[nsList->count].name);
		}
		for (i=0; i <= addr_count; i++, nsList->count++) {
			nsList->list[nsList->count].addr[0] = tmp_addr[i].addr[0];
			nsList->list[nsList->count].addr[1] = tmp_addr[i].addr[1];
			nsList->list[nsList->count].addr[2] = tmp_addr[i].addr[2];
			nsList->list[nsList->count].addr[3] = tmp_addr[i].addr[3];
		}
			
	    }
        }

        /* Advance the pointer over the resource record data */
        cp += dlen;

    } /* end of while */
    return(0);
}

int
queryNameServers(char *domain, hostEntry nsList, hostList *domList, hostList *sysList)
{
    union {
        HEADER hdr;            /* defined in resolv.h */
        u_char buf[PACKETSZ];  /* defined in arpa/nameser.h */
    } query;
    int queryLen;		/* buffer lengths */

    int i;                /* counter variable */
    struct sockaddr_in sin;
    unsigned short nsport = NAMESERVER_PORT;
    int sockFD = -1;
    u_short len;
    int listRet;
    char *cp;
    int amtToRead;
    int numRead;
    char	*answer = NULL;
    int	answerLen = 0;

    queryLen = res_mkquery(QUERY, domain, C_IN, T_AXFR, (char *)NULL, 0,
                 (struct rrec *)NULL, (char *)&query, sizeof(query)); 

    if (queryLen < 0) 
	return(4);

    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(nsport);
    (void)memcpy((void *)&sin.sin_addr, (void *)nsList.addr, 
	sizeof(nsList.addr));

    /*
     *  Set up a virtual circuit to the server.
     */
    if ((sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
   	return(20);
    }
    sigset(SIGALRM, catch_alm);
    alarm(TIMEOUT);
    if (connect(sockFD, (caddr_t)&sin, sizeof(sin)) < 0) {
        (void) close(sockFD);
        sockFD = -1;
        if (errno == ECONNREFUSED) 
		return(5);
        else
		return(6);
    }
    alarm(0);

    len = htons(queryLen);
    if (write(sockFD, (char *)&len, sizeof(len)) != sizeof(len) ||
        write(sockFD, (char *) &query, queryLen) != queryLen) {
		close(sockFD);
		sockFD= -1;
		return(7);
    }
    aNum = 0;
    count = 0;
    while (1) {
        unsigned short tmp;

        /*
         * Read the length of the response.
         */

        cp = (char *) &tmp;
        amtToRead = sizeof(u_short);
        while (amtToRead > 0 && (numRead=read(sockFD, cp, amtToRead)) > 0) {
    	    cp	  += numRead;
	    amtToRead -= numRead;
	}
	if (numRead <= 0) {
		return(8);
	}

	if ((len = htons(tmp)) == 0) {
		break;	/* nothing left to read */
	}

	/*
	 * The server sent too much data to fit the existing buffer --
	 * allocate a new one.
	 */
	if ((int) len > answerLen) {
		if (answerLen != 0) 
		    free(answer);
		
		answerLen = len;
		if (!(answer = (char *)malloc(answerLen))) 
			return(1);
	}

	/*
	 * Read the response.
	 */

	amtToRead = len;
	cp = answer;
	while (amtToRead > 0 && (numRead=read(sockFD, cp, amtToRead)) > 0) {
		cp += numRead;
		amtToRead -= numRead;
        }
        if (numRead <= 0) 
		return(8);
        
        listRet = listInfo(answer, cp, domain, nsList, domList, sysList);
	if (listRet == 1)
		continue;
        else 
		if (listRet == -1) {
			/* if (answer) free(answer);  */
			return(err_code);
		}
		else 
			if (listRet == 2) {
				/* if (answer) free(answer); */
				if (sysList->count > 1)
					qsort((hostEntry *)sysList->list,
						sysList->count,sizeof(hostEntry),ncmp);
				if (domList->count > 1)
					qsort((hostEntry *)domList->list,
						domList->count,sizeof(hostEntry),ncmp);
				break;
			}

    } /*while 1 */
    if (sockFD) {
	(void) close(sockFD);
	sockFD = -1;
    }
    return(0);
}

/****************************************************************
 * skipName -- This routine skips over a domain name.  If the   *
 *     domain name expansion fails, it reports an error and     *
 *     exits.  dn_skipname() is probably not on your manual     *
 *     page; it is similar to dn_expand() except that it just   *
 *     skips over the name.  dn_skipname() is in res_comp.c if  *
 *     you need to find it.                                     *
 ****************************************************************/
int
skipName(cp, endOfMsg)
u_char *cp;
u_char *endOfMsg;
{
    int n;

    if((n = dn_skipname(cp, endOfMsg)) < 0){
	err_code = 9;
        return(-1);
    }
    return(n);
}

/****************************************************************
 * skipToData -- This routine advances the cp pointer to the    *
 *     start of the resource record data portion.  On the way,  *
 *     it fills in the type, class, ttl, and data length        *
 ****************************************************************/
int
skipToData(cp, type, class, ttl, dlen, endOfMsg)
u_char  *cp;
u_short *type;
u_short *class;
u_long  *ttl;
u_short *dlen;
u_char  *endOfMsg;
{
    int	n;
    u_char *tmp_cp = cp;  /* temporary version of cp */

    /* Skip the domain name; it matches the name we looked up */
    if ((n = skipName(tmp_cp, endOfMsg)) < 0) {
	err_code = 9;
	return(-1);
    }
    tmp_cp += n;

    /*
     * Grab the type, class, and ttl.  The routines called
     * _getshort() and _getlong() are also resolver routines 
     * you may not find in a manual page.  They are in 
     * res_comp.c if you want to see them.
     */
    *type = _getshort(tmp_cp);
    tmp_cp += sizeof(u_short);
    *class = _getshort(tmp_cp);
    tmp_cp += sizeof(u_short);
    *ttl = _getlong(tmp_cp);
    tmp_cp += sizeof(u_long);
    *dlen = _getshort(tmp_cp);
    tmp_cp += sizeof(u_short);

    return(tmp_cp - cp);
}

int
strip_domain(string, domain)
    char *string, *domain;
{
    register char *dot;

	dot = string;
	if (*dot == '#')
		return(-1);
	while ((dot = strchr(dot, '.')) != NULL && strcasecmp(domain, ++dot)) 
		;
	if (dot != NULL) {
	    dot[-1] = '\0';
		return(1);
	}
    return (0);
}

int
listInfo(char *msg, char *endOfMsg, char *domain, hostEntry nsList, hostList *domList, hostList *sysList)
{
    char  *cp;       /* character pointer to parse DNS packet */
    u_short type;      /* types defined in arpa/nameser.h */
    int namelen;
    char *fullName[ALIMIT];
    char host_name[MAXDNAME];
    int dup;
    HEADER *headerPtr;
    int i;
    int stripped;
    u_short f_type;  
    char f_name[MAXDNAME];
    int n;

    headerPtr = (HEADER *)msg;
    cp = msg + sizeof(HEADER);
    if (headerPtr->rcode != NOERROR) {
	switch((int)headerPtr->rcode) {
		case FORMERR:
			err_code = 14;
			break;
		case SERVFAIL:
			err_code = 15;
			break;
		case NXDOMAIN:
			err_code = 16;
			break;
		case NOTIMP:
			err_code = 17;
			break;
		case REFUSED:
			err_code = 18;
			break;
		default:
			err_code = 13;
			break;
	}
	return -1;
    }
    if (ntohs(headerPtr->ancount) == 0) {
	err_code = 19;
	return -1;
    }
    if (ntohs(headerPtr->qdcount) > 0) {
	if ((n = skipName(cp, endOfMsg)) < 0)
		return(err_code);
    	cp += n + QFIXEDSZ;
    }

    fullName[aNum] = (char *)calloc(1, MAXDNAME);
    if (fullName[aNum] == NULL) {
	    err_code = 1;
	    return -1;
    }

    if ((namelen = dn_expand(msg, endOfMsg, cp, fullName[aNum], MAXDNAME))     
                    < 0) { 
	free(fullName[aNum]);
	err_code = 2;
        return -1;
    }
    cp += namelen;
    type = _getshort(cp);
    if (count == 0) {
	strcpy(f_name, fullName[0]);
	f_type = type;
	count++;
    }


    if (aNum && type == T_SOA) {
	    if (aNum > 0 && !strcasecmp(fullName[aNum], f_name)
			&& type == f_type) {
		    /* reach the end */
		    for (i=0; i<aNum; i++) {
			if (fullName[i])
				free(fullName[i]);
		    }
		    return 2;
	    }
	    else {
		    free(fullName[aNum]);
		    return 1;
	    }
    }
	
    switch (type) {
	    case T_A:
    		   for (i=0, dup=0; i < aNum && !dup; i++)
    			   dup = !strcasecmp(fullName[i], fullName[aNum]);
		   if (dup) {
    			   free(fullName[aNum]); 
    			   return 1;
               	   }
		   strcpy(host_name, fullName[aNum]);
		   stripped=strip_domain(host_name, domain);
		   if (stripped == -1)
			return 1;
		   if (strchr(host_name, '.') != NULL) 
			strcpy(host_name, fullName[aNum]);
		   if (!allocList(sysList)) {
			err_code = 1;
			return(-1);
		   }
		   sysList->list[sysList->count++].name=strdup(host_name);
      		   break;
	   case T_NS:
    		   for (i=0, dup=0; i < aNum && !dup; i++)
    			   dup = !strcasecmp(fullName[i], fullName[aNum]);
		   if (dup) {
    			   free(fullName[aNum]); 
    			   return 1;
               	   }
		    if (strcasecmp(fullName[aNum], domain))
		    {
		    if (!allocList(domList)) {
			err_code = 1;
			return(-1);
		    }
		    domList->list[domList->count++].name=strdup(fullName[aNum]);
	            }
		    break;
	   default:
		    free(fullName[aNum]);
		    return 1;
	   }
    aNum++;
    return 0;
}

catch_alm()
{
	/* connect takes too long */
	return(6);
}

ncmp(hostEntry *c1, hostEntry *c2)
{
	return(strcmp(c1->name, c2->name));
}
