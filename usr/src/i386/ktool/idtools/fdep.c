#ident	"@(#)ktool:i386/ktool/idtools/fdep.c	1.5.3.1"
#ident	"$Header:"

#include "inst.h"
#include "devconf.h"
#include "defines.h"
#include <stdio.h>

void fdep_pr_depend(), fdep_pr_interface();

/*
 * In a cross-environment, make sure this header is for the target system
 */
#include <sys/elf.h>


void
fdep_prsec(fp, drv)
FILE *fp;
driver_t *drv;
{
	char *pfx;

	pfx = drv->mdev.prefix;

	fprintf(fp, "\t.section\t.mod_dep,\"aw\",%d\n", SHT_MOD);
	fprintf(fp, "\t.globl\t%s_wrapper\n", pfx);
	fprintf(fp, "\t.long\t%s_wrapper\n", pfx);

	/*
	 * First section (one string) is the list of module dependencies.
	 */

	fdep_pr_depend(fp, drv);

	/*
	 * Next section is the interface requirements.
	 * For each interface, there is one string for the interface name,
	 * followed by a string for each version, terminated by a null string.
	 * The list of interfaces is terminated by another null string.
	 */

	fdep_pr_interface(fp, drv);

}


void
fdep_pr_depend(fp, drv)
FILE *fp;
driver_t *drv;
{

	struct depend_list *dep;

	/*
	 * This section (one string) is the list of module dependencies.
	 */
	fprintf(fp, "\t.string\t\"");
	for (dep = drv->mdev.depends; dep != NULL; dep = dep->next)
		fprintf(fp, " %s", dep->name);
	fprintf(fp, "\"\n");

}


void
fdep_pr_interface(fp, drv)
FILE *fp;
driver_t *drv;
{
	struct interface_list *ilp;
	unsigned int nver;

	/*
	 * these Next sections are the interface requirements.
	 * For each interface, there is one string for the interface name,
	 * followed by a string for each version, terminated by a null string.
	 * The list of interfaces is terminated by another null string.
	 */

	for (ilp = drv->mdev.interfaces; ilp != NULL; ilp = ilp->next) {
		fprintf(fp, "\t.string \"%s\"\n", ilp->name);
		for (nver = 0; ilp->versions[nver] != NULL; nver++) {
			fprintf(fp, "\t.string \"%s\"\n", ilp->versions[nver]);
		}
		fprintf(fp, "\t.string \"\"\n");
	}

	fprintf(fp, "\t.string \"\"\n");
}

