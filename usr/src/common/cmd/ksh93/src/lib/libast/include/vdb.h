#ident	"@(#)ksh93:src/lib/libast/include/vdb.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * virtual db file directory entry constants
 */

#ifndef VDB_MAGIC

#define VDB_MAGIC	"vdb"

#define VDB_DIRECTORY	"DIRECTORY"
#define VDB_UNION	"UNION"
#define VDB_DATE	"DATE"
#define VDB_MODE	"MODE"

#define VDB_DELIMITER	';'
#define VDB_IGNORE	'_'
#define VDB_FIXED	10
#define VDB_LENGTH	(sizeof(VDB_DIRECTORY)+2*(VDB_FIXED+1))
#define VDB_OFFSET	(sizeof(VDB_DIRECTORY))
#define VDB_SIZE	(VDB_OFFSET+VDB_FIXED+1)

#endif
