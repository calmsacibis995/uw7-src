#ident	"@(#)ksh93:src/lib/libast/include/ast_dir.h	1.1"
#pragma prototyped

/*
 * common dirent maintenance interface
 */

#ifndef _AST_DIR_H
#define _AST_DIR_H

#include <ast_lib.h>
#include <dirent.h>

#if _mem_d_fileno_dirent || _mem_d_ino_dirent
#if !_mem_d_fileno_dirent
#define _mem_d_fileno_dirent	1
#define d_fileno		d_ino
#endif
#endif

#if _mem_d_fileno_dirent
#define D_FILENO(d)		((d)->d_fileno)
#else
#define D_FILENO(d)		(1)
#endif

#if _mem_d_namlen_dirent
#define D_NAMLEN(d)		((d)->d_namlen)
#else
#define D_NAMLEN(d)		(strlen((d)->d_name))
#endif

#endif
