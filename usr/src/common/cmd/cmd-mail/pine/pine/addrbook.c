#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

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

/*====================================================================== 
    addrbook.c
    display, browse and edit the address book.

    Support routines are in adrbklib.c.

The policy for changing the address book is to write it immediately 
after the change is made, so there is no idea of having to save the 
address book.
 ====*/


#include "headers.h"
#include "adrbklib.h"


/*
 * We could make every use of an AdrBk_Entry go through a function call
 * like adrbk_get_ae().  Instead, we try to be smart and avoid the extra
 * function calls by knowing when the addrbook entry is still valid, either
 * because we haven't called any functions that could invalidate it or because
 * we have locked it in the cache.  If we do lock it, we need to be careful
 * that it eventually gets unlocked.  That can be done by an explicit
 * adrbk_get_ae(Unlock) call, or it is done implicitly when the address book
 * is written out.  The reason it can get invalidated is that the abe that
 * we get returned to us is just a pointer to a cached addrbook entry, and
 * that entry can be flushed from the cache by other addrbook activity.
 * So we need to be careful to make sure the abe is certain to be valid
 * before using it.
 *
 * Data structures for the display of the address book.  There's one
 * of these structures per line on the screen.
 *
 * Types: Title -- The title line for the different address books.  It has
 *		   a ptr to the text of the Title line.
 *    ClickHere -- This is the line that says to click here to
 *                 expand.  It changes types into the individual expanded
 *                 components once it is expanded.  It doesn't have any data
 *                 other than an implicit title.
 * ListClickHere --This is the line that says to click here to
 *                 expand the members of a distribution list.  It changes
 *                 types into the individual expanded ListEnt's (if any)
 *                 when it is expanded.  It has a ptr to an AdrBk_Entry.
 *    ListEmpty -- Line that says this is an empty distribution list.  No data.
 *        Empty -- Line that says this is an empty addressbook.  No data.
 *       Simple -- A single addressbook entry.  It has a ptr to an AdrBk_Entry.
 *                 When it is displayed, the fields are usually:
 *                 <nickname>       <fullname>       <address or another nic>
 *     ListHead -- The head of an address list.  This has a ptr to an
 *		   AdrBk_Entry.
 *                 <blank line> followed by
 *                 <nickname>       <fullname>       "DISTRIBUTION LIST:"
 *      ListEnt -- The rest of an address list.  It has a pointer to its
 *		   ListHead element and a ptr (other) to this specific address
 *		   (not a ptr to another AdrBk_Entry).
 *                 <blank>          <blank>          <address or another nic>
 *         Text -- A ptr to text.  For example, the ----- lines and
 *		   whitespace lines.
 *   Beginnning -- The (imaginary) elements before the first real element
 *          End -- The (imaginary) elements after the last real element
 */
typedef enum {DlNotSet, ClickHere, Empty, Title, Simple, ListHead,
	    ListClickHere, ListEmpty, ListEnt, Text, Beginning, End} LineType;
/* each line is one of these structures */
typedef struct addrscrn_disp {
    union {
        struct {
            adrbk_cntr_t  ab_element_number; /* which addrbook entry     */
	    adrbk_cntr_t  ab_list_offset;    /* which member of the list */
        }addrbook_entry;
        char        *text_ptr;
    }union_to_save_space;
    LineType       type;
} AddrScrn_Disp;
#define txt union_to_save_space.text_ptr
#define elnum union_to_save_space.addrbook_entry.ab_element_number
#define l_offset union_to_save_space.addrbook_entry.ab_list_offset

#define entry_is_checked    exp_is_expanded
#define entry_get_next      exp_get_next
#define entry_set_checked   exp_set_expanded
#define entry_unset_checked exp_unset_expanded
#define checked_free        exp_free

/*
 * Argument to expand_address and build_address_internal is a BuildTo,
 * which is either a char * address or an AdrBk_Entry * (if we have already
 * looked it up in an addrbook).
 */
typedef enum {Str, Abe} Build_To_Arg_Type;
typedef struct build_to {
    Build_To_Arg_Type type;
    union {
	char        *str;  /* normal looking address string */
	AdrBk_Entry *abe;  /* addrbook entry */
    }arg;
} BuildTo;

typedef enum {BP_Unset, BP_To, BP_Lcc} WhoSetUs;
typedef struct builder_private {
    WhoSetUs who;
    int      cksumlen;
    long     cksumval;
} BuilderPrivate;

typedef enum {DlcNotSet,
	      DlcTitleBlankTop,
	      DlcTitleDashTop,
	      DlcTitle,
	      DlcTitleDashBottom,
	      DlcTitleBlankBottom,
	      DlcClickHere,
	      DlcEmpty,
	      DlcNoPermission,
	      DlcSimple,
	      DlcListHead,
	      DlcListClickHere,
	      DlcListEmpty,
	      DlcListEnt,
	      DlcListBlankTop,
	      DlcListBlankBottom,
	      DlcBeginning,
	      DlcOneBeforeBeginning,
	      DlcTwoBeforeBeginning,
	      DlcEnd} DlCacheType;

typedef enum {Initialize, FirstEntry, LastEntry, ArbitraryStartingPoint,
	      DoneWithCache, FlushDlcFromCache, Lookup} DlMgrOps;
typedef enum {Warp, DontWarp} HyperType;
/*
 * The DlCacheTypes are the types that a dlcache element can be labeled.
 * The idea is that there needs to be enough information in the single
 * cache element by itself so that you can figure out what the next and
 * previous dl rows are by just knowing this one row.
 *
 * If there is only one addrbook, there are no DlcTitle* lines, and no
 * DlcClickHere lines.  The first line will be a DlcListHead or a DlcSimple.
 * If more than one addrbook, each has a title.  All but the first have
 * a DlcTitleBlankTop blank line before the title, and all of them have
 * the other 4 types of DlcTitle type lines.  An unexpanded book is a
 * DlcClickHere, an Empty book is a DlcEmpty.  Each DlcSimple is just a
 * single line.  Each list is a DlcListHead and some number of DlcListEnts
 * if there are any list members.  If there are none, there is a DlcListEmpty.
 * If the list isn't expanded, instead of a variable number of DlcListEnts
 * there is one DlcListClickHere after the DlcListHead.  Two lists are
 * separated by a DlcListBlankBottom belonging to the first list.  A list
 * followed or preceded by a DlcSimple address row has a
 * DlcListBlank(Top or Bottom) separating it from the DlcSimple.  A DlcTitle
 * and a list are separated by a DlcTitleBlank*, not a DlcListBlank*.
 * Each cache element has an adrbk number associated with it.  All of the
 * DlcTitle* rows, including the DlcTitleBlankTop, go with the adrbk they
 * are the title for.  DlcListBlankTop's have the elnum of the list
 * they're above.  DlcListBlankBottom's have the elnum of the list
 * they're below.  Above the top row of the display list is a type
 * DlcBeginning row.  Below the bottom row of the display list is a
 * type DlcEnd row.
 */
typedef struct dl_cache {
    long         global_row; /* disp_list row number */
    adrbk_cntr_t dlcelnum;   /* which elnum from that addrbook */
    adrbk_cntr_t dlcoffset;  /* offset in a list, only for ListEnt rows */
    short        adrbk_num;  /* which address book we're related to */
    DlCacheType  type;       /* type of this row */
    AddrScrn_Disp dl;	     /* the actual dl that goes with this row */
} DL_CACHE_S;

typedef enum {Nickname, Fullname, Addr, Filecopy, Comment, Notused,
	      Def, WhenNoAddrDisplayed, Checkbox} ColumnType;
/*
 * Users can customize the addrbook display, so this tells us which data
 * is in a particular column and how wide the column is.  There is an
 * array of these per addrbook, of length NFIELDS (number of possible cols).
 */
typedef struct column_description {
    ColumnType type;
    WidthType  wtype;
    int        req_width; /* requested width (for fixed and percent types) */
    int        width;     /* actual width to use */
    int        old_width;
} COL_S;

typedef enum {LocalPersonal, LocalGlobal} AddrBookType;
typedef enum {TotallyClosed, /* hash tables not even set up yet               */
	      Closed,     /* data not read in, no display list                */
	      NoDisplay,  /* data is accessible, no display list              */
	      HalfOpen,   /* data not accessible, initial display list is set */
	      Open        /* data is accessible and display list is set       */
	     } OpenStatus;
/*
 * There is one of these per addressbook.
 */
typedef struct peraddrbook {
    AddrBookType        type;
    AccessType          access;
    OpenStatus          ostatus;
    char               *nickname,
		       *filename;
    AdrBk              *address_book;        /* the address book handle */
    int                 gave_parse_warnings;
    COL_S               disp_form[NFIELDS];  /* display format */
    int			nick_is_displayed;   /* these are for convenient, */
    int			full_is_displayed;   /* fast access.  Could get   */
    int			addr_is_displayed;   /* same info from disp_form. */
    int			fcc_is_displayed;
    int			comment_is_displayed;
} PerAddrBook;

/*
 * Just one of these.  This keeps track of the current state of
 * the screen and which addressbook we're looking at.  It is really just
 * global data (accessed only from this file).  It's all in one structure so
 * that it's easier to recognize as global.
 */
typedef struct addrscreenstate {
    PerAddrBook   *adrbks;       /* array of addrbooks                    */
    int		   initialized,  /* have we done at least simple init?    */
                   n_addrbk,     /* how many addrbooks are there          */
                   how_many_personals, /* how many of those are personal? */
                   cur,          /* current addrbook                      */
                   cur_row,      /* currently selected line               */
                   old_cur_row,  /* previously selected line              */
                   l_p_page;	 /* lines per (screen) page               */
    long           top_ent;      /* index in disp_list of top entry on screen */
    int            ro_warning;   /* whether or not to give warning        */
    int            checkboxes;   /* whether or not to display checkboxes  */
    int            no_op_possbl; /* user can't do anything with current conf */
#ifdef	_WINDOWS
    long	   last_ent;	 /* index of last known entry		  */
#endif
} AddrScrState;

static AddrScrState as;

/*
 * AddrBookScreen is the maintenance screen, all the others are selection
 * screens.  Those that end in Com are called from the pico HeaderEditor,
 * either while in the composer or while editing an address book entry.
 * SelectManyNicks returns an array of nicknames.  SelectAddrLccCom and
 * SelectNicksCom return a comma-separated list of nicknames.  SelectNickTake,
 * SelectNickCom, and SelectNick all return a single nickname.  SelectAddrCom
 * returns a comma-separated list of addresses.  SelectAddr, SelectAddrTake,
 * and SelectAddrNoFullCom return a single address (which can actually be a
 * comma-separated list but is sort of intended to be a single address).
 * SelectAddrTake and SelectAddrNoFullCom eliminate the fullname field
 * before returning the address.  The ones that returns multiple nicknames
 * or multiple addresses all allow ListMode.  They are SelectAddrCom,
 * SelectAddrLccCom, SelectNicksCom, and SelectManyNicks (which automatically
 * starts in ListMode).
 */
typedef enum {AddrBookScreen,	   /* maintenance screen                     */
	      SelectAddrCom,	   /* returns list of addresses              */
	      SelectAddrLccCom,	   /* returns list of nicknames of lists     */
	      SelectNicksCom,	   /* just like SelectAddrLccCom, but allows
				      selecting simple *and* list entries    */
	      SelectAddr,	   /* returns single expanded entry          */
	      SelectAddrTake,	   /* same as above but fullname is stripped */
	      SelectAddrNoFullCom, /* same as above but from composer        */
	      SelectNick,	   /* returns single nickname                */
	      SelectNickTake,	   /* Same as SelectNick but different help  */
	      SelectNickCom,	   /* Same as SelectNick but from composer   */
	      SelectManyNicks	   /* auto ListMode, returns nicks in array  */
	     } AddrBookArg;

typedef struct save_state_struct {
    AddrScrState *savep;
    OpenStatus   *stp;
    DL_CACHE_S   *dlc_to_warp_to;
} SAVE_STATE_S;

/*
 * Information used to paint and maintain a line on the TakeAddr screen
 */
typedef struct takeaddr_line {
    int	                  checked;  /* addr is selected                     */
    int	                  skip_it;  /* skip this one                        */
    int	                  print;    /* for printing only                    */
    int	                  frwrded;  /* forwarded from another pine          */
    char                 *strvalue; /* alloc'd value string                 */
    ADDRESS              *addr;     /* original ADDRESS this line came from */
    char                 *nickname; /* The first TA may carry this extra    */
    char                 *fullname; /*   information                        */
    char                 *fcc;
    char                 *comment;
    struct takeaddr_line *next, *prev;
} TA_S;

typedef enum {ListMode, SingleMode} TakeAddrScreenMode;

typedef struct takeaddress_screen {
    TakeAddrScreenMode mode;
    TA_S              *current,
                      *top_line;
} TA_SCREEN_S;

static TA_SCREEN_S *ta_screen;

/*
 * Jump back to this location if we discover that one of the open addrbooks
 * has been changed by some other process.
 *
 * The trouble_filename variable is usually NULL (no trouble) but may be
 * set if adrbklib detected trouble in an addrbook.lu file.  In that case,
 * trouble_filename will be set to the name of the addressbook
 * with the problem.  It is used to force a rebuild of the .lu file.
 */
jmp_buf addrbook_changed_unexpectedly;
char   *trouble_filename;


void           ab_compose_to_addr PROTO((long));
void           ab_export PROTO((long, int));
void           ab_forward PROTO((struct pine *, long));
void           ab_goto_folder PROTO((int));
void           ab_print PROTO((void));
void           ab_resize PROTO(());
long           ab_whereis PROTO((int *, int));
void           add_abook_entry PROTO((TA_S *, char *, char *, char *,
								char *, int));
int            add_addresses_to_talist PROTO((struct pine *, long, char *,
						    TA_S **, ADDRESS *, int));
void           add_forced_entries PROTO((AdrBk *));
char          *addr_book PROTO((AddrBookArg, char *, char ***));
char          *addr_book_change_list PROTO((char **));
int            addr_book_delete PROTO((AdrBk *, int, long, int *));
void           addr_book_manynicks PROTO((char ***));
char          *addr_book_nick_for_edit PROTO((void));
char          *addr_book_seladdr PROTO((void));
char          *addr_book_seladdr_nofull PROTO((void));
char          *addr_book_selnick PROTO((void));
char          *addr_book_takeaddr PROTO((void));
char          *addr_lookup PROTO((char *, int *, int));
AdrBk_Entry   *addr_to_abe PROTO((ADDRESS *));
AccessType     adrbk_access PROTO((char *));
AdrBk_Entry   *adrbk_lookup_with_opens_by_nick PROTO((char *, int, int *, int));
int            adrbk_num_from_lineno PROTO((long));
AdrBk_Entry   *ae PROTO((long));
int            any_addrs_avail PROTO((long));
int            build_address_internal PROTO((BuildTo, char **, char **,
					    char **, int *, char **, int, int));
int            calculate_field_widths PROTO((void));
void           cancel_warning PROTO((int, char *));
PerAddrBook   *check_for_addrbook PROTO((char *));
void           clickable_warning PROTO((long));
ADDRESS       *copyaddrlist PROTO((ADDRESS *));
int            cur_addr_book PROTO((void));
char          *decode_fullname_of_addrstring PROTO((char *, int));
DL_CACHE_S    *dlc_from_listmem PROTO((AdrBk *, a_c_arg_t, char *));
DL_CACHE_S    *dlc_next PROTO((DL_CACHE_S *, DL_CACHE_S *));
DL_CACHE_S    *dlc_prev PROTO((DL_CACHE_S *, DL_CACHE_S *));
AddrScrn_Disp *dlist PROTO((long));
DL_CACHE_S    *dlc_mgr PROTO((long, DlMgrOps, DL_CACHE_S *));
int            dlcs_from_same_abe PROTO((DL_CACHE_S *, DL_CACHE_S *));
void           display_book PROTO((int, int, int, int, Pos *));
void           done_with_dlc_cache PROTO((void));
int            dup_addrs PROTO((ADDRESS *, ADDRESS *));
void           dump_a_dlc_to_debug PROTO((char *, DL_CACHE_S *));
void           dump_some_debugging PROTO((char *));
void           edit_entry PROTO((AdrBk *, AdrBk_Entry *, a_c_arg_t, Tag,
								int, int *));
int            edit_nickname PROTO((AdrBk *, AddrScrn_Disp *, int, char *,
						char *, HelpType, int, int));
int            eliminate_dups_and_us PROTO((TA_S *));
int            eliminate_dups_but_not_us PROTO((TA_S *));
int            eliminate_dups_and_maybe_us PROTO((TA_S *, int));
void           empty_warning PROTO((long));
char          *encode_fullname_of_addrstring PROTO((char *, char *));
void           end_adrbks PROTO((void));
int            entry_is_clickable PROTO((long));
int            est_size PROTO((ADDRESS *));
ADDRESS       *expand_address PROTO((BuildTo, char *, char *, int *, char **,
					 int *, char **, char **, int, int));
void           expand_addrs_for_pico PROTO((struct headerentry *));
void           fill_in_dl_field PROTO((DL_CACHE_S *));
TA_S          *fill_in_ta PROTO((TA_S **, ADDRESS *, int, char *));
int            find_in_book PROTO((long, char *, long *, int *));
TA_S          *first_checked PROTO((TA_S *));
long           first_line PROTO((long));
long           first_selectable_line PROTO((long));
TA_S          *first_sel_taline PROTO((TA_S *));
TA_S          *last_sel_taline PROTO((TA_S *));
TA_S          *first_taline PROTO((TA_S *));
TA_S          *whereis_taline PROTO((TA_S *));
void           flush_dlc_from_cache PROTO((DL_CACHE_S *));
void           free_cache_array PROTO((DL_CACHE_S **, int));
void           free_list PROTO((char ***));
void           free_taline PROTO((TA_S **));
int            funny_compare_dlcs PROTO((DL_CACHE_S *, DL_CACHE_S *));
DL_CACHE_S    *get_bottom_dl_of_adrbk PROTO((int, DL_CACHE_S *));
DL_CACHE_S    *get_dlc PROTO((long));
DL_CACHE_S    *get_first_dl_of_adrbk PROTO((int, DL_CACHE_S *));
DL_CACHE_S    *get_global_bottom_dlc PROTO((DL_CACHE_S *));
DL_CACHE_S    *get_global_top_dlc PROTO((DL_CACHE_S *));
int            get_line_of_message PROTO((STORE_S *, char *, int));
char          *getaltcharset PROTO ((char *, char **, char **));
int            grab_addrs_from_body PROTO((MAILSTREAM *, long, BODY *,
								    TA_S **));
void           init_ab_if_needed PROTO((void));
int            init_addrbooks PROTO((OpenStatus, int, int, int));
void           init_abook PROTO((PerAddrBook *, OpenStatus));
void           init_disp_form PROTO((PerAddrBook *, char **, int));
void           initialize_dlc_cache PROTO((void));
void           internal_take PROTO((AdrBk *, long));
int            is_addr PROTO((long));
int            is_empty PROTO((long));
int            is_talist_of_one PROTO((TA_S *));
int            line_is_selectable PROTO((long));
char         **list_of_checked PROTO((TA_S *));
char          *listmem PROTO((long));
adrbk_cntr_t   listmem_count_from_abe PROTO((AdrBk_Entry *));
char          *listmem_from_dl PROTO((AdrBk *, AddrScrn_Disp *));
int            matching_dlcs PROTO((DL_CACHE_S *, DL_CACHE_S *));
TA_S          *new_taline PROTO((TA_S **));
int            next_selectable_line PROTO((long, long *));
TA_S          *next_sel_taline PROTO((TA_S *));
TA_S          *next_taline PROTO((TA_S *));
int            nickname_check PROTO((char *, char **));
void           no_tabs_warning PROTO((void));
int            our_build_address PROTO((BuildTo, char **, char **, char **,
								    int, int));
void           paint_line PROTO((int, long, int, Pos *));
void           parse_format PROTO((char *, COL_S *));
char          *pico_cancel_for_adrbk_edit PROTO((void));
char          *pico_cancel_for_adrbk_take PROTO((void));
char          *pico_cancelexit_for_adrbk PROTO((char *));
char          *pico_sendexit_for_adrbk PROTO((void));
int            prev_selectable_line PROTO((long, long *));
TA_S          *pre_sel_taline PROTO((TA_S *));
TA_S          *pre_taline PROTO((TA_S *));
int            process_special_abook_attachments PROTO((MAILSTREAM *, long,
						    BODY *, BODY *, TA_S **));
void           readonly_warning PROTO((int, char *));
void           redraw_addr_screen PROTO((void));
void           restore_state PROTO((SAVE_STATE_S *));
void           rfc822_write_address_decode PROTO((char *, ADDRESS *, char **));
ADDRESS       *abe_to_address PROTO((AdrBk_Entry *, AddrScrn_Disp *,
				     AdrBk *, int *));
char          *abe_to_nick_or_addr_string PROTO((AdrBk_Entry *,
						 AddrScrn_Disp *, AdrBk *));
void           save_state PROTO((SAVE_STATE_S *));
int            search_book PROTO((long, int, long *, int *, int *));
int            search_in_one_line PROTO((AddrScrn_Disp *, AdrBk_Entry *, char *,
								char *));
PerAddrBook   *setup_for_addrbook_add PROTO((SAVE_STATE_S *, int));
void           strip_personal_quotes PROTO((ADDRESS *));
int            ta_take_marked_addrs PROTO((int, TA_S *, int));
int            ta_take_single_addr PROTO((TA_S *, int));
int            ta_mark_all PROTO((TA_S *));
int            ta_unmark_all PROTO((TA_S *));
void           take_to_addrbooks PROTO((char **, char *, char *, char *, char *,
								char *, int));
void           take_to_addrbooks_frontend PROTO((char **, char *, char *,
						 char *, char *, char *, int));
void           takeaddr_screen PROTO((struct pine *, TA_S *, int,
					TakeAddrScreenMode));
void           takeaddr_screen_redrawer_list PROTO((void));
void           takeaddr_screen_redrawer_single PROTO((void));
int            update_takeaddr_screen PROTO((struct pine *, TA_S *,
							TA_SCREEN_S *, Pos *));
PerAddrBook   *use_this_addrbook PROTO((int));
int            verify_addr PROTO((char *, char **, char **, BUILDER_ARG *));
int            verify_nick PROTO((char *, char **, char **, BUILDER_ARG *));
char          *view_message_for_pico PROTO((void));
void           warp_to_dlc PROTO((DL_CACHE_S *, long));
void           warp_to_beginning PROTO((void));
void           warp_to_end PROTO((void));
#ifdef	_WINDOWS
int	       addr_scroll_up PROTO((long));
int	       addr_scroll_down PROTO((long));
int	       addr_scroll_to_pos PROTO((long));
int	       addr_scroll_callback PROTO((int, long));
#endif


#define CLICKHERE       "[ Select Here to See Expanded List ]"
#define NO_PERMISSION   "[ Permission Denied ]"
#define EMPTY           "[ Empty ]"
#define READONLY        "(ReadOnly)"
#define NOACCESS        "(Un-readable)"
#define DISTLIST        "DISTRIBUTION LIST:"

#define MAX_FCC     MAX_ADDRESS
#define MAX_COMMENT (10000)

#define DING    1
#define NO_DING 0

/*
 * These constants are supposed to be suitable for use as longs where the longs
 * are representing a line number or message number.
 * These constants aren't suitable for use with type adrbk_cntr_t.  There is
 * a constant called NO_NEXT in adrbklib.h which you probably want for that.
 */
#define NO_LINE         (2147483645L)
#define CHANGED_CURRENT (NO_LINE + 1L)


/*
 * Make sure addrbooks are minimally initialized.
 */
void
init_ab_if_needed()
{
    dprint(9, (debugfile, "- init_ab_if_needed -\n"));

    if(!as.initialized)
      (void)init_addrbooks(Closed, 0, 0, 1);
}


/*
 * Sets everything up to get started.
 *
 * Args: want_status      -- The desired OpenStatus for all addrbooks.
 *       reset_to_top_or_bot -- Forget about the old location and put cursor
 *                           at top.  If value is 1, reset to top, if value
 *                           is -1, reset to bottom, else, don't reset.
 *       open_if_only_one -- If want_status is HalfOpen and there is only
 *                           one addressbook, then promote want_status to Open
 *       ro_warning       -- Issue ReadOnly warning if set, also sets global
 *
 * Return: number of addrbooks.
 */
int
init_addrbooks(want_status, reset_to_top_or_bot, open_if_only_one, ro_warning)
    OpenStatus want_status;
    int        reset_to_top_or_bot,
	       open_if_only_one,
	       ro_warning;
{
    register PerAddrBook *pab;
    char *q, **t;
    long line;

    dprint(2, (debugfile, "-- init_addrbooks(%s, %d, %d, %d) --\n",
		    want_status==Open ?
				"Open" :
				want_status==HalfOpen ?
					"HalfOpen" :
					want_status==NoDisplay ?
						"NoDisplay" :
						"Closed",
		    reset_to_top_or_bot, open_if_only_one, ro_warning));

    as.l_p_page = ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global)
					       - HEADER_ROWS(ps_global);

    /* already been initialized */
    if(as.n_addrbk){
	int i;

	as.ro_warning = ro_warning;

	/*
	 * Special case.  If there is only one addressbook we start the
	 * user out with that open, just like we did when there was always
	 * only one addressbook.
	 *
	 * Also start out open if expanded-view-of-addrbooks feature is set.
	 */
	if(want_status == HalfOpen &&
	   ((open_if_only_one && as.n_addrbk == 1) ||
	    F_ON(F_EXPANDED_ADDRBOOKS, ps_global)))
	    want_status = Open;

	/* open to correct state */
	for(i = 0; i < as.n_addrbk; i++)
	  init_abook(&as.adrbks[i], want_status);

	if(reset_to_top_or_bot == 1)
	  warp_to_beginning();
	else if(reset_to_top_or_bot == -1)
	  warp_to_end();

	if(reset_to_top_or_bot == 1){
	    as.top_ent     = 0L;
	    line           = first_selectable_line(0L);
	    if(line == NO_LINE)
	      as.cur_row = 0L;
	    else
	      as.cur_row = line;

	    if(as.cur_row >= as.l_p_page)
	      as.top_ent += (as.cur_row - as.l_p_page + 1);

	    as.old_cur_row = as.cur_row;
	}
	else if(reset_to_top_or_bot == -1){
	    as.top_ent     = first_line(0L - (long)as.l_p_page/2L);
	    line           = first_selectable_line(as.top_ent);
	    if(line == NO_LINE)
	      as.cur_row = 0L;
	    else
	      as.cur_row = line - as.top_ent;

	    if(as.cur_row >= as.l_p_page)
	      as.top_ent += (as.cur_row - as.l_p_page + 1);

	    as.old_cur_row = as.cur_row;
	}

	dprint(9, (debugfile, "init_addrbooks: already initialized: %d books\n",
				    as.n_addrbk));
        return(as.n_addrbk);
    }

    /* there are no addrbooks */
    if(as.initialized && !as.n_addrbk)
      return 0;

    as.initialized = 1;

    as.ro_warning = ro_warning;
    as.no_op_possbl = 0;


    if((!ps_global->VAR_GLOB_ADDRBOOK ||
        !ps_global->VAR_GLOB_ADDRBOOK[0] ||
        !ps_global->VAR_GLOB_ADDRBOOK[0][0]) &&
       (!ps_global->VAR_ADDRESSBOOK ||
        !ps_global->VAR_ADDRESSBOOK[0] ||
        !ps_global->VAR_ADDRESSBOOK[0][0]))
	return 0;
    

    /* count addressbooks */
    as.how_many_personals = 0;
    if(ps_global->VAR_ADDRESSBOOK &&
       ps_global->VAR_ADDRESSBOOK[0] &&
       ps_global->VAR_ADDRESSBOOK[0][0])
	for(t = ps_global->VAR_ADDRESSBOOK; *t != NULL; t++)
	  as.how_many_personals++;

    as.n_addrbk = as.how_many_personals;
    if(ps_global->VAR_GLOB_ADDRBOOK &&
       ps_global->VAR_GLOB_ADDRBOOK[0] &&
       ps_global->VAR_GLOB_ADDRBOOK[0][0])
	for(t = ps_global->VAR_GLOB_ADDRBOOK; *t != NULL; t++)
	  as.n_addrbk++;

    if(want_status == HalfOpen &&
       ((open_if_only_one && as.n_addrbk == 1) ||
	F_ON(F_EXPANDED_ADDRBOOKS, ps_global)))
	want_status = Open;

    as.cur      = 0;
    as.top_ent = 0L;
    as.cur_row = 0L;

    /*
     * allocate array of PerAddrBooks
     * (we don't give this up until we exit Pine, but it's small)
     */
    as.adrbks       = (PerAddrBook *)fs_get(as.n_addrbk * sizeof(PerAddrBook));
    memset((void *)as.adrbks, 0, as.n_addrbk * sizeof(PerAddrBook));

    /* init PerAddrBook data */
    for(as.cur = 0; as.cur < as.n_addrbk; as.cur++){
	char *nickname = NULL,
	     *filename = NULL;

	if(as.cur < as.how_many_personals)
	  q = ps_global->VAR_ADDRESSBOOK[as.cur];
	else
	  q = ps_global->VAR_GLOB_ADDRBOOK[as.cur - as.how_many_personals];

	pab = &as.adrbks[as.cur];
	
	/* Parse entry for optional nickname and filename */
	get_pair(q, &nickname, &filename, 0);

        strcpy(tmp_20k_buf, filename);
	fs_give((void **)&filename);

        filename = tmp_20k_buf;
	if(nickname == NULL)
	  pab->nickname = cpystr(filename);
	else
	  pab->nickname = nickname;

	if(*filename == '~')
	  fnexpand(filename, 20000);

	if(is_absolute_path(filename)){
	    pab->filename = cpystr(filename); /* fully qualified */
	}
	else{
	    char book_path[MAXPATH+1];
	    char *lc = last_cmpnt(ps_global->pinerc);

	    book_path[0] = '\0';
	    if(lc != NULL){
		strncpy(book_path, ps_global->pinerc, lc - ps_global->pinerc);
		book_path[lc - ps_global->pinerc] = '\0';
	    }

	    strcat(book_path, filename);
	    pab->filename = cpystr(book_path);
	}

	if(as.cur < as.how_many_personals)
	  pab->type  = LocalPersonal;
	else
	  pab->type  = LocalGlobal;

	pab->access = adrbk_access(pab->filename);

	/* global address books are forced readonly */
	if(as.cur >= as.how_many_personals && pab->access != NoAccess)
	  pab->access = ReadOnly;

	pab->ostatus  = TotallyClosed;

	if(as.ro_warning &&
	    open_if_only_one &&
	    as.n_addrbk == 1 &&
	    want_status == Open){

	    if(pab->access == ReadOnly)
	      readonly_warning(NO_DING, NULL);
	    else if(pab->access == NoAccess)
	      q_status_message(SM_ORDER, 0, 4,
		    "AddressBook not accessible, permission denied");
	}

	/*
	 * and remember that the memset above initializes everything
	 * else to 0
	 */

	init_abook(pab, want_status);
    }

    /*
     * Have to reset_to_top in this case since this is the first open,
     * regardless of the value of the argument, since these values haven't been
     * set before here.
     */
    warp_to_beginning();
    as.top_ent     = 0L;
    line           = first_selectable_line(0L);
    if(line == NO_LINE)
      as.cur_row = 0L;
    else
      as.cur_row = line;

    if(as.cur_row >= as.l_p_page)
      as.top_ent += (as.cur_row - as.l_p_page + 1);

    as.old_cur_row = as.cur_row;

    return(as.n_addrbk);
}


/*
 * Might help a little to debug problems.
 */
void
dump_some_debugging(message)
    char *message;
{
#ifdef DEBUG
    dprint(2, (debugfile, "- dump_some_debugging(%s) -\n", message));
    dprint(2, (debugfile, "initialized %d n_addrbk %d cur_row %d\n",
	as.initialized, as.n_addrbk, as.cur_row));
    dprint(2, (debugfile, "top_ent %ld ro_warning %d no_op_possbl %d\n",
	as.top_ent, as.ro_warning, as.no_op_possbl));
#endif /* DEBUG */
}


void
init_disp_form(pab, list, addrbook_num)
PerAddrBook *pab;
char       **list;
int	     addrbook_num;
{
    char *last_one;
    int   column = 0;

    memset((void *)pab->disp_form, 0, sizeof(COL_S));
    pab->disp_form[1].wtype = WeCalculate; /* so we don't get false AllAuto */

    if(as.checkboxes){
	pab->disp_form[column].wtype     = Fixed;
	pab->disp_form[column].req_width = 3;
	pab->disp_form[column++].type    = Checkbox;
    }

    /* if custom format is specified */
    if(list && list[0] && list[0][0]){
	/* find the one for addrbook_num */
	for(last_one = *list;
	    *list != NULL && addrbook_num;
	    addrbook_num--,list++)
	  last_one = *list;

	/* If not enough to go around, last one repeats */
	if(*list == NULL)
	  parse_format(last_one, &(pab->disp_form[column]));
	else
	  parse_format(*list, &(pab->disp_form[column]));
	
	if(column == 0 && pab->disp_form[0].wtype == AllAuto)
	  pab->disp_form[1].wtype = AllAuto;
    }
    else{  /* default */
	/* If 2nd wtype is AllAuto, the widths are calculated old way */
	pab->disp_form[1].wtype   = AllAuto;

	pab->disp_form[column++].type  = Nickname;
	pab->disp_form[column++].type  = Fullname;
	pab->disp_form[column++].type  = Addr;
	/* Fill in rest */
	while(column < NFIELDS)
	  pab->disp_form[column++].type = Notused;
    }
}


struct parse_tokens {
    char *name;
    ColumnType ctype;
};

struct parse_tokens ptokens[] = {
    {"NICKNAME", Nickname},
    {"FULLNAME", Fullname},
    {"ADDRESS",  Addr},
    {"FCC",      Filecopy},
    {"COMMENT",  Comment},
    {"DEFAULT",  Def},
    {NULL,       Notused}
};
/*
 * Parse format_str and fill in disp_form structure based on what's there.
 *
 * Args: format_str -- The format string from pinerc.
 *        disp_form -- This is where we fill in the answer.
 *
 * The format string consists of special tokens which give the order of
 * the columns to be displayed.  The possible tokens are NICKNAME,
 * FULLNAME, ADDRESS, FCC, COMMENT.  If a token is followed by
 * parens with an integer inside (FULLNAME(16)) then that means we
 * make that variable that many characters wide.  If it is a percentage, we
 * allocate that percentage of the columns to that variable.  If no
 * parens, that means we calculate it for the user.  The tokens are
 * delimited by white space.  A token of DEFAULT means to calculate the
 * whole thing as we would if no spec was given.  This makes it possible
 * to specify default for one addrbook and something special for another.
 */
void
parse_format(format_str, disp_form)
char  *format_str;
COL_S *disp_form;
{
    int column = 0;
    char *p, *q;
    struct parse_tokens *pt;
    int nicknames, fullnames, addresses, not_allauto;
    int warnings = 0;

    p = format_str;
    while(p && *p && column < NFIELDS){
	/* skip leading white space for next word */
	while(p && *p && isspace((unsigned char)(*p)))
	  p++;
    
	/* look for the ptoken this word matches */
	for(pt = ptokens; pt->name; pt++)
	    if(!struncmp(pt->name, p, strlen(pt->name)))
	      break;
	
	/* ignore unrecognized word */
	if(!pt->name){
	    char *r;

	    if((r=strindex(p, SPACE)) != NULL)
	      *r = '\0';

	    dprint(2, (debugfile, "parse_format: ignoring unrecognized word \"%s\" in address-book-formats\n", p));
	    q_status_message1(SM_ORDER, warnings++==0 ? 1 : 0, 4,
		"Ignoring unrecognized word \"%s\" in address-book-formats", p);
	    /* put back space */
	    if(r)
	      *r = SPACE;

	    /* skip unrecognized word */
	    while(p && *p && !isspace((unsigned char)(*p)))
	      p++;

	    continue;
	}

	disp_form[column].type = pt->ctype;

	/* skip over name and look for parens */
	p += strlen(pt->name);
	if(*p == '('){
	    p++;
	    q = p;
	    while(p && *p && isdigit((unsigned char)*p))
	      p++;
	    
	    if(p && *p && *p == ')' && p > q){
		disp_form[column].wtype = Fixed;
		disp_form[column].req_width = atoi(q);
	    }
	    else if(p && *p && *p == '%' && p > q){
		disp_form[column].wtype = Percent;
		disp_form[column].req_width = atoi(q);
	    }
	    else{
		disp_form[column].wtype = WeCalculate;
		if(disp_form[column].type == Nickname)
		  disp_form[column].req_width = 8;
		else
		  disp_form[column].req_width = 3;
	    }
	}
	else{
	    disp_form[column].wtype     = WeCalculate;
	    if(disp_form[column].type == Nickname)
	      disp_form[column].req_width = 8;
	    else
	      disp_form[column].req_width = 3;
	}

	if(disp_form[column].type == Def){
	    /* If any type is _DEFAULT_, the widths are calculated old way */
assign_default:
	    column = 0;
	    disp_form[1].wtype  = AllAuto;

	    disp_form[column++].type = Nickname;
	    disp_form[column++].type = Fullname;
	    disp_form[column++].type = Addr;
	    /* Fill in rest */
	    while(column < NFIELDS)
	      disp_form[column++].type = Notused;

	    return;
	}

	column++;
	/* skip text at end of word */
	while(p && *p && !isspace((unsigned char)(*p)))
	  p++;
    }

    if(column == 0){
	q_status_message(SM_ORDER, 0, 4,
	"address-book-formats has no recognizable words, using default format");
	goto assign_default;
    }

    /* Fill in rest */
    while(column < NFIELDS)
      disp_form[column++].type = Notused;

    /* check to see if user is just re-ordering default fields */
    nicknames = 0;
    fullnames = 0;
    addresses = 0;
    not_allauto = 0;
    for(column = 0; column < NFIELDS; column++){
	if(disp_form[column].type != Notused
	   && disp_form[column].wtype != WeCalculate)
	  not_allauto++;

	switch(disp_form[column].type){
	  case Nickname:
	    nicknames++;
	    break;

	  case Fullname:
	    fullnames++;
	    break;

	  case Addr:
	    addresses++;
	    break;

	  case Filecopy:
	  case Comment:
	    not_allauto++;
	    break;
	}
    }

    /*
     * Special case: if there is no address field specified, we put in
     * a special field called WhenNoAddrDisplayed, which causes list
     * entries to be displayable in all cases.
     */
    if(!addresses){
	for(column = 0; column < NFIELDS; column++)
	  if(disp_form[column].type == Notused)
	    break;
	
	if(column < NFIELDS){
	    disp_form[column].type  = WhenNoAddrDisplayed;
	    disp_form[column].wtype = Special;
	}
    }

    if(nicknames == 1 && fullnames == 1 && addresses == 1 && not_allauto == 0)
      disp_form[0].wtype = AllAuto; /* set to do default widths */
}


void
dump_a_dlc_to_debug(message, dlc)
    char *message;
    DL_CACHE_S *dlc;
{
#ifdef DEBUG
    char type[20];

    switch(dlc->type){
      case DlcTitleBlankTop:
	(void)strcpy(type, "TitleBlankTop");
	break;
      case DlcTitleDashTop:
	(void)strcpy(type, "TitleDashTop");
	break;
      case DlcTitle:
	(void)strcpy(type, "Title");
	break;
      case DlcTitleDashBottom:
	(void)strcpy(type, "TitleDashBottom");
	break;
      case DlcTitleBlankBottom:
	(void)strcpy(type, "TitleBlankBottom");
	break;
      case DlcClickHere:
	(void)strcpy(type, "ClickHere");
	break;
      case DlcListClickHere:
	(void)strcpy(type, "ListClickHere");
	break;
      case DlcListEmpty:
	(void)strcpy(type, "ListEmpty");
	break;
      case DlcEmpty:
	(void)strcpy(type, "Empty");
	break;
      case DlcNoPermission:
	(void)strcpy(type, "NoPermission");
	break;
      case DlcSimple:
	(void)strcpy(type, "Simple");
	break;
      case DlcListHead:
	(void)strcpy(type, "ListHead");
	break;
      case DlcListEnt:
	(void)strcpy(type, "ListEnt");
	break;
      case DlcListBlankTop:
	(void)strcpy(type, "ListBlankTop");
	break;
      case DlcListBlankBottom:
	(void)strcpy(type, "ListBlankBottom");
	break;
      case DlcNotSet:
	(void)strcpy(type, "NotSet");
	break;
      case DlcBeginning:
	(void)strcpy(type, "Beginning");
	break;
      case DlcOneBeforeBeginning:
	(void)strcpy(type, "OneBeforeBeginning");
	break;
      case DlcTwoBeforeBeginning:
	(void)strcpy(type, "TwoBeforeBeginning");
	break;
      case DlcEnd:
	(void)strcpy(type, "End");
	break;
    }

    dprint(2, (debugfile,
	"%s: type %s adrbk_num %d\n",
	message, type, (int)dlc->adrbk_num));
    dprint(2, (debugfile,
	"   global_row %ld dlcelnum %ld dlcoffset %ld\n",
	(long)dlc->global_row, (long)dlc->dlcelnum, (long)dlc->dlcoffset));

#endif /* DEBUG */
}


/*
 * Create addrbook_file.lu lookup file and exit.  This is for
 * use as a stand-alone creator of .lu files.
 */
void
just_update_lookup_file(addrbook_file, sort_rule_descrip)
    char *addrbook_file;
    char *sort_rule_descrip;
{
    int        i;
    int        sort_rule;
    NAMEVAL_S *v;
    AdrBk     *ab;
    char       warning[800];


    sort_rule = -1;
    for(i = 0; v = ab_sort_rules(i); i++){
       if(!strucmp(sort_rule_descrip, v->name)){
	   sort_rule = v->value;
	   break;
       }
    }

    if(sort_rule == -1){
	fprintf(stderr, "Sort rule %s unknown\n", sort_rule_descrip);
	exit(-1);
    }

    warning[0] = '\0';
    ab = adrbk_open(addrbook_file, NULL, warning, sort_rule, 1, 1);
    if(ab == NULL){
	if(*warning)
	  fprintf(stderr, "%s: %s\n", addrbook_file, warning);
	else
	  fprintf(stderr, "%s: %s\n",
		addrbook_file, error_description(errno));

	exit(-1);
    }

    if(!adrbk_is_in_sort_order(ab, 1)){
	adrbk_set_nominal_cachesize(ab, (long)adrbk_count(ab));
	(void)adrbk_sort(ab, (a_c_arg_t)0, (adrbk_cntr_t *)NULL, 1);
    }

    exit(0);
}


/*
 * Something was changed in options screen, so need to start over.
 */
void
addrbook_reset()
{
    dprint(2, (debugfile, "- addrbook_reset -\n"));
    completely_done_with_adrbks();
}


/*
 * Returns type of access allowed on this addrbook.
 */
AccessType
adrbk_access(filename)
    char *filename;
{
    char       fbuf[501];
    AccessType access = NoExists;

    build_path(fbuf, is_absolute_path(filename) ? NULL : ps_global->home_dir,
	       filename);

#if defined(DOS) || defined(OS2)
    /*
     * Microsoft networking causes some access calls to do a DNS query (!!!)
     * when it is turned on.  In particular, if there is a / in the filename,
     * this seems to happen.  So, just return does not exist in that case.
     */
    if(strindex(fbuf, '/') != NULL){
	dprint(2, (debugfile, "\"/\" not allowed in addrbook name\n"));
	return NoAccess;
    }
#else /* !DOS */
    /* also prevent backslash in non-DOS addrbook names */
    if(strindex(fbuf, '\\') != NULL){
	dprint(2, (debugfile, "\"\\\" not allowed in addrbook name\n"));
	return NoAccess;
    }
#endif /* !DOS */

    if(can_access(fbuf, ACCESS_EXISTS) == 0){
	if(can_access(fbuf, EDIT_ACCESS) == 0){
	    char *dir, *p;

	    dir = ".";
	    if(p = last_cmpnt(fbuf)){
		*--p = '\0';
		dir  = *fbuf ? fbuf : "/";
	    }

#if	defined(DOS) || defined(OS2)
	    /*
	     * If the dir has become a drive letter and : (e.g. "c:")
	     * then append a "\".  The library function access() in the
	     * win 16 version of MSC seems to require this.
	     */
	    if(isalpha((unsigned char) *dir)
	       && *(dir+1) == ':' && *(dir+2) == '\0'){
		*(dir+2) = '\\';
		*(dir+3) = '\0';
	    }
#endif	/* DOS || OS2 */

	    /*
	     * Even if we can edit the address book file itself, we aren't
	     * going to be able to change it unless we can also write in
	     * the directory that contains it (because we write into a
	     * temp file and then rename).
	     */
	    if(can_access(dir, EDIT_ACCESS) == 0)
	      access = ReadWrite;
	    else
	      access = ReadOnly;
	}
	else if(can_access(fbuf, READ_ACCESS) == 0)
	  access = ReadOnly;
	else
	  access = NoAccess;
    }
    
    return(access);
}


/*
 * Returns the index of the current address book.
 */
int
cur_addr_book()
{
    return(adrbk_num_from_lineno(as.top_ent + as.cur_row));
}


/*
 * Returns the index of the current address book.
 */
int
adrbk_num_from_lineno(lineno)
    long lineno;
{
    DL_CACHE_S *dlc;

    dlc = get_dlc(lineno);

    return(dlc->adrbk_num);
}


void
end_adrbks()
{
    int i;

    dprint(2, (debugfile, "- end_adrbks -\n"));

    if(!as.initialized)
      return;

    for(i = 0; i < as.n_addrbk; i++)
      init_abook(&as.adrbks[i], Closed);
}


/*
 * Free and close everything.
 */
void
completely_done_with_adrbks()
{
    register PerAddrBook *pab;
    int i;

    dprint(2, (debugfile, "- completely_done_with_adrbks -\n"));

    if(!as.initialized)
      return;

    for(i = 0; i < as.n_addrbk; i++)
      init_abook(&as.adrbks[i], TotallyClosed);

    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];

	if(pab->filename)
	  fs_give((void **)&pab->filename);

	if(pab->nickname)
	  fs_give((void **)&pab->nickname);
    }

    done_with_dlc_cache();

    if(as.adrbks)
      fs_give((void **)&as.adrbks);

    as.n_addrbk    = 0;
    as.initialized = 0;
}


/*
 * Save the screen state and the Open or Closed status of the addrbooks.
 */
void
save_state(state)
    SAVE_STATE_S *state;
{
    int                 i;
    DL_CACHE_S         *dlc;

    dprint(9, (debugfile, "- save_state -\n"));

    if(as.n_addrbk == 0)
      return;

    /* allocate space for saving the screen structure and save it */
    state->savep    = (AddrScrState *)fs_get(sizeof(AddrScrState));
    *(state->savep) = as; /* copy the struct */


    /* allocate space for saving the ostatus for each addrbook */
    state->stp = (OpenStatus *)fs_get(as.n_addrbk * sizeof(OpenStatus));

    for(i = 0; i < as.n_addrbk; i++)
      (state->stp)[i] = as.adrbks[i].ostatus;


    state->dlc_to_warp_to = (DL_CACHE_S *)fs_get(sizeof(DL_CACHE_S));
    dlc = get_dlc(as.top_ent + as.cur_row);
    *(state->dlc_to_warp_to) = *dlc; /* copy the struct */
}


/*
 * Restore the state.
 *
 * Side effect: Flushes addrbook entry cache entries so they need to be
 * re-fetched afterwords.  This only applies to entries obtained since
 * the call to save_state.
 * Also flushes all dlc cache entries, so dlist calls need to be repeated.
 */
void
restore_state(state)
    SAVE_STATE_S *state;
{
    int i;

    dprint(9, (debugfile, "- restore_state -\n"));

    if(as.n_addrbk == 0)
      return;

    as = *(state->savep);  /* put back cur_row and all that */

    /* restore addressbook OpenStatus to what it was before */
    for(i = 0; i < as.n_addrbk; i++){
	init_disp_form(&as.adrbks[i], ps_global->VAR_ABOOK_FORMATS, i);
	init_abook(&as.adrbks[i], (state->stp)[i]);
    }

    /*
     * jump cache back to where we were
     */
    warp_to_dlc(state->dlc_to_warp_to, as.top_ent+as.cur_row);

    fs_give((void **)&state->dlc_to_warp_to);
    fs_give((void **)&state->stp);
    fs_give((void **)&state->savep);
}


/*
 * Initialize or re-initialize this address book.
 *
 *  Args: pab        -- the PerAddrBook ptr
 *       want_status -- desired OpenStatus for this address book
 */
void
init_abook(pab, want_status)
    PerAddrBook *pab;
    OpenStatus   want_status;
{
    register OpenStatus new_status;

    dprint(7, (debugfile, "- init_abook -\n"));
    dprint(7, (debugfile,
	    "    addrbook nickname = %s filename = %s want ostatus %s\n",
		pab->nickname ? pab->nickname : "<null>",
		pab->filename ? pab->filename : "<null>",
		want_status==Open ?
			    "Open" :
			    want_status==HalfOpen ?
				    "HalfOpen" :
				    want_status==NoDisplay ?
					    "NoDisplay" :
					    want_status==Closed ?
						"Closed" :
						"TotallyClosed"));
    dprint(7, (debugfile, "    ostatus was %s, want %s\n",
		pab->ostatus==Open ?
			    "Open" :
			    pab->ostatus==HalfOpen ?
				    "HalfOpen" :
				    pab->ostatus==NoDisplay ?
					    "NoDisplay" :
					pab->ostatus==Closed ?
					    "Closed" :
					    "TotallyClosed",
		want_status==Open ?
			    "Open" :
			    want_status==HalfOpen ?
				    "HalfOpen" :
				    want_status==NoDisplay ?
					    "NoDisplay" :
					    want_status==Closed ?
						"Closed" :
						"TotallyClosed"));

    new_status = want_status;  /* optimistic default */

    if(want_status == TotallyClosed && pab->address_book != NULL){
	adrbk_close(pab->address_book);
	pab->address_book = NULL;
    }
    /*
     * If we don't need it, release some addrbook memory by calling
     * adrbk_partial_close().
     */
    else if((want_status == Closed || want_status == HalfOpen) &&
	pab->address_book != NULL){
	adrbk_partial_close(pab->address_book);
    }
    /* If we want the addrbook read in and it hasn't been, do so */
    else if((want_status == Open || want_status == NoDisplay) &&
	pab->address_book == NULL){
	if(pab->access != NoAccess){
	    char warning[800]; /* place to put a warning */
	    int sort_rule;
	    int force_not_valid = 0;

	    warning[0] = '\0';
	    if(pab->access == ReadOnly)
		sort_rule = AB_SORT_RULE_NONE;
	    else
		sort_rule = ps_global->ab_sort_rule;

	    if(trouble_filename){
		if(pab->filename)
		  force_not_valid = 
			       strcmp(trouble_filename, pab->filename) == 0;
		else
		  force_not_valid = *trouble_filename == '\0';

		if(force_not_valid){
#ifdef notdef
		    /*
		     * I go back and forth on whether or not this should
		     * be here.  If the sys admin screws up and copies the
		     * wrong .lu file into place where a large global
		     * addressbook is, do we want it to rebuild for all
		     * the users or not?  With this commented out we're
		     * rebuilding for everybody, just like we would for
		     * a personal addressbook.  Most likely that will mean
		     * that it gets rebuilt in /tmp for each current user.
		     * That's what I'm going with for now.
		     */
		    if(pab->type == LocalGlobal)
		      force_not_valid = 0;
#endif /* notdef */
		    fs_give((void **)&trouble_filename);
		}
	    }

	    pab->address_book = adrbk_open(pab->filename,
		   ps_global->home_dir, warning, sort_rule, 0, force_not_valid);

	    if(pab->address_book == NULL){
		pab->access = NoAccess;
		if(want_status == Open){
		    new_status = HalfOpen;  /* best we can do */
		    q_status_message1(SM_ORDER | SM_DING, *warning?1:3, 4,
				      "Error opening/creating address book %s",
				      pab->nickname);
		    if(*warning)
			q_status_message2(SM_ORDER, 3, 4, "%s: %s",
			    as.n_addrbk > 1 ? pab->nickname : "addressbook",
			    warning);
		}
		else
		    new_status  = Closed;

		dprint(1, (debugfile, "Error opening address book %s: %s\n",
			  pab->nickname, error_description(errno)));
	    }
	    else{
		if(pab->access == NoExists)
		  pab->access = ReadWrite;

		/* 200 is out of the blue */
		(void)adrbk_set_nominal_cachesize(pab->address_book,
		    min((long)adrbk_count(pab->address_book), 200L));

		if(pab->access == ReadWrite){
#if !(defined(DOS) && !defined(_WINDOWS))
		    long old_size;
#endif /* !DOS */

		    /*
		     * Add forced entries if there are any.  These are
		     * entries that are always supposed to show up in
		     * personal address books.  They're specified in the
		     * global config file.
		     */
		    add_forced_entries(pab->address_book);

		    /*
		     * For huge addrbooks, it really pays if you can make
		     * them read-only so that you skip adrbk_is_in_sort_order.
		     */
		    if(!adrbk_is_in_sort_order(pab->address_book, 0)){
/* DOS sorts will be very slow on large addrbooks */
#if !(defined(DOS) && !defined(_WINDOWS))
			old_size =
			    adrbk_set_nominal_cachesize(pab->address_book,
			    (long)adrbk_count(pab->address_book));
#endif /* !DOS */
			(void)adrbk_sort(pab->address_book,
			    (a_c_arg_t)0, (adrbk_cntr_t *)NULL, 0);
#if !(defined(DOS) && !defined(_WINDOWS))
			(void)adrbk_set_nominal_cachesize(pab->address_book,
			    old_size);
#endif /* !DOS */
		    }
		}

		new_status = want_status;
		dprint(1,
		      (debugfile,
		      "Address book %s (%s) opened with %ld items\n",
		       pab->nickname, pab->filename,
		       (long)adrbk_count(pab->address_book)));
		if(*warning){
		    dprint(1, (debugfile,
				 "Addressbook parse error in %s (%s): %s\n",
				 pab->nickname, pab->filename, warning));
		    if(!pab->gave_parse_warnings && want_status == Open){
			pab->gave_parse_warnings++;
			q_status_message2(SM_ORDER, 3, 4, "%s: %s",
			    as.n_addrbk > 1 ? pab->nickname : "addressbook",
			    warning);
		    }
		}
	    }
	}
	else{
	    if(want_status == Open){
		new_status = HalfOpen;  /* best we can do */
		q_status_message1(SM_ORDER | SM_DING, 3, 4,
		   "Insufficient permissions for opening address book %s",
		   pab->nickname);
	    }
	    else
	      new_status = Closed;
	}
    }

    pab->ostatus = new_status;
}


/*
 * Returns the addrbook entry for this display row.
 */
AdrBk_Entry *
ae(row)
    long row;
{
    PerAddrBook *pab;
    LineType type;
    AddrScrn_Disp *dl;

    dl = dlist(row);
    type = dl->type;
    if(!(type == Simple || type == ListHead ||
         type == ListEnt || type == ListClickHere))
      return((AdrBk_Entry *)NULL);

    pab = &as.adrbks[adrbk_num_from_lineno(row)];

    return(adrbk_get_ae(pab->address_book, (a_c_arg_t)dl->elnum, Normal));
}


/*
 * Returns a pointer to the member_number'th list member of the list
 * associated with this display line.
 */
char *
listmem(row)
    long row;
{
    PerAddrBook *pab;
    AddrScrn_Disp *dl;

    dl = dlist(row);
    if(dl->type != ListEnt)
      return((char *)NULL);

    pab = &as.adrbks[adrbk_num_from_lineno(row)];

    return(listmem_from_dl(pab->address_book, dl));
}


/*
 * Returns a pointer to the list member
 * associated with this display line.
 */
char *
listmem_from_dl(address_book, dl)
    AdrBk         *address_book;
    AddrScrn_Disp *dl;
{
    AdrBk_Entry *abe;
    char **p = (char **)NULL;

    /* This shouldn't happen */
    if(dl->type != ListEnt)
      return((char *)NULL);

    abe = adrbk_get_ae(address_book, (a_c_arg_t)dl->elnum, Normal);

    /*
     * If we wanted to be more careful, We'd go through the list making sure
     * we don't pass the end.  We'll count on the caller being careful
     * instead.
     */
    if(abe && abe->tag == List){
	p =  abe->addr.list;
	p += dl->l_offset;
    }

    return((p && *p) ? *p : (char *)NULL);
}


DL_CACHE_S *
dlc_from_listmem(address_book, elem, member_addr)
    AdrBk       *address_book;
    a_c_arg_t    elem;
    char        *member_addr;
{
    AdrBk_Entry *abe;
    char **p;
    static DL_CACHE_S dlc;

    abe = adrbk_get_ae(address_book, elem, Normal);

    if(abe->tag == List){
	for(p = abe->addr.list; *p; p++){
	    if(strcmp(*p, member_addr) == 0)
	      break;
	}
    }

    dlc.type      = DlcListEnt;
    dlc.dlcelnum  = (adrbk_cntr_t)elem;
    if(abe->tag == List)
      dlc.dlcoffset = p - abe->addr.list;
    else
      dlc.dlcoffset = 0;

    return(&dlc);
}


/*
 * How many members in list?
 */
adrbk_cntr_t
listmem_count_from_abe(abe)
    AdrBk_Entry *abe;
{
    char **p;

    if(abe->tag != List)
      return 0;

    for(p = abe->addr.list; p != NULL && *p != NULL; p++)
      ;/* do nothing */
    
    return((adrbk_cntr_t)(p - abe->addr.list));
}


/*
 * Return a ptr to the row'th line of the global disp_list.
 * Line numbers count up but you can't count on knowing which line number
 * goes with the first or the last row.  That is, row 0 is not necessarily
 * special.  It could be before the rows that make up the display list, after
 * them, or anywhere in between.  You can't tell what the last row is
 * numbered, but a dl with type End is returned when you get past the end.
 * You can't tell what the number of the first row is, but if you go past
 * the first row a dl of type Beginning will be returned.  Row numbers can
 * be positive or negative.  Their values have no meaning other than how
 * they line up relative to other row numbers.
 */
AddrScrn_Disp *
dlist(row)
    long row;
{
    DL_CACHE_S *dlc = (DL_CACHE_S *)NULL;

    dlc = get_dlc(row);

    if(dlc){
	fill_in_dl_field(dlc);
	return(&dlc->dl);
    }
    else{
	q_status_message(SM_ORDER | SM_DING, 5, 10,
		     "Bug in addrbook, not supposed to happen, re-syncing...");
	dprint(1,
	    (debugfile,
	"Bug in addrbook (null dlc in dlist(%ld), not supposed to happen\n",
	    row));
	/* jump back to a safe starting point */
	dump_some_debugging("panic_dlist");
	longjmp(addrbook_changed_unexpectedly, 1);
	/*NOTREACHED*/
    }
}


/*
 * This returns the actual dlc instead of the dl within the dlc.
 */
DL_CACHE_S *
get_dlc(row)
    long row;
{
    dprint(11, (debugfile, "- get_dlc(%ld) -\n", row));

    return(dlc_mgr(row, Lookup, (DL_CACHE_S *)NULL));
}


void
initialize_dlc_cache()
{
    dprint(11, (debugfile, "- initialize_dlc_cache -\n"));

    (void)dlc_mgr(NO_LINE, Initialize, (DL_CACHE_S *)NULL);
}


void
done_with_dlc_cache()
{
    dprint(9, (debugfile, "- done_with_dlc_cache -\n"));

    (void)dlc_mgr(NO_LINE, DoneWithCache, (DL_CACHE_S *)NULL);
}


/*
 * Move to new_dlc and give it row number row_number_to_assign_it.
 * We copy the passed in dlc in case the caller passed us a pointer into
 * the cache.
 */
void
warp_to_dlc(new_dlc, row_number_to_assign_it)
    DL_CACHE_S *new_dlc;
    long row_number_to_assign_it;
{
    DL_CACHE_S dlc;

    dprint(9, (debugfile, "- warp_to_dlc(%ld) -\n", row_number_to_assign_it));

    dlc = *new_dlc;

    (void)dlc_mgr(row_number_to_assign_it, ArbitraryStartingPoint, &dlc);
}


/*
 * Move to first dlc and give it row number 0.
 */
void
warp_to_beginning()
{
    dprint(9, (debugfile, "- warp_to_beginning -\n"));

    (void)dlc_mgr(0L, FirstEntry, (DL_CACHE_S *)NULL);
}


/*
 * Move to last dlc and give it row number 0.
 */
void
warp_to_end()
{
    dprint(9, (debugfile, "- warp_to_end -\n"));

    (void)dlc_mgr(0L, LastEntry, (DL_CACHE_S *)NULL);
}


/*
 * This flushes all of the cache that is related to this address book
 * entry, (the entry that this cache element refers to).  Or, if this doesn't
 * refer to an address book entry, it flushes this single cache line.
 */
void
flush_dlc_from_cache(dlc_to_flush)
    DL_CACHE_S *dlc_to_flush;
{
    dprint(11, (debugfile, "- flush_dlc_from_cache -\n"));

    (void)dlc_mgr(NO_LINE, FlushDlcFromCache, dlc_to_flush);
}


/*
 * Returns 1 if the dlc's match, 0 otherwise.
 */
int
matching_dlcs(dlc1, dlc2)
    DL_CACHE_S *dlc1, *dlc2;
{
    if(!dlc1 || !dlc2 ||
        dlc1->type != dlc2->type ||
	dlc1->adrbk_num != dlc2->adrbk_num)
	return 0;

    switch(dlc1->type){

      case DlcSimple:
      case DlcListHead:
      case DlcListBlankTop:
      case DlcListBlankBottom:
      case DlcListClickHere:
      case DlcListEmpty:
	return(dlc1->dlcelnum == dlc2->dlcelnum);

      case DlcListEnt:
	return(dlc1->dlcelnum  == dlc2->dlcelnum &&
	       dlc1->dlcoffset == dlc2->dlcoffset);

      case DlcTitleBlankTop:
      case DlcTitleDashTop:
      case DlcTitle:
      case DlcTitleDashBottom:
      case DlcTitleBlankBottom:
      case DlcClickHere:
      case DlcEmpty:
      case DlcNoPermission:
	return 1;

      case DlcNotSet:
      case DlcBeginning:
      case DlcOneBeforeBeginning:
      case DlcTwoBeforeBeginning:
      case DlcEnd:
	return 0;
    }
    /*NOTREACHED*/
}


/*
 * Compare two dlc's to see which comes later in the display list.
 *
 * THERE ARE BIG RESTRICTIONS!  First, dlc1 must be either a DlcSimple or a
 * DlcListHead!  Second, these aren't just any dlc's.  Dlc1 is a dlc
 * that is going to be inserted into the addrbook.  Therefore, it can have
 * the same element number as something already in the addrbook.  If it
 * has the same elnum as dlc2, then dlc2 will soon have an elnum that is
 * one higher when the cache is flushed.  So in that case, dlc1 is earlier
 * in the display list than dlc2.  But, if dlc1 has an elnum that is one higher
 * than dlc2's you don't want to decrease it because it is already correct.
 * In other words, you can't just subtract one from the elnum and then
 * do a regular compare.  Hence, this weirdo function.
 *
 * Actually, it still isn't quite right because it could be that it really
 * does have the same elnum.  For example, dlc2 could be a ListEnt that
 * is a member of dlc1, a ListHead.  We'll pretend that can't happen and
 * note that the consequences of it happening are not serious (display is
 * still correct).
 *
 * Returns > 0 if dlc1 >  dlc2 (comes later in display list),
 * Returns < 0 if dlc1 <  dlc2 (comes earlier in display list),
 * (We never call this with dlc1 == dlc2.)
 */
int
funny_compare_dlcs(dlc1, dlc2)
    DL_CACHE_S *dlc1, *dlc2;
{
    long test_elnum;

    if(!dlc2)
      return  1; /* arbitrarily, we shouldn't allow this to happen */

    if(!dlc1)
      return -1;

    switch(dlc2->type){

      case DlcBeginning:
      case DlcOneBeforeBeginning:
      case DlcTwoBeforeBeginning:
	return 1;

      case DlcEnd:
	return -1;
    }

    if(dlc1->adrbk_num != dlc2->adrbk_num)
      return(dlc1->adrbk_num - dlc2->adrbk_num);

    if(dlc1->type != DlcSimple && dlc1->type != DlcListHead)
      goto panic_abook_abort;

    switch(dlc2->type){

      case DlcSimple:
      case DlcListHead:
      case DlcListEnt:
      case DlcListBlankBottom:
      case DlcListBlankTop:
      case DlcListClickHere:
      case DlcListEmpty:
	/* this is the funny case discussed in the comments */
	test_elnum = dlc1->dlcelnum;
	if(test_elnum == dlc2->dlcelnum)
	  test_elnum--;

	return(test_elnum - dlc2->dlcelnum);

      case DlcTitleBlankTop:
      case DlcTitleDashTop:
      case DlcTitle:
      case DlcTitleDashBottom:
      case DlcTitleBlankBottom:
      case DlcNotSet:
	return 1;

      case DlcClickHere:
      case DlcEmpty:
      case DlcNoPermission:
      default:
	/* panic, not supposed to happen, fall through */
	break;
    }

panic_abook_abort:
    q_status_message(SM_ORDER | SM_DING, 5, 10,
		     "Bug in addrbook, not supposed to happen, re-syncing...");
    dprint(1,
	(debugfile,
	"Bug in addrbook, not supposed to happen, re-sync\n"));
    /* jump back to a safe starting point */
    dump_some_debugging("panic_abook_abort");
    dump_a_dlc_to_debug("dlc1", dlc1);
    dump_a_dlc_to_debug("dlc2", dlc2);
    longjmp(addrbook_changed_unexpectedly, 1);
    /*NOTREACHED*/
}


/*
 * Returns 1 if the dlc's are related to the same addrbook entry, 0 otherwise.
 */
int
dlcs_from_same_abe(dlc1, dlc2)
    DL_CACHE_S *dlc1, *dlc2;
{
    if(!dlc1 || !dlc2 || dlc1->adrbk_num != dlc2->adrbk_num)
      return 0;

    switch(dlc1->type){

      case DlcSimple:
      case DlcListHead:
      case DlcListBlankTop:
      case DlcListBlankBottom:
      case DlcListEnt:
      case DlcListClickHere:
      case DlcListEmpty:
	switch(dlc2->type){
	  case DlcTitleBlankTop:
	  case DlcTitleDashTop:
	  case DlcTitle:
	  case DlcTitleDashBottom:
	  case DlcTitleBlankBottom:
	  case DlcClickHere:
	  case DlcEmpty:
	  case DlcNoPermission:
	  case DlcNotSet:
	  case DlcBeginning:
	  case DlcOneBeforeBeginning:
	  case DlcTwoBeforeBeginning:
	  case DlcEnd:
	    return 0;

	  case DlcSimple:
	  case DlcListHead:
	  case DlcListBlankTop:
	  case DlcListBlankBottom:
	  case DlcListEnt:
	  case DlcListClickHere:
	  case DlcListEmpty:
	    return(dlc1->dlcelnum == dlc2->dlcelnum);
	}
	break;

      case DlcTitleBlankTop:
      case DlcTitleDashTop:
      case DlcTitle:
      case DlcTitleDashBottom:
      case DlcTitleBlankBottom:
      case DlcClickHere:
      case DlcEmpty:
      case DlcNoPermission:
      case DlcNotSet:
      case DlcBeginning:
      case DlcOneBeforeBeginning:
      case DlcTwoBeforeBeginning:
      case DlcEnd:
	return 0;
    }
    /*NOTREACHED*/
}


/* data for the display list cache */
static DL_CACHE_S *cache_array = (DL_CACHE_S *)NULL;
static long        valid_low,
		   valid_high;
static int         index_of_low,
		   size_of_cache,
		   n_cached;

/*
 * Manage the display list cache.
 *
 * The cache is a circular array of DL_CACHE_S elements.  It always
 * contains a contiguous set of display lines.
 * The lowest numbered line in the cache is
 * valid_low, and the highest is valid_high.  Everything in between is
 * also valid.  Index_of_low is where to look
 * for the valid_low element in the circular array.
 *
 * We make calls to dlc_prev and dlc_next to get new entries for the cache.
 * We need a starting value before we can do that.
 *
 * This returns a pointer to a dlc for the desired row.  If you want the
 * actual display list line you call dlist(row) instead of dlc_mgr.
 */
DL_CACHE_S *
dlc_mgr(row, op, dlc_start)
    long row;
    DlMgrOps op;
    DL_CACHE_S *dlc_start;
{
    int                new_index, known_index, next_index;
    DL_CACHE_S        *dlc = (DL_CACHE_S *)NULL;
    long               next_row;
    long               prev_row;


    if(op == Lookup){

	if(row >= valid_low && row <= valid_high){  /* already cached */

	    new_index = ((row - valid_low) + index_of_low) % size_of_cache;
	    dlc = &cache_array[new_index];

	}
	else if(row > valid_high){  /* row is past where we've looked */

	    known_index =
	      ((valid_high - valid_low) + index_of_low) % size_of_cache;
	    next_row    = valid_high + 1L;

	    /* we'll usually be looking for row = valid_high + 1 */
	    while(next_row <= row){

		new_index = (known_index + 1) % size_of_cache;

		dlc =
		  dlc_next(&cache_array[known_index], &cache_array[new_index]);

		/*
		 * This means somebody changed the file out from underneath
		 * us.  This would happen if dlc_next needed to ask for an
		 * abe to figure out what the type of the next row is, but
		 * adrbk_get_ae returned a NULL.  I don't think that can
		 * happen, but if it does...
		 */
		if(dlc->type == DlcNotSet){
		    dprint(1, (debugfile, "dlc_next returned DlcNotSet\n"));
		    dump_a_dlc_to_debug("dlc", dlc);
		    goto panic_abook_corrupt;
		}

		if(n_cached == size_of_cache){ /* replaced low cache entry */
		    valid_low++;
		    index_of_low = (index_of_low + 1) % size_of_cache;
		}
		else
		  n_cached++;

		valid_high++;

		next_row++;
		known_index = new_index; /* for next time through loop */
	    }
	}
	else if(row < valid_low){  /* row is back up the screen */

	    known_index = index_of_low;
	    prev_row = valid_low - 1L;

	    while(prev_row >= row){

		new_index = (known_index - 1 + size_of_cache) % size_of_cache;
		dlc =
		  dlc_prev(&cache_array[known_index], &cache_array[new_index]);

		if(dlc->type == DlcNotSet){
		    dprint(1, (debugfile, "dlc_prev returned DlcNotSet (1)\n"));
		    dump_a_dlc_to_debug("dlc", dlc);
		    goto panic_abook_corrupt;
		}

		if(n_cached == size_of_cache) /* replaced high cache entry */
		  valid_high--;
		else
		  n_cached++;

		valid_low--;
		index_of_low =
			    (index_of_low - 1 + size_of_cache) % size_of_cache;

		prev_row--;
		known_index = new_index;
	    }
	}
    }
    else if(op == Initialize){

	n_cached = 0;

	if(!cache_array || size_of_cache != 3 * as.l_p_page){
	    if(cache_array)
	      free_cache_array(&cache_array, size_of_cache);

	    size_of_cache = 3 * as.l_p_page;
	    cache_array =
		(DL_CACHE_S *)fs_get(size_of_cache * sizeof(DL_CACHE_S));
	    memset((void *)cache_array, 0, size_of_cache * sizeof(DL_CACHE_S));
	}

	/* this will return NULL below and the caller should ignore that */
    }
    /*
     * Flush all rows for a particular addrbook entry from the cache, but
     * keep the cache alive and anchored in the same place.  The particular
     * entry is the one that dlc_start is one of the rows of.
     */
    else if(op == FlushDlcFromCache){
	long low_entry;

	if(dlc_start->type == DlcSimple ||
	   dlc_start->type == DlcListHead ||
	   dlc_start->type == DlcListEnt ||
	   dlc_start->type == DlcListEmpty ||
	   dlc_start->type == DlcListClickHere){
	    /* find this entry in cache */
	    next_row = dlc_start->global_row - 1;
	    for(; next_row >= valid_low; next_row--){
		next_index = ((next_row - valid_low) + index_of_low) %
		    size_of_cache;
		if(!dlcs_from_same_abe(dlc_start, &cache_array[next_index]))
		    break;
	    }

	    low_entry = next_row + 1L;
	}
	else
	  low_entry = dlc_start->global_row;

	/*
	 * If low_entry now points one past a ListBlankBottom, delete that,
	 * too, since it may not make sense anymore.
	 */
	if(low_entry > valid_low){
	    next_index = ((low_entry -1L - valid_low) + index_of_low) %
		size_of_cache;
	    if(cache_array[next_index].type == DlcListBlankBottom)
		low_entry--;
	}

	if(low_entry > valid_low){ /* invalidate everything >= this */
	    n_cached -= (valid_high - (low_entry - 1L));
	    valid_high = low_entry - 1L;
	}
	else{
	    /*
	     * This is the tough case.  That entry was the first thing cached,
	     * so we need to invalidate the whole cache.  However, we also
	     * need to keep at least one thing cached for an anchor, so
	     * we need to get the dlc before this one and it should be a
	     * dlc not related to this same addrbook entry.
	     */
	    known_index = index_of_low;
	    prev_row = valid_low - 1L;

	    for(;;){

		new_index = (known_index - 1 + size_of_cache) % size_of_cache;
		dlc =
		  dlc_prev(&cache_array[known_index], &cache_array[new_index]);

		if(dlc->type == DlcNotSet){
		    dprint(1, (debugfile, "dlc_prev returned DlcNotSet (2)\n"));
		    dump_a_dlc_to_debug("dlc", dlc);
		    goto panic_abook_corrupt;
		}

		valid_low--;
		index_of_low =
			    (index_of_low - 1 + size_of_cache) % size_of_cache;

		if(!dlcs_from_same_abe(dlc_start, dlc))
		  break;

		known_index = new_index;
	    }

	    n_cached = 1;
	    valid_high = valid_low;
	}
    }
    /*
     * We have to anchor ourselves at a first element.
     * Here's how we start at the top.
     */
    else if(op == FirstEntry){
	initialize_dlc_cache();
	n_cached++;
	dlc = &cache_array[0];
	dlc = get_global_top_dlc(dlc);
	dlc->global_row = row;
	index_of_low = 0;
	valid_low    = row;
	valid_high   = row;
    }
    /* And here's how we start from the bottom. */
    else if(op == LastEntry){
	initialize_dlc_cache();
	n_cached++;
	dlc = &cache_array[0];
	dlc = get_global_bottom_dlc(dlc);
	dlc->global_row = row;
	index_of_low = 0;
	valid_low    = row;
	valid_high   = row;
    }
    /*
     * And here's how we start from an arbitrary position in the middle.
     * We root the cache at display line row, so it helps if row is close
     * to where we're going to be starting so that things are easy to find.
     * The dl that goes with line row is dl_start from addrbook number
     * adrbk_num_start.
     */
    else if(op == ArbitraryStartingPoint){
	AddrScrn_Disp      dl;

	initialize_dlc_cache();
	n_cached++;
	dlc = &cache_array[0];
	/*
	 * Save this in case fill_in_dl_field needs to free the text
	 * it points to.
	 */
	dl = dlc->dl;
	*dlc = *dlc_start;
	dlc->dl = dl;
	dlc->global_row = row;
	index_of_low = 0;
	valid_low    = row;
	valid_high   = row;
    }
    else if(op == DoneWithCache){

	n_cached = 0;
	if(cache_array)
	  free_cache_array(&cache_array, size_of_cache);
    }

    return(dlc);

panic_abook_corrupt:
    q_status_message(SM_ORDER | SM_DING, 5, 10,
	"Addrbook changed by another process, re-syncing...");
    dprint(1, (debugfile,
	"addrbook changed while we had it open?, re-sync\n"));
    dprint(2, (debugfile,
	"valid_low=%ld valid_high=%ld index_of_low=%d size_of_cache=%d\n",
	valid_low, valid_high, index_of_low, size_of_cache));
    dprint(2, (debugfile,
	"n_cached=%d new_index=%d known_index=%d next_index=%d\n",
	n_cached, new_index, known_index, next_index));
    dprint(2, (debugfile,
	"next_row=%ld prev_row=%ld row=%ld\n", next_row, prev_row, row));
    /* jump back to a safe starting point */
    longjmp(addrbook_changed_unexpectedly, 1);
    /*NOTREACHED*/
}


void
free_cache_array(c_array, size)
    DL_CACHE_S **c_array;
    int size;
{
    DL_CACHE_S *dlc;
    int i;

    for(i = 0; i < size; i++){
	dlc = &(*c_array)[i];
	/* free any allocated space */
	switch(dlc->dl.type){
	  case Text:
	  case Title:
	    if(dlc->dl.txt)
	      fs_give((void **)&dlc->dl.txt);

	    break;
	}
    }

    fs_give((void **)c_array);
}


/*
 * Get the dlc element that comes before "old".  The function that calls this
 * function is the one that keeps a cache and checks in the cache before
 * calling here.  New is a passed in pointer to a buffer where we fill in
 * the answer.
 */
DL_CACHE_S *
dlc_prev(old, new)
    DL_CACHE_S *old, *new;
{
    PerAddrBook  *pab;
    AdrBk_Entry  *abe;
    adrbk_cntr_t  list_count;

    new->adrbk_num  = -1;
    new->dlcelnum   = NO_NEXT;
    new->dlcoffset  = NO_NEXT;
    new->type       = DlcNotSet;
    pab = &as.adrbks[old->adrbk_num];

    switch(old->type){
      case DlcTitleBlankTop:
	new->adrbk_num = old->adrbk_num - 1;
	new = get_bottom_dl_of_adrbk(new->adrbk_num, new);
	break;

      case DlcTitleDashTop:
	if(old->adrbk_num == 0)
	  new->type = DlcOneBeforeBeginning;
	else
	  new->type = DlcTitleBlankTop;

	break;

      case DlcTitle:
	new->type = DlcTitleDashTop;
	break;

      case DlcTitleDashBottom:
	new->type = DlcTitle;
	break;

      case DlcTitleBlankBottom:
	new->type = DlcTitleDashBottom;
	break;

      case DlcClickHere:
      case DlcEmpty:
      case DlcNoPermission:
	if(as.n_addrbk == 1)
	  new->type = DlcOneBeforeBeginning;
	else
	  new->type = DlcTitleBlankBottom;

	break;

      case DlcSimple:
	if(old->dlcelnum == 0){
	  if(as.n_addrbk == 1)
	    new->type = DlcOneBeforeBeginning;
	  else
	    new->type = DlcTitleBlankBottom;
	}
	else{
	    new->dlcelnum = old->dlcelnum - 1;
	    abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum,
		Normal);
	    if(abe && abe->tag == Single)
	      new->type = DlcSimple;
	    else if(abe && abe->tag == List)
	      new->type = DlcListBlankBottom;
	}

	break;

      case DlcListHead:
	if(old->dlcelnum == 0){
	    if(as.n_addrbk == 1)
	      new->type = DlcOneBeforeBeginning;
	    else
	      new->type = DlcTitleBlankBottom;
	}
	else{
	    new->dlcelnum = old->dlcelnum - 1;
	    abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum,
		Normal);
	    if(abe && abe->tag == Single){
		new->type  = DlcListBlankTop;
		new->dlcelnum = old->dlcelnum;
	    }
	    else if(abe && abe->tag == List)
	      new->type  = DlcListBlankBottom;
	}

	break;

      case DlcListEnt:
	if(old->dlcoffset > 0){
	    new->type      = DlcListEnt;
	    new->dlcelnum  = old->dlcelnum;
	    new->dlcoffset = old->dlcoffset - 1;
	}
	else{
	    new->type     = DlcListHead;
	    new->dlcelnum = old->dlcelnum;
	}

	break;

      case DlcListClickHere:
      case DlcListEmpty:
	new->type     = DlcListHead;
	new->dlcelnum = old->dlcelnum;
	break;

      case DlcListBlankTop:  /* can only occur between a Simple and a List */
	new->type   = DlcSimple;
	new->dlcelnum  = old->dlcelnum - 1;
	break;

      case DlcListBlankBottom:
	new->dlcelnum = old->dlcelnum;
	abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum, Normal);
	if(F_ON(F_EXPANDED_DISTLISTS,ps_global)
	  || exp_is_expanded(pab->address_book->exp, (a_c_arg_t)new->dlcelnum)){
	    list_count = listmem_count_from_abe(abe);
	    if(list_count == 0)
	      new->type = DlcListEmpty;
	    else{
		new->type      = DlcListEnt;
		new->dlcoffset = list_count - 1;
	    }
	}
	else
	  new->type = DlcListClickHere;

	break;

      case DlcBeginning:
	new->type   = DlcBeginning;
	break;

      case DlcOneBeforeBeginning:
	new->type   = DlcTwoBeforeBeginning;
	break;

      case DlcTwoBeforeBeginning:
	new->type   = DlcBeginning;
	break;

      default:
	q_status_message(SM_ORDER | SM_DING, 5, 10,
	    "Bug in addrbook, not supposed to happen, re-syncing...");
	dprint(1,
	    (debugfile,
	    "Bug in addrbook, impossible case (%d) in dlc_prev, re-sync\n",
	    old->type));
	dump_a_dlc_to_debug("old", old);
	/* jump back to a safe starting point */
	longjmp(addrbook_changed_unexpectedly, 1);
	/*NOTREACHED*/
    }

    new->global_row = old->global_row - 1L;
    if(new->adrbk_num == -1)
      new->adrbk_num = old->adrbk_num;

    return(new);
}


/*
 * Get the dlc element that comes after "old".  The function that calls this
 * function is the one that keeps a cache and checks in the cache before
 * calling here.
 */
DL_CACHE_S *
dlc_next(old, new)
    DL_CACHE_S *old, *new;
{
    PerAddrBook  *pab;
    AdrBk_Entry  *abe;
    adrbk_cntr_t  ab_count;
    adrbk_cntr_t  list_count;

    new->adrbk_num  = -1;
    new->dlcelnum   = NO_NEXT;
    new->dlcoffset  = NO_NEXT;
    new->type       = DlcNotSet;
    pab = &as.adrbks[old->adrbk_num];

    switch(old->type){
      case DlcTitleBlankTop:
	new->type = DlcTitleDashTop;
	break;

      case DlcTitleDashTop:
	new->type = DlcTitle;
	break;

      case DlcTitle:
	new->type = DlcTitleDashBottom;
	break;

      case DlcTitleDashBottom:
	new->type = DlcTitleBlankBottom;
	break;

      case DlcTitleBlankBottom:
	if(pab->ostatus != Open && pab->access != NoAccess)
	  new->type = DlcClickHere;
	else
	  new = get_first_dl_of_adrbk(old->adrbk_num, new);

	break;

      case DlcClickHere:
      case DlcEmpty:
      case DlcNoPermission:
	if(old->adrbk_num == as.n_addrbk - 1)  /* last addrbook */
	  new->type = DlcEnd;
	else{
	    new->adrbk_num = old->adrbk_num + 1;
	    new->type = DlcTitleBlankTop;
	}

	break;

      case DlcSimple:
	ab_count = adrbk_count(pab->address_book);
	if(old->dlcelnum == ab_count - 1){  /* last element of this addrbook */
	    if(old->adrbk_num == as.n_addrbk - 1)  /* last addrbook */
	      new->type = DlcEnd;
	    else{
		new->adrbk_num = old->adrbk_num + 1;
		new->type = DlcTitleBlankTop;
	    }
	}
	else{
	    new->dlcelnum = old->dlcelnum + 1;
	    abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum,
		Normal);
	    if(abe->tag == Single)
	      new->type = DlcSimple;
	    else if(abe->tag == List)
	      new->type = DlcListBlankTop;
	}

	break;

      case DlcListHead:
	new->dlcelnum = old->dlcelnum;
	abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum, Normal);
	if(F_ON(F_EXPANDED_DISTLISTS,ps_global)
	  || exp_is_expanded(pab->address_book->exp, (a_c_arg_t)new->dlcelnum)){
	    list_count = listmem_count_from_abe(abe);
	    if(list_count == 0)
	      new->type = DlcListEmpty;
	    else{
		new->type      = DlcListEnt;
		new->dlcoffset = 0;
	    }
	}
	else
	  new->type = DlcListClickHere;

	break;

      case DlcListEnt:
	new->dlcelnum = old->dlcelnum;
	abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum, Normal);
	list_count = listmem_count_from_abe(abe);
	if(old->dlcoffset == list_count - 1){  /* last member of list */
	    ab_count = adrbk_count(pab->address_book);
	    if(old->dlcelnum == ab_count - 1){  /* last entry in addrbook */
		if(old->adrbk_num == as.n_addrbk - 1)  /* last addrbook */
		  new->type = DlcEnd;
		else{
		    new->type = DlcTitleBlankTop;
		    new->adrbk_num = old->adrbk_num + 1;
		}
	    }
	    else
	      new->type = DlcListBlankBottom;
	}
	else{
	    new->type      = DlcListEnt;
	    new->dlcoffset = old->dlcoffset + 1;
	}

	break;

      case DlcListClickHere:
      case DlcListEmpty:
	new->dlcelnum = old->dlcelnum;
	abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum, Normal);
	ab_count = adrbk_count(pab->address_book);
	if(old->dlcelnum == ab_count - 1){  /* last entry in addrbook */
	    if(old->adrbk_num == as.n_addrbk - 1)  /* last addrbook */
	      new->type = DlcEnd;
	    else{
		new->type = DlcTitleBlankTop;
		new->adrbk_num = old->adrbk_num + 1;
	    }
	}
	else
	  new->type = DlcListBlankBottom;

	break;

      case DlcListBlankTop:
	new->type   = DlcListHead;
	new->dlcelnum  = old->dlcelnum;
	break;

      case DlcListBlankBottom:
	new->dlcelnum = old->dlcelnum + 1;
	abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum, Normal);
	if(abe->tag == Single)
	  new->type = DlcSimple;
	else if(abe->tag == List)
	  new->type = DlcListHead;

	break;

      case DlcEnd:
	new->type = DlcEnd;
	break;

      case DlcOneBeforeBeginning:
	new = get_global_top_dlc(new);
	break;

      case DlcTwoBeforeBeginning:
	new->type = DlcOneBeforeBeginning;
	break;

      default:
	q_status_message(SM_ORDER | SM_DING, 5, 10,
	    "Bug in addrbook, not supposed to happen, re-syncing...");
	dprint(1,
	    (debugfile,
	    "Bug in addrbook, impossible case (%d) in dlc_next, re-sync\n",
	    old->type));
	dump_a_dlc_to_debug("old", old);
	/* jump back to a safe starting point */
	longjmp(addrbook_changed_unexpectedly, 1);
	/*NOTREACHED*/
    }

    new->global_row = old->global_row + 1L;
    if(new->adrbk_num == -1)
      new->adrbk_num = old->adrbk_num;

    return(new);
}


/*
 * Get the display line at the very top of whole addrbook screen display.
 *
 * If only one addrbook, no titles.
 */
DL_CACHE_S *
get_global_top_dlc(new)
    DL_CACHE_S *new;  /* fill in answer here */
{
    new->dlcelnum   = NO_NEXT;
    new->dlcoffset  = NO_NEXT;
    new->type       = DlcNotSet;

    if(as.n_addrbk > 1)
      new->type = DlcTitleDashTop;
    else /* only one addrbook, get top entry */
      new = get_first_dl_of_adrbk(0, new);

    new->adrbk_num  = 0;

    return(new);
}


/*
 * Get the last display line for the whole address book screen.
 * This gives us a way to start at the end and move back up.
 */
DL_CACHE_S *
get_global_bottom_dlc(new)
    DL_CACHE_S *new;  /* fill in answer here */
{
    new->dlcelnum   = NO_NEXT;
    new->dlcoffset  = NO_NEXT;
    new->type       = DlcNotSet;

    new->adrbk_num = as.n_addrbk - 1;

    new = get_bottom_dl_of_adrbk(new->adrbk_num, new);

    return(new);
}


/*
 * First dl in a particular addrbook, not counting title lines.
 */
DL_CACHE_S *
get_first_dl_of_adrbk(adrbk_num, new)
    int         adrbk_num;
    DL_CACHE_S *new;  /* fill in answer here */
{
    PerAddrBook  *pab;
    AdrBk_Entry  *abe;
    adrbk_cntr_t  ab_count;

    pab = &as.adrbks[adrbk_num];

    if(pab->access == NoAccess)
      new->type = DlcNoPermission;
    else{
	ab_count = adrbk_count(pab->address_book);
	if(ab_count == 0)
	  new->type = DlcEmpty;
	else{
	    new->dlcelnum = 0;
	    abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum,
		Normal);
	    if(abe->tag == Single)
	      new->type = DlcSimple;
	    else if(abe->tag == List)
	      new->type = DlcListHead;
	}
    }

    return(new);
}


/*
 * Find the last display line for addrbook number adrbk_num.
 */
DL_CACHE_S *
get_bottom_dl_of_adrbk(adrbk_num, new)
    int         adrbk_num;
    DL_CACHE_S *new;  /* fill in answer here */
{
    PerAddrBook  *pab;
    AdrBk_Entry  *abe;
    adrbk_cntr_t  ab_count;
    adrbk_cntr_t  list_count;

    pab = &as.adrbks[adrbk_num];

    if(pab->ostatus != Open){
	if(pab->access == NoAccess)
	  new->type = DlcNoPermission;
	else
	  new->type = DlcClickHere;
    }
    else{
	ab_count = adrbk_count(pab->address_book);
	if(ab_count == 0)
	  new->type = DlcEmpty;
	else{
	    new->dlcelnum = ab_count - 1;
	    abe = adrbk_get_ae(pab->address_book, (a_c_arg_t)new->dlcelnum,
		Normal);
	    if(abe->tag == Single)
	      new->type = DlcSimple;
	    else if(abe->tag == List){
		if(F_ON(F_EXPANDED_DISTLISTS,ps_global)
		   || exp_is_expanded(pab->address_book->exp,
				      (a_c_arg_t)new->dlcelnum)){
		    list_count = listmem_count_from_abe(abe);
		    if(list_count == 0)
		      new->type = DlcListEmpty;
		    else{
			new->type      = DlcListEnt;
			new->dlcoffset = list_count - 1;
		    }
		}
		else
		  new->type = DlcListClickHere;
	    }
	}
    }

    return(new);
}


/*
 * Uses information in new to fill in new->dl.
 */
void
fill_in_dl_field(new)
    DL_CACHE_S *new;
{
    AddrScrn_Disp *dl;
    PerAddrBook   *pab;
    char buf[MAX_SCREEN_COLS + 1];
    char buf2[1024];
    int screen_width = ps_global->ttyo->screen_cols;
    int len;

    buf[MAX_SCREEN_COLS] = '\0';
    screen_width = min(MAX_SCREEN_COLS, screen_width);

    dl = &(new->dl);

    /* free any previously allocated space */
    switch(dl->type){
      case Text:
      case Title:
	if(dl->txt)
	  fs_give((void **)&dl->txt);
    }

    /* set up new dl */
    switch(new->type){
      case DlcTitleBlankTop:
      case DlcTitleBlankBottom:
      case DlcListBlankTop:
      case DlcListBlankBottom:
	dl->type = Text;
	dl->txt  = cpystr("");
	break;

      case DlcTitleDashTop:
      case DlcTitleDashBottom:
	/* line of dashes in txt field */
	dl->type = Text;
	memset((void *)buf, '-', screen_width * sizeof(char));
	buf[screen_width] = '\0';
	dl->txt = cpystr(buf);
	break;

      case DlcNoPermission:
	dl->type = Text;
	dl->txt  = cpystr(NO_PERMISSION);
	break;

      case DlcTitle:
	dl->type = Title;
	pab = &as.adrbks[new->adrbk_num];
        /* title for this addrbook */
        memset((void *)buf, SPACE, screen_width * sizeof(char));
	buf[screen_width] = '\0';
        sprintf(buf2, "%s AddressBook <%s>",
		    (new->adrbk_num < as.how_many_personals) ?
			"Personal" :
			"Global",
                    pab->nickname ? pab->nickname : pab->filename);
        len = strlen(buf2);
        strncpy(buf, buf2, len);
        if(as.ro_warning){
            char *q;

            if(pab->access == ReadOnly){
                if(screen_width - len - (int)strlen(READONLY) > 3){
                    q = buf + screen_width - strlen(READONLY);
                    strcpy(q, READONLY);
                }
            }
            else if(pab->access == NoAccess){
                if(screen_width - len - (int)strlen(NOACCESS) > 3){
                    q = buf + screen_width - strlen(NOACCESS);
                    strcpy(q, NOACCESS);
                }
            }
        }

	dl->txt = cpystr(buf);
	break;

      case DlcClickHere:
	dl->type = ClickHere;
	break;

      case DlcListClickHere:
	dl->type  = ListClickHere;
	dl->elnum = new->dlcelnum;
	break;

      case DlcListEmpty:
	dl->type  = ListEmpty;
	dl->elnum = new->dlcelnum;
	break;

      case DlcEmpty:
	dl->type = Empty;
	break;

      case DlcSimple:
	dl->type  = Simple;
	dl->elnum = new->dlcelnum;
	break;

      case DlcListHead:
	dl->type  = ListHead;
	dl->elnum = new->dlcelnum;
	break;

      case DlcListEnt:
	dl->type     = ListEnt;
	dl->elnum    = new->dlcelnum;
	dl->l_offset = new->dlcoffset;
	break;

      case DlcBeginning:
      case DlcOneBeforeBeginning:
      case DlcTwoBeforeBeginning:
	dl->type = Beginning;
	break;

      case DlcEnd:
	dl->type = End;
	break;

      default:
	q_status_message(SM_ORDER | SM_DING, 5, 10,
	    "Bug in addrbook, not supposed to happen, re-syncing...");
	dprint(1,
	    (debugfile,
	    "Bug in addrbook, impossible dflt in fill_in_dl (%d)\n",
	    new->type));
	dump_a_dlc_to_debug("new", new);
	/* jump back to a safe starting point */
	longjmp(addrbook_changed_unexpectedly, 1);
	/*NOTREACHED*/
    }
}


/*
 * Args: start_disp     --  line to start displaying on when redrawing, 0 is
 *	 		    the top_of_screen
 *       cur_line       --  current line number (0 is 1st line we display)
 *       old_line       --  old line number
 *       redraw         --  flag requesting redraw as opposed to update of
 *			    current line
 *     start_pos        --  return position where highlighted text begins here
 *
 * Result: lines painted on the screen
 *
 * It either redraws the screen from line "start_disp" down or
 * moves the cursor from one field to another.
 */
void
display_book(start_disp, cur_line, old_line, redraw, start_pos)
    int  start_disp,
	 cur_line,
	 old_line,
	 redraw;
    Pos *start_pos;
{
    int screen_row, highlight;
    long global_row;
    Pos sp;

    dprint(9, (debugfile,
	"- display_book() -\n   top %d start %d cur_line %d redraw %d\n",
	as.top_ent, start_disp, cur_line, redraw));

    if(start_pos){
	start_pos->row = 0;
	start_pos->col = 0;
    }

    if(as.l_p_page <= 0)
      return;

#ifdef _WINDOWS
    mswin_beginupdate();
#endif
    if(redraw){
        /*--- Repaint all of the screen or bottom part of screen ---*/
        global_row = as.top_ent + start_disp;
        for(screen_row = start_disp;
	    screen_row < as.l_p_page;
	    screen_row++, global_row++){

            highlight = (screen_row == cur_line);
	    ClearLine(screen_row + HEADER_ROWS(ps_global));
            paint_line(screen_row + HEADER_ROWS(ps_global), global_row,
		highlight, &sp);
	    if(start_pos && highlight)
	      *start_pos = sp;
        }
    }
    else{

        /*--- Only update current, or move the cursor ---*/
        if(cur_line != old_line){

            /*--- Repaint old position to erase "cursor" ---*/
            paint_line(old_line + HEADER_ROWS(ps_global), as.top_ent + old_line,
                       0, &sp);
        }

        /*--- paint the position with the cursor ---*/
        paint_line(cur_line + HEADER_ROWS(ps_global), as.top_ent + cur_line,
                   1, &sp);
	if(start_pos)
	  *start_pos = sp;
    }

#ifdef _WINDOWS
    scroll_setpos(as.top_ent);
    mswin_endupdate();
#endif
    fflush(stdout);
}


/*
 * Paint a line on the screen
 *
 * Args: line    --  Line on screen to paint
 *    global_row --  Row number of line to paint
 *     highlight --  Line should be highlighted
 *     start_pos --  return position where text begins here
 *
 * Result: Line is painted
 *
 * The three field widths for the formatting are passed in.  There is an
 * implicit 2 spaces between the fields.
 *
 *    | fld_width[0] chars |__| fld_width[1] |__| fld_width[2] | ...
 */
void
paint_line(line, global_row, highlight, start_pos)
    int  line;
    long global_row;
    int  highlight;
    Pos *start_pos;
{
    int		  fld_col[NFIELDS],
		  fld_width[NFIELDS],
		  screen_width,
		  col,
		  scol = -1,
		  fld;
    char	  fld_control[NFIELDS][11],
		  full_control[11];
    char	  *string;
    AddrScrn_Disp *dl;
    AdrBk_Entry	  *abe;
    PerAddrBook	  *pab;

    dprint(10, (debugfile, "- paint_line(%d, %d) -\n", line, highlight));

    dl  = dlist(global_row);
    pab = &as.adrbks[adrbk_num_from_lineno(global_row)];
    start_pos->row = line;
    start_pos->col = 0; /* default */

    switch(dl->type){
      case Beginning:
      case End:
        return;
    }

    screen_width = ps_global->ttyo->screen_cols;
    sprintf(full_control, "%%-%d.%ds",   screen_width, screen_width);

    if(highlight)
      StartInverse();

    /* the types in this set span all columns */
    switch(dl->type){
      case Text:
      case Title:
      case ClickHere:
      case Empty:
	if(dl->type == ClickHere)
	  string = CLICKHERE;
	else if(dl->type == Empty)
	  string = EMPTY;
	else
	  string = dl->txt;

	/* center it */
	col = (screen_width - (int)strlen(string))/2;
	if(col >= 0)
	  PutLine0(line, col, string);
	else{
	    col = 0;
	    PutLine1(line, col, full_control, string);
	}

	start_pos->col = col;
	if(highlight)
	  EndInverse();

	return;
    }

    for(fld = 0; fld < NFIELDS; fld++)
      fld_width[fld] = pab->disp_form[fld].width;

    fld_col[0] = 0;
    for(fld = 1; fld < NFIELDS; fld++)
      fld_col[fld] = min(fld_col[fld-1]+fld_width[fld-1]+2, screen_width-1);

    for(fld = 0; fld < NFIELDS; fld++){
        if(pab->disp_form[fld].wtype == Special){
            sprintf(fld_control[fld], "%%-%d.%ds",
		fld_width[fld],fld_width[fld]);
	    fld_col[fld] = screen_width - fld_width[fld];
	}
        else if((fld+1) == NFIELDS
          || pab->disp_form[fld+1].type == Notused
          || pab->disp_form[fld+1].type == WhenNoAddrDisplayed)
          sprintf(fld_control[fld], "%%-%d.%ds",
	      fld_width[fld],fld_width[fld]);
        else
          sprintf(fld_control[fld], "%%-%d.%ds  ",
	      fld_width[fld],fld_width[fld]);
    }
      
    for(fld = 0; fld < NFIELDS; fld++){
      if(fld_width[fld] == 0)
	continue;

      switch(pab->disp_form[fld].type){
	case Notused:
	  break;

	case Nickname:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      abe = ae(global_row);
	      string = (abe && abe->nickname) ? abe->nickname : "";
	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld], string);
	      break;
	  }
	  break;

	case Fullname:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      abe = ae(global_row);
	      string = (abe && abe->fullname) ? abe->fullname : "";
	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld],
		  (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
							      string, NULL));
	      break;

	    case ListEnt:
	      /* continuation line */
	      if(line == HEADER_ROWS(ps_global)){
	        char temp[50];
		int  i, width, width1, width2;

		/*
		 * This field may overflow into next field because the
		 * fullname field doesn't usually come on the same line
		 * as the next field but does this time because it is
		 * a continuation line.
		 */
		width = fld_width[fld];
		for(i = fld+1; i < NFIELDS && fld_width[i] > 0; i++){
		    if(fld_col[fld] + width + 2 > fld_col[i]){
			width = max(fld_col[i] - fld_col[fld] - 2, 0);
			break;
		    }
		}

		width2 = min(11, width);
		width1 = max(width - width2 - 1, 0);
	        abe = ae(global_row);
	        string = (abe && abe->fullname) ? abe->fullname : "";
	        sprintf(temp, "%.*s%s%.*s", width1,
		  (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
							      string, NULL),
		  width1 ? " " : "",
		  width2,  "(continued)");
		if(highlight)
		  EndInverse();

	        PutLine1(line, fld_col[fld], fld_control[fld], temp);
		if(highlight)
		  StartInverse();
	      }
	      break;
	  }
	  break;

	case Addr:
	  switch(dl->type){
	    case ListClickHere:
	    case ListEmpty:
	      if(dl->type == ListClickHere)
		string = CLICKHERE;
	      else
	        string = EMPTY;

	      /* ok to go past edge of field for these types */
	      if((screen_width - fld_col[fld]) >= (int)strlen(string))
		col = fld_col[fld];  /* left-adjusted in column */
	      else
		col = screen_width - (int)strlen(string);

	      if(col >= 0)
	        PutLine0(line, col, string);
	      else{
	          col = 0;
	          PutLine1(line, col, full_control, string);
	      }

	      if(scol == -1)
		scol = col;

	      break;

	    case ListHead:
	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld], DISTLIST);
	      break;

	    case Simple:
	      abe = ae(global_row);
	      string = (abe && abe->tag == Single && abe->addr.addr) ?
		  abe->addr.addr : "";
	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld], string);
	      break;

	    case ListEnt:
	      string = listmem(global_row) ? listmem(global_row) : "";
	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld],
		  (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
							  string, NULL));
	      break;
	  }
	  break;

	case Filecopy:
	case Comment:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      abe = ae(global_row);
	      if(pab->disp_form[fld].type == Filecopy)
	        string = (abe && abe->fcc) ? abe->fcc : "";
	      else
	        string = (abe && abe->extra) ? abe->extra : "";

	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld],
		  (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
							      string, NULL));
	      break;
	  }
	  break;

	case Checkbox:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      if(entry_is_checked(pab->address_book->checks,
				  (a_c_arg_t)dl->elnum))
		string = "[X]";
	      else
	        string = "[ ]";

	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld], string);
	      break;
	  }
	  break;

	case WhenNoAddrDisplayed:
	  switch(dl->type){
	    case ListClickHere:
	    case ListEmpty:
	    case ListEnt:
	      if(dl->type == ListClickHere)
		string = CLICKHERE;
	      else if(dl->type == ListEmpty)
	        string = EMPTY;
	      else
		string = listmem(global_row)
			  ? (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						      listmem(global_row), NULL)
			  : "";

	      if(scol == -1)
		scol = fld_col[fld];

	      PutLine1(line, fld_col[fld], fld_control[fld], string?string:"");
	      break;
	  }
	  break;
      }
    }

    if(highlight)
      EndInverse();
    
    if(scol > 0)
      start_pos->col = scol;
}



/*
 * Set field widths for the columns of the display.  The idea is to
 * try to come up with something that works pretty well.  Getting it just
 * right isn't important.
 *
 * Col1 and col2 are arrays which contain some widths useful for
 * formatting the screen.  The 0th element is the max
 * width in that column.  The 1st element is the max of the third largest
 * width in each addressbook (yup, strange).
 *
 * The info above applies to the default case (AllAuto).  The newer methods
 * where the user specifies the format is the else part of the big if and
 * is quite a bit different.  It's all sort of ad hoc when we're asked
 * to calculate one of the fields for the user.
 *
 * Returns non-zero if the widths changed since the last time called.
 */
int
calculate_field_widths()
{
  int space_left, i, j, screen_width;
  int ret = 0;
  PerAddrBook *pab;
  WIDTH_INFO_S *widths;
  int max_nick, max_full, max_addr, third_full, third_addr, third_fcc;
  int col1[5], col2[5];
  int nick = -1, full = -1, addr = -1;
#define SEP 2  /* space between columns */

  dprint(9, (debugfile, "- calculate_field_widths -\n"));

  screen_width = ps_global->ttyo->screen_cols;

  /* calculate widths for each addrbook independently */
  for(j = 0; j < as.n_addrbk; j++){
    pab = &as.adrbks[j];

    max_nick   = 0;
    max_full   = 2;
    max_addr   = 2;
    third_full = 2;
    third_addr = 2;
    third_fcc  = 2;

    if(pab->ostatus == Open || pab->ostatus == NoDisplay){
      widths = &pab->address_book->widths;
      max_nick   = min(max(max_nick, widths->max_nickname_width), 25);
      max_full   = max(max_full, widths->max_fullname_width);
      max_addr   = max(max_addr, widths->max_addrfield_width);
      third_full = max(third_full, widths->third_biggest_fullname_width);
      if(third_full == 2)
        third_full = max(third_full, 2*max_full/3);

      third_addr = max(third_addr, widths->third_biggest_addrfield_width);
      if(third_addr == 2)
        third_addr = max(third_addr, 2*max_addr/3);

      third_fcc  = max(third_fcc, widths->third_biggest_fccfield_width);
      if(third_fcc == 2)
        third_fcc = max(third_fcc, 2*widths->max_fccfield_width/3);
    }

    /* figure out which order they're in and reset widths */
    for(i = 0; i < NFIELDS; i++){
      pab->disp_form[i].width = 0;
      switch(pab->disp_form[i].type){
        case Nickname:
	  nick = i;
	  break;

	case Fullname:
	  full = i;
	  break;

	case Addr:
	  addr = i;
	  break;
      }
    }

    /* Compute default format */
    if(pab->disp_form[1].wtype == AllAuto){

      col1[0] = max_full;
      col2[0] = max_addr;
      col1[1] = third_full;
      col2[1] = third_addr;
      col1[2] = 3;
      col2[2] = 3;
      col1[3] = 2;
      col2[3] = 2;
      col1[4] = 1;
      col2[4] = 1;

      space_left = screen_width;

      if(pab->disp_form[0].type == Checkbox){
	pab->disp_form[0].width = min(pab->disp_form[0].req_width,space_left);
	space_left = max(space_left-(pab->disp_form[0].width + SEP), 0);
      }

      /*
       * All of the nickname field should be visible,
       * and make it at least 3.
       */
      pab->disp_form[nick].width = min(max(max_nick, 3), space_left);

      /*
       * The SEP is for two blank columns between nickname and next field.
       * Those blank columns are taken automatically in paint_line().
       */
      space_left -= (pab->disp_form[nick].width + SEP);

      if(space_left > 0){
	for(i = 0; i < 5; i++){
	  /* try fitting most of each field in if possible */
	  if(col1[i] + SEP + col2[i] <= space_left){
	    int extra;

	    extra = space_left - col1[i] - SEP - col2[i];
	    /*
	     * try to stabilize nickname column shifts
	     * so that screen doesn't jump around when we make changes
	     */
	    if(i == 0 && pab->disp_form[nick].width < 7 &&
			extra >= (7 - pab->disp_form[nick].width)){
	      extra -= (7 - pab->disp_form[nick].width);
	      space_left -= (7 - pab->disp_form[nick].width);
	      pab->disp_form[nick].width = 7;
	    }

	    pab->disp_form[addr].width = col2[i] + extra/2;
	    pab->disp_form[full].width =
				space_left - SEP - pab->disp_form[addr].width;
	    break;
	  }
	}

	/*
	 * None of them would fit.  Toss addr field.
	 */
	if(i == 5){
	  pab->disp_form[full].width = space_left;
	  pab->disp_form[addr].width = 0;
	}
      }
      else{
	pab->disp_form[full].width = 0;
	pab->disp_form[addr].width = 0;
      }

      dprint(10, (debugfile, "Using %s choice: %d %d %d", enth_string(i+1),
	    pab->disp_form[nick].width, pab->disp_form[full].width,
	    pab->disp_form[addr].width));
    }
    else{  /* non-default case */
      int some_to_calculate = 0;
      int columns = 0;
      int used = 0;
      int avail_screen;
      int all_percents = 1;
      int pc_tot;

      /*
       * First count how many fields there are.
       * Fill in all the Fixed's while we're at it.
       */
      for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
	if(pab->disp_form[i].wtype == Fixed){
	  pab->disp_form[i].width = pab->disp_form[i].req_width;
	  all_percents = 0;
	}
	else if(pab->disp_form[i].wtype == WeCalculate){
	  pab->disp_form[i].width = pab->disp_form[i].req_width; /* for now */
	  some_to_calculate++;
	  all_percents = 0;
	}

	if(pab->disp_form[i].wtype != Special){
	  used += pab->disp_form[i].width;
	  columns++;
	}
      }

      used += ((columns-1) * SEP);
      avail_screen = screen_width - used;

      /*
       * Now that we know how much space we've got, we can
       * calculate the Percent columns.
       */
      if(avail_screen > 0){
        for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
	  if(pab->disp_form[i].wtype == Percent){
	    /* The 2, 200, and +100 are because we're rounding */
	    pab->disp_form[i].width =
		     ((2*pab->disp_form[i].req_width*avail_screen)+100) / 200;
	    used += pab->disp_form[i].width;
	  }
        }
      }

      space_left = screen_width - used;

      if(space_left < 0){
	/*
	 * If they're all percentages, and the percentages add up to 100,
	 * then we should fix the rounding problem.
	 */
	pc_tot = 0;
	if(all_percents){
          for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++)
	    if(pab->disp_form[i].wtype == Percent)
	      pc_tot += pab->disp_form[i].req_width;
	}

	/* fix the rounding problem */
	if(all_percents && pc_tot <= 100){
	  int col = columns;
	  int this_col = 0;
	  int fix = used - screen_width;

	  while(fix--){
	    if(col < 0)
	      col = columns;

	    /* find col'th column */
	    for(i=0, this_col=0; i < NFIELDS; i++){
	      if(pab->disp_form[i].wtype == Percent){
	        if(col == ++this_col)
	          break;
	      }
	    }

	    pab->disp_form[i].width--;
	    col--;
	  }
	}
	/*
	 * Assume they meant to have them add up to over 100%, so we
	 * just truncate the right hand edge.
	 */
	else{
	  int this_fix, space_over;

	  /* have to reduce space_over down to zero.  */
	  space_over = used - screen_width;
	  for(i=NFIELDS-1; i >= 0 && space_over > 0; i--){
	    if(pab->disp_form[i].type != Notused){
	      this_fix = min(pab->disp_form[i].width, space_over);
	      pab->disp_form[i].width -= this_fix;
	      space_over -= this_fix;
	    }
	  }
	}
      }
      else if(space_left > 0){
	if(some_to_calculate){
	  /* make nickname big enough to show all nicknames */
	  if(nick >= 0 && pab->disp_form[nick].wtype == WeCalculate){
	    --some_to_calculate;
	    if(pab->disp_form[nick].width != max_nick){
	      int this_fix;

	      this_fix = min(max_nick-pab->disp_form[nick].width, space_left);
	      pab->disp_form[nick].width += this_fix;
	      space_left -= this_fix;
	    }
	  }

	  if(!some_to_calculate && space_left > 0)
	    goto none_to_calculate;

	  if(space_left > 0){
	    int weight  = 0;
	    int used_wt = 0;

	    /* add up total weight */
	    for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
	      if(i != nick && pab->disp_form[i].wtype == WeCalculate){
	        switch(pab->disp_form[i].type){
		  case Fullname:
		    weight  += max(third_full, pab->disp_form[i].width);
		    used_wt += pab->disp_form[i].width;
		    break;

		  case Addr:
		    weight  += max(third_addr, pab->disp_form[i].width);
		    used_wt += pab->disp_form[i].width;
		    break;

		  case Filecopy:
		    weight  += max(third_fcc, pab->disp_form[i].width);
		    used_wt += pab->disp_form[i].width;
		    break;
		}
	      }
	    }

	    if(weight > 0){
	      int this_fix;

	      if(weight - used_wt <= space_left){
	        for(i = 0;
	            i < NFIELDS && pab->disp_form[i].type != Notused;
		    i++){
		  if(i != nick && pab->disp_form[i].wtype == WeCalculate){
		    switch(pab->disp_form[i].type){
		      case Fullname:
		        this_fix = third_full - pab->disp_form[i].width;
		        space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;

		      case Addr:
			this_fix = third_addr - pab->disp_form[i].width;
			space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;

		      case Filecopy:
			this_fix = third_fcc - pab->disp_form[i].width;
			space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;
		    }
		  }
		}

		/* if still space left and a comment field, all to comment */
		if(space_left){
	          for(i = 0;
		      i < NFIELDS && pab->disp_form[i].type != Notused;
		      i++){
		    if(pab->disp_form[i].type == Comment &&
		       pab->disp_form[i].wtype == WeCalculate){
		      pab->disp_form[i].width += space_left;
		      space_left = 0;
		    }
		  }
		}
	      }
	      else{ /* not enough space, dole out weighted pieces */
		int was_sl = space_left;

	        for(i = 0;
		    i < NFIELDS && pab->disp_form[i].type != Notused;
		    i++){
		  if(i != nick && pab->disp_form[i].wtype == WeCalculate){
		    switch(pab->disp_form[i].type){
		      case Fullname:
			/* round down */
			this_fix = (third_full * was_sl)/weight;
			space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;

		      case Addr:
			this_fix = (third_addr * was_sl)/weight;
			space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;

		      case Filecopy:
			this_fix = (third_fcc * was_sl)/weight;
			space_left -= this_fix;
			pab->disp_form[i].width += this_fix;
			break;
		    }
		  }
		}
	      }
	    }

	    /* give out rest */
	    while(space_left > 0){
	      for(i=NFIELDS-1; i >= 0 && space_left > 0; i--){
	        if(i != nick && pab->disp_form[i].wtype == WeCalculate){
		  pab->disp_form[i].width++;
		  space_left--;
		}
	      }
	    }
	  }
	}
	else{
	  /*
	   * If they're all percentages, and the percentages add up to 100,
	   * then we just have to fix a rounding problem.  Otherwise, we'll
	   * assume the user meant to have them add up to less than 100.
	   */
none_to_calculate:
	  pc_tot = 0;
	  if(all_percents){
	    for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++)
	      if(pab->disp_form[i].wtype == Percent)
	        pc_tot += pab->disp_form[i].req_width;
	  }

	  if(all_percents && pc_tot >= 100){
	    int col = columns;
	    int this_col = 0;
	    int fix = screen_width - used;

	    while(fix--){
	      if(col < 0)
	        col = columns;

	      /* find col'th column */
	      for(i=0, this_col=0; i < NFIELDS; i++){
	        if(pab->disp_form[i].wtype == Percent){
	          if(col == ++this_col)
		    break;
		}
	      }

	      pab->disp_form[i].width++;
	      col--;
	    }
	  }
	  /* else, user specified less than 100%, leave it */
	}
      }
      /* else space_left == zero, nothing to do */

      /*
       * Check for special case.  If we find it, this is the case where
       * we want to display the list entry field even though there is no
       * address field displayed.  All of the display width is probably
       * used up by now, so we just need to pick some arbitrary width
       * for these lines.  Since these lines are separate from the other
       * lines we've been calculating, we don't have to worry about running
       * into them, except for list continuation lines which we're not
       * going to worry about.
       */
      for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
	if(pab->disp_form[i].wtype == Special){
	    pab->disp_form[i].width = min(strlen(CLICKHERE), screen_width);
	    break;
	}
      }
    }

    /* check for width changes */
    for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
      if(pab->disp_form[i].width != pab->disp_form[i].old_width){
	ret++;  /* Tell the caller the screen changed */
	pab->disp_form[i].old_width = pab->disp_form[i].width;
      }
    }

    pab->nick_is_displayed    = 0;
    pab->full_is_displayed    = 0;
    pab->addr_is_displayed    = 0;
    pab->fcc_is_displayed     = 0;
    pab->comment_is_displayed = 0;
    for(i = 0; i < NFIELDS && pab->disp_form[i].type != Notused; i++){
      if(pab->disp_form[i].width > 0){
        switch(pab->disp_form[i].type){
	  case Nickname:
	    pab->nick_is_displayed++;
	    break;
	  case Fullname:
	    pab->full_is_displayed++;
	    break;
	  case Addr:
	    pab->addr_is_displayed++;
	    break;
	  case Filecopy:
	    pab->fcc_is_displayed++;
	    break;
	  case Comment:
	    pab->comment_is_displayed++;
	    break;
	}
      }
    }
  }

  return(ret);
}


void
redraw_addr_screen()
{
    dprint(7, (debugfile, "- redraw_addr_screen -\n"));

    ab_resize();
    if(as.l_p_page <= 0)
      return;

    (void)calculate_field_widths();
    display_book(0, as.cur_row, -1, 1, (Pos *)NULL);
}


/*
 * Little front end for address book screen so it can be called out
 * of the main command loop in pine.c
 */
void
addr_book_screen(pine_state)
    struct pine *pine_state;
{
    dprint(1, (debugfile, "\n\n --- ADDR_BOOK_SCREEN ---\n\n"));

    mailcap_free(); /* free resources we won't be using for a while */

    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1, (debugfile, "RESETTING address book... addr_book_screen!\n"));
	addrbook_reset();
    }

    (void)addr_book(AddrBookScreen, "ADDRESS BOOK", NULL);
    end_adrbks();
    pine_state->prev_screen = addr_book_screen;
}


/*ARGSUSED*/
/*
 * Call address book from message composer
 *
 * Args: error_mess -- pointer to return error messages in (unused here)
 *
 * Returns: pointer to returned address, or NULL if nothing returned
 */
char *
addr_book_compose(error_mess)
    char **error_mess;
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "--- addr_book_compose ---\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book... addr_book_compose!\n"));
	addrbook_reset();
    }

    /*
     * We used to use SelectAddrCom here, but we changed it so that it
     * returns a list of nicknames instead of the expanded addresses.  That
     * allows the composer to then call build_address and associate the
     * correct fcc with a nickname.  Previously, the composer didn't call
     * the builder after doing a selector call.
     */
    p = addr_book(SelectNicksCom, "COMPOSER: SELECT ADDRESS", NULL);

    end_adrbks();

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*ARGSUSED*/
/*
 * Call address book from message composer for Lcc line
 *
 * Args: error_mess -- pointer to return error messages in (unused here)
 *
 * Returns: pointer to returned address, or NULL if nothing returned
 */
char *
addr_book_compose_lcc(error_mess)
    char **error_mess;
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "--- addr_book_compose_lcc ---\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book... addr_book_compose_lcc!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectAddrLccCom, "COMPOSER: SELECT LIST", NULL);

    end_adrbks();

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*ARGSUSED*/
/*
 * Call address book from message composer for Lcc line
 *
 * Args: error_mess -- pointer to return error messages in (unused here)
 *
 * Returns: pointer to returned address, or NULL if nothing returned
 */
char *
addr_book_change_list(error_mess)
    char **error_mess;
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "--- addr_book_change_list ---\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book... addr_book_change_list!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectNicksCom, "ADDRESS BOOK (Edit): SELECT ADDRESSES",
		    NULL);

    end_adrbks();

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*ARGSUSED*/
/*
 * Call address book from pine_simple_send
 *
 * Returns: pointer to returned address, or NULL if nothing returned
 */
char *
addr_book_bounce()
{
    return(addr_book_seladdr());
}


/*
 * Call address book from take address screen
 *
 * Returns: pointer to returned nickname, or NULL if nothing returned
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
char *
addr_book_takeaddr()
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_takeaddr -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book...addr_book_takeaddr!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectNickTake, "TAKEADDR: SELECT NICKNAME", NULL);

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*
 * Call address book from editing screen for nickname field.
 *
 * Returns: pointer to returned nickname, or NULL if nothing returned
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
char *
addr_book_nick_for_edit()
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_nick_for_edit -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book...addr_book_nick_for_edit!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectNickCom, "SELECT NICKNAME", NULL);

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*
 * Call address book for generic nickname select
 *
 * Returns: pointer to returned nickname, or NULL if nothing returned
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
char *
addr_book_selnick()
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_selnick -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book...addr_book_selnick!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectNick, "SELECT NICKNAME", NULL);

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*
 * Call address book for generic address select
 *
 * Returns: pointer to returned address, or NULL if nothing returned
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
char *
addr_book_seladdr()
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_seladdr -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book...addr_book_seladdr!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectAddr, "SELECT ADDRESS", NULL);

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*
 * Call address book for address select
 *
 * Returns: pointer to returned address without the fullname,
 *          or NULL if nothing returned
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
char *
addr_book_seladdr_nofull()
{
    char *p;
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_seladdr_nofull -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile,"RESETTING address book...addr_book_seladdr_nofull!\n"));
	addrbook_reset();
    }

    p = addr_book(SelectAddrTake, "SELECT ADDRESS", NULL);

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(p);
}


/*
 * Call address book for selecting a list of nicknames
 *
 * Returns: array is returned in argument
 *
 * The caller is assumed to handle the closing of the addrbooks, so we
 * don't call end_adrbks().
 */
void
addr_book_manynicks(return_array)
    char ***return_array;
{
    jmp_buf        save_jmp_buf;

    dprint(1, (debugfile, "- addr_book_manynicks -\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1,
	    (debugfile, "RESETTING address book...addr_book_manynicks!\n"));
	addrbook_reset();
    }

    (void)addr_book(SelectManyNicks, "SELECT NICKNAMES", return_array);
    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
}


static struct key ab_keys[] =
     {{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
      {NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
      {"P","PrevEntry",KS_NONE},	{"N","NextEntry",KS_NONE},
      {"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
      {"D","Delete",KS_DELETE},		{"A","AddNew",KS_NONE},
      {"C","ComposeTo",KS_COMPOSER},	{"W","WhereIs",KS_WHEREIS},
      {"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
      {"Q","Quit",KS_EXIT},		{NULL,NULL,KS_NONE},
      {"L","ListFldrs",KS_FLDRLIST},	{"G","GotoFldr",KS_GOTOFLDR},
      {"I","Index",KS_FLDRINDEX},	{NULL,NULL,KS_NONE},
      {"Y","prYnt",KS_PRINT},		{"T","TakeAddr",KS_TAKEADDR},
      {"X","eXport",KS_EXPORT},		{"F","Forward",KS_NONE}};
INST_KEY_MENU(ab_keymenu, ab_keys);
#define OTHER_KEY  1
#define MAIN_KEY   2
#define SELECT_KEY 3
#define DELETE_KEY 8
#define ADD_KEY    9
#define SENDTO_KEY 10
#define TAKE_KEY   21
#define FORW_KEY   23


/*
 *  Main address book screen 
 *
 * Control loop for address book.  Commands are executed out of a big
 * switch and screen painting is done.
 * The argument controls whether or not it is called to return an address
 * to the composer, a nickname to the TakeAddr screen, or just for address
 * book maintenance.
 *
 * Args: style -- how we were called
 *
 * Return: might return a string for the composer to edit (if SelectAddrCom
 * style)
 *         or a nickname (if SelectNick),
 *	   or an array of nicknames (if SelectManyNicks).
 */
char *
addr_book(style, title, return_array)
    AddrBookArg style;
    char       *title;
    char     ***return_array;
{
    int		     r, c, orig_c, i,
		     command_line,
		     did_delete_flag,
                     quit,           /* loop control                         */
                     km_popped,      /* menu is popped up in blank menu mode */
		     current_changed_flag,  /* only current row needs update */
		     was_clickable_last_time, /* on CLICKHERE last time thru */
		     start_disp,     /* Paint from this line down (0 is top) */
		     rdonly,         /* cur addrbook read only               */
		     empty,          /* cur addrbook empty                   */
		     are_selecting,  /* called as ^T selector                */
		     from_composer,  /* from composer                        */
		     listmode_ok,    /* ok to do ListMode with this style    */
		     selecting_nick, /* selecting nickname(s)                */
		     selecting_one_nick,
		     selecting_mult_nicks,
		     no_fullname,    /* return address without fullname      */
		     checkedn,       /* how many are checked                 */
		     def_cmd,        /* default command                      */
                     warped;         /* we warped through hyperspace to a
				        new location in the display list     */
    long	     fl,
		     new_ent,
		     new_top_ent,    /* entry on top of screen after oper    */
		     new_line;       /* new line number after operation      */
    char            *addr;
    bitmap_t         bitmap;
    struct key_menu *km;
    OtherMenu        what;
    PerAddrBook     *pab;
    AddrScrn_Disp   *dl;
    struct pine     *ps;
    Pos              cursor_pos;


    dprint(1, (debugfile, "--- addr_book ---  (%s)\n",
		  style==AddrBookScreen      ? "AddrBookScreen"            :
		   style==SelectAddrCom       ? "SelectAddrCom"            :
		    style==SelectAddrLccCom    ? "SelectAddrLccCom"        :
		     style==SelectNicksCom      ? "SelectNicksCom"         :
		      style==SelectAddr          ? "SelectAddr"            :
		       style==SelectAddrTake      ? "SelectAddrTake"       :
			style==SelectAddrNoFullCom ? "SelectAddrNoFullCom" :
			 style==SelectNick          ? "SelectNick"         :
			  style==SelectNickTake      ? "SelectNickTake"    :
			   style==SelectNickCom       ? "SelectNickCom"    :
			    style==SelectManyNicks     ? "SelectManyNicks" :
			                                  "UnknownStyle"));

    ps = ps_global;
    km = &ab_keymenu;

    from_composer  = (style == SelectAddrCom || style == SelectAddrLccCom
		      || style == SelectNicksCom || style == SelectNickCom
		      || style == SelectAddrNoFullCom);
    are_selecting  = (style != AddrBookScreen);
    selecting_one_nick = (style == SelectNick || style == SelectNickTake
		          || style == SelectNickCom);
    selecting_mult_nicks = (style == SelectAddrLccCom
			    || style == SelectNicksCom
			    || style == SelectManyNicks);
    selecting_nick = selecting_one_nick || selecting_mult_nicks;
    listmode_ok    = (style == SelectAddrCom || style == SelectAddrLccCom
		      || style == SelectNicksCom || style == SelectManyNicks);
    no_fullname    = (style == SelectAddrTake || style == SelectAddrNoFullCom);
    as.checkboxes  = (style == SelectManyNicks);  /* auto ListMode */

    def_cmd = F_ON(F_USE_FK,ps_global) ? PF4 : are_selecting ? 's' : 'c';

    /* Coming in from the composer, may need to reset the window */
    if(from_composer){
	fix_windsize(ps_global);
	init_sigwinch();
	mark_status_dirty();
	mark_titlebar_dirty();
	mark_keymenu_dirty();
    }

    command_line = -FOOTER_ROWS(ps_global); /* third line from the bottom */
    what         = FirstMenu;

    if(!init_addrbooks(HalfOpen, 1, 1, !are_selecting)){
	q_status_message(SM_ORDER | SM_DING,3,4,"No Address Book Configured");
	if(!are_selecting)
	  ps_global->next_screen = ps_global->prev_screen;

        return NULL;
    }


    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	/* nothing checked to start */
	if(pab->address_book && pab->address_book->checks)
	  checked_free(pab->address_book->checks);

	init_disp_form(pab, ps_global->VAR_ABOOK_FORMATS, i);
    }

    (void)calculate_field_widths();

    quit                     = 0;
    km_popped		     = 0;
    ps->mangled_screen       = 1;
    current_changed_flag     = 0;
    start_disp               = 0;
    was_clickable_last_time  = 0; /* doesn't matter what it is */
    checkedn                 = 0;

    c = 'x'; /* For display_message the first time through */


    while(!quit){
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps_global);
		/*
		 * Have to repaint from earliest change down, including
		 * at least the last two body lines.
		 */
		if(ps_global->mangled_body) /* it was already mangled */
		  start_disp = min(start_disp, as.l_p_page -2);
		else if(current_changed_flag){
		    ps_global->mangled_body = 1;
		    start_disp = min(min(as.cur_row, as.l_p_page -2),
					as.old_cur_row);
		}
		else{
		    ps_global->mangled_body = 1;
		    start_disp = as.l_p_page -2;
		}
	    }
	}

	ps_global->redrawer = redraw_addr_screen;

        if(new_mail(0, c == NO_OP_IDLE ? 0 : 2, 1) >= 0)
	  ps->mangled_header = 1;

        if(streams_died())
          ps->mangled_header = 1;

	if(ps->mangled_screen){
	    ps->mangled_header   = 1;
	    ps->mangled_body     = 1;
	    ps->mangled_footer   = 1;
	    start_disp           = 0;
	    current_changed_flag = 0;
	    ps->mangled_screen   = 0;
	}

	if(ps->mangled_header){
            set_titlebar(title, ps_global->mail_stream,
                         ps_global->context_current, ps_global->cur_folder,
                         ps_global->msgmap, 1,
                         FolderName, 0, 0);
	    ps->mangled_header = 0;
	}

	if(ps->mangled_body){
	    int old_cur;

	    if(calculate_field_widths())
	      start_disp = 0;

	    display_book(start_disp,
			 as.cur_row,  
			 as.old_cur_row,  
			 1,
			 &cursor_pos);

	    as.old_cur_row   = as.cur_row;
	    ps->mangled_body = 0;
            start_disp       = 0;
	    old_cur          = as.cur;
	    as.cur           = cur_addr_book();
	    pab              = &as.adrbks[as.cur];
	    if(as.cur != old_cur)
	      q_status_message1(SM_ORDER, 0, 2, "Now in addressbook %s",
						pab->nickname);
#ifdef	_WINDOWS
	    {
		long i;

		for(i = as.top_ent; dlist(i)->type != End; i++)
		  ;

		scroll_setrange((as.last_ent = i) - 1L);
	    }
#endif
	}
	/* current entry has been changed */
	else if(current_changed_flag){
	    int old_cur;
	    int need_redraw;

	    need_redraw = calculate_field_widths();

	    /*---------- Update the current entry, (move or change) -------*/
	    display_book(need_redraw ? 0 : as.cur_row,
			 as.cur_row,  
			 as.old_cur_row,  
			 need_redraw,
			 &cursor_pos);

	    as.old_cur_row       = as.cur_row;
	    current_changed_flag = 0;
	    old_cur              = as.cur;
	    as.cur               = cur_addr_book();
	    pab                  = &as.adrbks[as.cur];
	    if(as.cur != old_cur)
	      q_status_message1(SM_ORDER, 0, 2, "Now in addressbook %s",
						pab->nickname);
        }

	dprint(9, (debugfile, "addr_book: top of loop, addrbk %d top_ent %ld cur_row %d\n", as.cur, as.top_ent, as.cur_row));

	/*
	 * This is a check to catch the case where we move from a non-
	 * clickable row into a clickable row or vice versa.  That means
	 * the footer changes.
	 */
	if(!ps->mangled_footer && ((was_clickable_last_time &&
		!entry_is_clickable(as.top_ent+as.cur_row)) ||
	   (!was_clickable_last_time &&
		entry_is_clickable(as.top_ent+as.cur_row))))
	    ps->mangled_footer = 1;

	was_clickable_last_time = entry_is_clickable(as.top_ent+as.cur_row);

        if(ps->mangled_footer){

	    setbitmap(bitmap);
	    if(are_selecting){
		km->how_many = 1;
		ab_keys[MAIN_KEY].name   = "E";
		ab_keys[MAIN_KEY].label  = "ExitSelect";
		KS_OSDATASET(&ab_keys[MAIN_KEY], KS_EXITMODE);
		ab_keys[SELECT_KEY].name  = "S";
		ab_keys[SELECT_KEY].label = "[Select]";
		def_cmd = F_ON(F_USE_FK,ps_global) ? PF4 : 's';
		KS_OSDATASET(&ab_keys[SELECT_KEY], KS_NONE);
		clrbitn(OTHER_KEY, bitmap);
		clrbitn(TAKE_KEY, bitmap);
		clrbitn(FORW_KEY, bitmap);
		clrbitn(ADD_KEY, bitmap);
		clrbitn(SENDTO_KEY, bitmap);
		KS_OSDATASET(&ab_keys[DELETE_KEY], KS_NONE);
		if(as.checkboxes){
		    ab_keys[DELETE_KEY].name  = "X";
		    ab_keys[DELETE_KEY].label = "[Set/Unset]";
		    ab_keys[SELECT_KEY].label = "Select";
		    def_cmd = F_ON(F_USE_FK,ps_global) ? PF9 : 'x';
		    if(entry_is_clickable(as.top_ent+as.cur_row)){
			def_cmd = F_ON(F_USE_FK,ps_global) ? PF4 : 's';
			ab_keys[DELETE_KEY].label = "Set/Unset";
			ab_keys[SELECT_KEY].label = "[Select]";
		    }
		}
		else if(listmode_ok){
		    ab_keys[DELETE_KEY].name   = "L";
		    ab_keys[DELETE_KEY].label  = "ListMode";
		}
		else
		  clrbitn(DELETE_KEY, bitmap);
	    }
	    else{
		km->how_many = 2;
		ab_keys[MAIN_KEY].name   = "M";
		ab_keys[MAIN_KEY].label  = "Main Menu";
		KS_OSDATASET(&ab_keys[MAIN_KEY], KS_MAINMENU);
		if(entry_is_clickable(as.top_ent+as.cur_row)){
		    ab_keys[SELECT_KEY].name  = "S";
		    ab_keys[SELECT_KEY].label = "[Select]";
		    def_cmd = F_ON(F_USE_FK,ps_global) ? PF4 : 's';
		    KS_OSDATASET(&ab_keys[SELECT_KEY], KS_NONE);
		}
		else{
		    ab_keys[SELECT_KEY].name  = "V";
		    ab_keys[SELECT_KEY].label = "[View/Edit]";
		    def_cmd = F_ON(F_USE_FK,ps_global) ? PF4 : 'v';
		    KS_OSDATASET(&ab_keys[SELECT_KEY], KS_NONE);
		}

		ab_keys[DELETE_KEY].name   = "D";
		ab_keys[DELETE_KEY].label  = "Delete";
		KS_OSDATASET(&ab_keys[DELETE_KEY], KS_DELETE);
		if(was_clickable_last_time)  /* it's still *this* time now */
		  clrbitn(SENDTO_KEY, bitmap);
	    }

	    if(km_popped){
		FOOTER_ROWS(ps_global) = 3;
		clearfooter(ps_global);
	    }

	    draw_keymenu(km, bitmap, ps_global->ttyo->screen_cols,
				   1-FOOTER_ROWS(ps_global), 0, what, 0);
	    ps->mangled_footer = 0;
	    what               = SameTwelve;
	    if(km_popped){
		FOOTER_ROWS(ps_global) = 1;
		mark_keymenu_dirty();
	    }
	}

	rdonly   = (pab->access == ReadOnly);
	empty    = is_empty(as.cur_row+as.top_ent);
	if(as.no_op_possbl){
	    q_status_message(SM_ORDER | SM_DING, 0, 4,
		"No address book operations possible");
	}

	/*------------ display any status messages ------------------*/
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 3;
	    mark_status_unknown();
	}

	display_message(c);
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 1;
	    mark_status_unknown();
	}

	if(F_OFF(F_SHOW_CURSOR, ps_global)){
	    /* reset each time through to catch screen size changes */
	    cursor_pos.row =ps_global->ttyo->screen_rows-FOOTER_ROWS(ps_global);
	    cursor_pos.col = 0;
	}

	MoveCursor(cursor_pos.row, cursor_pos.col);


	/*---------------- Get command and validate -------------------*/
#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0);
	register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
		       ps_global->ttyo->screen_rows-(FOOTER_ROWS(ps_global)+1),
		       ps_global->ttyo->screen_cols);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(addr_scroll_callback);
#endif
	c = read_command();
#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(NULL);
#endif
        orig_c = c;

	if(c == ctrl('M') || c == ctrl('J')) /* set up default */
	  c = def_cmd;

	if(c < 'z' && isupper((unsigned char)c))
	  c = tolower((unsigned char)c);

	if(km->which == 1 && c >= PF1 && c <= PF12)
          c = PF2OPF(c);

        c = validatekeys(c); 

        dprint(5, (debugfile, "Addrbook command :'%c' (%d)\n", c, c));

	if(km_popped)
	  switch(c){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case PF2:
	    case OPF2:
	    case 'o' :
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps_global);
	      break;
	  }

	/*------------- execute command ----------------*/
	switch(c){

            /*------------ Noop   (new mail check) --------------*/
          case NO_OP_IDLE:
	  case NO_OP_COMMAND: 
	    break;


            /*----------- Help -------------------*/
	  case PF1:
	  case OPF1:
	  case '?':
	  case ctrl('G'):
	    if(FOOTER_ROWS(ps_global) == 1 && km_popped == 0){
		km_popped = 2;
		ps_global->mangled_footer = 1;
		break;
	    }

	    if(are_selecting){
		/* single nick select from TakeAddr */
		if(style == SelectNickTake)
		  helper(h_select_nickname_take, "HELP ON ADDRESS BOOK", 1);
		/* single nick select from addrbook */
		else if(selecting_one_nick)
		  helper(h_select_nickname, "HELP ON ADDRESS BOOK", 1);
		/* can use X checkbox command now */
		else if(as.checkboxes)
		  helper(h_use_address_bookx, "HELP ON ADDRESS BOOK", 1);
		/* ListMode command available */
		else if(listmode_ok)
		  helper(h_use_address_bookl, "HELP ON ADDRESS BOOK", 1);
		/* no ListMode command available */
		else
		  helper(h_select_addr, "HELP ON ADDRESS BOOK", 1);
	    }
	    /* general maintenance screen */
	    else{
		ps_global->next_screen = SCREEN_FUN_NULL;
		helper(h_address_book, "HELP ON ADDRESS BOOK", 0);
	    }

	    /*
	     * We need this next_screen test in order that helper() can
	     * have a return to Main Menu key.  If helper is called with
	     * a third argument of 1, that isn't one of the possibilities.
	     */
	    if(!are_selecting && ps_global->next_screen != SCREEN_FUN_NULL)
              quit = 1;

	    ps->mangled_screen = 1;
	    break;

             
            /*---------- display other key bindings ------*/
          case PF2:
          case OPF2:
          case 'o' :
            if(are_selecting)
              goto bleep;

            if(c == 'o')
	      warn_other_cmds();

            what = NextTwelve;
            ps->mangled_footer = 1;
            break;


            /*------------- Back to main menu or exit to caller -------*/
	  case PF3:
	  case 'm':
	  case 'e':
	    if(!are_selecting && c == 'e'){
	        /* backwards compatibility message */
		q_status_message(SM_ORDER | SM_DING, 0, 2,
	      "Command \"E\" not defined.  Use \"View/Edit\" to edit an entry");
		break;
	    }

	    if(are_selecting && c == 'm')
              goto bleep;

	    if(!are_selecting)
              ps_global->next_screen = main_menu_screen;

	    if(!(are_selecting && as.checkboxes && checkedn > 0)
	       || want_to("Really abandon your selections ",
			  'y', 'x', NO_HELP, 0, 0) == 'y')
	      quit = 1;

	    break;


            /*------- Select and Edit ---------------*/
	  case PF4:
	  case 's':
	    /* backwards compatibility message */
	    if(c == 's'
	      && !(are_selecting || entry_is_clickable(as.top_ent+as.cur_row))){
		q_status_message(SM_ORDER | SM_DING, 0, 2,
		 "Command \"S\" not defined.  Use \"AddNew\" to create a list");
		break;
	    }

	  select:
	    if(c == PF4 && !are_selecting
	       && !entry_is_clickable(as.top_ent+as.cur_row))
	      goto edit;

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		DL_CACHE_S *dlc_to_flush;

		start_disp = as.cur_row;  /* will redraw from here down */
		dlc_to_flush = get_dlc(as.top_ent+as.cur_row);
		if(dlc_to_flush->type == DlcClickHere){

		    /*
		     * open this addrbook and fill in display list
		     */

		    /* flush the CLICKHERE line from dlc cache */
		    flush_dlc_from_cache(dlc_to_flush);
		    init_abook(pab, Open);
		    if(!are_selecting && pab->access == ReadOnly)
		      readonly_warning(NO_DING, pab->nickname);
		}
		else{

		    /*
		     * expand this distribution list
		     */

		    /*
		     * Mark this list expanded, then flush the
		     * ListCLICKHERE line from dlc cache.  When we get the
		     * line again it will notice the expanded flag and change
		     * the type to DlcListEnt (if any entries).  The check
		     * in the if should be unnecessary, since we won't get
		     * here if it is turned on.
		     */
		    if(F_OFF(F_EXPANDED_DISTLISTS,ps_global))
		      exp_set_expanded(pab->address_book->exp,
			(a_c_arg_t)dlc_to_flush->dlcelnum);

		    flush_dlc_from_cache(dlc_to_flush);
		    dlc_to_flush = get_dlc(as.top_ent+as.cur_row);
		    if(dlc_to_flush->type == DlcListEmpty){
			as.cur_row--;
			start_disp--;
			if(as.cur_row < 0){
			    as.top_ent--;
			    as.cur_row = 0;
			    start_disp  = 0;
			}
		    }
		}

		ps->mangled_body = 1;
	    }
	    else if(are_selecting){
              /* Select an entry to mail to or a nickname to add to */
	      if(!any_addrs_avail(as.top_ent+as.cur_row)){
	          q_status_message(SM_ORDER | SM_DING, 0, 4,
	   "No entries in address book. Use ExitSelect to leave address book");
	          break;
	      }

	      if(as.checkboxes || is_addr(as.top_ent+as.cur_row)){
		  BuildTo        bldto;
		  char          *to    = NULL;
		  char          *error = NULL;
		  AdrBk_Entry   *abe;

		  dl = dlist(as.top_ent+as.cur_row);

		  if(selecting_one_nick
		     || (selecting_mult_nicks && return_array
			 && !as.checkboxes)){
		      char nickbuf[MAX_NICKNAME + 1];

		      strncpy(nickbuf,
			  ae(as.top_ent+as.cur_row)->nickname,
			  MAX_NICKNAME);
		      if(selecting_one_nick)
			return(cpystr(nickbuf));
		      else{
			  *return_array = (char **)fs_get(2 * sizeof(char *));
			  memset((void *)*return_array, 0, 2 * sizeof(char *));
			  (*return_array)[0] = cpystr(nickbuf);
			  return(NULL);
		      }
		  }
		  else if(as.checkboxes && checkedn <= 0){
		      q_status_message(SM_ORDER, 0, 1,
			"Use \"X\" to mark addresses or lists");
		      break;
		  }
		  else if(as.checkboxes){
		      size_t incr = 100, avail, alloced;
		      int ind;

		      /*
		       * Have to run through all of the checked entries
		       * in all of the address books.
		       * Put the nicknames together into one long
		       * string with comma separators and let 
		       * our_build_address handle the parsing.
		       */
		      if(selecting_mult_nicks && return_array){
			  *return_array = (char **)fs_get((checkedn+1)
							* sizeof(char *));
			  memset((void *)*return_array, 0, (checkedn+1)
							   * sizeof(char *));
			  ind = 0;
		      }
		      else{
			  to = (char *)fs_get(incr);
			  *to = '\0';
			  avail = incr;
			  alloced = incr;
		      }

		      for(i = 0; i < as.n_addrbk; i++){
			  EXPANDED_S *next_one;
			  adrbk_cntr_t num;
			  AddrScrn_Disp fake_dl;
			  char *a_string;

			  pab = &as.adrbks[i];
			  if(pab->address_book)
			    next_one = pab->address_book->checks;
			  else
			    continue;

			  while((num = entry_get_next(&next_one)) != NO_NEXT){
			      abe = adrbk_get_ae(pab->address_book,
						 (a_c_arg_t)num, Normal);
			      /*
			       * Since we're picking up address book entries
			       * directly from the address books and have
			       * no knowledge of the display lines they came
			       * from, we don't know the dl's that go with
			       * them.  We need to pass a dl to abe_to_nick
			       * but it really is only going to use the
			       * type in this case.
			       */
			      dl = &fake_dl;
			      dl->type = (abe->tag == Single) ? Simple
							      : ListHead;
			      a_string = abe_to_nick_or_addr_string(abe,
							 dl, pab->address_book);
			      if(selecting_mult_nicks && return_array){
				  (*return_array)[ind++] = a_string;
			      }
			      else{
				  while(abe && avail
					  < (size_t)strlen(a_string)+1){
				      alloced += incr;
				      avail   += incr;
				      fs_resize((void **)&to, alloced);
				  }

				  if(!*to)
				    strcpy(to, a_string);
				  else{
				      strcat(to, ",");
				      strcat(to, a_string);
				  }

				  avail -= (strlen(a_string) + 1);
			      }
			  }
		      }

		      /*
		       * Return the nickname list for lcc so that the
		       * correct fullname can make it to the To line.
		       * If we expand it ahead of time, the list name
		       * and first user's fullname will get mushed together.
		       * If an entry doesn't have a nickname then we're
		       * out of luck as far as getting the right entry
		       * in the To line goes.
		       */
		      if(selecting_mult_nicks){
			  if(return_array)
			    return(NULL); /* return value is ignored */
			  else
			    return(to);
		      }

		      bldto.type    = Str;
		      bldto.arg.str = to;
		  }
		  else{
		      /* Select an address, but not using checkboxes */
		      if(selecting_mult_nicks){
			if(dl->type != ListHead && style == SelectAddrLccCom){
			    q_status_message(SM_ORDER, 0, 4,
	  "You may only select lists for lcc, use bcc for other addresses");
			    break;
			}
			else{
			    /*
			     * Even though we're supposedly selecting
			     * nicknames, we have a special case here to
			     * select a single member of a distribution
			     * list.  This happens with style SelectNicksCom
			     * which is the regular ^T entry from the
			     * composer, and it allows somebody to mail to
			     * a single member of a distribution list.
			     */
				abe = ae(as.top_ent+as.cur_row);
			    return(abe_to_nick_or_addr_string(abe, dl,
							    pab->address_book));
			    }
			}
		      else{
			  if(dl->type == ListEnt){
			      bldto.type    = Str;
			      bldto.arg.str =
				      listmem_from_dl(pab->address_book, dl);
			  }
			  else if(dl->type == ListHead && no_fullname){
			      q_status_message(SM_ORDER, 0, 4,
	  "You may not select a list, select a single address instead");
			      break;
			  }
			  else{
			      bldto.type    = Abe;
			      bldto.arg.abe = ae(as.top_ent+as.cur_row);
			  }
		      }
		  }
		      
		  if(no_fullname){
		      if(bldto.type == Str){
			  char *q, *p, *t;

			  q = cpystr(bldto.arg.str);
			  /*
			   * make an attempt to remove full name if it's there
			   */
			  if((p = strrindex(q, '>')) == NULL)
			    addr = q;
			  else{
			      *p = '\0';
			      if((t = strrindex(q, '<')) != NULL){
				  addr = cpystr(t+1);
				  fs_give((void **)&q);
				  removing_leading_white_space(addr);
				  removing_trailing_white_space(addr);
			      }
			      else{
				  *p = '>';
				  addr = q;
			      }
			  }
		      }
		      else
		        addr = cpystr((bldto.arg.abe &&
						bldto.arg.abe->addr.addr)
					? bldto.arg.abe->addr.addr : "");
		  }
		  else{
		      (void)our_build_address(bldto, &addr, &error, NULL, 1, 0);
		      /* Have to rfc1522_decode the addr */
		      if(addr){
			  char *p, *dummy = NULL;

			  p = (char *)fs_get((strlen(addr) + 1) * sizeof(char));
			  if(rfc1522_decode((unsigned char *)p, addr, &dummy)
						      == (unsigned char *)p){
			      fs_give((void **)&addr);
			      addr = p;
			  }
			  else
			    fs_give((void **)&p);
			  
			  if(dummy)
			    fs_give((void **)&dummy);
		      }
		  }

		  if(to)
		    fs_give((void **)&to);

		  if(error){
		      q_status_message1(SM_ORDER, 3, 4, "%s", error);
		      fs_give((void **)&error);
		  }

		  return(addr);  /* Caller frees this */
	      }
	      else{
	          q_status_message1(SM_ORDER, 3, 4, "No %s selected",
		      selecting_nick ? "nickname" : "address");
	          break;
	      }
	    }

	    break;


            /*----- Edit existing or Add new ---------*/
	  case 'a':
          case PF10:
          case 'v':
	    if(are_selecting)
	      goto bleep;
edit:
	    if((c == 'v' || c == PF4 || c == KEY_MOUSE)
	      && !any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to view");
                break;
            }

	    if(((dl=dlist(as.top_ent+as.cur_row))->type == ClickHere)
	      || ((c == 'v' || c == PF4 || c == KEY_MOUSE)
		  && dl->type == ListClickHere)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(rdonly && (c == 'a' || c == PF10)){
		readonly_warning(NO_DING, NULL);
                break;
	    }

	    if(empty && (c == 'v' || c == PF4 || c == KEY_MOUSE)){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    warped = 0;
	    if(c == 'a' || c == PF10) 
	      edit_entry(pab->address_book, (AdrBk_Entry *)NULL, NO_NEXT,
							  NotSet, 0, &warped);
	    else{
		if(is_addr(as.cur_row+as.top_ent)){
		    AdrBk_Entry *abe, *abe_copy;
		    a_c_arg_t entry;

		    entry = dl->elnum;
		    abe = adrbk_get_ae(pab->address_book, entry, Normal);
		    abe_copy = adrbk_newentry();
		    abe_copy->nickname
		       = abe->nickname ? cpystr(abe->nickname) : abe->nickname;
		    abe_copy->fullname
		       = abe->fullname ? cpystr(abe->fullname) : abe->fullname;
		    abe_copy->fcc
		       = abe->fcc ? cpystr(abe->fcc) : abe->fcc;
		    abe_copy->extra
		       = abe->extra ? cpystr(abe->extra) : abe->extra;
		    abe_copy->tag = abe->tag;
		    if(abe_copy->tag == Single)
		      abe_copy->addr.addr = cpystr(abe->addr.addr);
		    else{
			char **p;
			size_t size;
			int j;

			/* copy list */
			/* count up size of list */
			for(p = abe->addr.list; p != NULL && *p != NULL; p++)
			  ;/* do nothing */
			
			size = p - abe->addr.list;
			abe_copy->addr.list = (char **)fs_get((size+1)
							    * sizeof(char *));
			for(j = 0; j < size; j++)
			  abe_copy->addr.list[j] = cpystr(abe->addr.list[j]);

			abe_copy->addr.list[size] = NULL;
		    }

		    edit_entry(pab->address_book, abe_copy, entry,
			       abe->tag, rdonly, &warped);
		    free_ae(pab->address_book, &abe_copy);
		}
		else{
		    q_status_message(SM_ORDER, 0, 3,
				     "Current line is not editable");
		    break;
		}
	    }

	    /*
	     * Warped means we got plopped down somewhere in the display
	     * list so that we don't know where we are relative to where
	     * we were before we warped.  The current line number will
	     * be zero, since that is what the warp would have set.
	     */
	    {long old_l_p_p, old_top_ent, old_cur_row;

		if(warped){
		    as.top_ent = first_line(0L - as.l_p_page/2L);
		    as.cur_row = 0L - as.top_ent;
		}
		else{
		    /*
		     * If we didn't warp, that means we didn't change at all,
		     * so keep old screen.
		     */
		    old_l_p_p   = as.l_p_page;
		    old_top_ent = as.top_ent;
		    old_cur_row = as.cur_row;
		}

		/* Window size may have changed while in pico. */
		ab_resize();

		/* fix up what ab_resize messed up */
		if(!warped && old_l_p_p == as.l_p_page){
		    as.top_ent     = old_top_ent;
		    as.cur_row     = old_cur_row;
		    as.old_cur_row = old_cur_row;
		}
	    }

	    ps->mangled_screen = 1;
	    break;


            /*----------------------- Move Up ---------------------*/
          case PF5:
	  case 'p':
          case ctrl('P'):
          case KEY_UP:
	    if(any_addrs_avail(as.top_ent+as.cur_row)){

		r = prev_selectable_line(as.cur_row+as.top_ent, &new_line);
		if(r == 0){
		    q_status_message(SM_INFO, 0, 1, "Already on first line.");
		    break;
		}

		i = ((c == ctrl('P') || c == KEY_UP)
		     && dlist(as.top_ent - 1L)->type != Beginning)
		      ? min(HS_MARGIN(ps), as.cur_row) : 0;
                as.cur_row = new_line - as.top_ent;
                if(as.cur_row - i < 0){
		    if(c == ctrl('P') || c == KEY_UP){
			/*-- Past top of page --*/
			as.top_ent += (as.cur_row - i);
			as.cur_row  = i;
		    }
		    else{
			new_top_ent = first_line(as.top_ent - as.l_p_page);
			as.cur_row += (as.top_ent - new_top_ent);
			as.top_ent  = new_top_ent;
		    }

		    start_disp       = 0;
		    ps->mangled_body = 1;
                }
		else
                  current_changed_flag++;
	    }
	    else
	      empty_warning(as.top_ent+as.cur_row);

	    break;


            /*------------------- Move Down -------------------*/
          case PF6:
          case '\t':
          case 'n':
          case ctrl('N'):
          case KEY_DOWN:
	    if(any_addrs_avail(as.top_ent+as.cur_row)){

		r = next_selectable_line(as.cur_row+as.top_ent, &new_line);
		if(r == 0){
		    q_status_message(SM_INFO, 0, 1, "Already on last line.");
		    break;
		}

		/* adjust for scrolling margin */
		i = (c == ctrl('N') || c == KEY_DOWN) ? HS_MARGIN(ps) : 0;
                as.cur_row = new_line - as.top_ent;
		if(as.cur_row >= as.l_p_page - i){
		    if(c == ctrl('N') || c == KEY_DOWN){
			/*-- Past bottom of page --*/
			as.top_ent += (as.cur_row - (as.l_p_page - i) + 1);
			as.cur_row  = (as.l_p_page - i) - 1;
		    }
		    else{
			/*-- Changed pages --*/
			as.top_ent += as.l_p_page;
			as.cur_row -= as.l_p_page;
		    }

		    start_disp       = 0;
		    ps->mangled_body = 1;
                }
		else
                  current_changed_flag++;
	    }
	    else
	      empty_warning(as.top_ent+as.cur_row);

	    break;


#ifdef MOUSE		
	  case KEY_MOUSE:
	    {
		MOUSEPRESS mp;
	    
		/*
		 * Get the mouse down.  Convert to content row number.
		 * If the row is selectable, do the single or double click
		 * operation.
		 */
		mouse_get_last(NULL, &mp);
		mp.row -= HEADER_ROWS(ps_global);
		if(line_is_selectable(as.top_ent + mp.row)){
		    if(mp.doubleclick){
			if(are_selecting && as.checkboxes
			   && !entry_is_clickable(as.top_ent+as.cur_row))
			  goto togglex;
			else if(!are_selecting
			   && !entry_is_clickable(as.top_ent+as.cur_row))
			  goto edit;
			else
			  goto select;
		    }

		    as.cur_row = mp.row;
		    current_changed_flag++;
		}
	    }
	    break;
#endif	    


            /*------------- Page Up ----------------*/
	  case '-':
          case ctrl('Y'): 
	  case PF7:
	  case KEY_PGUP:
            /*------------- Page Down --------------*/
          case SPACE:
          case ctrl('V'): 
          case '+':		    
	  case PF8:
	  case KEY_PGDN:
	    /* if Up */
	    if(c == '-' || c == ctrl('Y') || c == PF7 || c == KEY_PGUP){
		/* find first line on prev page */
		new_top_ent = first_line(as.top_ent - as.l_p_page);
		if(new_top_ent == NO_LINE)
		    break;

		/* find first selectable line */
		fl = first_selectable_line(new_top_ent);
		if(fl == NO_LINE)
		    break;

		if(as.top_ent == new_top_ent && as.cur_row == (fl-as.top_ent)){
		    q_status_message(SM_INFO, 0, 1, "Already on first page.");
		    break;
		}

		if(as.top_ent == new_top_ent)
                    current_changed_flag++;
		else
		    as.top_ent = new_top_ent;
	    }
	    /* else Down */
	    else{
		/* find first selectable line on next page */
		fl = first_selectable_line(as.top_ent + as.l_p_page);
		if(fl == NO_LINE)
		    break;

		/* if there is another page, scroll */
		if(fl - as.top_ent >= as.l_p_page){
		    new_top_ent = as.top_ent + as.l_p_page;
		}
		/* on last page already */
		else{
		    new_top_ent = as.top_ent;
		    if(as.cur_row == (fl - as.top_ent)){ /* no change */
			q_status_message(SM_INFO,0,1,"Already on last page.");
			break;
		    }
		}

		if(as.top_ent == new_top_ent)
                    current_changed_flag++;
		else
		    as.top_ent = new_top_ent;
	    }

	    as.cur_row = fl - as.top_ent;
	    if(!current_changed_flag){
		ps->mangled_body = 1;
		start_disp  = 0;
	    }

	    break;


	    /*------------- Delete item from addrbook ---------*/
	  case PF9:
	  case 'd': 
	    if(as.checkboxes && c == PF9)
	      goto togglex;
	    
	    if(!as.checkboxes && c == PF9 && listmode_ok)
	      goto listmodeon;

	    if(are_selecting)
	      goto bleep;

	    if(!any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to delete");
                break;
	    }

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(rdonly){
		readonly_warning(NO_DING, NULL);
		break;
	    }

	    if(empty){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    warped = 0;
            did_delete_flag = addr_book_delete(pab->address_book,
					       command_line,
                                               as.cur_row+as.top_ent,
                                               &warped);
	    ps->mangled_footer = 1;
	    if(did_delete_flag){
		if(warped){
		    as.top_ent = first_line(0L - as.l_p_page/2L);
		    as.cur_row = 0L - as.top_ent;
		    start_disp = 0;
		}
		else{
		    /*
		     * In case the line we're now at is not a selectable
		     * field.
		     */
		    new_line = first_selectable_line(as.cur_row+as.top_ent);
		    if(new_line != NO_LINE
		       && new_line != as.cur_row+as.top_ent){
			as.cur_row = new_line - as.top_ent;
			if(as.cur_row < 0){
			    as.top_ent -= as.l_p_page;
			    as.cur_row += as.l_p_page;
			}
			else if(as.cur_row >= as.l_p_page){
			    as.top_ent += as.l_p_page;
			    as.cur_row -= as.l_p_page;
			}
		    }

		    start_disp = min(as.cur_row, as.old_cur_row);
		}

	        ps->mangled_body = 1;
	    }

            break;


	    /*------------- Toggle checkbox ---------*/
	  case 'x': 
	  togglex:
            if(!are_selecting)
              goto export;

	    if(!as.checkboxes)
	      goto bleep;

	    if(!any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to select");
                break;
	    }

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(empty){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    if(is_addr(as.top_ent+as.cur_row)){
		dl = dlist(as.top_ent+as.cur_row);

		if(style == SelectAddrLccCom && dl->type != ListHead)
		  q_status_message(SM_ORDER, 0, 4,
	  "You may only select lists for lcc, use bcc for personal entries");
		else if(dl->type == ListHead || dl->type == Simple){
                    current_changed_flag++;
		    if(entry_is_checked(pab->address_book->checks,
				        (a_c_arg_t)dl->elnum)){
			entry_unset_checked(pab->address_book->checks,
					    (a_c_arg_t)dl->elnum);
			checkedn--;
		    }
		    else{
			entry_set_checked(pab->address_book->checks,
					    (a_c_arg_t)dl->elnum);
			checkedn++;
		    }
		}
		else
		  q_status_message(SM_ORDER, 0, 4,
      "You may not select list members, only whole lists or personal entries");
	    }
	    else
              q_status_message(SM_ORDER, 0, 4,
		  "You may only select addresses or lists");

            break;


	    /*---------- Turn on ListMode -----------*/
	  listmodeon:
	    as.checkboxes = 1;
	    for(i = 0; i < as.n_addrbk; i++){
		pab = &as.adrbks[i];
		init_disp_form(pab, ps_global->VAR_ABOOK_FORMATS, i);
	    }

	    (void)calculate_field_widths();
	    ps->mangled_footer = 1;
	    ps->mangled_body = 1;
	    start_disp  = 0;
            q_status_message(SM_ORDER, 0, 4,
		  "Use \"X\" to select addresses or lists");
            break;


            /*--------- Compose -----------*/
          case PF11:
	  case 'c':
	    if(are_selecting)
	      goto bleep;

	    ab_compose_to_addr(as.top_ent+as.cur_row);
	    /*
	     * Window size may have changed in composer.
	     * Pine_send will have reset the window size correctly,
	     * but we still have to reset our data structures.
	     */
	    ab_resize();
	    ps->mangled_screen = 1;
            break;


            /*----------- Where is (search) ----------------*/
	  case PF12:
	  case 'w':
	  case ctrl('W'):
	    warped = 0;
	    new_top_ent = ab_whereis(&warped, command_line);

	    if(new_top_ent != NO_LINE){
		if(warped || new_top_ent != as.top_ent){
		    as.top_ent     = new_top_ent;
		    start_disp     = 0;
		    ps->mangled_body = 1;
		}
		else
		    current_changed_flag++;
	    }

	    ps->mangled_footer = 1;
	    break;


            /*--------- QUIT pine -----------*/
          case OPF3:
	  case 'q':
            if(are_selecting)
              goto bleep;

            ps_global->next_screen = quit_screen;
	    quit = 1;
            break;


            /*--------- Folders -----------*/
          case OPF5:
	  case 'l':
	    if(!as.checkboxes && c == 'l' && listmode_ok)
	      goto listmodeon;

            if(are_selecting)
              goto bleep;

            ps_global->next_screen = folder_screen;
	    quit = 1;
            break;


            /*---------- Open specific new folder ----------*/
          case OPF6:
	  case 'g':
            if(are_selecting || ps_global->nr_mode)
              goto bleep;

	    ab_goto_folder(command_line);
	    quit = 1;
            break;


            /*--------- Index -----------*/
          case OPF7:
	  case 'i':
            if(are_selecting)
              goto bleep;

            ps_global->next_screen = mail_index_screen;
	    quit = 1;
            break;


	    /*----------------- Print --------------------*/
	  case OPF9: 
	  case 'y':
	    ab_print();
	    ps->mangled_screen = 1;
            break;


	    /*------ Take from one to another -------------*/
	  case OPF10: 
	  case 't':
	    {
	    AdrBk_Entry   *abe;
	    adrbk_cntr_t   new_entry_num;
	    AddrScrn_Disp  save_dl;
	    DL_CACHE_S     dlc_restart;
	    char          *save_nick = NULL,
			  *save_addr = NULL;
	    int            found_old_one = 0;

	    if(are_selecting)
	      goto bleep;

	    if(!any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to take");
                break;
	    }

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(empty){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    /*
	     * Save some state information so that we can probably
	     * redraw the screen halfway intelligently.
	     */
	    dl = dlist(as.top_ent+as.cur_row);
	    save_dl.type = dl->type;
	    abe = ae(as.top_ent+as.cur_row);
	    if(abe && abe->nickname)
	      save_nick = cpystr(abe->nickname);

	    switch(save_dl.type){
	      case ListEnt:
		save_dl.l_offset = dl->l_offset;
		if(abe && abe->addr.list &&
		   listmem_count_from_abe(abe) > save_dl.l_offset &&
		   abe->addr.list[save_dl.l_offset])
		  save_addr = cpystr(abe->addr.list[save_dl.l_offset]);

		/* fall through */

	      case Simple:
	      case ListHead:
		save_dl.elnum = dl->elnum;
		break;
	    }

            internal_take(pab->address_book, as.cur_row+as.top_ent);
	    
	    /*
	     * We want to try to position the cursor at the same place it
	     * was before the take.  Depending on what happened during the
	     * take, the entry we were looking at before could now be at
	     * the same elnum in the addrbook, or at + or -1 from that
	     * spot.  Look for that old nickname.  If we can't find it,
	     * that means we did a take to the same nickname we were taking
	     * from and changed something so that it sorts differently now.
	     * That should be rare.  In that case, just go with elnum.
	     * In any case, we don't have enough information to redraw the
	     * screen in the same place as it was before, so we center the
	     * place we were before in the middle of the screen.
	     */
	    if(save_nick &&
	       (abe = /* that is supposed to be a single '=' */
		     adrbk_get_ae(pab->address_book,
			 (a_c_arg_t)save_dl.elnum, Normal))
		     && abe->nickname
		     && strcmp(save_nick, abe->nickname) == 0){
		new_entry_num = save_dl.elnum;
		found_old_one++;
	    }
	    else if(save_nick &&
		    (abe =
	           adrbk_get_ae(pab->address_book,
		       (a_c_arg_t)(save_dl.elnum+1), Normal))
		       && abe->nickname
		       && strcmp(save_nick, abe->nickname) == 0){
		new_entry_num = save_dl.elnum + 1;
		found_old_one++;
	    }
	    else if(save_nick &&
		    (abe =
	           adrbk_get_ae(pab->address_book,
		       (a_c_arg_t)(save_dl.elnum-1), Normal))
		       && abe->nickname
		       && strcmp(save_nick, abe->nickname) == 0){
		new_entry_num = save_dl.elnum - 1;
		found_old_one++;
	    }
	    else{
		abe = adrbk_get_ae(pab->address_book,
		    (a_c_arg_t)save_dl.elnum, Normal);
		new_entry_num = save_dl.elnum;
	    }

	    /*
	     * Build a dlc and warp to it.
	     */
	    if(abe && abe->tag == Single){
		dlc_restart.adrbk_num = as.cur;
		dlc_restart.dlcelnum  = new_entry_num;
		dlc_restart.type      = DlcSimple;
		warp_to_dlc(&dlc_restart, 0L);
		/* put current entry in middle of screen */
		as.top_ent = first_line(0L - (long)as.l_p_page/2L);
		as.cur_row = 0L - as.top_ent;
	    }
	    else if(abe && abe->tag == List){

		dlc_restart.adrbk_num = as.cur;
		dlc_restart.dlcelnum  = new_entry_num;

		if(found_old_one &&
		   save_dl.type == ListEnt &&
		   save_addr &&
		   listmem_count_from_abe(abe) > save_dl.l_offset &&
		   abe->addr.list &&
		   abe->addr.list[save_dl.l_offset] &&
		   strcmp(abe->addr.list[save_dl.l_offset], save_addr)==0){

		    dlc_restart.type      = DlcListEnt;
		    dlc_restart.dlcoffset = save_dl.l_offset;
		}
		else
		  dlc_restart.type      = DlcListHead;

		warp_to_dlc(&dlc_restart, 0L);
		/* put current entry in middle of screen */
		as.top_ent = first_line(0L - (long)as.l_p_page/2L);
		as.cur_row = 0L - as.top_ent;
	    }
	    else{
		/* No abe to go to, shouldn't happen. */
		warp_to_beginning();
		as.cur         = 0;
		as.top_ent     = 0L;
		new_ent        = first_selectable_line(0L);
		if(new_ent == NO_LINE)
		  as.cur_row = 0L;
		else
		  as.cur_row = new_ent;

		if(as.cur_row >= as.l_p_page)
		  as.top_ent += (as.cur_row - as.l_p_page + 1);
	    }

	    ps->mangled_screen = 1;
	    if(save_nick)
	      fs_give((void **)&save_nick);

	    if(save_addr)
	      fs_give((void **)&save_addr);
	    }

            break;


	  case OPF11: 
	  export:
	    if(!any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to export");
                break;
	    }

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(empty){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    if(!is_addr(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "Nothing to export");
                break;
	    }

	    ab_export(as.top_ent+as.cur_row, command_line);
            ps->mangled_footer = 1;
            break;


	  case OPF12: 
	  case 'f': 
	    if(!any_addrs_avail(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "No entries to forward");
                break;
	    }

	    if(entry_is_clickable(as.top_ent+as.cur_row)){
		clickable_warning(as.top_ent+as.cur_row);
		break;
	    }

	    if(empty){
		empty_warning(as.top_ent+as.cur_row);
                break;
	    }

	    if(!is_addr(as.top_ent+as.cur_row)){
                q_status_message(SM_ORDER, 0, 4, "Nothing to forward");
                break;
	    }

	    dl = dlist(as.top_ent+as.cur_row);
	    if(dl->type != ListHead && dl->type != Simple){
                q_status_message(SM_ORDER, 0, 4,
		    "Can only forward whole entries");
                break;
	    }

	    ab_forward(ps, as.top_ent+as.cur_row);
            ps->mangled_footer = 1;
            break;


          case KEY_RESIZE:
          case ctrl('L'):
	    mark_status_dirty();
	    mark_titlebar_dirty();
	    mark_keymenu_dirty();
	    ClearBody();
            ps->mangled_screen = 1;
	    if(c == KEY_RESIZE)
	      ab_resize();

	    break;


	  /* backwards compatibility message */
	  case 'z':
	    if(!are_selecting)
	      q_status_message(SM_ORDER | SM_DING, 0, 2,
	     "Command \"Z\" not defined.  Use \"View/Edit\" to add to a list");
	    else
	      goto bleep;

	    break;

	  default:
          bleep:
	    bogus_command(orig_c, F_ON(F_USE_FK,ps_global) ? "F1" : "?");
	    break;
	}
    }
    
    return NULL;
}


/*
 * Post a readonly addrbook warning.
 *
 * Args: bell -- Ring the bell
 *       name -- Include this addrbook name in warning.
 */
void
readonly_warning(bell, name)
    int        bell;
    char      *name;
{
    q_status_message2(SM_ORDER | (bell ? SM_DING : 0), 0, 4,
		      "AddressBook%s%s is Read Only",
		      name ? " " : "",
		      name ? name : "");
}


/*
 * Post an empty addrbook warning.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 */
void
empty_warning(cur_line)
    long cur_line;
{
    register AddrScrn_Disp *dl;

    dl = dlist(cur_line);
    q_status_message1(SM_ORDER, 0, 4, "%s is Empty", dl->type == Empty
						? "Address Book"
						: "Distribution List");
}


/*
 * Tell user to click on this to expand.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 */
void
clickable_warning(cur_line)
    long cur_line;
{
    register AddrScrn_Disp *dl;

    dl = dlist(cur_line);
    q_status_message1(SM_ORDER, 0, 4, "%s not expanded, use \"S\" to expand",
	dl->type == ClickHere ? "Address Book"
			      : "Distribution List");
}


/*
 * Post a cancellation warning.
 *
 * Args: bell -- Ring the bell
 *       what -- Text to display
 */
void
cancel_warning(bell, what)
    int   bell;
    char *what;
{
    q_status_message1(SM_INFO | (bell ? SM_DING : 0), 0, 2,
		      "Address book %s cancelled", what);
}


/*
 * Post a no tabs warning.
 */
void
no_tabs_warning()
{
    q_status_message(SM_ORDER, 0, 4, "Tabs not allowed in address book");
}


/*
 * 1522 encode the personal name portion of addr and return an allocated
 * copy of the resulting address string.
 */
char *
encode_fullname_of_addrstring(addr, charset)
    char *addr;
    char *charset;
{
    char    *pers_encoded,
            *tmp_a_string,
            *ret = NULL;
    ADDRESS *adr;
    static char *fakedomain = "@";

    tmp_a_string = cpystr(addr ? addr : "");
    adr = NULL;
    rfc822_parse_adrlist(&adr, tmp_a_string, fakedomain);
    fs_give((void **)&tmp_a_string);

    if(!adr)
      return(cpystr(""));

    if(adr->personal && adr->personal[0]){
	pers_encoded = cpystr(rfc1522_encode(tmp_20k_buf,
				(unsigned char *)adr->personal,
				charset));
	fs_give((void **)&adr->personal);
	adr->personal = pers_encoded;
    }

    ret = (char *)fs_get((size_t)est_size(adr));
    ret[0] = '\0';
    rfc822_write_address(ret, adr);
    mail_free_address(&adr);
    return(ret);
}


/*
 * 1522 decode the personal name portion of addr and return an allocated
 * copy of the resulting address string.
 */
char *
decode_fullname_of_addrstring(addr, verbose)
    char *addr;
    int   verbose;
{
    char    *pers_decoded,
            *tmp_a_string,
            *ret = NULL,
	    *dummy = NULL;
    ADDRESS *adr;
    static char *fakedomain = "@";

    tmp_a_string = cpystr(addr ? addr : "");
    adr = NULL;
    rfc822_parse_adrlist(&adr, tmp_a_string, fakedomain);
    fs_give((void **)&tmp_a_string);

    if(!adr)
      return(cpystr(""));

    if(adr->personal && adr->personal[0]){
	pers_decoded
	    = cpystr((char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					    adr->personal,
					    verbose ? NULL : &dummy));
	fs_give((void **)&adr->personal);
	adr->personal = pers_decoded;
	if(dummy)
	  fs_give((void **)&dummy);
    }

    ret = (char *)fs_get((size_t)est_size(adr));
    ret[0] = '\0';
    rfc822_write_address(ret, adr);
    mail_free_address(&adr);
    return(ret);
}


/*
 * Give expanded view of this address entry.
 * Call scrolltool to do the work.
 *
 * Args: headents -- The headerentry array from pico.
 */
void
expand_addrs_for_pico(headents)
    struct headerentry *headents;
{
    BuildTo      bldto;
    STORE_S     *store;
    char        *error = NULL, *addr = NULL;
    char         spaces[40], buf[MAX_ADDRESS+1];
    SAVE_STATE_S state;
    ADDRESS     *adrlist = NULL, *a;
    int          j, address_index;
    void       (*redraw)() = ps_global->redrawer;

    dprint(2, (debugfile, "- expand_addrs_for_pico -\n"));

    ps_global->redrawer = NULL;
    fix_windsize(ps_global);

    save_state(&state);

    for(j=0; headents[j].name != NULL; j++)
      if(strncmp(headents[j].name, "Address", 7) == 0)
	break;

    address_index = j;
    if(headents[address_index].name == NULL)
      panic("programmer botch in expand_addrs_for_pico");

    bldto.type    = Str;
    bldto.arg.str = *headents[address_index].realaddr;
    our_build_address(bldto, &addr, &error, NULL, 0, 0);
    if(error){
	q_status_message1(SM_ORDER, 3, 4, "%s", error);
	fs_give((void **)&error);
    }
    
    if(addr)
      rfc822_parse_adrlist(&adrlist, addr, ps_global->maildomain);
    
    if(!(store = so_get(CharStar, NULL, EDIT_ACCESS))){
	q_status_message(SM_ORDER | SM_DING, 3, 3, "Error allocating space.");
	restore_state(&state);
	return;
    }

    for(j = 0; j < address_index; j++){
	so_puts(store, headents[j].prompt);
	so_puts(store, *headents[j].realaddr);
	so_puts(store, "\n");
    }

    so_puts(store, headents[address_index].prompt);

    a = adrlist;
    if(!a)
      so_puts(store, "<none>");
    else{
	so_puts(store, (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    addr_string(a, buf), NULL));
	a = a->next;
    }

    if(a)
      sprintf(spaces, ",\n%*s", headents[address_index].prlen, " ");

    for(; a; a = a->next){
	so_puts(store, spaces);
	so_puts(store, (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    addr_string(a, buf), NULL));
    }

    so_puts(store, "\n");
    scrolltool(so_text(store), "ADDRESS BOOK (Rich View)", ViewAbookText,
	       CharStar, NULL);
    so_give(&store);

    restore_state(&state);

    if(addr)
      fs_give((void **)&addr);
    
    if(adrlist)
      mail_free_address(&adrlist);

    ps_global->redrawer = redraw;
}


static long        msgno_for_pico_callback;
static BODY       *body_for_pico_callback = NULL;
static ENVELOPE   *env_for_pico_callback = NULL;

/*
 * Callback from TakeAddr editing screen to see message that was being
 * viewed.  Call scrolltool to do the work.
 */
char *
view_message_for_pico()
{
    STORE_S     *store;
    gf_io_t      pc;
    void       (*redraw)() = ps_global->redrawer;
    SourceType   src = CharStar;

    dprint(2, (debugfile, "- view_message_for_pico -\n"));

    ps_global->redrawer = NULL;
    fix_windsize(ps_global);

#ifdef DOS
    src = FileStar;
#endif

    if(!(store = so_get(src, NULL, EDIT_ACCESS))){
	q_status_message(SM_ORDER | SM_DING, 3, 3, "Error allocating space.");
	return(NULL);
    }

    gf_set_so_writec(&pc, store);

    format_message(msgno_for_pico_callback, env_for_pico_callback,
		   body_for_pico_callback, FM_NEW_MESS, pc);

    scrolltool(so_text(store), "MESSAGE TEXT", ViewAbookText,
	       src, NULL);
    so_give(&store);

    ps_global->redrawer = redraw;

    return(NULL);
}


/*
prompt::name::help::prlen::maxlen::realaddr::
builder::affected_entry::next_affected::selector::key_label::
display_it::break_on_comma::is_attach::rich_header::only_file_chars::
single_space::sticky::dirty::start_here::KS_ODATAVAR
*/
static struct headerentry headents_templ[]={
  {"Nickname  : ",  "Nickname",  h_composer_abook_nick, 12, 0, NULL,
   verify_nick,   NULL, NULL, addr_book_nick_for_edit, "To AddrBk",
   1, 0, 0, 0, 0, 1, 0, 0, 0, KS_NONE},
  {"Fullname  : ",  "Fullname",  h_composer_abook_full, 12, 0, NULL,
   NULL,          NULL, NULL, view_message_for_pico,   "To Message",
   1, 0, 0, 0, 0, 1, 0, 0, 0, KS_NONE},
  {"Fcc       : ",  "FileCopy",  h_composer_abook_fcc, 12, 0, NULL,
   NULL,          NULL, NULL, folders_for_fcc,         "To Fldrs",
   1, 0, 0, 0, 0, 1, 0, 0, 0, KS_NONE},
  {"Comment   : ",  "Comment",   h_composer_abook_comment, 12, 0, NULL,
   NULL,          NULL, NULL, view_message_for_pico,   "To Message",
   1, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"Addresses : ",  "Addresses", h_composer_abook_addrs, 12, 0, NULL,
   verify_addr,   NULL, NULL, addr_book_change_list,   "To AddrBk",
   1, 1, 0, 0, 0, 1, 0, 0, 0, KS_NONE},
  {NULL, NULL, NO_HELP, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE}
};
#define N_NICK    0
#define N_FULL    1
#define N_FCC     2
#define N_COMMENT 3
#define N_ADDR    4
#define N_END     5

static char        *nick_saved_for_pico_check;
static AdrBk       *abook_saved_for_pico_check;
static SAVE_STATE_S state_saved_for_pico_check;

/*
 * Args: abook  -- Address book handle
 *       abe    -- AdrBk_Entry of old entry to work on.  If NULL, this will
 *                 be a new entry.  This has to be a pointer to a copy of
 *                 an abe that won't go away until we finish this function.
 *                 In other words, don't pass in a pointer to an abe in
 *                 the cache, copy it first.  The tag on this abe is only
 *                 used to decide whether to read abe->addr.list or
 *                 abe->addr.addr, not to determine what the final result
 *                 will be.  That's determined solely by how many addresses
 *                 there are after the user edits.
 *       entry  -- The entry number of the old entry that we will be changing.
 *      old_tag -- If we're changing an old entry, then this is the tag of
 *                 that old entry.
 *     readonly -- Call pico with readonly flag
 *       warped -- We warped to a new part of the addrbook
 *                 (We also overload warped in a couple places and use it's
 *                  being set as an indicator of whether we are Taking or
 *                  not.  It will be NULL if we are Taking.)
 */
void
edit_entry(abook, abe, entry, old_tag, readonly, warped)
    AdrBk       *abook;
    AdrBk_Entry *abe;
    a_c_arg_t    entry;
    Tag          old_tag;
    int          readonly;
    int         *warped;
{
    AdrBk_Entry local_abe;
    struct headerentry *he;
    PICO pbuf;
    STORE_S *msgso;
    adrbk_cntr_t old_entry_num, new_entry_num = NO_NEXT;
    int rc = 0, resort_happened = 0, list_changed = 0, which_addrbook;
    int editor_result, i = 0, add;
    char *nick, *full, *fcc, *comment, *dcomment, *fname, *dummy, *pp;
    char **orig_encoded = NULL, **orig_decoded = NULL;
    char **new_encoded = NULL, **new_decoded = NULL;
    char *addr_encoded  = NULL, *addr_decoded = NULL;
    char **p, **q;
    Tag new_tag;
    char titlebar[30];
    long length;

    dprint(2, (debugfile, "- edit_entry -\n"));

    old_entry_num = (adrbk_cntr_t)entry;
    save_state(&state_saved_for_pico_check);
    abook_saved_for_pico_check = abook;

    add = (abe == NULL);  /* doing add or change? */
    if(add){
	local_abe.nickname  = "";
	local_abe.fullname  = "";
	local_abe.fcc       = "";
	local_abe.extra     = "";
	local_abe.addr.addr = "";
	local_abe.tag       = NotSet;
	abe = &local_abe;
	old_entry_num = NO_NEXT;
    }

    new_tag = abe->tag;

    memset((void *)&pbuf, 0, sizeof(pbuf));
    pbuf.raw_io        = Raw;
    pbuf.showmsg       = display_message_for_pico;
    pbuf.newmail       = new_mail_for_pico;
    pbuf.msgntext      = NULL;
    pbuf.upload	       = NULL;
    pbuf.ckptdir       = checkpoint_dir_for_pico;
    pbuf.mimetype      = NULL;
    pbuf.exittest      = pico_sendexit_for_adrbk;
    pbuf.canceltest    = warped ? pico_cancel_for_adrbk_edit
				: pico_cancel_for_adrbk_take;
    pbuf.expander      = expand_addrs_for_pico;
    pbuf.resize	       = resize_for_pico;
    pbuf.keybinit      = init_keyboard;
    pbuf.helper        = helper;
    pbuf.alt_ed        = NULL;
    pbuf.alt_spell     = NULL;
    pbuf.quote_str     = "";
    pbuf.fillcolumn    = ps_global->composer_fillcol;
    pbuf.menu_rows     = FOOTER_ROWS(ps_global) - 1;
    pbuf.ins_help      = h_composer_ins;
    pbuf.search_help   = h_composer_search;
    pbuf.browse_help   = h_composer_browse;
    pbuf.composer_help = h_composer;
    sprintf(titlebar, "ADDRESS BOOK (%s)", readonly ? "View" : "Edit");
    pbuf.pine_anchor   = set_titlebar(titlebar,
				      ps_global->mail_stream,
				      ps_global->context_current,
				      ps_global->cur_folder,ps_global->msgmap, 
				      0, FolderName, 0, 0);
    pbuf.pine_version  = pine_version;
    pbuf.pine_flags    = flags_for_pico(ps_global);
    pbuf.pine_flags   |= P_ABOOK;
    if(readonly)
      pbuf.pine_flags |= P_VIEW;

    /* An informational message */
    if(msgso = so_get(PicoText, NULL, EDIT_ACCESS)){
	pbuf.msgtext = (void *)so_text(msgso);
	/*
	 * It's nice if we can make it so these lines make sense even if
	 * they don't all make it on the screen, because the user can't
	 * scroll down to see them.  So just make each line a whole sentence
	 * that doesn't need the others below it to make sense.
	 */
	if(add){
	    so_puts(msgso,
"\n Fill in the fields just like you would in the composer.");
	    so_puts(msgso,
"\n To form a list, just enter multiple comma-separated addresses.");
	    so_puts(msgso,
"\n To add to a list, use the View/Edit cmd instead of the AddNew cmd.");
	}
	else{
	    so_puts(msgso,
"\n Edit any of the fields, just like you would do in the composer.");
	    so_puts(msgso,
"\n Additional comma-separated addresses may be entered in the address field.");
	}

	so_puts(msgso,
"\n It is ok to leave fields blank.  Press ^X to save the new entry.");
    }

    he = (struct headerentry *)fs_get((N_END+1) * sizeof(struct headerentry));
    memset((void *)he, 0, (N_END+1) * sizeof(struct headerentry));
    pbuf.headents      = he;

    /* make a copy of each field */
    nick = cpystr(abe->nickname);
    nick_saved_for_pico_check = cpystr(abe->nickname);
    he[N_NICK]          = headents_templ[N_NICK];
    he[N_NICK].realaddr = &nick;

    dummy = NULL;
    full = cpystr((char *)rfc1522_decode((unsigned char *)tmp_20k_buf+10000,
			       abe->fullname, &dummy));
    if(dummy)
      fs_give((void **)&dummy);

    he[N_FULL]          = headents_templ[N_FULL];
    he[N_FULL].realaddr = &full;

    fcc = cpystr(abe->fcc);
    he[N_FCC]          = headents_templ[N_FCC];
    he[N_FCC].realaddr = &fcc;

    dummy = NULL;
    dcomment = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf, abe->extra,
								    &dummy);
    if(dummy)
      fs_give((void **)&dummy);

    comment = cpystr(dcomment);
    he[N_COMMENT]          = headents_templ[N_COMMENT];
    he[N_COMMENT].realaddr = &comment;

    he[N_ADDR]          = headents_templ[N_ADDR];
    he[N_ADDR].realaddr = &addr_decoded;
    if(abe->tag == Single){
	if(abe->addr.addr){
	    orig_encoded = (char **)fs_get(2 * sizeof(char *));
	    orig_decoded = (char **)fs_get(2 * sizeof(char *));
	    orig_encoded[0] = cpystr(abe->addr.addr);
	    orig_encoded[1] = NULL;
	}
    }
    else if(abe->tag == List){
	if(listmem_count_from_abe(abe) > 0){
	    orig_encoded = (char **)fs_get(
				      (size_t)(listmem_count_from_abe(abe) + 1)
					* sizeof(char *));
	    orig_decoded = (char **)fs_get(
				      (size_t)(listmem_count_from_abe(abe) + 1)
					* sizeof(char *));
	    for(q = orig_encoded, p = abe->addr.list; p && *p; p++, q++)
	      *q = cpystr(*p);
	    
	    *q = NULL;
	}
    }

    /*
     * Orig_encoded is the original list saved in encoded form.
     * Now save the original list but in decoded form.
     */
    for(q = orig_decoded, p = orig_encoded; p && *p; p++, q++){
	/*
	 * Here we have an address string, which we need to parse, then
	 * decode the fullname, possibly quote it, then turn it back into
	 * a string.
	 */
	*q = decode_fullname_of_addrstring(*p, 0);
    }

    if(q)
      *q = NULL;

    /* figure out how large a string we need to allocate */
    length = 0L;
    for(p = orig_decoded; p && *p; p++)
      length += (strlen(*p) + 2);
    
    if(length)
      length -= 2L;

    pp = addr_decoded = (char *)fs_get((size_t)(length+1L) * sizeof(char));
    *pp = '\0';
    for(p = orig_decoded; p && *p; p++){
	sstrcpy(&pp, *p);
	if(*(p+1))
	  sstrcpy(&pp, ", ");
    }

    if(verify_addr(addr_decoded, NULL, NULL, NULL) < 0)
      he[N_ADDR].start_here = 1;

    /*
     * Now we have orig_encoded -- a list of encoded addresses
     *             orig_decoded -- a list of decoded addresses
     *             addr_decoded -- orig_decoded put together into a decoded,
     *				    comma-separated string
     *             new_decoded will be the edited, decoded list
     *             new_encoded will be the encoded version of that
     */

    he[N_END] = headents_templ[N_END];
    for(i = 0; i < N_END; i++){
	/* no callbacks in some cases */
	if(readonly || ((i == N_FULL || i == N_COMMENT)
			&& !env_for_pico_callback)){
	    he[i].selector  = NULL;
	    he[i].key_label = NULL;
	}

	/* no builders for readonly */
	if(readonly)
	  he[i].builder = NULL;
    }

    /* pass to pico and let user change them */
    editor_result = pico(&pbuf);

    if(editor_result & COMP_GOTHUP)
      hup_signal();
    else{
	fix_windsize(ps_global);
	init_signals();
    }

    if(editor_result & COMP_CANCEL){
	if(!readonly)
	  q_status_message1(SM_ORDER, 0, 3, "%s is cancelled",
				warped ? "Edit" : "Take");
    }
    else if(editor_result & COMP_EXIT){
	/* don't allow adding null entry */
	if(add && !*nick && !*full && !*fcc && !*comment && !*addr_decoded)
	  goto outtahere;

	/*
	 * addr_decoded is now the decoded string which has been edited
	 */
	if(addr_decoded)
	  new_decoded = parse_addrlist(addr_decoded);

	if(!new_decoded || !new_decoded[0])
	  q_status_message(SM_ORDER, 3, 5, "Warning: entry has no addresses");

	if(!new_decoded || !new_decoded[0] || !new_decoded[1])
	  new_tag = Single;  /* one or zero addresses means its a Single */
	else
	  new_tag = List;    /* more than one addresses means its a List */

	if(new_tag == List && old_tag == List){
	    /*
	     * If Taking, make sure we write it even if user didn't edit
	     * it any further.
	     */
	    if(!warped)
	      list_changed++;
	    else if(he[N_ADDR].dirty)
	      for(q = orig_decoded, p = new_decoded; p && *p && q && *q; p++, q++)
	        if(strcmp(*p, *q) != 0){
		    list_changed++;
		    break;
	        }
	    
	    if(!list_changed && he[N_ADDR].dirty
	      && ((!(p && *p) && (q && *q)) || ((p && *p) && !(q && *q))))
	      list_changed++;

	    if(list_changed){
		/*
		 * need to delete old list members and add new members below
		 */
		rc = adrbk_listdel_all(abook, (a_c_arg_t)old_entry_num);
	    }
	    else{
		/* don't need new_decoded */
		if(new_decoded)
		  free_list(&new_decoded);
	    }

	    if(addr_decoded)
	      fs_give((void **)&addr_decoded);
	}
	else if(new_tag == List && old_tag == Single
	       || new_tag == Single && old_tag == List){
	    /* delete old entry */
	    rc = adrbk_delete(abook, (a_c_arg_t)old_entry_num, 0);
	    old_entry_num = NO_NEXT;
	    if(addr_decoded && new_tag == List)
	      fs_give((void **)&addr_decoded);
	}

	if(new_tag == Single && addr_decoded){
	    /*
	     * Compare addr_decoded to each of orig_decoded.
	     * If it matches one, make addr_encoded equal to orig_encoded
	     * for that one, else encode it in our charset.
	     */
	    for(q = orig_decoded, p = orig_encoded; q && *q; q++, p++){
		if(!he[N_ADDR].dirty || strcmp(*q, addr_decoded) == 0)
		  break;
	    }

	    if(q && *q && p && *p)  /* got a match, use what we already had */
	      addr_encoded = cpystr(*p);
	    else  /* encode in our charset */
	      addr_encoded = encode_fullname_of_addrstring(addr_decoded,
						      ps_global->VAR_CHAR_SET);
	}

	/*
	 * This will be an edit in the cases where the tag didn't change
	 * and an add in the cases where it did.
	 */
	if(rc == 0)
	  rc = adrbk_add(abook,
		       (a_c_arg_t)old_entry_num,
		       nick,
		       he[N_FULL].dirty ? rfc1522_encode(tmp_20k_buf,
						       (unsigned char *)full,
						       ps_global->VAR_CHAR_SET)
					: abe->fullname,
		       new_tag == Single ? addr_encoded : NULL,
		       fcc,
		       he[N_COMMENT].dirty ? rfc1522_encode(tmp_20k_buf +
							       2*MAX_FULLNAME,
						       (unsigned char *)comment,
						       ps_global->VAR_CHAR_SET)
					   : abe->extra,
		       new_tag,
		       &new_entry_num,
		       &resort_happened);
    }
    
    if(rc == 0 && new_tag == List && new_decoded){
	char **t;

	/*
	 * Build a new list.
	 * For each entry in new_decoded, look through orig_decoded.  If
	 * matched, go with that orig_encoded, else encode that entry.
	 */
	for(p = new_decoded; *p; p++)
	  ;/* just counting for alloc below */
	
	t = new_encoded = (char **)fs_get((size_t)((p - new_decoded) + 1)
							    * sizeof(char *));
	memset((void *)new_encoded, 0, ((p-new_decoded)+1) * sizeof(char *));
	for(p = new_decoded; p && *p; p++){
	    for(q = orig_decoded; q && *q; q++)
	      if(strcmp(*q, *p) == 0)
		break;

	    if(q && *q)  /* got a match, use what we already have */
	      *t++ = cpystr(orig_encoded[q - orig_decoded]);
	    else  /* encode in our charset */
	      *t++ = encode_fullname_of_addrstring(*p, ps_global->VAR_CHAR_SET);
	}

	rc = adrbk_nlistadd(abook, (a_c_arg_t)new_entry_num, new_encoded);
    }

    restore_state(&state_saved_for_pico_check);

    if(rc == -2 || rc == -3){
	q_status_message1(SM_ORDER | SM_DING, 3, 4,
			"Error updating address book: %s",
			rc == -2 ? error_description(errno) : "Pine bug");
    }
    else if(rc == 0
       && strucmp(nick, nick_saved_for_pico_check) != 0
       && (editor_result & COMP_EXIT)
       && (fname = addr_lookup(nick, &which_addrbook, as.cur))){
	q_status_message4(SM_ORDER, 5, 9,
	    "Warning! nickname %s also exists in \"%s\"%s%s",
	    nick, as.adrbks[which_addrbook].nickname,
	    (fname && *fname) ? " as " : "",
	    (fname && *fname) ? fname : "");
	if(fname)
	  fs_give((void **)&fname);
    }

    if(resort_happened || list_changed){
	DL_CACHE_S dlc_restart;

	dlc_restart.adrbk_num = as.cur;
	dlc_restart.dlcelnum  = new_entry_num;
	switch(new_tag){
	  case Single:
	    dlc_restart.type = DlcSimple;
	    break;
	  
	  case List:
	    dlc_restart.type = DlcListHead;
	    break;
	}

	warp_to_dlc(&dlc_restart, 0L);
	if(warped)
	  *warped = 1;
    }

outtahere:
    if(he){
	for(i = 0; i < N_END; i++)
	  if(he[i].bldr_private)
	  fs_give((void **)&(he[i].bldr_private));

	fs_give((void **)&he);
    }

    if(msgso)
      so_give(&msgso);

    if(nick)
      fs_give((void **)&nick);
    if(full)
      fs_give((void **)&full);
    if(fcc)
      fs_give((void **)&fcc);
    if(comment)
      fs_give((void **)&comment);

    if(addr_decoded)
      fs_give((void **)&addr_decoded);
    if(addr_encoded)
      fs_give((void **)&addr_encoded);
    if(nick_saved_for_pico_check)
      fs_give((void **)&nick_saved_for_pico_check);

    if(orig_decoded)
      free_list(&orig_decoded);
    if(orig_encoded)
      free_list(&orig_encoded);

    if(new_decoded)
      free_list(&new_decoded);
    if(new_encoded)
      free_list(&new_encoded);
}


/*ARGSUSED*/
int
verify_nick(given, expanded, error, fcc)
    char	 *given,
		**expanded,
		**error;
    BUILDER_ARG  *fcc;
{
    char *tmp;

    tmp = cpystr(given);
    removing_leading_white_space(tmp);
    removing_trailing_white_space(tmp);

    if(nickname_check(tmp, error)){
	fs_give((void **)&tmp);
	return -2;
    }

    if(strucmp(tmp, nick_saved_for_pico_check) != 0){
	restore_state(&state_saved_for_pico_check);
	if(adrbk_lookup_by_nick(abook_saved_for_pico_check,
				   tmp, (adrbk_cntr_t *)NULL)){
	    if(error){
		char buf[MAX_NICKNAME + 80];

		sprintf(buf, "\"%s\" already in address book.", tmp);
		*error = cpystr(buf);
	    }
	    
	    fs_give((void **)&tmp);
	    save_state(&state_saved_for_pico_check);
	    return -2;
	}

	save_state(&state_saved_for_pico_check);
    }

    if(expanded)
      *expanded = tmp;

    /* This is so pico will erase any old message */
    if(error)
      *error = cpystr("");

    return 0;
}


/*
 * Args: to      -- the passed in line to parse
 *       full_to -- Address of a pointer to return the full address in.
 *		    This will be allocated here and freed by the caller.
 *                  However, this function is just going to copy "to".
 *                  We're just looking for the error messages.
 *       error   -- Address of a pointer to return an error message in.
 *		    This will be allocated here and freed by the caller.
 *       fcc     -- This should be passed in NULL.
 *                  This builder doesn't support affected_entry's.
 *
 * Result:  0 is returned if address was OK, 
 *         -2 if address wasn't OK.
 *
 * Side effect: Can flush addrbook entry cache entries so they need to be
 * re-fetched afterwords.
 */
int
verify_addr(to, full_to, error, fcc)
    char	 *to,
		**full_to,
		**error;
    BUILDER_ARG	 *fcc;
{
    register char *p;
    int            ret_val;
    BuildTo        bldto;
    jmp_buf        save_jmp_buf;

    dprint(2, (debugfile, "- verify_addr - (%s)\n", to ? to : "nul"));

    if(fcc)
      panic("programmer botch in verify_addr");

    /* check to see if to string is empty to avoid work */
    for(p = to; p && *p && isspace((unsigned char)(*p)); p++)
      ;/* do nothing */

    if(!p || !*p){
	if(full_to)
	  *full_to = cpystr(to ? to : "");  /* because pico does a strcmp() */

	return 0;
    }

    if(full_to != NULL)
      *full_to = (char *)NULL;

    if(error != NULL)
      *error = (char *)NULL;
    
    /*
     * If we end up jumping back here because somebody else changed one of
     * our addrbooks out from underneath us, we may well leak some memory.
     * That's probably ok since this will be very rare.
     */
    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	if(full_to && *full_to)
	  fs_give((void **)full_to);

	q_status_message(SM_ORDER, 3, 5, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... verify_addr(%s)!\n", to));
	addrbook_reset();
    }

    bldto.type    = Str;
    bldto.arg.str = to;

    ret_val = build_address_internal(bldto,NULL,error,NULL,NULL,NULL,1,1);

    if(full_to){
	*full_to = cpystr(to ? to : "");
	if(ret_val >= 0){
	    removing_leading_white_space(*full_to);
	    removing_trailing_white_space(*full_to);
	}
    }

    /* This is so pico will erase the old message */
    if(error != NULL && *error == NULL)
      *error = cpystr("");

    if(ret_val < 0)
      ret_val = -2;  /* cause pico to stay on same header line */

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(ret_val);
}


/*
 *  Call back for pico to prompt the user for exit confirmation
 *
 * Returns: either NULL if the user accepts exit, or string containing
 *	 reason why the user declined.
 */      
char *
pico_sendexit_for_adrbk()
{
    char *rstr = NULL;
    void (*redraw)() = ps_global->redrawer;

    ps_global->redrawer = NULL;
    fix_windsize(ps_global);
    
    switch(want_to("Exit and save changes ", 'y', 0, NO_HELP, 0, 0)){
      case 'y':
	break;

      case 'n':
	rstr = "Use ^C to abandon changes you've made";
	break;

#ifdef OLDWAY
      /* ^C */
      case 'x':
	rstr = "";
	break;
#endif
    }

    ps_global->redrawer = redraw;
    return(rstr);
}


/*
 *  Call back for pico to prompt the user for exit confirmation
 *
 * Returns: either NULL if the user accepts exit, or string containing
 *	 reason why the user declined.
 */      
char *
pico_cancelexit_for_adrbk(word)
    char *word;
{
    char prompt[90];
    char *rstr = NULL;
    void (*redraw)() = ps_global->redrawer;

    strcat(strcat(strcpy(prompt, "Cancel "), word),
	   " (answering \"Yes\" will abandon any changes made) ");
    ps_global->redrawer = NULL;
    fix_windsize(ps_global);
    
    switch(want_to(prompt, 'y', 'x', NO_HELP, 0, 0)){
      case 'y':
	rstr = "";
	break;

      case 'n':
      case 'x':
	break;
    }

    ps_global->redrawer = redraw;
    return(rstr);
}


char *
pico_cancel_for_adrbk_take()
{
    return(pico_cancelexit_for_adrbk("take"));
}


char *
pico_cancel_for_adrbk_edit()
{
    return(pico_cancelexit_for_adrbk("changes"));
}


/*
 * Validate selected address with build_address, save addrbook state,
 * call composer, restore addrbook state.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 */
void
ab_compose_to_addr(cur_line)
    long cur_line;
{
    int		   good_addr;
    char          *addr,
		  *error = NULL,
		  *fcc;
    AddrScrn_Disp *dl;
    AdrBk_Entry   *abe;
    SAVE_STATE_S   state;  /* For saving state of addrbooks temporarily */
    BuildTo      bldto;

    dprint(2, (debugfile, "- ab_compose_to_addr -\n"));

    save_state(&state);

    fcc  = NULL;
    addr = NULL;

    if(is_addr(cur_line)){

	dl  = dlist(cur_line);
	abe = ae(cur_line);

	if(dl->type == ListHead && listmem_count_from_abe(abe) == 0){
	    q_status_message(SM_ORDER, 0, 4, "Warning:  this list is empty!");
	    good_addr = 0;
	}
	else if(dl->type == ListEnt){
	    bldto.type    = Str;
	    bldto.arg.str = listmem(cur_line);
	    good_addr = (our_build_address(bldto,&addr,&error,&fcc,0,0) >= 0);
	}
	else{
	    bldto.type    = Abe;
	    bldto.arg.abe = abe;
	    good_addr = (our_build_address(bldto,&addr,&error,&fcc,0,0) >= 0);
	}

	if(error){
	    q_status_message1(SM_ORDER, 3, 4, "%s", error);
	    fs_give((void **)&error);
	}

	if(!good_addr && addr)
	  fs_give((void **)&addr); /* relying on fs_give setting
					addr to NULL */
    }

    compose_mail(addr, fcc, NULL);

    restore_state(&state);

    if(addr)
      fs_give((void **)&addr);

    if(fcc)
      fs_give((void **)&fcc);
}


/*
 * Export the addresses of a list into a file.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       command_line -- The screen line on which to prompt
 */
void
ab_export(cur_line, command_line)
    long cur_line;
    int  command_line;
{
    int		   good_addr, over = 0;
    char          *addr,
		  *error = NULL;
    AddrScrn_Disp *dl;
    AdrBk_Entry   *abe;
    BuildTo        bldto;
    struct variable *vars = ps_global->vars;

    dprint(2, (debugfile, "- ab_export -\n"));

    if(ps_global->restricted){
	q_status_message(SM_ORDER, 0, 3,
	    "Pine demo can't export addresses to files");
	return;
    }

    addr = NULL;
    dl  = dlist(cur_line);
    abe = ae(cur_line);

    if(dl->type == ListHead && listmem_count_from_abe(abe) == 0){
	error = "List is empty, nothing to export!";
	good_addr = 0;
    }
    else if(dl->type == ListEnt){
	bldto.type    = Str;
	bldto.arg.str = listmem(cur_line);
	good_addr = (our_build_address(bldto,&addr,&error,NULL,0,0) >= 0);
    }
    else{
	bldto.type    = Abe;
	bldto.arg.abe = abe;
	good_addr = (our_build_address(bldto,&addr,&error,NULL,0,0) >= 0);
    }

    /* Have to rfc1522_decode the addr */
    if(addr){
	char *p;

	p = (char *)fs_get((strlen(addr) + 1) * sizeof(char));
	if(rfc1522_decode((unsigned char *)p,addr,NULL) == (unsigned char *)p){
	    fs_give((void **)&addr);
	    addr = p;
	}
	else
	  fs_give((void **)&p);
    }

    if(error){
	q_status_message1(SM_ORDER, 3, 4, "%s", error);
	fs_give((void **)&error);
    }

    if(good_addr){
	int   quoted = 0;
	char *p;
	char  prompt[100];

	/*
	 * Change the unquoted commas into newlines.  Not worth it to do
	 * complicated quoting, just consider double quotes.
	 */
	for(p = addr; *p; p++){
	    if(*p == '"')
	      quoted = !quoted;
	    else if(!quoted && *p == ','){
		*p++ = '\n';
		removing_leading_white_space(p);
		p--;
	    }
	}

	if(addr && *addr){
	    static ESCKEY_S export_opts[] = {
		{ctrl('T'), 10, "^T", "To Files"},
		{-1, 0, NULL, NULL}};
	    HelpType help;
	    char     filename[MAXPATH+1], full_filename[MAXPATH+1];
	    char    *ill;
	    int      rc, orig_errno, failure = 0;
	    STORE_S *store;
	    gf_io_t  pc;
	    long     start_of_append;

	    help = NO_HELP;
	    filename[0] = '\0';
	    while(1){
#ifdef	DOS
		(void)strcpy(prompt, "File to save addresses in: ");
#else
		sprintf(prompt,
		    "EXPORT: (copy addresses) to file in %s directory: ",
		     F_ON(F_USE_CURRENT_DIR, ps_global) ? "current"
		     : VAR_OPER_DIR ? VAR_OPER_DIR : "home");
#endif

		rc = optionally_enter(filename, command_line, 0, MAXPATH, 1, 0,
			 prompt, export_opts, help, 0);

		/* file browser */
		if(rc == 10){
		    if(filename[0])
		      strcpy(full_filename, filename);
		    else if(F_ON(F_USE_CURRENT_DIR, ps_global))
		      (void)getcwd(full_filename, MAXPATH);
		    else if(VAR_OPER_DIR)
		      build_path(full_filename, VAR_OPER_DIR, filename);
		    else
		      build_path(full_filename, ps_global->home_dir, filename);

		    rc = file_lister("EXPORT", full_filename, filename, TRUE,
				     FB_SAVE);

		    if(rc == 1){
			strcat(full_filename, "/");
			strcat(full_filename, filename);
			break;
		    }
		    else
		      continue;
		}
		else if(rc == 3){
		    help = (help == NO_HELP) ? h_oe_export : NO_HELP;
		    continue;
		}

		removing_trailing_white_space(filename);
		removing_leading_white_space(filename);
		if(rc == 1 || filename[0] == '\0'){
		    q_status_message(SM_ORDER, 0, 2, "Export cancelled");
		    goto fini;
		}

		if(rc == 4)
		  continue;

		/*-- check out and expand file name. give error messages --*/
		strcpy(full_filename, filename);
		if((ill = filter_filename(filename)) != NULL){
		    q_status_message1(SM_ORDER | SM_DING, 3, 3, "%s", ill);
		    continue;
		}
#if defined(DOS) || defined(OS2)
		if(is_absolute_path(full_filename)){
		    fixpath(full_filename, MAXPATH);
		}
#else
		if(full_filename[0] == '~'){
		    if(fnexpand(full_filename, sizeof(full_filename)) == NULL){
			p = strindex(full_filename, '/');
			if(p != NULL)
			  *p = '\0';

			q_status_message1(SM_ORDER | SM_DING, 3, 3,
			      "Error expanding file name: \"%s\" unknown user",
				      full_filename);
			continue;
		    }
		}
#endif
		if(!is_absolute_path(full_filename)){
		    if(F_ON(F_USE_CURRENT_DIR, ps_global))
		      (void)strcpy(full_filename, filename);
		    else if(VAR_OPER_DIR)
		      build_path(full_filename, VAR_OPER_DIR, filename);
		    else
		      build_path(full_filename, ps_global->home_dir, filename);
		}

		break; /* Must have got an OK file name */

	    }

	    if(VAR_OPER_DIR && !in_dir(VAR_OPER_DIR, full_filename)){
		q_status_message1(SM_ORDER, 0, 2,
		    "Can't export to file outside of %s", VAR_OPER_DIR);
		goto fini;
	    }

	    /* ---- full_filename already contains the absolute path ---*/
	    if(!can_access(full_filename, ACCESS_EXISTS)){
		static ESCKEY_S access_opts[] = {
		    {'o', 'o', "O", "Overwrite"},
		    {'a', 'a', "A", "Append"},
		    {-1, 0, NULL, NULL}};

		rc = strlen(filename);
		sprintf(prompt,
		    "File \"%s%s\" already exists.  Overwrite or append it ? ",
			(rc > 20) ? "..." : "",
			filename + ((rc > 20) ? rc - 20 : 0));
		switch(radio_buttons(prompt, -FOOTER_ROWS(ps_global),
				     access_opts, 'a', 'x', NO_HELP, RB_NORM)){
		  case 'o' :
		    over = 1;
		    if(unlink(full_filename) < 0){	/* BUG: breaks links */
			q_status_message2(SM_ORDER | SM_DING, 3, 5,
					  "Error deleting old %s: %s",
					  full_filename,
					  error_description(errno));
			goto fini;
		    }

		    break;

		  case 'a' :
		    over = -1;
		    break;

		  case 'x' :
		  default :
		    q_status_message(SM_ORDER, 0, 2, "Export cancelled");
		    goto fini;
		}
	    }

	    dprint(5, (debugfile, "Opening file \"%s\" for export\n",
		full_filename));

	    if(!(store = so_get(FileStar, full_filename, WRITE_ACCESS))){
		q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Error opening file \"%s\" for address export: %s",
			  full_filename, error_description(errno));
		goto fini;
	    }
	    else
	      gf_set_so_writec(&pc, store);

	    start_of_append = ftell((FILE *)so_text(store));

	    if(!so_puts(store, addr) || !so_puts(store, NEWLINE)){
		orig_errno = errno;	/* save incase things are really bad */
		failure    = 1;
	    }

	    so_give(&store);				/* release storage */

	    if(failure){
#ifndef	DOS
		truncate(full_filename, start_of_append);
#endif
		dprint(1, (debugfile, "FAILED Export: file \"%s\" : %s\n",
		       full_filename,  error_description(orig_errno)));
		q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Error exporting to \"%s\" : %s",
			  filename, error_description(orig_errno));
	    }
	    else
	      q_status_message2(SM_ORDER,0,3,
			  "Addresses %s to file \"%s\"",
			  over==0 ? "exported"
				  : over==1 ? "overwritten" : "appended",
			  filename);
	}
    }

fini:
    if(addr)
      fs_give((void **)&addr);
}


/*
 * Forward an address book entry via email attachment.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       command_line -- The screen line on which to prompt
 */
void
ab_forward(ps, cur_line)
    struct pine *ps;
    long         cur_line;
{
    AddrScrn_Disp *dl;
    AdrBk_Entry   *abe;
    ENVELOPE      *outgoing = NULL;
    BODY          *pb, *body = NULL;
    PART         **pp;
    ADDRESS       *a, *adrlist = NULL;
    char          *sig, *p,
                  *init_addr = NULL,
		  *addr = NULL,
                  *error = NULL,
                  *next_piece = NULL,
                  *charset = NULL,
		  *tmp = NULL,
		  *decoded,
		   buf[MAX_ADDRESS+1];
    gf_io_t        pc;
    long           length;
    BuildTo        bldto;
    int            are_some_unqualified = 0, expand_nicks = 0, len = 0;

    dprint(2, (debugfile, "- ab_forward -\n"));

    dl  = dlist(cur_line);
    if(dl->type != ListHead && dl->type != Simple)
      return;

    abe = ae(cur_line);
    if(!abe){
	q_status_message(SM_ORDER, 3, 3, "Trouble accessing current entry");
	return;
    }

    outgoing             = mail_newenvelope();
    outgoing->message_id = generate_message_id(ps);
    outgoing->subject = cpystr("Forwarded address book entry for Pine");

    body                                      = mail_newbody();
    body->type                                = TYPEMULTIPART;
    /*---- The TEXT part/body ----*/
    body->contents.part                       = mail_newbody_part();
    body->contents.part->body.type            = TYPETEXT;
    /*--- Allocate an object for the body ---*/
    if(body->contents.part->body.contents.binary =
				(void *)so_get(PicoText, NULL, EDIT_ACCESS)){
	pp = &(body->contents.part->next);
	if(sig = get_signature()){
	    if(*sig)
	      so_puts((STORE_S *)body->contents.part->body.contents.binary,sig);

	    fs_give((void **)&sig);
	}

	so_puts((STORE_S *)body->contents.part->body.contents.binary, "\n  [ Attached to this message is an entry from the sender's Pine address     ]\n  [ book.  To add it to your Pine address book, use the \"TakeAddr\" command. ]\n");
    }
    else{
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Problem creating space for message text");
	goto bomb;
    }


    /*---- create the attachment, and write abook entry into it ----*/
    *pp             = mail_newbody_part();
    pb              = &((*pp)->body);
    pb->type        = TYPEAPPLICATION;
    pb->encoding    = ENCOTHER;  /* let data decide */
    pb->id          = generate_message_id(ps);
    pb->subtype     = cpystr("DIRECTORY");
    pb->description = cpystr("Pine addressbook entry");
    pb->parameter   = mail_newbody_parameter();
    pb->parameter->attribute = cpystr("profile");
    pb->parameter->value     = cpystr("X-Email-Abook-Entry");
    pb->contents.msg.env  = NULL;
    pb->contents.msg.body = NULL;

    if(pb->contents.binary = (void *)so_get(CharStar, NULL, EDIT_ACCESS)){
	gf_set_so_writec(&pc, (STORE_S *)pb->contents.binary);

	if(abe->nickname && abe->nickname[0]){
	    sprintf(tmp_20k_buf, "X-Nickname: %s\r\n", abe->nickname);
	    gf_puts(tmp_20k_buf, pc);
	}

	if(abe->fullname && abe->fullname[0]){
	    decoded
	      = (char *)rfc1522_decode((unsigned char *)(tmp_20k_buf+10000),
		 abe->fullname, &charset);
	    sprintf(tmp_20k_buf, "CN%s%s: %s\r\n",
		 (charset && *charset) ? ";charset=" : "",
		 (charset && *charset) ? charset : "",
		 decoded);
	    gf_puts(tmp_20k_buf, pc);
	    if(charset)
	      fs_give((void **)&charset);
	}

	if(abe->fcc && abe->fcc[0]){
	    sprintf(tmp_20k_buf, "X-Fcc: %s\r\n", abe->fcc);
	    gf_puts(tmp_20k_buf, pc);
	}

	/*
	 * Fold long comment lines.  Don't even worry about folding
	 * the other types of lines.
	 */
#define FOLDHERE 72
	next_piece = abe->extra;
	if(next_piece && *next_piece){
	    tmp = (char *)fs_get(strlen(next_piece) + 1);
	    decoded = (char *)rfc1522_decode((unsigned char *)tmp,
		      next_piece, &charset);
	    sprintf(tmp_20k_buf, "Misc%s%s:",
		 (charset && *charset) ? ";charset=" : "",
		 (charset && *charset) ? charset : "");

	    len = strlen(tmp_20k_buf);
	    next_piece = decoded;
	    gf_puts(tmp_20k_buf, pc);
	    if(charset)
	      fs_give((void **)&charset);
	}
	
	while(next_piece && *next_piece){
	    if(strlen(next_piece) + len < FOLDHERE){
		sprintf(tmp_20k_buf, " %s\r\n", next_piece);
		gf_puts(tmp_20k_buf, pc);
		break;
	    }
	    else{ /* fold it */
		char save_char;
		int  i, starting_point, higher, lower, winner = -1;

		starting_point = FOLDHERE - len;
		/* find a good folding spot */
		for(i = 0; i < 40 && winner == -1; i++){
		    higher = starting_point + i;
		    lower  = starting_point - 1 - i;
		    if(!next_piece[higher]
		       || isspace((unsigned char)next_piece[higher]))
		      winner = higher;

		    if(!next_piece[lower]
		       || isspace((unsigned char)next_piece[lower]))
		      winner = lower;
		}

		if(winner == -1) /* if no good folding spot, fold at FOLDHERE */
		  winner = starting_point;
		
		save_char = next_piece[winner];
		next_piece[winner] = '\0';
		sprintf(tmp_20k_buf, " %s\r\n", next_piece);
		gf_puts(tmp_20k_buf, pc);
		next_piece[winner] = save_char;
		next_piece += winner;
		if(isspace((unsigned char)save_char))
		  next_piece++;
	    }

	    len = 0;
	}

	if(tmp)
	  fs_give((void **)&tmp);

	/*
	 * Search through the addresses to see if there are any
	 * that are unqualified, and so would be different if
	 * expanded.
	 */
	if(abe->tag == Single && abe->addr.addr && abe->addr.addr[0]){
	    if(!strindex(abe->addr.addr, '@'))
	      are_some_unqualified++;
	}
	else{
	    char **ll;

	    for(ll = abe->addr.list; ll && *ll; ll++){
		if(!strindex(*ll, '@')){
		    are_some_unqualified++;
		    break;
		}
	    }
	}

	if(are_some_unqualified){
	    switch(want_to("Expand nicknames", 'y', 'x', h_ab_forward, 0, 0)){
	      case 'x':
		q_status_message(SM_ORDER, 0, 4, "Forward cancelled");
		goto bomb;
		break;
		
	      case 'y':
		expand_nicks = 1;
		break;
		
	      case 'n':
		expand_nicks = 0;
		break;
	    }
	}

	/* expand nicknames and fully-qualify unqualified names */
	if(expand_nicks){
	    if(abe->tag == Single && abe->addr.addr && abe->addr.addr[0])
	      init_addr = cpystr(abe->addr.addr);
	    else{
		char **ll;

		/* figure out how large a string we need to allocate */
		length = 0L;
		for(ll = abe->addr.list; ll && *ll; ll++)
		  length += (strlen(*ll) + 2);

		if(length)
		  length -= 2L;
		
		init_addr = (char *)fs_get((size_t)(length+1L) * sizeof(char));
		p = init_addr;
	    
		for(ll = abe->addr.list; ll && *ll; ll++){
		    sstrcpy(&p, *ll);
		    if(*(ll+1))
		      sstrcpy(&p, ", ");
		}
	    }

	    bldto.type    = Str;
	    bldto.arg.str = init_addr; 
	    our_build_address(bldto, &addr, &error, NULL, 0, 0);
	    if(error){
		q_status_message1(SM_ORDER, 3, 4, "%s", error);
		fs_give((void **)&error);
		goto bomb;
	    }

	    if(addr)
	      rfc822_parse_adrlist(&adrlist, addr, ps->maildomain);
	    
	    for(a = adrlist; a; a = a->next){
		decoded
		  = (char *)rfc1522_decode((unsigned char *)(tmp_20k_buf+10000),
		     addr_string(a, buf), &charset);
		sprintf(tmp_20k_buf, "Email%s%s: %s\r\n",
		     (charset && *charset) ? ";charset=" : "",
		     (charset && *charset) ? charset : "",
		     decoded);
		gf_puts(tmp_20k_buf, pc);
		if(charset)
		  fs_give((void **)&charset);
	    }
	}
	else{ /* don't expand or qualify */
	    if(abe->tag == Single && abe->addr.addr && abe->addr.addr[0]){
		decoded
		  = (char *)rfc1522_decode((unsigned char *)(tmp_20k_buf+10000),
		     abe->addr.addr, &charset);
		sprintf(tmp_20k_buf, "Email%s%s: %s\r\n",
		     (charset && *charset) ? ";charset=" : "",
		     (charset && *charset) ? charset : "",
		     decoded);
		gf_puts(tmp_20k_buf, pc);
		if(charset)
		  fs_give((void **)&charset);
	    }
	    else{
		char **ll;

		for(ll = abe->addr.list; ll && *ll; ll++){
		    decoded = (char *)rfc1522_decode(
					(unsigned char *)(tmp_20k_buf+10000),
					*ll, &charset);
		    sprintf(tmp_20k_buf, "Email%s%s: %s\r\n",
			 (charset && *charset) ? ";charset=" : "",
			 (charset && *charset) ? charset : "",
			 decoded);
		    gf_puts(tmp_20k_buf, pc);
		    if(charset)
		      fs_give((void **)&charset);
		}
	    }
	}

	/* This sets parameter charset, if necessary, and encoding */
	set_mime_type_by_grope(pb);
	pb->size.bytes =
		strlen((char *)so_text((STORE_S *)pb->contents.binary));
    }
    else{
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Problem creating space for message text");
	goto bomb;
    }

    pine_send(outgoing, &body, "FORWARDING ADDRESS BOOK ENTRY", NULL,
	      NULL, NULL, NULL, NULL, NULL, 0);
    
    ps->mangled_screen = 1;

bomb:
    if(outgoing)
      mail_free_envelope(&outgoing);
    if(body)
      pine_free_body(&body);
    if(addr)
      fs_give((void **)&addr);
    if(init_addr)
      fs_give((void **)&init_addr);
    if(adrlist)
      mail_free_address(&adrlist);
}


/*
 * Go to folder.
 *
 *       command_line -- The screen line on which to prompt
 */
void
ab_goto_folder(command_line)
    int command_line;
{
    char *go_folder;
    CONTEXT_S *tc;

    dprint(2, (debugfile, "- ab_goto_folder -\n"));

    tc = (ps_global->context_last
	      && !(ps_global->context_current->type & FTYPE_BBOARD)) 
	       ? ps_global->context_last : ps_global->context_current;

    go_folder = broach_folder(command_line, 1, &tc);

#if defined(DOS) && !defined(_WINDOWS)
    if(go_folder && *go_folder == '{' && coreleft() < 20000){
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Not enough memory to open IMAP folder");
	go_folder = NULL;
    }
#endif /* !DOS */

    if(go_folder != NULL)
      visit_folder(ps_global, go_folder, tc);
}


/*
 * Execute whereis command.
 *
 * Returns value of the new top entry, or NO_LINE if cancelled.
 */
long
ab_whereis(warped, command_line)
    int *warped;
    int  command_line;
{
    int rc, wrapped = 0;
    long new_top_ent, new_line;

    dprint(2, (debugfile, "- ab_whereis -\n"));

    rc = search_book(as.top_ent+as.cur_row, command_line,
		    &new_line, &wrapped, warped);

    new_top_ent = NO_LINE;

    if(rc == -2)
      cancel_warning(NO_DING, "search");

    else if(rc == -1)
      q_status_message(SM_ORDER, 0, 4, "Word not found");

    else if(rc == 0){  /* search succeeded */

	if(wrapped)
	  q_status_message(SM_INFO, 0, 2, "Search wrapped to beginning");

	/* know match is on the same page */
	if(!*warped &&
	    new_line >= as.top_ent &&
	    new_line < as.top_ent+as.l_p_page)
	    new_top_ent = as.top_ent;
	/* don't know whether it is or not, reset top_ent */
	else
	  new_top_ent = first_line(new_line - as.l_p_page/2);

	as.cur_row  = new_line - new_top_ent;
    }

    return(new_top_ent);
}


/*
 * Print out the display list.
 */
void
ab_print()
{
    AddrScrn_Disp *dl; 
    long lineno;
    AdrBk_Entry *abe;
    long save_line;
    DL_CACHE_S dlc_buf, *match_dlc;
    char *fullname, *addr, *lm; 

    if(open_printer("address books ") == 0){

	save_line = as.top_ent + as.cur_row;
	match_dlc = get_dlc(save_line);
	dlc_buf   = *match_dlc;
	match_dlc = &dlc_buf;

	warp_to_beginning();
	lineno = 0L;

	for(dl = dlist(lineno);
	    dl->type != End;
	    dl = dlist(++lineno)){
	    switch(dl->type){
	      case Simple:
		abe = ae(lineno);
		fullname = abe->fullname ? abe->fullname : "";
		addr = (abe->tag == Single && abe->addr.addr)
			    ? abe->addr.addr : "";
		print_text3("%-10.10s %-35.35s %s\n",
			abe->nickname,
			(char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						fullname, NULL),
			addr);
		break;

	      case ListHead:
		abe = ae(lineno);
		fullname = abe->fullname ? abe->fullname : "";
		print_text3("%-10.10s %-35.35s %s\n",
			abe->nickname,
			(char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						fullname, NULL),
			DISTLIST);
		break;

	      case ListEnt:
		lm = listmem(lineno) ? listmem(lineno) : "";
	print_text1("                                               %s\n",
			    (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    lm, NULL));
		break;

	      case ClickHere:
		print_text1("%s\n", CLICKHERE);
		break;

	      case Empty:
		print_text1("%s\n", EMPTY);
		break;

	      case ListEmpty:
    print_text1("                                               %s\n", EMPTY);
		break;

	      case Text:
	      case Title:
		print_text1("%s\n", dl->txt);
		break;

	      case ListClickHere:
		break;
	    }
	}

	close_printer();

	/*
	 * jump cache back to where we started so that the next
	 * request won't cause us to page through the whole thing
	 */
	warp_to_dlc(match_dlc, save_line);
    }
}


/*
 * recalculate display parameters for window size change
 */
void
ab_resize()
{
    long new_line;
    int  old_l_p_p;
    DL_CACHE_S dlc_buf, *dlc_restart;

    old_l_p_p   = as.l_p_page;
    as.l_p_page = ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global)
					       - HEADER_ROWS(ps_global);

    dprint(9, (debugfile, "- ab_resize -\n    l_p_p was %d, now %d\n",
	old_l_p_p, as.l_p_page));

    if(as.l_p_page <= 0)
      return;

    new_line       = as.top_ent + as.cur_row;
    as.top_ent     = first_line(new_line - as.l_p_page/2);
    as.cur_row     = new_line - as.top_ent;
    as.old_cur_row = as.cur_row;

    /* need this to re-initialize Text and Title lines in display */
    /* get the old current line (which may be the wrong width) */
    dlc_restart = get_dlc(new_line);
    /* flush it from cache */
    flush_dlc_from_cache(dlc_restart);
    /* re-get it (should be right now) */
    dlc_restart = get_dlc(new_line);
    /* copy it to local storage */
    dlc_buf = *dlc_restart;
    dlc_restart = &dlc_buf;
    /* flush everything from cache and add that one line back in */
    warp_to_dlc(dlc_restart, new_line);
}


/*
 * Returns 0 if we know for sure that there are no
 * addresses available in any of the addressbooks.
 *
 * Easiest would be to start at 0 and go through the addrbook, but that will
 * be very slow for big addrbooks if we're not close to 0 already.  Instead,
 * starting_hint is a hint at a good place to start looking.
 */
int
any_addrs_avail(starting_hint)
    long starting_hint;
{
    register AddrScrn_Disp *dl;
    long lineno;

    /*
     * Look from lineno backwards first, in hopes of finding it in cache.
     */
    lineno = starting_hint;
    for(dl=dlist(lineno);
	dl->type != Beginning;
	dl = dlist(--lineno)){
	switch(dl->type){
	  case Simple:
	  case ListEnt:
	  case ListHead:
	  case ClickHere:
	  case ListClickHere:
	    return 1;
	}
    }

    /* search from here forward if we still don't know */
    lineno = starting_hint;
    for(dl=dlist(lineno);
	dl->type != End;
	dl = dlist(++lineno)){
	switch(dl->type){
	  case Simple:
	  case ListEnt:
	  case ListHead:
	  case ClickHere:
	  case ListClickHere:
	    return 1;
	}
    }

    return 0;
}


/*
 * Returns 1 if this line is a clickable line.
 */
int
entry_is_clickable(lineno)
    long lineno;
{
    register AddrScrn_Disp *dl;

    if((dl = dlist(lineno)) &&
		    (dl->type == ClickHere || dl->type == ListClickHere))
      return 1;

    return 0;
}


/*
 * Returns 1 if an address or list is selected.
 */
int
is_addr(lineno)
    long lineno;
{
    register AddrScrn_Disp *dl;

    if((dl = dlist(lineno)) && (dl->type == ListHead ||
					dl->type == ListEnt  ||
					dl->type == Simple))
	return 1;

    return 0;
}


/*
 * Returns 1 if type of line is Empty.
 */
int
is_empty(lineno)
    long lineno;
{
    register AddrScrn_Disp *dl;

    if((dl = dlist(lineno)) && (dl->type == Empty || dl->type == ListEmpty))
      return 1;

    return 0;
}


/*
 * Returns 1 if this line is of a type that can have a cursor on it.
 */
int
line_is_selectable(lineno)
    long lineno;
{
    register AddrScrn_Disp *dl;

    if((dl = dlist(lineno)) && (dl->type == Text      ||
				dl->type == Title     ||
				dl->type == ListEmpty ||
				dl->type == Beginning ||
				dl->type == End))
	return 0;

    return 1;
}


/*
 * Find the first selectable line greater than or equal to line.  That is,
 * the first line the cursor is allowed to start on.
 * (If there are none >= line, it will find the highest one.)
 *
 * Returns the line number of the found line or NO_LINE if there isn't one.
 */
long
first_selectable_line(line)
    long line;
{
    long lineno;
    register PerAddrBook *pab;
    int i;

    /* skip past non-selectable lines */
    for(lineno=line;
	!line_is_selectable(lineno) && dlist(lineno)->type != End;
	lineno++)
	;/* do nothing */

    if(line_is_selectable(lineno))
      return(lineno);

    /*
     * There were no selectable lines from lineno on down.  Trying looking
     * back up the list.
     */
    for(lineno=line-1;
	!line_is_selectable(lineno) && dlist(lineno)->type != Beginning;
	lineno--)
	;/* do nothing */

    if(line_is_selectable(lineno))
      return(lineno);

    /*
     * No selectable lines at all.
     * If some of the addrbooks are still not displayed, it is too
     * early to set the no_op_possbl flag.  Or, if some of the addrbooks
     * are empty but writable, then we should not set it either.
     */
    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	if(pab->ostatus != Open && pab->ostatus != HalfOpen)
	  return NO_LINE;

	if(pab->access == ReadWrite && adrbk_count(pab->address_book) == 0)
	  return NO_LINE;
    }

    as.no_op_possbl++;
    return NO_LINE;
}


/*
 * Find the first line greater than or equal to line.  (Any line, not
 * necessarily selectable.)
 *
 * Returns the line number of the found line or NO_LINE if there is none.
 *
 * Warning:  This just starts at the passed in line and goes forward until
 * it runs into a line that isn't a Beginning line.  If the line passed in
 * is not in the dlc cache, it will have no way to know when it gets to the
 * real beginning.
 */
long
first_line(line)
    long line;
{
    long lineno;
    register PerAddrBook *pab;
    int i;

    for(lineno=line;
       dlist(lineno)->type == Beginning;
       lineno++)
	;/* do nothing */

    if(dlist(lineno)->type != End)
      return(lineno);
    else{
	for(i = 0; i < as.n_addrbk; i++){
	    pab = &as.adrbks[i];
	    if(pab->ostatus != Open && pab->ostatus != HalfOpen)
	      return NO_LINE;
	}

	as.no_op_possbl++;
	return(NO_LINE);
    }
}


/*
 * Find the line and field number of the next selectable line, keeping the
 * field about the same.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       new_line     -- Return value: new line position
 *
 * Result: The new line number is set.
 *       The value 1 is returned if OK or 0 if there is no next line.
 */
int
next_selectable_line(cur_line, new_line)
    long  cur_line;
    long *new_line;
{
    /* skip over non-selectable lines */
    for(cur_line++;
	!line_is_selectable(cur_line) && dlist(cur_line)->type != End;
	cur_line++)
	;/* do nothing */

    if(dlist(cur_line)->type == End)
      return 0;

    *new_line = cur_line;
    return 1;
}


/*
 * Find the line and field number of the previous selectable line, keeping the
 * field about the same.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       new_line     -- Return value: new line position
 *
 * Result: The new line number is set.
 *       The value 1 is returned if OK or 0 if there is no previous line.
 */
int
prev_selectable_line(cur_line, new_line)
    long  cur_line;
    long *new_line;
{
    /* skip backwards over non-selectable lines */
    for(cur_line--;
	!line_is_selectable(cur_line) && dlist(cur_line)->type != Beginning;
	cur_line--)
	;/* do nothing */

    if(dlist(cur_line)->type == Beginning)
      return 0;

    *new_line = cur_line;

    return 1;
}


/*
 * Delete an entry from the address book
 *
 * Args: abook        -- The addrbook handle into access library
 *       command_line -- The screen line on which to prompt
 *       cur_line     -- The entry number in the display list
 *       warped       -- We warped to a new part of the addrbook
 *
 * Result: returns 1 if an entry was deleted, 0 if not.
 *
 * The main routine above knows what to repaint because it's always the
 * current entry that's deleted.  Here confirmation is asked of the user
 * and the appropriate adrbklib functions are called.
 */
int
addr_book_delete(abook, command_line, cur_line, warped)
    AdrBk *abook;
    int    command_line;
    long   cur_line;
    int   *warped;
{
    char   ch, *cmd, *dname;
    char   prompt[40+MAX_ADDRESS+1]; /* 40 is len of string constants below */
    int    rc = command_line; /* nuke warning about command_line unused */
    register AddrScrn_Disp *dl;
    AdrBk_Entry     *abe;
    DL_CACHE_S      *dlc_to_flush;

    dprint(2, (debugfile, "- addr_book_delete -\n"));

    if(warped)
      *warped = 0;

    dl  = dlist(cur_line);
    abe = adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Normal);

    switch(dl->type){
      case Simple:
	dname =	(abe->fullname && abe->fullname[0])
			? (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    abe->fullname, NULL)
			: abe->nickname ? abe->nickname : "";
        cmd   = "Really delete \"%.50s\"";
        break;

      case ListHead:
	dname =	(abe->fullname && abe->fullname[0])
			? (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    abe->fullname, NULL)
			: abe->nickname ? abe->nickname : "";
	cmd   = "Really delete ENTIRE list \"%.50s\"";
        break;

      case ListEnt:
        dname = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					    listmem_from_dl(abook, dl), NULL);
	cmd   = "Really delete \"%.100s\" from list";
        break;
    } 

    dname = dname ? dname : "";
    cmd   = cmd   ? cmd   : "";

    sprintf(prompt, cmd, dname);
    ch = want_to(prompt, 'n', 'n', NO_HELP, 0, 0);
    if(ch == 'y'){
	dlc_to_flush = get_dlc(cur_line);
	if(dl->type == Simple || dl->type == ListHead){
	    /*--- Kill a single entry or an entire list ---*/
            rc = adrbk_delete(abook, (a_c_arg_t)dl->elnum, 1);
	}
	else if(listmem_count_from_abe(abe) > 2){
            /*---- Kill an entry out of a list ----*/
            rc = adrbk_listdel(abook, (a_c_arg_t)dl->elnum,
		    listmem_from_dl(abook, dl));
	}
	else{
	    char *nick, *full, *addr, *fcc, *comment;
	    adrbk_cntr_t new_entry_num = NO_NEXT;

	    /*---- Convert a List to a Single entry ----*/

	    /* Save old info to be transferred */
	    nick    = cpystr(abe->nickname);
	    full    = cpystr(abe->fullname);
	    fcc     = cpystr(abe->fcc);
	    comment = cpystr(abe->extra);
	    if(listmem_count_from_abe(abe) == 2)
	      addr = cpystr(abe->addr.list[1 - dl->l_offset]);
	    else
	      addr = cpystr("");

            rc = adrbk_delete(abook, (a_c_arg_t)dl->elnum, 0);
	    if(rc == 0)
	      adrbk_add(abook,
			NO_NEXT,
			nick,
			full,
			addr,
			fcc,
			comment,
			Single,
			&new_entry_num,
			NULL);

	    fs_give((void **)&nick);
	    fs_give((void **)&full);
	    fs_give((void **)&fcc);
	    fs_give((void **)&comment);
	    fs_give((void **)&addr);

	    if(rc == 0){
		DL_CACHE_S dlc_restart;

		dlc_restart.adrbk_num = as.cur;
		dlc_restart.dlcelnum  = new_entry_num;
		dlc_restart.type = DlcSimple;
		warp_to_dlc(&dlc_restart, 0L);
		*warped = 1;
		return 1;
	    }
	}

	if(rc == 0){
	    q_status_message(SM_ORDER, 0, 3,
		"Entry deleted, address book updated");
            dprint(2, (debugfile, "abook: Entry %s\n",
		(dl->type == Simple || dl->type == ListHead) ? "deleted"
							     : "modified"));
	    /*
	     * Remove deleted line and everything after it from
	     * the dlc cache.  Next time we try to access those lines they
	     * will get filled in with the right info.
	     */
	    flush_dlc_from_cache(dlc_to_flush);
            return 1;
        }
	else{
	    PerAddrBook     *pab;

	    if(rc != -5)
              q_status_message1(SM_ORDER | SM_DING, 3, 5,
			      "Error updating address book: %s",
		    error_description(errno));
	    pab = &as.adrbks[as.cur];
            dprint(1, (debugfile, "Error deleting entry from %s (%s): %s\n",
		pab->nickname, pab->filename, error_description(errno)));
        }

	return 0;
    }
    else{
	q_status_message(SM_INFO, 0, 2, "Entry not deleted");
	return 0;
    }
}


/*
 * Edit a nickname field.
 *
 * Args: abook     -- the addressbook handle
 *       dl        -- display list line (NULL if new entry)
 *    command_line -- line to prompt on
 *       orig      -- nickname to edit
 *       prompt    -- prompt
 *      this_help  -- help
 * return_existing -- changes the behavior when a user types in a nickname
 *                    which already exists in this abook.  If not set, it
 *                    will just keep looping until the user changes; if set,
 *                    it will return -8 to the caller and orig will be set
 *                    to the matching nickname.
 *
 * Returns: -10 to cancel
 *          -9  no change
 *          -7  only case of nickname changed (only happens if dl set)
 *          -8  existing nickname chosen (only happens if return_existing set)
 *           0  new value copied into orig
 */
int
edit_nickname(abook, dl, command_line, orig, prompt, this_help,
		return_existing, takeaddr)
    AdrBk         *abook;
    AddrScrn_Disp *dl;
    int            command_line;
    char          *orig,
		  *prompt;
    HelpType       this_help;
    int            return_existing,
		   takeaddr;
{
    char         edit_buf[MAX_NICKNAME + 1];
    HelpType     help;
    int          rc;
    AdrBk_Entry *check, *passed_in_ae;
    ESCKEY_S     ekey[2];
    SAVE_STATE_S state;  /* For saving state of addrbooks temporarily */
    char        *error = NULL;

    ekey[0].ch    = ctrl('T');
    ekey[0].rval  = 2;
    ekey[0].name  = "^T";
    ekey[0].label = "To AddrBk";

    ekey[1].ch    = -1;

    edit_buf[MAX_NICKNAME] = '\0';
    strncpy(edit_buf, orig, MAX_NICKNAME);
    if(dl)
      passed_in_ae = adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Lock);
    else
      passed_in_ae = (AdrBk_Entry *)NULL;

    help  = NO_HELP;
    rc    = 0;
    check = NULL;
    do{
	if(error){
	    q_status_message(SM_ORDER, 3, 4, error);
	    fs_give((void **)&error);
	}

	/* display a message because adrbk_lookup_by_nick returned positive */
	if(check){
	    if(return_existing){
		strcpy(orig, edit_buf);
		if(passed_in_ae)
		  (void)adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Unlock);
		return -8;
	    }

            q_status_message1(SM_ORDER, 0, 4,
		    "Already an entry with nickname \"%s\"", edit_buf);
	}

	if(rc == 3)
          help = (help == NO_HELP ? this_help : NO_HELP);

	rc = optionally_enter(edit_buf, command_line, 0, MAX_NICKNAME, 1,
			  0, prompt, ekey, help, 0);

	if(rc == 1)  /* ^C */
	  break;

	if(rc == 2){ /* ^T */
	    void (*redraw) () = ps_global->redrawer;
	    char *returned_nickname;

	    push_titlebar_state();
	    save_state(&state);
	    if(takeaddr)
	      returned_nickname = addr_book_takeaddr();
	    else
	      returned_nickname = addr_book_selnick();

	    restore_state(&state);
	    if(returned_nickname){
		strncpy(edit_buf, returned_nickname, MAX_NICKNAME);
		fs_give((void **)&returned_nickname);
	    }

	    ClearScreen();
	    pop_titlebar_state();
	    redraw_titlebar();
	    if(ps_global->redrawer = redraw) /* reset old value, and test */
	      (*ps_global->redrawer)();
	}
            
    }while(rc == 2 ||
	   rc == 3 ||
	   rc == 4 ||
	   nickname_check(edit_buf, &error) ||
           ((check =
	       adrbk_lookup_by_nick(abook, edit_buf, (adrbk_cntr_t *)NULL)) &&
	     check != passed_in_ae));

    if(rc != 0){
	if(passed_in_ae)
	  (void)adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Unlock);

	return -10;
    }

    /* only the case of nickname changed */
    if(passed_in_ae && check == passed_in_ae && strcmp(edit_buf, orig)){
	(void)adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Unlock);
	strcpy(orig, edit_buf);
	return -7;
    }

    if(passed_in_ae)
      (void)adrbk_get_ae(abook, (a_c_arg_t)dl->elnum, Unlock);

    if(strcmp(edit_buf, orig) == 0) /* no change */
      return -9;
    
    strcpy(orig, edit_buf);
    return 0;
}


/*
 * return values of search_in_one_line are or'd combination of these
 */
#define MATCH_NICK      0x1  /* match in field 0 */
#define MATCH_FULL      0x2  /* match in field 1 */
#define MATCH_ADDR      0x4  /* match in field 2 */
#define MATCH_FCC       0x8  /* match in fcc field */
#define MATCH_COMMENT  0x10  /* match in comment field */
#define MATCH_BIGFIELD 0x20  /* match in one of the fields that crosses the
				whole screen, like a Title field */
#define MATCH_LISTMEM  0x40  /* match list member */
/*
 * Prompt user for search string and call find_in_book.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       command_line -- The screen line to prompt on
 *       new_line     -- Return value: new line position
 *       wrapped      -- Wrapped to beginning of display, tell user
 *       warped       -- Warped to a new location in the addrbook
 *
 * Result: The new line number is set if the search is successful.
 *         Returns 0 if found, -1 if not, -2 if cancelled.
 *       
 */
int
search_book(cur_line, command_line, new_line, wrapped, warped)
    long cur_line;
    int  command_line;
    long *new_line;
    int  *wrapped,
	 *warped;
{
    int          find_result, rc;
    static char  search_string[MAX_SEARCH + 1] = { '\0' };
    char         prompt[MAX_SEARCH + 50], nsearch_string[MAX_SEARCH+1];
    HelpType	 help;
    ESCKEY_S     ekey[4];
    PerAddrBook *pab;
    long         nl;

    dprint(7, (debugfile, "- search_book -\n"));

    sprintf(prompt, "Word to search for [%s]: ", search_string);
    help              = NO_HELP;
    nsearch_string[0] = '\0';

    ekey[0].ch    = 0;
    ekey[0].rval  = 0;
    ekey[0].name  = "";
    ekey[0].label = "";

    ekey[1].ch    = ctrl('Y');
    ekey[1].rval  = 10;
    ekey[1].name  = "^Y";
    ekey[1].label = "First Adr";

    ekey[2].ch    = ctrl('V');
    ekey[2].rval  = 11;
    ekey[2].name  = "^V";
    ekey[2].label = "Last Adr";

    ekey[3].ch    = -1;

    while(1){
        rc = optionally_enter(nsearch_string, command_line, 0, MAX_SEARCH,
                              1, 0, prompt, ekey, help, 0);
        if(rc == 3){
            help = help == NO_HELP ? h_oe_searchab : NO_HELP;
            continue;
        }
	else if(rc == 10){
	    *warped = 1;
	    warp_to_beginning();  /* go to top of addrbooks */
	    if((nl=first_selectable_line(0L)) != NO_LINE){
		*new_line = nl;
		q_status_message(SM_INFO, 0, 2, "Searched to first entry");
		return 0;
	    }
	    else{
		q_status_message(SM_INFO, 0, 2, "No entries");
		return -1;
	    }
	}
	else if(rc == 11){
	    *warped = 1;
	    warp_to_end();  /* go to bottom */
	    if((nl=first_selectable_line(0L)) != NO_LINE){
		*new_line = nl;
		q_status_message(SM_INFO, 0, 2, "Searched to last entry");
		return 0;
	    }
	    else{
		q_status_message(SM_INFO, 0, 2, "No entries");
		return -1;
	    }
	}

        if(rc != 4)
          break;
    }

        
    if(rc == 1 || (search_string[0] == '\0' && nsearch_string[0] == '\0'))
      return -2;

    if(nsearch_string[0] != '\0'){
        search_string[MAX_SEARCH] = '\0';
        strncpy(search_string, nsearch_string, MAX_SEARCH);
    }

    find_result = find_in_book(cur_line, search_string, new_line, wrapped);
    
    if(*wrapped)
      *warped = 1;

    if(find_result){
	int also = 0, notdisplayed = 0;

	pab = &as.adrbks[adrbk_num_from_lineno(*new_line)];
	if(find_result & MATCH_NICK){
	    if(pab->nick_is_displayed)
	      also++;
	    else
	      notdisplayed++;
	}

	if(find_result & MATCH_FULL){
	    if(pab->full_is_displayed)
	      also++;
	    else
	      notdisplayed++;
	}

	if(find_result & MATCH_ADDR){
	    if(pab->addr_is_displayed)
	      also++;
	    else
	      notdisplayed++;
	}

	if(find_result & MATCH_FCC){
	    if(pab->fcc_is_displayed)
	      also++;
	    else
	      notdisplayed++;
	}

	if(find_result & MATCH_COMMENT){
	    if(pab->comment_is_displayed)
	      also++;
	    else
	      notdisplayed++;
	}

	if(find_result & MATCH_LISTMEM){
	    AddrScrn_Disp *dl;

	    dl = dlist(*new_line);
	    if(F_OFF(F_EXPANDED_DISTLISTS,ps_global)
	      && !exp_is_expanded(pab->address_book->exp, (a_c_arg_t)dl->elnum))
	      notdisplayed++;
	}

	if(notdisplayed > 1)
	  q_status_message3(SM_ORDER,0,4, "%satched string in %s %sfields",
	      also ? "Also m" : "M",
	      comatose(notdisplayed),
	      also ? "other " : "");
	else if(notdisplayed == 1)
	  q_status_message2(SM_ORDER,0,4, "%satched string in %s",
	      also ? "Also m" : "M",
	      (find_result & MATCH_NICK && !pab->nick_is_displayed)     ?
							    "Nickname field"
                : (find_result & MATCH_FULL && !pab->full_is_displayed) ?
							    "Fullname field"
                : (find_result & MATCH_ADDR && !pab->addr_is_displayed) ?
							    "Address field"
                : (find_result & MATCH_FCC && !pab->fcc_is_displayed)   ?
							    "Fcc field"
                : (find_result & MATCH_COMMENT && !pab->comment_is_displayed) ?
							    "Comment field"
                : (find_result & MATCH_LISTMEM) ? "list member address" : "?");


	/* be sure to be on a selectable field */
	if(!line_is_selectable(*new_line))
	  if((nl=first_selectable_line(*new_line+1)) != NO_LINE)
	    *new_line = nl;
    }

    return(find_result ? 0 : -1);
}


/*
 * Search the display list for the given string.
 *
 * Args: cur_line     -- The current line position (in global display list)
 *			 of cursor
 *       string       -- String to search for
 *       new_line     -- Return value: new line position
 *       wrapped      -- Wrapped to beginning of display during search
 *
 * Result: The new line number is set if the search is successful.
 *         Returns 0 -- string not found
 *          Otherwise, a bitmask of which fields the string was found in.
 */
int
find_in_book(cur_line, string, new_line, wrapped)
    long  cur_line;
    char *string;
    long *new_line;
    int  *wrapped;
{
    register AddrScrn_Disp *dl;
    long                    nl, nl_save;
    int			    fields;
    AdrBk_Entry            *abe;
    char                   *listaddr = NULL;
    DL_CACHE_S             *dlc,
			    dlc_save; /* a local copy */


    dprint(9, (debugfile, "- find_in_book -\n"));

    /*
     * Save info to allow us to get back to where we were if we can't find
     * the string.  Also used to stop our search if we wrap back to the
     * start and search forward.
     */

    nl_save = cur_line;
    dlc = get_dlc(nl_save);
    dlc_save = *dlc;

    *wrapped = 0;
    nl = cur_line + 1L;

    /* start with next line and search to the end of the disp_list */
    dl = dlist(nl);
    while(dl->type != End){
	if(dl->type == Simple ||
	   dl->type == ListHead ||
	   dl->type == ListEnt ||
	   dl->type == ListClickHere){
	    abe = ae(nl);
	    if(dl->type == ListEnt)
	      listaddr = listmem(nl);
	}
	else
	  abe = (AdrBk_Entry *)NULL;

	if(fields=search_in_one_line(dl, abe, listaddr, string))
	  goto found;

	dl = dlist(++nl);
    }


    /*
     * Wrap back to the start of the addressbook and search forward
     * from there.
     */
    warp_to_beginning();  /* go to top of addrbooks */
    nl = 0L;  /* line number is always 0 after warp_to_beginning */
    *wrapped = 1;

    dlc = get_dlc(nl);
    while(!matching_dlcs(&dlc_save, dlc) && dlc->type != DlcEnd){

	fill_in_dl_field(dlc);
	dl = &dlc->dl;

	if(dl->type == Simple ||
	   dl->type == ListHead ||
	   dl->type == ListEnt ||
	   dl->type == ListClickHere){
	    abe = ae(nl);
	    if(dl->type == ListEnt)
	      listaddr = listmem(nl);
	}
	else
	  abe = (AdrBk_Entry *)NULL;

	if(fields=search_in_one_line(dl, abe, listaddr, string))
	  goto found;

	dlc = get_dlc(++nl);
    }

    /* see if it is in the current line */
    fill_in_dl_field(dlc);
    dl = &dlc->dl;

    if(dl->type == Simple ||
       dl->type == ListHead ||
       dl->type == ListEnt ||
       dl->type == ListClickHere){
	abe = ae(nl);
	if(dl->type == ListEnt)
	  listaddr = listmem(nl);
    }
    else
      abe = (AdrBk_Entry *)NULL;

    fields = search_in_one_line(dl, abe, listaddr, string);
    if((dl->type == Text || dl->type == Title) && dl->txt)
      fs_give((void **)&dl->txt);

    /* jump cache back to where we started */
    *wrapped = 0;
    warp_to_dlc(&dlc_save, nl_save);
    if(fields)
      *new_line = nl_save;  /* because it was in current line */

    nl = *new_line;

found:
    *new_line = nl;
    return(fields);
}


/*
 * Look in line dl for string.
 *
 * Args: dl     -- the display list for this line
 *       abe    -- AdrBk_Entry if it is an address type
 *     listaddr -- list member if it is of type ListEnt
 *       string -- look for this string
 *
 * Result:  0   -- string not found
 *          Otherwise, a bitmask of which fields the string was found in.
 *      MATCH_NICK      0x1
 *      MATCH_FULL      0x2
 *      MATCH_ADDR      0x4
 *      MATCH_FCC       0x8
 *      MATCH_COMMENT  0x10
 *      MATCH_BIGFIELD 0x20
 *      MATCH_LISTMEM  0x40
 */
int
search_in_one_line(dl, abe, listaddr, string)
    AddrScrn_Disp *dl;
    AdrBk_Entry   *abe;
    char          *listaddr;
    char          *string;
{
    register int c;
    int ret_val = 0;
    char **lm;

    for(c = 0; c < 5; c++){
      switch(c){
	case 0:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      if(srchstr(abe->nickname, string))
		ret_val |= MATCH_NICK;

	      break;

	    case Text:
	    case Title:
	      if(srchstr(dl->txt, string))
		ret_val |= MATCH_BIGFIELD;
	  }
	  break;

	case 1:
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      if(abe && srchstr(
			(char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						abe->fullname, NULL),
			string))
		ret_val |= MATCH_FULL;
	  }
	  break;

	case 2:
	  switch(dl->type){
	    case Simple:
	      if(srchstr((abe && abe->tag == Single) ?
		  abe->addr.addr : NULL, string))
		ret_val |= MATCH_ADDR;

	      break;

	    case ListEnt:
	      if(srchstr(
		 (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						     listaddr, NULL), string))
		ret_val |= MATCH_LISTMEM;

	      break;

	    case ClickHere:
	      if(srchstr(CLICKHERE, string))
		ret_val |= MATCH_BIGFIELD;

	      break;

	    case ListClickHere:
	      if(abe)
	        for(lm = abe->addr.list;
		    !(ret_val & MATCH_LISTMEM) && *lm; lm++)
	          if(srchstr(*lm, string))
		    ret_val |= MATCH_LISTMEM;

	      break;

	    case Empty:
	    case ListEmpty:
	      if(srchstr(EMPTY, string))
		ret_val |= MATCH_BIGFIELD;

	      break;
	  }
	  break;

	case 3:  /* fcc */
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      if(abe && srchstr(abe->fcc, string))
		ret_val |= MATCH_FCC;
	  }
	  break;

	case 4:  /* comment */
	  switch(dl->type){
	    case Simple:
	    case ListHead:
	      if(abe && srchstr(
		        (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						abe->extra, NULL),
			string))
		ret_val |= MATCH_COMMENT;
	  }
	  break;
      }
    }

    return(ret_val);
}


/*
 * Add an entry to address book.
 * It is for capturing addresses off incoming mail.
 * This is a front end for take_to_addrbooks.
 * It is also used for replacing an existing entry and for adding a single
 * new address to an existing list.
 *
 * The reason this is here is so that when Taking a single address, we can
 * rearrange the fullname to be Last, First instead of First Last.
 *
 * Args: ta_entry -- the entry from the take screen
 *   command_line -- line to prompt on
 *
 * Result: item is added to one of the address books,
 *       an error message is queued if appropriate.
 */
void
add_abook_entry(ta_entry, nick, fullname, fcc, comment, command_line)
    TA_S *ta_entry;
    char *nick;
    char *fullname;
    char *fcc;
    char *comment;
    int   command_line;
{
    ADDRESS *addr;
    char     new_fullname[MAX_FULLNAME + 1],
	     new_address[MAX_ADDRESS + 1];
    char   **new_list, *charset = NULL;
    char    *old_fullname;
    int      need_to_encode = 0;

    dprint(2, (debugfile, "-- add_abook_entry --\n"));

    /*-- rearrange full name (Last, First) ---*/
    new_fullname[0]              = '\0';
    new_fullname[MAX_FULLNAME]   = '\0';
    new_fullname[MAX_FULLNAME-1] = '\0';
    addr = ta_entry->addr;
    if(!fullname && addr->personal != NULL){
	/*
	 * In order to swap First Last to Last, First we have to decode
	 * and re-encode.
	 */
	old_fullname = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						addr->personal, &charset);
	if(old_fullname == addr->personal){
	    if(charset)
	      fs_give((void **)&charset);

	    charset = NULL;
	}
	else
	  need_to_encode++;

	if(strindex(old_fullname, ',') != NULL){
	    int add_quotes = 0;
	    char *nf;

	    nf = new_fullname;
	    /*
	     * We'll get this wrong if it is already quoted but the quote
	     * doesn't start right at the beginning.
	     */
	    if(old_fullname[0] != '"'){
		add_quotes++;
		*nf++ = '"';
	    }

	    strncpy(nf, old_fullname, MAX_FULLNAME-2);
	    if(add_quotes)
	      strcat(nf, "\"");
	}
	else if(strindex(old_fullname, SPACE) == NULL){  /* leave word */
	    strncpy(new_fullname, old_fullname, MAX_FULLNAME);
	}
	else{
	    char *p, *q, *r;

	    /* switch to Last, First */
	    p = old_fullname;
	    while((q = strindex(p, SPACE)) != NULL)
	      p = q + 1;

	    for(q = p, r = new_fullname; *q; *r++ = *q++)
	      ;/* do nothing */

	    *r++ = ',';
	    *r++ = SPACE;
	    for(q = old_fullname; q < p; *r++ = *q++)
	      ;/* do nothing */

	    *r = '\0';
	    for(r--;
		r >= new_fullname && isspace((unsigned char)*r);
		*r-- = '\0')
	      ;/* do nothing */
	}
    }

    if(need_to_encode){
	char *s;
	char  buf[MAX_FULLNAME + 10];

	/* re-encode in original charset */
	s = rfc1522_encode(buf, (unsigned char *)new_fullname,
	    charset ? charset : ps_global->VAR_CHAR_SET);
	if(s != new_fullname)
	  strncpy(new_fullname, s, MAX_FULLNAME);
	
	new_fullname[MAX_FULLNAME-1] = '\0';
	if(charset)
	  fs_give((void **)&charset);
    }

    /* initial value for new address */
    new_address[0] = '\0';
    new_address[MAX_ADDRESS] = '\0';
    if(addr->mailbox){
	char *scratch, *p, *t, *u;
	unsigned long l;

	scratch = (char *)fs_get((size_t)est_size(addr));
	scratch[0] = '\0';
	rfc822_write_address(scratch, addr);
	if(p = srchstr(scratch, "@.RAW-FIELD.")){
	  for(t = p; ; t--)
	    if(*t == '&'){		/* find "leading" token */
		*t++ = ' ';		/* replace token */
		*p = '\0';		/* tie off string */
		u = (char *)rfc822_base64((unsigned char *)t,
					  (unsigned long)strlen(t), &l);
		*p = '@';		/* restore 'p' */
		rplstr(p, 12, "");	/* clear special token */
		rplstr(t, strlen(t), u);
		fs_give((void **)&u);
	    }
	    else if(t == scratch)
	      break;
	}

	strncpy(new_address, scratch, MAX_ADDRESS);

	if(scratch)
	  fs_give((void **)&scratch);
    }

    if(ta_entry->frwrded){
	ADDRESS *a;
	int i;
	char buf[MAX_ADDR_EXPN+1];

	for(i = 0, a = addr; a; i++, a = a->next)
	  ;/* just counting for alloc below */

	new_list = (char **)fs_get((i+1) * sizeof(char *));
	for(i = 0, a = addr; a; i++, a = a->next)
	  new_list[i] = cpystr(addr_string(a, buf));

	new_list[i] = NULL;
    }
    else{
	new_list = (char **)fs_get(2 * sizeof(char *));
	new_list[0] = cpystr(ta_entry->strvalue);
	new_list[1] = NULL;
    }

    take_to_addrbooks_frontend(new_list, nick,
			       fullname ? fullname : new_fullname,
			       new_address, fcc, comment, command_line);
    free_list(&new_list);
}


void
take_to_addrbooks_frontend(new_entries, nick, fullname, addr, fcc,
			    comment, cmdline)
    char **new_entries;
    char  *nick;
    char  *fullname;
    char  *addr;
    char  *fcc;
    char  *comment;
    int    cmdline;
{
    jmp_buf        save_jmp_buf;

    dprint(2, (debugfile, "-- take_to_addrbooks_frontend --\n"));

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	q_status_message(SM_ORDER, 5, 10, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... take_to_addrbooks_frontend!\n"));
	addrbook_reset();
    }

    take_to_addrbooks(new_entries, nick, fullname, addr, fcc, comment, cmdline);
    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
}


/*
 * Add to address book, called from take screen.
 * It is also used for adding to an existing list or replacing an existing
 * entry.
 *
 * Args: new_entries -- a list of addresses to add to a list or to form
 *                      a new list with
 *              nick -- if adding new entry, suggest this for nickname
 *          fullname -- if adding new entry, use this for fullname
 *              addr -- if only one new_entry, this is its addr
 *               fcc -- if adding new entry, use this for fcc
 *           comment -- if adding new entry, use this for comment
 *      command_line -- line to prompt on
 *
 * Result: item is added to one of the address books,
 *       an error message is queued if appropriate.
 */
void
take_to_addrbooks(new_entries, nick, fullname, addr, fcc, comment, command_line)
    char **new_entries;
    char  *nick;
    char  *fullname;
    char  *addr;
    char  *fcc;
    char  *comment;
    int    command_line;
{
    char          new_nickname[MAX_NICKNAME + 1];
    char          prompt[200], **p;
    int           rc, listadd = 0, ans, i;
    AdrBk        *abook;
    SAVE_STATE_S  state;
    PerAddrBook  *pab;
    AdrBk_Entry  *abe = (AdrBk_Entry *)NULL, *abe_copy;
    adrbk_cntr_t  entry_num = NO_NEXT;
    size_t        tot_size, new_size, old_size;
    Tag           old_tag = NotSet;

    dprint(2, (debugfile, "-- take_to_addrbooks --\n"));

    pab = setup_for_addrbook_add(&state, command_line);

    /* check we got it opened ok */
    if(pab == NULL || pab->address_book == NULL)
      goto take_to_addrbooks_cancel;

    abook = pab->address_book;

    /*----- nickname ------*/
    sprintf(prompt,
      "Enter new or existing nickname (one word and easy to remember): ");
    new_nickname[0] = '\0';
    if(nick){
	strncpy(new_nickname, nick, MAX_NICKNAME);
	new_nickname[MAX_NICKNAME] = '\0';
    }

    rc = edit_nickname(abook, (AddrScrn_Disp *)NULL, command_line,
		new_nickname, prompt, h_oe_takenick, 1, 1);
    if(rc == -8){  /* this means an existing nickname was entered */
	static ESCKEY_S choices[] = {
	    {'r', 'r', "R", "Replace"},
	    {'a', 'a', "A", "Add"},
	    {-1, 0, NULL, NULL}};

	abe = adrbk_lookup_by_nick(abook, new_nickname, &entry_num);
	if(!abe){  /* this shouldn't happen */
	    q_status_message1(SM_ORDER, 0, 4,
		"Already an entry %s in address book!",
		new_nickname);
	    goto take_to_addrbooks_cancel;
	}

	old_tag = abe->tag;

	sprintf(prompt,
	    "%s %s (%s) exists, replace or add addresses to it ? ",
	    abe->tag == List ? "List" : "Entry",
	    new_nickname,
	    (abe->fullname && abe->fullname[0])
			? (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						    abe->fullname, NULL)
			: "<no long name>");

	ans = radio_buttons(prompt,
			    command_line,
			    choices,
			    'a',
			    'x',
			    h_oe_take_or_replace,
			    RB_NORM);
	if(ans == 'a')
	  listadd++;
	else if(ans != 'r')
	  goto take_to_addrbooks_cancel;
    }
    else if(rc != 0 && rc != -9)  /* -9 means a null nickname */
      goto take_to_addrbooks_cancel;

    if((long)abook->count > MAX_ADRBK_SIZE ||
       (old_tag == NotSet && (long)abook->count >= MAX_ADRBK_SIZE)){
	q_status_message(SM_ORDER, 3, 5,
	    "Address book is at maximum size. TakeAddr cancelled.");
	dprint(2, (debugfile, "Addrbook at Max size, TakeAddr cancelled\n"));
	goto take_to_addrbooks_cancel;
    }

    if(listadd){
	/* count up size of existing list */
	if(abe->tag == List){
	    for(p = abe->addr.list; p != NULL && *p != NULL; p++)
	      ;/* do nothing */
	
	    old_size = p - abe->addr.list;
	}
	/* or size of existing single address */
	else
	  old_size = 1;
    }
    else /* don't care about old size, they will be tossed in edit_entry */
      old_size = 0;

    /* make up an abe to pass to edit_entry */
    abe_copy = adrbk_newentry();
    abe_copy->nickname = cpystr(new_nickname);
    abe_copy->tag = List;

    if(listadd){
	abe_copy->fullname = cpystr((abe->fullname && abe->fullname[0])
							? abe->fullname : "");
	abe_copy->fcc   = cpystr((abe->fcc && abe->fcc[0]) ? abe->fcc : "");
	abe_copy->extra = cpystr((abe->extra&&abe->extra[0]) ? abe->extra : "");
    }
    else{
	/* use passed in info if available */
	abe_copy->fullname = cpystr(fullname ? fullname : "");
	abe_copy->fcc      = cpystr(fcc ? fcc : "");
	abe_copy->extra    = cpystr(comment ? comment : "");
    }

    /* count up size of new list */
    for(p = new_entries; p != NULL && *p != NULL; p++)
      ;/* do nothing */
    
    new_size = p - new_entries;
    tot_size = old_size + new_size;
    abe_copy->addr.list = (char **)fs_get((tot_size+1) * sizeof(char *));
    memset((void *)abe_copy->addr.list, 0, (tot_size+1) * sizeof(char *));
    if(old_size > 0){
	if(abe->tag == List){
	    for(i = 0; i < old_size; i++)
	      abe_copy->addr.list[i] = cpystr(abe->addr.list[i]);
	}
	else
	  abe_copy->addr.list[0] = cpystr(abe->addr.addr);
    }

    /* add new addresses to list */
    if(tot_size == 1 && addr)
      abe_copy->addr.list[0] = cpystr(addr);
    else
      for(i = 0; i < new_size; i++)
        abe_copy->addr.list[old_size + i] = cpystr(new_entries[i]);

    abe_copy->addr.list[tot_size] = NULL;

    edit_entry(abook, abe_copy, (a_c_arg_t)entry_num, old_tag, 0, NULL);

    /* free copy */
    free_ae(abook, &abe_copy);
    restore_state(&state);
    return;

take_to_addrbooks_cancel:
    cancel_warning(NO_DING, "addition");
    restore_state(&state);
}


/*
 * Prep addrbook for TakeAddr add operation.
 *
 * Arg: savep -- Address of a pointer to save addrbook state in.
 *      stp   -- Address of a pointer to save addrbook state in.
 *
 * Returns: a PerAddrBook pointer, or NULL.
 */
PerAddrBook *
setup_for_addrbook_add(state, command_line)
    SAVE_STATE_S *state;
    int	          command_line;
{
    PerAddrBook *pab;

    init_ab_if_needed();
    save_state(state);

    if(as.n_addrbk == 0){
        q_status_message(SM_ORDER, 3, 4, "Can't open address book!");
        return NULL;
    }
    else
      pab = use_this_addrbook(command_line);
    
    if(!pab)
      return NULL;

    /* initialize addrbook so we can add to it */
    init_abook(pab, Open);

    if(pab->ostatus != Open){
        q_status_message(SM_ORDER, 3, 4, "Can't open address book!");
        return NULL;
    }

    if(pab->access != ReadWrite){
	if(pab->access == ReadOnly)
	  readonly_warning(NO_DING, NULL);
	else if(pab->access == NoAccess)
	  q_status_message(SM_ORDER, 3, 4,
		"AddressBook not accessible, permission denied");

        return NULL;
    }

    return(pab);
}


/*
 *  Interface to address book lookups for callers outside or inside this file.
 *
 * Args: nickname       -- The nickname to look up
 *       which_addrbook -- If matched, addrbook number it was found in.
 *       not_here       -- If non-negative, skip looking in this abook.
 *
 * Result: returns NULL or the corresponding fullname.  The fullname is
 * allocated here so the caller must free it.
 *
 * This opens the address books if they haven't been opened and restores
 * them to the state they were in upon entry.
 */
char *
addr_lookup(nickname, which_addrbook, not_here)
    char *nickname;
    int  *which_addrbook;
    int   not_here;
{
    AdrBk_Entry  *abe;
    SAVE_STATE_S  state;
    char         *fullname;

    dprint(9, (debugfile, "- addr_lookup -\n"));

    init_ab_if_needed();
    save_state(&state);

    abe = adrbk_lookup_with_opens_by_nick(nickname,0,which_addrbook,not_here);

    fullname = (abe && abe->fullname) ? cpystr(abe->fullname) : NULL;

    restore_state(&state);

    return(fullname);
}


/*
 * These chars in nicknames will mess up parsing.
 *
 * Returns 0 if ok, 1 if not.
 * Returns an allocated error message on error.
 */
int
nickname_check(nickname, error)
    char  *nickname;
    char **error;
{
    register char *t;
    char buf[100];

    if((t = strindex(nickname, SPACE)) ||
       (t = strindex(nickname, ',')) ||
       (t = strindex(nickname, '"')) ||
       (t = strindex(nickname, ';')) ||
       (t = strindex(nickname, ':')) ||
       (t = strindex(nickname, '@')) ||
       (t = strindex(nickname, '(')) ||
       (t = strindex(nickname, ')')) ||
       (t = strindex(nickname, '\\')) ||
       (t = strindex(nickname, '[')) ||
       (t = strindex(nickname, ']')) ||
       (t = strindex(nickname, '<')) ||
       (t = strindex(nickname, '>'))){
	char s[4];
	s[0] = '"';
	s[1] = *t;
	s[2] = '"';
	s[3] = '\0';
	if(error){
	    sprintf(buf, "%s not allowed in nicknames",
		*t == SPACE ?
		    "Blank spaces" :
		    *t == ',' ?
			"Commas" :
			*t == '"' ?
			    "Quotes" :
			    s);
	    *error = cpystr(buf);
	}

	return 1;
    }

    return 0;
}


/*
 * This is like build_address() only it doesn't close
 * everything down when it is done, and it doesn't open addrbooks that
 * are already open.  Other than that, it has the same functionality.
 * It opens addrbooks that haven't been opened and saves and restores the
 * addrbooks open states (if save_and_restore is set).
 *
 * Args: to                    -- the address to attempt expanding (see the
 *				   description in expand_address)
 *       full_to               -- a pointer to result
 *		    This will be allocated here and freed by the caller.
 *       error                 -- a pointer to an error message, if non-null
 *       fcc                   -- a pointer to returned fcc, if non-null
 *		    This will be allocated here and freed by the caller.
 *		    *fcc should be null on entry.
 *       save_and_restore      -- restore addrbook states when finished
 *
 * Results:    0 -- address is ok
 *            -1 -- address is not ok
 * full_to contains the expanded address on success, or a copy of to
 *         on failure
 * *error  will point to an error message on failure it it is non-null
 *
 * Side effect: Can flush addrbook entry cache entries so they need to be
 * re-fetched afterwords.
 */
int
our_build_address(to, full_to, error, fcc, save_and_restore, simple_verify)
    BuildTo to;
    char  **full_to,
	  **error,
	  **fcc;
    int     save_and_restore, simple_verify;
{
    int ret;

    dprint(7, (debugfile, "- our_build_address -  (%s)\n",
	(to.type == Str) ? (to.arg.str ? to.arg.str : "nul")
			 : (to.arg.abe->nickname ? to.arg.abe->nickname
						: "no nick")));

    if(to.type == Str && !to.arg.str || to.type == Abe && !to.arg.abe){
	if(full_to)
	  *full_to = cpystr("");
	ret = 0;
    }
    else
      ret = build_address_internal(to, full_to, error, fcc, NULL, NULL,
			       save_and_restore, simple_verify);

    dprint(8, (debugfile, "   our_build_address says %s address\n",
	ret ? "BAD" : "GOOD"));

    return(ret);
}


/*
 * This is the builder used by the composer for the Lcc line.
 *
 * Args: lcc     -- the passed in Lcc line to parse
 *      full_lcc -- Address of a pointer to return the full address in.
 *		    This will be allocated here and freed by the caller.
 *       error   -- Address of a pointer to return an error message in.
 *                  This is not allocated so should not be freed by the caller.
 *       to_line -- This is a pointer to text for affected entries which
 *		    we may be changing.  The first one in the list is the
 *		    To entry.  We may put the name of the list in empty
 *		    group syntax form there (like  List Name: ;).
 *		    The second one in the list is the fcc field.
 *		    The tptr members already point to text allocated in the
 *		    caller.  We may free and reallocate here, caller will
 *		    free the result in any case.
 *
 * Result:  0 is returned if address was OK, 
 *         -1 if address wasn't OK.
 *
 * Side effect: Can flush addrbook entry cache entries so they need to be
 * re-fetched afterwords.
 */
int
build_addr_lcc(lcc, full_lcc, error, to_line)
    char	 *lcc,
		**full_lcc,
		**error;
    BUILDER_ARG	 *to_line;
{
    int		    ret_val,
		    no_repo = 0;	/* fcc or lcc not reproducible */
    BuildTo	    bldlcc;
    BuilderPrivate *bp = NULL;
    char	   *p,
		   *fcc_local = NULL,
		   *to = NULL,
		   *dummy;
    long	    csum;
    jmp_buf         save_jmp_buf;

    dprint(2, (debugfile, "- build_addr_lcc - (%s)\n", lcc ? lcc : "nul"));

    /* check to see if to string is empty to avoid work */
    for(p = lcc; p && *p && isspace((unsigned char)*p); p++)
      ;/* do nothing */

    if(!p || !*p){
	if(full_lcc)
	  *full_lcc = cpystr(lcc ? lcc : ""); /* because pico does a strcmp() */

	return 0;
    }

    if(error != NULL)
      *error = (char *)NULL;

    /*
     * If we end up jumping back here because somebody else changed one of
     * our addrbooks out from underneath us, we may well leak some memory.
     * That's probably ok since this will be very rare.
     */
    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	if(full_lcc && *full_lcc)
	  fs_give((void **)full_lcc);

	q_status_message(SM_ORDER, 3, 5, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... build_address(%s)!\n", lcc));
	addrbook_reset();
    }

    bldlcc.type    = Str;
    bldlcc.arg.str = lcc;
    /*
     * Lcc is first affected_entry and fcc is second.
     * The conditional stuff for the fcc argument says to only change the
     * fcc if the fcc pointer is passed in non-null, and the To pointer
     * is also non-null.  If they are null, that means they've already been
     * entered (are sticky).  We don't affect fcc if either fcc or To has
     * been typed in.
     */
    ret_val = build_address_internal(bldlcc,
		full_lcc,
		error,
		(to_line && to_line->next && to_line->next->tptr
		   && to_line->tptr) ? &fcc_local : NULL,
		&no_repo,
		(to_line && to_line->tptr) ? &to : NULL,
		1, 0);

    /* full_lcc is what ends up in the Lcc: line */
    if(full_lcc && *full_lcc){
	/*
	 * Have to rfc1522_decode the full_to string before sending it back.
	 * This should not be necessary anymore since it is being decoded
	 * in build_address_internal.  Just being safe.
	 */
	p = (char *)fs_get((strlen(*full_lcc) + 1) * sizeof(char));
	dummy = NULL;
	if(rfc1522_decode((unsigned char *)p, *full_lcc, &dummy)
						== (unsigned char *)p){
	    fs_give((void **)full_lcc);
	    *full_lcc = p;
	}
	else
	  fs_give((void **)&p);

	if(dummy)
	  fs_give((void **)&dummy);
    }

    /* to is what ends up in the To: line */
    if(to && *to){
	/*
	 * Have to rfc1522_decode the full_to string before sending it back.
	 * This should not be necessary anymore since it is being decoded
	 * in build_address_internal.  Just being safe.
	 */
	p = (char *)fs_get((strlen(to) + 1) * sizeof(char));
	dummy = NULL;
	if(rfc1522_decode((unsigned char *)p, to, &dummy)
						== (unsigned char *)p){
	    fs_give((void **)&to);
	    to = p;
	}
	else
	  fs_give((void **)&p);

	if(dummy)
	  fs_give((void **)&dummy);

	if(to_line->xtra)
	  bp = (BuilderPrivate *)(*(to_line->xtra));

	if(bp && bp->who == BP_Lcc){
	    int len;

	    len = strlen(lcc);
	    if(len >= bp->cksumlen){
		int save;

		save = lcc[bp->cksumlen];
		lcc[bp->cksumlen] = '\0';
		csum = line_hash(lcc);
		lcc[bp->cksumlen] = save;
	    }
	    else
	      csum = bp->cksumval + 1;
	}

	if(!bp || (bp->who == BP_Lcc && csum != bp->cksumval)){
	    /* replace to value */
	    if(to_line->tptr)
	      fs_give((void **)&to_line->tptr);

	    to_line->tptr = to;

	    /* put in new xtra for next time through */
	    if(to_line->xtra){
		if(*(to_line->xtra))
		  fs_give(to_line->xtra);

		/* only need an xtra if no_repo is set */
		if(no_repo){
		    *(to_line->xtra)
			      = (void *)fs_get(sizeof(struct builder_private));
		    bp = (BuilderPrivate *)(*(to_line->xtra));
		    bp->who = BP_Lcc;
		    bp->cksumlen = strlen((full_lcc && *full_lcc)
					    ? *full_lcc : "");
		    bp->cksumval = line_hash((full_lcc && *full_lcc)
					    ? *full_lcc : "");
		}
	    }
	}
	else
	  fs_give((void **)&to);
    }

    if(fcc_local){
	/*
	 * If xtra is set, that means fcc was set from a list
	 * during some previous builder call.
	 * If the current To line contains xtra as a prefix, then
	 * we should leave things as they are.  In order to decide
	 * that, we look at a hash value computed from the two strings.
	 */
	bp = NULL;
	if(to_line->next->xtra)
	  bp = (BuilderPrivate *)(*(to_line->next->xtra));

	if(bp && bp->who == BP_Lcc){
	    int len;

	    len = strlen(lcc);
	    if(len >= bp->cksumlen){
		int save;

		save = lcc[bp->cksumlen];
		lcc[bp->cksumlen] = '\0';
		csum = line_hash(lcc);
		lcc[bp->cksumlen] = save;
	    }
	    else
	      csum = bp->cksumval + 1;
	}

	if(!bp || (bp->who == BP_Lcc && csum != bp->cksumval)){
	    /* replace fcc value */
	    if(to_line->next->tptr)
	      fs_give((void **)&to_line->next->tptr);

	    to_line->next->tptr = fcc_local;

	    /* put in new xtra for next time through */
	    if(to_line->next->xtra){
		if(*(to_line->next->xtra))
		  fs_give(to_line->next->xtra);

		/* only need an xtra if no_repo is set */
		if(no_repo){
		    *(to_line->next->xtra)
			      = (void *)fs_get(sizeof(struct builder_private));
		    bp = (BuilderPrivate *)(*(to_line->next->xtra));
		    bp->who = BP_Lcc;
		    bp->cksumlen = strlen((full_lcc && *full_lcc)
					    ? *full_lcc : "");
		    bp->cksumval = line_hash((full_lcc && *full_lcc)
					    ? *full_lcc : "");
		}
	    }
	}
	else
	  fs_give((void **)&fcc_local);
    }


    if(error != NULL && *error == NULL)
      *error = cpystr("");

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(ret_val);
}


/*
 * This is the build_address used by the composer to check for an address
 * in the addrbook.
 *
 * Args: to      -- the passed in line to parse
 *       full_to -- Address of a pointer to return the full address in.
 *		    This will be allocated here and freed by the caller.
 *       error   -- Address of a pointer to return an error message in.
 *		    This will be allocated here and freed by the caller.
 *       fcc     -- Address of a pointer to return the fcc in is in
 *		    fcc->tptr.  It will have already been allocated by the
 *		    caller but we may free it and reallocate if we wish.
 *		    Caller will free it.
 *
 * Result:  0 is returned if address was OK, 
 *         -1 if address wasn't OK.
 *
 * Side effect: Can flush addrbook entry cache entries so they need to be
 * re-fetched afterwords.
 */
int
build_address(to, full_to, error, fcc)
    char	 *to,
		**full_to,
		**error;
    BUILDER_ARG	 *fcc;
{
    char   *p;
    int     ret_val, no_repo = 0;
    BuildTo bldto;
    BuilderPrivate *bp = NULL;
    char   *fcc_local = NULL, *dummy = NULL;
    long    csum;
    jmp_buf save_jmp_buf;

    dprint(2, (debugfile, "- build_address - (%s)\n", to ? to : "nul"));

    /* check to see if to string is empty to avoid work */
    for(p = to; p && *p && isspace((unsigned char)*p); p++)
      ;/* do nothing */

    if(!p || !*p){
	if(full_to)
	  *full_to = cpystr(to ? to : "");  /* because pico does a strcmp() */

	return 0;
    }

    if(full_to != NULL)
      *full_to = (char *)NULL;

    if(error != NULL)
      *error = (char *)NULL;

    /*
     * If we end up jumping back here because somebody else changed one of
     * our addrbooks out from underneath us, we may well leak some memory.
     * That's probably ok since this will be very rare.
     *
     * The reason for the memcpy of the jmp_buf is that we may actually
     * be indirectly calling this function from within the address book.
     * For example, we may be in the address book screen and then run
     * the ComposeTo command which puts us in the composer, then we call
     * build_address from there which resets addrbook_changed_unexpectedly.
     * Once we leave build_address we need to reset addrbook_changed_un...
     * because this position on the stack will no longer be valid.
     * Same is true of the other setjmp's in this file which are wrapped
     * in memcpy calls.
     */
    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	if(full_to && *full_to)
	  fs_give((void **)full_to);

	q_status_message(SM_ORDER, 3, 5, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... build_address(%s)!\n", to));
	addrbook_reset();
    }

    bldto.type    = Str;
    bldto.arg.str = to;
    ret_val = build_address_internal(bldto, full_to, error,
		    fcc ? &fcc_local : NULL, &no_repo, NULL, 1, 0);

    /*
     * Have to rfc1522_decode the full_to string before sending it back.
     * This should not be necessary anymore since it is being decoded
     * in build_address_internal.  Just being safe.
     */
    if(full_to && *full_to ){
	p = (char *)fs_get((strlen(*full_to) + 1) * sizeof(char));
	if(rfc1522_decode((unsigned char *)p, *full_to, &dummy)
						    == (unsigned char *)p){
	    fs_give((void **)full_to);
	    *full_to = p;
	}
	else
	  fs_give((void **)&p);

	if(dummy)
	  fs_give((void **)&dummy);
    }

    if(fcc_local){
	/*
	 * If fcc->xtra is set, that means fcc was set from a list
	 * during some previous builder call.
	 * If the current To line contains the old expansion as a prefix, then
	 * we should leave things as they are.  In order to decide that,
	 * we look at a hash value computed from the strings.
	 */
	if(fcc->xtra)
	  bp = (BuilderPrivate *)(*(fcc->xtra));

	if(bp && bp->who == BP_To){
	    int len;

	    len = strlen(to);
	    if(len >= bp->cksumlen){
		int save;

		save = to[bp->cksumlen];
		to[bp->cksumlen] = '\0';
		csum = line_hash(to);
		to[bp->cksumlen] = save;
	    }
	    else
	      csum = bp->cksumval + 1;  /* something not equal to cksumval */
	}

	if(!bp || (bp->who == BP_To && csum != bp->cksumval)){
	    /* replace fcc value */
	    if(fcc->tptr)
	      fs_give((void **)&fcc->tptr);

	    fcc->tptr = fcc_local;

	    /* put in new xtra for next time through */
	    if(fcc->xtra){
		if(*(fcc->xtra))
		  fs_give(fcc->xtra);

		/* only need an xtra if no_repo is true */
		if(no_repo){
		    *(fcc->xtra)
			    = (void *)fs_get(sizeof(struct builder_private));
		    bp = (BuilderPrivate *)(*(fcc->xtra));
		    bp->who = BP_To;
		    bp->cksumlen = strlen(((full_to && *full_to)
					    ? *full_to : ""));
		    bp->cksumval = line_hash(((full_to && *full_to)
					    ? *full_to : ""));
		}
	    }
	}
	else
	  fs_give((void **)&fcc_local);  /* unused in this case */
    }

    /* This is so pico will erase the old message */
    if(error != NULL && *error == NULL)
      *error = cpystr("");

    memcpy(addrbook_changed_unexpectedly, save_jmp_buf, sizeof(jmp_buf));
    return(ret_val);
}


#define FCC_SET    1	/* Fcc was set */
#define LCC_SET    2	/* Lcc was set */
#define FCC_NOREPO 4	/* Fcc was set in a non-reproducible way */
#define LCC_NOREPO 8	/* Lcc was set in a non-reproducible way */
/*
 * Given an address, expand it based on address books, local domain, etc.
 * This will open addrbooks if needed before checking (actually one of
 * its children will open them).
 *
 * Args: to       -- The given address to expand (see the description
 *			in expand_address)
 *       full_to  -- Returned value after parsing to.
 *       error    -- This gets pointed at error message, if any
 *       fcc      -- Returned value of fcc for first addr in to
 *       no_repo  -- Returned value, set to 1 if the fcc or lcc we're
 *			returning is not reproducible from the expanded
 *			address.  That is, if we were to run
 *			build_address_internal again on the resulting full_to,
 *			we wouldn't get back the fcc again.  For example,
 *			if we expand a list and use the list fcc from the
 *			addrbook, the full_to no longer contains the
 *			information that this was originally list foo.
 *       save_and_restore -- restore addrbook state when done
 *
 * Result:  0 is returned if address was OK, 
 *         -1 if address wasn't OK.
 * The address is expanded, fully-qualified, and personal name added.
 *
 * Input may have more than one address separated by commas.
 *
 * Side effect: Can flush addrbook entry cache entries so they need to be
 * re-fetched afterwords.
 */
int
build_address_internal(to, full_to, error, fcc, no_repo, lcc, save_and_restore,
		       simple_verify)
    BuildTo to;
    char  **full_to,
	  **error,
	  **fcc,
	  **lcc;
    int    *no_repo;
    int     save_and_restore, simple_verify;
{
    ADDRESS *a;
    int      loop, i;
    int      tried_route_addr_hack = 0;
    int      did_set = 0;
    char    *tmp = NULL;
    SAVE_STATE_S  state;
    PerAddrBook  *pab;

    dprint(8, (debugfile, "- build_address_internal -  (%s)\n",
	(to.type == Str) ? (to.arg.str ? to.arg.str : "nul")
			 : (to.arg.abe->nickname ? to.arg.abe->nickname
						: "no nick")));

    init_ab_if_needed();
    if(save_and_restore)
      save_state(&state);

start:
    loop = 0;
    ps_global->c_client_error[0] = '\0';

    a = expand_address(to, ps_global->maildomain,
		       F_OFF(F_QUELL_LOCAL_LOOKUP, ps_global)
			 ? ps_global->maildomain : NULL,
		       &loop, fcc, &did_set, lcc, error,
		       0, simple_verify);

    /*
     * If the address is a route-addr, expand_address() will have rejected
     * it unless it was enclosed in brackets, since route-addrs can't stand
     * alone.  Try it again with brackets.  We should really be checking
     * each address in the list of addresses instead of assuming there is
     * only one address, but we don't want to have this function know
     * all about parsing rfc822 addrs.
     */
    if(!tried_route_addr_hack &&
        ps_global->c_client_error[0] != '\0' &&
	((to.type == Str && to.arg.str && to.arg.str[0] == '@') ||
	 (to.type == Abe && to.arg.abe->tag == Single &&
				 to.arg.abe->addr.addr[0] == '@'))){
	BuildTo      bldto;

	tried_route_addr_hack++;

	tmp = (char *)fs_get((size_t)(MAX_ADDR_FIELD + 3));

	/* add brackets to whole thing */
	if(to.type == Str)
	  strcat(strcat(strcpy(tmp, "<"), to.arg.str), ">");
	else
	  strcat(strcat(strcpy(tmp, "<"), to.arg.abe->addr.addr), ">");

	loop = 0;
	ps_global->c_client_error[0] = '\0';

	bldto.type    = Str;
	bldto.arg.str = tmp;

	if(a)
	  mail_free_address(&a);

	/* try it */
	a = expand_address(bldto, ps_global->maildomain,
			   F_OFF(F_QUELL_LOCAL_LOOKUP, ps_global)
			     ? ps_global->maildomain : NULL,
		       &loop, fcc, &did_set, lcc, error,
		       0, simple_verify);

	/* if no error this time, use it */
	if(ps_global->c_client_error[0] == '\0'){
	    if(save_and_restore)
	      restore_state(&state);

	    /*
	     * Clear references so that Addrbook Entry caching in adrbklib.c
	     * is allowed to throw them out of cache.
	     */
	    for(i = 0; i < as.n_addrbk; i++){
		pab = &as.adrbks[i];
		if(pab->ostatus == Open || pab->ostatus == NoDisplay)
		  adrbk_clearrefs(pab->address_book);
	    }

	    goto ok;
	}
	else  /* go back and use what we had before, so we get the error */
	  goto start;
    }

    if(save_and_restore)
      restore_state(&state);

    /*
     * Clear references so that Addrbook Entry caching in adrbklib.c
     * is allowed to throw them out of cache.
     */
    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	if(pab->ostatus == Open || pab->ostatus == NoDisplay)
	  adrbk_clearrefs(pab->address_book);
    }

    if(ps_global->c_client_error[0] != '\0'){
        /* Parse error.  Return string as is and error message */
	if(full_to){
	    if(to.type == Str)
	      *full_to = cpystr(to.arg.str);
	    else{
	        if(to.arg.abe->nickname && to.arg.abe->nickname[0])
	          *full_to = cpystr(to.arg.abe->nickname);
	        else if(to.arg.abe->tag == Single)
	          *full_to = cpystr(to.arg.abe->addr.addr);
	        else
	          *full_to = cpystr("");
	    }
	}

        if(error != NULL)
          *error = cpystr(ps_global->c_client_error);

        dprint(2, (debugfile,
	    "build_address_internal returning parse error: %s\n",
                   ps_global->c_client_error));
	if(a)
	  mail_free_address(&a);

	if(tmp)
	  fs_give((void **)&tmp);

        return -1;
    }

    /*
     * If there's a loop in the addressbook, we modify the address and
     * send an error back, but we still return 0.
     */
ok:
    if(loop && error != NULL)
      *error = cpystr("Loop or Duplicate detected in addressbook!");


    if(full_to){
	*full_to = (char *)fs_get((size_t)est_size(a));
	*full_to[0] = '\0';
	/*
	 * Assume that quotes surrounding the whole personal name are
	 * not meant to be literal quotes.  That is, the name
	 * "Joe College, PhD." is quoted so that we won't do the
	 * switcheroo of Last, First, not so that the quotes will be
	 * literal.  Rfc822_write_address will put the quotes back if they
	 * are needed, so Joe College would end up looking like
	 * "Joe College, PhD." <joe@somewhere.edu> but not like
	 * "\"Joe College, PhD.\"" <joe@somewhere.edu>.
	 */
	strip_personal_quotes(a);
	rfc822_write_address(*full_to, a);
    }

    if(no_repo && (did_set & FCC_NOREPO || did_set & LCC_NOREPO))
      *no_repo = 1;

    /*
     * The condition in the leading if means that addressbook fcc's
     * override the fcc-rule (because did_set will be set).
     */
    if(fcc && !(did_set & FCC_SET)){
	char *fcc_got = NULL;

	if((ps_global->fcc_rule == FCC_RULE_LAST
	    || ps_global->fcc_rule == FCC_RULE_CURRENT)
	   && strcmp(fcc_got = get_fcc(NULL), ps_global->VAR_DEFAULT_FCC)){
	    if(*fcc)
	      fs_give((void **)fcc);
	    
	    *fcc = cpystr(fcc_got);
	}
	else if(a && a->host){ /* not group syntax */
	    if(*fcc)
	      fs_give((void **)fcc);

	    if(!tmp)
	      tmp = (char *)fs_get((size_t)200);

	    if((ps_global->fcc_rule == FCC_RULE_RECIP ||
		ps_global->fcc_rule == FCC_RULE_NICK_RECIP) &&
		  get_uname(a ? a->mailbox : NULL, tmp, 200))
	      *fcc = cpystr(tmp);
	    else
	      *fcc = cpystr(ps_global->VAR_DEFAULT_FCC);
	}
	else{ /* first addr is group syntax */
	    if(!*fcc)
	      *fcc = cpystr(ps_global->VAR_DEFAULT_FCC);
	    /* else, leave it alone */
	}

	if(fcc_got)
	  fs_give((void **)&fcc_got);
    }

    if(a)
      mail_free_address(&a);

    if(tmp)
      fs_give((void **)&tmp);

    return 0;
}


/*
 * Expand an address string against the address books, local names, and domain.
 *
 * Args:  to      -- this is either an address string to parse (one or more
 *		      address strings separated by commas) or it is an
 *		      AdrBk_Entry, in which case it refers to a single addrbook
 *		      entry.  If it is an abe, then it is treated the same as
 *		      if the nickname of this entry was passed in and we
 *		      looked it up in the addrbook, except that it doesn't
 *		      actually have to have a non-null nickname.
 *     userdomain -- domain the user is in
 *    localdomain -- domain of the password file (usually same as userdomain)
 *  loop_detected -- pointer to an int we set if we detect a loop in the
 *		     address books (or a duplicate in a list)
 *       fcc      -- Returned value of fcc for first addr in a_string
 *       did_set  -- expand_address set the fcc (need this in case somebody
 *                     sets fcc explicitly to a value equal to default-fcc)
 *  simple_verify -- don't add list full names or expand 2nd level lists
 *
 * Result: An adrlist of expanded addresses is returned
 *
 * If the localdomain is NULL, then no lookup against the password file will
 * be done.
 */
ADDRESS *
expand_address(to, userdomain, localdomain, loop_detected, fcc, did_set,
    lcc, error, recursing, simple_verify)
    BuildTo  to;
    char    *userdomain,
	    *localdomain;
    int     *loop_detected;
    char   **fcc;
    int     *did_set;
    char   **lcc;
    char   **error;
    int      recursing, simple_verify;
{
    size_t       domain_length, length;
    ADDRESS     *adr, *a, *a_tail, *adr2, *a2, *a_temp;
    AdrBk_Entry *abe, *abe2;
    char        *list, *l1, **l2;
    char        *tmp_a_string, *q, *dummy;
    BuildTo      bldto;
    static char *fakedomain;

    dprint(9, (debugfile, "- expand_address -  (%s)\n",
	(to.type == Str) ? (to.arg.str ? to.arg.str : "nul")
			 : (to.arg.abe->nickname ? to.arg.abe->nickname
						: "no nick")));

    /*
     * We use the domain "@" to detect an unqualified address.  If it comes
     * back from rfc822_parse_adrlist with the host part set to "@", then
     * we know it must have been unqualified (so we should look it up in the
     * addressbook).  Later, we also use a c-client hack.  If an ADDRESS has
     * a host part that begins with @ then rfc822_write_address()
     * will write only the local part and leave off the @domain part.
     *
     * We also malloc enough space here so that we can strcpy over host below.
     */
    if(!recursing){
	domain_length = max(localdomain!=NULL ? strlen(localdomain) : (size_t)0,
			    userdomain!=NULL ? strlen(userdomain) : (size_t)0);
	fakedomain = (char *)fs_get(domain_length + 1);
	memset((void *)fakedomain, '@', domain_length);
	fakedomain[domain_length] = '\0';
    }

    adr = NULL;

    if(to.type == Str){
	/* rfc822_parse_adrlist feels free to destroy input so send copy */
	tmp_a_string = cpystr(to.arg.str);
	/* remove trailing comma */
	for(q = tmp_a_string + strlen(tmp_a_string) - 1;
	    q >= tmp_a_string && (*q == SPACE || *q == ',');
	    q--)
	  *q = '\0';

	rfc822_parse_adrlist(&adr, tmp_a_string, fakedomain);
	fs_give((void **)&tmp_a_string);
    }
    else{
	/* if we've already looked it up, fake an adr */
	adr = mail_newaddr();
	adr->mailbox = cpystr(to.arg.abe->nickname);
	adr->host    = cpystr(fakedomain);
    }

    for(a = adr, a_tail = adr; a;){

	/* start or end of c-client group syntax */
	if(!a->host){
            a_tail = a;
            a      = a->next;
	    continue;
	}
        else if(a->host[0] != '@'){
            /* Already fully qualified hostname */
            a_tail = a;
            a      = a->next;
        }
        else{
            /*
             * Hostname is "@" indicating name wasn't qualified.
             * Need to look up in address book, and the password file.
             * If no match then fill in the local domain for host.
             */
	    if(to.type == Str)
              abe = adrbk_lookup_with_opens_by_nick(a->mailbox,
						    !recursing,
						    (int *)NULL, -1);
	    else
              abe = to.arg.abe;

            if(abe == NULL){
		/* normal case */
	        if(F_OFF(F_COMPOSE_REJECTS_UNQUAL, ps_global)){
		    if(localdomain != NULL && a->personal == NULL){
			/* lookup in passwd file for local full name */
			a->personal = local_name_lookup(a->mailbox); 

			strcpy(a->host, a->personal == NULL ? userdomain :
							      localdomain);
		    }
		    else
		      strcpy(a->host, userdomain);
		}
		else{
		    if(error != NULL && *error == (char *)NULL){
			char ebuf[200];

			sprintf(ebuf, "%s not in addressbook", a->mailbox);
			*error = cpystr(ebuf);
		    }
		}

                /*--- Move to next address in list -----*/
                a_tail = a;
                a = a->next;

            }
	    /* expand first list, but not others if simple_verify */
	    else if(abe->tag == List && simple_verify && recursing){
                /*--- Move to next address in list -----*/
                a_tail = a;
                a = a->next;
	    }
	    else{
                /*
                 * There was a match in the address book.  We have to do a lot
                 * here because the item from the address book might be a 
                 * distribution list.  Treat the string just like an address
                 * passed in to parse and recurse on it.  Then combine
                 * the personal names from address book.  Lastly splice
                 * result into address list being processed
                 */

		/* first addr in list and fcc needs to be filled in */
		if(a == adr && fcc && !(*did_set & FCC_SET)){
		    /*
		     * Easy case for fcc.  This is a nickname that has
		     * an fcc associated with it.
		     */
		    if(abe->fcc && abe->fcc[0]){
			if(*fcc)
			  fs_give((void **)fcc);

			if(!strcmp(abe->fcc, "\"\""))
			  *fcc = cpystr("");
			else
			  *fcc = cpystr(abe->fcc);

			/*
			 * After we expand the list, we no longer remember
			 * that it came from this address book entry, so
			 * we wouldn't be able to set the fcc again based
			 * on the result.  This tells our caller to remember
			 * that for us.
			 */
			*did_set |= (FCC_SET | FCC_NOREPO);
		    }
		    /*
		     * else if fcc-rule=fcc-by-nickname, use that
		     */
		    else if(abe->nickname && abe->nickname[0] &&
			    (ps_global->fcc_rule == FCC_RULE_NICK ||
			     ps_global->fcc_rule == FCC_RULE_NICK_RECIP)){
			if(*fcc)
			  fs_give((void **)fcc);

			*fcc = cpystr(abe->nickname);
			/*
			 * After we expand the list, we no longer remember
			 * that it came from this address book entry, so
			 * we wouldn't be able to set the fcc again based
			 * on the result.  This tells our caller to remember
			 * that for us.
			 */
			*did_set |= (FCC_SET | FCC_NOREPO);
		    }
		}

		/* lcc needs to be filled in */
		if(a == adr  &&
		    lcc      &&
		   (!*lcc || !**lcc)){
		    ADDRESS *atmp;
		    char    *tmp;

		    /* return fullname for To line */
		    if(abe->fullname && *abe->fullname){
			if(*lcc)
			  fs_give((void **)lcc);

			atmp = mail_newaddr();
			dummy = NULL;
			q = (char *)rfc1522_decode((unsigned char *)
			    (tmp_20k_buf+10000), abe->fullname, &dummy);
			if(dummy)
			  fs_give((void **)&dummy);

			atmp->mailbox = cpystr(q);
			tmp = (char *)fs_get((size_t)est_size(atmp));
			tmp[0] = '\0';
			/* write the phrase with quoting */
			rfc822_write_address(tmp, atmp);
			*lcc = (char *)fs_get((size_t)(strlen(tmp)+1+1));
			strcpy(*lcc, tmp);
			strcat(*lcc, ";");
			mail_free_address(&atmp);
			fs_give((void **)&tmp);
			*did_set |= (LCC_SET | LCC_NOREPO);
		    }
		}

                if(recursing && abe->referenced){
                     /*---- caught an address loop! ----*/
                    fs_give(((void **)&a->host));
		    (*loop_detected)++;
                    a->host = cpystr("***address-loop-in-addressbooks***");
                    continue;
                }

                abe->referenced++;   /* For address loop detection */
                if(abe->tag == List){
                    length = 0;
                    for(l2 = abe->addr.list; *l2; l2++)
                        length += (strlen(*l2) + 1);

                    list = (char *)fs_get(length + 1);
		    list[0] = '\0';
                    l1 = list;
                    for(l2 = abe->addr.list; *l2; l2++){
                        if(l1 != list)
                          *l1++ = ',';
                        strcpy(l1, *l2);
                        l1 += strlen(l1);
                    }

		    bldto.type    = Str;
		    bldto.arg.str = list;
                    adr2 = expand_address(bldto, userdomain, localdomain,
					  loop_detected, fcc, did_set,
					  lcc, error, 1, simple_verify);
                    fs_give((void **)&list);
                }
                else if(abe->tag == Single){
                    if(strucmp(abe->addr.addr, a->mailbox)){
			bldto.type    = Str;
			bldto.arg.str = abe->addr.addr;
                        adr2 = expand_address(bldto, userdomain,
					      localdomain, loop_detected,
					      fcc, did_set, lcc,
					      error, 1, simple_verify);
                    }
		    else{
                        /*
			 * A loop within plain single entry is ignored.
			 * Set up so later code thinks we expanded.
			 */
                        adr2          = mail_newaddr();
                        adr2->mailbox = cpystr(abe->addr.addr);
                        adr2->host    = cpystr(userdomain);
                        adr2->adl     = cpystr(a->adl);
                    }
                }

                abe->referenced--;  /* Janet Jackson <janet@dialix.oz.au> */
                if(adr2 == NULL){
                    /* expanded to nothing, hack out of list */
                    a_temp = a;
                    if(a == adr){
                        adr    = a->next;
                        a      = adr;
                        a_tail = adr;
                    }
		    else{
                        a_tail->next = a->next;
                        a            = a->next;
                    }

		    a_temp->next = NULL;  /* So free won't do whole list */
                    mail_free_address(&a_temp);
                    continue;
                }

                /*
                 * Personal names:  If the expanded address has a personal
                 * name and the address book entry is a list with a fullname,
		 * tack the full name from the address book on in front.
                 * This mainly occurs with a distribution list where the
                 * list has a full name, and the first person in the list also
                 * has a full name.
		 *
		 * This algorithm doesn't work very well if lists are
		 * included within lists, but it's not clear what would
		 * be better.
                 */
		if(abe->fullname && abe->fullname[0]){
		    if(adr2->personal && adr2->personal[0]){
			if(abe->tag == List){
			    /* combine list name and existing name */
			    char *name;

			    if(!simple_verify){
				name = (char *)fs_get(strlen(adr2->personal) +
						  strlen(abe->fullname) + 5);
				dummy = NULL;
				q = (char *)rfc1522_decode((unsigned char *)
				    (tmp_20k_buf+10000), abe->fullname, &dummy);
				if(dummy)
				  fs_give((void **)&dummy);

				sprintf(name, "%s -- %s", q, adr2->personal);
				fs_give((void **)&adr2->personal);
				adr2->personal = name;
			    }
			}
			else{
			    /* replace with nickname fullname */
			    fs_give((void **)&adr2->personal);
			    dummy = NULL;
			    q = (char *)rfc1522_decode((unsigned char *)
				(tmp_20k_buf+10000), abe->fullname, &dummy);
			    if(dummy)
			      fs_give((void **)&dummy);

			    adr2->personal =
				cpystr(adrbk_formatname(q));
			}
		    }
		    else{
			if(abe->tag != List || !simple_verify){
			    if(adr2->personal)
			      fs_give((void **)&adr2->personal);

			    dummy = NULL;
			    q = (char *)rfc1522_decode((unsigned char *)
				(tmp_20k_buf+10000), abe->fullname, &dummy);
			    if(dummy)
			      fs_give((void **)&dummy);

			    adr2->personal = cpystr(adrbk_formatname(q));
			}
		    }
		}

                /* splice new list into old list and remove replaced addr */
                for(a2 = adr2; a2->next != NULL; a2 = a2->next)
		  ;/* do nothing */

                a2->next = a->next;
                if(a == adr)
                  adr    = adr2;
		else
                  a_tail->next = adr2;

                /* advance to next item, and free replaced ADDRESS */
                a_tail       = a2;
                a_temp       = a;
                a            = a->next;
                a_temp->next = NULL;  /* So free won't do whole list */
                mail_free_address(&a_temp);
            }

        }

	if((a_tail == adr && fcc && !(*did_set & FCC_SET))
	   || !a_tail->personal)
	  /*
	   * This looks for the addressbook entry that matches the given
	   * address.  It looks through all the addressbooks
	   * looking for an exact match and then returns that entry.
	   */
	  abe2 = addr_to_abe(a_tail);
	else
	  abe2 = NULL;

	/*
	 * If there is no personal name yet but we found the address in
	 * an address book, then we take the fullname from that address
	 * book entry and use it.  One consequence of this is that if I
	 * have an address book entry with address hubert@cac.washington.edu
	 * and a fullname of Steve Hubert, then there is no way I can
	 * send mail to hubert@cac.washington.edu without having the
	 * personal name filled in for me.
	 */
	if(!a_tail->personal && abe2 && abe2->fullname && abe2->fullname[0]){
	    dummy = NULL;
	    q = (char *)rfc1522_decode((unsigned char *)(tmp_20k_buf+10000),
				       abe2->fullname, &dummy);
	    if(dummy)
	      fs_give((void **)&dummy);

	    a_tail->personal = cpystr(adrbk_formatname(q));
	}

	/* if it's first addr in list and fcc hasn't been set yet */
	if(a_tail == adr && fcc && !(*did_set & FCC_SET)){
	    if(abe2 && abe2->fcc && abe2->fcc[0]){
		if(*fcc)
		  fs_give((void **)fcc);

		if(!strcmp(abe2->fcc, "\"\""))
		  *fcc = cpystr("");
		else
		  *fcc = cpystr(abe2->fcc);

		*did_set |= FCC_SET;
	    }
	    else if(abe2 && abe2->nickname && abe2->nickname[0] &&
		    (ps_global->fcc_rule == FCC_RULE_NICK ||
		     ps_global->fcc_rule == FCC_RULE_NICK_RECIP)){
		if(*fcc)
		  fs_give((void **)fcc);

		*fcc = cpystr(abe2->nickname);
		*did_set |= FCC_SET;
	    }
	}

	/*
	 * Lcc needs to be filled in.
	 * Bug: if ^T select was used to put the list in the lcc field, then
	 * the list will have been expanded already and the fullname for
	 * the list will be mixed with the initial fullname in the list,
	 * and we don't have anyway to tell them apart.  We could look for
	 * the --.  We could change expand_address so it doesn't combine
	 * those two addresses.
	 */
	if(adr &&
	    a_tail == adr  &&
	    lcc      &&
	   (!*lcc || !**lcc)){
	    if(adr->personal){
		ADDRESS *atmp;
		char    *tmp;

		if(*lcc)
		  fs_give((void **)lcc);

		atmp = mail_newaddr();
		atmp->mailbox = cpystr(adr->personal);
		tmp = (char *)fs_get((size_t)est_size(atmp));
		tmp[0] = '\0';
		/* write the phrase with quoting */
		rfc822_write_address(tmp, atmp);
		*lcc = (char *)fs_get((size_t)(strlen(tmp)+1+1));
		strcpy(*lcc, tmp);
		strcat(*lcc, ";");
		mail_free_address(&atmp);
		fs_give((void **)&tmp);
		*did_set |= (LCC_SET | LCC_NOREPO);
	    }
	}
    }

    if(!recursing)
      fs_give((void **)&fakedomain);

    return(adr);
}


/*
 * Run through the adrlist "adr" and strip off any enclosing quotes
 * around personal names.  That is, change "Joe L. Normal" to
 * Joe L. Normal.
 */
void
strip_personal_quotes(adr)
    ADDRESS *adr;
{
    int   len;
    register char *p, *q;

    while(adr){
	if(adr->personal){
	    len = strlen(adr->personal);
	    if(len > 1
	       && adr->personal[0] == '"'
	       && adr->personal[len-1] == '"'){
		adr->personal[len-1] = '\0';
		p = adr->personal;
		q = p + 1;
		while(*p++ = *q++)
		  ;
	    }
	}

	adr = adr->next;
    }
}


/*
 * Given an address, try to find the first nickname that goes with it.
 * Copies that nickname into the passed in buffer, which is assumed to
 * be at least MAX_NICKNAME+1 in length.  Returns NULL if it can't be found,
 * else it returns a pointer to the buffer.
 */
char *
get_nickname_from_addr(adr, buffer)
    ADDRESS *adr;
    char    *buffer;
{
    AdrBk_Entry *abe;
    char        *ret = NULL;
    SAVE_STATE_S state;
    jmp_buf	 save_jmp_buf;

    state.savep = NULL;
    state.stp = NULL;
    state.dlc_to_warp_to = NULL;

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	if(state.savep)
	  fs_give((void **)&(state.savep));
	if(state.stp)
	  fs_give((void **)&(state.stp));
	if(state.dlc_to_warp_to)
	  fs_give((void **)&(state.dlc_to_warp_to));

	q_status_message(SM_ORDER, 3, 5, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... get_nickname_from_addr()!\n"));
	addrbook_reset();
    }

    init_ab_if_needed();
    save_state(&state);

    abe = addr_to_abe(adr);

    if(abe && abe->nickname && abe->nickname[0]){
	strncpy(buffer, abe->nickname, MAX_NICKNAME);
	ret = buffer;
    }

    restore_state(&state);

    return(ret);
}


/*
 * Given an address, try to find the first fcc that goes with it.
 * Copies that fcc into the passed in buffer, which is assumed to
 * be at least MAXFOLDER+1 in length.  Returns NULL if it can't be found,
 * else it returns a pointer to the buffer.
 */
char *
get_fcc_from_addr(adr, buffer)
    ADDRESS *adr;
    char    *buffer;
{
    AdrBk_Entry *abe;
    char        *ret = NULL;
    SAVE_STATE_S state;
    jmp_buf	 save_jmp_buf;

    state.savep = NULL;
    state.stp = NULL;
    state.dlc_to_warp_to = NULL;

    memcpy(save_jmp_buf, addrbook_changed_unexpectedly, sizeof(jmp_buf));
    if(setjmp(addrbook_changed_unexpectedly)){
	if(state.savep)
	  fs_give((void **)&(state.savep));
	if(state.stp)
	  fs_give((void **)&(state.stp));
	if(state.dlc_to_warp_to)
	  fs_give((void **)&(state.dlc_to_warp_to));

	q_status_message(SM_ORDER, 3, 5, "Resetting address book...");
	dprint(1, (debugfile,
	    "RESETTING address book... get_fcc_from_addr()!\n"));
	addrbook_reset();
    }

    init_ab_if_needed();
    save_state(&state);

    abe = addr_to_abe(adr);

    if(abe && abe->fcc && abe->fcc[0]){
	if(!strcmp(abe->fcc, "\"\""))
	  buffer[0] = '\0';
	else
	  strncpy(buffer, abe->fcc, MAXFOLDER);

	ret = buffer;
    }

    restore_state(&state);

    return(ret);
}


static char *last_fcc_used;
/*
 * Returns alloc'd fcc.
 */
char *
get_fcc(fcc_arg)
    char *fcc_arg;
{
    char *fcc;

    /*
     * Use passed in arg unless it is the same as default (and then
     * may use that anyway below).
     */
    if(fcc_arg && strcmp(fcc_arg, ps_global->VAR_DEFAULT_FCC))
      fcc = cpystr(fcc_arg);
    else{
	if(ps_global->fcc_rule == FCC_RULE_LAST && last_fcc_used)
	  fcc = cpystr(last_fcc_used);
	else if(ps_global->fcc_rule == FCC_RULE_CURRENT
		&& ps_global->mail_stream
		&& !(ps_global->inbox_stream
		    && ps_global->inbox_stream == ps_global->mail_stream)
		&& !IS_NEWS(ps_global->mail_stream)
		&& ps_global->cur_folder
		&& ps_global->cur_folder[0]){
	    CONTEXT_S *cntxt = ps_global->context_current;
	    char *rs = NULL;

	    if(((cntxt->use) & CNTXT_SAVEDFLT))
	      rs = ps_global->cur_folder;
	    else
	      rs = ps_global->mail_stream->mailbox;

	    fcc = cpystr((rs&&*rs) ? rs : ps_global->VAR_DEFAULT_FCC);
	}
	else
	  fcc = cpystr(ps_global->VAR_DEFAULT_FCC);
    }
    
    return(fcc);
}


/*
 * Save the fcc for use with next composition.
 */
void
set_last_fcc(fcc)
    char *fcc;
{
    if(fcc){
	if(!last_fcc_used)
	  last_fcc_used = cpystr(fcc);
	else if(strcmp(last_fcc_used, fcc)){
	    if(strlen(last_fcc_used) >= strlen(fcc))
	      strcpy(last_fcc_used, fcc);
	    else{
		fs_give((void **)&last_fcc_used);
		last_fcc_used = cpystr(fcc);
	    }
	}
    }
}


/*
 * Figure out what the fcc is based on the given to address.
 *
 * Returns an allocated copy of the fcc, to be freed by the caller, or NULL.
 */
char *
get_fcc_based_on_to(to)
    ADDRESS *to;
{
    ADDRESS    *addr;
    char       *str, buf[MAX_ADDR_EXPN];
    BUILDER_ARG tmp_fcc;

    if(!to || !to->host || to->host[0] == '.')
      return(NULL);

    /* pick off first address */
    addr = copyaddr(to);
    str = cpystr(addr_string(addr, buf));
    mail_free_address(&addr);
    tmp_fcc.tptr = NULL;
    tmp_fcc.next = NULL;
    tmp_fcc.xtra = NULL;
    (void)build_address(str, NULL, NULL, &tmp_fcc);

    if(str)
      fs_give((void **)&str);
    
    return(tmp_fcc.tptr);
}


/*
 * Look through addrbooks for nickname, opening addrbooks first
 * if necessary.  It is assumed that the caller will restore the
 * state of the addrbooks if desired.
 *
 * Args: nickname       -- the nickname to lookup
 *       clearrefs      -- clear reference bits before lookup
 *       which_addrbook -- If matched, addrbook number it was found in.
 *       not_here       -- If non-negative, skip looking in this abook.
 *
 * Results: A pointer to an AdrBk_Entry is returned, or NULL if not found.
 * Stop at first match (so order of addrbooks is important).
 */
AdrBk_Entry *
adrbk_lookup_with_opens_by_nick(nickname, clearrefs, which_addrbook, not_here)
    char *nickname;
    int   clearrefs;
    int  *which_addrbook;
    int   not_here;
{
    AdrBk_Entry *abe = (AdrBk_Entry *)NULL;
    int i;
    PerAddrBook *pab;

    dprint(5, (debugfile, "- adrbk_lookup_with_opens_by_nick(%s) -\n",
	nickname));

    for(i = 0; i < as.n_addrbk; i++){

	if(i == not_here)
	  continue;

	pab = &as.adrbks[i];

	if(pab->ostatus != Open && pab->ostatus != NoDisplay)
	  init_abook(pab, NoDisplay);

	if(clearrefs)
	  adrbk_clearrefs(pab->address_book);

	abe = adrbk_lookup_by_nick(pab->address_book,
		    nickname,
		    (adrbk_cntr_t *)NULL);
	if(abe)
	  break;
    }

    if(abe && which_addrbook)
      *which_addrbook = i;

    return(abe);
}


/*
 * Find the addressbook entry that matches the argument address.
 * Searches through all addressbooks looking for the match.
 * Opens addressbooks if necessary.  It is assumed that the caller
 * will restore the state of the addrbooks if desired.
 *
 * Args: addr -- the address we're trying to match
 *
 * Returns:  NULL -- no match found
 *           abe  -- a pointer to the addrbook entry that matches
 */
AdrBk_Entry *
addr_to_abe(addr)
    ADDRESS *addr;
{
    register PerAddrBook *pab;
    int adrbk_number;
    AdrBk_Entry *abe = NULL;
    char *abuf = NULL;

    if(!(addr && addr->mailbox))
      return (AdrBk_Entry *)NULL;

    abuf = (char *)fs_get((size_t)(MAX_ADDR_FIELD + 1));

    strncpy(abuf, addr->mailbox, MAX_ADDR_FIELD);
    if(addr->host && addr->host[0])
      strncat(strncat(abuf, "@", MAX_ADDR_FIELD), addr->host, MAX_ADDR_FIELD);

    /* for each addressbook */
    for(adrbk_number = 0; adrbk_number < as.n_addrbk; adrbk_number++){

	pab = &as.adrbks[adrbk_number];

	if(pab->ostatus != Open && pab->ostatus != NoDisplay)
	  init_abook(pab, NoDisplay);

	abe = adrbk_lookup_by_addr(pab->address_book,
				   abuf,
				   (adrbk_cntr_t *)NULL);
	if(abe)
	  break;
    }

    if(abuf)
      fs_give((void **)&abuf);

    return(abe);
}


/*
 * Turn a list of address structures into a formatted string
 *
 * Args: adrlist -- An adrlist
 *       f       -- Function to use to print one address in list.  If NULL,
 *                  use rfc822_write_address_decode to print whole list.
 *       verbose -- Include [charset] string if charset doesn't match ours.
 * Result:  comma separated list of addresses which is
 *                                     malloced here and returned
 *		(the list is rfc1522 decoded unless f is *not* NULL)
 */
char *
addr_list_string(adrlist, f, verbose)
    ADDRESS *adrlist;
    char    *(*f) PROTO((ADDRESS *, char *));
    int      verbose;
{
    int               len;
    char             *list, *s, string[MAX_ADDR_EXPN+1];
    register ADDRESS *a;

    if(!adrlist)
      return(cpystr(""));
    
    if(f){
	len = 0;
	for(a = adrlist; a; a = a->next)
	  len += (strlen((*f)(a, string)) + 2);

	list = (char *)fs_get((size_t)len);
	s    = list;
	s[0] = '\0';

	for(a = adrlist; a; a = a->next){
	    sstrcpy(&s, (*f)(a, string));
	    if(a->next){
		*s++ = ',';
		*s++ = SPACE;
	    }
	}
    }
    else{
	char *charset = NULL;

	list = (char *)fs_get((size_t)est_size(adrlist));
	list[0] = '\0';
	rfc822_write_address_decode(list, adrlist, verbose ? NULL : &charset);
	if(charset)
	  fs_give((void **)&charset);
    }

    return(list);
}


/*
 * Turn an AdrBk_Entry into a nickname (if it has a nickname) or a
 * formatted addr_string which has been rfc1522 decoded.
 *
 * Args: abe      -- the AdrBk_Entry
 *       dl       -- the corresponding dl
 *       abook    -- which addrbook the abe is in (only used for type ListEnt)
 *
 * Result:  allocated string is returned
 */
char *
abe_to_nick_or_addr_string(abe, dl, abook)
    AdrBk_Entry   *abe;
    AddrScrn_Disp *dl;
    AdrBk         *abook;
{
    ADDRESS       *addr;
    char          *a_string;

    if(!dl || !abe)
      return(cpystr(""));

    if((dl->type == Simple || dl->type == ListHead)
       && abe->nickname && abe->nickname[0])
      return(cpystr(abe->nickname));

    addr = abe_to_address(abe, dl, abook, NULL);
    a_string = addr_list_string(addr, NULL, 0); /* always produces a string */
    if(addr)
      mail_free_address(&addr);
    
    return(a_string);
}


/*
 * Turn an AdrBk_Entry into an address list
 *
 * Args: abe      -- the AdrBk_Entry
 *       dl       -- the corresponding dl
 *       abook    -- which addrbook the abe is in (only used for type ListEnt)
 *       how_many -- The number of addresses is returned here
 *
 * Result:  allocated address list or NULL
 */
ADDRESS *
abe_to_address(abe, dl, abook, how_many)
    AdrBk_Entry   *abe;
    AddrScrn_Disp *dl;
    AdrBk         *abook;
    int           *how_many;
{
    char          *fullname, *tmp_a_string;
    char          *list, *l1, **l2;
    char          *fakedomain = "@";
    ADDRESS       *addr = NULL;
    size_t         length;
    int            count = 0;

    if(!dl || !abe)
      return(NULL);

    fullname = (abe->fullname && abe->fullname[0]) ? abe->fullname : NULL;

    switch(dl->type){
      case Simple:
	/* rfc822_parse_adrlist feels free to destroy input so send copy */
	tmp_a_string = cpystr(abe->addr.addr);
	rfc822_parse_adrlist(&addr, tmp_a_string, fakedomain);

	if(tmp_a_string)
	  fs_give((void **)&tmp_a_string);

	if(fullname){
	    if(addr->personal)
	      fs_give((void **)&addr->personal);

	    addr->personal = cpystr(adrbk_formatname(fullname));
	}

	count++;
        break;

      case ListEnt:
	/* rfc822_parse_adrlist feels free to destroy input so send copy */
	tmp_a_string = cpystr(listmem_from_dl(abook, dl));
	rfc822_parse_adrlist(&addr, tmp_a_string, fakedomain);
	if(tmp_a_string)
	  fs_give((void **)&tmp_a_string);

	count++;
        break;

      case ListHead:
	length = 0;
	for(l2 = abe->addr.list; *l2; l2++)
	  length += (strlen(*l2) + 1);

	list = (char *)fs_get(length + 1);
	l1 = list;
	for(l2 = abe->addr.list; *l2; l2++){
	    if(l1 != list)
	      *l1++ = ',';
	    strcpy(l1, *l2);
	    l1 += strlen(l1);
	    count++;
	}

	rfc822_parse_adrlist(&addr, list, fakedomain);
	if(list)
	  fs_give((void **)&list);

        break;
      
      default:
	dprint(2,
	   (debugfile, "default case in abe_to_address, shouldn't happen\n"));
	break;
    } 

    if(how_many)
      *how_many = count;

    return(addr);
}


/* Write RFC822 address with 1522 decoding of personal name
 * Accepts: pointer to destination string
 *	    address to interpret
 * (This is a copy of c-client rfc822_write_address except for the decoding.)
 */

void
rfc822_write_address_decode (dest,adr,charset)
    char    *dest;
    ADDRESS *adr;
    char   **charset;
{
  extern const char *rspecials;

  while (adr) {
				/* start of group? */
    if (adr->mailbox && !adr->host) {
				/* yes, write group name */
	  rfc822_cat(dest,
		     (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					    adr->mailbox, charset),
		     rspecials);
      strcat (dest,": ");	/* write group identifier */
      adr = adr->next;		/* move to next address block */
    }
    else {			/* end of group? */
      if (!adr->host) strcat (dest,";");
				/* simple case? */
      else if (!(adr->personal || adr->adl)) rfc822_address (dest,adr);
      else {			/* no, must use phrase <route-addr> form */
	if (adr->personal) {	/* in case have adl but no personal name */
	  rfc822_cat(dest,
		     (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					    adr->personal, charset),
		     rspecials);
	  strcat (dest," ");
	}
	strcat (dest,"<");	/* write address delimiter */
	rfc822_address (dest,adr);/* write address */
	strcat (dest,">");	/* closing delimiter */
      }
				/* delimit if there is one */
      if ((adr = adr->next) && adr->mailbox) strcat (dest,", ");
    }
  }
}


/*
 * Format an address structure into a string
 *
 * Args: addr -- Single ADDRESS structure to turn into a string
 *
 * Result:  Fills in buf and returns pointer to it.
 * Just uses the c-client call to do this.
 *		(the address is not rfc1522 decoded)
 */
char *
addr_string(addr, buf)
    ADDRESS *addr;
    char    *buf;
{
    ADDRESS tmp_addr;

    *buf = '\0';
    tmp_addr = *addr;
    tmp_addr.next = NULL;
    rfc822_write_address(buf, &tmp_addr);
    return(buf);
}


/*
 * Format an address structure into a simple string: "mailbox@host"
 *
 * Args: addr -- Single ADDRESS structure to turn into a string
 *	 buf -- buffer to write address in;
 *
 * Result:  Returns pointer to internal static formatted string.
 * Just uses the c-client call to do this.
 */
char *
simple_addr_string(addr, buf)
    ADDRESS *addr;
    char    *buf;
{
    *buf = '\0';
    rfc822_address(buf, addr);
    return(buf);
}


/*
 * Check to see if address is that of the current user running pine
 *
 * Args: a  -- Address to check
 *       ps -- The pine_state structure
 *
 * Result: returns 1 if it matches, 0 otherwise.
 *
 * The mailbox must match the user name and the hostname must match.
 * In matching the hostname, matches occur if the hostname in the address
 * is blank, or if it matches the local hostname, or the full hostname
 * with qualifying domain, or the qualifying domain without a specific host.
 * Note, there is a very small chance that we will err on the
 * non-conservative side here.  That is, we may decide two addresses are
 * the same even though they are different (because we do case-insensitive
 * compares on the mailbox).  That might cause a reply not to be sent to
 * somebody because they look like they are us.  This should be very,
 * very rare.
 *
 * It is also considered a match if any of the addresses in alt-addresses
 * matches a.  The check there is simpler.  It parses each address in
 * the list, adding maildomain if there wasn't a domain, and compares
 * mailbox and host in the ADDRESS's for equality.
 */
int
address_is_us(a, ps)
    ADDRESS     *a;
    struct pine *ps;
{
    char **t;
    int ret;

    if(!a || a->mailbox == NULL)
      ret = 0;

    /* at least LHS must match, but case-independent */
    else if(strucmp(a->mailbox, ps->VAR_USER_ID) == 0

                      && /* and hostname matches */

    /* hostname matches if it's not there, */
    (a->host == NULL ||
       /* or if hostname and userdomain (the one user sets) match exactly, */
      ((ps->userdomain && a->host && strucmp(a->host,ps->userdomain) == 0) ||

              /*
               * or if(userdomain is either not set or it is set to be
               * the same as the localdomain or hostname) and (the hostname
               * of the address matches either localdomain or hostname)
               */
             ((ps->userdomain == NULL ||
               strucmp(ps->userdomain, ps->localdomain) == 0 ||
               strucmp(ps->userdomain, ps->hostname) == 0) &&
              (strucmp(a->host, ps->hostname) == 0 ||
               strucmp(a->host, ps->localdomain) == 0)))))
      ret = 1;

    /*
     * If no match yet, check to see if it matches any of the alternate
     * addresses the user has specified.
     */
    else if(!ps_global->VAR_ALT_ADDRS ||
       !ps_global->VAR_ALT_ADDRS[0] ||
       !ps_global->VAR_ALT_ADDRS[0][0])
	ret = 0;  /* none defined */

    else{
	ret = 0;
	for(t = ps_global->VAR_ALT_ADDRS; *t != NULL; t++){
	    ADDRESS *alt_addr;
	    char    *alt;

	    alt  = cpystr(*t);
	    alt_addr = NULL;
	    rfc822_parse_adrlist(&alt_addr, alt, ps_global->maildomain);
	    if(alt)
	      fs_give((void **)&alt);

	    if(address_is_same(a, alt_addr)){
		if(alt_addr)
		  mail_free_address(&alt_addr);

		ret = 1;
		break;
	    }

	    if(alt_addr)
	      mail_free_address(&alt_addr);
	}
    }

    return(ret);
}


/*
 * Compare the two addresses, and return true if they're the same,
 * false otherwise
 *
 * Args: a -- First address for comparison
 *       b -- Second address for comparison
 *
 * Result: returns 1 if it matches, 0 otherwise.
 */
int
address_is_same(a, b)
    ADDRESS *a, *b;
{
    return(a && b && a->mailbox && b->mailbox && a->host && b->host
	   && strucmp(a->mailbox, b->mailbox) == 0
           && strucmp(a->host, b->host) == 0);
}


/*
 * Compute an upper bound on the size of the array required by
 * rfc822_write_address for this list of addresses.
 *
 * Args: adrlist -- The address list.
 *
 * Returns -- an integer giving the upper bound
 */
int
est_size(adrlist)
    ADDRESS *adrlist;
{
    ADDRESS	  *b;
    register int   cnt = 0;
    register char *p;

    for(b = adrlist; b; b = b->next){
	if(b->personal){
	    extern char *tspecials;		/* from c-client rfc822.c */

	    cnt += strlen(b->personal);
	    /* Look for any chars special to RFC822 whose quoting
	     * will make the address longer when turned into a string.
	     */
	    if(strpbrk(b->personal, tspecials)){
		cnt += 2;
		for(p=strpbrk(b->personal,"\\\""); p; p=strpbrk(++p,"\\\""))
		  cnt += 2;
	    }
	}

	cnt   += (b->mailbox  ? strlen(b->mailbox)  : 0);
	cnt   += (b->adl      ? strlen(b->adl)      : 0);
	cnt   += (b->host     ? strlen(b->host)     : 0);
	/*
	 * add room for:
         *   possible single space between fullname and addr
         *   left and right brackets
         *   @ sign
         *   possible : for route addr
         *   , <space>
	 *
	 * So I really think that adding 7 is enough.  Instead, I'll add 10.
	 */
	cnt   += 10;
    }

    return(max(cnt, 200));  /* just making sure */
}


/*
 * Interact with user to figure out which address book they want to add a
 * new entry (TakeAddr) to.
 *
 * Args: command_line -- just the line to prompt on
 *
 * Results: returns a pab pointing to the selected addrbook, or NULL.
 */
PerAddrBook *
use_this_addrbook(command_line)
    int command_line;
{
    HelpType   help;
    int        rc = 0;
    PerAddrBook  *pab, *the_only_pab;
#define MAX_ABOOK 1000
    int        i, abook_num, count_read_write;
    char       addrbook[MAX_ABOOK + 1],
               prompt[MAX_ABOOK + 81];
    static ESCKEY_S ekey[] = {
	{-2,   0,   NULL, NULL},
	{ctrl('P'), 10, "^P", "Prev AddrBook"},
	{ctrl('N'), 11, "^N", "Next AddrBook"},
	{KEY_UP,    10, "", ""},
	{KEY_DOWN,  11, "", ""},
	{-1, 0, NULL, NULL}};

    dprint(8, (debugfile, "\n - use_this_addrbook -\n"));

    /* check for only one ReadWrite addrbook */
    count_read_write = 0;
    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	/*
	 * NoExists is counted, too, so the user can add to an empty
	 * addrbook the first time.
	 */
	if(pab->access == ReadWrite || pab->access == NoExists){
	    count_read_write++;
	    the_only_pab = &as.adrbks[i];
	}
    }

    /* only one usable addrbook, use it */
    if(count_read_write == 1)
      return(the_only_pab);

    /* no addrbook to write to */
    if(count_read_write == 0){
	q_status_message1(SM_ORDER | SM_DING, 3, 4,
			  "No %sAddressbook to Take to!",
			  (as.n_addrbk > 0) ? "writable " : "");
	return NULL;
    }

    /* start with the first addrbook */
    abook_num = 0;
    pab       = &as.adrbks[abook_num];
    strncpy(addrbook, pab->nickname, MAX_ABOOK);
    addrbook[MAX_ABOOK] = '\0';
    sprintf(prompt, "Take to which addrbook : %s",
	(pab->access == ReadOnly || pab->access == NoAccess) ?
	    "[ReadOnly] " : "");
    help = NO_HELP;
    ps_global->mangled_footer = 1;
    do{
	if(!pab)
          q_status_message1(SM_ORDER, 3, 4, "No addressbook \"%s\"", addrbook);

	if(rc == 3)
          help = (help == NO_HELP ? h_oe_chooseabook : NO_HELP);

	rc = optionally_enter(addrbook, command_line, 0, MAX_ABOOK, 1,
                                  0, prompt, ekey, help, 0);

	if(rc == 1) /* ^C */
	  break;

	if(rc == 10){			/* Previous addrbook */
	    if(--abook_num < 0)
	      abook_num = as.n_addrbk - 1;

	    pab = &as.adrbks[abook_num];
	    strncpy(addrbook, pab->nickname, MAX_ABOOK);
	    sprintf(prompt, "Take to which addrbook : %s",
		(pab->access == ReadOnly || pab->access == NoAccess) ?
		    "[ReadOnly] " : "");
	}
	else if(rc == 11){			/* Next addrbook */
	    if(++abook_num > as.n_addrbk - 1)
	      abook_num = 0;

	    pab = &as.adrbks[abook_num];
	    strncpy(addrbook, pab->nickname, MAX_ABOOK);
	    sprintf(prompt, "Take to which addrbook : %s",
		(pab->access == ReadOnly || pab->access == NoAccess) ?
		    "[ReadOnly] " : "");
	}

    }while(rc == 2 || rc == 3 || rc == 4 || rc == 10 || rc == 11 || rc == 12 ||
           !(pab = check_for_addrbook(addrbook)));
            
    if(rc != 0)
      return NULL;

    return(pab);
}


/*
 * Return a pab pointer to the addrbook which corresponds to the argument.
 * 
 * Args: addrbook -- the string representing the addrbook.
 *
 * Results: returns a PerAddrBook pointer for the referenced addrbook, NULL
 *          if none.  First the nicknames are checked and then the filenames.
 *          This must be one of the existing addrbooks.
 */
PerAddrBook *
check_for_addrbook(addrbook)
    char *addrbook;
{
    register int i;
    register PerAddrBook *pab;

    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	if(strucmp(pab->nickname, addrbook) == 0)
	  break;
    }

    if(i < as.n_addrbk)
      return(pab);

    for(i = 0; i < as.n_addrbk; i++){
	pab = &as.adrbks[i];
	if(strucmp(pab->filename, addrbook) == 0)
	  break;
    }

    if(i < as.n_addrbk)
      return(pab);
    
    return NULL;
}


static struct key takeaddr_keys_listmode[] = 
       {{"?","Help",KS_SCREENHELP},	{"W","WhereIs",KS_WHEREIS},
	{"E","ExitTake",KS_EXITMODE},	{"T","Take",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next", KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"X","[Set/Unset]",KS_NONE},	{"A","SetAll",KS_NONE},
	{"U","UnSetAll",KS_NONE},	{"S","SinglMode",KS_NONE}};
INST_KEY_MENU(takeaddr_keymenu_listmode, takeaddr_keys_listmode);

static struct key takeaddr_keys_singlemode[] = 
       {{"?","Help",KS_SCREENHELP},	{"W","WhereIs",KS_WHEREIS},
	{"E","ExitTake",KS_EXITMODE},	{"T","[Take]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next", KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{"L","ListMode",KS_NONE}};
INST_KEY_MENU(takeaddr_keymenu_singlemode, takeaddr_keys_singlemode);


/*
 * Screen for selecting which addresses to Take to address book.
 *
 * Args:      ps -- Pine state
 *       ta_list -- Screen is formed from this list of addresses
 *  how_many_selected -- how many checked initially in ListMode
 *          mode -- which mode to start in
 *           fcc -- use this fcc if no other available
 *       comment -- use this comment if no other available
 *
 * Result: an address book may be updated
 */
void
takeaddr_screen(ps, ta_list, how_many_selected, mode)
    struct pine       *ps;
    TA_S              *ta_list;
    int                how_many_selected;
    TakeAddrScreenMode mode;
{
    int		  orig_ch, dline, give_warn_message, command_line;
    int           ch = 'x',
                  km_popped = 0,
                  directly_to_take = 0,
                  done = 0;
    TA_S	 *current = NULL,
		 *ctmp = NULL;
    TA_SCREEN_S   screen;
    Pos           cursor_pos;

    command_line = -FOOTER_ROWS(ps);  /* third line from the bottom */

    screen.current = screen.top_line = NULL;
    screen.mode    = mode;

    if(ta_list == NULL){
	q_status_message(SM_INFO, 0, 2, "No addresses to take, cancelled");
	return;
    }

    current	       = first_sel_taline(ta_list);
    ps->mangled_screen = 1;
    ta_screen	       = &screen;

    if(is_talist_of_one(current)){
	directly_to_take++;
	screen.mode = SingleMode; 
    }
    else if(screen.mode == ListMode)
      q_status_message(SM_INFO, 0, 1,
	    "List mode: Use \"X\" to mark addresses to be included in list");
    else
      q_status_message(SM_INFO, 0, 1,
	    "Single mode: Use \"P\" or \"N\" to select desired address");

    while(!done){
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps);
		ps->mangled_body = 1;
	    }
	}

	if(screen.mode == ListMode)
	  ps->redrawer = takeaddr_screen_redrawer_list;
	else
	  ps->redrawer = takeaddr_screen_redrawer_single;

	if(ps->mangled_screen){
	    ps->mangled_header = 1;
	    ps->mangled_footer = 1;
	    ps->mangled_body   = 1;
	    ps->mangled_screen = 0;
	}

	/*----------- Check for new mail -----------*/
	if(new_mail(0, NM_TIMING(ch), 1) >= 0)
	  ps->mangled_header = 1;

#ifdef _WINDOWS
	mswin_beginupdate();
#endif
	if(ps->mangled_header){
	    char tbuf[40];

	    sprintf(tbuf, "TAKE ADDRESS SCREEN (%s Mode)",
				    (screen.mode == ListMode) ? "List"
							      : "Single");
            set_titlebar(tbuf, ps->mail_stream, ps->context_current,
                             ps->cur_folder, ps->msgmap, 1, FolderName, 0, 0);
	    ps->mangled_header = 0;
	}

	dline = update_takeaddr_screen(ps, current, &screen, &cursor_pos);
	if(F_OFF(F_SHOW_CURSOR, ps)){
	    cursor_pos.row = ps->ttyo->screen_rows - FOOTER_ROWS(ps);
	    cursor_pos.col = 0;
	}

	/*---- This displays new mail notification, or errors ---*/
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 3;
	    mark_status_unknown();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(ps_global) = 1;
	    mark_status_unknown();
	}

	/*---- Redraw footer ----*/
	if(ps->mangled_footer){
	    bitmap_t bitmap;

	    if(km_popped){
		FOOTER_ROWS(ps) = 3;
		clearfooter(ps);
	    }

	    setbitmap(bitmap);
	    ps->mangled_footer = 0;
	    if(screen.mode == ListMode)
	      draw_keymenu(&takeaddr_keymenu_listmode, bitmap,
		    ps->ttyo->screen_cols, 1-FOOTER_ROWS(ps_global),
		    0, FirstMenu, 0);
	    else
	      draw_keymenu(&takeaddr_keymenu_singlemode, bitmap,
		    ps->ttyo->screen_cols, 1-FOOTER_ROWS(ps_global),
		    0, FirstMenu, 0);
	    if(km_popped){
		FOOTER_ROWS(ps) = 1;
		mark_keymenu_dirty();
	    }
	}

#ifdef _WINDOWS
	mswin_endupdate();
#endif
        /*------ Read the command from the keyboard ----*/
	MoveCursor(cursor_pos.row, cursor_pos.col);

#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0);
	register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
		       ps_global->ttyo->screen_rows - (FOOTER_ROWS(ps) + 1),
		       ps_global->ttyo->screen_cols);
#endif

	if(directly_to_take)  /* bypass this screen */
	  ch = orig_ch = 't';
	else
	  ch = orig_ch = read_command();

#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif

        if(ch <= 0xff && isupper((unsigned char)ch))
          ch = tolower((unsigned char)ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps);
	      break;
	  }

	switch(ch){
	  case '?':				/* help! */
	  case PF1:
	  case ctrl('G'):
	    if(FOOTER_ROWS(ps_global) == 1 && km_popped == 0){
		km_popped = 2;
		ps_global->mangled_footer = 1;
		break;
	    }

	    helper(h_takeaddr_screen, "HELP FOR TAKE ADDRESS SCREEN", 1);
	    ps->mangled_screen = 1;
	    break;

	  case 'e':				/* exit takeaddr screen */
	  case PF3:
	  case ctrl('C'):
	    cancel_warning(NO_DING, "addition");
	    done++;
	    break;

	  case 't':  /* take */
	  case PF4:
	  case ctrl('M'):
	  case ctrl('J'):
	  if((ch == ctrl('M') || ch == ctrl('J'))
	     && screen.mode == ListMode)
	    goto SelectCase;  /* default is different in this case */

	  TakeAddrCase:
	    if(screen.mode == ListMode)
	      done = ta_take_marked_addrs(how_many_selected,
		       first_sel_taline(current), command_line);
	    else
	      done = ta_take_single_addr(current, command_line);

	    if(!done)
	      directly_to_take = 0;

	    break;

	  case 'n':				/* next list element */
	  case PF6:
	  case '\t':
	  case ctrl('N'):
	  case KEY_DOWN:
	  case KEY_SCRLDNL:
	    if(ctmp = next_sel_taline(current))
	      current = ctmp;
	    else
	      q_status_message(SM_INFO, 0, 1, "Already on last line.");

	    break;

	  case 'p':				/* previous list element */
	  case PF5:
	  case ctrl('P'):			/* up arrow */
	  case KEY_UP:
	  case KEY_SCRLUPL:
	    if(ctmp = pre_sel_taline(current))
	      current = ctmp;
	    else
	      q_status_message(SM_INFO, 0, 1, "Already on first line.");

	    break;

	  case '+':				/* page forward */
	  case SPACE:
	  case PF8:
	  case ctrl('V'):
	  case KEY_PGDN:
	    give_warn_message = 1;
	    while(dline++ < ps->ttyo->screen_rows - FOOTER_ROWS(ps)){
	        if(ctmp = next_sel_taline(current)){
		    current = ctmp;
		    give_warn_message = 0;
		}
	        else
		    break;
	    }

	    if(give_warn_message)
	      q_status_message(SM_INFO, 0, 1, "Already on last page.");

	    break;

	  case '-':				/* page backward */
	  case PF7:
	  case ctrl('Y'):
	  case KEY_PGUP:
	    /* move to top of screen */
	    give_warn_message = 1;
	    while(dline-- > HEADER_ROWS(ps_global)){
	        if(ctmp = pre_sel_taline(current)){
		    current = ctmp;
		    give_warn_message = 0;
		}
	        else
		    break;
	    }

	    /* page back one screenful */
	    while(++dline < ps->ttyo->screen_rows - FOOTER_ROWS(ps)){
	        if(ctmp = pre_sel_taline(current)){
		    current = ctmp;
		    give_warn_message = 0;
		}
	        else
		    break;
	    }

	    if(give_warn_message)
	      q_status_message(SM_INFO, 0, 1, "Already on first page.");

	    break;

	  case 'w':				/* whereis */
	  case PF2:
	  case ctrl('W'):
	    if(ctmp = whereis_taline(current))
	      current = ctmp;
	    
	    ps->mangled_footer = 1;
	    break;

	  case KEY_SCRLTO:
	    /* no op for now */
	    break;

#ifdef MOUSE	    
	  case KEY_MOUSE:
	    {
		MOUSEPRESS mp;

		mouse_get_last(NULL, &mp);
		mp.row -= HEADER_ROWS(ps_global);
		ctmp = screen.top_line;
		if(mp.doubleclick){
		    if(screen.mode == SingleMode) 
		      goto TakeAddrCase;
		    else
		      goto SelectCase;
		}
		else{
		    while(mp.row && ctmp != NULL){
			--mp.row;
			do  ctmp = ctmp->next;
			while(ctmp != NULL && ctmp->skip_it && !ctmp->print);
		    }

		    if(ctmp != NULL && !ctmp->skip_it)
		      current = ctmp;
		}
	    }
	    break;
#endif

	  case ctrl('L'):
          case KEY_RESIZE:
	    ClearScreen();
	    ps->mangled_screen = 1;
	    break;

	  case 'x':  /* select or unselect this addr */
	  case PF9:
	  SelectCase:
	    if(screen.mode != ListMode)
	      goto bleep;

	    current->checked = 1 - current->checked;  /* flip it */
	    how_many_selected += (current->checked ? 1 : -1);
	    break;

	  case 'a':  /* select all */
	  case PF10:
	    if(screen.mode != ListMode)
	      goto bleep;

	    how_many_selected = ta_mark_all(first_sel_taline(current));
	    ps->mangled_body = 1;
	    break;

	  case 'u':  /* unselect all */
	  case PF11:
	    if(screen.mode != ListMode)
	      goto bleep;

	    how_many_selected = ta_unmark_all(first_sel_taline(current));
	    ps->mangled_body = 1;
	    break;

	  case 's':  /* switch to SingleMode */
	  case 'l':  /* switch to ListMode */
	  case PF12:
	    if(screen.mode == ListMode && ch == 'l'){
		q_status_message(SM_INFO, 0, 1,
		   "Already in ListMode.  Press \"S\" for Single entry mode.");
		break;
	    }

	    if(screen.mode == SingleMode && ch == 's'){
		q_status_message(SM_INFO, 0, 1,
		   "Already in SingleMode.  Press \"L\" for List entry mode.");
		break;
	    }

	    if(screen.mode == ListMode){
		screen.mode = SingleMode;
		q_status_message(SM_INFO, 0, 1,
		  "Single mode: Use \"P\" or \"N\" to select desired address");
	    }
	    else{
		screen.mode = ListMode;
		q_status_message(SM_INFO, 0, 1,
	    "List mode: Use \"X\" to mark addresses to be included in list");

		if(how_many_selected <= 1){
		    how_many_selected =
				    ta_unmark_all(first_sel_taline(current));
		    current->checked = 1;
		    how_many_selected++;
		}
	    }

	    ps->mangled_screen = 1;
	    break;

	  default:
	  bleep:
	    bogus_command(orig_ch, F_ON(F_USE_FK, ps) ? "F1" : "?");

	  case NO_OP_IDLE:			/* simple timeout */
	  case NO_OP_COMMAND:
	    break;
	}
    }

    /* clean up */
    if(current)
      while(current->prev)
	current = current->prev;

    while(current){
	ctmp = current->next;
	free_taline(&current);
	current = ctmp;
    }

    ps->mangled_screen = 1;
}


/*
 * Previous selectable TA line.
 * skips over the elements with skip_it set
 */
TA_S *
pre_sel_taline(current)
    TA_S *current;
{
    TA_S *p;

    if(!current)
      return NULL;

    p = current->prev;
    while(p && p->skip_it)
      p = p->prev;
    
    return(p);
}


/*
 * Previous TA line, selectable or just printable.
 * skips over the elements with skip_it set
 */
TA_S *
pre_taline(current)
    TA_S *current;
{
    TA_S *p;

    if(!current)
      return NULL;

    p = current->prev;
    while(p && (p->skip_it && !p->print))
      p = p->prev;
    
    return(p);
}


/*
 * Next selectable TA line.
 * skips over the elements with skip_it set
 */
TA_S *
next_sel_taline(current)
    TA_S *current;
{
    TA_S *p;

    if(!current)
      return NULL;

    p = current->next;
    while(p && p->skip_it)
      p = p->next;
    
    return(p);
}


/*
 * Next TA line, including print only lines.
 * skips over the elements with skip_it set unless they are print lines
 */
TA_S *
next_taline(current)
    TA_S *current;
{
    TA_S *p;

    if(!current)
      return NULL;

    p = current->next;
    while(p && (p->skip_it && !p->print))
      p = p->next;
    
    return(p);
}


/*
 * WhereIs for TakeAddr screen.
 *
 * Returns the line match is found in or NULL.
 */
TA_S *
whereis_taline(current)
    TA_S *current;
{
    TA_S *p;
    int   rc, found = 0, wrapped = 0;
    char *result = NULL, buf[MAX_SEARCH+1], tmp[MAX_SEARCH+3];
    static char last[MAX_SEARCH+1];
    HelpType help;
    static ESCKEY_S ekey[] = {
	{0, 0, "", ""},
	{ctrl('Y'), 10, "^Y", "Top"},
	{ctrl('V'), 11, "^V", "Bottom"},
	{-1, 0, NULL, NULL}};

    if(!current)
      return NULL;

    /*--- get string  ---*/
    buf[0] = '\0';
    sprintf(tmp, "Word to find %s%s%s: ",
	     (last[0]) ? "[" : "",
	     (last[0]) ? last : "",
	     (last[0]) ? "]" : "");
    help = NO_HELP;
    while(1){
	rc = optionally_enter(buf,-FOOTER_ROWS(ps_global),0,MAX_SEARCH,1,0,
				 tmp,ekey,help,0);
	if(rc == 3)
	  help = help == NO_HELP ? h_config_whereis : NO_HELP;
	else if(rc == 0 || rc == 1 || rc == 10 || rc == 11 || !buf[0]){
	    if(rc == 0 && !buf[0] && last[0])
	      strcpy(buf, last);

	    break;
	}
    }

    if(rc == 0 && buf[0]){
	p = current;
	while(p = next_taline(p))
	  if(srchstr((char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					    p->strvalue, NULL),
		     buf)){
	      found++;
	      break;
	  }

	if(!found){
	    p = first_taline(current);

	    while(p != current)
	      if(srchstr((char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						p->strvalue, NULL),
			 buf)){
		  found++;
		  wrapped++;
		  break;
	      }
	      else
		p = next_taline(p);
	}
    }
    else if(rc == 10){
	current = first_sel_taline(current);
	result = "Searched to top";
    }
    else if(rc == 11){
	current = last_sel_taline(current);
	result = "Searched to bottom";
    }
    else{
	current = NULL;
	result = "WhereIs cancelled";
    }

    if(found){
	current = p;
	result  = wrapped ? "Search wrapped to beginning" : "Word found";
	strcpy(last, buf);
    }

    q_status_message(SM_ORDER,0,3,result ? result : "Word not found");
    return(current);
}


/*
 * Call the addrbook functions which add the checked addresses.
 *
 * Args: how_many_selected -- how many addresses are checked
 *                  f_line -- the first ta line
 *
 * Returns: 1 -- we're done, caller should return
 *          0 -- we're not done
 */
int
ta_take_marked_addrs(how_many_selected, f_line, command_line)
    int   how_many_selected;
    TA_S *f_line;
    int   command_line;
{
    char **new_list;
    TA_S  *p;

    if(how_many_selected == 0){
	q_status_message(SM_ORDER, 0, 4,
  "No addresses marked for taking. Use ExitTake to leave TakeAddr screen");
	return 0;
    }

    if(how_many_selected == 1){
	for(p = f_line; p; p = next_sel_taline(p))
	  if(p->checked && !p->skip_it)
	    break;

	if(p)
	  add_abook_entry(p,
			  (p->nickname && p->nickname[0]) ? p->nickname : NULL,
			  (p->fullname && p->fullname[0]) ? p->fullname : NULL,
			  (p->fcc && p->fcc[0]) ? p->fcc : NULL,
			  (p->comment && p->comment[0]) ? p->comment : NULL,
			  command_line);
    }
    else{
	new_list = list_of_checked(f_line);
	for(p = f_line; p; p = next_sel_taline(p))
	  if(p->checked && !p->skip_it)
	    break;

	take_to_addrbooks_frontend(new_list, p ? p->nickname : NULL,
		p ? p->fullname : NULL, NULL, p ? p->fcc : NULL,
		p ? p->comment : NULL, command_line);
	free_list(&new_list);
    }

    return 1;
}


int
ta_take_single_addr(cur, command_line)
    TA_S *cur;
    int   command_line;
{
    add_abook_entry(cur,
		    (cur->nickname && cur->nickname[0]) ? cur->nickname : NULL,
		    (cur->fullname && cur->fullname[0]) ? cur->fullname : NULL,
		    (cur->fcc && cur->fcc[0]) ? cur->fcc : NULL,
		    (cur->comment && cur->comment[0]) ? cur->comment : NULL,
		    command_line);

    return 1;
}


/*
 * Mark all of the addresses with a check.
 *
 * Args: f_line -- the first ta line
 *
 * Returns the number of lines checked.
 */
int
ta_mark_all(f_line)
    TA_S *f_line;
{
    TA_S *ctmp;
    int   how_many_selected = 0;

    for(ctmp = f_line; ctmp; ctmp = next_sel_taline(ctmp)){
	ctmp->checked = 1;
	how_many_selected++;
    }

    return(how_many_selected);
}


/*
 * Does the takeaddr list consist of a single address?
 *
 * Args: f_line -- the first ta line
 *
 * Returns 1 if only one address and 0 otherwise.
 */
int
is_talist_of_one(f_line)
    TA_S *f_line;
{
    if(f_line == NULL)
      return 0;

    /* there is at least one, see if there are two */
    if(next_sel_taline(f_line) != NULL)
      return 0;
    
    return 1;
}


/*
 * Turn off all of the check marks.
 *
 * Args: f_line -- the first ta line
 *
 * Returns the number of lines checked (0).
 */
int
ta_unmark_all(f_line)
    TA_S *f_line;
{
    TA_S *ctmp;

    for(ctmp = f_line; ctmp; ctmp = ctmp->next)
      ctmp->checked = 0;

    return 0;
}


/*
 * Manage display of the Take Address screen.
 *
 * Args:     ps -- pine state
 *      current -- the current TA line
 *       screen -- the TA screen
 *   cursor_pos -- return good cursor position here
 */
int
update_takeaddr_screen(ps, current, screen, cursor_pos)
    struct pine  *ps;
    TA_S	 *current;
    TA_SCREEN_S  *screen;
    Pos          *cursor_pos;
{
    int	   dline;
    TA_S  *top_line,
          *ctmp;
    int    longest, i, j;
    char   buf1[MAX_SCREEN_COLS + 1];
    char   buf2[MAX_SCREEN_COLS + 1];
    char  *p, *q;
    int screen_width = ps->ttyo->screen_cols;
    Pos    cpos;

    cpos.row = HEADER_ROWS(ps); /* default return value */

    /* calculate top line of display */
    dline = 0;
    ctmp = top_line = first_taline(current);
    do
      if(((dline++) % (ps->ttyo->screen_rows - HEADER_ROWS(ps)
			  - FOOTER_ROWS(ps))) == 0)
	  top_line = ctmp;
    while(ctmp != current && (ctmp = next_taline(ctmp)));

#ifdef _WINDOWS
    /*
     * Figure out how far down the top line is from the top and how many
     * total lines there are.  Dumb to loop every time thru, but
     * there aren't that many lines, and it's cheaper than rewriting things
     * to maintain a line count in each structure...
     */
    for(dline = 0, ctmp = top_line; ctmp; ctmp = pre_taline(ctmp))
      dline++;

    scroll_setpos(dline - 1L);

    for(ctmp = next_taline(top_line); ctmp ; ctmp = next_taline(ctmp))
      dline++;

    scroll_setrange(dline);
#endif


    /* mangled body or new page, force redraw */
    if(ps->mangled_body || screen->top_line != top_line)
      screen->current = NULL;
    
    /* find length of longest line for nicer formatting */
    longest = 0;
    for(dline = 0, ctmp = top_line;
	dline < ps->ttyo->screen_rows - FOOTER_ROWS(ps) - HEADER_ROWS(ps);
	dline++, ctmp = next_taline(ctmp)){
	int len;

	if(ctmp
	   && !ctmp->print
	   && ctmp->strvalue
	   && longest < (len
		   =strlen((char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						     ctmp->strvalue, NULL))))
	  longest = len;
    }

#define LENGTH_OF_THAT_STRING 5   /* "[X]  " */
    longest = min(longest, ps->ttyo->screen_cols);

    /* loop thru painting what's needed */
    for(dline = 0, ctmp = top_line;
	dline < ps->ttyo->screen_rows - FOOTER_ROWS(ps) - HEADER_ROWS(ps);
	dline++, ctmp = next_taline(ctmp)){

	/*
	 * only fall thru painting if something needs painting...
	 */
	if(!ctmp || !screen->current || ctmp == screen->current ||
							ctmp == current){
	    ClearLine(dline + HEADER_ROWS(ps));
	    if(!ctmp || !ctmp->strvalue)
	      continue;
	}

	p = buf1;
	if(ctmp == current){
	    cpos.row = dline + HEADER_ROWS(ps);  /* col set below */
	    StartInverse();
	}

	if(ctmp->print)
	  j = 0;
	else
	  j = LENGTH_OF_THAT_STRING;

	/*
	 * Copy the value to a temp buffer expanding tabs, and
	 * making sure not to write beyond screen right...
	 */
	q = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
						     ctmp->strvalue, NULL);

	for(i = 0; q[i] && j < ps->ttyo->screen_cols; i++){
	    if(q[i] == ctrl('I')){
		do
		  *p++ = SPACE;
		while(j < ps->ttyo->screen_cols && ((++j)&0x07));
		      
	    }
	    else{
		*p++ = q[i];
		j++;
	    }
	}

	*p = '\0';

	/* mark lines which have check marks */
	if(ctmp == current){
	    if(screen->mode == ListMode){
		sprintf(buf2, "[%c]  %-*.*s",
		    ctmp->checked ? 'X' : SPACE,
		    longest, longest, buf1);
		cpos.col = 1; /* position on the X */
	    }
	    else{
		sprintf(buf2, "     %-*.*s",
		    longest, longest, buf1);
		cpos.col = 5; /* 5 spaces before text */
	    }
	}
	else{
	    if(ctmp->print){
		int len;
		char *start_printing;

		memset((void *)buf2, '-', screen_width * sizeof(char));
		buf2[screen_width] = '\0';
		len = strlen(buf1);
		if(len > screen_width){
		    len = screen_width;
		    buf1[len] = '\0';
		}

		start_printing = buf2 + (screen_width - len) / 2;
		sprintf(start_printing, "%s", buf1);
		if(len < screen_width)
		  start_printing[strlen(start_printing)] = '-';
	    }
	    else{
		if(screen->mode == ListMode)
	          sprintf(buf2, "[%c]  %s", ctmp->checked ? 'X' : SPACE, buf1);
		else
	          sprintf(buf2, "     %s", buf1);
	    }
	}

	PutLine0(dline + HEADER_ROWS(ps), 0, buf2);

	if(ctmp == current)
	  EndInverse();
    }

    ps->mangled_body = 0;
    screen->top_line = top_line;
    screen->current  = current;
    if(cursor_pos)
      *cursor_pos = cpos;

    return(cpos.row);
}


void
takeaddr_screen_redrawer_list()
{
    ps_global->mangled_body = 1;
    (void)update_takeaddr_screen(ps_global, ta_screen->current, ta_screen,
	(Pos *)NULL);
}


void
takeaddr_screen_redrawer_single()
{
    ps_global->mangled_body = 1;
    (void)update_takeaddr_screen(ps_global, ta_screen->current, ta_screen,
	(Pos *)NULL);
}


/*
 * new_taline - create new TA_S, zero it out, and insert it after current.
 *                NOTE current gets set to the new TA_S, too!
 */
TA_S *
new_taline(current)
    TA_S **current;
{
    TA_S *p;

    p = (TA_S *)fs_get(sizeof(TA_S));
    memset((void *)p, 0, sizeof(TA_S));
    if(current){
	if(*current){
	    p->next	     = (*current)->next;
	    (*current)->next = p;
	    p->prev	     = *current;
	    if(p->next)
	      p->next->prev = p;
	}

	*current = p;
    }

    return(p);
}


void
free_taline(p)
    TA_S **p;
{
    if(p){
	if((*p)->addr)
	  mail_free_address(&(*p)->addr);

	if((*p)->strvalue)
	  fs_give((void **)&(*p)->strvalue);

	if((*p)->nickname)
	  fs_give((void **)&(*p)->nickname);

	if((*p)->fullname)
	  fs_give((void **)&(*p)->fullname);

	if((*p)->fcc)
	  fs_give((void **)&(*p)->fcc);

	if((*p)->comment)
	  fs_give((void **)&(*p)->comment);

	if((*p)->prev)
	  (*p)->prev->next = (*p)->next;

	if((*p)->next)
	  (*p)->next->prev = (*p)->prev;

	fs_give((void **)p);
    }
}


/*
 * Return the first selectable TakeAddr line.
 *
 * Args: q -- any line in the list
 */
TA_S *
first_sel_taline(q)
    TA_S *q;
{
    TA_S *first;

    if(q == NULL)
      return NULL;

    first = NULL;
    /* back up to the head of the list */
    while(q){
	first = q;
	q = pre_sel_taline(q);
    }

    /*
     * If first->skip_it, that means we were already past the first
     * legitimate line, so we have to look in the other direction.
     */
    if(first->skip_it)
      first = next_sel_taline(first);
    
    return(first);
}


/*
 * Return the last selectable TakeAddr line.
 *
 * Args: q -- any line in the list
 */
TA_S *
last_sel_taline(q)
    TA_S *q;
{
    TA_S *last;

    if(q == NULL)
      return NULL;

    last = NULL;
    /* go to the end of the list */
    while(q){
	last = q;
	q = next_sel_taline(q);
    }

    /*
     * If last->skip_it, that means we were already past the last
     * legitimate line, so we have to look in the other direction.
     */
    if(last->skip_it)
      last = pre_sel_taline(last);
    
    return(last);
}


/*
 * Return the first TakeAddr line, selectable or just printable.
 *
 * Args: q -- any line in the list
 */
TA_S *
first_taline(q)
    TA_S *q;
{
    TA_S *first;

    if(q == NULL)
      return NULL;

    first = NULL;
    /* back up to the head of the list */
    while(q){
	first = q;
	q = pre_taline(q);
    }

    /*
     * If first->skip_it, that means we were already past the first
     * legitimate line, so we have to look in the other direction.
     */
    if(first->skip_it && !first->print)
      first = next_taline(first);
    
    return(first);
}


/*
 * Find the first TakeAddr line which is checked, beginning with the
 * passed in line.
 *
 * Args: head -- A passed in TakeAddr line, usually will be the first
 *
 * Result: returns a pointer to the first checked line.
 *         NULL if no such line
 */
TA_S *
first_checked(head)
    TA_S *head;
{
    TA_S *p;

    p = head;

    for(p = head; p; p = next_sel_taline(p))
      if(p->checked && !p->skip_it)
	break;

    return(p);
}


/*
 * Form a list of strings which are addresses to go in a list.
 * These are entries in a list, so can be full rfc822 addresses.
 * The strings are allocated here.
 *
 * Args: head -- A passed in TakeAddr line, usually will be the first
 *
 * Result: returns an allocated array of pointers to allocated strings
 */
char **
list_of_checked(head)
    TA_S *head;
{
    TA_S  *p;
    int    count;
    char **list, **cur;
    ADDRESS *a;
    char buf[MAX_ADDR_EXPN+1];

    /* first count them */
    for(p = head, count = 0; p; p = next_sel_taline(p)){
	if(p->checked && !p->skip_it){
	    if(p->frwrded){
		/*
		 * Remove fullname, fcc, comment, and nickname since not
		 * appropriate for list values.
		 */
		if(p->fullname)
		  fs_give((void **)&p->fullname);
		if(p->fcc)
		  fs_give((void **)&p->fcc);
		if(p->comment)
		  fs_give((void **)&p->comment);
		if(p->nickname)
		  fs_give((void **)&p->nickname);
	      
		for(a = p->addr; a; a = a->next)
		  count++;
	    }
	    else{
		/*
		 * Don't even attempt to include bogus addresses in
		 * the list.  If the user wants to get at those, they
		 * have to try taking only that single address.
		 */
		if(p->addr && p->addr->host && p->addr->host[0] == '.')
		  p->skip_it = 1;
		else
		  count++;
	    }
	}
    }
    
    /* allocate pointers */
    list = (char **)fs_get((count + 1) * sizeof(char *));
    memset((void *)list, 0, (count + 1) * sizeof(char *));

    cur = list;

    /* allocate and point to address strings */
    for(p = head; p; p = next_sel_taline(p)){
	if(p->checked && !p->skip_it){
	    if(p->frwrded)
	      for(a = p->addr; a; a = a->next)
		*cur++ = cpystr(addr_string(a, buf));
	    else
	      *cur++ = cpystr(p->strvalue);
	}
    }

    return(list);
}


/*
 * Free the list allocated above.
 *
 * Args: list -- The address of the list that was returned above.
 */
void
free_list(list)
    char ***list;
{
    char **p;

    for(p = *list; *p; p++)
      fs_give((void **)p);

    fs_give((void **)list);
}


/*
 * Execute command to take addresses out of message and put in the address book
 *
 * Args: ps     -- pine state
 *       msgmap -- the MessageMap
 *       agg    -- this is aggregate operation if set
 *
 * Result: The entry is added to an address book.
 */     
void
cmd_take_addr(ps, msgmap, agg)
    struct pine *ps;
    MSGNO_S     *msgmap;
    int          agg;
{
    long      i;
    ENVELOPE *env;
    int       how_many_selected,
	      added,
	      special_processing = 0,
	      select_froms = 0;
    TA_S     *current,
	     *prev_comment_line,
	     *ta;
    BODY     **body_h,
	      *body = NULL;

    dprint(2, (debugfile, "\n - taking address into address book - \n"));

    if(agg && !pseudo_selected(msgmap))
      return;

    if(mn_total_cur(msgmap) == 1)
      special_processing = 1;

    if(agg)
      select_froms = 1;

    ps->mangled_footer = 1;
    how_many_selected = 0;
    current = NULL;

    /* this is a non-selectable label */
    current = fill_in_ta(&current, (ADDRESS *)NULL, 0,
	       " These entries are taken from the attachments ");
    prev_comment_line = current;

    /*
     * Add addresses from special attachments for each message.
     */
    added = 0;
    for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap)){
	env = mail_fetchstructure(ps->mail_stream, mn_m2raw(msgmap, i),
				  &body);
	if(!env){
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
	       "Can't take address into address book. Error accessing folder");
	    goto bomb;
	}

	added += process_special_abook_attachments(ps->mail_stream,
						   mn_m2raw(msgmap, i),
						   body, body, &current);
    }

    /*
     * add a comment line to separate attachment address lines
     * from header address lines
     */
    if(added > 0){
	current = fill_in_ta(&current, (ADDRESS *)NULL, 0,
	       " These entries are taken from the msg headers ");
	prev_comment_line = current;
	select_froms = 0;
    }
    else{  /* turn off header comment, and no separator comment */
	prev_comment_line->print = 0;
	prev_comment_line        = NULL;
    }

    body = NULL;
    /*
     * Add addresses from headers of messages.
     */
    for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap)){

	if(special_processing)
	  body_h = &body;
	else
	  body_h = NULL;

	env = mail_fetchstructure(ps->mail_stream, mn_m2raw(msgmap, i),
				  body_h);
	if(!env){
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
	       "Can't take address into address book. Error accessing folder");
	    goto bomb;
	}

	added = add_addresses_to_talist(ps, i, "from", &current,
					env->from, select_froms);
	if(select_froms)
	  how_many_selected += added;

	(void)add_addresses_to_talist(ps,i,"reply-to",&current,env->reply_to,0);
	(void)add_addresses_to_talist(ps, i, "to", &current, env->to, 0);
	(void)add_addresses_to_talist(ps, i, "cc", &current, env->cc, 0);
	(void)add_addresses_to_talist(ps, i, "bcc", &current, env->bcc, 0);
    }
	
    /*
     * Check to see if we added an explanatory line about the
     * header addresses but no header addresses.  If so, remove the
     * explanatory line.
     */
    if(prev_comment_line){
	how_many_selected -=
	    eliminate_dups_and_us(first_sel_taline(current));
	for(ta = prev_comment_line->next; ta; ta = ta->next)
	  if(!ta->skip_it)
	    break;

	/* all entries were skip_it entries, turn off print */
	if(!ta){
	    prev_comment_line->print = 0;
	    prev_comment_line        = NULL;
	}
    }

    /*
     * If there's only one message we let the user view it using ^T
     * from the header editor.
     */
    if(special_processing && env && body){
	msgno_for_pico_callback = mn_m2raw(msgmap, mn_first_cur(msgmap));
	body_for_pico_callback  = body;
	env_for_pico_callback   = env;
    }
    else{
	env_for_pico_callback   = NULL;
	body_for_pico_callback  = NULL;
    }

    /*
     * If a single message, also look for addresses in the text of the body.
     */
    added = 0;
    if(special_processing && body){
	current = fill_in_ta(&current, (ADDRESS *)NULL, 0,
    " Below this line are some possibilities taken from the text of the msg ");
	prev_comment_line = current;
	added = grab_addrs_from_body(ps->mail_stream,
				     mn_m2raw(msgmap, mn_first_cur(msgmap)),
				     body, &current);
	if(added == 0){
	    prev_comment_line->print = 0;
	    prev_comment_line        = NULL;
	}
    }

    current = first_sel_taline(current);
    how_many_selected -= eliminate_dups_and_us(current);

    /*
     * Check to see if we added some below the comment line and
     * then decided they're all dups.  In that case, we don't
     * want to print out the line below the header list.
     */
    if(added > 0 && prev_comment_line){
	for(ta = prev_comment_line->next; ta; ta = ta->next)
	  if(!ta->skip_it)
	    break;

	/* all entries were skip_it entries, turn off print */
	if(!ta){
	    prev_comment_line->print = 0;
	    prev_comment_line        = NULL;
	}
    }

    takeaddr_screen(ps, current, how_many_selected,
		    agg ? ListMode : SingleMode);

bomb:
    env_for_pico_callback   = NULL;
    body_for_pico_callback  = NULL;

    if(agg)
      restore_selected(msgmap);
}


int
add_addresses_to_talist(ps, msgno, field, old_current, adrlist, checked)
    struct pine *ps;
    long         msgno;
    char        *field;
    TA_S       **old_current;
    ADDRESS     *adrlist;
    int          checked;
{
    ADDRESS *addr;
    int      added = 0;

    for(addr = adrlist; addr; addr = addr->next){
	if(addr->host && addr->host[0] == '.'){
	    char *h, *fields[2];

	    fields[0] = field;
	    fields[1] = NULL;
	    if(h = xmail_fetchheader_lines(ps->mail_stream, msgno, fields)){
		char *p, fname[32];

		sprintf(fname, "%s:", field);
		for(p = h; p = strstr(p, fname); )
		  rplstr(p, strlen(fname), "");   /* strip field strings */
		
		sqznewlines(h);                   /* blat out CR's & LF's */
		for(p = h; p = strchr(p, TAB); )
		  *p++ = ' ';                     /* turn TABs to space */
		
		if(*h){
		    unsigned long l;
		    ADDRESS *new_addr;

		    new_addr = (ADDRESS *)fs_get(sizeof(ADDRESS));
		    memset(new_addr, 0, sizeof(ADDRESS));

		    /*
		     * Base64 armor plate the gunk to protect against
		     * c-client quoting in output.
		     */
		    p = (char *)rfc822_binary((void *)h,
					      (unsigned long)strlen(h), &l);
		    new_addr->mailbox = (char *)fs_get(strlen(p) + 4);
		    sprintf(new_addr->mailbox, "&%s", p);
		    fs_give((void **)&p);
		    new_addr->host = cpystr(".RAW-FIELD.");
		    fill_in_ta(old_current, new_addr, 0, (char *)NULL);
		    mail_free_address(&new_addr);
		}

		fs_give((void **)&h);
	    }

	    return(0);
	}
    }

    /* no syntax errors in list, add them all */
    for(addr = adrlist; addr; addr = addr->next){
	fill_in_ta(old_current, addr, checked, (char *)NULL);
	added++;
    }

    return(added);
}


/*
 * Look for Application/directory attachments and process them.
 * Return number of lines added to the ta_list.
 */
int
process_special_abook_attachments(stream, msgno, root, body, ta_list)
    MAILSTREAM *stream;
    long        msgno;
    BODY       *root;
    BODY       *body;
    TA_S      **ta_list;
{
    ADDRESS   *addrlist;
    char      *addrs,        /* comma-separated list of addresses */
	      *fakedomain = "@",
	      *value,
	      *encoded,
	      *nickname,
	      *fullname,
	      *fcc,
	      *tag,
	      *num = NULL,
	      *defaulttype = NULL,
	      *charset = NULL,
	      *altcharset,
	      *comment,
	     **lines = NULL,
	     **ll;
    size_t     space;
    int        used = 0,
	       selected = 0;
    PART      *part;
    PARAMETER *parm;

    /*
     * Look through all the subparts searching for our special type.
     */
    if(body && body->type == TYPEMULTIPART){
	for(part = body->contents.part; part; part = part->next)
	  selected += process_special_abook_attachments(stream, msgno, root,
						      &part->body, ta_list);
    }
    /* only look in rfc822 subtype of type message */
    else if(body && body->type == TYPEMESSAGE
	    && body->subtype && !strucmp(body->subtype, "rfc822")){
	selected += process_special_abook_attachments(stream, msgno, root,
					      body->contents.msg.body, ta_list);
    }
    /* found one */
    else if(body && body->type == TYPEAPPLICATION
            && body->subtype && !strucmp(body->subtype, "directory")){

	/*
	 * The Application/directory type has a possible parameter called
	 * "defaulttype" that we need to look for.  There is also a
	 * possible default "charset".  We don't care about any of the
	 * other parameters.
	 */
	for(parm = body->parameter; parm; parm = parm->next)
	  if(!strucmp("defaulttype", parm->attribute))
	    break;
	
	if(parm)
	  defaulttype = parm->value;

	/* ...and look for possible charset parameter */
	for(parm = body->parameter; parm; parm = parm->next)
	  if(!strucmp("charset", parm->attribute))
	    break;
	
	if(parm)
	  charset = parm->value;

	num = partno(root, body);
	lines = detach_abook_att(stream, msgno, body, num);
	if(num)
	  fs_give((void **)&num);

	nickname = fullname = comment = fcc = NULL;
#define CHUNK (size_t)500
	space = CHUNK;
	/* make comma-separated list of email addresses in addrs */
	addrs = (char *)fs_get((space+1) * sizeof(char));
	*addrs = '\0';
	for(ll = lines; ll && *ll && **ll; ll++){
	    altcharset = NULL;
	    value = getaltcharset(*ll, &tag, &altcharset);

	    /* support default tag */
	    if(*tag == '\0' && defaulttype){
		fs_give((void **)&tag);
		tag = cpystr(defaulttype);
	    }

	    /* add another address to addrs */
	    if(!strucmp(tag, "email")){
		if(value && *value){
		    encoded = encode_fullname_of_addrstring(value,
			(altcharset && *altcharset) ? altcharset
						    : (charset && *charset)
						     ? charset
						     : ps_global->VAR_CHAR_SET);
		    /* allocate more space */
		    if((used + strlen(encoded) + 1) > space){
			space += CHUNK;
			fs_resize((void **)&addrs, (space+1) * sizeof(char));
		    }

		    if(*addrs)
		      strcat(addrs, ",");

		    strcat(addrs, encoded);
		    used += (strlen(encoded) + 1);
		    if(encoded)
		      fs_give((void **)&encoded);
		}
	    }
	    else if(!strucmp(tag, "misc")){
		if(value && *value){
		    encoded = rfc1522_encode(tmp_20k_buf,
					     (unsigned char *)value,
			(altcharset && *altcharset) ? altcharset
						    : (charset && *charset)
						     ? charset
						     : ps_global->VAR_CHAR_SET);
		    comment = cpystr(encoded);
		}
	    }
	    else if(!strucmp(tag, "x-fcc")){
		if(value && *value)
		  fcc = cpystr(value);
	    }
	    else if(!strucmp(tag, "cn")){
		if(value && *value){
		    encoded = rfc1522_encode(tmp_20k_buf,
					     (unsigned char *)value,
			(altcharset && *altcharset) ? altcharset
						    : (charset && *charset)
						     ? charset
						     : ps_global->VAR_CHAR_SET);
		    fullname = cpystr(encoded);
		}
	    }
	    /* suggested nickname */
	    else if(!strucmp(tag, "x-nickname")){
		if(value && *value)
		  nickname = cpystr(value);
	    }

	    if(tag)
	      fs_give((void **)&tag);

	    if(altcharset)
	      fs_give((void **)&altcharset);
	}

	/*
	 * Parse it into an addrlist, which is what fill_in_ta wants
	 * to see.
	 */
	addrlist = NULL;
	rfc822_parse_adrlist(&addrlist, addrs, fakedomain);
	if(addrs)
	  fs_give((void **)&addrs);

	selected++;
	*ta_list = fill_in_ta(ta_list, addrlist, 0,
			      fullname ? fullname : "Forwarded Entry");
	(*ta_list)->nickname = nickname;
	(*ta_list)->fullname = fullname;
	(*ta_list)->comment  = comment;
	(*ta_list)->fcc      = fcc;

	if(lines)
	  free_list(&lines);

	if(addrlist)
	  mail_free_address(&addrlist);
    }

    return(selected);
}


/*
 * Look through line which is supposed to look like
 *
 *  type;charset=iso-8859-1;encoding=base64: stuff
 *
 * Type might be email, or x-nickname, ...  It is optional because of the
 * defaulttype parameter.  The semicolon and colon are special chars.  Each
 * parameter in this line is a semicolon followed by the parameter type "="
 * the parameter value.  Parameters are optional, too.  There is always a colon,
 * followed by stuff.  Whitespace can be everywhere up to where stuff starts.
 * There is also an optional <group> "." preceding the type, which we will
 * ignore.  It's for grouping related lines.
 *
 * BUG:  Ignoring encoding for now.  It shouldn't happen in any reasonable
 *       entry we're concerned with. If it does, it's not the end of the world.
 *
 * Args: line     -- the line to look at
 *       type     -- this is a return value, and is an allocated copy of
 *                    the type, freed by the caller
 *       alt      -- this is a return value, and is an allocated copy of
 *                    the value of the alternate charset, to be freed by
 *                    the caller.  For example, this might be "iso-8859-2".
 *
 * Return value: a pointer to the start of stuff, or NULL if the syntax
 * isn't right.  It's possible for stuff to be equal to "".
 */
char *
getaltcharset(line, type, alt)
    char  *line;
    char **type;
    char **alt;
{
    char *p, *q, *left_semi, *group_dot, *colon, *start_of_cset, tmpsave;
    static char *cset = "charset";

    if(type)
      *type = NULL;

    if(alt)
      *alt = NULL;

    colon = strindex(line, ':');
    if(!colon)
      return NULL;

    left_semi = strindex(line, ';');
    if(left_semi && left_semi > colon)
      left_semi = NULL;
    
    group_dot = strindex(line, '.');
    if(group_dot && (group_dot > colon || left_semi && group_dot > left_semi))
      group_dot = NULL;

    /*
     * Type is everything up to the semicolon, or the colon if no semicolon.
     * However, we want to skip optional <group> ".".
     */
    if(type){
	q = left_semi ? left_semi : colon;
	tmpsave = *q;
	*q = '\0';
	*type = cpystr(group_dot ? group_dot+1 : line);
	*q = tmpsave;
	sqzspaces(*type);
    }

#define SKIP_WHITESPACE(p) do{while(*p && isspace((unsigned char)*p))p++;}while(0)

    if(left_semi && alt
       && (p = srchstr(left_semi+1, cset))
       && p < colon){
	p += strlen(cset);
	SKIP_WHITESPACE(p);
	if(*p++ == '='){
	    SKIP_WHITESPACE(p);
	    if(p < colon){
		start_of_cset = p;
		p++;
		while(p < colon && !isspace((unsigned char)*p) && *p != ';')
		  p++;
		
		tmpsave = *p;
		*p = '\0';
		*alt = cpystr(start_of_cset);
		*p = tmpsave;
	    }
	}
    }

    p = colon + 1;
    SKIP_WHITESPACE(p);

    return(p);
}


/*
 * Fetch this body part, content-decode it if necessary, split it into
 * lines, and return the lines in an allocated array, which is freed
 * by the caller.  Folded lines are replaced by single long lines.
 */
char **
detach_abook_att(stream, msgno, body, partnum)
    MAILSTREAM *stream;
    long        msgno;
    BODY       *body;
    char       *partnum;
{
    unsigned long length;
    char    *text  = NULL, /* raw text */
	    *dtext = NULL, /* content-decoded text */
	   **res   = NULL, /* array of decoded lines */
	    *lptr,         /* line pointer */
	    *next_line;
    int      i, count;

    /* make our own copy of text so we can change it */
    text = cpystr(mail_fetchbody(stream, msgno, partnum, &length));

    /* decode the text */
    switch(body->encoding){
      default:
      case ENC7BIT:
      case ENC8BIT:
      case ENCBINARY:
	dtext = text;
	break;

      case ENCBASE64:
	dtext = (char *)rfc822_base64((unsigned char *)text,
				      (unsigned long)strlen(text),
				      &length);
	if(text)
	  fs_give((void **)&text);

	break;

      case ENCQUOTEDPRINTABLE:
	dtext = (char *)rfc822_qprint((unsigned char *)text,
				      (unsigned long)strlen(text),
				      &length);
	if(text)
	  fs_give((void **)&text);

	break;
    }

    /* count the lines */
    next_line = dtext;
    count = 0;
    for(lptr = next_line; lptr && *lptr; lptr = next_line){
	for(next_line = lptr; *next_line; next_line++){
	    /*
	     * Find end of current line.  This should really be CRLF,
	     * but we'll accept either CR or LF.
	     */
	    if(*next_line == '\r' || *next_line == '\n'){
		next_line++;
		/* Find start of next line */
		while(*next_line && (*next_line == '\r' || *next_line == '\n'))
		  next_line++;
		
		/* not a folded line, count it */
		if(!*next_line || (*next_line != SPACE && *next_line != TAB))
		  break;
	    }
	}

	count++;
    }

    /* allocate space for resulting array of lines */
    res = (char **)fs_get((count + 1) * sizeof(char *));
    memset((void *)res, 0, (count + 1) * sizeof(char *));
    next_line = dtext;
    for(i=0, lptr=next_line; lptr && *lptr && i < count; lptr=next_line, i++){
	/*
	 * Move next_line to start of next line and null terminate
	 * current line.
	 */
	for(next_line = lptr; *next_line; next_line++){
	    /*
	     * Find end of current line.  This should really be CRLF,
	     * but we'll accept either CR or LF.
	     */
	    if(*next_line == '\r' || *next_line == '\n'){
		next_line++;
		/* Find start of next line */
		while(*next_line && (*next_line == '\r' || *next_line == '\n'))
		  next_line++;
		
		/* not a folded line, terminate it */
		if(!*next_line || (*next_line != SPACE && *next_line != TAB)){
		    *(next_line-1) = '\0';
		    break;
		}
	    }
	}

	sqznewlines(lptr);  /* turns folded lines into long lines, and
				eliminates trailing CR/LF */
	res[i] = cpystr(lptr);
    }

    if(dtext)
      fs_give((void **)&dtext);

    res[count] = '\0';
    return(res);
}


/*
 * Look for possible addresses in the first text part of a message for
 * use by TakeAddr command.
 * Returns the number of TA lines added.
 */
int
grab_addrs_from_body(stream, msgno, body, ta_list)
    MAILSTREAM *stream;
    long        msgno;
    BODY       *body;
    TA_S      **ta_list;
{
#define MAXLINESZ 2000
    char       line[MAXLINESZ + 1];
    STORE_S   *so;
    gf_io_t    pc;
    SourceType src = CharStar;
    int        added = 0;
    char      *at, *colon, *p, *next_p, *start, *end, *fn_end;
    char      *tmp_a_string, *tmp_personal;
    char       save_end, save_fn_end;
    char      *fakedomain = "@";
    ADDRESS   *addr;

    /*
     * If it is text/plain or it is multipart with a first part of text/plain,
     * we want to continue, else forget it.
     */
    if(!((body->type == TYPETEXT && body->subtype &&
		!strucmp(body->subtype, "plain"))
			      ||
         (body->type == TYPEMULTIPART && body->contents.part
		&& body->contents.part->body.type == TYPETEXT
		&& !strucmp(body->contents.part->body.subtype, "plain"))))
      return 0;

#ifdef DOS
    src = TmpFileStar;
#endif

    if((so = so_get(src, NULL, EDIT_ACCESS)) == NULL)
      return 0;

    gf_set_so_writec(&pc, so);
    if(!get_body_part_text(stream, body, msgno, "1", pc, NULL, NULL)){
	so_give(&so);
	return 0;
    }

    so_seek(so, 0L, 0);

    while(get_line_of_message(so, line, MAXLINESZ + 1)){

	colon = NULL;

	/* process each @ in the line */
	for(p = (char *)line; at = strindex(p, '@'); p = next_p){

	    next_p = at + 1;

	    /* find start of address */
	    start = at;
	    while(start > p && !isspace((unsigned char)*(start-1)))
	      start--;

	    /* remove parens */
	    if(*start == '(')
	      start++;
	    
	    /* find end of address */
	    end = at;
	    while(*end != '\0' && !isspace((unsigned char)*end))
	      end++;

	    save_end = *end;
	    *end = '\0';

	    end--;
	    /*
	     * remove periods, commas, closing parens, question
	     * marks, and exclamation marks
	     */
	    if(end > start &&
		       (*end == '.' ||
		        *end == ',' ||
			*end == ')' ||
			*end == '!' ||
			*end == '?')){
		*(end + 1) = save_end;
		save_end = *end;
		*end = '\0';
	    }
	    else
	      end++;

	    tmp_personal = NULL;

	    if(*start == '<' && end > start && *(end - 1) == '>'){
		/*
		 * Take a shot at looking for full name
		 * If we can find a colon maybe we've got a header line
		 * embedded in the body.
		 *
		 * Or, if this isn't the first one in the line we'll just
		 * go back to a comma.
		 */
		if(colon && (colon = strrindex(p, ',')) ||
		   (colon = strrindex(p, ':'))){
		    /* skip white space after colon */
		    colon++;
		    while(colon < start && isspace((unsigned char)*colon))
		      colon++;

		    if(colon < start){
			/* remove white space between fullname and start */
			fn_end = start - 1;
			while(fn_end > colon
			      && isspace((unsigned char)*(fn_end - 1)))
			  fn_end--;

			save_fn_end = *fn_end;
			*fn_end = '\0';
			tmp_personal = cpystr(colon);
			*fn_end = save_fn_end;
		    }
		}
	    }

	    /* rfc822_parse_adrlist feels free to destroy input so send copy */
	    tmp_a_string = cpystr(start);
	    *end = save_end;
	    addr = NULL;
	    ps_global->c_client_error[0] = '\0';
	    rfc822_parse_adrlist(&addr, tmp_a_string, fakedomain);
	    if(tmp_a_string)
	      fs_give((void **)&tmp_a_string);

	    if(tmp_personal){
		if(addr){
		    if(addr->personal)
		      fs_give((void **)&addr->personal);

		    addr->personal = tmp_personal;
		}
		else
		  fs_give((void **)&tmp_personal);
	    }

	    if((addr && addr->host && addr->host[0] == '@') ||
	       ps_global->c_client_error[0]){
		mail_free_address(&addr);
		continue;
	    }

	    if(addr){
		added++;
		*ta_list = fill_in_ta(ta_list, addr, 0, (char *)NULL);
		mail_free_address(&addr);
	    }
	}
    }

    so_give(&so);
    return(added);
}


/*
 * Get the next line of the object pointed to by source.
 * Skips empty lines.
 * Linebuf is a passed in buffer to put the line in.
 * Linebuf is null terminated and \r and \n chars are removed.
 * 0 is returned when there is no more input.
 */
int
get_line_of_message(source, linebuf, linebuflen)
    STORE_S *source;
    char    *linebuf;
    int      linebuflen;
{
    int pos = 0;
    unsigned char c;

    if(linebuflen < 2)
      return 0;

    while(so_readc(&c, source)){
	if(c == '\n' || c == '\r'){
	  if(pos > 0)
	    break;
	}
	else{
	    linebuf[pos++] = c;
	    if(pos >= linebuflen - 2)
	      break;
	}
    }

    linebuf[pos] = '\0';

    return(pos);
}


/*
 * Copy the first address in list a and return it in allocated memory.
 */
ADDRESS *
copyaddr(a)
    ADDRESS *a;
{
    ADDRESS *new;

    new = mail_newaddr();
    if(a->personal)
      new->personal = cpystr(a->personal);

    if(a->adl)
      new->adl      = cpystr(a->adl);

    if(a->mailbox)
      new->mailbox  = cpystr(a->mailbox);

    if(a->host)
      new->host     = cpystr(a->host);

    new->next = NULL;

    return(new);
}


/*
 * Copy the whole list a.
 */
ADDRESS *
copyaddrlist(a)
    ADDRESS *a;
{
    ADDRESS *new = NULL, *head = NULL, *current;

    for(; a; a = a->next){
	new = mail_newaddr();
	if(!head)
	  head = current = new;
	else{
	    current->next = new;
	    current = new;
	}

	if(a->personal)
	  new->personal = cpystr(a->personal);

	if(a->adl)
	  new->adl      = cpystr(a->adl);

	if(a->mailbox)
	  new->mailbox  = cpystr(a->mailbox);

	if(a->host)
	  new->host     = cpystr(a->host);
    }

    if(new)
      new->next = NULL;

    return(head);
}


/*
 * Inserts a new entry based on addr in the TA list.
 *
 * Args: old_current -- the TA list
 *              addr -- the address for this line
 *           checked -- start this line out checked
 *      print_string -- if non-null, this line is informational and is just
 *                       printed out, it can't be selected
 */
TA_S *
fill_in_ta(old_current, addr, checked, print_string)
    TA_S    **old_current;
    ADDRESS  *addr;
    int       checked;
    char     *print_string;
{
    TA_S *new_current;

    /* c-client convention for group syntax, which we don't want to deal with */
    if(!print_string && (!addr || !addr->mailbox || !addr->host))
      new_current = *old_current;
    else{

	new_current           = new_taline(old_current);
	if(print_string && addr){
	    new_current->frwrded  = 1;
	    new_current->skip_it  = 0;
	    new_current->print    = 0;
	    new_current->checked  = checked;
	    new_current->addr     = copyaddrlist(addr);
	    new_current->strvalue = cpystr(print_string);
	}
	else if(print_string){
	    new_current->frwrded  = 0;
	    new_current->skip_it  = 1;
	    new_current->print    = 1;
	    new_current->checked  = 0;
	    new_current->addr     = 0;
	    new_current->strvalue = cpystr(print_string);
	}
	else{
	    char buf[MAX_ADDR_EXPN+1];

	    new_current->frwrded  = 0;
	    new_current->skip_it  = 0;
	    new_current->print    = 0;
	    new_current->checked  = checked;
	    new_current->addr     = copyaddr(addr);
	    if(addr->host[0] == '.')
	      new_current->strvalue
		  = cpystr("Error in address (ok to try Take anyway)");
	    else
	      new_current->strvalue
			      = cpystr(addr_string(new_current->addr,buf));
	}
    }

    return(new_current);
}


int
eliminate_dups_and_us(list)
    TA_S *list;
{
    return(eliminate_dups_and_maybe_us(list, 1));
}


int
eliminate_dups_but_not_us(list)
    TA_S *list;
{
    return(eliminate_dups_and_maybe_us(list, 0));
}


/*
 * Look for dups in list and mark them so we'll skip them.
 *
 * We also throw out any addresses that are us (if us_too is set), since
 * we won't usually want to take ourselves to the addrbook.
 * On the otherhand, if there is nothing but us, we leave it.
 *
 * Returns the number of dups eliminated that were also selected.
 */
int
eliminate_dups_and_maybe_us(list, us_too)
    TA_S *list;
    int   us_too;
{
    ADDRESS *a, *b;
    TA_S    *ta, *tb;
    int eliminated = 0;

    /* toss dupes */
    for(ta = list; ta; ta = ta->next){

	if(ta->skip_it) /* already tossed */
	  continue;

	a = ta->addr;

	/* Check addr "a" versus tail of the list */
	for(tb = ta->next; tb; tb = tb->next){
	    b = tb->addr;
	    if(dup_addrs(a, b)){
		if(ta->checked || !(tb->checked)){
		    /* throw out b */
		    if(tb->checked)
		      eliminated++;

		    tb->skip_it = 1;
		}
		else{ /* tb->checked */
		    /* throw out a */
		    ta->skip_it = 1;
		    break;
		}
	    }
	}
    }

    if(us_too){
	/* check whether all remaining addrs are us */
	for(ta = list; ta; ta = ta->next){

	    if(ta->skip_it) /* already tossed */
	      continue;

	    a = ta->addr;

	    if(!address_is_us(a, ps_global))
	      break;
	}

	/* if at least one address that isn't us, remove us from the list */
	if(ta){
	    for(ta = list; ta; ta = ta->next){

		if(ta->skip_it) /* already tossed */
		  continue;

		a = ta->addr;

		if(address_is_us(a, ps_global)){
		    if(ta->checked)
		      eliminated++;

		    /* throw out a */
		    ta->skip_it = 1;
		}
	    }
	}
    }

    return(eliminated);
}


/*
 * Returns 1 if x and y match, 0 if not.
 */
int
dup_addrs(x, y)
    ADDRESS *x, *y;
{
    return(x && y && strucmp(x->mailbox, y->mailbox) == 0
           &&  strucmp(x->host, y->host) == 0);
}


/*
 * "Take" address(es) from current active entry and put in any address book
 *
 * Args: abook        -- Address book handle
 *       cur_line     -- The current line position (in global display list)
 *			 of cursor
 *
 * Result: The entry is added to one of the address books.
 */     
void
internal_take(abook, cur_line)
    AdrBk *abook;
    long   cur_line;
{
    ADDRESS       *addr, *a, *a2;
    int            how_many_selected;
    TA_S          *current;
    AdrBk_Entry   *abe;
    AddrScrn_Disp *dl;
    char          *fcc, *comment, *fullname, *nickname;
    TakeAddrScreenMode mode;

    dprint(2, (debugfile, "\n - taking address from address book - \n"));

    dl  = dlist(cur_line);
    abe = ae(cur_line);
    if(!dl || !abe)
      return;

    fcc      = (abe->fcc && abe->fcc[0]) ? abe->fcc : NULL;
    comment  = (abe->extra && abe->extra[0]) ? abe->extra : NULL;
    fullname = (abe->fullname && abe->fullname[0]) ? abe->fullname : NULL;
    nickname = (abe->nickname && abe->nickname[0]) ? abe->nickname : NULL;
    how_many_selected = 0;
    current = NULL;

    addr = abe_to_address(abe, dl, abook, &how_many_selected);
    if(!addr)
      return;

    switch(dl->type){
      case Simple:
	mode = SingleMode;
	current = fill_in_ta(&current, addr, 0, (char *)NULL);
        break;

      case ListEnt:
	mode = SingleMode;
	current = fill_in_ta(&current, addr, 0, (char *)NULL);
	fcc = NULL;
	comment = NULL;
	fullname = NULL;
	nickname = NULL;
        break;

      case ListHead:
	mode = ListMode;
	for(a = addr; a; a = a->next){
	    a2 = copyaddr(a);
	    current = fill_in_ta(&current, a2, 1, (char *)NULL);
	    if(a2)
	      mail_free_address(&a2);
	}

        break;
      
      default:
	dprint(2,
	    (debugfile, "default case in internal_take, shouldn't happen\n"));
	return;
    } 

    if(addr)
      mail_free_address(&addr);

    current = first_sel_taline(current);

    how_many_selected -= eliminate_dups_but_not_us(current);
    current = first_sel_taline(current);
    if(current){
	if(fullname && *fullname)
	  current->fullname = cpystr(fullname);

	if(fcc && *fcc)
	  current->fcc = cpystr(fcc);

	if(comment && *comment)
	  current->comment = cpystr(comment);

	if(nickname && *nickname)
	  current->nickname = cpystr(nickname);
    }

    takeaddr_screen(ps_global, current, how_many_selected, mode);
}


/*
 * Add entries specified by system administrator.  If the nickname already
 * exists, it is not touched.
 */
void
add_forced_entries(abook)
    AdrBk *abook;
{
    AdrBk_Entry *abe;
    char *nickname, *fullname, *address;
    char *end_of_nick, *end_of_full, **t;


    if(!ps_global->VAR_FORCED_ABOOK_ENTRY ||
       !ps_global->VAR_FORCED_ABOOK_ENTRY[0] ||
       !ps_global->VAR_FORCED_ABOOK_ENTRY[0][0])
	return;

    for(t = ps_global->VAR_FORCED_ABOOK_ENTRY; *t != NULL; t++){
	nickname = *t;

	/*
	 * syntax for each element is
	 * nick[whitespace]|[whitespace]Fullname[WS]|[WS]Address
	 */

	/* find end of nickname */
	end_of_nick = nickname;
	while(*end_of_nick
	      && !isspace((unsigned char)*end_of_nick)
	      && *end_of_nick != '|')
	  end_of_nick++;

	/* find the pipe character between nickname and fullname */
	fullname = end_of_nick;
	while(*fullname && *fullname != '|')
	  fullname++;
	
	if(*fullname)
	  fullname++;

	*end_of_nick = '\0';
	abe = adrbk_lookup_by_nick(abook, nickname, NULL);

	if(!abe){  /* If it isn't there, add it */

	    /* skip whitespace before fullname */
	    while(*fullname && isspace((unsigned char)*fullname))
	      fullname++;

	    /* find the pipe character between fullname and address */
	    end_of_full = fullname;
	    while(*end_of_full && *end_of_full != '|')
	      end_of_full++;

	    if(!*end_of_full){
		dprint(2, (debugfile,
		    "missing | in forced-abook-entry \"%s\"\n", nickname));
		continue;
	    }

	    address = end_of_full + 1;

	    /* skip whitespace before address */
	    while(*address && isspace((unsigned char)*address))
	      address++;

	    if(*address == '('){
		dprint(2, (debugfile,
		   "no lists allowed in forced-abook-entry \"%s\"\n", address));
		continue;
	    }

	    /* go back and remove trailing white space from fullname */
	    while(*end_of_full == '|' || isspace((unsigned char)*end_of_full)){
		*end_of_full = '\0';
		end_of_full--;
	    }

	    (void)adrbk_add(abook,
			   NO_NEXT,
			   nickname,
			   fullname,
			   address,
			   NULL,
			   NULL,
			   Single,
			   (adrbk_cntr_t *)NULL,
			   (int *)NULL);
	}
    }
}



#ifdef	_WINDOWS
/*
 * addr_scroll_up - adjust the global screen state struct such that pine's
 *		    window on the data is shifted DOWN (i.e., the data's
 *		    scrolled up).
 */
int
addr_scroll_up(count)
    long count;
{
    if(count < 0)
	return(addr_scroll_down(-count));
    else if(count){
	long i;

	for(i = count; i && as.top_ent + 1 < as.last_ent; i--, as.top_ent++)
	  ;

	as.cur_row = ((i = as.cur_row - (count - i)) < 0) ? 0 : i;
	while(!line_is_selectable(as.top_ent + as.cur_row))
	  as.cur_row++;

	as.old_cur_row = as.cur_row;
    }

    return(1);
}


/*
 * addr_scroll_up - adjust the global screen state struct such that pine's
 *		    window on the data is shifted UP (i.e., the data's
 *		    scrolled down).
 */
int
addr_scroll_down(count)
    long count;
{
    if(count < 0)
      return(addr_scroll_up(-count));
    else if(count){
	long i;

	for(i = count; i && as.top_ent; i--, as.top_ent--)
	  ;

	as.cur_row = ((i = as.cur_row + (count - i)) < as.l_p_page - 1)
		       ? i : (as.l_p_page - 1);
	while(!line_is_selectable(as.top_ent + as.cur_row))
	  as.cur_row--;

	as.old_cur_row = as.cur_row;
    }

    return(1);
}


/*
 * addr_scroll_to_pos - scroll the address book data in pine's window such
 *			tthat the given "line" is at the top of the page.
 */
int
addr_scroll_to_pos(line)
    long line;
{
    return(addr_scroll_up(line - as.top_ent));
}


/*----------------------------------------------------------------------
     MSWin scroll callback.  Called during scroll message processing.
	     


  Args: cmd - what type of scroll operation.
	scroll_pos - parameter for operation.  
			used as position for SCROLL_TO operation.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
addr_scroll_callback (cmd, scroll_pos)
    int	 cmd;
    long scroll_pos;
{
    int paint = TRUE;
    
    switch (cmd) {
      case MSWIN_KEY_SCROLLUPLINE:
	paint = addr_scroll_down (1);
	break;

      case MSWIN_KEY_SCROLLDOWNLINE:
	paint = addr_scroll_up (1);
	break;

      case MSWIN_KEY_SCROLLUPPAGE:
	paint = addr_scroll_down (as.l_p_page);
	break;

      case MSWIN_KEY_SCROLLDOWNPAGE:
	paint = addr_scroll_up (as.l_p_page);
	break;

      case MSWIN_KEY_SCROLLTO:
	paint = addr_scroll_to_pos (scroll_pos);
	break;
    }

    if(paint)
      display_book(0, as.cur_row, -1, 1, (Pos *)NULL);

    return(paint);
}
#endif	/* _WINDOWS */
