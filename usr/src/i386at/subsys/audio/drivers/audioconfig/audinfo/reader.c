/*
 *	@(#)reader.c	7.1	10/22/97	12:21:51
 */
#include "confdata.h"

int
main()
{
	load_confdata("config.dat");
	dump_confdata();
}
