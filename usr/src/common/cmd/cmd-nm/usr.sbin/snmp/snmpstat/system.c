#ident	"@(#)system.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)system.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
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

#ifndef lint
static char SNMPID[] = "@(#)system.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * Revision History: 
 *
 * 2/6/89 JDC Amended copyright notice 
 *
 * Professionalized error messages a bit 
 *
 * Expanded comments and documentation 
 *
 * Added code for genErr 
 *
 */

/*
 * system.c 
 *
 * print out the system entries 
 */

#include <unistd.h>
#include "snmpio.h"

int print_sys_info();
int print_sysII_info();

static char *dots[] = {
	"sysDescr",
	"sysObjectID",
	"sysUpTime",
	""
};
static char *dotsII[] = {
	"sysContact",
	"sysName",
	"sysLocation",
	"sysServices",
	""
};

/*----------------------------------------------------------------------------*

A structure used to generate singular and plurals of the words day,hour
miniute etc.

*----------------------------------------------------------------------------*/
typedef struct
{
	char	*cat_single;		/* Catalogue entry for singular */
	char	*cat_plural;		/* Catalogue entry for plural */
	char	*default_single;	/* Default singular string */
	char	*default_plural;	/* Default plural string */
}TIME_STRINGS;

/* Indexes into an array of TIME_STRINGS */
#define	DAY_STR		0
#define HOUR_STR	1
#define MIN_STR		2
#define SEC_STR		3
#define CENT_STR	4

/* An initialized array of TIME_STRINGS */
TIME_STRINGS time_strings[ CENT_STR + 1 ] =
	{
	{":56",":83"," %ld day"," %ld days"},
	{":57",":84"," %ld hour"," %ld hours"},
	{":58",":85"," %ld minute"," %ld minutes"},
	{":59",":86"," %ld second"," %ld seconds"},
	{":60",":87"," %ld hundredth"," %ld hundredths"}
	};

syspr(community)
	char *community;
{
	int ret;

	ret = doreq(dots, community, print_sys_info);
	(void) doreq(dotsII, community, print_sysII_info);
	return(ret);
}

/* ARGSUSED */
print_sys_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	OctetString *descr;
	char oid[128];
	unsigned long timeticks;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	descr = vb_ptr->vb_ptr->value->os_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	if (make_dot_from_obj_id(vb_ptr->vb_ptr->value->oid_value, oid) == -1) {
		fprintf(stderr, gettxt(":48", "snmpstat: can't translate %s\n"), dots[index]);
		return(-1);
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	timeticks = vb_ptr->vb_ptr->value->sl_value;

	if (lineno == 0) {
		printf(gettxt(":49", "System Group\n"));
	}

	print_octetstring(gettxt(":50", "Description"), descr);
	printf(gettxt(":51", "ObjectID: %s\n"), oid);
	print_timeticks(gettxt(":52", "UpTime"), timeticks);

	return(0);
}

/* ARGSUSED */
print_sysII_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	OctetString *contact;
	OctetString *name;
	OctetString *location;
	int services;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dotsII[index], vb_ptr->vb_ptr->name, lineno + 1)) {
		contact = NULL;
	} else {
		contact = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		name = NULL;
	} else {
		name = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		location = NULL;
	} else {
		location = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		services = 0;
	} else {
		services = vb_ptr->vb_ptr->value->sl_value;
	}

	if (contact != NULL)
		print_octetstring(gettxt(":53", "Contact"), contact);
	if (name != NULL)
		print_octetstring(gettxt(":54", "Name"), name);
	if (location != NULL)
		print_octetstring(gettxt(":55", "Location"), location);
	if (services != 0)
		print_services(services);

	return(0);
}

print_octetstring(label, os_ptr)
	char *label;
	OctetString *os_ptr;
{
	int i;

	printf("%s: ", label);
	for (i = 0; i < os_ptr->length; i++)
		putchar((int) os_ptr->octet_ptr[i]);
	putchar('\n');
}

/*----------------------------------------------------------------------------*

Function :	plural_time()

Description :	Generates one part of a time string in days, hours etc. This
		takes care of pluralization and international language
		support.

Given :		Type of time ( days, hours etc. )
		Value of time

Returns :	Pointer to a suitable print format string either in a static
		structure or the message catalogue.
		On internal error returns a fixed message

*----------------------------------------------------------------------------*/

char	*plural_time( int type, int value )

{
/* Check we have a valid index */
if( (type > CENT_STR) || (type < 0) )
	/* Ooops */
	return( "Time string error");

if( value == 1 )
	/* Return singular string */
	return( gettxt( time_strings[ type ].cat_single,
			time_strings[ type ].default_single ));
else
	/* Return plural string */
	return( gettxt( time_strings[ type ].cat_plural,
			time_strings[ type ].default_plural ));

}

/*---------------------------------------------------------------------------*

Function :	print_timeticks()

Description :	Prints out a time string

Given :		A lable to put in front of the string
		The time to print

Returns :	Nothing

*---------------------------------------------------------------------------*/
print_timeticks(label, timeticks)
	char *label;
	unsigned long timeticks;
{
	unsigned long tim;
	int docomma = 0;
	int time_units;				/* Current type of unit */

	/* This array contains division ratios between each type of time unit
	and timeticks ... do these calculations ONCE at build time */
	unsigned long ratios[ CENT_STR + 1] = { ( 24 * 60 * 60  * 100 * 1),
						( 60 * 60 * 100 * 1),
						( 60 * 100 * 1),
						( 100 * 1),
						( 1 )};
	printf("%s:", label);

	for( time_units = DAY_STR ; time_units <= CENT_STR ; time_units ++ )
	{
		/* Work out number of current time units */
		tim = timeticks / ratios[ time_units ] ;
		if (tim) 
		{
			if (docomma)
				printf(",");

			printf(plural_time( time_units , tim), tim);
			docomma = 1;
		}

		/* Work out remaining time ticks */
		timeticks %= ratios[ time_units];

		/* If no more ticks don't waste time finishing loop */
		if( timeticks == 0 )
			break;

	}

	printf("\n");
}

#define twoto(n)	(1 << (n))

char *services_list[] = {
	"none",
	"physical",
	"datalink/subnetwork",
	"internet",
	"end-to-end",
	"5",
	"6",
	"applications"
};

print_services(services)
	int services;
{
	int i;
	int need_comma;

	printf(gettxt(":61", "Services:"));

	i = sizeof(services_list)/sizeof(services_list[0]) - 1;
	need_comma = 0;
	while (services != 0 && i > 0) {
		if (services & twoto(i - 1)) {
			if (need_comma)
				printf(",");
			printf(" %s", services_list[i]);
			need_comma = 1;
			services -= twoto(i);
		}
		i--;
	}
	printf("\n");
}
