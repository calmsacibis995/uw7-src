#ident	"@(#)Space.c	1.3"
#ident	"$Header$"

/* 
 *	NB. This file is edited by mouseadmin(1M) at ISL and other invocations
 * 	to set the value of the tunables below. Ths parsing is very crude 
 * 	and does not understand C concepts such as comments. The only occurence 
 * 	of the variable name MUST be its declaration/definition below, otherwise
 * 	mouseadmin(1M) will probably mess up the file, and idbuild(1M) will fail.
 */

/*
 *	Tunable to select the type of un-autodetectable mouse. If the autosense 
 * 	fails then if non zero, the driver uses the Mouse Systems Corporation 
 *	protocol, otherwise it defaults to the M+ protocol.
 */

int	smse_MSC_selected = 0;

/* 
 * Overrides any autosensed protocol selection: it can 
 * be use when the autosense fails and as a workaround for protocol selection
 * errors. Setting to (1,2,3) selects MM,MSC,M+ protocols, respectively. 
 */

int	smse_force_msetype = 0;

