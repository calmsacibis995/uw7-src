#ident	"@(#)pcintf:pkg_rlock/rlockshm.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)rlockshm.c	3.4);  /* 8/21/91 21:43:47 */

/*
   (c) Copyright 1985 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.	 No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*---------------------------------------------------------------------------
 *
 *	rlockshm.c - initialize the RLOCK package's shared memory
 *
 *	usage:
 *		rlockshm [-cdmor] [-AFHLOUV] [name=data] ...
 *	where:
 *		documented:
 *		-c	- create the shared memory segment
 *		-d	- show the default configuration (only)
 *		-m	- display miscellaneous existing segment information
 *		-o	- handle the old style (multi-segment) shared memory
 *		-r	- remove the shared memory segment
 *
 *		name=data	- zero or more configuration settings (new style
 *				  shared memory only)
 *
 *		undocumented:
 *		-A	- display all (even unused) entries
 *		-F	- display the file header table
 *		-H	- display the hashed file header table
 *		-L	- display the record lock table
 *		-O	- display the open file table
 *		-U	- print usage with undocumented options (only)
 *		-V	- print the version/copyright (only)
 *
 *	the documented options must be supported for the customer's use.  the
 *	undocumented ones are only for Locus support, and need not be defined
 *	for the customer.
 *
 *	normal operation is to attach to the shared memory, not to display
 *	anything.  this has the effect of creating the default segment if it
 *	doesn't already exist, and verifing the ability to attach to it in
 *	any case.  however, some options will do the operation and immediately
 *	exit, doing nothing else (marked with 'only').
 *
 *	modifications to the default configuration parameters can be made
 *	by adding one or more "name=value" strings following the options.
 *	note that this is only available for the new style shared memory.
 *
 *---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include <rlock.h>
#include <lmf.h>

#include <internal.h>

/* Defines for NLS */
/* N = file name, L = Lang, C = Codeset */
#define NLS_PATH "/usr/pci/%N/%L.%C:/usr/lib/merge/%N/%L.%C"

#define NLS_FILE "dosmsg"
#define NLS_LANG "En"

#define NLS_DOMAIN "LCC.PCI.UNIX.RLOCKSHM"

/* Explanation: The path will be /pci/lib/en.lmf, the domain will
 * be: LCC.PCI.UNIX.RLOCKSHM
 */

extern char *optarg;
extern int  optind;

/*
 * standard exit status values.
 */

#define EX_GOOD		0			/* all is well */
#define EX_BAD		1			/* something is screwed */

/*
 * internal variables.
 */

static char *usageP = "Usage: %s [-cdmor] [name=data] ...\n";
static char *fullUsageP = "Usage: %s [-cdmor] [-AFHLOUV] [name=data] ...\n";

/*
 * internal functions.
 */

static void displayConfig();
static void dispMisc();
static void dispFileTable();
static void dispHashTable();
static void dispLockTable();
static void dispOpenTable();

/*
 *	main() - main routine
 *
 *	input:	argc - count of arguments
 *		argV - vector of same
 *
 *	proc:	initialize shared memory.
 *
 *	output:	(int) - standard UNIX process return value
 */

int
main(argc, argV)
int argc;
char *argV[];
{	int optChar, retCode, cfgIndex;
	char *actionStrP;
	bool createShm, attachShm, oldStyle;
	bool showAll, dMisc, dFileTable, dHashTable, dLockTable, dOpenTable;

	/*
	 * set global system needs and get the options.
	 */

	createShm	= FALSE;
	attachShm	= TRUE;
	oldStyle	= FALSE;
	showAll		= FALSE;
	dMisc		= FALSE;
	dFileTable	= FALSE;
	dHashTable	= FALSE;
	dLockTable	= FALSE;
	dOpenTable	= FALSE;

	/* Open the message file && push the domain */
	if (lmf_open_file(NLS_FILE, NLS_LANG, NLS_PATH) >= 0 &&
	    lmf_push_domain(NLS_DOMAIN)) {
		fprintf(stderr,
			"%s: Can't push domain \"%s\", lmf_errno %d\n",
			argV[0], NLS_DOMAIN, lmf_errno);
	}

	usageP = lmf_get_message_copy("RLOCKSHM900", usageP);

	fullUsageP = lmf_get_message_copy("RLOCKSHM901", fullUsageP);

	while ((optChar = getopt(argc, argV, "cdmorAFHLOUV")) != -1)
	{
		switch (optChar)
		{
			case 'c':	/* create the shared memory */
				createShm = TRUE;
				attachShm = FALSE;
				break;
			case 'd':	/* display default config (only) */
				displayConfig();
				return EX_GOOD;
			case 'm':	/* display misc. information */
				dMisc = TRUE;
				break;
			case 'o':	/* use old-style shared memory */
				oldStyle = TRUE;
				break;
			case 'r':	/* remove the shared memory */
				createShm = FALSE;
				attachShm = FALSE;
				break;
			case 'A':	/* display 'all' information */
				showAll = TRUE;
				break;
			case 'F':	/* display the file table */
				dFileTable = TRUE;
				break;
			case 'H':	/* display the hash table */
				dHashTable = TRUE;
				break;
			case 'L':	/* display the lock table */
				dLockTable = TRUE;
				break;
			case 'O':	/* display the open file table */
				dOpenTable = TRUE;
				break;
			case 'U':	/* full usage message (only) */
				printf(fullUsageP, argV[0]);
				return EX_GOOD;
			case 'V':	/* display the version (only) */
				fputs("\nVersion 1.1\n", stdout);
				fputs("Copyright (c) Locus Computing", stdout);
				fputs(" Corporation 1985, 1986,", stdout);
				fputs(" 1988, 1989.\n", stdout);
				fputs(lmf_get_message("RLOCKSHM5",
					"All Rights Reserved.\n\n"), stdout);
				return EX_GOOD;
			default:
				printf(usageP, argV[0]);
				return EX_BAD;
		}
	}
	if (oldStyle && (argc != optind))
	{
		printf(usageP, argV[0]);
		fputs(lmf_get_message("RLOCKSHM6",
			"Config is valid only with new style shared memory.\n"),
			stdout);
		return EX_BAD;
	}

	/*
	 * decode and set the configuration values.
	 */

	for (cfgIndex = optind; cfgIndex < argc; cfgIndex++)
	{
		if (!_rlSetCfg(argV[cfgIndex]))
			return EX_BAD;
	}

	/*
	 * create or remove the shared table segment.
	 */

	if (createShm)
	{
		retCode = oldStyle ? _rlMCreate() : rlockCreate();
		actionStrP = "initialize";
	}
	else if (attachShm)
	{
		retCode = oldStyle ? _rlMAttach() : rlockAttach();
		actionStrP = "attach";
	}
	else
	{
		retCode = oldStyle ? _rlMRemove() : rlockRemove();
		actionStrP = "remove";
	}

	/*
	 * if we created or attached, and the function didn't fail, we may want
	 * to see the data.
	 */

	if ((createShm || attachShm) && (retCode == RL_SUCCESS))
	{
		if (dMisc) dispMisc();
		if (dFileTable) dispFileTable(showAll);
		if (dHashTable) dispHashTable(showAll);
		if (dLockTable) dispLockTable(showAll);
		if (dOpenTable) dispOpenTable(showAll);
	}

	/*
	 * figure out what went on, tell the user if we failed.
	 */

	if (retCode == RL_SUCCESS)
		return EX_GOOD;

	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("RLOCKSHM7",
		"Unable to %1 common shared memory, "),
		"%s" , actionStrP),
		stdout);
	if (rlockErr == RLERR_SYSTEM)
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM8",
			"(system error: %1).\n"),
			"%d" , errno),
			stdout);
	else
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM9","%1.\n"),
			"%s" , rlockEString(rlockErr)),
			stdout);
	return EX_BAD;
}

/*
 *	STATIC	displayConfig() - display the default configuration
 *
 *	input:	(none)
 *
 *	proc:	just print the default configuration.
 *
 *	output:	(void) - none
 *
 *	global:	_rlCfgData - checked
 */

static void
displayConfig()
{	r0 int index;

	fputs(lmf_get_message("RLOCKSHM10",
		"\nDefault configuration information (new-style only):\n\n"), 
		stdout);
	if (nil(_rlCfgData.cfgBaseP))
		fputs(lmf_get_message("RLOCKSHM11",
			"\tsegment attach address\t\t(program selected)\n"),
			stdout);
	else
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM12",
			"\tsegment attach address\t\t%1\n"),
			"%#.8lx" , _rlCfgData.cfgBaseP),
			stdout);
	fputs("\n", stdout);
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("RLOCKSHM14",
		"\tshared memory key\t\t%1\n"),
		"%#.8lx", _rlCfgData.cfgShmKey),
		stdout);
	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("RLOCKSHM15", "\tlockset key\t\t\t%1\n"),
		"%#.8lx" , _rlCfgData.cfgLsetKey),
			stdout);
	fputs("\n", stdout);
	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("RLOCKSHM17",
		"\topen file table entries\t\t%1\n"),
		"%d" , _rlCfgData.cfgOpenTable),
		stdout);
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("RLOCKSHM18",
		"\tfile header table entries\t%1\n"),
		"%d" , _rlCfgData.cfgFileTable),
		stdout);
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("RLOCKSHM19",
		"\thashed file table entries\t%1\n"),
		"%d" , _rlCfgData.cfgHashTable),
		stdout);
	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("RLOCKSHM20",
		"\trecord lock table entries\t%1\n"),
		"%d" , _rlCfgData.cfgLockTable),
		stdout);
	fputs("\n", stdout);
	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("RLOCKSHM22",
		"\tindividual record locks\t\t%1\n"),
		"%d" , _rlCfgData.cfgRecLocks),
		stdout);
	fputs("\n", stdout);
	fputs(lmf_get_message("RLOCKSHM24",
		"These data may be changed with these configurable"),
		stdout);
	fputs(lmf_get_message("RLOCKSHM25"," parameter names:\n"),
		stdout);
	fputs("\n", stdout);
	for (index = 0; !nil(_rlCfgNames[index].cnNameP); index++)
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM27","\t%1 - %2\n"),
			"%s%s" , 
			_rlCfgNames[index].cnNameP, 
			_rlCfgNames[index].cnDescP),
			stdout);
}

/*
 *	STATIC	dispMisc() - display miscellaneous information
 *
 *	input:	(none)
 *
 *	proc:	display the information kept in the _rlockShm table.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

static void
dispMisc()
{
	/*
	 * tell the user what is being printed, then print it.
	 */

	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("RLOCKSHM28",
		"\nMiscellaneous existing memory information (%1 style):\n\n"),
		_rlockShm.useOldStyle ? "old" : "new", "%s" ),
		stdout);
	if (_rlockShm.useOldStyle)
	{
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM29",
			"\topen file table entries\t\t%1 @ %2 bytes\n"),
			"%d%d" , _rlockShm.openSize, sizeof(oldOpenFile)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM30",
			"\tfile header table entries\t%1 @ %2 bytes\n"),
			"%d%d" , _rlockShm.fhdrSize, sizeof(oldFileHdr)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM31",
			"\thashed file table entries\t%1 @ %2 bytes\n"),
			"%d%d" , _rlockShm.hfhdrSize, sizeof(indexT)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM32",
			"\trecord lock table entries\t%1 @ %2 bytes\n"), 
			"%d%d" , _rlockShm.lockSize, sizeof(oldRecLock)),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM34",
			"\topen segment base\t\t%1\n"),
			"%#.8lx" , _rlockShm.oOpenSegmentP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM35",
			"\tfile segment base\t\t%1\n"),
			"%#.8lx" , _rlockShm.oFhdrSegmentP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM36",
			"\tlock segment base\t\t%1\n"),
			"%#.8lx" , _rlockShm.oLockSegmentP),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM38",
			"\topen file table\t\t\t%1\n"), 
			"%#.8lx", _rlockShm.oOpenTableP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM39",
			"\tfile header table\t\t%1\n"), 
			"%#.8lx", _rlockShm.oFhdrTableP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM40",
			"\thashed file table\t\t%1\n"),
			"%#.8lx", _rlockShm.oHfhdrTablePP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM41",
			"\trecord lock table\t\t%1\n"),
			"%#.8lx" , _rlockShm.oRlockTableP),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM43",
			"\topen table free list\t\t%1\n"),
			"%#.8lx" , *_rlockShm.oOpenFreePP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM44",
			"\tfile table free list\t\t%1\n"),
			"%#.8lx" , *_rlockShm.oFhdrFreePP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM45",
			"\tlock table free list\t\t%1\n"),
			"%#.8lx" , *_rlockShm.oLockFreePP),
			stdout);
	}
	else
	{
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM46",
			"\topen file table entries\t\t%1 @ %2 bytes\n"),
			"%d%d", _rlockShm.openSize, sizeof(openFileT)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM47",
			"\tfile header table entries\t%1 @ %2 bytes\n"),
			"%d%d" , _rlockShm.fhdrSize, sizeof(fileHdrT)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM48",
			"\thashed file table entries\t%1 @ %2 bytes\n"),
			"%d%d", _rlockShm.hfhdrSize, sizeof(indexT)),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM49",
			"\trecord lock table entries\t%1 @ %2 bytes\n"),
			"%d%d", _rlockShm.lockSize, sizeof(recLockT)),
			stdout);
      		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM50",
			"\ttotal segment size\t\t%1 bytes\n"),
			"%ld" , _rlockShm.segSize),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM52",
			"\tindividual record locks\t\t%1\n"),
			"%d", _rlockShm.recLocks),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM54",
			"\tattachment base\t\t\t%1"), 
			"%#.8lx", _rlockShm.segmentP),
			stdout);
		if (_rlCfgData.cfgBaseP == 0) 
			fputs(lmf_get_message("RLOCKSHM55",
				" (program selected)"),
				stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM57",
			"\topen file table\t\t\t%1\n"), 
			"%#.8lx", _rlockShm.openTableP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM58",
			"\tfile header table\t\t%1\n"),
			"%#.8lx", _rlockShm.fhdrTableP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM59",
			"\thashed file table\t\t%1\n"), 
			"%#.8lx", _rlockShm.hfhdrTableP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM60",
			"\trecord lock table\t\t%1\n"), 
			"%#.8lx", _rlockShm.rlockTableP),
			stdout);
		fputs("\n", stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM62",
			"\topen table free list index\t%1\n"), 
			"%ld", *_rlockShm.openFreeIndexP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("RLOCKSHM63",
			"\tfile table free list index\t%1\n"),
			"%ld", *_rlockShm.fhdrFreeIndexP),
			stdout);
		fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("RLOCKSHM64",
			"\tlock table free list index\t%1\n"),
			"%ld", *_rlockShm.lockFreeIndexP),
			stdout);
	}
}

/*
 *	STATIC	dispFileTable() - display the file header table
 *
 *	input:	showAll - display all data?
 *
 *	proc:	display the information kept in the file header table.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

static void
dispFileTable(showAll)
bool showAll;
{	fileHdrT *entryP, *lastP;
	oldFileHdr *oldEntryP, *oldLastP;

	/*
	 * what we display depends on the style of memory being used.
	 */

	fputs(lmf_get_message("RLOCKSHM65","\nFile header table entries:\n"),
		stdout);
	if (_rlockShm.useOldStyle)
	{
		printf("%12s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM101", "open-list"));
		printf("%12s", lmf_get_message("RLOCKSHM102", "lock-list"));
		printf("%12s", lmf_get_message("RLOCKSHM103", "hash-link"));
		printf("%12s\n", lmf_get_message("RLOCKSHM104", "unique-ID"));
		oldLastP = _rlockShm.oFhdrTableP + _rlockShm.fhdrSize;
		for (oldEntryP = _rlockShm.oFhdrTableP;
		     oldEntryP < oldLastP;
		     oldEntryP++)
		{
			if (!showAll && (oldEntryP->uniqueID == 0L))
				continue;
			printf("%#12lx%#12lx%#12lx%#12lx%12ld\n",
				oldEntryP, oldEntryP->openList,
				oldEntryP->lockList, oldEntryP->hashLink,
				oldEntryP->uniqueID);
		}
	}
	else
	{
		printf("%6s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM105", "open-index"));
		printf("%12s", lmf_get_message("RLOCKSHM106", "lock-index"));
		printf("%12s", lmf_get_message("RLOCKSHM103", "hash-index"));
		printf("%12s\n", lmf_get_message("RLOCKSHM104", "unique-ID"));
		lastP = _rlockShm.fhdrTableP + _rlockShm.fhdrSize;
		for (entryP = _rlockShm.fhdrTableP;
		     entryP < lastP;
		     entryP++)
		{
			if (!showAll && !UNIQUE_ID_USED(entryP->fhUniqueID))
				continue;
			printf("%6d%12ld%12ld%12ld%12ld,%ld\n",
				(int)(entryP - _rlockShm.fhdrTableP),
				entryP->fhOpenIndex,
				entryP->fhLockIndex,
				entryP->fhHashIndex,
				entryP->fhUniqueID.fsysNumb,
				entryP->fhUniqueID.fileNumb);
		}
	}
}

/*
 *	STATIC	dispHashTable() - display the hashed file header table
 *
 *	input:	showAll - display all data?
 *
 *	proc:	display the information kept in the file header table.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

static void
dispHashTable(showAll)
bool showAll;
{	indexT *entryP, *lastP;
	oldFileHdr **oldEntryPP, **oldLastPP;

	/*
	 * what we display depends on the style of memory being used.
	 */

	printf(lmf_get_message("RLOCKSHM70",
		"\nHashed file header table entries:\n"));
	if (_rlockShm.useOldStyle)
	{
		printf("%12s", lmf_get_message("RLOCKSHM200", "entry"));
		printf("%12s\n", lmf_get_message("RLOCKSHM201", "file-hdr"));
		oldLastPP = _rlockShm.oHfhdrTablePP + _rlockShm.hfhdrSize;
		for (oldEntryPP = _rlockShm.oHfhdrTablePP;
		     oldEntryPP < oldLastPP;
		     oldEntryPP++)
		{
			if (!showAll && nil(*oldEntryPP))
				continue;
			printf("%#12lx%#12lx\n", oldEntryPP, *oldEntryPP);
		}
	}
	else
	{
		printf("%6s", lmf_get_message("RLOCKSHM200", "entry"));
		printf("%12s\n", lmf_get_message("RLOCKSHM201", "file-hdr"));
		lastP = _rlockShm.hfhdrTableP + _rlockShm.hfhdrSize;
		for (entryP = _rlockShm.hfhdrTableP;
		     entryP < lastP;
		     entryP++)
		{
			if (!showAll && (*entryP == INVALID_INDEX))
				continue;
			printf("%6d%12ld\n",
				(int)(entryP - _rlockShm.hfhdrTableP),
				*entryP);
		}
	}
}

/*
 *	STATIC	dispLockTable() - display the record lock table
 *
 *	input:	showAll - display all data?
 *
 *	proc:	display the information kept in the record lock table.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

static void
dispLockTable(showAll)
bool showAll;
{	recLockT *entryP, *lastP;
	oldRecLock *oldEntryP, *oldLastP;

	/*
	 * what we display depends on the style of memory being used.
	 */

	printf(lmf_get_message("RLOCKSHM75",
		"\nRecord locking table entries:\n"));
	if (_rlockShm.useOldStyle)
	{
		printf("%12s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM111", "next-lock"));
		printf("%12s", lmf_get_message("RLOCKSHM112", "lock-low"));
		printf("%12s", lmf_get_message("RLOCKSHM113", "lock-high"));
		printf("%12s", lmf_get_message("RLOCKSHM114", "sess-ID"));
		printf("%12s\n", lmf_get_message("RLOCKSHM115", "dos-PID"));
		oldLastP = _rlockShm.oRlockTableP + _rlockShm.lockSize;
		for (oldEntryP = _rlockShm.oRlockTableP;
		     oldEntryP < oldLastP;
		     oldEntryP++)
		{
			if (!showAll && (oldEntryP->lockHi == 0L))
				continue;
			printf("%#12lx%#12lx%#12ld%#12ld%#12d%12d\n",
				oldEntryP, oldEntryP->nextLock,
				oldEntryP->lockLow, oldEntryP->lockHi,
				oldEntryP->sessID, oldEntryP->dosPID);
		}
	}
	else
	{
		printf("%6s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM116", "next-index"));
		printf("%12s", lmf_get_message("RLOCKSHM105", "open-index"));
		printf("%12s", lmf_get_message("RLOCKSHM112", "lock-low"));
		printf("%12s", lmf_get_message("RLOCKSHM113", "lock-high"));
		printf("%12s", lmf_get_message("RLOCKSHM114", "sess-ID"));
		printf("%12s\n", lmf_get_message("RLOCKSHM115", "dos-PID"));
		lastP = _rlockShm.rlockTableP + _rlockShm.lockSize;
		for (entryP = _rlockShm.rlockTableP;
		     entryP < lastP;
		     entryP++)
		{
			if (!showAll && (entryP->rlLockHi == 0L))
				continue;
			printf("%6d%12ld%12ld%12ld%12ld%12ld%12ld\n",
				(int)(entryP - _rlockShm.rlockTableP),
				entryP->rlNextIndex,
				entryP->rlOpenIndex,
				entryP->rlLockLow,
				entryP->rlLockHi,
				entryP->rlSessID,
				entryP->rlDosPID);
		}
	}
}

/*
 *	STATIC	dispOpenTable() - display the open file table
 *
 *	input:	showAll - display all data?
 *
 *	proc:	display the information kept in the open file table.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

static void
dispOpenTable(showAll)
bool showAll;
{	openFileT *entryP, *lastP;
	oldOpenFile *oldEntryP, *oldLastP;

	/*
	 * what we display depends on the style of memory being used.
	 */

	fputs(lmf_get_message("RLOCKSHM80","\nOpen file table entries:\n"),
		stdout);
	if (_rlockShm.useOldStyle)
	{
		printf("%12s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM120", "next-open"));
		printf("%12s", lmf_get_message("RLOCKSHM121", "header"));
		printf("%12s", lmf_get_message("RLOCKSHM114", "sess-ID"));
		printf("%12s", lmf_get_message("RLOCKSHM115", "dos-PID"));
		printf("%8s",  lmf_get_message("RLOCKSHM122", "acc"));
		printf("%8s\n", lmf_get_message("RLOCKSHM123", "deny"));
		oldLastP = _rlockShm.oOpenTableP + _rlockShm.openSize;
		for (oldEntryP = _rlockShm.oOpenTableP;
		     oldEntryP < oldLastP;
		     oldEntryP++)
		{
			if (!showAll && nil(oldEntryP->header))
				continue;
			printf("%#12lx%#12lx%#12lx%12d%12d%#8x%#8x\n",
				oldEntryP,
				oldEntryP->nextOpen,
				oldEntryP->header,	
				oldEntryP->sessID,
				oldEntryP->dosPID,
				oldEntryP->accMode,
				oldEntryP->denyMode);
		}
	}
	else
	{
		printf("%6s", lmf_get_message("RLOCKSHM100", "entry"));
		printf("%12s", lmf_get_message("RLOCKSHM116", "next-index"));
		printf("%12s", lmf_get_message("RLOCKSHM130", "fhdr-index"));
		printf("%12s", lmf_get_message("RLOCKSHM114", "sess-ID"));
		printf("%12s", lmf_get_message("RLOCKSHM115", "dos-PID"));
		printf("%12s", lmf_get_message("RLOCKSHM131", "file-desc"));
		printf("%6s",  lmf_get_message("RLOCKSHM122", "acc"));
		printf("%6s\n", lmf_get_message("RLOCKSHM123", "deny"));
		lastP = _rlockShm.openTableP + _rlockShm.openSize;
		for (entryP = _rlockShm.openTableP;
		     entryP < lastP;
		     entryP++)
		{
			if (!showAll &&
			    (entryP->ofFHdrIndex == INVALID_INDEX))
				continue;
			printf("%6d%12ld%12ld%12ld%12ld%12ld%#6x%#6x\n",
				(int)(entryP - _rlockShm.openTableP),
				entryP->ofNextIndex,
				entryP->ofFHdrIndex,
				entryP->ofSessID,
				entryP->ofDosPID,
				entryP->ofFileDesc,
				entryP->ofAccMode,
				entryP->ofDenyMode);
		}
	}
}
