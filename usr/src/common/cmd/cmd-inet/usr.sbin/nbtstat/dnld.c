#ident "@(#)dnld.c	1.4"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Lachman Technology, Inc.
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
/*      SCCS IDENTIFICATION        */

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stropts.h>

#include <netinet/in.h>
#include <sys/nb/nb_const.h>
#include <sys/nb/nb_ioctl.h>
#include <sys/nb/nbuser.h>

#include "nbdnld.h"
#include <sys/errno.h>
extern int errno;

#include <locale.h>
#include "../nb_msg.h"

#include <sys/nb/nbtpi.h>

extern nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif


#define bzero(s, len)   memset(s, 0, len)

struct	nbhosts	{
	struct	nbioc_host	nbh;
	struct	nbhosts		*next;
};

struct	nbgroups	{
	struct	nbioc_group	nbg;
	struct	nbgroups	*next;
};

struct	nbhosts		*nbh_head;
struct	nbgroups	*nbg_head;

extern char *strtok();
extern FILE *yyin;

extern void print_name(uchar_t *p);

void
buildnbname(s1,s2)
char *s1, *s2;
{
	char *p1, *p2;

	for ( p1=s1, p2=s2; *p1 != '\0' && p2 < s2 + NB_NAMESIZE; *p2++, *p1++ ) {
		if ( islower(*p1) )
			*p2 = toupper(*p1);
		else
			*p2 = *p1;
	}
	for ( ; p2 < s2 + NB_NAMESIZE; *p2++ = ' ' );
}
void
dlnbhost(int fd, struct nbioc_host *nbh)
{
#ifdef TEST
	write(2,nbh,sizeof(struct nbioc_host));
#else
	struct strioctl strioctl;
#ifdef	DEBUG_NB
	printf("dlnbhost: ip addr 0x%x, name <", nbh->ipaddr);
	print_name((uchar_t *) nbh->name);
	printf(">\n");
#endif	/* DEBUG_NB */
	strioctl.ic_cmd	= NBIOCTL_CACHE_ADDHOST;
	strioctl.ic_timout = 0;
	strioctl.ic_len	= sizeof(struct nbioc_host);
	strioctl.ic_dp	= (char *)nbh;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
		perror("NBHOSTS ioctl failed");
		exit(1);
	}
#endif	/* TEST */
}

void
dlnbgroup(int fd, struct nbioc_group *nbg)
{
#ifdef TEST
	write(2,nbg,sizeof(struct nbioc_group));
#else
	struct strioctl strioctl;
#ifdef	DEBUG_NB
	int	i;
	printf("dlnbgroup: name <");
	print_name((uchar_t *)nbg->name);
	printf(">\n");
	for (i = 0; i < MAXMEMBERS; i++)
	{
		print_name((uchar_t *) nbg->member[i]);
		printf(" ");
		if ((i + 1) % 4 == 0)
			printf("\n");

	}
#endif	/* DEBUG_NB */
	strioctl.ic_cmd	= NBIOCTL_CACHE_ADDGRP;
	strioctl.ic_timout = 0;
	strioctl.ic_len	= sizeof(struct nbioc_group);
	strioctl.ic_dp	= (char *)nbg;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
		perror("NBGROUP ioctl failed");
		exit(1);
	}
#endif	/* TEST */
}

#ifdef DEBUG_NB
char *
cvt_nbname(char *name, char *str)
{
	memcpy(str, name, NB_NAMESIZE);
	str[NB_NAMESIZE] = '\0';
	return(str);
}
#endif

/*  Routine: store_hostname()
 *
 *	Called from parser only
 *
 *	Input: nbname	Validated NetBIOS host name (16 chars, space filled)
 *		   ipaddr	Validated IP address (output of inet_addr())
 *
 */

void
store_hostname(char *nbname, unsigned long ipaddr)
{
	struct	nbhosts	*nbhp;

#ifdef DEBUG_NB
	char prtname[NB_NAMESIZE+1];
	printf("Store hostname([%s], 0x%0x)\n",
				cvt_nbname(nbname, prtname), ipaddr);
#endif

	if ( (nbhp = (struct nbhosts *)malloc(sizeof(struct nbhosts))) == NULL ) {
		errno=ENOMEM;
		perror("");
		exit(1);
	}
	memcpy(nbhp->nbh.name, nbname, NB_NAMESIZE);
	nbhp->nbh.ipaddr = ipaddr;
	nbhp->next = nbh_head;
	nbh_head = nbhp;
}

/*  Routine: store_domainname()
 *
 *	Called from parser only
 *
 *	Input: nbgname	Validated NetBIOS group name (16 chars, space filled)
 *		   nbname	Validated NetBIOS host name (16 chars, space filled)
 *
 */

void
store_domainname(char *nbgname, char *nbname)
{
	register	i;
	int			found;
	struct		nbgroups	*nbgp;

#ifdef DEBUG_NB
	char prtname1[NB_NAMESIZE+1];
	char prtname2[NB_NAMESIZE+1];
	printf("Store domainname([%s], [%s])\n",
				cvt_nbname(nbgname,prtname1), cvt_nbname(nbname,prtname2));
#endif

	for ( nbgp = nbg_head; nbgp; nbgp = nbgp->next ) {
		if ( memcmp(nbgp->nbg.name, nbgname, NB_NAMESIZE) == 0 ) {
			break;
		}
	}

	if ( nbgp == NULL ) {
		if ( (nbgp = (struct nbgroups *)malloc(sizeof(struct nbgroups)))
					== NULL ) {
			errno=ENOMEM;
			perror("");
			exit(1);
		}
		bzero ((char *)nbgp, sizeof(struct nbgroups));
		memcpy(nbgp->nbg.name, nbgname, NB_NAMESIZE);
		nbgp->next = nbg_head;
		nbg_head = nbgp;
	}

	for ( i = 0; i < MAXMEMBERS; i++ ) {
		if ( nbgp->nbg.member[i][0] == '\0' ) {
			memcpy(nbgp->nbg.member[i], nbname, NB_NAMESIZE);
			break;
		}
	}
}

lmhosts(int fd, char *nbfile)
{
	register i,j;

	char	*nbgname, *nbname, *nbaddr;
	char 	*groupmembers[MAXMEMBERS];
	int	groupcount;
	FILE	*fp;
	struct	nbhosts		*nbhp;
	struct	nbgroups	*nbgp;
	struct	nbioc_host	nbhost;
	struct	nbioc_group	nbgroup;

	nbgname = nbname = nbaddr = NULL;
	groupcount = 0;

	if ( (fp = fopen(nbfile, "r")) == (FILE *)0 ) {
		fprintf(stderr, MSGSTR(MSG_NB_EOFILE, 
		   "Unable to open input file %s\n"), nbfile);
		exit(1);
	}
#ifdef	DEBUG_NB
	printf("Loading cache from %s\n", nbfile);
#endif
	nbh_head = (struct nbhosts *)NULL;
	nbg_head = (struct nbgroups *)NULL;
	yyin = fp;
	yyparse();

#ifdef DEBUG_NB
	printf("Parse finished OK\n");
#endif

	for ( nbhp = nbh_head; nbhp; nbhp = nbhp->next ) {

		if ( nbhp->nbh.name[NB_NAMESIZE-1] != 0x20 ) {
			/* Special value in last byte - application specific */

			dlnbhost(fd, &(nbhp->nbh));
		}
		else { /* Normal NetBIOS name - add entries required by LM/AS */

			nbhp->nbh.name[NB_NAMESIZE-1] = 0x20;
			dlnbhost(fd, &(nbhp->nbh));

			nbhp->nbh.name[NB_NAMESIZE-1] = 0x00;
			dlnbhost(fd, &(nbhp->nbh));

			nbhp->nbh.name[NB_NAMESIZE-1] = 0x03;
			dlnbhost(fd, &(nbhp->nbh));
		}
	}

	for ( nbgp = nbg_head; nbgp; nbgp = nbgp->next ) {

		nbgp->nbg.name[NB_NAMESIZE-1] = 0x1c;  /* Group names end <1C> */
		dlnbgroup(fd, &(nbgp->nbg));
	}

	fclose(fp);
}
