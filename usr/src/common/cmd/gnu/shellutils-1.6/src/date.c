/* date - print or set the system date and time
   Copyright (C) 1989, 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Options:
   -u		Display or set the date in universal instead of local time.
   +FORMAT	Specify custom date output format, described below.
   MMDDhhmm[[CC]YY][.ss]	Set the date in the format described below.

   If one non-option argument is given, it is used as the date to which
   to set the system clock, and must have the format:
   MM	month (01..12)
   DD	day in month (01..31)
   hh	hour (00..23)
   mm	minute (00..59)
   CC	first 2 digits of year (optional, defaults to current) (00..99)
   YY	last 2 digits of year (optional, defaults to current) (00..99)
   ss	second (00..61)

   If a non-option argument that starts with a `+' is specified, it
   is used to control the format in which the date is printed; it
   can contain any of the `%' substitutions allowed by the strftime
   function.  A newline is always added at the end of the output.

   David MacKenzie <djm@ai.mit.edu> */

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"

/* This is portable and avoids bringing in all of the ctype stuff. */
#define isdigit(c) ((c) >= '0' && (c) <= '9')

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef STDC_HEADERS
time_t mktime ();
size_t strftime ();
time_t time ();
#endif

int putenv ();
int stime ();

char *xrealloc ();
time_t get_date ();
time_t posixtime ();
void error ();
void set_date ();
void show_date ();
void usage ();

/* putenv string to use Universal Coordinated Time.
   POSIX.2 says it should be "TZ=UCT0" or "TZ=GMT0". */
#ifndef TZ_UCT
#if defined(hpux) || defined(__hpux__) || defined(ultrix) || defined(__ultrix__) || defined(USG)
#define TZ_UCT "TZ=GMT0"
#else
#define TZ_UCT "TZ="
#endif
#endif

/* The name this program was run with, for error messages. */
char *program_name;

/* If nonzero, work in universal (Greenwich mean) time instead of local. */
int universal_time = 0;
/* The time to display, -1 means system time. */
time_t when = -1;

void
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int install_date = 1;
  char *new_date = 0;
  char* format = 0;

  program_name = argv[0];

  while ((c = getopt (argc, argv, "s:un")) != EOF)
    switch (c)
      {
      case 's':
	new_date = optarg;
	break;
      case 'u':
	universal_time = 1;
	break;
      case 'n':
	install_date = 0;
	break;
      default:
	usage ();
      }

  if (universal_time && putenv (TZ_UCT) != 0)
    error (1, 0, "virtual memory exhausted");

  if (new_date)
    {
      when = get_date (new_date, NULL);
      if (when == -1)
	error (1, 0, "invalid new date");
    }

  switch (argc - optind)
    {
    case 0:
      break;
    case 1:
      if (*argv[optind] == '+')
	format = argv[optind] + 1;
      else {
	when = posixtime (argv[optind]);
	if (when == -1)
	  error (1, 0, "invalid new date");
      }
      break;
    default:
      usage ();
    }

  if (when != -1 && install_date && stime (&when) == -1) {
    error (0, errno, "cannot set date");
    when = -1;
  }

  show_date(format);
  exit (0);
}

/* Display the current date and/or time according to the format specified
   in FORMAT, followed by a newline.  If FORMAT is NULL, use the
   standard output format (ctime style but with a timezone inserted). */

void
show_date (format)
     char *format;
{
  struct tm *tm;
  char *out = NULL;
  size_t out_length = 0;

  if (when == -1)
    time (&when);
  tm = localtime (&when);

  if (format == NULL)
    format = "%a %b %e %H:%M:%S %Z %Y";
  do
    {
      out_length += 200;
      out = (char *) xrealloc (out, out_length);
    }
  while (strftime (out, out_length, format, tm) == 0);
  printf ("%s\n", out);
  free (out);
}

void
usage ()
{
  fprintf (stderr, "\
Usage: %s [-n] [-u] [-s new-date] [+FORMAT] [MMDDhhmm[[CC]YY][.ss]]\n",
	   program_name);
  exit (1);
}
