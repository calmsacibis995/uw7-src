#ident  @(#)mkskel.sh	7.1     97/10/31
#! /bin/sh

cat <<!
/* File created from flex.skl via mkskel.sh */

#include "flexdef.h"

const char *skel[] = {
!

sed 's/\\/&&/g' $* | sed 's/"/\\"/g' | sed 's/.*/  "&",/'

cat <<!
  0
};
!
