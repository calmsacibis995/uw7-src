/* copyright "%c%" */
#ident	"@(#)hwmetric:main.c	1.4"
#ident	"$Header$"




/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* main.c - top-level code of the 'hwmetric' command */

#include <string.h>
#include <sys/types.h>
#include <sys/psm.h>
#include <sys/dl.h>
#include <sys/ksynch.h>
#include <sys/ddi.h>

#ifdef HEADERS_INSTALLED
# include <sys/cpumtr.h>
# include <sys/cgmtr.h>
#else /* !HEADERS_INSTALLED */
# include "../cpumtr/cpumtr.h"
# include "../cgmtr/cgmtr.h"
#endif /* !HEADERS_INSTALLED */

#include <stdlib.h>

#include "hwmetric.h"

int Num_cpus = 0;
int Num_cgs = 0;

static const char *Argv0;

/* Display the usage message with a diagnostic. */
static void
usage (const char *diagnostic) {
  pfmterr ("%s.\n\n", diagnostic);

  pfmtact ("Usage:\n");
  pfmtact ("  %s -i [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -a _metric_ [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -A _metric_ [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -D [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -d _metric_ [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -t _bufsize_ _samples_ [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s -T _bufsize_ _samples_ [_meter_[, . . . ]]\n", Argv0);
  pfmtact ("  %s [-k on | off] [-p on | off]\n", Argv0);

  exit (-1);
} /* usage () */

/* Process the command-line option. */
static void
do_option (int option_char, int argc, const char *argv[]) {

  switch (option_char) {
    case 'i':
      switch (argc) {
        case 0: do_info (""); break;
        case 1: do_info (argv[0]); break;
        case 2: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 'a':
    case 'A':
      switch (argc) {
        case 0: usage ("Missing metric");
        case 1: do_activate (option_char == 'A', argv[0], ""); break;
        case 2: do_activate (option_char == 'A', argv[0], argv[1]); break;
        case 3: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 'D':
      switch (argc) {
        case 0: do_deactivate (""); break;
        case 1: do_deactivate (argv[0]); break;
        case 2: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 'd':
      switch (argc) {
        case 0: usage ("Missing metric");
        case 1: do_deactivate_metric (argv[0], ""); break;
        case 2: do_deactivate_metric (argv[0], argv[1]); break;
        case 3: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 't':
      switch (argc) {
        case 0: usage ("Missing bufsize");
        case 1: usage ("Missing samples");
        case 2: do_cpu_trace (argv[0], argv[1], ""); break;
        case 3: do_cpu_trace (argv[0], argv[1], argv[2]); break;
        case 4: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 'T':
      switch (argc) {
        case 0: usage ("Missing bufsize");
        case 1: usage ("Missing samples");
        case 2: do_cg_trace (argv[0], argv[1], ""); break;
        case 3: do_cg_trace (argv[0], argv[1], argv[2]); break;
        case 4: usage ("Extra operand");
        default: usage ("Extra operands");
      }
      break;

    case 'k':
    case 'p':
      if (argc < 1)
        usage ("Missing operand");

      if (option_char == 'k') {
        if (streq (argv[0], "on")) {
          syscall_hooks (TRUE);
        } else if (streq (argv[0], "off")) {
          syscall_hooks (FALSE);
        } else {
          usage ("Invalid operand to -k option");
        }

      } else /* option_char == 'p' */ {
        if (streq (argv[0], "on")) {
          swtch_hooks (TRUE);
        } else if (streq (argv[0], "off")) {
          swtch_hooks (FALSE);
        } else {
          usage ("Invalid operand to -p option");
        }
      }

      if (argc >= 2) {
        if (argv[1][0] != '-')
          usage ("Each option must begin with '-'");

        option_char = argv[1][1];
        if (option_char == '\0')
          usage ("Missing option-letter");
        if (argv[1][2] != '\0')
          usage ("You cannot combine option-letters");

        do_option (option_char, argc - 2, argv + 2);
      }
      break;

    default:
      usage ("Invalid option-letter");
  }
} /* do_option () */

int
main (int argc, const char *argv[]) {
  int option_char;
  pml_type pml;

  Argv0 = argv[0];

  if (argc < 2)
    return 0;

  if (argv[1][0] != '-')
    usage ("The first option must begin with a '-'");

  option_char = argv[1][1];
  if (option_char == '\0')
    usage ("Missing option-letter");
  if (argv[1][2] != '\0')
    usage ("You cannot combine option-letters");

  pml_init (&pml, "");

  do_option (option_char, argc - 2, argv + 2);
  return 0;
} /* main () */
