#ident	"@(#)localedef:charmap.c	1.1.1.1"
#include <stdio.h>
#include <sys/types.h>
#include "_colldata.h"
#include "_localedef.h"

/* save codeset name for later retrieval by nl_langinfo from ctype structure */
void
kf_codeset_func(unsigned char *line, void**args)
{
	unsigned char *tline;

	line = skipspace(line);

	tline = line;
	while(isgraph(*tline))
		tline++;
	if(line == tline) {
		diag(ERROR,TRUE,":73:Invalid codeset identifier.\n");
		return;
	}
	savecodeset(line,tline-line);
	if(!EOL(tline))
			diag(WARN, TRUE, extra_char_warn);

}

	

/* These do not provide enough information to do anything useful and so we
	ignore them. 
*/
void kf_mbmin_func(){}
void kf_mbmax_func(){}



/*
	extension to standard to give sufficient information to know
	how to interpret code values given in charmap section

	possible values (meanings):

	encoding none	(straight 8 bit encoding)
	encoding utf-8	(code values will be taken as UNICODE/ISO 10646 values)
	encoding euc m1:d1,m2:d2,m3:d3 (code values are taken as multibyte euc
									including SS2 and SS3 specifiers;
									m1-m3 are memory lengths of codesets 1-3
									and d1-d3 are display lengths a la old
									CSWIDTH specifier)

	If no encoding keyword is given, will default to NONE if no values given 
	over one byte and to utf-8 if values ar eover 1 byte.
*/

void
kf_encoding_func(unsigned char *line, void **args)
{
	extern unsigned char * setupeuc(unsigned char *);
	/* assumes no encoding keyword in defcharmap */
	if(strt_codent.cd_codelink != NULL) {
		diag(FATAL, TRUE, ":47:Encoding specifier must precede CHARMAP specifier\n");
	}
	line = skipspace(line);
	
	if(strncmp((char *)line, "euc", 3) == 0) {
		ENCODING = MBENC_EUC;
		line +=3;
		line = setupeuc(line);	/* set up in ctype structures */
	}
	else if(strncmp((char *)line, "utf-8",5) == 0) {
		ENCODING = MBENC_UTF8;
		_MBYTE = 6;
		line +=5;
	}
	else if(strncmp((char *)line,"none",4) == 0) {
		ENCODING = MBENC_NONE;
		_MBYTE = 1;
		line +=4;
	}
	else {
		diag(FATAL,TRUE,":141:Unknown encoding type %s.",line);
	}

	line = skipspace(line);
	if(!EOL(line))
		diag(WARN, TRUE, extra_char_warn);

}

int default_dispwidth = DEFAULT_DISPWIDTH;

/* 
	extension to standard to specify display widths of characters
	- only allowed for non EUC encodings (euc is more simply handled with 
	  encoding extension)
	
	The syntax is a sequence of lines starting with a line containing the
	keyword WIDTH and ending with one containing the keywords END WIDTH.
	Each line in between has a codepoint specifier followed by whitepsace 
	followed by a display width.  The codepoint specifier can be either a 
	single codepoint or a range of codepoints.  The display width is an 
	integer.
*/
void
kf_WIDTH_func(unsigned char *line_remain, void **args)
{
	struct codent *tcode, *tcode2;
	int bytes, width;

	/* This test only works if soft definitions are processed after user 
	supplied ones */
	if(strt_codent.cd_codelink == NULL) {
		diag(FATAL, TRUE, ":39:CHARMAP specifier must precede WIDTH specifier\n");
	}

	if(ENCODING == MBENC_EUC) {
		diag(FATAL,TRUE,":155:WIDTH specifier illegal with EUC encoding.\n");
	}

	line_remain = skipspace(line_remain);
	if(!EOL(line_remain))
		diag(WARN, TRUE, extra_char_warn);

	while((line_remain = getline()) != NULL) {
		/* normal end point */
		if(strncmp((char *)line_remain,"END WIDTH",9) == 0) {
			line_remain +=9;
			line_remain = skipspace(line_remain);
			if(!EOL(line_remain))
					diag(WARN, TRUE, extra_char_warn);
			return;
		}

		if((tcode = getlocchar(&line_remain,'\0',NULL)) == NULL) {
			diag(ERROR,TRUE,":142:Unknown entry in WIDTH specification\n");
			continue;
		}
		line_remain = skipspace(line_remain);

		/* If ellipsis, then get rest of range */
		if(strncmp((char *)line_remain,"...",3) == 0) {
			line_remain +=3;
			line_remain = skipspace(line_remain);
			if((tcode2 = getlocchar(&line_remain,'\0',NULL)) == NULL) {
				diag(ERROR,TRUE,":142:Unknown entry in WIDTH specification\n");
				continue;
			}
			if(tcode->cd_code > tcode2->cd_code) {
				diag(ERROR,TRUE,":80:Invalid range in WIDTH specification\n");
				continue;
			}
		}
		else tcode2 = tcode;
		
		if(sscanf((char *)line_remain,"%d %n",&width,&bytes) != 1 ||
			width < 0 || width > 0xff) {
			diag(ERROR, TRUE, ":75:Invalid display width argument\n");
			continue;
		}

		line_remain += bytes;

		/* set display width for all codepoints in range */
		for(;;) {
			tcode->cd_dispwidth = width;
			if(tcode == tcode2)
				break;
			tcode = tcode->cd_codelink;
		}

		if(!EOL(line_remain))
			diag(WARN, TRUE, extra_char_warn);
		
	}
	diag(ERROR,TRUE,":90:Missing END WIDTH keyword.\n");
}

void
kf_CHARMAP_func(unsigned char *line_remain, void **args)
{
	mboolean_t errret;
	int num, num2, n, n2;
	wchar_t code;
	unsigned char *name, *name2, *tmp;

	line_remain = skipspace(line_remain);
	if(!EOL(line_remain))
		diag(WARN, TRUE, extra_char_warn);

	while((line_remain = getline()) != NULL) {
		name2 = NULL;
		if(strncmp((char *)line_remain,"END CHARMAP", 11) == 0) {
			line_remain += 11;
			line_remain = skipspace(line_remain);

			if(!EOL(line_remain)) {
				diag(WARN, TRUE,extra_char_warn);
			}
			return;
		}
		if((name = getsymchar(&line_remain)) == NULL) {
			diag(ERROR,TRUE, ":95:Missing symbolic name\n");
			continue;
		}

		/* if ellipsis, get rest of range */
		if(strncmp((char *)line_remain,"...",3) == 0) {
			line_remain += 3;
			line_remain = skipspace(line_remain);
			if((name2 = getsymchar(&line_remain)) == NULL) {
				diag(ERROR, TRUE, ":94:Missing symbolic name after ellipsis\n");
				continue;
			}
			/* Set tmp pointing at the first of a trailing
			 * sequence of decimal digits.
			 */
			tmp = (unsigned char *)strchr((char *)name, '\0');
			while (tmp != name) {
				if (!isdigit(*--tmp)) {
					tmp++;
					break;
				}
			}
			if(strncmp((char *)name,(char *)name2,tmp-name) != 0) {
				diag(ERROR,TRUE,":132:Symbolic name prefixes in range do not match\n");
				continue;
			}

			/* If anything about the names breaks the rules of a range, complain*/
			if(sscanf((char *) tmp,"%d%n",&num,&n) != 1 ||
				sscanf((char *) (name2+(tmp-name)),"%d%n",&num2,&n2) != 1 ||
				num > num2 ||
				*(tmp + n) != '\0' ||
				*(name2 + (tmp-name) + n2) != '\0') {

				diag(ERROR, TRUE, ":78:Invalid or incompatible symbolic names in range\n");
				continue;
			}
			/* make sure using the bigger space for creating the names below */
			if(n2 > n) {
				tmp = name2 + (tmp - name);
				n2 = n;
				name = name2;
			}
				n = n2;
		}
		line_remain = skipspace(line_remain);

		if((code = getnum(&line_remain)) == WEOF) {
			diag(ERROR, TRUE, ":93:Missing or invalid codepoint value\n");
			continue;
		}
		if(name2 != NULL) {
			/* add name code value pairs for whole range */
			while(num <= num2) {
				sprintf((char *) tmp,"%.*d",n,num);
				if(addpair(name,code,softdefn) == NULL) {
					diag(ERROR,FALSE, ":6:Cannot add symbol %s to symbol table\n",name);
					continue;
				}

				num++;
				code++;
			}
		}
		else
			if(addpair(name,code,softdefn) == NULL) {
				diag(ERROR,FALSE, ":6:Cannot add symbol %s to symbol table\n",name);
				continue;
			}
		/* If user has not specified an encoding, default to utf-8 if 
		   multibyte characters are present. */
		if(code > 0xff) {
			if( ENCODING == MBENC_OLD) {
				ENCODING = MBENC_UTF8;
				_MBYTE = 6;
			}
			else if (ENCODING == MBENC_NONE) {
				diag(FATAL,TRUE,":99:Multibyte characters not allowed for encoding NONE\n");
			}
		}
	}
	diag(ERROR,TRUE,":108:No END CHARMAP directive found\n");
}

extern void *kf_comment_list[], *kf_escape_list[];

static struct keyword_func kf_charmap_tab[] = {
	{ "<code_set_name>", kf_codeset_func, NULL},
	{ "<comment_char>", kf_comesc_func, kf_comment_list},
	{ "<encoding>", kf_encoding_func, NULL},
	{ "<escape_char>", kf_comesc_func, kf_escape_list},
	{ "<mb_cur_max>", kf_mbmax_func, NULL},
	{ "<mb_cur_min>", kf_mbmin_func, NULL},
	{ "CHARMAP", kf_CHARMAP_func, NULL},
	{ "WIDTH", kf_WIDTH_func, NULL }
};

struct kf_hdr kfh_charmap = {
	8,
	kf_charmap_tab
};
	
