/*----------------------------------------------------------------------
  $Id$

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

#ifndef _ADRBKLIB_INCLUDED
#define _ADRBKLIB_INCLUDED

/*
 *  Some notes:
 *
 * There is considerable indirection going on to get to an address book
 * entry.  The idea is that we don't want to have to read in the entire
 * address book (takes too long and uses too much memory) so we have a
 * quick way to get to any entry.  There are "count" entries in the addrbook.
 * For each entry, there is an EntryRef element.  So there is an
 * array of count EntryRef elements, 0, ..., count-1.  These EntryRef elements
 * aren't all kept in memory.  Instead, there is a cache of EntryRef
 * elements.  The size of that cache is set with adrbk_set_nominal_cachesize().
 * That cache is fronted by a hashtable with the hashvalue just the element
 * number mod size_of_hashtable.  The size of the hashtable is just set so
 * that every hash bucket has 10 entries in it, so we have a constant short
 * time for looking for a cache hit.  EntryRef entries are prebuilt from
 * the addrbook file and stored in the EntryRef section of the addrbook.lu
 * file.  An EntryRef element consists of:
 *      uid_nick, uid_addr  Unique id's computed from nickname and address
 *      offset              Offset into addrbook to start of nickname
 *      next_nick           Next entry for nickname with same hash value
 *      next_addr           Next entry for address with same hash value
 *      ae                  Pointer to cached AdrBk_Entry (not stored on disk)
 * To get an AdrBk_Entry, say for element 13, adrbk_get_entryref(13) is
 * called.  That looks in the appropriate hash bucket for 13 and runs
 * through the list looking for a match on element 13.  If it doesn't find
 * it, it reads the entry from the addrbook.lu file as above.  That entry
 * can be found easily because it is at a calculable offset from the
 * start of the addrbook.lu file.  Then adrbk_init_ae() is called to parse
 * the entry and fill in an ae, which is pointed to by the cached EntryRef.
 * That's how you look up a particular entry *number* in the addrbook.  That
 * would be used for browsing the addrbook or searching the addrbook.
 *
 * The order that the address book is stored in on disk is the order that
 * the entries will be displayed in.  When an address book is opened,
 * if it is ReadWrite the sort order is checked and it is sorted if it is
 * out of order.  If an addrbook is already correctly sorted but doesn't
 * have a .lu file yet, the .lu file will be created, then the sort will be
 * checked by going through all N entries.  The sort_rule will be stored
 * into the .lu file so that the next time this happens the check can
 * be avoided.  That is, once the addrbook is sorted once, all operations
 * on it preserve the sort order.  So unless it is changed externally, or
 * the .lu file is removed, or the sort_rule is changed, this check will
 * never have to look at any of the entries again.
 *
 * You also want to be able to lookup an entry based on the nickname.  The
 * composer does this all the time by calling build_address().  That needs
 * to be very fast, so is done through another hashtable.  This table is
 * also precomputed and stored in the addrbook.lu file.  When the addrbook
 * is first opened, that hash table is read into memory.  Each entry in the
 * hashtable is a adrbk_cntr_t element number, which points to the head
 * of the list of entries with the same hash value.  That list is what
 * uses the next_nick variable in the EntryRef.  The hashtable is not very
 * large.  Just four (or 2) bytes per entry and size of hashtable entries.  The
 * size of the table varies with the size of the addrbook, and is set by
 * the function hashtable_size().  It is also stored in the addrbook.lu
 * file so need not be the value returned by hashtable_size.  That hashtable
 * is kept open the whole time Pine is running, once it has been opened.  To
 * look up an entry for nickname "nick", you compute the hash value (ab_hash)
 * of "nick" and the uid (ab_uid) of "nick".  Look in the hash array at
 * element number hash_value and find a pointer (an index into the EntryRef
 * array).  Now you would get that EntryRef just like above, since you
 * are now working with the element number.  Check to see if that EntryRef
 * has the same uid as "nick".  If so, that's the one you want.  If not,
 * next_nick is the index of the next entry with the same hash value.
 * Follow that list checking uid's until you find it or hit the end.
 * There is an entirely analogous hash table for lookup by address instead
 * of lookup by nickname.  That only applies to addresses of Single entries,
 * not Lists.
 *
 * Some sizes:
 *
 *  AdrHash is an array of adrbk_cntr_t of length hashtable_size.
 *  This size is usually set by the hashtable_size function.
 *  There are two of those, one for nicknames and one for addresses.
 *  These are kept open the whole time the program is being run (once opened).
 *  Those are also stored on disk in addrbook.lu.  They take up more
 *  space there since they are written out in ASCII for portability.
 *
 *  An EntryRef cache element is size 16-20.  The size of this cache is usually
 *  kept fairly small (200) but sorting cries out for it to be larger.
 *  When sorting in Unix, it is set equal to the "count" of the addrbook,
 *  smaller for DOS.  Each valid EntryRef cache element points to an
 *  EntryRef in memory.  Each of those is 20-24 fixed bytes.  Part of that is
 *  a pointer to a possible cached AdrBk_Entry.  An AdrBk_Entry is 22 bytes
 *  of fixed space, but it also has pointers to variable amounts of memory
 *  for nickname, fullname, addresses, fcc, and comments.  A typical
 *  entry probably uses about 50 bytes of that extra space, so altogether,
 *  a cached EntryRef with a fully-filled in ae takes around 100 bytes.
 *  If you had a 30,000 entry addrbook, setting the cache size to "count"
 *  for sorting takes up about 3,000,000 bytes of memory.
 *
 * That's the story for adrbklib.c.  There is also some allocation happening
 * in addrbook.c.  In particular, the display is a window into an array
 * of rows, at least one row per addrbook entry plus more for lists.
 * Each row is an AddrScrn_Disp structure and those should typically take
 * up 6 or 8 bytes.  A cached copy of addrbook entries is not kept, just
 * the element number to look it up (and often get it out of the EntryRef
 * cache).  In order to avoid having to allocate all those rows, this is
 * also in the form of a cache.  Only 3 * screen_length rows are kept in
 * the cache, and the cache is always a simple interval of rows.  That is,
 * rows between valid_low and valid_high are all in the cache.  Row numbers
 * in the display list are a little mysterious.  There is no origin.  That
 * is, you don't necessarily know where the start or end of the display
 * is.  You only know how to go forward and backward and how to "warp"
 * to new locations in the display and go forward and backward from there.
 * This is because we don't know how many rows there are all together.  It
 * is also a way to avoid searching through everything to get somewhere.
 * If you want to go to the End, you warp there and start backwards instead
 * of reading through all the entries to get there.  If you edit an entry
 * so that it sorts into a new location, you warp to that new location to
 * save processing all of the entries in between.
 *
 *
 * Notes about RFC1522 encoding:
 *
 * If the fullname field contains other than US-ASCII characters, it is
 * encoded using the rules of RFC1522 or its successor.  The value actually
 * stored in the file is encoded, even if it matches the character set of
 * the user.  This is so it is easy to pass the entry around or to change
 * character sets without invalidating entries in the address book.  When
 * a fullname is displayed, it is first decoded.  If the fullname field is
 * encoded as being from a character set other than the user's character
 * set, that will be retained until the user edits the field.  Then it will
 * change over to the user's character set.  The comment field works in
 * the same way, as do the "phrase" fields of any addresses.  On outgoing
 * mail, the correct character set will be retained if you use ComposeTo
 * from the address book screen.  However, if you use a nickname from the
 * composer or ^T from the composer, the character set will be lost if it
 * is different from the user's character set.
 */

#define NFIELDS 11 /* one more than num of data fields in addrbook entry */

/*
 * Disk file takes up more space when HUGE is defined, but we think it is
 * not enough more to offset the convenience of no limits.  So it is always
 * defined.  Should still work if you want to undefine it.
 */
#define HUGE_ADDRBOOKS

/*
 * The type a_c_arg_t is a little confusing.  It's the type we use in
 * place of adrbk_cntr_t when we want to pass an adrbk_cntr_t in an argument.
 * We were running into problems with the integral promotion of adrbk_cntr_t
 * args.  A_c_arg_t has to be large enough to hold a promoted adrbk_cntr_t.
 * So, if adrbk_cntr_t is unsigned short, then a_c_arg_t needs to be int if
 * int is larger than short, or unsigned int if short is same size as int.
 * Since usign16_t always fits in a short, a_c_arg_t of unsigned int should
 * always work for !HUGE.  For HUGE, usign32_t will be either an unsigned int
 * or an unsigned long.  If it is an unsigned long, then a_c_arg_t better be
 * an unsigned long, too.  If it is an unsigned int, then a_c_arg_t could
 * be an unsigned int, too.  However, if we just make it unsigned long, then
 * it will be the same in all cases and big enough in all cases.
 *
 * In the HUGE case, we could use usign32_t for the a_c_arg_t typedef.
 * There is no actual advantage to be gained, though.  The only place it
 * would make a difference is on machines where an int is 32 bits and a
 * long is 64 bits, in which case the 64-bit long is probably the more
 * efficient size to use anyway.
 */

#ifdef HUGE_ADDRBOOKS

typedef usign32_t adrbk_cntr_t;  /* addrbook counter type                */
typedef unsigned long a_c_arg_t;     /* type of arg passed for adrbk_cntr_t  */
#define NO_NEXT ((adrbk_cntr_t)-1)
#define MAX_ADRBK_SIZE (2000000000L) /* leave room for extra display lines   */
#define MAX_HASHTABLE_SIZE 150000

# else /* !HUGE_ADDRBOOKS */

typedef usign16_t adrbk_cntr_t; /* addrbook counter type                */
typedef unsigned a_c_arg_t;          /* type of arg passed for addrbk_cntr_t */
#define NO_NEXT ((adrbk_cntr_t)-1)
#define MAX_ADRBK_SIZE ((long)(NO_NEXT - 2))
#define MAX_HASHTABLE_SIZE 60000

# endif /* !HUGE_ADDRBOOKS */

/*
 * The value NO_NEXT is reserved to mean that there is no next address, or that
 * there is no address number to return.  This is similar to getc returning
 * -1 when there is no char to get, but since we've defined this to be
 * unsigned we need to reserve one of the valid values for this purpose.
 * With current implementation it needs to be all 1's, so memset initialization
 * will work correctly.
 */

typedef long adrbk_uid_t;   /* the UID of a name or address */

typedef enum {ReadOnly, ReadWrite, NoAccess, NoExists} AccessType;
typedef enum {NotSet, Single, List} Tag;
typedef enum {Normal, Delete, SaveDelete, Lock, Unlock} Handling;

/* This is what is actually used by the routines that manipulate things */
typedef struct adrbk_entry {
    char *nickname;
    char *fullname;    /* of simple addr or list                        */
    union addr {
        char *addr;    /* for simple Single entries                     */
        char **list;   /* for distribution lists                        */
    } addr;
    char *fcc;         /* fcc specific for when sending to this address */
    char *extra;       /* comments field                                */
    char  referenced;  /* for detecting loops during lookup             */
    Tag   tag;         /* single addr (Single) or a list (List)         */
} AdrBk_Entry;

/*
 * This points to the data in a file.  It gives us a smaller way to store
 * data and a fast way to lookup a particular entry.  When we need the
 * actual data we look in the file and produce an AdrBk_Entry.
 */
typedef struct entry_ref {
    adrbk_uid_t  uid_nick;  /* uid(nickname) */
    adrbk_uid_t  uid_addr;  /* uid(address) */
    long         offset;    /* offset into file where this entry starts    */
    adrbk_cntr_t next_nick; /* index of next nickname with same hash value */
    adrbk_cntr_t next_addr;
    AdrBk_Entry *ae;        /* cached ae */
} EntryRef;

typedef struct er_cache_elem {
    EntryRef *entry;            /* the cached entryref */
    struct er_cache_elem *next; /* pointers to other cached entryrefs */
    struct er_cache_elem *prev;
    adrbk_cntr_t elem;          /* the index in the entryref array of entry */
    Handling handling;          /* handling instructions */
    unsigned char lock_ref_count; /* when reaches zero, it's unlocked */
}ER_CACHE_ELEM_S;

/*
 * hash(nickname) -> index into harray
 * harray(index) is an index into an array of EntryRef's.
 */
typedef struct adrhash {
    adrbk_cntr_t *harray;   /* the hash array, alloc'd (hash(name) points
				   into this array and the value in this
				   array is an index into the EntryRef array */
} AdrHash;

/* information useful for displaying the addrbook */
typedef struct width_stuff {
    int max_nickname_width;
    int max_fullname_width;
    int max_addrfield_width;
    int max_fccfield_width;
    int third_biggest_fullname_width;
    int third_biggest_addrfield_width;
    int third_biggest_fccfield_width;
} WIDTH_INFO_S;

typedef struct expanded_list {
    adrbk_cntr_t          ent;
    struct expanded_list *next;
} EXPANDED_S;

typedef struct adrbk {
    char         *orig_filename;       /* passed in filename                 */
    char         *filename;            /* addrbook filename                  */
    char         *temp_filename;       /* tmp file while writing out changes */
    FILE         *fp;                  /* fp for filename                    */
    AdrHash      *hash_by_nick;
    AdrHash      *hash_by_addr;
    char         *hashfile;
    int           delete_hashfile;     /* remove tmp hashfile when done      */
    char         *temp_hashfile;       /* tmp file while writing out changes */
    FILE         *fp_hash;
    AccessType    hashfile_access;     /* access permission for hashfile     */
    adrbk_cntr_t  htable_size;         /* how many entries in AdrHash tables */
    adrbk_cntr_t  count;               /* how many entries in addrbook       */
    time_t        last_change_we_know_about;/* to look for others changing it*/
    ER_CACHE_ELEM_S **head_cache_elem; /* array of cache elem heads          */
    ER_CACHE_ELEM_S **tail_cache_elem;
    int            *n_ae_cached_in_this_bucket;
    long            nominal_max_cached;
    adrbk_cntr_t    er_hashsize;
    WIDTH_INFO_S  widths;              /* helps addrbook.c format columns    */
    long          deleted_cnt;         /* how many #DELETED entries in abook */
    int           sort_rule;
    EXPANDED_S   *exp;                 /* this is for addrbook.c to use.  A
			       list of expanded list entry nums is kept here */
    EXPANDED_S   *checks;              /* this is for addrbook.c to use.  A
			       list of checked entry nums is kept here */
} AdrBk;

#define ONE_HUNDRED_DAYS (60L * 60L * 24L * 100L)
/*
 * When address book entries are deleted, they are left in the file
 * with the nickname prepended with a string like #DELETED-96/01/25#, 
 * which stands for year 96, month 1, day 25 of the month.  When one of
 * these entries is more than ABOOK_DELETED_EXPIRE_TIME seconds old,
 * then it will be totally removed from the address book the next time
 * and adrbk_write() is done.  This is for emergencies where somebody
 * deletes something from their address book and would like to get it
 * back.  You get it back by editing the nickname field manually to remove
 * the extra 18 characters off the front.
 */
#define ABOOK_DELETED_EXPIRE_TIME   ONE_HUNDRED_DAYS


/*
 * There are no restrictions on the length of any of the fields, except that
 * there are some restrictions in the current input routines.
 * There can be no more than 65534 entries (unless HUGE) in a single addrbook.
 */

/*
 * The on-disk address book has entries that look like:
 *
 * Nickname TAB Fullname TAB Address_Field TAB Fcc TAB Comment
 *
 * An entry may be broken over more than one line but only at certain
 * spots.  A continuation line starts with spaces (spaces, not white space).
 * One place a line break can occur is after any of the TABs.  The other
 * place is in the middle of a list of addresses, between addresses.
 * The Address_Field may be either a simple address without the fullname
 * or brackets, or it may be an address list.  An address list is
 * distinguished by the fact that it begins with "(" and ends with ")".
 * Addresses within a list are comma separated and each address in the list
 * may be a full rfc822 address, including Fullname and so on.
 *
 * Examples:
 * fred TAB Flintstone, Fred TAB fred@bedrock.net TAB fcc-flintstone TAB comment
 * or
 * fred TAB Flintstone, Fred TAB \n
 *    fred@bedrock.net TAB fcc-flintstone TAB \n
 *    comment
 * somelist TAB Some List TAB (fred, \n
 *    Barney Rubble <barney@bedrock.net>, wilma@bedrock.net) TAB \n
 *    fcc-for-some-list TAB comment
 *
 * There is also an on-disk file (called hashfile in the structure)
 * which is useful for quick access to particular entries.  It has the
 * form:
 *                Header
 *                EntryRef Array
 *                HashTable by Nickname
 *                HashTable by Address
 *                Trailer
 *
 * This file is written in ASCII so that it will be portable to multiple
 * clients.  It is named addrbook.lu, where addrbook is the name of the
 * addrbook file.  lu stands for LookUp.
 *
 *       Header -- magic number "P # * E @"
 *            <SPACE> two character version string
 *            <SPACE> hash table size "\n" integer padded on left with spaces
 * ifdef HUGE_ADDRBOOKS
 *	It takes up 10 character slots
 * else
 *	Value restricted to take up 5 character slots
 *
 * EntryRef Arr -- An array of N OnDiskEntryRef's, where N is the number
 *                     of entries in the address book.  Each entry is followed
 *                     by "\n".
 *
 *    HashTables -- Arrays of hash table size integers, each of which is
 *                     padded on left with spaces so it takes up 5 character
 *                     (10 if HUGE_ADDRBOOKS)
 *                     slots.  Each integer ends with a "\n" to make it
 *                     easier to use debugging tools.
 *    and another one of those for by address hash table
 *
 *      Trailer -- magic number "P # * E @"
 *            <SPACE> N  "\n" the same N as the number of EntryRef entries
 *        (N is 10 wide if HUGE_ADDRBOOKS, 5 otherwise)
 *           DELETED_CNT "\n"  (11 chars wide, count of #DELETED lines)
 *           W1 W2 W3 W4 W5 W6 W7 "\n"   These W's are widths for helping
 *                             to make a nice display of the addrbook.  Each
 *                             consists of a SPACE followed by a one or two
 *                             digit number.
 *           time created        10 bytes, seconds since Jan 1 70
 *           <SPACE> sort_rule "\n"  Sort rule used to sort last time
 *                                    (from AB_SORT... in pine.h)
 *                                    2 bytes, decimal number.
 *                    W1 = max_nickname_width
 *                    W2 = max_fullname_width
 *                    W3 = max_addrfield_width
 *                    W4 = max_fccfield_width
 *                    W5 = third_biggest_fullname_width
 *                    W6 = third_biggest_addrfield_width
 *                    W7 = third_biggest_fccfield_width
 *
 * Each OnDiskEntryRef looks like:
 *      next_nick  -- 5 bytes, positive integer padded on left with spaces.
 *      next_addr  -- 5 bytes, positive integer padded on left with spaces.
 *                   (each of those is 10 with HUGE_ADDRBOOKS)
 *      uid_nick   -- 11 bytes, integer padded on left with spaces.
 *      uid_addr   -- 11 bytes, integer padded on left with spaces.
 *      offset     -- 10 bytes, positive integer padded on left with spaces.
 * Each of these is separated by an extra SPACE, as well, so there are
 * 47 bytes per array element (counting the newline).
 */

#define ADRHASH_FILE_SUFFIX      ".lu"
#ifdef HUGE_ADDRBOOKS
#define ADRHASH_FILE_VERSION_NUM "14"
#else
#define ADRHASH_FILE_VERSION_NUM "13"
#endif
#define PMAGIC                   "P#*E@"
#define LEGACY_PMAGIC            "P#*@ "  /* sorry about that */

#define SIZEOF_PMAGIC       (5)
#define SIZEOF_SPACE        (1)
#define SIZEOF_NEWLINE      (1)
#define SIZEOF_VERSION_NUM  (2)
#define SIZEOF_SORT_RULE    (2)
#define SIZEOF_WIDTH        (2)
#define SIZEOF_ASCII_USHORT (5)
#define SIZEOF_ASCII_ULONG  (10)
#define SIZEOF_ASCII_LONG   (11)  /* because of possible "-" sign */
#define SIZEOF_UID          SIZEOF_ASCII_LONG
#define SIZEOF_FILEOFFSET   SIZEOF_ASCII_ULONG
#define SIZEOF_TIMESTAMP    SIZEOF_ASCII_ULONG
#define SIZEOF_DELETED_CNT  SIZEOF_ASCII_LONG

#ifdef HUGE_ADDRBOOKS

#define SIZEOF_HASH_SIZE    SIZEOF_ASCII_ULONG
#define SIZEOF_HASH_INDEX   SIZEOF_ASCII_ULONG

#else /* !HUGE_ADDRBOOKS */

#define SIZEOF_HASH_SIZE    SIZEOF_ASCII_USHORT
#define SIZEOF_HASH_INDEX   SIZEOF_ASCII_USHORT

#endif /* !HUGE_ADDRBOOKS */

#define SIZEOF_COUNT        SIZEOF_HASH_INDEX

#define TO_FIND_HDR_PMAGIC  (0)
#define TO_FIND_VERSION_NUM (TO_FIND_HDR_PMAGIC + SIZEOF_PMAGIC + SIZEOF_SPACE)
#define TO_FIND_HTABLE_SIZE (TO_FIND_VERSION_NUM + \
			     SIZEOF_VERSION_NUM + SIZEOF_SPACE)
#define SIZEOF_HDR (TO_FIND_HTABLE_SIZE + \
		    SIZEOF_HASH_SIZE + SIZEOF_NEWLINE)

#define TO_FIND_SORT_RULE (-(SIZEOF_SORT_RULE + SIZEOF_NEWLINE))
#define TO_FIND_TIMESTAMP (TO_FIND_SORT_RULE + \
                        (-(SIZEOF_TIMESTAMP + SIZEOF_SPACE)))
#define TO_FIND_WIDTHS (TO_FIND_TIMESTAMP + \
		    (-(7 * (SIZEOF_SPACE + SIZEOF_WIDTH) + SIZEOF_NEWLINE)))
#define TO_FIND_DELETED_CNT (TO_FIND_WIDTHS + \
			(-(SIZEOF_DELETED_CNT + SIZEOF_NEWLINE)))
#define TO_FIND_COUNT (TO_FIND_DELETED_CNT + \
			(-(SIZEOF_COUNT + SIZEOF_NEWLINE)))
#define TO_FIND_TRLR_PMAGIC (TO_FIND_COUNT + \
			(-(SIZEOF_PMAGIC + SIZEOF_SPACE)))
#define SIZEOF_TRLR (-(TO_FIND_TRLR_PMAGIC))

#define SIZEOF_ENTRYREF_ENTRY (SIZEOF_HASH_INDEX + SIZEOF_SPACE + \
			       SIZEOF_HASH_INDEX + SIZEOF_SPACE + \
			       SIZEOF_UID        + SIZEOF_SPACE + \
			       SIZEOF_UID        + SIZEOF_SPACE + \
			       SIZEOF_FILEOFFSET + SIZEOF_NEWLINE)
#define SIZEOF_HTABLE_ENTRY (SIZEOF_HASH_INDEX + SIZEOF_NEWLINE)


/*
 * Prototypes
 */
int             adrbk_add PROTO((AdrBk *, a_c_arg_t, char *, char *, char *, \
				 char *, char *, Tag, adrbk_cntr_t *, int *));
void            adrbk_clearrefs PROTO((AdrBk *));
void            adrbk_close PROTO((AdrBk *));
void            adrbk_partial_close PROTO((AdrBk *));
adrbk_cntr_t    adrbk_count PROTO((AdrBk *));
int             adrbk_delete PROTO((AdrBk *, a_c_arg_t, int));
char           *adrbk_formatname PROTO((char *));
AdrBk_Entry    *adrbk_get_ae PROTO((AdrBk *, a_c_arg_t, Handling));
int             adrbk_is_in_sort_order PROTO((AdrBk *, int));
int             adrbk_listadd PROTO((AdrBk *, a_c_arg_t, char *));
int             adrbk_nlistadd PROTO((AdrBk *, a_c_arg_t, char **));
int             adrbk_listdel PROTO((AdrBk *, a_c_arg_t, char *));
int             adrbk_listdel_all PROTO((AdrBk *, a_c_arg_t));
AdrBk_Entry    *adrbk_lookup_by_addr PROTO((AdrBk *, char *, adrbk_cntr_t *));
AdrBk_Entry    *adrbk_lookup_by_nick PROTO((AdrBk *, char *, adrbk_cntr_t *));
AdrBk_Entry    *adrbk_newentry PROTO((void));
AdrBk          *adrbk_open PROTO((char *, char *, char *, int, int, int));
long            adrbk_get_nominal_cachesize PROTO((AdrBk *));
long            adrbk_set_nominal_cachesize PROTO((AdrBk *, long));
int             adrbk_sort PROTO((AdrBk *, a_c_arg_t, adrbk_cntr_t *, int));
void            exp_free PROTO((EXPANDED_S *));
adrbk_cntr_t	exp_get_next PROTO ((EXPANDED_S **));
int             exp_is_expanded PROTO((EXPANDED_S *, a_c_arg_t));
void            exp_set_expanded PROTO((EXPANDED_S *, a_c_arg_t));
void            exp_unset_expanded PROTO((EXPANDED_S *, a_c_arg_t));
void            free_ae PROTO((AdrBk *, AdrBk_Entry **));
char          **parse_addrlist PROTO((char *));

#endif /* _ADRBKLIB_INCLUDED */
