#pragma comment(exestr, "@(#) infparse.c 25.3 94/01/04 ")
/*
 *	Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/*
 *      MODIFICATION HISTORY
 *
 *	Created	scol!rroscoe	Thu April 11 1996	LTD-245-2202
 */
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>		
#include <errno.h>


#define VENDOR_FILE "vendor"
#define DBFORMAT "dbformat"

/*
 * Sections
 */
#define VERSION "Version"
#define STRINGS "Strings"
#define MANUFACTURER "Manufacturer"

/*
 * Fields
 */
#define SIGNATURE "Signature"
#define CLASS "Class"
#define PROVIDER "Provider"
#define ADDREG "AddReg"
#define HKR "HKR"
/*
 * Other defs
 */
#define CHICAGO "\"$CHICAGO$\""
#define MODEM "Modem"

FILE *ifile;
char *ConfigProvider = NULL;
int debug = 0;
int warning = 0;
char *dbformat;
char *dbasedir;

char *notset = "notset";


/*
 * Structures used to store the parsed configuration
 */

struct hkr {
	struct hkr	*hkr_next;	/* Next def */
	char *hkr_path;		/* Sub path within the modems software key */
	char *hkr_key;		/* Name of the key */
	char *hkr_format;	/* Data type, 0 or NULL implies string,
				   value 1 implies hex string */
	char *hkr_value;		/* Value list, comma separated */
};

struct addreg {
	char 		*ar_name;
	struct hkr 	*ar_hkr;
};

struct install {
	struct install	*inst_next;	/* Next addreg section */
	struct addreg	*inst_addreg;	/* addreg section */
};

struct devdetails {
	struct devdetails	*devd_next;	/* Next compatible device */
	char		*devd_id;   	/* ID */
	int		devd_rank;	/* Rank of device */
	struct install 	*devd_install;	/* Pointer to install section */
};

struct device {
	struct device	*dev_next;	/* Next device in list */
	char		*dev_name;	/* Devices name */
	struct devdetails	*dev_details;	/* List of device details */
};

struct manuf {
	struct manuf	*man_next;	/* Next manufacturer */
	char 		*man_name;	/* Manufacturer name */
	struct device	*man_dev;	/* This manuf list of devices */
};

struct manuf *manufacturer = NULL;

/*
 * Structures store the read configuration info
*/
struct def {
	struct def 	*def_next;	/* Next def in list */
	short		def_type;	/* Assignment or definition */
	char 		*def_name;	/* Name of variable assigned */
	char 		*def_value;	/* Value assigned or defined */
};

enum { ASSIGNMENT, DEFINITION };

struct section {
	struct section	*sect_next;	/* Next in list */
	char 		*sect_name;	/* Name of section */
	struct def 	*sect_defs;	/* Section definitions */
	union {				/* Pointer to parsed defintion */
		struct manuf *a_manuf;
		struct device *a_device;
		struct install *a_install;
		struct addreg *a_addreg;
	} a;
};

#define sect_manuf a.a_manuf
#define sect_device a.a_device
#define sect_install a.a_install
#define sect_addreg a.a_addreg

struct section *section_h = NULL; /* Ptr to first section in list */
struct section *strings_sect = NULL; /* Used to get to sting section quickly */

/*
 *  This holds the format of the device output files
 */
struct specent {
	struct specent	*se_next;	/* Next entry */
	char 		*se_fname;	/* Field name */
	struct hkr	*se_hkr;	/* The data */
};

struct specent *spec = NULL, *spec_tail = NULL;

/*
 * Allocate a section header
 */
struct section *
alloc_sect(char *name) 
{
	struct section *sect = (struct section *)malloc(sizeof(struct section));

	if (!sect) {
		printf("Out of memory - malloc() - alloc_sect()\n");
		exit(1);
	}

	sect->sect_name = (char *)malloc(strlen(name) + 1);
	if (!sect->sect_name) {
		printf("Out of memory - malloc() - alloc_sect()\n");
		exit(1);
	}

	strcpy(sect->sect_name, name);
	sect->sect_defs = NULL;
	sect->sect_next = section_h;
	sect->sect_manuf = NULL;
	section_h = sect;
	return(sect);
}

struct def *
alloc_def(short type, char *name, char *value)
{
	struct def *def = (struct def *)malloc(sizeof(struct def));

	if (!def) {
		printf("Out of memory - malloc() - alloc_def()\n");
		exit(1);
	}

	def->def_value = (char *)malloc(strlen(value) + 1);
	if (!def->def_value) {
		printf("Out of memory - malloc() - alloc_def()\n");
		exit(1);
	}

	if (type == ASSIGNMENT) {
		def->def_name = (char *)malloc(strlen(name) + 1);
		if (!def->def_name) {
			printf("Out of memory - malloc() - alloc_def()\n");
			exit(1);
		}
		strcpy(def->def_name, name);
	}

	def->def_next = section_h->sect_defs;
	section_h->sect_defs = def;
	def->def_type = type;
	strcpy(def->def_value, value);
	return(def);
}

/*
 * Trim white space from the rhs of a line
 */
trimright(char *line)
{
	char *p;

	p = line + strlen(line);
	while (p >= line && isspace(*(p - 1)))
		p--;

	*p = 0;
}

/*
 * Display the HKR parts of an addreg section
 */
dump_hkr(struct hkr *hkr, char *pre)
{


	if (debug < 2) {
		/*		printf("					hkr at %x\n", hkr);*/
		return;
	}

	while (hkr) {
		printf("%spath = %s\n%skey = %s\n%sformat = %s\n%svalue =%s\n\n",
		       pre,hkr->hkr_path,
		       pre, hkr->hkr_key,
		       pre, hkr->hkr_format,
		       pre, hkr->hkr_value
		       );
		hkr = hkr->hkr_next;
	}
}
/*
 * Display the reg entry list
 */
dump_reg(struct addreg *s)
{
	printf("			Addreg : %s\n", s->ar_name);
	dump_hkr(s->ar_hkr,"				");
}

/*
 * Display the install section list
 */
dump_inst(struct install *s)
{
	while (s) {
		printf("		Install : \n");
		dump_reg(s->inst_addreg);
		s = s->inst_next;
	}
}

/*
 * Display a device list
 */
dump_dev(struct device *s)
{
	struct devdetails *dev;

	while (s) {
		printf("	Dev : %s\n", s->dev_name);

		dev = s->dev_details;
		while (dev) {
			printf("		%s	%d\n",
			       dev->devd_id, dev->devd_rank);

			dump_inst(dev->devd_install);
			dev = dev->devd_next;
		}
		s = s->dev_next;
	}
}

/*
 * Display the parsed config
 */
dump_manuf()
{
	struct manuf *s = manufacturer;

	while (s) {
		printf("Manufacturer%s\n", s->man_name);
		dump_dev(s->man_dev);
		s = s->man_next;
	} 
}

/*
 * Display read configuration
 */
dump_defs(struct section *sect)
{
	struct def *def;

	def = sect->sect_defs;
	while (def) {

		switch(def->def_type) {
		case ASSIGNMENT:
			printf("	ASS %s = %s\n", def->def_name, def->def_value);
			break;

		case DEFINITION:
			printf("	DEF %s\n", def->def_value);
			break;
		}

		def = def->def_next;
	}
}

dump_sects()
{
	struct section *s;

	s = section_h;
	while (s) {
		printf("Section : %s\n", s->sect_name);
		dump_defs(s);
		s = s->sect_next;
	}

}

/*
 * Construct a HKR structure containing formatting data
 */
struct hkr *
process_spec_format(char *s, char *e)
{
	struct hkr *hkr;
	int len = e - s;

	/*printf("process_spec_format:  '%.*s'\n", len, s);*/

	hkr = (struct hkr *)malloc(sizeof(struct hkr));
	if (!hkr) {
		printf("Cannot malloc for struct hkr !\n");
		exit(1);
	}

	hkr->hkr_next = NULL;
	hkr->hkr_format = NULL;
	hkr->hkr_value = NULL;
	hkr->hkr_path = NULL;
	hkr->hkr_key = NULL;

	hkr->hkr_format = (char *)malloc(len + 1);
	if (!hkr->hkr_format) {
		printf("Cannot malloc for hkr_format\n");
		exit(1);
	}

	memcpy(hkr->hkr_format, s, len);
	*(hkr->hkr_format + len) = 0;

	/*printf("hkr_format = '%s'\n", hkr->hkr_format);*/
	return(hkr);
}

/*
 * Construct a HKR structure containing a path/key entry
 */
struct hkr *
process_spec_hkr(char *s, char *e)
{
	struct hkr *hkr;
	char *p;
	int len = e - s;

	/*printf("process_spec_hkr:  '%.*s'\n", len, s);*/

	hkr = (struct hkr *)malloc(sizeof(struct hkr));
	if (!hkr) {
		printf("Cannot malloc for struct hkr !\n");
		exit(1);
	}

	hkr->hkr_next = NULL;
	hkr->hkr_format = NULL;
	hkr->hkr_value = NULL;
	hkr->hkr_path = NULL;
	hkr->hkr_key = NULL;

	/* Find the comma */
	p = s;
	while (p < e && *p != ',')
		p++;

	if (*p != ',') {
		printf("Bad format in %.*s\n", len, s);
		exit(1);
	}

	/* Length of path */

	len = p - s;

	hkr->hkr_path = (char *)malloc(len + 1);
	if (!hkr->hkr_path) {
		printf("Cannot malloc for hkr_path\n");
		exit(1);
	}

	memcpy(hkr->hkr_path, s, len);
	*(hkr->hkr_path + len) = 0;

	p++;	/* Over the comma */

	/*
	 * Skip any white space
	 */
	while (p < e && isspace(*p))
		p++;

	if (p == e) {
		printf("Bad format in %.*s\n", len, s);
		exit(1);
	}
		
	s = p;

	/* Length of key */

	len = e - p;

	hkr->hkr_key = (char *)malloc(len + 1);
	if (!hkr->hkr_key) {
		printf("Cannot malloc for hkr_key \n");
		exit(1);
	}

	memcpy(hkr->hkr_key, s, len);
	*(hkr->hkr_key + len) = 0;

	/*printf("hkr_path = '%s'\n", hkr->hkr_path);
	printf("hkr_key = '%s'\n", hkr->hkr_key);*/

	return(hkr);
}

/*
 * Parse the spec line ... add to the spec list
 * 
 * Lines are in the format:
 * Field_name = %path, key%  [%path, key% ....]
 * 
 * Formatting after the = is significant.
 */
process_spec_line(char *line)
{
	char *p;
	struct specent *sp;
	struct hkr *hkr = NULL, *hkr_tail = NULL;

	trimright(line);
	/*printf("process_spec_line: line %s\n", line);*/

	sp = (struct specent *)malloc(sizeof(struct specent));
	if (!sp) {
		printf("Failed to malloc() for specent\n");
		exit(1);
	}
	sp->se_hkr = NULL;
	sp->se_next = NULL;

	/* Find the equals sign */

	p = line;
	while (*p && *p != '=')
		p++;

	if (*p == '=')
		*p = 0;
	else {
		printf("Error in %s spec file\n", dbformat);
		printf("Line : %s\n", line);
		exit(1);
	}

	trimright(line);

	/* Now we have the field name */

	sp->se_fname = strdup(line);

	p++;
	line = p;

	/* Find the definition list */
	
	while (*p) {

		/* Search to a % .. */
		while (*p && *p != '%')
			p++;

		if (p != line) {

			/* Found formatting - Create formatting entry */

			hkr = process_spec_format(line, p);
			if (!hkr_tail) {
				sp->se_hkr = hkr;
				hkr_tail = hkr;
			} else {
				hkr_tail->hkr_next = hkr;
				hkr_tail = hkr;
			}

			line = p;
			continue;
		}
		
		/*
		 * Found a HKR entry ...increment past the % and 
		 * find the closing % 
		 */

		line++;
		p++;

		while (*p && *p != '%')
			p++;

		if (*p != '%') {
			printf("Error in %s spec file - unbalances %\n", dbformat);
			exit(1);
		}
		
		/* Got the complete spec */

		hkr = process_spec_hkr(line, p);
		if (!hkr_tail) {
			sp->se_hkr = hkr;
			hkr_tail = hkr;
		} else {
			hkr_tail->hkr_next = hkr;
			hkr_tail = hkr;
		}

		p++;
		line = p;
	}

	/* Add this to the spec list */

	if (!spec_tail) {
		spec_tail = sp;
		spec = sp;
	} else {
		spec_tail->se_next = sp;
		spec_tail = sp;
	}
}

/*
 * Given a filled HRK from the INF file, update the spec values if we need it
 */
spec_update(struct hkr *ihkr)
{
	struct specent *sp = spec;
	struct hkr *hkr;

	while (sp) {
		hkr = sp->se_hkr;
		while (hkr) {
			if (strcasecmp(hkr->hkr_path, ihkr->hkr_path) == 0 &&
			    strcasecmp(hkr->hkr_key, ihkr->hkr_key) == 0) {
				if (debug >4)
					printf("Update '%s, %s' -> %s\n",
					       hkr->hkr_path,
					       hkr->hkr_key,
					       ihkr->hkr_value);
				hkr->hkr_value = ihkr->hkr_value;
			}
			hkr = hkr->hkr_next;
		}
		sp = sp->se_next;
	}
}


/*
 * Display the dbase  spec
 */
dump_spec()
{
	struct specent *sp = spec;

	while (sp) {
		printf("%s = \n", sp->se_fname);
		dump_hkr(sp->se_hkr, "	");
		sp = sp->se_next;
	}
}

/*
 * Reset all the value pointers in the spec to NULL
 * Called before updating the spec for a particular driver
 */
reset_spec()
{
	struct specent *sp = spec;
	struct hkr *hkr;

	while (sp) {
		hkr = sp->se_hkr;
		while (hkr) {
			hkr->hkr_value = NULL;
			hkr = hkr->hkr_next;
		}
		sp = sp->se_next;
	}
}

/*
 * Parse the output file specification
 */
parse_spec()
{
	FILE *fp;
	char line[1024];

	fp = fopen(dbformat, "r");

	if (!fp) {
		printf("Couldn't open file %s\n", dbformat);
		perror("fopen");
		exit(1);
	}

	/*
	 * Read the spec config file
	 */
	while (fgets(line, 1024, fp)) 
		process_spec_line(line);

	if (debug > 0)
		dump_spec();
}

main(int argc, char *argv[])
{
	char line[1024];
	extern char *optarg;
	extern int optind, optopt;
	int c;

	/* Check args */
	
	while ((c = getopt(argc, argv, (const char *)"d:")) != -1) {
		switch (c) {
		case 'd':
			debug = atoi(optarg);
			break;

		case ':':        /* argument missing */
		case '?':
		default:
			fprintf(stderr,
				"Usage: %s [-d debug_level] dbformat_file  dbase_dir inf_file\n", argv[0]);
			exit(1);
		}
	}

	if (optind + 3 > argc) {
		fprintf(stderr,
			"Usage: %s [-d debug_level] dbformat_file dbase_dir inf_file\n", argv[0]);
		exit(1);
	}

	/*
	 * Get dbase format file
	 */

	dbformat = argv[optind++];
	dbasedir = argv[optind++];

	if (debug)
		printf("Debug = %d\n", debug);

 	/*
	 * Read the output specification
	 */

	parse_spec();

	/* Initialise section name */

	ifile = fopen(argv[optind], "r");

	if (!ifile) {
		perror("fopen");
		exit(1);
	}

	/*
	 * Read the config file
	 */
	while (fgets(line, 1024, ifile)) 
		process_line(line);

	/*
	 * Now parse the config
	 */

	parse_config();

	/*
	 * Now create dialer files and database
	 */

	create_dir(dbasedir);

	gen_manufacturer();

	if (warning) {
		printf("Completed with %d warnings.\n", warning);
	}

	exit(warning);
}

/*
 * Parse the section header
 */
parse_header(char *line)
{
	char s_name[128];	/* section name */
	char *s;

	line++;

	s = s_name;
	*s = 0;

	while (*line && *line != ']')
		*s++ = *line++;
	*s = 0;

	alloc_sect(s_name);
/*	printf("Section = %s\n", s_name);*/
}

/*
 * Parse a line
 */
process_line(char *line)
{
	static char bline[2048];
	static int building = 0;
	static char *b = bline;

	char *p, *e;

/*	printf("Line: %s\n", line);*/

	p = line;

	/* Skip any white space */

	while(*p && isspace(*p))
		p++;

	if (!*p) {

		/* Blank line */

		if (building) {
			/* End of continuation */
			p = bline;
			building = 0;
			goto parse;
		} else
			return;
	}

	/* Is this line complete .. or is there a continuation mark ? */

	e = p + strlen(p) - 1;
	while (e >= p && isspace(*e))
		e--;

	if (*e == '\\') {
		*e = 0;
		/*		printf("Continued line !! \n%s\n", p);*/
		strcpy(b, p);
		b += strlen(p);
		building = 1;
		return;
	}

	/* Check if this is the end of a continued line */

	if (building) {
		/*		printf("End of continued line !!\n%s\n", p);*/
		strcpy(b, p);
		building = 0;
		b = bline;
		p = bline;
	}

parse:
	/* Check for section header */

	if (*p == '[') {
		/* Get section name */
		parse_header(p);
		return;
	}

	/* This is not a section header ... so parse the line */

	parse_def(p);
}


/*
 * Parse a definition
 */
parse_def(char *line)
{
	char *p = line;
	short type = DEFINITION;
	char *vname = NULL;

	/* Scan for a non-quoted equals sign, if found then
	 * def type is an assignment, otherwise it is a definition
	 */

	while (*p) {

		if (*p == '"') {

			p++;
			/* Scan to closing quote */

			while (*p && *p != '"')
				p++;

			if (!*p) {
				printf("WARNING: Unbalanced quotes, line is \n%s\n",
				       line);
				printf("Line skipped ..\n");
				warning++;
				return;
			}

		} else if (*p == '=') {

			if (type == ASSIGNMENT) {
				p++;
				continue;
			}

			type = ASSIGNMENT;

			*p = 0;
			p++;

			vname = line;
			trimright(vname);
/*			printf("Var is '%s'\n", vname);a*/

			while(*p && isspace(*p))
				p++;
			line = p;
			continue;

		} else if (*p == ';') {

			/* We have a comment in the line */
/*			printf("Line has comment\n");*/
			*p = 0;
			break;
		}
		p++;
	}

	trimright(line);

	/* Scan to eol or unquoted semicolon */

	if (type == DEFINITION && *line == 0) {
/*		printf("Blank line !!!\n");*/
		return;
	}

/*	printf("Def is '%s'\n", line);*/

	alloc_def(type, vname, line);
}


/*
 * Find a named section 
 */
struct section *
find_sect(char *name)
{
	struct section *s = NULL;

/*	printf("find_sect: name %s\n", name);*/

	s = section_h;
	while (s) {
		if (strcasecmp(s->sect_name, name) == 0) {
/*			printf("find_sect: found\n");*/
			return(s);
		}
		s = s->sect_next;
	}
/*	printf("find_sect: NOT found\n");*/
	return(NULL);
}

/*
 * Return the value (in the named section) given a variable name
 */
char *
find_value(struct section *sect, char *name)
{
	struct def *d;

/*	printf("find_value: Sect %s, name %s\n", sect->sect_name, name);*/
	d = sect->sect_defs;

	while (d) {
		if (d->def_type == ASSIGNMENT && strcmp(d->def_name, name) == 0){
/*			printf("find_value: value = %s\n", d->def_value);*/
			return(d->def_value);
		}
		d = d->def_next;
	}
/*	printf("find_value: not found\n");*/
	return(NULL);
}

/*
 * Return the value in the Strings section given a variable name
 */
char *
find_string(char *name)
{
	struct def *d;

/*	printf("find_string: name %s\n", name);*/

	d = strings_sect->sect_defs;
	while (d) {
		if (d->def_type == ASSIGNMENT && strcmp(d->def_name, name) == 0){
/*			printf("find_string: value = %s\n", d->def_value);*/
			return(d->def_value);
		}
		d = d->def_next;
	}
/*	printf("find_string: not found\n");*/
	return(NULL);
}
	
/*
 * If a value is a String reference resolve it
 */
char *
resolve_value(char *v)
{
	char var[128];
	int len;

/*	printf("resolve_value: name %s\n", v);*/

	len = strlen(v);
	if (*v == '%' && *(v + len - 1)  == '%') {
/*		printf("resolve_value: %s needs resolving\n", v);*/
		strncpy(var, v+1, len - 2);
		var[len - 2] = 0;
		v = find_string(var);
	}
/*	printf("resolve_value: return %s\n", v);*/
	return(v);
}

/*
 * Validate that the config file is for us 
*/
validate_config()
{
	struct section *s;

	/* Find the Version section and check that we have a chicago
	 * INF file
	 */

	s = find_sect(VERSION);

	if (!s) {
		printf("Couldn't find the %s section\n", VERSION);
		exit(1);
	}

	if (strcasecmp(CHICAGO, find_value(s, SIGNATURE)) != 0) {
		printf("Bad Signature : %s\n", find_value(s, SIGNATURE));
		exit(1);
	}

	/* Check that our class is modem */
	
	if (strcasecmp(MODEM, find_value(s, CLASS)) != 0) {
		printf("Bad Signature : %s\n", find_value(s, CLASS));
		exit(1);
	}

	/* Find the strings section */

	strings_sect = find_sect(STRINGS);

	if (!strings_sect) {
		printf("Couldn't find the %s section\n", STRINGS);
		exit(1);
	}

	/* Get the provider */

	ConfigProvider = find_value(s, PROVIDER);
	if (!ConfigProvider) {
		printf("%s, %s field not defined\n", VERSION, PROVIDER);
		exit(1);
	}

	/* Resolve any strings if needed */

	ConfigProvider = resolve_value(ConfigProvider);
	if (!ConfigProvider) {
		printf("WARNING: %s, %s field incorrectly defined\n", VERSION, PROVIDER);
	}
}

/*
 * This is similar to srtok, but only takes on character as the token
 * seperator, and returns a null string if an empty field is found
 */
char *
mstrtok(char *s, char t)
{
	static char *p, *start;
	static char *nullstring = "";

	if (s)
		p = s;

	start = p;

	if (!*p)
		return(NULL);
	
	if (*p && *p == t) {
		p++;
		return(nullstring);
	}

	while (*p && *p != t)
		p++;

	if (*p) {
		*p = 0;
		p++;
	}
	return(start);
}
/*
 * Get the Registry info for a section
 *
 * [add-registry-section]
 * reg-root-string, [subkey], [value-name], [flag], [value]
 * [reg-root-string, [subkey], [value-name], [flag], [value]]
 *
 */
struct addreg *
addreg_section(char *section_name)
{
	struct addreg *ar;
	struct section *s;
	struct def *d;
	struct hkr *hkr, *hkr_head = NULL;
	char *start, *val, *p;

	if (debug > 4)
		printf("addreg_section: [%s]\n", section_name);

	s = find_sect(section_name);
	if (!s) {
		printf("addreg_section: Section %s not found\n", section_name);
		warning++;
		return(NULL);
	}

	if (s->sect_addreg)
		return(s->sect_addreg);

	ar = (struct addreg *)malloc(sizeof(struct addreg));
	if (!ar) {
		printf("Cannot malloc for struct addreg\n");
		exit(1);
	}
	ar->ar_hkr = NULL;
	ar->ar_name = strdup(section_name);

	d = s->sect_defs;
	if (!d) {
		printf("addreg_section: No %s definitions found\n", section_name);
		warning++;
		return(NULL);
	}

	while (d) {

		if (d->def_type != DEFINITION) {
			printf("Incorrect %s definition, expected definitions\n",
				section_name);
			printf("name = %s\nvalue=%s\n", d->def_name, d->def_value);
			exit(1);
		}

		val = strdup(d->def_value); 

		if (debug > 4)
			printf("addreg_section: val = %s\n", val);

		p = mstrtok(val, ',');
		if (strcasecmp(p, HKR) != 0) {
			d = d->def_next;
			continue;
		}

		hkr = (struct hkr *)malloc(sizeof(struct hkr));
		if (!hkr) {
			printf("Cannot malloc for struct hkr\n");
			exit(1);
		}

/*printf("Type ... %s\n", p);*/
		p = mstrtok(NULL, ',');
		while(*p && isspace(*p))
			p++;

		trimright(p);
		hkr->hkr_path = p;
/*printf("	path = %s\n", p);*/
		

		p = mstrtok(NULL, ',');
		while(*p && isspace(*p))
			p++;
		trimright(p);
		hkr->hkr_key = p;
/*printf("	key = %s\n", p);*/



		p = mstrtok(NULL, ',');
		while(*p && isspace(*p))
			p++;
		trimright(p);
		hkr->hkr_format = p;
/*printf("	format = %s\n", p);*/

		p = mstrtok(NULL, '\0');
		while(*p && isspace(*p))
			p++;

		trimright(p);
/* Remember that the value may have %% ... which maps to % */
		hkr->hkr_value = p;

/*printf("	value = %s\n", p);*/

		hkr->hkr_next = hkr_head;
		hkr_head = hkr;

		d = d->def_next;
	}
       	ar->ar_hkr = hkr_head;

	s->sect_addreg = ar;
	return(ar);
}

/*
 * Get the install section for a device
 *
 *
 * [install-section-name]
 * LogConfig = log-config-section-name[, log-config-section-name]...
 * Copyfiles=file-list-section[,<file-list-section>]...
 * Renfiles=file-list-section[,file-list-section]...
 * Delfiles=file-list-section[,file-list-section]...
 * UpdateInis=update-ini-section[,update-ini-section]...
 * UpdateIniFields=update-inifields-section[,update-inifields-section]...
 * AddReg=add-registry-section[,add-registry-section]...
 * DelReg=del-registry-section[,del-registry-section]...
 * Ini2Reg=ini-to-registry-section[,ini-to-registry-section]...
 * UpdateCfgSys=update-config-section
 * UpdateAutoBat=update-autoexec-section
 * Reboot | Restart
 *
 * We are interested in the AddReg fields
 *
 */
struct install *
install_section(char *section_name)
{
	struct section *s;
	char *addreg_sect, *start, *val;
	struct install *install, *install_head = NULL, *install_tail = NULL;

	if (debug > 4)
		printf("install_section: [%s]\n", section_name);

	s = find_sect(section_name);
	if (!s) {
		printf("WARNING: Section %s not found\n", section_name);
		warning++;
		return(NULL);
	}

	if (s->sect_install)
		return(s->sect_install);


	start = find_value(s, ADDREG);
	if (!start) {
		printf("WARNING: No %s field in section %s\n", ADDREG, section_name);
		warning++;
		return(NULL);
	}

	start = val = strdup(start);

	while (addreg_sect = strtok(start, ",")) {
		start = NULL;

		trimright(addreg_sect);
		while(*addreg_sect && isspace(*addreg_sect))
			addreg_sect++;

		if (debug > 4)
			printf("install_section : AddReg Section %s\n",
			       addreg_sect);
		
		install = (struct install *)malloc(sizeof(struct install));
		if (!install) {
			printf("Cannot malloc for struct install\n");
			exit(1);
		}

		install->inst_next = NULL;
		install->inst_addreg = addreg_section(addreg_sect);

		/* Chain this in */
		if (!install_head) {
			install_head = install;
			install_tail = install;
		} else {
			install_tail->inst_next = install;
			install_tail = install;
		}
	}
	free(val);

	s->sect_install = install_head;
	return(install_head);
}


/*
 * Parse a device entry, returning a device structure
 */
struct devdetails *
parse_device(char *devname, char *devinf)
{
	struct devdetails *devd;
	char *install_sect, *p, *last = NULL, *val;
	int commas = 0;

	val = strdup(devinf);

	if (debug > 4)
		printf("parse_device: Parsing %s = %s\n", devname, devinf);

	devd = (struct devdetails *)malloc(sizeof(struct devdetails));
	if (!devd) {
		printf("Cannot malloc for struct devdetails\n");
		exit(1);
	}
	devd->devd_next = NULL;

	/* Get the install-section-name */

	install_sect = mstrtok(val, ',');
	trimright(install_sect);

	/* Okay ... the last item id the ID string, and the number
	 * of commas - 1 is the rank
	 */

	while (p = mstrtok(NULL, ',')) {
		commas++;
		last = p;
	}

	if (commas == 0) {
		printf("WARNING: Comma expected in device definition\n%s = %s\n",
		       devname, devinf);
		warning++;
		if (!install_sect) {
			free(val);
			return(NULL);
		}
		
		printf("WARNING: Install section assumed is %s\n", install_sect);
	}

	if (debug > 4)
		printf("parse_device: Id = %s, Rank = %d, line %s\n",
		       last, commas - 1, devinf);

	while(last && *last && isspace(*last))
		last++;

	if (last && *last) {
		devd->devd_id = strdup(last);
		devd->devd_rank = commas - 1;
	} else {
		devd->devd_id = notset;
		devd->devd_rank = - 1;
	}
	
	devd->devd_install = install_section(install_sect);
	free(val);
	return(devd);
}

/*
 * Get the device section config
 *
 * [device-section]
 * device-description = install-section-name, device-id [, comaptible-id, ..]
 *
 */
struct device *
device_section(char *section_name)
{
	struct section *s;
	struct def *d;
	char *install_sect, *dev_name;
	struct device *dev, *dev_head = NULL;
	struct devdetails *details;

	if (debug > 4)
		printf("device_section: [%s]\n", section_name);

	s = find_sect(section_name);
	if (!s) {
		printf("WARNING: Section %s not found\n", section_name);
		warning++;
		return(NULL);
	}

	if (s->sect_device)
		return(s->sect_device);

	d = s->sect_defs;
	if (!d) {
		printf("WARNING: No %s definitions found\n", section_name);
		warning++;
		return(NULL);
	}
	
	while (d) {

		if (d->def_type != ASSIGNMENT) {
			printf("WARNING: Incorrect %s definition, expected assignments\n",
				section_name);
			warning++;
			d = d->def_next;
			continue;
		}

		/* Get the device name  but dump the quotes */

		dev_name = strdup(resolve_value(d->def_name));

		if (*dev_name && *dev_name == '"') {
			dev_name++;
			*(dev_name + strlen(dev_name) - 1) = 0;
		}

		if (debug > 4)
			printf("device_section: Device: %s\n", dev_name);

		details = parse_device(d->def_name, d->def_value);

		if (!details) {
			printf("WARNING: Device %s skipped ..\n", dev_name);
			warning++;
			d = d->def_next;
			continue;
		}

		/* 
		 *Is this a new device ... or an alternate definition
		 * for an existing one ?
		 */

		dev = dev_head;
		while (dev) {
			if (strcmp(dev_name, dev->dev_name) == 0)
				break;
			dev = dev->dev_next;
		}

		if (!dev) {
			/* It's a new device */
			dev = (struct device *)malloc(sizeof(struct device));
			if (!dev) {
				printf("Can't malloc for struct device\n");
				exit(1);
			}
			dev->dev_name = dev_name;
			dev->dev_details = details;
			dev->dev_next = dev_head;
			dev_head = dev;
		} else {
			/* Existing device - new definition */
			details->devd_next = dev->dev_details;
			dev->dev_details = details;
		}

		d = d->def_next;
	}
	s->sect_device = dev_head;
	return(dev_head);
}

/*
 * For each defined manufacturer, create the modem config
 *
 * [Manufacturer]
 * manufacturer-name | %strings-key% = device-section-name
 *
 */
manufacturer_section()
{
	struct section *manuf;
	struct def *d;
	struct manuf *m;
	char *p;

	/*
	 * Find the section name ... for this manufacturer
	 */

	manuf = find_sect(MANUFACTURER);
	if (!manuf) {
		printf("manufacturer_section: Couldn't find the %s section\n", MANUFACTURER);
		warning++;
		return(NULL);
	}

	d = manuf->sect_defs;
	if (!d) {
		printf("manufacturer_section: No %s definitions found\n", MANUFACTURER);
		warning++;
		return(NULL);
	}
	
	while (d) {

		if (d->def_type != ASSIGNMENT) {
			printf("manufacturer_section: Incorrect %s definition, expected assignments\n",
				MANUFACTURER);
			warning++;
			d = d->def_next;
			continue;
		}

		if (debug > 4)
			printf("manufacterer_section: name %s,  section %s\n",
			       resolve_value(d->def_name), d->def_value);

		m = (struct manuf *)malloc(sizeof(struct manuf));
		if (!m) {
			printf("Couldn't malloc for struct manuf\n");
			exit(1);
		}

		m->man_name = strdup(resolve_value(d->def_name));

		if (*(m->man_name) == '"') {
			m->man_name++;
			*(m->man_name + strlen(m->man_name) - 1) = 0;
		}
		
		/* Parse the devices section */

		m->man_dev = device_section(d->def_value);

		/* Link in this manufacturer section definition */

		m->man_next = manufacturer;
		manufacturer = m;

		d = d->def_next;
	}
}

/*
 * Parse the modem config
 */
parse_config()
{
	validate_config();

	manufacturer_section();

	if (debug > 0)
		dump_manuf();
}


/*
 * Format a HKR value for output
 */
char *
format_value(char *v)
{
	char op[1024];
	char *p = op;
	int quotes = 0;

	/*
	 *  Remove surrounding quotes, also %% maps to % ... also
	 * skip <cr> ... we don't need that 
	 */

	/*printf("Was %s now ", v);*/
	if (*v == '"') {
		quotes = 1;
		v++;
	}


	while (*v) {
		if (strncmp(v, "<cr>", 4) == 0) {
			v += 4;
			continue;
		}

		*p = *v;

		if (*v == '%' && *(v + 1) == '%')
			v++;
		p++;
		v++;
	}
	if (quotes)
		*(p - 1) = 0;
	else
		*p = 0;

	/*printf("%s\n", op);*/
	return(op);
}

/*
 * Generate the AT dial strings 
 */
gen_dialstrings(FILE *fp, struct install *i)
{
	struct hkr *hkr;
	struct specent *sp = spec;

	/* 
	 * Now set the spec for this device
	 */

	reset_spec();

	while (i) {
		/*		printf("AddReg Section : %s\n", i->inst_addreg->ar_name);*/
		hkr = i->inst_addreg->ar_hkr;

		while (hkr) {
			spec_update(hkr);
			hkr = hkr->hkr_next;
		}
		
		i = i->inst_next;
	}

	/*
	 * Read the updated spec and ouput the strings
	 */
	while (sp) {
		fprintf(fp, "%s	", sp->se_fname);

		hkr = sp->se_hkr;
		while (hkr) {
			if (hkr->hkr_format)
				fprintf(fp, "%s", hkr->hkr_format);
			else if (hkr->hkr_value)
				fprintf(fp, "%s", 
					format_value(hkr->hkr_value));
			/*			else
				fprintf(fp, "NOT SET - Error");*/

			hkr = hkr->hkr_next;
		}
		fprintf(fp, "\n");
		sp = sp->se_next;
	}
	
}

/*
 * Given a modem name, generate a dialer name
 * Remove enclosing quotes.
 */
char *
gen_dialername(char *desc)
{
	char *dialer = (char *)malloc(strlen(desc) + 1);
	char *p = dialer;

	/* Spaces to _  and punctuation to - */

	while (*desc) {
		if (isspace(*desc)) {
			*p = '_';
		} else if (ispunct(*desc)) {
			*p = '-';
		} else {
			*p = *desc;
		}
		p++;
		desc++;
	}

	*p = 0;

	return(dialer);
}

gen_devices(struct device *d)
{
	char *dialer_name;
	struct devdetails *devd;
	FILE *dfp;

	while (d) {
		dialer_name = gen_dialername(d->dev_name);

		if (debug > 0)
			printf("gen_devices: Device %s, dialer %s\n",
			       d->dev_name, dialer_name);

		dfp = fopen(dialer_name, "w");
		if (!dfp) {
			printf("Couldn't fopen '%s' for writing\n", dialer_name);
			exit(1);
		}

		fprintf(dfp, "DIALER	%s\n", dialer_name);
		fprintf(dfp, "DESC	%s\n", d->dev_name);
		
		devd = d->dev_details;

		fprintf(dfp, "DETECT	");
		while (devd) {
			fprintf(dfp, "%s %d ", 
			       devd->devd_id, devd->devd_rank);
			devd = devd->devd_next;
		}
		fprintf(dfp, "\n");

		gen_dialstrings(dfp, d->dev_details->devd_install);

		fclose(dfp);

		d = d->dev_next;
	}
}

char *
mkdirname(char *d)
{
	static char dn[1024];
	char *p = dn;

	while (*d) {
		if (isspace(*d)) {
			*p = '_';
		} else if (ispunct(*d)) {
			*p = '-';
		} else
			*p = *d;
		p++;
		d++;
	}

	*p = 0;

	return dn;
}
	
	  

/*
 * Create a directory if it doesn't already exist
 */
create_dir(char *dir)
{
	struct stat stats;

	/*
	 * First remove unliked characters from the dir name 
	 */

	if (stat(dir, &stats) >= 0) {

		if (!S_ISDIR(stats.st_mode)) {
			printf("Failed  to create dir %s - a file already exists !\n",
			       dir);
			exit(1);
		}

	} else {

		if (errno == ENOENT) {

			if (mkdir(dir, 0755) < 0) {
				printf("mkdir of directory %s failed\n", dir);
				perror("mkdir");
				exit(1);
			}

		} else {
			printf("Failed to stat directory %s\n", dir);
			perror("stat on directory");
			exit(1);
		}
	}

	if (chdir(dir) < 0) {
		printf("Failed to change directory to %s\n", dir);
		perror("chdir");
		exit(1);
	}

	return;
}
	
/*
 * Gen files
 */

gen_manufacturer()
{
	struct manuf *m = manufacturer;
	FILE *vfp;

	while (m) {

		if (debug > 0) {
			printf("gen_manufacturer: Manufacturer %s\n",
			       m->man_name);
		}

		/* Create a directory to store this vendors database */

		create_dir(mkdirname(m->man_name));

		vfp = fopen(VENDOR_FILE, "w");
		if (!vfp) {
			printf("Failed to open Vendor file for writing\n");
			exit(1);
		}
		fprintf(vfp, "{%s}\n", m->man_name);
		fclose(vfp);

		/* Create Vendors directory and description file */

		gen_devices(m->man_dev);

		if (chdir("..") < 0) {
			printf("Failed to change directory to ..\n");
			perror("chdir");
			exit(1);
		}

		m = m->man_next;
	}
}

