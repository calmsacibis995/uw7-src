#pragma ident	"@(#)dtm:readdb.c	1.41.2.2"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <stdio.h>
#include <X11/Intrinsic.h>

#include <MGizmo/Gizmo.h>

#include <mapfile.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

#define TOKEN_EOF	(1024 + 1)
#define TOKEN_STRING	(1024 + 2)
#define TOKEN_SEMICOLON	(1024 + 3)
#define TOKEN_COMMA	(1024 + 4)

#define GET_SEMICOLON()		get_this_token(TOKEN_SEMICOLON)
#define GET_BEGIN()		get_this_token(TOKEN_BEGIN)
#define CFILE			(cfcfp->fullpath)

typedef struct {
	char *ptr;		/* ptr to string */
	int  len;		/* length of string */
} TokenValue;

/*
 * This is the maximum # of nesting of includes.
 */
#define MAX_NUM_LEVELS		16

#define TOKEN_BEGIN		0
#define TOKEN_END		1
#define TOKEN_CLASS		2
#define TOKEN_MENU		3
#define TOKEN_DONTCOPY		4
#define TOKEN_INSTANCE		5
#define TOKEN_INCLUDE		6
#define TOKEN_INHERIT		7
#define TOKEN_DONT_CHANGE	8
#define TOKEN_DONT_DELETE	9
#define TOKEN_LOCKED		10
#define TOKEN_TEMPLATE		11
#define TOKEN_INACTIVE		12
#define TOKEN_MANUAL_CLASSING_ONLY	13

static struct KeyWordEntry {
	const char *const name;
	const int   len;
} keywordtbl[] = {
	{ "BEGIN",		5 },
	{ "END",		3 },
	{ "CLASS",		5 },
	{ "MENU",		4 },
	{ "DONTCOPY",		8 },
	{ "INSTANCE",		8 },
	{ "INCLUDE",		7 },
	{ "INHERIT",		7 },
	{ "DONT_CHANGE",	11 },
	{ "DONT_DELETE",	11 },
	{ "LOCKED",		6 },
	{ "TEMPLATE",		8 },
	{ "INACTIVE",		8 },
	{ "MANUAL_CLASSING_ONLY",	20 },
	{ NULL,	0 }
};	/* keyword list */

static int get_token();
static int get_this_token();
static int get_property();
static void add_property(DmFnameKeyPtr fnkp, char *pname, char *value);
static char *get_str();
static char *get_stre();
static int insert_tree();
static DmFnameKeyPtr get_class();
static DmFnameKeyPtr Dm__ReadFileClassDB();
static void print_class(DmFnameKeyPtr fnkp);

/*** Global variables ***/
/*
 * Since one can only read one file at a time, it is ok to have these global
 * variables, for now. Watch out when we have multi-threading.
 */
static DmFnameKeyPtr first;		/* beginning of the file class list */
static DmFclassFilePtr first_file;	/* beginning of the class file list */
static DmFclassFilePtr cfcfp;		/* current class file ptr */
static DmMapfilePtr mp[MAX_NUM_LEVELS];	/* mapped file */
static TokenValue tok;			/* global token struct */
static int level = 0;			/* current file level */

DmFnameKeyPtr
DmGetClassInfo(fnkp, name)
DmFnameKeyPtr fnkp;
char *name;
{
	if (fnkp) {
		for (; fnkp; fnkp=fnkp->next) {
			if (!(fnkp->attrs & (DM_B_OVERRIDDEN | DM_B_DELETED |
				DM_B_CLASSFILE)) && !strcmp(fnkp->name, name))
				return(fnkp);
		}
	}

	/* try built-in classes */
	return((DmFnameKeyPtr)DmStrToFmodeKey(name));
}

/*
 * This routine removes the last class entry from the link list, in the case
 * of an INCLUDE failure.
 */
static void
PopClass(DmFnameKeyPtr del_fnkp)
{
	if (del_fnkp->prev)
		del_fnkp->prev->next = NULL;
}

static void
AddClass(DmFnameKeyPtr new_fnkp)
{
	/* add it to the list */
	if (first) {
		DmFnameKeyPtr fnkp;

		/* find the end of the list */
		for (fnkp=first; fnkp->next; fnkp=fnkp->next);
		fnkp->next = new_fnkp;
		new_fnkp->prev = fnkp;
	}
	else {
		first = new_fnkp;
		first->prev = NULL;
	}

	new_fnkp->next = NULL;

	/* copy class file's perm attrs */
	new_fnkp->attrs |= cfcfp->attrs & DM_B_READONLY;
}

/*
 * This routine removes the last file entry from the link list, in the case
 * of an INCLUDE failure.
 */
static void
PopFile(DmFclassFilePtr del_fcfp)
{
	PopClass((DmFnameKeyPtr)del_fcfp);

	if (first_file) {
		DmFclassFilePtr fcfp;
		DmFclassFilePtr prev;

		for (fcfp=first_file; fcfp->next_file; fcfp=fcfp->next_file)
			if (fcfp->next_file == del_fcfp) {
				fcfp->next_file = NULL;
				free(del_fcfp);
				break;
			}
	}
	else
		first_file = NULL;
}

static void
AddFile(new_fcfp)
DmFclassFilePtr new_fcfp;
{
	/* make sure this is flagged as a class file */
	new_fcfp->attrs |= DM_B_CLASSFILE;

	/* add it to the list */
	if (first_file) {
		DmFclassFilePtr fcfp;

		/* find the end */
		for (fcfp=first_file; fcfp->next_file; fcfp=fcfp->next_file);
		fcfp->next_file = new_fcfp;
	}
	else
		first_file = new_fcfp;

	new_fcfp->next_file = NULL;

	cfcfp = new_fcfp;
	AddClass((DmFnameKeyPtr)new_fcfp);
}

static DmFnameKeyPtr
get_class()
{
	register DmFnameKeyPtr fnkp;
	DmFnameKeyPtr prev_fnkp;
	int toktype;
	char *dtfclass = DmDTProp(FILEDB_PATH, NULL);
	char *name;

	/* initialize class */
	if ((fnkp = (DmFnameKeyPtr)calloc(1, sizeof(DmFnameKeyRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(NULL);
	}

	if ((fnkp->fcp = DmNewFileClass((void *)fnkp)) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		free(fnkp);
		return(NULL);
	}

	/* get name of class */
	if ((fnkp->name = get_str()) == NULL)
		goto err_exit;

	/* check for redefintion */
	if (prev_fnkp = DmGetClassInfo(first, fnkp->name)) {
		/*
		 * Redefinition means the new class is overriding the previous
		 * one.
		 */
		prev_fnkp->attrs |= DM_B_OVERRIDDEN;
	}

	/* Either a class attribute or a comma-separated list of class
	 * attributes or BEGIN is expected at this point.  Note that a
	 * an extraneous comma at the end a list of class attributes is
	 * ignored.  Class attributes must NOT be within quotes.
	 */
	while ((toktype = get_token(&tok)) != TOKEN_END) {
		switch(toktype) {
		case TOKEN_BEGIN:
			goto begin;
		case TOKEN_TEMPLATE:
			fnkp->attrs |= DM_B_TEMPLATE_CLASS;
			break;
		case TOKEN_INACTIVE:
			fnkp->attrs |= DM_B_INACTIVE_CLASS;
			break;
		case TOKEN_MANUAL_CLASSING_ONLY:
			fnkp->attrs |= DM_B_MANUAL_CLASSING_ONLY;
			break;
		case TOKEN_LOCKED:
			fnkp->attrs |= DM_B_LOCKED;
			break;
		case TOKEN_DONT_CHANGE:
			fnkp->attrs |= DM_B_DONT_CHANGE;
			break;
		case TOKEN_DONT_DELETE:
			fnkp->attrs |= DM_B_DONT_DELETE;
			break;
		case TOKEN_COMMA:
			break;
		default:
			goto err_exit;
		}
	}

begin:
	while ((toktype = get_token(&tok)) != TOKEN_END) {
		switch(toktype) {
		case TOKEN_MENU:
			if (get_property(fnkp, DT_PROP_ATTR_MENU) == 0)
				goto err_exit;
			break;
		case TOKEN_DONTCOPY:
			if (get_property(fnkp, DT_PROP_ATTR_DONTCOPY) == 0)
				goto err_exit;
			break;
		case TOKEN_INSTANCE:
			if (get_property(fnkp, DT_PROP_ATTR_INSTANCE) == 0)
				goto err_exit;
			break;
		case TOKEN_EOF:
			Dm__VaPrintMsg(TXT_CLASSDB_EOF, CFILE,
				       MF_LINENO(mp[level]));
			goto err_exit;
			break;
		case TOKEN_STRING:
			{
			/* try property names */
			char *pname = (char *)strndup(tok.ptr, tok.len);
			char *value = get_stre();

			if ((DtGetProperty(&(fnkp->fcp->plist), pname, NULL))
				 == NULL)
			{
				add_property(fnkp, pname, value);
			}
			else {
				Dm__VaPrintMsg(TXT_CLASSDB_DUP_PROP, CFILE,
					       MF_LINENO(mp[level]), pname);
				free(pname);
				free(value);
				goto err_exit;
			}
			} /* case TOKEN_STRING */
			break;
		default:
			Dm__VaPrintMsg(TXT_SYNTAX, CFILE, MF_LINENO(mp[level]));
			goto err_exit;
			break;
		} /* switch */
	}

#ifdef DEBUG
	print_class(fnkp);
#endif

	fnkp->level = level;

	/* set the appropriate bit on to be used by Icon Setup */
	if (strcmp(CFILE, dtfclass))
		fnkp->attrs |= DM_B_SYSTEM_CLASS;
	else
		fnkp->attrs |= DM_B_PERSONAL_CLASS;

	AddClass(fnkp);
	return(fnkp);

err_exit:
	free(fnkp->fcp);
	free(fnkp);
	return(NULL);
}

static DmFnameKeyPtr
get_file()
{
	DmFclassFilePtr save_fcfp;
	DmFclassFilePtr fcfp;

	/* initialize class */
	if ((fcfp=(DmFclassFilePtr)calloc(1,sizeof(DmFclassFileRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(NULL);
	}

	/* get name of class */
	if ((fcfp->name = get_stre()) == NULL)
		goto err_exit;

	/* save current fcfp */
	save_fcfp = cfcfp;

	/*
	 * Must do this first before processing the file, because the class
	 * file entry is served as the header of all the class entries in
	 * the file.
	 */
	AddFile(fcfp);

	/* level must be set for all classes to prevent classes from being
	 * written out to the wrong file during an update, even for files
	 * that are empty or inaccessible.
	 */
	fcfp->level = level;

	/* begin the next level */
	level++;

	/*
	 * Take the timestamp before start reading the file. Otherwise, there
	 * is a small window that another process may changed the file
	 * between the time the file was read and the time the timestamp was
	 * taken.
	 */
	fcfp->last_read_time = time(NULL);

	if (!Dm__ReadFileClassDB(fcfp)) {
		cfcfp = save_fcfp;
		level--;
		return(NULL);
	}
	level--;

	/* restore current fcfp */
	cfcfp = save_fcfp;

	return((DmFnameKeyPtr)fcfp);

err_exit:
	XtFree(fcfp->name);
	free(fcfp);
	return(NULL);
}

static DmFnameKeyPtr
parse_tokens()
{
	int toktype;
	DmFnameKeyPtr local_first_fnkp = NULL;
	DmFnameKeyPtr fnkp;

	while ((toktype = get_token(&tok)) != TOKEN_EOF) {
		switch(toktype) {
		case TOKEN_CLASS:
			/* assume class name */
			if ((fnkp = get_class()) == NULL)
				return(NULL);
			break;
		case TOKEN_INCLUDE:
			if ((fnkp = get_file()) == NULL)
				/* If INCLUDE failed, then just ignore it */
				continue;
			break;
		default:
			Dm__VaPrintMsg(TXT_SYNTAX, CFILE, MF_LINENO(mp[level]));
			return(NULL);
		}

		if (!local_first_fnkp)
			local_first_fnkp = fnkp;

	}

	return(local_first_fnkp);
}

static DmFnameKeyPtr
Dm__ReadFileClassDB(fcfp)
DmFclassFilePtr fcfp;
{
	char *path;
	DmFnameKeyPtr fp = NULL;

	/*
	 * Do env var & desktop prop expansion.
	 * Note: Since desktop property expansion is done here, any
	 * dynamic changes afterward will NOT affect the class files.
	 */
	path = Dm__expand_sh(fcfp->name, DmDTProp, NULL);

	if (*path != '/')
		fcfp->fullpath = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
					"classdb", path, NULL,
					NULL, NULL, 0, (XtFilePredicate)NULL);
	else
		fcfp->fullpath = strdup(path);

	/* May want to try $HOME/%N here */
	if (fcfp->fullpath == NULL) {
		Dm__VaPrintMsg(TXT_CANT_OPEN_CLASSDB_FILE, path);
		PopFile(fcfp);
		return(NULL);
	}

	/* check write permission */
	if (access(fcfp->fullpath, W_OK))
		fcfp->attrs |= DM_B_READONLY;

	if (mp[level] = Dm__mapfile(fcfp->fullpath, PROT_READ, MAP_SHARED)) {
		fp = parse_tokens();
		Dm__unmapfile(mp[level]);
	}

	return(fp);
}

DmFnameKeyPtr
DmReadFileClassDB(filename)
char *filename;
{
	DmFclassFilePtr fcfp;

	/* initialize global variables */
	level = 0;
	first = NULL;
	first_file = NULL;

	/* Make a class file entry for the top level file */
	if ((fcfp = (DmFclassFilePtr)calloc(1, sizeof(DmFclassFileRec)))
		== NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(NULL);
	}

	fcfp->name = strdup(filename);
	AddFile(fcfp);

	if (Dm__ReadFileClassDB(fcfp))
		return(first);
	else
		return(NULL);
}

static int
get_token(token)
TokenValue *token;
{
	register int c;
	char *save = NULL;
	char *nextp;
	int  match_this;

again:
	/* skip white spaces */
	while (isspace(MF_PEEKC(mp[level])) || MF_PEEKC(mp[level]) == '\n')
		MF_NEXTC(mp[level]);

	if (MF_EOF(mp[level]))
		return(TOKEN_EOF);
	c = MF_PEEKC(mp[level]);

	match_this = 0;
	switch(c) {
	case '\'':
		match_this = '\'';
		/* FALLS THROUGH */
	case '"':
		if (!match_this)
			match_this = '"';

		/* get a quoted string */
		MF_NEXTC(mp[level]);
		save = MF_GETPTR(mp[level]);
		Dm__findchar(mp[level], match_this);
		if (MF_GETPTR(mp[level]) == NULL) {
			Dm__VaPrintMsg(TXT_CLASSDB_QUOTE, CFILE,
				       MF_LINENO(mp[level]));
			return(TOKEN_EOF);
		}
		token->ptr = save;
		token->len = MF_GETPTR(mp[level]) - save;
		MF_NEXTC(mp[level]);
		return(TOKEN_STRING);
	case ';':
		/* ';' */
		MF_NEXTC(mp[level]);
		return(TOKEN_SEMICOLON);
	case ',':
		/* ',' */
		MF_NEXTC(mp[level]);
		return(TOKEN_COMMA);
	case '#':
		/* comment */
		Dm__findchar(mp[level], '\n');
		goto again;
	default:
		{
		/* get a keyword? */
		struct KeyWordEntry *kp = keywordtbl;
		int len;

		save = MF_GETPTR(mp[level]);
		while (MF_NOT_EOF(mp[level]) &&
		       !strchr(",;\"", MF_PEEKC(mp[level])) &&
		       !isspace(MF_PEEKC(mp[level])))
				MF_NEXTC(mp[level]);
		len = MF_GETPTR(mp[level]) - save;

		/* lookup a keyword? */
		while (kp->name) {
			if ((len == kp->len) &&
		    	!Dm__strnicmp(kp->name, save, len)) {
				return(kp - keywordtbl);
			}
			kp++;
		}

		/* not a keyword, so just say it is a string */
		token->ptr = save;
		token->len = len;
		return(TOKEN_STRING);
		} /* default */
	} /* switch(c) */
}

static char *
get_str()
{
	if (get_token(&tok) != TOKEN_STRING) {
		Dm__VaPrintMsg(TXT_CLASSDB_STRING, CFILE, MF_LINENO(mp[level]));
		return(0);
	}

	/* should check for escaped chars here */
	return((char *)strndup(tok.ptr, tok.len));
}

static int
get_this_token(toktype)
int toktype;
{
	if (get_token(&tok) != toktype) {
		const char *str;

		switch (toktype) {
		case TOKEN_SEMICOLON:
			str = ";";
			break;
		case TOKEN_COMMA:
			str = ",";
			break;
		default:
			str = keywordtbl[toktype].name;
			break;
		}
		Dm__VaPrintMsg(TXT_CLASSDB_BAD_TOKEN, CFILE,
			       MF_LINENO(mp[level]), str);
		return(0);
	}
	return(1);
}

static char *
get_stre()
{
	char *p;

	if ((p = get_str()) == 0)
		return(0);

	if (GET_SEMICOLON() == 0)
		return(0);
	return(p);
}

static int
get_property(fp, attrs)
DmFnameKeyPtr fp;
DtAttrs attrs;
{
	int toktype;
	char *name;
	char *value;

	while ((toktype = get_token(&tok)) != TOKEN_STRING) {
		switch(toktype) {
		case TOKEN_MENU:
			attrs |= DT_PROP_ATTR_MENU;
			break;
		case TOKEN_DONTCOPY:
			attrs |= DT_PROP_ATTR_DONTCOPY;
			break;
		case TOKEN_EOF:
			Dm__VaPrintMsg(TXT_CLASSDB_EOF, CFILE,
				       MF_LINENO(mp[level]));
			return(0);
		default:
			Dm__VaPrintMsg(TXT_SYNTAX, CFILE,
				       MF_LINENO(mp[level]));
			return(0);
		}
	}

	name = (char *)strndup(tok.ptr, tok.len);

	if ((value = get_str()) == 0)
		return(0);

	if (GET_SEMICOLON() == 0)
		return(0);

	/* put the entry into fp */
	DtAddProperty(&(fp->fcp->plist), name, value, attrs);
	return(1);
}

static int
DmWriteFileClassDB(fcfp)
DmFclassFilePtr fcfp;
{
	DmFnameKeyPtr fnkp;
	int fd;
	FILE *file;
	DmObjectPtr op;
	DtPropPtr pp;
	extern int errno;
	unsigned short ilevel; /* initial file level */

	/* check last modification time */
	/* update last modification time */

	if ((fd = open(fcfp->fullpath, O_WRONLY | O_TRUNC | O_CREAT,
			 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH | S_ISGID))) == -1) {
		Dm__VaPrintMsg(TXT_CANT_OPEN_CLASSDB_FILE, fcfp->fullpath);
		return(-1);
	}

	if (lockf(fd, F_LOCK, 0L) == -1) {
		Dm__VaPrintMsg(TXT_LOCK_FILE, fcfp->fullpath, errno);
		close(fd);
		return(-2);
	}

	if ((file = fdopen(fd, "w")) == NULL) {
		Dm__VaPrintMsg(TXT_CANT_OPEN_CLASSDB_FILE, fcfp->fullpath);
		close(fd);
		return(-3);
	}

	/* Check if current file is empty, if it is not $HOME/.dtfclass.
	 * fcfp->level==fcfp->next->level, if it is empty or if the current
	 * file is $HOME/.dtfclass.  All INCLUDEs and classes within .dtfclass
	 * have a level of 0, the same level as .dtfclass itself.
	 */
	if ((DmFnameKeyPtr)(fcfp) == DESKTOP_FNKP(Desktop) ||
		(fcfp->next && fcfp->level != fcfp->next->level))
	{
	   for(fnkp=fcfp->next, ilevel=fnkp->level; fnkp; fnkp=fnkp->next) {
		if (fnkp->level > ilevel)
			continue;
		if (fnkp->level < ilevel)
			break;
		if (fnkp->attrs & DM_B_CLASSFILE) {
			DmFclassFilePtr fcfp = (DmFclassFilePtr)fnkp;

			if (!(fnkp->attrs & DM_B_BAD_CLASSFILE)) {
				fprintf(file, "INCLUDE\t%s;\n", fcfp->name);
			}
		}
		else if (fnkp->fcp->plist.count) {
			char buffer[64];
			char quote;

#define SINGLE_QUOTE	'\''
#define DOUBLE_QUOTE	'"'

			fprintf(file, "CLASS '%s'", fnkp->name);
			if ((fnkp->attrs & DM_B_DONT_CHANGE) &&
			  (fnkp->attrs & DM_B_DONT_DELETE))
				fprintf(file, " LOCKED");
			else if (fnkp->attrs & DM_B_DONT_CHANGE)
				fprintf(file, " DONT_CHANGE");
			else if (fnkp->attrs & DM_B_DONT_DELETE)
				fprintf(file, " DONT_DELETE");

			if (fnkp->attrs & DM_B_MANUAL_CLASSING_ONLY)
				fprintf(file, " MANUAL_CLASSING_ONLY");
			if (fnkp->attrs & DM_B_TEMPLATE_CLASS)
				fprintf(file, " TEMPLATE");
			if (fnkp->attrs & DM_B_INACTIVE_CLASS)
				fprintf(file, " INACTIVE");

			fprintf(file, "\nBEGIN\n");
			pp = DtFindProperty(&(fnkp->fcp->plist), 0);
			while (pp) {
				/* Skip properties translated to a specific
				 * locale.  The original English value that
				 * is stored in properties marked DT_ATTR_PROP
				 * _BACKUP will be written out instead.  This
				 * is to preserve the original English property
				 * values that are needed for file classing.
				 */
				if (!(pp->attrs & DT_PROP_ATTR_TRANSLATED)) {
				quote = strchr(pp->value, '"') ?
					SINGLE_QUOTE : DOUBLE_QUOTE;
				fprintf(file, "\t%s%s\t%c%s%c;\n",
					DtAttrToString(pp->attrs, buffer),
					pp->name, quote, pp->value, quote);
				}
				pp = DtFindProperty(NULL, 0);
			}
			fprintf(file, "END\n\n");
#undef SINGLE_QUOTE
#undef DOUBLE_QUOTE
		}
	   }
	}
	fclose(file);
	return(0);
}

int
DmWriteFileClassDBList(fnkp)
DmFnameKeyPtr fnkp;
{
#define CHANGED_CLASSFILE	(DM_B_CLASSFILE | DM_B_WRITE_FILE)


	/* loop through the list to find file structs to be updated */
	for (;fnkp; fnkp=fnkp->next)
		if ((fnkp->attrs & CHANGED_CLASSFILE) == CHANGED_CLASSFILE) {
			DmWriteFileClassDB((DmFclassFilePtr)fnkp);
			fnkp->attrs &= ~DM_B_WRITE_FILE;
		}
}

#ifdef DEBUG
static void
print_class(fnkp)
DmFnameKeyPtr fnkp;
{
	printf("CLASS %s\nBEGIN\n", fnkp->name);
	DtPrintPropList(&(fnkp->fcp->plist));
	printf("END\n");
}
#endif

/****************************procedure*header*****************************
 add_property - Adds a file class property.  If property is _PATTERN or
 _FILEPATH, set the appropriate attributes if the property value is 
 translated.
 */
static void
add_property(DmFnameKeyPtr fnkp, char *pname, char *value)
{
	char *dayone_name;

	if (!strcmp(pname, "_FILEPATH") || !strcmp(pname, "_PATTERN")) {
		dayone_name = Dm_DayOneName(value, DESKTOP_LOGIN(Desktop));
		if (dayone_name) {
			if (strcmp(dayone_name, value)) {
				/* Mark translated property so that they will
				 * be skipped when writing properties to disk.
		   		 */
				DtAddProperty(&(fnkp->fcp->plist), pname,
					dayone_name, DT_PROP_ATTR_TRANSLATED);

				/* Back up original property value to be used
				 * when writing properties to disk.
		   		 */
		  		DtAddProperty(&(fnkp->fcp->plist), pname,
					value, DT_PROP_ATTR_BACKUP);
			} else {
				DtAddProperty(&(fnkp->fcp->plist), pname,
					value, 0);
			}
			FREE(dayone_name);
		} else {
			DtAddProperty(&(fnkp->fcp->plist), pname, value, 0);
		}
	} else {
		DtAddProperty(&(fnkp->fcp->plist), pname, value, 0);
	}
}
