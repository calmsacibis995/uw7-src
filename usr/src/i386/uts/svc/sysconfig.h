#ifndef _SVC_SYSCONFIG_H	/* wrapper symbol for kernel use */
#define _SVC_SYSCONFIG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/sysconfig.h	1.5.5.1"
#ident	"$Header$"

/*
 * cmd values for _sysconfig system call. 
 * Additions to this list should be reflected in truss(1).
 *
 * WARNING: This is an undocumented system call,
 * therefore future compatibility can not be guaranteed. 
 */ 

#define UNUSED			1
#define _CONFIG_NGROUPS		2	/* # configured supplemental groups */
#define _CONFIG_CHILD_MAX	3	/* max # of processes per uid session */
#define _CONFIG_OPEN_FILES	4	/* max # of open files per process */
#define _CONFIG_POSIX_VER	5	/* POSIX version */
#define _CONFIG_PAGESIZE	6	/* system page size */
#define _CONFIG_CLK_TCK		7	/* ticks per second */
#define _CONFIG_XOPEN_VER	8	/* XOPEN version */
#define _CONFIG_NACLS_MAX	9	/* for Enhanced Security */
#define	_CONFIG_ARG_MAX		10	/* max length of exec args */
#define _CONFIG_NPROC		11	/* # processes system is config for */
#define _CONFIG_NENGINE		12	/* # configured processors (CPUs) */
#define _CONFIG_NENGINE_ONLN	13	/* # online processors (CPUs) */
#define _CONFIG_TOTAL_MEMORY	14	/* total memory */
#define _CONFIG_USEABLE_MEMORY	15	/* user + system memory */
#define _CONFIG_GENERAL_MEMORY	16	/* user only memory */
#define _CONFIG_DEDICATED_MEMORY 17 	/* dedicated memory */
#define _CONFIG_NCGS_CONF	18	/* # CGs in system */
#define _CONFIG_NCGS_ONLN	19	/* # CGs online now */
#define _CONFIG_MAX_ENG_PER_CG	20	/* max engines per CG */
#define _CONFIG_CACHE_LINE	21	/* memory cache line size */
#define _CONFIG_SYSTEM_ID	22	/* system id assigned at ISL */

#endif /*_SVC_SYSCONFIG_H*/
