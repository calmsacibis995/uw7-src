#ident	"@(#)pcintf:pkg_rlock/set_cfg.c	1.2"
#include <sccs.h>

SCCSID(@(#)set_cfg.c	3.2);  /* 6/12/91 20:45:05 */

/*
   (c) Copyright 1989 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

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
 *	set_cfg.c - set configuration data
 *
 *	routines included:
 *		_rlSetCfg()
 *
 *	this file handles all modification of the configurable parameters.
 *	note that it doesn't define the external configuration data table
 *	itself, however.
 *
 *---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <string.h>
#include <ctype.h>

#include <lmf.h>

#include <rlock.h>

#include <internal.h>
#include <rlock_cfg.h>

extern void free();
extern long strtol();

#define NLS_DOMAIN "LCC.PCI.UNIX.SET_CFG"

/* Explanation: The path will be /pci/lib/en.lmf, the domain will
 * be: LCC.PCI.UNIX.SET_CFG
 */

/*
 * when decoding the configuration data, NAME_DATA_SEP is the token separator.
 */

#define NAME_DATA_SEP	'='

/*
 * this is the actual definition of the internal format of the configuration
 * data.
 *
 * WARNING: since the _rlCfgData structure is initialized here, any changes
 * to the structure must be checked, to be sure that the initialization
 * isn't invalidated.
 */

cfgDataT _rlCfgData = {
	DEF_SHM_BASE,
	MAKE_SHM_KEY(DEF_USER_KEY),
	MAKE_LS_KEY(DEF_USER_KEY),
	DEF_OPEN_TABLE,
	DEF_FILE_TABLE,
	DEF_HASH_TABLE,
	DEF_LOCK_TABLE,
	DEF_REC_LOCKS
};

/*
 * the following is the list of all valid configuration names.  note that
 * ordering is important, and must be kept in step with the definitions that
 * follow it.  the definitions are the associated indices into this table.
 */

cfgNameT _rlCfgNames[] = {
	{ "base",	"segment attach address (0 = program selected)"	},
	{ "shmkey",	"shared memory key (low word only)"		},
	{ "lsetkey",	"lockset key (low word only)"			},
	{ "keys",	"set both shmkey and lsetkey to the same value"	},
	{ "opentable",	"max number of open file table entries"		},
	{ "filetable",	"max number of file header table entries"	},
	{ "hashtable",	"max number of hashed file table entries"	},
	{ "locktable",	"max number of record lock table entries"	},
	{ "reclocks",	"max number of individual record locks"		},
	{ NIL,		NIL						}
};

#define CFG_BASE		0
#define CFG_SHM_KEY		(CFG_BASE + 1)
#define CFG_LSET_KEY		(CFG_SHM_KEY + 1)
#define CFG_KEYS		(CFG_LSET_KEY + 1)
#define CFG_OPEN_TABLE		(CFG_KEYS + 1)
#define CFG_FILE_TABLE		(CFG_OPEN_TABLE + 1)
#define CFG_HASH_TABLE		(CFG_FILE_TABLE + 1)
#define CFG_LOCK_TABLE		(CFG_HASH_TABLE + 1)
#define CFG_REC_LOCKS		(CFG_LOCK_TABLE + 1)

/*
 * internal functions.
 */

static bool setData();

/* Have we performed nls initialization of _rlCfgNames? */
static bool nls_inited = FALSE;

/*
 *	_rlSetCfg() - set a command line configuration options
 *
 *	input:	cfgDataP - pointer to the option string
 *
 *	proc:	decode the string and set the config data if its okay.  if
 *		not, report the error and return.
 *
 *	output:	(bool) - was the option valid?
 *
 */

bool
_rlSetCfg(cfgDataP)
char *cfgDataP;
{	char *nameP, *dataP, *localCfgP;
	char *strdup();
	bool setWorked;

	if ( nls_inited == FALSE ) {
	    r0 int index;
	    char nls_buf[30];
	    lmf_push_domain(NLS_DOMAIN);
	    for (index = 0; !nil(_rlCfgNames[index].cnNameP); index++) {
		sprintf(nls_buf, "SET_CFG%d", 100 + (index * 2) );
		_rlCfgNames[index].cnNameP = lmf_get_message_copy(nls_buf, 
			_rlCfgNames[index].cnNameP);

		sprintf(nls_buf, "SET_CFG%d", 101 + (index * 2) );
		_rlCfgNames[index].cnNameP = lmf_get_message_copy(nls_buf, 
			_rlCfgNames[index].cnDescP);
	    }
	    lmf_pop_domain();
	    nls_inited = TRUE;
	}
		
	/*
	 * first, get a local copy of the string, we will be playing around
	 * with it, and we will want to display the entire thing in case of
	 * an error.
	 */

	localCfgP = strdup(cfgDataP);
	if (nil(localCfgP))
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_get_message("SET_CFG1",
		"Unable to create local space for testing cfg data.\n"));
	    lmf_pop_domain();
	    return FALSE;
	}

	/*
	 * now we can pull the name and data from the local string.  the
	 * name is everything up to the NAME_DATA_SEP character, and the
	 * data is everything else.  Note that we need to replace the
	 * NAME_DATA_SEP character with an EOS, so we can use the name
	 * string properly.
	 */

	nameP = localCfgP;
	dataP = strchr(localCfgP, NAME_DATA_SEP);
	if (nil(dataP))
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0, 
		lmf_get_message("SET_CFG2",
		"Illegal configuration string: '%1'\n"), 
		"%s" , cfgDataP));
	    lmf_pop_domain();
	    free(localCfgP);
	    return FALSE;
	}
	*dataP++ = EOS;
	if (*nameP == EOS)
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0,
		lmf_get_message("SET_CFG3","No name supplied for '%1'.\n"),
		"%s" , cfgDataP));
	    lmf_pop_domain();
	    free(localCfgP);
	    return FALSE;
	}
	if (*dataP == EOS)
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0,
		lmf_get_message("SET_CFG4","No data supplied for '%1'.\n"),
		"%s" , cfgDataP));
	    lmf_pop_domain();
	    free(localCfgP);
	    return FALSE;
	}

	/*
	 * okay, we have data that is syntactically valid.  try to set it.
	 */

	setWorked = setData(nameP, dataP);
	free(localCfgP);
	return setWorked;
}

/*
 *	STATIC	setData() - try to set the data
 *
 *	input:	nameP - the data's name
 *		dataP - its value
 *
 *	proc:	if the name and data are legal, set them, otherwise return
 *		an appropriate error condition.  there are other ways to
 *		do this, notably, with a structure and loop.  however, for
 *		what we have to do, the extra complexity is not worth the
 *		memory savings.  there definitely wouldn't be a time
 *		savings by using the loop.
 *
 *	output:	(bool) - did it work?
 *
 *	global:	_rlCfgData - set
 */

static bool
setData(nameP, dataP)
r0 char *nameP;
char *dataP;
{	r0 int index;
	ulong numericData, maxLegalValue;
	char *endDataP;

	/*
	 * compare the name against each one we know, and find the index
	 * corresponding to the name.
	 */

	for (index = 0; !nil(_rlCfgNames[index].cnNameP); index++)
	{
	    if (EQUAL(_rlCfgNames[index].cnNameP, nameP))
		break;
	}
	if (nil(_rlCfgNames[index].cnNameP))
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0, 
		lmf_get_message("SET_CFG5","Unknown name: '%1'\n"),
		"%s" , nameP));
	    lmf_pop_domain();
	    return FALSE;
	}

	/*
	 * currently, all configurable data is unsigned numeric, so we can use
	 * the same decoding for each.  note that since we don't have a proper
	 * conversion routine for unsigned characters, we have to manually
	 * verify that there are no 'negative' signs in the input, then
	 * convert the output of strtol(3c) appropriately.  unfortunately, in
	 * order to check for a leading '-', we must go through gyrations to
	 * pass over any 'white space'.  the alternative (strchr(3c)) may find
	 * a '-' in the middle of a number, which isn't a negative number, it is
	 * just illegal.
	 *
	 * NOTE: we actually need to test for the '-' in the input string,
	 * since some values are effectively negative, and a simple check of 
	 * the cast value won't work properly.  for example, a base address
	 * of 0xc0000000 will show up as a negative number, even though it may
	 * be perfectly valid.
	 */

	while (isspace(*dataP)) dataP++;
	if (*dataP == '-')
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0, 
		lmf_get_message("SET_CFG6",
		"The data for '%1' must be non-negative.\n"),
		"%s" , nameP));
	    lmf_pop_domain();
	    return FALSE;
	}
	numericData = (ulong)strtol(dataP, &endDataP, 0);
	if (*endDataP != EOS)
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0,
		lmf_get_message("SET_CFG7","The data for '%1' is invalid.\n"),
		"%s" , nameP));
	    lmf_pop_domain();
	    return FALSE;
	}

	/*
	 * not only that, but all of them must fit into an (indexT) type,
	 * except for CFG_BASE, which is a (char *), and the keys and number
	 * of individual record locks, which must be (ushort) types.
	 */

	if (index == CFG_BASE)
	    maxLegalValue = (ulong)MAX_POINTER;
	else if ((index == CFG_SHM_KEY) ||
		 (index == CFG_LSET_KEY) ||
		 (index == CFG_KEYS) ||
		 (index == CFG_REC_LOCKS))
	{
	    maxLegalValue = MAX_USHORT;
	}
	else
	    maxLegalValue = MAX_INDEX;
	if (numericData > maxLegalValue)
	{
	    lmf_push_domain(NLS_DOMAIN);
	    puts(lmf_format_string((char *)0, 0, 
		lmf_get_message("SET_CFG8",
		"The data for '%1' must be less than %2.\n"),
		"%s%d", nameP, maxLegalValue));
	    lmf_pop_domain();
	    return FALSE;
	}

	/*	
	 * we have a valid number.  insert it into the proper table entry,
	 * and we are done.
	 */

	switch (index)
	{
	    case CFG_BASE:
		_rlCfgData.cfgBaseP = (char FAR *)numericData;
		break;
	    case CFG_SHM_KEY:
		_rlCfgData.cfgShmKey = MAKE_SHM_KEY((ushort)numericData);
		break;
	    case CFG_LSET_KEY:
		_rlCfgData.cfgLsetKey = MAKE_LS_KEY((ushort)numericData);
		break;
	    case CFG_KEYS:
		_rlCfgData.cfgShmKey = MAKE_SHM_KEY((ushort)numericData);
		_rlCfgData.cfgLsetKey = MAKE_LS_KEY((ushort)numericData);
		break;
	    case CFG_REC_LOCKS:
		_rlCfgData.cfgRecLocks = (indexT)numericData;
		break;
	    case CFG_OPEN_TABLE:
		_rlCfgData.cfgOpenTable = (indexT)numericData;
		break;
	    case CFG_FILE_TABLE:
		_rlCfgData.cfgFileTable = (indexT)numericData;
		break;
	    case CFG_HASH_TABLE:
		_rlCfgData.cfgHashTable = (indexT)numericData;
		break;
	    case CFG_LOCK_TABLE:
		_rlCfgData.cfgLockTable = (indexT)numericData;
	}
	return TRUE;
}
