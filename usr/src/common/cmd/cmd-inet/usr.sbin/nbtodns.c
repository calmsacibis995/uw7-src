#ident	"@(#)nbtodns.c	1.5"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1996 SCO ltd
 * All rights reserved.
 *
 */

/* 
 * nbtodns 
 *
 * utility converts a netbios name to a DNS encoded name and vice verca 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/nb/nbuser.h>
#include <sys/nb/nb_const.h>
#include <libgen.h>

#include <locale.h>
#include "nb_msg.h"
nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif

#define DNS_NAMELEN  (NB_ENAMESIZE + 1)


char dnsname[DNS_NAMELEN];

char *
getnext(char *p, unsigned char *c)
{
	if (*p == NULL)
		return NULL;

	if(*p == '\\') {
		/* potential escaped character */
		if (strncmp(p, "\\0x", 3) == 0) {
			p += 3;
			if (sscanf(p, "%2x", c) != 1) {
				fprintf(stderr, MSGSTR(MSG_NB_ESC,
				   "Illegal escape sequence in name\n"));
				exit(1);
			};
			p+=2;
			return p;
		}
		/* not an escaped character */
	}
	*c = *p;
	return ++p;

}

void
usage(char *bin_name)
{
	fprintf(stderr, MSGSTR(MSG_NB_USAGE1, "Usage:nbtodns NetBIOS-name\n"));
	fprintf(stderr, MSGSTR(MSG_NB_USAGE2, "Usage:nbtodns -d DNS-name\n"));
	fprintf(stderr, MSGSTR(MSG_NB_USAGE3, "Usage:dnstonb DNS-name\n"));
	exit(1); 
}

cvtupper(char *str)
{
        while(*str)
                *str++ = toupper(*str);
}


void
dnstonb(char *str)
{
	unsigned char c, c1, c2;
	int i;
	char *p = str;

	printf("\"");

	for (i = 0; i < NB_NAMESIZE; i++) {
		c1 = *p++ - 'A';
		if (c1 < 0 || c1 > 15){
			fprintf(stderr, MSGSTR(MSG_NB_SCANFAIL, 
			   "\nError scan failed on name %s\n"), str);
			exit(1);
		}
		c2 = *p++ - 'A';
		if (c2 < 0 || c2 > 15){
			fprintf(stderr, MSGSTR(MSG_NB_SCANFAIL, 
			   "\nError scan failed on name %s\n"), str);
			exit(1);
		}
		c = (c1 << 4) | c2;
		if(c < ' ' || c > '~')
			printf("\\0x%2.2x", c);
		else
			printf("%c", c);
	}

	printf("\"\n");

	exit(0);
}

int
inrange(char c)
{
	if(isupper(c))
		return 1;
	if(isdigit(c))
		return 1;
	if(c == '-')
		return 1;

	return 0;
}

void
nbtodns(char *p)
{
	unsigned char c;
	int i;
	char *pp;
	char *wp = dnsname;
	char *pends = dnsname + DNS_NAMELEN -1;
	char tflag = 0;
	char *fb = NULL;

	/* figure out if it needs converting */

	pp = p;
	while(*pp){
		if((pp == p) && ! isupper(*pp)){
			tflag++;
			break;
		}
		if(!inrange(*pp)){	/* will find escaped chars as well */
			if ((*pp == NULL) || (*pp == ' ')){
				if (fb == NULL)
					fb = pp;
			}
			else {
				tflag++;
				break;
			}
		}
		else if(fb != NULL){	/* valid character - after  */
			tflag++;	/* a space or blank */
			break;	
		}
		pp++;
	}

	if(!tflag){
		printf("%s\n", p);	
		exit(0);
	}	

        /* encoding is according to the RFC */

	i = 0;
	do {
		p=getnext(p, &c);
                *wp++ = ((c >> 4) & 0xf) + 'A';
                *wp++ = (c & 0xf) + 'A';
		i++;
        } while (*p != NULL);

        *wp = 0;

	if (i != NB_NAMESIZE) {
		fprintf(stderr, MSGSTR(MSG_NB_NAMELEN, 
		   "NetBIOS name must be %d characters long\n"), NB_NAMESIZE);
		exit(1);
	}

	printf("%s\n", dnsname);
	exit(0);

}

main(int argc, char *argv[])
{
	setlocale(LC_ALL,"");
	catd = catopen(MF_NB, MC_FLAGS);

	if(strcmp(basename(argv[0]), "nbtodns") == 0) {
		switch(argc){
		case 2:
			nbtodns(argv[1]);
			break;
		case 3:
			dnstonb(argv[2]);
			break;
		default:
			usage(argv[0]);
		}
	} else  {		/* assume dnstonb */
		switch(argc){
		case 2:
			dnstonb(argv[1]);
			break;
		default:
			usage(argv[0]);
		}
	}
}
