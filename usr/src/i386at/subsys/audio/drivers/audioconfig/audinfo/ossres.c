/*
**	@(#)ossres.c	7.1	10/22/97	12:21:48
** Utility to read OSS data file and dump all the resources and
** allowable values for a particular card.
**
** The card is specified on the command line by its OSS key or description.
**
** e.g.:     ossres ctl0028		# dumps data for SB16 PnP
**
**           ossres sb16		# dumps data for standard SB16
**
** or even:  ossres "Creative SoundBlaster 32/AWE PnP (type-1)"
**
**
** Keys and descriptions are case-insensitive.
**
**
** Author: Steve Ginzburg, July 22 1997
*/

#define TCL

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "confdata.h"


int find_card(char *id);
void dump_node_resources(FILE *f, int nodenum);


int find_card(char *id)
{
	int menu;

	for(menu = 0; menu < nmenus; menu++)
		{
		if(strcasecmp(menus[menu].key,  id) == 0 ||
		   strcasecmp(menus[menu].name, id) == 0)
			return menu;
		}

	return -1;
}


/*
** Recursively dump all resources needed for this node and all child
** nodes to the given output stream.
*/
void dump_node_resources(FILE *f, int nodenum)
{
	node *n = &nodes[nodenum];

	int resnum, refnum;
	resource *r;

	int i;

	for(resnum = 0; resnum < n->nres; resnum++)
		{
#ifdef TCL
		fprintf(f, "{ ");
#endif

		r = &(n->res[resnum]);

		switch(r->type)
			{
			case RT_PORT:
				fprintf(f,
#ifdef TCL
					"{ TYPE \"IO\" } { DEV \"%s\" } ",
#else
					"IO port for %s\n",
#endif
						n->name);
				/* <HACK> */
				if(r->length == -1)
					r->length = 1;
				/* </HACK> */
				fprintf(f,
#ifdef TCL
				"{ LENGTH %x } { DEFAULT %x } ",
#else
				"%x %x",
#endif
					r->length, r->default_value);
#ifdef TCL
				fprintf(f, "{ OPTIONS {");
#endif
				for(i=0; i < r->choices.n; i++)
					fprintf(f, " %x", r->choices.ints[i]);
#ifdef TCL
				fprintf(f, " } } ");
#else
				fprintf(f, "\n");
#endif
				break;
			case RT_IRQ:
			case RT_DMA:
				fprintf(f,
#ifdef TCL
					"{ TYPE \"%s\" } { DEV \"%s\" } ",
#else
					"%s for %s\n",
#endif
					((r->type == RT_IRQ) ? "IRQ" : "DMA"),
					n->name);
				fprintf(f,
#ifdef TCL
					"{ DEFAULT %d } ",
#else
					"%d",
#endif
					r->default_value);
#ifdef TCL
				fprintf(f, "{ OPTIONS {");
#endif
				for(i=0; i < r->choices.n; i++)
					fprintf(f, " %d", r->choices.ints[i]);
#ifdef TCL
				fprintf(f, " } } ");
#else
				fprintf(f, "\n");
#endif
				break;
			}
#ifdef TCL
		fprintf(f, "} ");
#endif
		}


	for(refnum = 0; refnum < n->nref; refnum++)
		dump_node_resources(f, n->ref[refnum]);
}



main(int argc, char *argv[])
{
	int menu, node;
	FILE *out = stdout;

	if(argc != 2)
		{
		fprintf(stderr, "usage: %s oss_key | make/model\n", argv[0]);
		exit(1);
		}

	load_confdata("config.dat");

	if((menu = find_card(argv[1])) == -1)
		{
		fprintf(out, "{}\n");	/* No such card / not supported */
		exit(0);
		}

#ifdef TCL
	fprintf(out, "{ ");
#endif

	/* Card name */
	fprintf(out,
#ifdef TCL
		"{ DESC \"%s\" } ",
#else
		"%s\n",
#endif
		menus[menu].name);

	/* Bus type */
	fprintf(out,
#ifdef TCL
		"{ BUSTYPE %s } ",
#else
		"%s\n",
#endif
	(nodes[menus[menu].nodenum].options & OPT_PNP) ? "PNP" : "ISA");

	/* oss ID */
	fprintf(out,
#ifdef TCL
		"{ ID %s } ",
#else
		"%s\n",
#endif
	menus[menu].key);

#ifdef TCL
	fprintf(out, "{ RESOURCES { ");
#endif
	dump_node_resources(out, menus[menu].nodenum);
#ifdef TCL
	fprintf(out, "} } ");
#endif

#ifdef TCL
	fprintf(out, "}\n");
#endif

	exit(0);
}



