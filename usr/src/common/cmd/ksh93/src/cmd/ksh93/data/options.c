#ident	"@(#)ksh93:src/cmd/ksh93/data/options.c	1.1"
#pragma prototyped

#include	<shell.h>
#include	"FEATURE/options"
#include	"name.h"
#include	"shtable.h"

/*
 * This is the list of invocation and set options
 * This list must be in in ascii sorted order
 */

#define bit32(x)	((x)&0xffff?bit16(x):16+bit16((x)>>16))
#define bit16(x)	((x)&0xff?bit8(x):8+bit8((x)>>8))
#define bit8(x)		((x)&0xf?bit4(x):4+bit4((x)>>4))
#define bit4(x)		((x)&0x3?bit2(x):2+bit2((x)>>2))
#define bit2(x)		((x)&1?0:1)

const Shtable_t shtab_options[] =
{
	"allexport",		bit32(SH_ALLEXPORT),
	"bgnice",		bit32(SH_BGNICE),
	"emacs",		bit32(SH_EMACS),
	"errexit",		bit32(SH_ERREXIT),
	"gmacs",		bit32(SH_GMACS),
	"ignoreeof",		bit32(SH_IGNOREEOF),
	"interactive",		bit32(SH_INTERACTIVE),
	"keyword",		bit32(SH_KEYWORD),
	"markdirs",		bit32(SH_MARKDIRS),
	"monitor",		bit32(SH_MONITOR),
	"noexec",		bit32(SH_NOEXEC),
	"noclobber",		bit32(SH_NOCLOBBER),
	"noglob",		bit32(SH_NOGLOB),
	"nolog",		bit32(SH_NOLOG),
	"notify",		bit32(SH_NOTIFY),
	"nounset",		bit32(SH_NOUNSET),
	"privileged",		bit32(SH_PRIVILEGED),
	"restricted",		bit32(SH_RESTRICTED),
	"trackall",		bit32(SH_TRACKALL),
	"verbose",		bit32(SH_VERBOSE),
	"vi",			bit32(SH_VI),
	"viraw",		bit32(SH_VIRAW),
	"xtrace",		bit32(SH_XTRACE),
	"",			0
};

const Shtable_t shtab_attributes[] =
{
	{"-nnameref",	NV_REF},
	{"-xexport",	NV_EXPORT},
	{"-rreadonly",	NV_RDONLY},
	{"-ttagged",	NV_TAGGED},
	{"-Eexponential",(NV_INTEGER|NV_DOUBLE|NV_EXPNOTE)},
	{"-Ffloat",	(NV_INTEGER|NV_DOUBLE)},
	{"++short",	(NV_INTEGER|NV_SHORT)},
	{"++unsigned",	(NV_INTEGER|NV_UNSIGN)},
	{"-iinteger",	NV_INTEGER},
	{"-Hfilename",	NV_HOST},
	{"-llowercase",	NV_UTOL},
	{"-Zzerofill",	NV_ZFILL},
	{"-Lleftjust",	NV_LJUST},
	{"-Rrightjust",	NV_RJUST},
	{"-uuppercase",	NV_LTOU},
	{"-Aarray",	NV_ARRAY},
	{"",		0}
};
