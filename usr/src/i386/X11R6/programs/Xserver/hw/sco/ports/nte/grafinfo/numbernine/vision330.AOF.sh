#!/bin/sh

#
#	@(#) vision330.AOF.sh 11.2 97/10/29 
#
# Copyright (C) 1997 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for purposes 
# authorized by the license agreement provided they include this notice 
# and the associated copyright notice with any such product.  The 
# information in this file is provided "AS IS" without warranty.
# 

PATH=/usr/X/lib/vidconf/AOF/bin:$PATH
vrom | strings | grep -q "9FX Vision 330"
