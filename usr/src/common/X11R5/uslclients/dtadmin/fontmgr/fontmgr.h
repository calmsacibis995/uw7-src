#ident	"@(#)dtadmin:fontmgr/fontmgr.h	1.12.1.4"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       fontmgr.h
 */

#include <Xol/array.h>
#include <message.h>

#define AFM_DIR "afm"


#define MAX_PATH_STRING 512
#define MAX_STRING 128
#define STR_MATCH 0
#define STR_ALLOC 50
#define MIN_MATCH 5

#define FIELD_COUNT 14
#define DELIM '-'
#define PROPORTIONAL_SPACING 'p'
#define NUM_FAMILY_ITEMS 10
#define NUM_LIST_ITEMS  4

#define FAMILY 0
#define LOOK 1
#define POINT 2

#define MIN_PS_VALUE  6
#define MAX_PS_VALUE  100

#define DEFAULT_CACHE_SIZE "800"
#define DEFAULT_POINT_SIZE "12"
#define DEFAULT_PS_VALUE    12

#define DEFAULT_FAMILY "Lucida"
#define DEFAULT_LOOK   "Bold   "

/* this must be updated when catalog_menu_item[] is updated */
enum prop_constant { E_GENERAL_PROP, E_ATM_PROP, E_FOLIO_PROP,
			 E_SPEEDO_PROP, E_MAX_PROP };

/*
 * user defined types
 */
typedef struct _string_array_type {
  char **strs;
  int n_strs;
  int alloc_strs;
} string_array_type;

typedef struct _config_type {
    char keyword[MAX_STRING];
    char default_value[MAX_STRING];
    int  match_len;
    char value[MAX_PATH_STRING];
    Boolean replaced;
    Boolean renderer_exist;
} config_type;

#define PFB_TYPE	1
#define PFA_TYPE	2
#define AFM_TYPE	3	
#define INF_TYPE	4


typedef struct add_db {
    char file_name[MAX_STRING];
    int    pfa_disk;
    int    pfb_disk;
    int    afm_disk;
    int    inf_disk;
    int    fontname_found;
    int    missing_afm;
    int	   selected;
    int	   done;
} add_db;

typedef struct _add_type {             /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    string_array_type *font_name; 
    string_array_type *disk_label;
    char device[PATH_MAX];
    Widget popup;
    Widget prompt;
    Widget font_list;
    Widget gauge;
    add_db *db;
    int font_cnt;
    int select_cnt;
    int slider_val;
    int disk_num;
    int cfg_found;
    int fontfiles_found;
    Boolean adobe_foundry;
} add_type;

typedef struct _prop_type {
    config_type *general;
    config_type *cur_cfg;
    Widget popup;
    int cur_prop;
    int cur_parse_section;
    char filename[MAX_PATH_STRING];
} prop_type;

typedef struct _font_type {
    char * xlfd_name;
    char *calc_ps;
    Boolean bitmap;
} font_type;

typedef struct _family_info {
    char * name;
    _OlArrayType(StyleArray) * l;
} family_info;
_OlArrayStruct(family_info, FamilyArray);

typedef struct _style_info {
    char * style_name;          /* XtNlabel */
    _OlArrayType(PSArray) * l;
} style_info;
_OlArrayStruct(style_info, StyleArray);

typedef struct _ps_info {
    char * ps;
    font_type *l;              /* XtNuserData */
} ps_info;
_OlArrayStruct(ps_info, PSArray);

typedef struct _delete_type {             /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    string_array_type *font_name;
    string_array_type *xlfd;
    string_array_type *selected_xlfd;
    string_array_type *file_name;
    string_array_type *selected_dir;
    Widget popup;
    Widget font_list;
    Widget confirm;
    Boolean bitmap;
} delete_type;

typedef struct _view_type {              /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    _OlArrayType(FamilyArray) *family_array;
    Widget form;
    Widget upper;
    Widget family_caption;
    Widget family_exclusive;
    Widget style_caption;
    Widget style_exclusive;
    Widget size_caption;
    Widget size_exclusive;
    Widget sample_text;
    Widget footer_text;
    Widget size_window;
    Widget ps_text;
    int font_state[3];
    String outline_xlfd;
    Boolean bitmap;
    char cur_family[MAX_STRING];
    char cur_style[MAX_STRING];
    int  cur_size;
    char cur_xlfd[MAX_PATH_STRING];
    char prev_xlfd[MAX_PATH_STRING];
    XtIntervalId timer_id;
    _OlArrayType(PSArray) * ps_array;
    char display_xlfd[MAX_PATH_STRING];
    char postscript_name[MAX_PATH_STRING];
    int calc_size;
    int resx;
    int resy;
    int dimx;
    int dimy;

} view_type;


typedef struct _weight_type {
    String code;
    int value;
} weight_type;

typedef struct _slant_type {
    String code;
    String translation;
} slant_type;

typedef struct _xlfd_type {
    char family[MAX_STRING];
    char weight[40];
    char slant[40];
    char set_width[40];
    char add_style[40];
    char size[40];
    char spacing[40];
    Boolean bitmap;
    char resolutionX[40];
    char resolutionY[40];
    char average_width[40];
    char charset[40];
    char encoding[40];
    char pixel[40];
    char orig_size[40];
} xlfd_type;

