#ident	"@(#)r5fonts:clients/mkfontscale/mkfontscale.c	1.12"

/*
 * This utility is used to generate a fonts.scale file.
 * It also updates the lp download directory.
 * It looks at the ascii Type 1 font file and generates a XLFD name
 * usage: mkfontscale dir1 dir2 ...
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <X11/Intrinsic.h>

#define MAX_STRING 128
#define MAX_PATH_STRING 256
#define STR_MATCH 0
#define STR_ALLOC 50
#define XtMalloc(c) (void *) malloc(c)

#define MAP_DIR          "/usr/share/lib/hostfontdir"
#define MAP_FILENAME     MAP_DIR "/map"

typedef struct _string_array_type {
  char **strs;
  int n_strs;
  int alloc_strs;
} string_array_type;

typedef struct lp_map_type {
  char **font;
  char **file;
  int n_strs;
  int alloc_strs;
} lp_map_type;

static string_array_type info;
static lp_map_type lp_map_table;
static String derive_ps;
static float DeciPointsPerInch = 72.27;
static int resolution[] = { 0 /* 75, 100 we might need this later */ };
static char *foundry_table[] = { "adobe", "ibm", "bitstream", "monotype", "linotype" };
static char *common_family[] = {
    "Courier-Bold",
    "Courier-BoldOblique",
    "Courier-Oblique",
    "Courier",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Helvetica-Oblique",
    "Helvetica",
    "Symbol",
    "Times-Bold",
    "Times-BoldItalic",
    "Times-Italic",
    "Times-Roman"
};

static char *set_width_table[] = {
    "condensed",
    "compressed",
    "narrow",
    "wide",
    "thin"
};

static char *add_style_table[] = {
    "lombardic capitals",
    "small capitals",
    "capitals",
    "expert",
    "alternate",
    "kursiv",
    "slanted",
    "old style figures",
    "old style",
    "dfr"
    };


/*
 * take out a substring from a string leaving the string without the substring
 */
static void
CondenseStr(String str, String sub_str)
{
    char buf[MAX_STRING], *p;

    if (p = strstr(str, sub_str)) {
	strncpy( buf, str, (p-str));
	strcpy( buf+(p-str), p+strlen(sub_str));
	strcpy(str, buf);
    }
} /* CondenseString */


/*
 * remove leading, trailing, and extra in between blanks.
 * this routine is half-ass but suffice for now
 */
static void
PolishStr(String str)
{
    int i, j=0;
    char tmp[MAX_STRING];

    for(i=0; str[i]; i++) {
	if (str[i] == ' ') {
	    if ((i == 0)              /* if first char */
		|| (str[i+1] == ' ')  /* if next char is a blank */
		|| (str[i+1] == 0))   /* if next char is eol */
		continue;
	}
	tmp[j++] = str[i];
    }
    tmp[j] = 0;  /* put in eol */

    strcpy(str, tmp);

} /* PolishStr */


static void
SubstituteChar(char *str, char old_ch, char new_ch)
{
    for (;*str;str++)
	if (*str == old_ch)
	    *str = new_ch;
}


void
InsertStringDB(info, str)
     string_array_type *info;
     char *str;
{
    /* if inserting into unallocated DB */
    if (info->alloc_strs == 0) {
	info->alloc_strs = STR_ALLOC;
	info->strs = (char **) malloc( sizeof(char *) * info->alloc_strs);
	info->n_strs = 0;
    }
	/* allocate more space if needed */
    else if (info->n_strs >= info->alloc_strs) {	
	info->alloc_strs += STR_ALLOC;
	info->strs = (char **) realloc( info->strs,
			      sizeof(char *) * info->alloc_strs);
    }
    info->strs[info->n_strs] = XtNewString(str);
    (info->n_strs)++;
} /* end of InsertStringDB */


void
InsertLpMap(lp_map_type *info,
	      char *font_name,
	      char *file_name)
{
    int i;

    /* if inserting into unallocated DB */
    if (info->alloc_strs == 0) {
	info->alloc_strs = STR_ALLOC;
	info->font = (char **) malloc( sizeof(char *) * info->alloc_strs);
	info->file = (char **) malloc( sizeof(char *) * info->alloc_strs);
	info->n_strs = 0;
    }
	/* allocate more space if needed */
    else if (info->n_strs >= info->alloc_strs) {	
	info->alloc_strs += STR_ALLOC;
	info->font = (char **) realloc( info->font,
			      sizeof(char *) * info->alloc_strs);
	info->file = (char **) realloc( info->file,
			      sizeof(char *) * info->alloc_strs);
    }

    /* 
     * make sure font name entry is unique 
     */
    for (i=0; i<info->n_strs; i++)
	if (strcmp(font_name, info->font[i]) == STR_MATCH)
	    return;
    
    info->font[info->n_strs] = XtNewString(font_name);
    info->file[info->n_strs] = XtNewString(file_name);
    (info->n_strs)++;
} /* end of InsertLpMap */


void
DeleteStringDB(info, i)
    string_array_type *info;
    int i;
{
    if (i >= info->n_strs)
	return;

    free(info->strs[i]);
    (info->n_strs)--;
    info->strs[i] = info->strs[info->n_strs];

} /* end of DeleteStringDB */


void
DeleteStringsDB(info)
     string_array_type *info;
{
    int i;

    for(i=0; i<info->n_strs; i++) {
	free(info->strs[i]);
    }
    info->n_strs = 0;
} /* end of DeleteStringsDB */


static String
LowercaseStr( str)
    String str;
{
    int i;
  
    for(i=0; str[i]; i++)
	str[i] = tolower(str[i]);
    return str;
} /* end of LowercaseStr */


/* skip white spaces, returns TRUE if not end of line */
static Boolean
SkipSpace( line)
    char **line;
{
    char *ptr = *line;

    /* while not end of string do */
    for (; *ptr; ptr++)
	/* if not white space char then break */
	if (!isspace(*ptr))
	    break;
    
    *line = ptr;
    if (*ptr)
	return TRUE;
    else
	return FALSE;
} /* end of SkipSpace */




static Boolean
FontFileMatchRenderer( String file_name,int *type)
{
    static char body[MAX_STRING], suffix[MAX_STRING];

    if (sscanf(file_name, "%[^.].%s", body, suffix) != 2) {
	return FALSE;
    }

    LowercaseStr( suffix);
    if ((strcmp(suffix, "ps") != STR_MATCH) &&
	(strcmp(suffix, "pfb") != STR_MATCH) &&
	(strcmp(suffix, "pfa") != STR_MATCH))
	return FALSE;

    if (strcmp(suffix, "pfb") == STR_MATCH) *type = 1;
    else
    if (strcmp(suffix, "pfa") == STR_MATCH) *type = 2;
    else *type = 3;
    return TRUE;

} /* end of FontFileMatchRenderer */


static Boolean
WriteFile( String dir_name)
{
    char full_name[MAX_PATH_STRING];
    FILE *file;
    int i;

    sprintf (full_name, "%s/%s", dir_name, "fonts.scale");
    file = fopen (full_name, "w");
    if (!file)
    {
	return FALSE;
    }

    fprintf(file, "%d\n", info.n_strs);
    for (i = 0; i < info.n_strs; i++) {
	fprintf (file, "%s\n", info.strs[i]);
    }
    fclose (file);
    return TRUE;

} /* end of WriteFile */


static Boolean
GetXLFD( String dir_name, String file_name, int *type)
{
    char tmpname[MAX_PATH_STRING];
    char full_name[MAX_PATH_STRING];
    FILE *file;
    int i;
    static char buf[MAX_PATH_STRING];
    static char buf2[MAX_PATH_STRING];
    static char buf3[MAX_PATH_STRING];
    static char end_marker[]="currentdict end";
    static char foundry[MAX_STRING];
    static char family[MAX_STRING];
    static char font_name[MAX_STRING];
    static char weight[MAX_STRING];
    static char slant[MAX_STRING];
    static char set_width[MAX_STRING];
    static char add_style[MAX_STRING];
    static char pitch[MAX_STRING];
    static char registry[MAX_STRING];
    static char encoding[MAX_STRING];
    static char orig_encoding[MAX_STRING];
    char *p, *buf_p;
    int size, pixel_size;

    *foundry = *family = *weight = *slant = *set_width = *add_style = 0;
    *pitch = *registry = *encoding = *font_name = 0;
    
    sprintf (full_name, "%s/%s", dir_name, file_name);
    tmpname[0] = 0;
    if (*type == 1) /* pfb file */ {
		/* convert pfb file to pfa and read that file to
			get the xlfd name */

	strcpy(tmpname, "/tmp/");
	strcat(tmpname, file_name);
	BinaryToAscii(full_name, tmpname, 1);
	strcpy(full_name, tmpname);
	}
    file = fopen (full_name, "r");
    if (!file)
    {
	return FALSE;
    }

    while (fgets(buf, sizeof(buf), file) != NULL) {
	buf_p = buf;
	SkipSpace(&buf_p);
	if (strncmp(buf_p, end_marker, sizeof(end_marker)) == STR_MATCH)
	    break;
	if (*weight == 0) {
	    if (sscanf( buf_p, "/Weight ( %[^)]", buf2) == 1) {
		strcpy( weight, buf2);
		continue;
	    }
	}
 	if (*pitch == 0) {
	    if (sscanf( buf_p, "/isFixedPitch %[^ ]", buf2) == 1) {
		if (strcmp( buf2, "true") == STR_MATCH)
		    strcpy(pitch, "m");
		else
		    strcpy(pitch, "p");
		continue;
	    }
	}
 	if (*foundry == 0) {
	    if (sscanf( buf_p, "/Notice %[^\n]", buf2) == 1) {
		LowercaseStr(buf2);
		for (i=0; i<XtNumber(foundry_table); i++)
		    if (strstr(buf2, foundry_table[i])) {
			strcpy(foundry, foundry_table[i]);
			break;
		    }			
		continue;
	    }
	}
	if (*family == 0) {
	    if (sscanf( buf_p, "/FamilyName ( %[^)]", buf2) == 1) {
		strcpy(family, buf2);
		continue;
	    }
	}
	if (*font_name == 0) {
	    if (sscanf( buf_p, "/FontName /%[^ ]", buf2) == 1) {
		strcpy(font_name, buf2);
	    }
	}

	if (*slant == 0) {
	    if (sscanf( buf_p, "/FullName ( %[^)]", buf2) == 1) {
		strcpy(slant, buf2);
		continue;
	    }
	}
	if (*registry == 0) {
	    if (sscanf( buf_p, "/Encoding %[^ ]", buf2) == 1) {
		if (strcmp( buf2, "StandardEncoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "1");
		} else  
		if (strcmp( buf2, "ISOLatin2Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "2");
		} else 
		if (strcmp( buf2, "ISOLatin3Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "3");
		} else 
		if (strcmp( buf2, "ISOLatin4Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "4");
		} else 
		if (strcmp( buf2, "ISOLatin5Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "5");
		} else 
		if (strcmp( buf2, "ISOLatinCyrillicEncoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "5");
		} else 
		if (strcmp( buf2, "ISOLatin6Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "6");
		} else 
		if (strcmp( buf2, "ISOLatinArabicEncoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "6");
		} else 
		if (strcmp( buf2, "ISOLatin7Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "7");
		} else 
		if (strcmp( buf2, "ISOLatinGreekEncoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "7");
		} else 
		if (strcmp( buf2, "ISOLatin8Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "8");
		} else 
		if (strcmp( buf2, "ISOLatinHebrewEncoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "8");
		} else 
		if (strcmp( buf2, "ISOLatin9Encoding") == STR_MATCH) {
		    strcpy(registry, "iso8859");
		    strcpy(encoding, "9");
		} else  {
		    strcpy(registry, "adobe");
		    strcpy(encoding, "fontspecific");
		}
		continue;
	    }
	}
    }
    fclose (file);
    if (tmpname) unlink(tmpname);
    /* if bogus file then dont generate XLFD entry */
    if (*family == 0) {
	return FALSE;
    }
    
    SubstituteChar(family, '-', ' ');

    /*
     * take out FamilyName and Weight from FullName (buf2)
     */
    strcpy(buf2,slant);
    CondenseStr(buf2, family);
    CondenseStr(buf2, weight);
    LowercaseStr(buf2);

    /* get slant */
    strcpy(slant, "r");
    if (strstr( buf2, "ita")) /* italic */
	strcpy(slant, "i");
    else if (strstr(buf2, "obl"))/*oblique*/
	strcpy(slant, "o");
		
    /* get set_width */
    if (strstr(buf2, "ultra"))
	strcpy(set_width, "ultra ");
    if (strstr(buf2, "extra"))
	strcpy(set_width, "extra ");
		
    /*
     * concatenate set_width
     */
    for(i=0; i<XtNumber(set_width_table); i++) {
	if (strstr(buf2, set_width_table[i])) {
	    strcat(set_width, set_width_table[i]);
	    break;
	}
    }    
    if (*set_width == 0)
	strcpy(set_width,"normal");
    PolishStr(set_width);
    
    /*
     * get add_style
     */
    for(i=0; i<XtNumber(add_style_table); i++) {
	if (strstr(buf2, add_style_table[i])) {
	    strcpy(add_style, add_style_table[i]);
	    break;
	}
    }    

    strcpy(orig_encoding, encoding);
    if (*foundry == 0)
	strcpy(foundry, "unknown");
    for(i=0; i<XtNumber(resolution); i++) {

	/* insert in lp map file */
	if (i==0) {
	    Boolean do_map = TRUE;
	    int j;

	    for (j=0; j<XtNumber(common_family); j++)
		if (strcmp(font_name, common_family[j])==STR_MATCH) {
		    do_map = FALSE;
		    break;
		}
	    if (do_map) {
		sprintf(buf, "%s/%s", dir_name, file_name);
		realpath(buf, buf2);
		InsertLpMap(&lp_map_table, font_name, buf2);
	    }
	}
	strcpy(encoding, orig_encoding);
	for (;;) {
	    sprintf(buf, "%s -%s-%s-%s-%s-%s-%s-0-0-%d-%d-%s-0-%s-%s",
		    file_name, foundry, family, weight, slant, set_width,
		    add_style, resolution[i], resolution[i], pitch,
		    registry, encoding);
	    InsertStringDB( &info, buf);

	    /* do derived instances */
	    for(p = derive_ps; p; p++) {
		if (sscanf( p, "%d", &size) != 1)
		    break;
		pixel_size = ((resolution[i] * size) / DeciPointsPerInch);
		sprintf(buf, "%s -%s-%s-%s-%s-%s-%s-%d-%d0-%d-%d-%s-0-%s-%s",
		    file_name, foundry, family, weight, slant, set_width,
		    add_style, pixel_size, size,
		    resolution[i], resolution[i], pitch,
		    registry, encoding);
		InsertStringDB( &info, buf);
		p = strchr(p, ',');
		if (p == NULL)
		    break;
	    }
	
	    if (*encoding == '1')
		strcpy(encoding, "adobe");
	    else
		break;
	}
    } /* for i */
    return TRUE;

} /* end of GetXLFD */


static Boolean
DoDirectory(dir_name)
    char	*dir_name;
{
    Boolean		status;
    DIR			*dirp;
    int			type;
    struct dirent	*direntp;

    if ((dirp = opendir (dir_name)) == NULL) {
	fprintf(stderr,"no such directory %s\n",dir_name);
	return FALSE;
	}
    DeleteStringsDB(&info);
    while ((direntp = readdir (dirp)) != NULL) {
	if (FontFileMatchRenderer (direntp->d_name, &type))
	    GetXLFD( dir_name, direntp->d_name, &type);
    }
    closedir(dirp);

    return WriteFile( dir_name);

} /* end of DoDirectory */


/* find first  white space, returns TRUE if not end of line */
static Boolean
FindSpace( line)
    char **line;
{
    char *ptr = *line;

    /* while not end of string do */
    for (; *ptr; ptr++)
	/* if white space char then break */
	if (isspace(*ptr))
	    break;
    
    *line = ptr;
    if (*ptr)
	return TRUE;
    else
	return FALSE;
} /* end of FindSpace */


/* 
 * This routine makes sure each entry in the map file is valid. If it
 * can't find the font file, it will remove the entry.
 * Only absolute path font files are checked.
 */
static void
CleanMap()
{
    FILE *file;
    char buf[MAX_PATH_STRING], font_file[MAX_PATH_STRING];
    char font_name[MAX_PATH_STRING];
    char* p, *start;
    struct stat statb;
    int str_len;

    if ((file = fopen(MAP_FILENAME, "r")) == NULL) {
	/* if map file doesn't exist, we don't have to clean anything */
	return;
    }

    while (fgets(buf, sizeof(buf), file) != NULL) {
	/* init */
	*font_file = 0;
	strcpy(font_name, buf);
	if ((str_len = strlen(font_name)) > 0)
	    font_name[str_len-1] = 0; /* remove new-line char */

	p = buf;
	if (SkipSpace(&p) == FALSE)
	    goto insert_table;

	if (*p == '%')
	    /* got a comment, parse next line */
	    goto insert_table;

	/*
	 * get font name
	 */
	start = p;
	if (FindSpace(&p) == FALSE) goto insert_table;
	strncpy(font_name, start, p-start);
	font_name[p-start] = 0; /* terminator */

	if (SkipSpace(&p) == FALSE) goto insert_table;
	start = p;

	/* don't worry about relative pathnames */
	if (*start != '/') goto insert_table;

	FindSpace(&p);
	strncpy(font_file, start, p-start);
	font_file[p-start] = 0; /* terminator */

	/* if font file doesn't exist, don't put it in map_table */
	if (stat(font_file, &statb) == -1)
	    continue;

insert_table:
	InsertLpMap(&lp_map_table, font_name, font_file);
    }
    fclose(file);
} /* end of CleanMap */


/* 
 * write out to map file
 */
static void
NewMap()
{
    FILE *file;
    int i;

    if ((file = fopen(MAP_FILENAME, "w")) == NULL) {
	fprintf(stderr,
		"mkfontscale: can't open %s for writing\n", MAP_FILENAME);
	return;
    }

    for(i=0; i < lp_map_table.n_strs; i++)
	fprintf(file, "%s\t%s\n", lp_map_table.font[i], lp_map_table.file[i]);

    fclose(file);
} /* end of NewMap */


main (argc, argv)
    int argc;
    char **argv;
{
    int i;

    derive_ps = (String) getenv("DERIVED_INSTANCE_PS");
    if (argc == 1)
    {
	if (!DoDirectory(getcwd(NULL, MAXPATHLEN)))
	{
	    fprintf (stderr, "%s: failed to create directory in %s\n",
		     argv[0], getcwd(NULL, MAXPATHLEN));
	    exit (1);
	}
    }
    else
	for (i = 1; i < argc; i++) {
	    if (!DoDirectory(argv[i]))
	    {
		fprintf (stderr, "%s: failed to create directory in %s\n",
			 argv[0], argv[i]);
		exit (1);
	    }
 	}

    /*
     * remove entries in map file that are bogus
     * and write out new map file 
     */
    mkdir(MAP_DIR, 0755);
    CleanMap();
    NewMap();
    exit(0);	
}
