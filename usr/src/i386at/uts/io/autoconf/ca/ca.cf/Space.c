#ident	"@(#)kern-i386at:io/autoconf/ca/ca.cf/Space.c	1.1"
#ident	"$Header$"

#include <sys/param.h>
#include <sys/types.h>

#define	LOW_TO_HIGH	0x00
#define	HIGH_TO_LOW	0x01

uint_t ca_config_order = LOW_TO_HIGH;
