:
#
#  @(#) evcconf.sh 12.1 95/05/09 
#
# Copyright (C) 1991 The Santa Cruz Operation, Inc.
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
#
#  This shell script will determine if there is a EVC graphics board
#  configured as the primary adapter, find its slot number
#  and build a EVC grafinfo file acoordingly.
#

PATH=/etc:/bin:/usr/bin
export PATH 

GRAFDIR=/usr/X11R6.1/lib/grafinfo/olivetti
TEMPLATE=evc.tmpl
XGI_FILE=evc.xgi

#***********  Board Identification Definitions ***********

OLI_ID="3D89"   # Manufacturer Id  Olivetti
DEC_ID="10A3"   #                  DEC 
EVC_ID="101"    # Product ID       EVC board


#  /etc/eisa_nvm BOARD_ID  returns a list of strings like: 
#      8 OLI1011 -1 -1 ....
#      The 1st field: <slotNr>
#          2nd field: <Vendor code><ProductId><revision>
#      We check for the ProductId "101" (4-6. character) and return the 
#      first slot nr found with that ID.
#
getslot() {
awk \
    'BEGIN { nodata = 1 ; EVC_ID = "101" }
           { ProductID = substr( $2, 4, 3);
             if( ProductID == EVC_ID && nodata == 1 )
             {
                printf("%s ", $1 );
                nodata = 0;
	     }
           }
     END { if (nodata) {
              printf "999" 
           }
         } '
}
#
# createxgi - instantiate grafinfo file
#
createxgi() {
    read SLOT_NR
    if [ $SLOT_NR -ne 999 ]
    then
	rm -f $XGI_FILE
        sed < $TEMPLATE > $XGI_FILE -e \
         "s/@SLOT@/$SLOT_NR/g"
        chown bin $XGI_FILE
        chgrp bin $XGI_FILE
        chmod 444 $XGI_FILE
    fi
}

#main()  


if [ ! -x /etc/eisa_nvm  -o ! -f $GRAFDIR/$TEMPLATE -o ! -c /dev/eisa0 ]
then
	exit 1
fi

cd $GRAFDIR

#
#  Get list of slot descriptions, match ProductId, create xgi file
#
eisa_nvm "BOARD_ID" 2> /dev/null | getslot  | createxgi 

exit 0
