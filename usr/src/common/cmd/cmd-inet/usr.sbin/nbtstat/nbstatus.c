#ident	"@(#)nbstatus.c	1.9"

/*      Copyright (c) 1997 The Santa Cruz Operation, Inc.. All Rights
 *	Reserved.    
 */
	
/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE SANTA CRUZ
 *	OPERATION INC.  
 *      The copyright notice above does not evidence any        
 *      actual or intended publication of such source code.     
 */
/*
 *
 * MODIFICATION HISTORY
 * WHEN		WHO	ID	WHAT
 * 23 Oct 97	stuarth	S000	Tidy up the name search code in show_astat
 *				to use a for{} loop & switch. Replace the U
 * 				in the nbtstat -c display with W for WINS.
 *
 */
#include <stdio.h>
#include <varargs.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <stropts.h>
#include <memory.h>
#include <errno.h>
#include <sys/nb/nbuser.h>
#include <sys/nb/nb_const.h>
#include <sys/nb/nb_ioctl.h>

#include <sys/nb/nbtpi.h>
#include <sys/sysmacros.h>

#include <sys/utsname.h>
#include <sys/time.h>

#include <locale.h>
#include "../nb_msg.h"
extern nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif

/* astat buffer holds "server" names
 * header is struct nb_astat (~ 60 bytes)
 * entries are nb_astat_name (~ 18 bytes)
 * 4096 buffer is sufficient for aprox 224 entries
 */
#define	ASTAT_BUFSIZE	4096

/* sstat buffer holds connections information
 * header is nb_ssnstat (~ 6 bytes)
 * entries are nb_ssninfo (~37 bytes)
 * 4096 buffer is sufficient for aprox 110 connections 
 * Code now resizes if this is insufficient
 */
#define SSTAT_BUFSIZE	4096
#define MAX_SSTAT_BUFSIZE 65535

#define GROUP_CACHE_BUFSIZE	(sizeof(struct nbioc_group) * MAXGROUPS)
#define HOST_CACHE_BUFSIZE	8192

#define MAX_NAME_TYPE	8
#define MAX_SESS_TYPE	8

char *name_type[MAX_NAME_TYPE];
char *sess_type[MAX_SESS_TYPE];


char	gethex();


void
init_arrays()
{
	name_type[0] = MSGSTR(MSG_NB_TYPE0,"Registration pending");
	name_type[1] = MSGSTR(MSG_NB_TYPEUNK,"???");
	name_type[2] = MSGSTR(MSG_NB_TYPEUNK,"???");
	name_type[3] = MSGSTR(MSG_NB_TYPEUNK,"???");
	name_type[4] = MSGSTR(MSG_NB_TYPE4,"Registered");
	name_type[5] = MSGSTR(MSG_NB_TYPE5,"Deregistered");
	name_type[6] = MSGSTR(MSG_NB_TYPE6,"Alias");
	name_type[7] = MSGSTR(MSG_NB_TYPE7,"Alias Deregistered");

	sess_type[0] = MSGSTR(MSG_NB_SESSUNK,"???");
	sess_type[1] = MSGSTR(MSG_NB_SESS1,"LISTEN");
	sess_type[2] = MSGSTR(MSG_NB_SESS2,"CALL");
	sess_type[3] = MSGSTR(MSG_NB_SESS3,"ESTAB");
	sess_type[4] = MSGSTR(MSG_NB_SESS4,"CLOSING");
	sess_type[5] = MSGSTR(MSG_NB_SESS5,"CLOSED");
	sess_type[6] = MSGSTR(MSG_NB_SESS6,"ABORT");
	sess_type[7] = MSGSTR(MSG_NB_SESSUNK,"???");
}
void
print_name(uchar_t *p)
{
	int	i;
	for (i = 0; i < NBNAMSZ - 1 ; i++, p++)
		if (isprint(*p))
			putchar(*p);
		else
			putchar('^');
	
	printf(" x%2.2x", (*p) & 0xff);
}

int
cache_print_name(uchar_t *p)
{
	int	i, flag = 0;
	int	count = 0;

	putchar('"');
	for (i = 0; i < NBNAMSZ  ; i++, p++)
	{
		if (isprint(*p))
		{
			putchar(*p);
			count++;
		}
		else
		{
			printf("\\0x%2.2x", (*p) & 0xff);
			count += 5;
		}
	}
	putchar('"');
	count += 2;

	return(count);
}
void
print_ipaddr(IPADDR	addr)
{
	uchar_t *a = (uchar_t *)&addr;

	printf("%d.%d.%d.%d", *a, *(a + 1), *(a + 2), *(a + 3));

}
/*
 * find_domain
 * Given a pointer to the group/member structure 'group', find_domain
 * searches the 'group' structure for 'name'.
 * Return values:	pointer to group_name if name found
 *			otherwise NULL
 */
char	*
find_domain(struct nbioc_group *group, char *name)
{
	struct	nbioc_group	*nptr;
	int			i;

	for (nptr = group; nptr <group + MAXGROUPS; nptr++)
	{
	    if (*nptr->name != '\0')
	    {
		for (i = 0; i < MAXMEMBERS; i++)
		{
		    if(memcmp((void *)nptr->member[i], (void *)name,
				NBNAMSZ) == 0)
			return (nptr->name);
		}
	    }
	}
	return ((char *)NULL);
}

void
show_sstat(int fd)
{
	ushort	i;
	int	size_sstat;
	struct	nb_ssnstat *sstatp;
	struct  nb_ssninfo *sinfop;
	struct  strioctl strioctl;

	size_sstat = SSTAT_BUFSIZE;

retry:
	if ((sstatp = (struct nb_ssnstat *) malloc(size_sstat)) == NULL) {
		fprintf(stderr, MSGSTR(MSG_NB_MALLOC,"malloc failed\n"));
		exit(2);
	}

	strioctl.ic_cmd = NBIOCTL_SSTAT;
	strioctl.ic_timout = 0;
	strioctl.ic_dp = (char *)sstatp;
	strioctl.ic_len = size_sstat;

	if (ioctl(fd, I_STR, &strioctl) < 0) {
		/* 
		 * if this fails assume it is 
		 * because too many sessions were running and the 
		 * ioctl buffer was insufficient.
		 * Reallocate bigger buffer and retry.
		 *
		 * Maximum size of buffer is controlled by 
		 * ushort ncb_length field in ncb structure 
		 * in kernel.
		 *
		 */
		if(size_sstat < MAX_SSTAT_BUFSIZE) {
			size_sstat *= 2;
			size_sstat = MIN(size_sstat, MAX_SSTAT_BUFSIZE);
			free(sstatp);
			goto retry;
		}
		perror("ioctl failed");
		fprintf(stderr, MSGSTR(MSG_NB_SSTAT,
		   "\nCannot obtain session status\n"));
		exit(1);
	}

	printf(MSGSTR(MSG_NB_HDR1, "\nSession status:\n"));
	printf(MSGSTR(MSG_NB_HDR2,
	    	"name num %u, # sessions %u, rcv dg=%u, rcv any=%u\n"),
		sstatp->ss_num, 
		sstatp->ss_nsession, 
		sstatp->ss_nrcvdg,
		sstatp->ss_nrcvany);
	printf(MSGSTR(MSG_NB_HDR3, 
	"LSN\tState\tLocal Name      Soc  Remote Name     Soc  rcvs  sends\n"));

	for (i = 0; i < sstatp->ss_nsession; i++) {
		sinfop = &sstatp->ss_session[i];
		printf("%u\t%s\t", sinfop->si_lsn, sess_type[sinfop->si_state]);
		print_name(sinfop->si_lname);
		printf("  ");
		print_name(sinfop->si_rname);
		printf("  %4u  %4u\n", sinfop->si_nrecv, sinfop->si_nsend);
	}
}

show_astat(int fd, char *nbname)
{
	int		i, size_astat;
	int		j, k, try;
	char	c, *ptr;
	char	name[NBNAMSZ];		/* name used in status query */
	char	*astatp_name;

	static char	dispname[ (NBNAMSZ * 4) + 1 ];	/* name displayed */
	struct nb_astat	*astatp;
	struct nb_astat_name *astatnp;
	struct strioctl	strioctl;

	size_astat = ASTAT_BUFSIZE;

	if ((astatp = (struct nb_astat *) malloc(size_astat)) == NULL) {
		fprintf(stderr, MSGSTR(MSG_NB_MALLOC,"malloc failed\n"));
		exit(2);
	}

	astatp_name = (char *)astatp;
/* WINS_CLI vvvv */
	if (nbname) {
		/* Space fill the name to NBNAMSZ */

		memset( name, ' ', NBNAMSZ );

		i = strlen(nbname);

		if ( nbname[0] == '{' ) {
			if ( nbname[i-1] != '}' ) {
				/*
				 * printf("Name <%s> missing end brace\n", nbname);
				 */
				fprintf(stderr, MSGSTR(MSG_NB_NLEN, 
			   		"Name <%s> is too long\n"), nbname);
				exit(3);
			}
			i -= 2;
			ptr = nbname + 1;
			for ( j = 0, k = 0; k < NBNAMSZ && j < i; k++, j++ ) {
				c = ptr[j];
				if (c == '\\' && ptr[j+1] == '0' && ptr[j+2] == 'x'
					&& isxdigit(ptr[j+3]) && isxdigit(ptr[j+4])) {
						c = gethex(ptr+j+3);
						j += 4;
				}
				name[k] = c;
			}
			if ( j < i && k == NBNAMSZ ) {
				fprintf(stderr, MSGSTR(MSG_NB_NLEN, 
			   		"Name <%s> is too long\n"), nbname);
				exit(3);
			}
		}	
		else {
			if (i > NBNAMSZ) {
				fprintf(stderr, MSGSTR(MSG_NB_NLEN, 
			   		"Name <%s> is too long\n"), nbname);
				exit(3);
			}
			memcpy(name, nbname, i);
		}
	}
	else {
		memset( name, 0, NBNAMSZ );
		*name = '*';
	}

/* WINS_CLI ^^^^ */

	for ( i = 0, j = 0; i < NBNAMSZ && j < sizeof(dispname); i++, j++ ) 
	{
		c = name[i];
		if ( c < ' ') {
			ptr = dispname + j;
			sprintf(ptr, "<%0.2x>",c);
			j += 3;
		}
		else
			dispname[j] = c;
	}
	dispname[j] = '\0';

	strioctl.ic_cmd	= NBIOCTL_ASTAT;
	strioctl.ic_timout = -1;
	strioctl.ic_dp	= (char *)astatp;
	strioctl.ic_len	= size_astat;

#define	UPPER_CASE_SPC	0
#define	LOWER_CASE_SPC	1
#define	UPPER_CASE_NULL	2
#define	LOWER_CASE_NULL	3
#define AS_IS			4
#define	NO_MORE			5

	if (*nbname == '{' || *name == '*')
		try = AS_IS;
	else
		try = 0;

	for (; try < NO_MORE; try++)
	{
		switch(try)
		{
			case UPPER_CASE_SPC:
				for ( j = 0; j < NBNAMSZ; j++ )
					astatp_name[j] = toupper(name[j]);
				astatp_name[NBNAMSZ - 1] = ' ';
				break;
			case UPPER_CASE_NULL:
				for ( j = 0; j < NBNAMSZ; j++ )
					astatp_name[j] = toupper(name[j]);
				astatp_name[NBNAMSZ - 1] = '\0';
				break;
			case LOWER_CASE_SPC:
				for ( j = 0; j < NBNAMSZ; j++ )
					astatp_name[j] = tolower(name[j]);
				astatp_name[NBNAMSZ - 1] = ' ';
				break;
			case LOWER_CASE_NULL:
				for ( j = 0; j < NBNAMSZ; j++ )
					astatp_name[j] = tolower(name[j]);
				astatp_name[NBNAMSZ - 1] = '\0';
				break;
			case AS_IS:
			default:
				memcpy(astatp_name, name, NBNAMSZ);
				break;
		}
	    
		/* Now do the actual ioctl				*/
		if (ioctl(fd, I_STR, &strioctl) >= 0) 
			break;
#ifdef	DEBUG_NB
		else
		{
			printf("DEBUG:No response on try %d for ", 
				 try);
			(void )cache_print_name((uchar_t *)astatp_name);
			printf("\n");
		}
#endif	/* DEBUG_NB */
	}
	if (try == NO_MORE)
	{
	    fprintf(stderr, MSGSTR(MSG_NB_NORESP,
	       "No response for %s\n"), dispname);
	    exit(1);
	}

	/*
	 * Windows interprets this field as the MAC address
	 * This NETBIOS implementation interprets bytes 2 thru 5 as the 
	 * IP address; NetBIOS is a bit far up the protocol stack to be 
	 * concerned about MAC addresses.
	 * Bytes 0 & 1 are a mystery
	 */
#ifndef	NEEDED	
	printf(MSGSTR(MSG_NB_HDR6, "UNIT ID = "));

	for (i = 0; i < 6; i++)
		printf(" %2.2X", astatp->as_unit[i] & 0xff);
	printf ("\n");

#else
	printf(MSGSTR(MSG_NB_HDR4, "Node ip address:"));

	for (i = 2; i < 6; i++)
		printf(" %2.2X ", astatp->as_unit[i] & 0xff);
#endif	

	if (nbname)
		printf(MSGSTR(MSG_NB_RNT, 
		   "Remote name table (%d names):\n"), astatp->as_names);
	else
		printf(MSGSTR(MSG_NB_LNT,
		   "Local name table (%d names):\n"), astatp->as_names);
	printf(MSGSTR(MSG_NB_HDR5, "\nName            Soc Num Status\n"));

	astatnp = (struct nb_astat_name *)(astatp+1);
	for (i = 0; i < astatp->as_names; i++, astatnp++) {
		print_name(astatnp->asn_name);
		printf(" %3u %s %s\n", astatnp->asn_num,
			(astatnp->asn_flags & NB_ASTAT_GROUP) ? 
					"Group " : "Unique",
			name_type[astatnp->asn_flags & NB_ASTAT_DD]);
	}

}

void show_astat_ip (int nb_fd, unsigned long ipaddr)
{
}
void show_cache (int nb_fd)
{
	struct	nbioc_group	*groupbuf, *nptr;
	struct	nb_namestat	*hostbuf;
	struct 	nb_csstat	*host;
	int			count, i;
	char			*domain_name;
	struct strioctl		strioctl;
	time_t			current_time;
	struct utsname		sysname;

#ifdef	DEBUG_NB
	printf("show_cache: nb_fd = %d, GROUP_CACHE_BUFSIZE = %d\n", 
			nb_fd, GROUP_CACHE_BUFSIZE);
#endif
	/* malloc space to receive group names */
	if ((groupbuf = (struct nbioc_group *) malloc(GROUP_CACHE_BUFSIZE)) == NULL)
	{
		fprintf(stderr, MSGSTR(MSG_NB_MALLOC,"malloc failed\n"));
		exit(2);
	}

	/* send ioctl to receive group names */

	strioctl.ic_cmd	= NBIOCTL_CACHE_GETGRP;
	strioctl.ic_timout = 0;
	strioctl.ic_dp	= (char *)groupbuf;
	strioctl.ic_len	= GROUP_CACHE_BUFSIZE;


	if (ioctl(nb_fd, I_STR, &strioctl) < 0) 
	{
		fprintf(stderr, MSGSTR(MSG_NB_NOGROUP,
			   "ioctl to get group names fails\n"));
		exit(1);
	}

#ifdef	DEBUG_NB
	/* print group names as heading comment */

	for (nptr = groupbuf; nptr < groupbuf + MAXGROUPS; nptr++)
	{
	    if((*nptr->name) != '\0')
	    {
		int l = 0;

		printf("\n# Group name: ");
		cache_print_name((uchar_t *) nptr->name);
		printf("\n# Members: ");
		for (i = 0; i < MAXMEMBERS; i++)
		{
		    char *c = nptr->member[i];
		    if (*c != '\0')
		    {
			l += cache_print_name((uchar_t *) c);
			if (l >= 60)
			{
			    printf("\n#");
			    l = 0;
			}
		    }
		    else
			break;
		}
	    }
	}

#endif	/* DEBUG_NB */
	/* malloc space to receive unique names */

	if ((hostbuf = (struct nb_namestat *)malloc(HOST_CACHE_BUFSIZE)) == NULL)
	{
		fprintf(stderr, MSGSTR(MSG_NB_MALLOC,"malloc failed\n"));
		exit(2);
	}

	/* send ioctl to receive host names */

#ifdef	DEBUG_NB
	printf("\thostbuf 0x%x\n", hostbuf);
#endif

	strioctl.ic_cmd	= NBIOCTL_CACHE_GETHOST;
	strioctl.ic_timout = 0;
	strioctl.ic_dp	= (char *)hostbuf;
	strioctl.ic_len	= HOST_CACHE_BUFSIZE;


	if (ioctl(nb_fd, I_STR, &strioctl) < 0) 
	{
		fprintf(stderr, MSGSTR(MSG_NB_NOHOST,
			   "ioctl to get host names fails\n"));
		exit(1);
	}

	count = hostbuf->nm_num_unam;
	host = &(hostbuf->nm_csstat[0]);
#ifdef	DEBUG_NB
	printf("\t# unique names %d, host 0x%x\n", count, host);
#endif
	(void) uname(&sysname);

	/* print heading */
	printf("\n#\n");
	printf(MSGSTR(MSG_NB_LNCHEAD1,
		"#  Contents of the local name cache for %s\n"),
		sysname.nodename);
	current_time = time(0);
	printf("#  %s\n", ctime(&current_time));

	printf(MSGSTR(MSG_NB_LNCHEAD2, 
		"# IP address	Remote name \t\t[#DOM: domain_name]        #Flags Time\n"));
	/* print each unique name as :-
	   ip address	name	[#DOM]	# flags   time		*/

	for (i = 0; i < count; i++, host++)
	{
	    print_ipaddr(host->cs_addr);
	    printf("\t");
	    cache_print_name((uchar_t *)host->cs_name);
	    printf("\t");
	    domain_name = find_domain(groupbuf, (char *)host->cs_name);
	    if (domain_name)
	    {
		printf("#DOM:");
		cache_print_name((uchar_t *)domain_name);
	    }
	    else
		printf("\t\t\t   ");
	    printf("%s", ((host->cs_flags) & NB_NSR_GROUP) ? "#GRP" : "#UNI");
	    printf("%s", ((host->cs_flags) & (NB_NSR_PENDING_WINS1 | NB_NSR_PENDING_WINS2)) ? " W " :"   ");
	    printf("%s", ((host->cs_flags) & NB_NSR_PENDING) ? "P" :" ");
	    printf("%s", ((host->cs_flags) & NB_NSR_PENDING_RES) ? "R" :"");
	    printf("%s", ((host->cs_flags) & NB_NSR_PENDING_BC) ? "B" :"");
	    if (!((host->cs_flags) & NB_NSR_STATIC))
	        printf("%d", host->cs_time);
	    printf("\n");
	}


}

void clear_cache (int nb_fd)
{
	struct strioctl	strioctl;

	strioctl.ic_cmd	= NBIOCTL_CACHE_DELALL;
	strioctl.ic_timout = 0;
	strioctl.ic_dp	= (char *)0;
	strioctl.ic_len	= 0;
#ifdef	DEBUG_NB
	printf("clear_cache\n");
#endif
	if (ioctl(nb_fd, I_STR, &strioctl) < 0) 
	{
		fprintf(stderr, MSGSTR(MSG_NB_CDAFAIL,
			   "Cache_delete_all failed\n"));
	}
}

/* WINS_CLI vvvv */
char gethex(char *ptr)
{
	int	hi, lo;

	hi = isdigit(ptr[0])
			? (ptr[0] - '0') : ((ptr[0] - 'a') + 10);
	lo = isdigit(ptr[1])
			? (ptr[1] - '0') : ((ptr[1] - 'a') + 10);
	return((char)((hi * 16) + lo));
}
/* WINS_CLI ^^^^ */
