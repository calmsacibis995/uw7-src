#ident	"@(#)ksh93:src/lib/libcmd/cmdlib.h	1.1"
#pragma prototyped

/*
 * common ast cmd library definitions
 */

#ifndef _CMDLIB_H
#define _CMDLIB_H

#include <ast.h>
#include <cmd.h>
#include <error.h>
#include <stak.h>

#define wc_count	_cmd_wccount
#define wc_init		_cmd_wcinit
#define rev_line	_cmd_revline

extern int		rev_line(Sfio_t*, Sfio_t*, off_t);

#endif
