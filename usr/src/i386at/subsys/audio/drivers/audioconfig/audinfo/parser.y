%{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "confdata.h"

	char *outputname;
	extern char yytext[];

	int node_number = 0, newnum = 0;

	int verbose = 0;

	node *node_table[1000];
	node *newnodes[1000];
	int newnums[1000] = {0};

	int nmenu = 0;
	menu *menu_table[1000];

	int fd = -1;

#define HASH_MAX 1001

	node *hash_table[HASH_MAX] = {NULL};

	int ndescr = 0;
	char *descr[MAXDESCR];

int yyerror(char *s);
void add_shadow(resource *res, int offs, int len);
node *create_node(int type);
node *add_resource(node *nd, resource *res, int rtype, int num);
void copy_node(node *nd, char *name);
void merge_node(node *nd, node *nd2);
void link_node(node *nd, node *nd2);
void add_dependency(node *nd, char *name);
void add_menu(node *nd, char *key, char *name);
void set_name(node *nd, char *name);
int hash_code(char *s);
node *find_node(char *s);
void set_download(node *nd, char *id, int type, load_defs *opt);
int create_descr(char *s);

void finish(void);

%}
%union {
	int ival;
	char *str;
	intlist *ilist;
	resource *res;
	node *node;
	load_defs *ld;
}

%token IDENTIFIER HEXADECIMAL DECIMAL STR NAME
%token DEVICE OPTION SINGLE PORT LENGTH CHOICE DEFAULT FIXED PNP PSEUDOPNP NOSINGLE STATIC NOSTATIC TOUCHONLY CONTROL
%token IRQ DMA OPTIONAL NOFIXED NOOPTIONAL RANGE SHADOW REQUIRES DETECT CONFLICT
%token ARCH CARD LIKE INCLUDE MENU TYPE ALIAS DOWNLOAD BIN HEX ALIGN
%token BUS ISA ISAX MCA PCI TEXT

%type <ival> HEXA DEC integer option optionlist busid filetype
%type <ilist> integerlist
%type <res> resourceoptions
%type <node> def devicedef archdef carddef deviceopts archopts cardopts
%type <node> includedefs
%type <str> ID STRING
%type <ld> loadopts
%%

program		: deflist
		  {finish();}
		;

deflist	: deflist def
		| /* Empty */
		;

def		: devicedef
		| archdef
		| carddef
		;

devicedef	: DEVICE ID '{' deviceopts '}'
		  {
			$$ = $4;
			set_name($$, $2);
		  }
		;

archdef		:  ARCH ID '{' archopts '}'
		  {
			$$ = $4;
			set_name($$, $2);
		  }
		;

carddef		:  CARD ID '{' cardopts '}'
		  {
			$$ = $4;
			set_name($$, $2);
		  }
		;

deviceopts	: OPTION optionlist ';' deviceopts
		  {
			$$ = $4;
			$$->options = $2;
		  }
		| TEXT STRING ';' deviceopts
		  {
			$$ = $4;
			$$->descr = create_descr($2);
		  }
		| LIKE ID ';' deviceopts
		  {
			$$ = create_node(NT_DEVICE);
			copy_node($$, $2);
			merge_node($$, $4);
		  }
		| BUS busid ';' deviceopts
		  {
			$$ = $4;
			$$->bustype = $2;
		  }
		| PORT DEC '{' resourceoptions '}' deviceopts
		  {
			$$ = add_resource($6, $4, RT_PORT, $2);
		  }
		| IRQ DEC '{' resourceoptions '}' deviceopts
		  {
			$$ = add_resource($6, $4, RT_IRQ, $2);
		  }
		| DMA DEC '{' resourceoptions '}' deviceopts
		  {
			$$ = add_resource($6, $4, RT_DMA, $2);
		  }
		| REQUIRES ID ';' deviceopts
		  {
			$$ = $4;
			add_dependency($$, $2);
		  }
		| DOWNLOAD ID filetype '{' loadopts '}' deviceopts
		  {
			$$ = $7;
			set_download($$, $2, $3, $5);
		  }
		| /* Empty */
		  {
			$$ = create_node(NT_DEVICE);
		  }
		;

optionlist	: option
		| option ',' optionlist
		  {
			$$ = $1 | $3;
		  }
		| /* Empty */
		  {
			$$ = 0;
		  }
		;

option		: FIXED
		  { $$ = OPT_FIXED;}
		| SINGLE
		  { $$ = OPT_SINGLE;}
		| OPTIONAL
		  { $$ = OPT_OPTIONAL;}
		| STATIC
		  { $$ = OPT_STATIC;}
		| CONFLICT
		  { $$ = OPT_CONFLICT;}
		| PNP
		  { $$ = OPT_PNP; }
		| TOUCHONLY
		  { $$ = OPT_TOUCHONLY; }
		| PSEUDOPNP
		  { $$ = OPT_PSEUDOPNP; }
		| NOFIXED
		  { $$ = OPT_NOFIXED;}
		| NOSINGLE
		  { $$ = OPT_NOSINGLE;}
		| NOOPTIONAL
		  { $$ = OPT_NOOPTIONAL;}
		| NOSTATIC
		  { $$ = OPT_NOSTATIC;}
		;

resourceoptions	: LENGTH DEC ';' resourceoptions
		  {
			$$ = $4;
			$$->length = $2;
		  }
		| CHOICE integerlist ';' resourceoptions
		  {
			$$ = $4;
			memcpy((char*)&$$->choices, (char*)$2, sizeof(intlist));
			$$->range_min = -1; /* Override range */
			if ($$->default_value == -1)
			   $$->default_value = $2->ints[0];
		  }
		| RANGE integer ',' integer ',' integer ';' resourceoptions
		  {
			$$ = $8;
			$$->range_min = $2;
			$$->range_max = $4;
			$$->range_align = $6;
			$$->choices.n = 0;
			if ($$->default_value == -1)
			   $$->default_value = $2;
		  }
		| SHADOW integer ',' integer ';' resourceoptions
		  {
			$$ = $6;
			add_shadow($$, $2, $4);
		  }
		| DEFAULT integer ';' resourceoptions
		  {
			$$ = $4;
			$$->default_value = $2;
		  }
		| OPTION optionlist ';' resourceoptions
		  {
			$$ = $4;
			$$->options = $2;
		  }
		| TEXT STRING ';' resourceoptions
		  {
			$$ = $4;
			$$->descr = create_descr($2);
		  }
		| ALIGN integer ';' resourceoptions
		  {
			$$ = $4;
			$$->align = $2;
		  }
		| /* Empty */
		  {
			$$ = (resource*)malloc(sizeof(resource));
			if ($$ == NULL) yyerror("malloc(resource) failed");

			$$->type = 0;
			$$->descr = -1;
			$$->choices.n = 0;
			$$->default_value = -1;
			$$->options = 0;
			$$->length = -1;
			$$->align = -1;
			$$->range_min = $$->range_max = $$->range_align = -1;
			$$->nshadows = 0;
		  }
		;

integerlist	: integer
		  {
			int num = $1;

			$$ = (intlist*)malloc(sizeof(intlist));
			if ($$ == NULL)
			   yyerror("malloc(intlist) failed\n");
			$$->n=1;
			$$->ints[0]=num;
			$$->mask=0;
			if (num >=0 && num<32)
			   ($$->mask) |= (1 << (num));
		  }
		| integer ',' integerlist
		  {
			int i;

			if ($3->n >= INTLIST_MAX)
			   yyerror("intlist overflow");

			/* Insert this value _before_ the existing ones */
			for (i=$3->n;i>0;i--)
			    $3->ints[i] = $3->ints[i-1];
			$3->ints[0] = $1;
			if ($1 >=0 && $1<32)
			   ($3->mask) |= (1 << ($1));
			$3->n++;
			$$=$3;
		  }
		;

integer		: HEXA
		| DEC
		;

archopts	: DETECT ID ';' archopts
		  {
			$$ = $4;
		  }
		| TEXT STRING ';' archopts
		  {
			$$ = $4;
			$$->descr = create_descr($2);
		  }
		| INCLUDE ID includedefs archopts
		  {
			node *nd;

			$$ = $4;
			nd = create_node(NT_ARCH);
			copy_node(nd, $2);
			merge_node(nd, $4);
			merge_node(nd, $3);
			link_node($$, nd);
		  }
		| LIKE ID ';' archopts
		  {
			$$ = create_node(NT_ARCH);
			copy_node($$, $2);
			merge_node($$, $4);
		  }
		| BUS busid ';' archopts
		  {
			$$ = $4;
			$$->bustype = $2;
		  }
		| PORT DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_PORT, $2);
		  }
		| IRQ DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_IRQ, $2);
		  }
		| DMA DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_DMA, $2);
		  }
		| DOWNLOAD ID filetype '{' loadopts '}' archopts
		  {
			$$ = $7;
			set_download($$, $2, $3, $5);
		  }
		| REQUIRES ID ';' archopts
		  {
			$$ = $4;
			add_dependency($$, $2);
		  }
		| /* Empty */
		  {
			$$ = create_node(NT_ARCH);
		  }
		;

loadopts	: NAME STRING ';' loadopts
		   { $$ = $4; $$->name = $2;}
		| TEXT STRING ';' loadopts
		  {
			$$ = $4;
			$$->descr = create_descr($2);
		  }
		| /* Empty */
		  {
			$$ = malloc(sizeof(load_defs));
			$$->name = NULL;
			$$->descr = -1;
		  }
		;

cardopts	: TYPE ID ';' cardopts
		  {
			$$ = create_node(NT_CARD);
			copy_node($$, $2);
			merge_node($$, $4);
		  }
		| DETECT ID ';' cardopts
		  {
			$$ = $4;
		  }
		| TEXT STRING ';' cardopts
		  {
			$$ = $4;
			$$->descr = create_descr($2);
		  }
		| BUS busid ';' cardopts
		  {
			$$ = $4;
			$$->bustype = $2;
		  }
		| LIKE ID ';' cardopts
		  {
			$$ = create_node(NT_CARD);
			copy_node($$, $2);
			merge_node($$, $4);
		  }
		| MENU ID STRING ';' cardopts
		  {
			$$ = $5;
			add_menu($$, $2, $3);
		  }
		| CONTROL STRING ';' cardopts
		  {
			$$ = $4;
			$$->control_string = create_descr($2);
		  }
		| PORT DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_PORT, $2);
		  }
		| IRQ DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_IRQ, $2);
		  }
		| DMA DEC '{' resourceoptions '}' archopts
		  {
			$$ = add_resource($6, $4, RT_DMA, $2);
		  }
		| DOWNLOAD ID filetype '{' loadopts '}' cardopts
		  {
			$$ = $7;
			set_download($$, $2, $3, $5);
		  }
		| REQUIRES ID ';' cardopts
		  {
			$$ = $4;
			add_dependency($$, $2);
		  }
		| INCLUDE ID includedefs cardopts
		  {
			node *nd;

			$$ = $4;
			nd = create_node(NT_CARD);
			copy_node(nd, $2);
			merge_node(nd, $4);
			merge_node(nd, $3);
			link_node($$, nd);
		  }
		| OPTION optionlist ';' cardopts
		  {
			$$ = $4;
			$$->options = $2;
		  }
		| /* Empty */
		  {
			$$ = create_node(NT_CARD);
		  }
		;

includedefs	: '{' deviceopts '}'
		  { $$ = $2; }
		| ';'
		  { $$ = NULL;}
		;

busid		: ISA {$$ = BT_ISA;}
		| ISAX {$$ = BT_ISAX;}
		| MCA {$$ = BT_MCA;}
		| PCI {$$ = BT_PCI;}
		;

filetype	: BIN
		  {$$=FT_BIN;}
		| HEX
		  {$$=FT_HEX;}
		;

/* Pseudo tokens */

HEXA		: HEXADECIMAL
			{
				int i; if (sscanf(yytext, "%x", &i)!=1)
			 	yyerror("Invalid hexadecimal value");
				$$ = i;
			}
		;
DEC		: DECIMAL
			{
				int i=0; if (sscanf(yytext, "%d", &i)!=1)
			 	yyerror("Invalid decimal value");
				$$ = i;
			}
		;

ID		: IDENTIFIER
		     {
			$$ = (char*)malloc(strlen(yytext)+1);
			if ($$ == NULL)
			   yyerror("malloc(string) failed");
			strcpy($$, yytext);
		     }

STRING		: STR
		     { /* Copy string, remove quotes */
			int i, l;
			extern int linecount;

			$$ = (char*)malloc(strlen(yytext));
			if ($$ == NULL)
			   yyerror("malloc(string) failed");
			strcpy($$, yytext+1);
			l = strlen($$);
			$$[l-1] = 0;

			for (i=0;i<l;i++)
			    if($$[i] == '\n')
				linecount++;
		     }
%%

int
yyerror(char *s)
{
	extern int linecount;

	fprintf(stderr, "Line %d: %s near %s\n", linecount, s, yytext);
	exit(-1);
}

int
main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <outputfile>\n", argv[0]);
		exit(1);
	}

	outputname = argv[1];

	yyparse();

	exit(0);
}

void
add_shadow(resource *res, int offs, int len)
{
	if (res->nshadows >= 8)
	   yyerror("shadow port overflow");

	res->shadows[res->nshadows].offset=offs;
	res->shadows[res->nshadows].len=len;
	res->nshadows++;
}

node*
create_node(int type)
{
	node *nd;

	if (node_number >= 1000)
	   yyerror("node table overflow");

	node_table[node_number] = nd = (node*)malloc(sizeof(node));
	if (nd == NULL)
	   yyerror("malloc(node) failed\n");

	nd->type = type;
	nd->descr = -1;
	nd->control_string = -1;
	nd->hl = NULL;
	nd->bustype = 0;
	nd->name[0] = 0;
	nd->number = node_number++;
 	nd->newnum = -1;
	nd->options = 0;
	nd->nres = 0;
	nd->nref = 0;

	nd->dl.type=0;
	nd->dl.driver[0]=0;
	nd->dl.filename[0]=0;

	return nd;
}

void
merge_resource(resource *res, resource *res2)
{
	int i;

	res->options &= (OPT_FIXED|OPT_OPTIONAL|OPT_SINGLE|OPT_PNP|OPT_PSEUDOPNP||OPT_STATIC|OPT_CONFLICT|OPT_TOUCHONLY);

	res->options |= res2->options;

	if (res->options & OPT_NOFIXED) res->options &= ~(OPT_FIXED|OPT_NOFIXED);
	if (res->options & OPT_NOSINGLE) res->options &= ~(OPT_SINGLE|OPT_NOSINGLE);
	if (res->options & OPT_NOOPTIONAL) res->options &= ~(OPT_OPTIONAL|OPT_NOOPTIONAL);
	if (res->options & OPT_NOSTATIC) res->options &= ~(OPT_STATIC|OPT_NOSTATIC);
    
	if (res2->choices.n)
	   memcpy((char*)&res->choices, (char*)&res2->choices, sizeof(intlist));

	if (res2->descr != -1)
	   res->descr = res2->descr;

	if (res2->default_value != -1)
	   res->default_value = res2->default_value;

	if (res2->length != -1)
	   res->length = res2->length;

	if (res2->align != -1)
	   res->align = res2->align;

	if (res2->range_min != -1)
	{
		res->range_min = res2->range_min;
		res->range_max = res2->range_max;
		res->range_align = res2->range_align;
	}

	for (i=0;i<res2->nshadows;i++)
	{
	   if (res->nshadows >= 8)
	      yyerror("Shadow port overflow (copy)");

	   res->shadows[res->nshadows].offset = res2->shadows[i].offset;
	   res->shadows[res->nshadows].len = res2->shadows[i].len;
	   res->nshadows++;
	}
}

node *
add_resource(node *nd, resource *res, int rtype, int num)
{
	int i;

	if (nd->nres >= 8)
	   yyerror("Resource overflow");

	res->type = rtype;
	res->num = num;

	for (i=0;i<nd->nres;i++)
	if (nd->res[i].type == rtype && nd->res[i].num == num)
	{ /* Duplicate */

		merge_resource(&nd->res[i], res);
		return nd;
	}

/*
 * Insert to the beginning of the list
 */
	for (i=nd->nres;i>0;i--)
	   memcpy((char*)&nd->res[i], (char*)&nd->res[i-1], sizeof(resource));

	memcpy((char*)&nd->res[0], (char*)res, sizeof(resource));

	nd->nres++;

	return nd;
}

void
copy_node(node *nd, char *name)
{
	node *nd2;
	int type = nd->type;
	int descr = nd->descr;
	int ctrl = nd->control_string;
	int num = nd->number;

	if ((nd2=find_node(name))==NULL)
	   {
		fprintf(stderr, "Node name %s\n", name);
		yyerror("Undefined node\n");
	   }

	memcpy((char*)nd, (char*)nd2, sizeof(node));
	nd->type = type;
	nd->number = num;
	if (descr != -1)
	   nd->descr = descr;
	if (ctrl != -1)
	   nd->control_string = ctrl;
	nd->hl = NULL;
}

void
merge_node(node *nd, node *nd2)
{
	int i;

	if (nd2 == NULL)
	   return;

	nd->options &= (OPT_FIXED|OPT_OPTIONAL|OPT_SINGLE|OPT_PNP|OPT_PSEUDOPNP|OPT_STATIC|OPT_CONFLICT|OPT_TOUCHONLY);
	nd->options |= nd2->options;

	if (nd->options & OPT_NOFIXED) nd->options &= ~(OPT_FIXED|OPT_NOFIXED);
	if (nd->options & OPT_NOSINGLE) nd->options &= ~(OPT_SINGLE|OPT_NOSINGLE);
	if (nd->options & OPT_NOOPTIONAL) nd->options &= ~(OPT_OPTIONAL|OPT_NOOPTIONAL);
	if (nd->options & OPT_NOSTATIC) nd->options &= ~(OPT_STATIC|OPT_NOSTATIC);

	if (nd2->bustype)
	   nd->bustype = nd2->bustype;

	for (i=0;i<nd2->nres;i++)
	{
	   resource *res = &nd2->res[i];
	   add_resource(nd, res, res->type, res->num);
	}

	if (nd2->dl.type)
	{
	   nd->dl.type = nd2->dl.type;
	   strcpy(nd->dl.driver, nd2->dl.driver);
	   strcpy(nd->dl.filename, nd2->dl.filename);
	}

	if (nd2->descr != -1)
	   nd->descr = nd2->descr;
	if (nd2->control_string != -1)
	   nd->control_string = nd2->control_string;
}

void
link_node(node *nd, node *nd2)
{
	int i;

	if (nd->nref >= 10)
	   yyerror("Node link overflow");

	for (i=nd->nref;i>0;i--)
	    nd->ref[i] = nd->ref[i-1];

	nd->ref[0] = nd2->number;
	nd->nref++;
}

void
add_dependency(node *nd, char *name)
{
}

int
create_descr(char *s)
{
	int n = ndescr++;

	if (ndescr > MAXDESCR)
	   yyerror("Too many description texts");

	descr[n] = s;
	
	return n;
}

void
add_menu(node *nd, char *key, char *text)
{
	if (nmenu >= 1000)
	   yyerror("menu table overflow");

	menu_table[nmenu] = (menu*)malloc(sizeof(menu));

	menu_table[nmenu]->nodenum = nd->number;
	strcpy(menu_table[nmenu]->name, text);
	strcpy(menu_table[nmenu]->key, key);
	nmenu++;
}

int
hash_code(char *s)
{
	int l = strlen(s);

	if (*s == 0)
	   return 0;

	return (s[0]*13+s[l/2]*17+s[l-1]*19) % HASH_MAX;
}

void
set_name(node *nd, char *name)
{
	int hc = hash_code(name);

	strcpy(nd->name, name);

	nd->hl = hash_table[hc];
	hash_table[hc] = nd;
}

node
*find_node(char *s)
{
	node *nd = hash_table[hash_code(s)];

	while (nd)
	{
		if (strcmp(nd->name, s)==0)
		   return nd;

		nd = nd->hl;
	}

	return NULL;
}

void set_download(node *nd, char *id, int type, load_defs *opt)
{
	nd->dl.type = type;
	nd->dl.descr = opt->descr;
	strcpy(nd->dl.driver, id);
	strcpy(nd->dl.filename, opt->name);
}

void
dump_resource(int indent, resource *res)
{
  int i;

  if (!verbose)
     return;

  for (i=0;i<indent;i++)
	printf("\t");

	printf("\t\trestype=%d, num=%d",res->type, res->num);
	printf(", default=%x", res->default_value);
	printf("\n");
}

void
dump_node(int indent, node *nd)
{
  int i;

  if (nd->newnum == -1)
  {
	nd->newnum = newnum++;
	newnums[nd->number] = nd->newnum;
	newnodes[nd->newnum] = nd;
  } 

  if (verbose)
  {
    for (i=0;i<indent;i++)
  	printf("\t");
  
    printf("\t%s: num=%d, bus=%d, type=%d, opt=%x, nres=%d",
  	 nd->name, nd->number, nd->bustype, nd->type, nd->options, nd->nres);
    if (nd->dl.type)
  	printf(", dnld=%s,%d,%s", nd->dl.driver, nd->dl.type, nd->dl.filename);
    printf("\n");
  }

  for (i=0;i<nd->nref;i++)
	dump_node(indent+1, node_table[nd->ref[i]]);
  for (i=0;i<nd->nres;i++)
	dump_resource(indent, &nd->res[i]);
}

int
rescmp(const void *a, const void *b)
{
	resource *r1 = (resource*)a;
	resource *r2 = (resource*)b;

	if (r1->type < r2->type)
	   return -1;

	if (r1->type > r2->type)
	   return 1;

	return r1->num-r2->num;
}

void
save_to_file(char *fname)
{
	filehdr fh;
	int i;

	int p;

	int dtab[MAXDESCR];

	unlink(fname);

	memset((char *)&fh, 0, sizeof(filehdr));

	if ((fd=open(fname, O_WRONLY|O_CREAT, 0644))==-1)
	{
	    perror(fname);
	    exit(-1);
	}

	strcpy(fh.id, "USSCONF");
	fh.endian = 0x12345678;
	strcpy(fh.copying, "Copyright (C) 4Front Technologies, 1996. All rights reserved.");
	fh.menuptr = sizeof(fh);
	fh.nodeptr = fh.menuptr + (nmenu * sizeof(menu));
	fh.descrptr = fh.nodeptr + (ndescr * sizeof(int));

	fh.nmenu = nmenu;
	fh.nnode = newnum;
	fh.ndescr = ndescr;

	fh.version = HDR_VERSION;

	p = 0;

	for (i=0;i<ndescr;i++)
	{
	   dtab[i] = p;
	   p = p + strlen(descr[i]) + 1;
	}

	fh.stringsz = p;
	fh.stringptr = fh.descrptr + ndescr*4;

	if (write(fd, (char*)&fh, sizeof(fh)) != sizeof(fh))
	{
	   perror(fname);
	   exit(-1);
	}

	for (i=0;i<nmenu;i++)
	{
	   menu_table[i]->nodenum = newnums[menu_table[i]->nodenum];
	   if (write(fd, (char*)menu_table[i], sizeof(menu))!=sizeof(menu))
	   {
	   	perror(fname);
	   	exit(-1);
	   }
	}

	for (i=0;i<newnum;i++)
	{
	   int j;
	   node *nd = newnodes[i];
	   
 	   for (j=0;j<nd->nref;j++)
	   {
		nd->ref[j] = newnums[nd->ref[j]];
	   }

	   if (nd->nres > 1)
	      qsort(nd->res, nd->nres, sizeof(resource), rescmp);

	   if (write(fd, (char*)newnodes[i], sizeof(node))!=sizeof(node))
	   {
	   	perror(fname);
	   	exit(-1);
	   }
	}

	if (write(fd, (char*)dtab, ndescr*4)!=(ndescr*4))
	  {
	   	perror(fname);
	   	exit(-1);
	  }

	for (i=0;i<ndescr;i++)
	{
	   int l = strlen(descr[i]) + 1;

	   if (write(fd, descr[i], l)!=l)
	   {
	   	perror(fname);
	   	exit(-1);
	   }
	}

	close(fd);
	fd = -1;
}

int
menucmp(menu **a, menu **b)
{
	return strcmp((*a)->name, (*b)->name);
}

void
finish(void)
{
	int i, n;

	qsort(menu_table, nmenu, sizeof(menu*), menucmp);

	for (i=0;i<nmenu;i++)
	{
		n = menu_table[i]->nodenum;
		if (verbose)
		   printf("Menu %s/%s = '%s'\n", node_table[n]->name, 
						 menu_table[i]->key,
						 menu_table[i]->name);

		dump_node(0, node_table[n]);
	}

	save_to_file(outputname);
}
