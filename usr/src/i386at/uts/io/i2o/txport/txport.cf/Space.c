#ident  "@(#)Space.c	1.4"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 *
 */

#include <sys/types.h>
#include "config.h"

#define I2O_MAX_HBAS	4
int	i2o_cntls;

/*
 * DEBUG
 */
unsigned long I2O_TRANS_DEBUG = 0;

