#ident	"@(#)gid.h	1.2"
#ident  "$Header$"


/*
 * The gid_blk structure is used in the search for the default
 * uid.  Each gid_blk represent a range of gid(s) that are currently
 * used on the system.
*/
struct	gid_blk { 			
	struct	gid_blk	*link;
	gid_t		low;		/* low bound for this uid block */
	gid_t		high; 		/* high bound for this uid block */
};

