:
#
# @(#) ncrvidtyp.sh 12.1 95/05/09 
#
# Copyright (C) 1992 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for 
# purposes authorized by the license agreement provided they include
# this notice and the associated copyright notice with any such 
# product.  The information in this file is provided "AS IS" without 
# warranty.
#
#
# Copyright (c) 1992 NCR Corporation, Dayton Ohio, USA

#	NCR VGA configuration script


PATH=/usr/X11R6.1/lib/vidconf/scripts:/etc:/bin:/usr/bin
export PATH


ncr_vidtyp
case $? in
    0) 
       rm -f /usr/X11R6.1/lib/grafinfo/ncr/ncrvga.xgi
       ln /usr/X11R6.1/lib/grafinfo/ncr/77c22.graf /usr/X11R6.1/lib/grafinfo/ncr/ncrvga.xgi
       exit 0
       ;;
    2) 
       rm -f /usr/X11R6.1/lib/grafinfo/ncr/ncrvga.xgi
       ln /usr/X11R6.1/lib/grafinfo/ncr/77c22e.graf /usr/X11R6.1/lib/grafinfo/ncr/ncrvga.xgi
       exit 0
       ;;
esac

exit $?

