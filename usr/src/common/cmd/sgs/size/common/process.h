#ident	"@(#)size:common/process.h	1.1"
    /*  process.h contains format strings for printing size information
     *
     *  Different format strings are used for hex, octal and decimal
     *  output.  The appropriate string is chosen by the value of numbase:
     *  pr???[0] for hex, pr???[1] for octal and pr???[2] for decimal.
     */


/* FORMAT STRINGS */

static char *prusect[3] = {
        "%lx",
        "%lo",
        "%ld"
        };

static char *prusum[3] = {
        " = 0x%lx\n",
        " = 0%lo\n",
        " = %ld\n"
        };
