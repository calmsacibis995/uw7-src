#ident "@(#)cvbinCdecl.c	5.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1994-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * cvbinCdecl
 * - converts an arbitrary input into a nicely 
 *   formatted, comma- separated list of character 
 *   initializers
 * - the output is used in a C character array declaration
 */
#include <stdio.h>

main(int argc, char *argv[])
{
    int c;
    int count;

    count = 0;
    while ((c = getchar()) != EOF) {
        if (count == 0) {
            printf("'\\%03o'", c);
            count = 1;
        }
        else if (count == 9) {
            printf(", '\\%03o',\n", c);
            count = 0;
        }
        else {
            printf(", '\\%03o'", c);
            count++;
        }
    }

    exit(0);
}
