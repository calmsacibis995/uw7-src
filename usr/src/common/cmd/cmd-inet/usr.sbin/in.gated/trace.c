#ident	"@(#)trace.c	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#define	INCLUDE_TIME
#define	INCLUDE_FILE
#define	INCLUDE_STAT

#include "include.h"
#include "krt.h"
#include "parse.h"

trace *trace_global;						/* Global options */
static trace_file trace_files = { &trace_files, &trace_files };	/* List of files */
int trace_nosyslog = TRACE_LOG_NORMAL;

#define	TRACE_FILES(tfp) \
    for (tfp = trace_files.trf_forw; tfp != &trace_files; tfp = tfp->trf_forw) {
#define	TRACE_FILES_END(tfp) }

char trace_buffer[BUFSIZ];
char *trace_ptr = trace_buffer;

static block_t trace_trace_index;
static block_t trace_file_index;

static const bits trace_types[] = {
    { TR_ALL,			"all" },
    { TR_NORMAL|TR_ROUTE,	"general" },
    { TR_STATE,			"state" },
    { TR_NORMAL,		"normal" },
    { TR_POLICY,		"policy" },
    { TR_TASK,			"task" },
    { TR_TIMER,			"timer" },
    { TR_ROUTE,			"route" },
    { TR_DETAIL,		"detail" },
    { TR_DETAIL_RECV,		"recv-detail" },
    { TR_DETAIL_SEND,		"send-detail" },
    { TR_PACKET,		"packet" },
    { TR_PACKET_RECV,		"recv-packet" },
    { TR_PACKET_SEND,		"send-packet" },
    { 0,			NULL }
};

static const bits trace_global_types[] = {
    { TR_PARSE,			"parse" },
    { TR_ADV,			"adv" },
    { TR_KRT_SYMBOLS,		"symbols" },
    { TR_KRT_IFLIST,		"iflist" },
    { 0,			NULL }
};

static const bits trace_control_types[] = {
    { TRC_NOSTAMP,		"nostamp" },
    { TRC_LOGONLY,		"logonly" },
    { TRC_NL_BEFORE,		"nl_before" },
    { TRC_NL_AFTER,		"nl_after" },
    { 0,			NULL }
} ;

/*  */

#ifdef	SYSLOG_DEBUG

static const bits syslog_pri_bits[] = {
    { LOG_MASK(LOG_EMERG),	"EMERG" },
    { LOG_MASK(LOG_ALERT),	"ALERT" },
    { LOG_MASK(LOG_CRIT),	"CRIT" },
    { LOG_MASK(LOG_ERR),	"ERR" },
    { LOG_MASK(LOG_WARNING),	"WARNING" },
    { LOG_MASK(LOG_NOTICE),	"NOTICE" },
    { LOG_MASK(LOG_INFO),	"INFO" },
    { LOG_MASK(LOG_DEBUG),	"DEBUG" },
    { 0,		NULL }
    } ;

static const bits syslog_facility_bits[] = {
    { LOG_KERN,		"KERN" },
    { LOG_USER,		"USER" },
    { LOG_MAIL,		"MAIL" },
    { LOG_DAEMON,	"DAEMON" },
    { LOG_AUTH,		"AUTH" },
    { LOG_SYSLOG,	"SYSLOG" },
    { LOG_LPR,		"LPR" },
    { LOG_NEWS,		"NEWS" },
    { LOG_UUCP,		"UUCP" },
    { LOG_CRON,		"CRON" },
    { LOG_LOCAL0,	"LOCAL0" },
    { LOG_LOCAL1,	"LOCAL1" },
    { LOG_LOCAL2,	"LOCAL2" },
    { LOG_LOCAL3,	"LOCAL3" },
    { LOG_LOCAL4,	"LOCAL4" },
    { LOG_LOCAL5,	"LOCAL5" },
    { LOG_LOCAL6,	"LOCAL6" },
    { LOG_LOCAL7,	"LOCAL7" },
    { 0,		NULL }
} ;

static const bits syslog_bits[] = {
    { LOG_PID,		"PID" },
    { LOG_CONS,		"CONS" },
    { LOG_ODELAY,	"ODELAY" },
    { LOG_NDELAY,	"NDELAY" },
    { LOG_NOWAIT,	"NOWAIT" },
    { 0, NULL }
} ;

static char * syslog_name = (char *) 0;
static flag_t syslog_options = (flag_t) 0;
static flag_t syslog_mask = LOG_UPTO(LOG_DEBUG);

void
openlog __PF3(name, const char *,
	      options, flag_t,
	      facility, flag_t)
{
    if (syslog_name) {
	task_mem_free((task *) 0, syslog_name);
    }
    syslog_name = task_mem_strdup((task *) 0, name);
    syslog_options = options;

    (void) fprintf(stderr,
		   "openlog: name %s options %s facility LOG_%s\n",
		   syslog_name,
		   trace_bits(syslog_bits, options),
		   trace_bits(syslog_facility_bits, facility));
}

void
setlogmask __PF1(mask, flag_t)
{

    (void) fprintf(stderr,
		   "setlogmask: old %s new %s\n",
		   trace_bits(syslog_pri_bits, syslog_mask),
		   trace_bits(syslog_pri_bits, mask));
		   
    syslog_mask = mask;
}


#ifdef	STDARG
void
syslog(int pri, const char *msg, ...)
#else	/* STDARG */
void
syslog(va_alist)
va_dcl
#endif	/* STDARG */
{
    va_list ap;
    flag_t pri_mask;
    char buf[BUFSIZ];
#ifdef	STDARG
    va_start(ap, msg);
#else	/* STDARG */
    const char *msg;
    int pri;

    va_start(ap);

    pri = va_arg(ap, int);
    msg = va_arg(ap, const char *);
#endif	/* STDARG */

    pri_mask = LOG_MASK(pri);
    
    (void) vsprintf(buf, msg, ap);
    (void) fprintf(stderr,
		   "SYSLOG\nSYSLOG LOG_%s %s%s",
		   trace_bits(syslog_pri_bits, pri_mask),
		   BIT_TEST(syslog_mask, pri_mask) ? "" : "SKIP ",
		   syslog_name);
    if (BIT_TEST(syslog_options, LOG_PID)) {
	(void) fprintf(stderr,
		       "[%d]",
		       task_pid);
    }
    (void) fprintf(stderr,
		   ": %s\nSYSLOG\n",
		   buf);
}
#endif	/* SYSLOG_DEBUG */


void
trace_syslog __PF1(pri, int)
{
    switch (trace_nosyslog) {
    case TRACE_LOG_TRACE:
	if (pri < LOG_NOTICE) {
	    (void) fwrite((char *) trace_buffer, sizeof (char), (size_t) (trace_ptr - trace_buffer), stderr);
	    putc('\n', stderr);
#ifdef	MUST_FFLUSH
	    (void) fflush(stderr);
#endif
	}
	break;

    case TRACE_LOG_NONE:
	break;
    }
}


/**/

/*
 *	Convert tracing flags to printable
 */
const char *
trace_string __PF2(tr_flags, flag_t,
		   types, const bits *)
{

    switch (tr_flags) {
    case TR_ALL:
	return " all";

    case 0:
	return " none";

    default:
	return trace_bits2(types, trace_types, tr_flags);
    }
}


/*
 *	Display trace options enabled
 */
void
trace_display __PF2(trp, trace *,
		    level, flag_t)
{
    trace_tf(trp,
	     level,
	     TRC_NL_BEFORE|TRC_NL_AFTER,
	     ("Tracing flags enabled:%s%s %s",
	      trp->tr_control ? " " : "",
	      trace_bits(trace_control_types, trp->tr_control),
	      trace_string(trp->tr_flags, trp->tr_names)));
}

/**/


/*
 * Close trace file
 */
static void
trace_close __PF1(tfp, trace_file *)
{
    if (tfp->trf_FILE) {
	if (tfp->trf_FILE != stderr) {
	    (void) fclose(tfp->trf_FILE);
	}
	tfp->trf_FILE = NULL;
    }
    return;
}


void
trace_file_free __PF1(tfp, trace_file *)
{
    assert(!tfp || tfp->trf_refcount);
    if (tfp && !--tfp->trf_refcount) {
#ifdef	DEBUG
	syslog(LOG_CRIT, "trace_file_free: freeing %s",
	       tfp->trf_file ? tfp->trf_file : "stderr");
#endif	/* DEBUG */
	if (tfp->trf_FILE) {
	    trace_close(tfp);
	}
	if (tfp->trf_file) {
	    task_mem_free((task *) 0, tfp->trf_file);
	}
	REMQUE(tfp);
	task_block_free(trace_file_index, (void_t) tfp);
    }
}


trace *
trace_create __PF0(void)
{
    trace *trp = (trace *) task_block_alloc(trace_trace_index);

    return trace_alloc(trp);
}


trace *
trace_alloc __PF1(trp, trace *)
{
    if (trp) {
	trp->tr_refcount++;
    }

    return trp;
}


trace *
trace_free __PF1(trp, trace *)
{
    assert(trp->tr_refcount);
    if (!--trp->tr_refcount) {
	if (trp->tr_file) {
	    trace_file_free(trp->tr_file);
	}
	task_block_free(trace_trace_index, (void_t) trp);
    }

    return (trace *) 0;
}

/**/
  
/*
 * Turn off tracing.
 */
void
trace_off __PF1(tfp, trace_file *)
{
    if (tfp->trf_FILE) {
	trace_log_tf(trace_global,
		     TRC_NL_BEFORE|TRC_NL_AFTER,
		     LOG_INFO,
		     ("Tracing to \"%s\" suspended",
		      tfp->trf_file ? tfp->trf_file : "(stderr)"));
	if (tfp->trf_FILE != stderr) {
	    (void) fclose(tfp->trf_FILE);
	}
	tfp->trf_FILE = NULL;
    }
    return;
}


void
trace_close_all __PF0(void)
{
    register trace_file *tfp;

    TRACE_FILES(tfp) {
	trace_close(tfp);
    } TRACE_FILES_END(tfp) ;
}

/*
 * Locate a given trace file descriptor
 */
trace_file *
trace_file_locate __PF4(file, char *,
			limit_size, off_t,
			limit_files, u_int,
			flags, flag_t)
{
    int check_limits = 0;
    register trace_file *tfp;

#ifndef	FLAT_FS
    if (file && *file != '/') {
	/* Make trace file name absolute */
	char *fn = task_mem_malloc((task *) 0,
				   (size_t) (strlen(file) + strlen(task_path_start) + 2));

	(void) strcpy(fn, task_path_start);
	(void) strcat(fn, "/");
	(void) strcat(fn, file);

	file = fn;
    }
#endif	/* FLAT_FS */

    /* Locate an already created trace_file blocks */
    if (file) {
	/* Search the list of existing blocks */
	
	TRACE_FILES(tfp) {
	    if (tfp->trf_file
		&& !strcmp(file, tfp->trf_file)) {
		/* Found it! */

		if (tfp->trf_file && tfp->trf_refcount) {
		    check_limits++;
		}
		break;
	    }
	} TRACE_FILES_END(tfp) ;
    } else {
	/* Inherit the global file if it exists */

	if (trace_global && trace_global->tr_file) {
	    /* Inherit global file */

	    tfp = trace_global->tr_file;
	} else {
	    /* No global tracing to inherit */

	    return (trace_file *) 0;
	}
    }

    if (tfp == &trace_files) {
	/* Not found, create and insert on list */

	INSQUE(tfp = (trace_file *) task_block_alloc(trace_file_index),
	       trace_files.trf_back);
	tfp->trf_file = file;
	tfp->trf_limit_size = limit_size;
	tfp->trf_limit_files = limit_files;
	tfp->trf_flags = flags;
    }

    if (check_limits) {
	/* If tracing to a real file */
	
	/* Verify that limits match */
	if (tfp->trf_limit_size != limit_size) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_WARNING,
			 ("trace_file_locate: replacing %s file size limit %u with %u",
			  tfp->trf_file ? tfp->trf_file : "stderr",
			  tfp->trf_limit_size,
			  limit_size));
	    tfp->trf_limit_size = limit_size;
	}
	if (tfp->trf_limit_files != limit_files) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_WARNING,
			 ("trace_file_locate: replacing %s file count limit %u with %u",
			  tfp->trf_file ? tfp->trf_file : "stderr",
			  tfp->trf_limit_files,
			  limit_files));
	    tfp->trf_limit_files = limit_files;
	}
	if ((flags ^ tfp->trf_flags) & TRF_REPLACE) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_WARNING,
			 ("trace_file_locate: changing %s to %s",
			  tfp->trf_file ? tfp->trf_file : "stderr",
			  BIT_TEST(flags, TRF_REPLACE) ? "replace" : "append"));
	    tfp->trf_flags = (tfp->trf_flags & ~TRF_REPLACE) | (flags & TRF_REPLACE);
	}
    }
	
    return trace_file_alloc(tfp);
}


/*
 * Turn on tracing.
 */
void
trace_on __PF1(tfp, trace_file *)
{

    if (!tfp->trf_FILE) {
	if (!tfp->trf_file) {
	    /* No file specified, use stderr */

	    tfp->trf_FILE = stderr;
	} else {
	    int fd;
	    int append = BIT_TEST(tfp->trf_flags, TRF_REPLACE) == 0;
#ifndef	NO_STAT
	    struct stat stbuf;
#endif	/* NO_STAT */

	    if (BIT_TEST(task_state, TASKS_RECONFIG)) {
		/* Don't replace if reconfiguring to the same file */
		append = TRUE;
	    }
	
#ifndef	NO_STAT
	    if (stat(tfp->trf_file, &stbuf) < 0) {
		switch (errno) {
		case ENOENT:
		    tfp->trf_size = 0;
		    break;

		default:
		    syslog(LOG_ERR, "trace_on: stat(%s): %m",
			   tfp->trf_file);
		    return;
		}
	    } else {
		/* Indicate current file size.  Will be reset later if not appending */
		tfp->trf_size = stbuf.st_size;

		switch (stbuf.st_mode & S_IFMT) {
		default:
		    /* This may be a pipe, where we won't be able to seek */
		    append = FALSE;
		    if (tfp->trf_limit_size) {
			/* Don't want to try to rename a pipe! */

			tfp->trf_limit_size = 0;
			trace_log_tf(trace_global,
				     0,
				     LOG_WARNING,
				     ("trace_on: \"%s\" is not a regular file, ignoring size limit",
				      tfp->trf_file));
		    }
		    /* Fall through */

		case S_IFREG:
		    break;

		case S_IFDIR:
		case S_IFBLK:
		case S_IFLNK:
		    trace_log_tf(trace_global,
				 0,
				 LOG_ERR,
				 ("trace_on: \"%s\" is not a regular file",
				  tfp->trf_file));
		    return;
		}
	    }
#endif	/* NO_STAT */

	    /* First try to open the file */
	    fd = open(tfp->trf_file, O_RDWR | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
	    if (fd < 0) {
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("Could not open \"%s\": %m",
			      tfp->trf_file));
		return;
	    }
	    /* Then try to lock the file */
	    /* Lock the file to insure only one gated writes to it at a time */
	    if (flock(fd, LOCK_EX|LOCK_NB) < 0) {
		int error = errno;

		switch (error) {
		case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
		case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
		    trace_log_tf(trace_global,
				 0,
				 LOG_ERR,
				 ("trace_on: trace file \"%s\" appears to be in use",
				  tfp->trf_file));
		    break;

		default:
		    trace_log_tf(trace_global,
				 0,
				 LOG_ERR,
				 ("trace_on: Could not obtain lock on \"%s\": %m",
				  tfp->trf_file));
		    break;
		}
		(void) close(fd);
		return;
	    }
	    if (!append) {
		/* Indicate the file is empty */
		tfp->trf_size = 0;
	    }

	    /* Now close the old one */
	    if (tfp->trf_FILE) {
		trace_close(tfp);
	    }

	    /* And finally open the stream */
	    tfp->trf_FILE = fdopen(fd, append ? "a" : "w");
	    if (!tfp->trf_FILE) {
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_on: can not open \"%s\" for writing: %m",
			      tfp->trf_file));
		return;
	    }
	}

#ifndef vax11c
	setvbuf(tfp->trf_FILE, NULL, _IOLBF, 0);
#endif	/* vax11c */

	if (tfp->trf_file) {
	    syslog(LOG_INFO,
		   "trace_on: tracing to \"%s\" started",
		   tfp->trf_file);

	    (void) fprintf(tfp->trf_FILE,
			   "%s%strace_on: Tracing to \"%s\" started\n",
			   time_string,
			   *time_string ? " " : "",
			   tfp->trf_file);
	}
    }

    /* Reset replace indication */
    BIT_RESET(tfp->trf_flags, TRF_REPLACE);
}


/*
 * Toggle tracing
 */
void
trace_toggle __PF0(void)
{
    int turn_off = 0;
    register trace_file *tfp;

    /* First figure out if tracing is on or off */
    TRACE_FILES(tfp) {
	if (tfp->trf_FILE) {
	    turn_off++;
	    break;
	}
    } TRACE_FILES_END(tfp) ;
    
    if (turn_off) {
	/* Tracing is on, turn it off */

	/* Then any others */
	TRACE_FILES(tfp) {
	    if (tfp->trf_FILE != stderr) {
		trace_off(tfp);
	    }
	} TRACE_FILES_END(tfp) ;
    } else {
	/* Tracing is off, try to turn it on */

	/* Then any others */
	TRACE_FILES(tfp) {
	    if (tfp->trf_FILE != stderr) {
		trace_on(tfp);
	    }
	} TRACE_FILES_END(tfp) ;
    }
}


trace *
trace_set_global __PF2(types, const bits *,
		       inherit, flag_t)
{
    trace *trp = (trace *) 0;
    
    if (trace_global && trace_global->tr_flags) {
	trp = trace_create();
	trp->tr_names = types;
	trp->tr_flags = trace_global->tr_flags & (inherit|TR_INHERIT);
	trp->tr_control = trace_global->tr_control & TRC_INHERIT;
	trp->tr_file = trace_file_alloc(trace_global->tr_file);
    }

    return trp;
}

/**/
/*
 *  Rotate the trace files
 */
static void
trace_rotate __PF1(tfp, trace_file *)
{
    int i = tfp->trf_limit_files - 2;
    char file1[MAXPATHLEN];
    char file2[MAXPATHLEN];
    char *cp = file1;

    (void) fputs("trace_rotate: Rotating trace files\n",
		 tfp->trf_FILE);

    syslog(LOG_INFO, "trace_rotate: rotating %s",
	   tfp->trf_file);

    /* Close this one */
    trace_close(tfp);

    while (i >= 0) {
	(void) sprintf(file2,
		       "%s.%d",
		       tfp->trf_file,
		       i--);
	if (i < 0) {
	    cp = tfp->trf_file;
	} else {
	    (void) sprintf(file1,
			   "%s.%d",
			   tfp->trf_file,
			   i);
	}

	if (rename(cp, file2) < 0) {
	    switch (errno) {
	    case ENOENT:
		break;

	    default:
		syslog(LOG_ERR, "trace_rotate: rename(%s, %s): %m",
		       file1,
		       file2);
	    }
	}
    }

    /* And open new file */
    trace_on(tfp);
}


/*
 *  Parse trace flags specified on the command line
 */
flag_t
trace_args __PF1(flags, char *)
{
    int i = 1;
    flag_t tr_flags = 0;
    register char *cp = flags;

    while ((cp = (char *) index(cp, ','))) {
	    i++;
	    *cp++ = (char) 0;
    }

    for (cp = flags; i--; cp += strlen(cp) + 1) {
	if (!strcasecmp(cp, "none")) {
	    tr_flags = (flag_t) 0;
	} else {
	    static const bits *types[] = {
		trace_types,
		trace_global_types,
		(bits *) 0
	    } ;
	    const bits *p = (bits *) 0, **pp;

	    for (pp = types; *pp; pp++) {

		for (p = *pp; p->t_bits; p++) {
		    if (!strcasecmp(cp, p->t_name)) {
		        BIT_SET(tr_flags, p->t_bits);
		        break;
		    }
		}

		if (p->t_bits) {
		    break;
		}
	    }
		
	    if (!p->t_bits) {
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("%s: unknown trace flag: %s",
			      task_progname,
			      cp));
	    }
	}
    }

    return tr_flags;
}


#define	BITS_N_STRINGS	4
#define	BITS_NEW_INDEX	(bits_index = ((bits_index + 1) % BITS_N_STRINGS))
#define	BITS_STRING	bits_strings[bits_index]
static int bits_index;
static char bits_strings[BITS_N_STRINGS][LINE_MAX];

/*
 *	Return pointer to static string with current trace flags.
 */
char *
trace_bits __PF2(bp, const bits *,
		 mask, flag_t)
{
    char *s = BITS_STRING;
    register char *dp = s ;
    const bits *p;
    flag_t seen = 0;

    *dp = (char) 0;

    for (p = bp; p->t_bits; p++) {
	if (BIT_MATCH(mask, p->t_bits) && !BIT_MATCH(seen, p->t_bits)) {
	    register const char *sp = p->t_name;

	    BIT_SET(seen, p->t_bits);
	    
	    if (dp > s) {
		*dp++ = ' ';
	    }
			
	    while (*sp) {
		*dp++ = *sp++;
	    }
	}
    }

    *dp = (char) 0;

    BITS_NEW_INDEX;

    return s;
}


/*
 *	Return pointer to static string with current trace flags.
 */
char *
trace_bits2 __PF3(bp1, const bits *,
		  bp2, const bits *,
		  mask, flag_t)
{
    char *s = BITS_STRING;
    register char *dp = s;
    const bits *p, *bp = bp1;
    flag_t seen = 0;

    *dp = (char) 0;

    do {
	for (p = bp; p && p->t_bits; p++) {
	    if (BIT_MATCH(mask, p->t_bits) && !BIT_MATCH(seen, p->t_bits)) {
		register const char *sp = p->t_name;

		BIT_SET(seen, p->t_bits);
	    
		if (dp > s) {
		    *dp++ = ' ';
		}
			
		while (*sp) {
		    *dp++ = *sp++;
		}
	    }
	}
    } while (bp != bp2 && (bp = bp2)) ;

    *dp = (char) 0;

    BITS_NEW_INDEX;
	
    return s;
}


const char *
trace_value __PF2(bp, const bits *,
		  value, int)
{
    const bits *p;

    for (p = bp; p->t_name; p++) {
	if (p->t_bits == (u_int) value) {
	    return p->t_name;
	}
    }

    return (char *) 0;
}


#ifndef	trace_state
char *
trace_state __PF2(bp, bits *,
		  mask, flag_t)
{

    return bp[mask].t_name;
}

#endif	/* trace_state */

/*
 *  Dump tracing state for a task
 */
void
trace_task_dump __PF2(fd, FILE *,
		      trp, trace *)
{
    (void) fprintf(fd, "\t\tTrace options:%s%s %s\n",
		   trp->tr_control ? " " : "",
		   trace_bits(trace_control_types, trp->tr_control),
		   trace_string(trp->tr_flags, trp->tr_names));
    if (trp->tr_file) {
	(void) fprintf(fd, "\t\tTrace file: %s",
		       trp->tr_file->trf_file
		       ? trp->tr_file->trf_file
		       : "(stderr)");
	if (trp->tr_file->trf_limit_size) {
	    (void) fprintf(fd, "\tsize %u\tfiles %u\n",
			   trp->tr_file->trf_limit_size,
			   trp->tr_file->trf_limit_files);
	} else {
	    (void) fprintf(fd, "\n");
	}
    }
}

/*
 *  Dump everything
 */
static void
trace_do_dump __PF1(tp, task *)
{
    int tries = 10;
    FILE *fp = NULL;
    char path_dump[MAXPATHLEN];

    (void) sprintf(path_dump, _PATH_DUMP,
		   task_progname);

    while (fp == NULL && tries--) {
	int fd;
	int can_seek = TRUE;
#ifndef	NO_STAT
	struct stat stbuf;

	if (stat(path_dump, &stbuf) < 0) {
	    switch (errno) {
	    case ENOENT:
		break;

	    default:
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_do_dump: stat(%s): %m",
			      path_dump));
		return;
	    }
	} else {
	    switch (stbuf.st_mode & S_IFMT) {
	    default:
		/* Might be a FIFO (pipe) Indicate we can't seek */
		can_seek = FALSE;
		/* Fall through */

	    case S_IFREG:
		break;

	    case S_IFDIR:
	    case S_IFBLK:
	    case S_IFLNK:
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_do_dump: \"%s\" is not a regular file",
			      path_dump));
		return;
	    }
	}
#endif	/* NO_STAT */

	/* First try to open the file */
	fd = open(path_dump, O_RDWR | O_CREAT | (can_seek ? O_APPEND : 0), 0644);
	if (fd < 0) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_ERR,
			 ("Could not open \"%s\": %m",
			  path_dump));
	    return;
	}
	/* Then try to lock the file */
	/* Lock the file to insure only one gated writes to it at a time */
	if (flock(fd, LOCK_EX|LOCK_NB) < 0) {
	    int error = errno;

	    switch (error) {
	    case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
	    case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
#ifdef	NO_FORK
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_do_dump: dump file \"%s\" in use",
			      path_dump));
		break;
#else	/* NO_FORK */
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_do_dump: dump file \"%s\" in use, waiting...",
			      path_dump));
		(void) close(fd);
		sleep(15);
		continue;
#endif	/* NO_FORK */

	    default:
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("trace_do_dump: Could not obtain lock on \"%s\": %m",
			      path_dump));
		break;
	    }
	    (void) close(fd);
	    return;
	}

	/* And finally open the stream */
	fp = fdopen(fd, can_seek ? "a" : "w");
	if (!fp) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_ERR,
			 ("trace_do_dump: can not open \"%s\" for writing: %m",
			  path_dump));
	    return;
	}
    }
    
#ifndef vax11c
    setvbuf(fp, NULL, _IOLBF, 0);
#endif	/* vax11c */
    trace_log_tf(trace_global,
		 0,
		 LOG_INFO,
		 ("trace_do_dump: processing dump to %s",
		  path_dump));
    (void) fprintf(fp, "\f\n\t%s[%d] version %s memory dump on %s at %s\n",
		   task_progname,
		   task_mpid,
		   gated_version,
		   task_hostname,
		   time_full);

    if (krt_version_kernel) {
	(void) fprintf(fp, "\t\t%s\n\n", krt_version_kernel);
    }

    (void) fprintf(fp, "\t\tStarted at %s\n",
		   task_time_start.gt_ctime);

    /* Dump all the trace files */
    {
	register trace_file *tfp;

	(void) fprintf(fp, "Tracing:\n\tGlobal:\t%s\n\tFiles:\n",
		       trace_string(trace_global ? trace_global->tr_flags : (flag_t) 0, (bits *) 0));

	TRACE_FILES(tfp) {
	    (void) fprintf(fp, "\t\t%s:\n\t\t\t%s  refcount %u  size %u  limit %u  files %u\n",
			   tfp->trf_file ? tfp->trf_file : "(stderr)",
			   tfp->trf_FILE ? "opened" : "closed",
			   tfp->trf_refcount,
			   tfp->trf_size,
			   tfp->trf_limit_size,
			   tfp->trf_limit_files);
	} TRACE_FILES_END(tfp) ;

	(void) fprintf(fp, "\n");
    }
    
    /* Task_dump dumps all protocols */
    task_dump(fp);

    (void) fprintf(fp, "\nDone\n");
    
    (void) fflush(fp);
    (void) fclose(fp);

    task_timer_peek();
    
    trace_log_tf(trace_global,
		 0,
		 LOG_INFO,
		 ("trace_do_dump: dump completed to %s",
		  path_dump));
}


#ifndef	NO_FORK
static task *trace_dump_task;		/* Pointer to the dump task */

static void
trace_dump_done __PF1(tp, task *)
{
    trace_dump_task = (task *) 0;
    task_delete(tp);
}

#endif	/* NO_FORK */


void
trace_dump __PF1(now, int)
{

#ifndef	NO_FORK
    if (trace_dump_task) {
	trace_log_tf(trace_global,
		     0,
		     LOG_ERR,
		     ("trace_dump: %s already active",
		      task_name(trace_dump_task)));
    } else if (now) {
	trace_do_dump((task *) 0);
    } else {
	trace_dump_task = task_alloc("TraceDump",
				     TASKPRI_PROTO,
				     trace_set_global((bits *) 0, (flag_t) 0));
	task_set_child(trace_dump_task, trace_dump_done);
	task_set_process(trace_dump_task, trace_do_dump);
	if (!task_fork(trace_dump_task)) {
	    task_quit(EINVAL);
	}
    }
#else	/* NO_FORK */
    trace_do_dump((task *) 0);
#endif	/* NO_FORK */
}


void
trace_cleanup __PF0(void)
{
    if (trace_global
	&& trace_global->tr_file
	&& trace_global->tr_file->trf_FILE != stderr) {
	/* Release tracing */
	
	trace_freeup(trace_global);
    }
#ifdef	DEBUG
    {
	trace_file *tfp;
	
	TRACE_FILES(tfp) {
	    syslog(LOG_CRIT,
		   "trace_cleanup: file %s still open, refcount %d",
		   tfp->trf_file ? tfp->trf_file : "stderr",
		   tfp->trf_refcount);
	} TRACE_FILES_END(tfp) ;
    }
#endif	/* DEBUG */
}


void
trace_reinit __PF0(void)
{
    register trace_file *tfp;

    if (trace_global
	&& (!trace_global->tr_flags
	    || !trace_global->tr_refcount)) {
	/* No flags set, get rid of trace block */

	trace_global = trace_free(trace_global);
    }
    
    TRACE_FILES(tfp) {
	if (!tfp->trf_FILE) {
	    trace_on(tfp);
	}
    } TRACE_FILES_END(tfp) ;
	
}

void
trace_init __PF0(void)
{
    trace_file *trf;
    
    /* Set allocation index */
    trace_trace_index = task_block_init(sizeof (trace), "trace");
    trace_file_index = task_block_init(sizeof (trace_file), "trace_file");

    /* Start tracing to stderr */
    trace_global = trace_create();
    trace_global->tr_names = trace_global_types;

    trf = (trace_file *) task_block_alloc(trace_file_index);
    INSQUE(trf, trace_files.trf_back);
    trace_global->tr_file = trace_file_alloc(trf);
    
    trace_on(trace_global->tr_file);
}

void
trace_init2 __PF0(void)
{

    if (!trace_global->tr_file->trf_file) {
	/* No trace file specified */

	if (trace_global->tr_flags) {
	    /* Tracing flags were specified, we trace to the console */

	    task_newstate(TASKS_NODETACH, (flag_t) 0);
	} else {
	    /* No trace flags specifed, close tracing so we can daemonize */

	    trace_freeup(trace_global);
	}
    }
}

/**/
void
trace_trace __PF2(trp, trace *,
		  cf, flag_t)
{
    char time_buffer[LINE_MAX];
    register char *dp = time_buffer;
    register char *sp;
    size_t tsize, tbsize, size;
    FILE *file = trp->tr_file->trf_FILE;

    if (!BIT_TEST(cf, TRC_NOSTAMP)) {
	sp = time_string;
	while (*sp) {
	    *dp++ = *sp++;
	}
	*dp++ = ' ';
    }

    if (task_mpid != task_pid) {
	*dp++ = '[';
	sp = task_pid_str;
	while (*sp) {
	    *dp++ = *sp++;
	}
	*dp++ = ']';
	*dp++ = ' ';
    }
    *dp = (char) 0;

    size = (tbsize = dp - time_buffer) + (tsize = trace_ptr - trace_buffer) + 1;
    if (file != stderr &&
	trp->tr_file->trf_limit_size &&
	(trp->tr_file->trf_size + size) > trp->tr_file->trf_limit_size) {
	/* Time to rotate files */
	
	/* Inform our audience */
	if (tbsize) {
	    (void) fwrite(time_buffer, sizeof (char), tbsize, file);
	}

	/* Rotate them around */
	trace_rotate(trp->tr_file);
	file = trp->tr_file->trf_FILE;
    }

    trp->tr_file->trf_size += size;

    if (BIT_TEST(cf, TRC_NL_BEFORE)) {
	if (tbsize) {
	    (void) fwrite(time_buffer, sizeof (char), tbsize, file);
	}
	putc('\n', file);
    }
    if (tbsize) {
	(void) fwrite(time_buffer, sizeof (char), tbsize, file);
    }
    (void) fwrite((char *) trace_buffer, sizeof (char), tsize, file);
    if (BIT_TEST(cf, TRC_NL_AFTER)) {
	putc('\n', file);
	if (tbsize) {
	    (void) fwrite(time_buffer, sizeof (char), tbsize, file);
	}
    }
    putc('\n', file);
#ifdef	MUST_FFLUSH
    (void) fflush(file);
#endif
}


flag_t
trace_parse_packet __PF3(detail, u_int,
			 inout, u_int,
			 indx, u_int)
{
    static flag_t flags[2][3][6] = {
	/* Packets */
	{
	    /* Recv and Send */	     
	    { TR_PACKET, TR_PACKET_1, TR_PACKET_2,
		  TR_PACKET_3, TR_PACKET_4, TR_PACKET_5 },
	    /* Recv */
	    { TR_PACKET_RECV, TR_PACKET_RECV_1, TR_PACKET_RECV_2,
		  TR_PACKET_RECV_3, TR_PACKET_RECV_4, TR_PACKET_RECV_5 },
	    /* Send */
	    { TR_PACKET_SEND, TR_PACKET_SEND_1, TR_PACKET_SEND_2,
		  TR_PACKET_SEND_3, TR_PACKET_SEND_4, TR_PACKET_SEND_5 }
	},
	/* Detail */
	{
	    /* Recv and Send */
	    { TR_DETAIL, TR_DETAIL_1, TR_DETAIL_2,
		  TR_DETAIL_3, TR_DETAIL_4, TR_DETAIL_5 },		
	    /* Recv */
	    { TR_DETAIL_RECV, TR_DETAIL_RECV_1, TR_DETAIL_RECV_2,
		TR_DETAIL_RECV_3, TR_DETAIL_RECV_4, TR_DETAIL_RECV_5 },
	    /* Send */
	    { TR_DETAIL_SEND, TR_DETAIL_SEND_1, TR_DETAIL_SEND_2,
		TR_DETAIL_SEND_3, TR_DETAIL_SEND_4, TR_DETAIL_SEND_5 }
	}
    };

    return flags[detail][inout][indx];
}


/*
 *	Prefill the trace buffer
 */

#ifdef	STDARG
/*VARARGS2*/
void
tracef(const char *fmt,...)
#else	/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
void
tracef(va_alist)
va_dcl
#endif	/* STDARG */
{
    va_list ap;
#ifdef	STDARG
    va_start(ap, fmt);
#else	/* STDARG */
    byte *fmt;

    va_start(ap);

    fmt = va_arg(ap, byte *);
#endif	/* STDARG */

    if (fmt && *fmt) {
	trace_ptr += vsprintf(trace_ptr, fmt, ap);
    }
    va_end(ap);
}
