:
#
#	@(#) qvisconf.sh 11.2 98/01/09
#
# Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right
# to use, modify, and incorporate this code into other products for
# purposes authorized by the license agreement provided they include
# this notice and the associated copyright notice with any such
# product.  The information in this file is provided "AS IS" without
# warranty.
#
# Configure QVision 1024/1280 memory address mode on EISA systems
#

PATH=/etc:/bin:/usr/bin
export PATH 

GRAFDIR=/usr/X11R6.1/lib/grafinfo/compaq

#
#  Find primary QVision slot
#
get_brdid() 
{
  found_it=0
  for BOARD_ID in CPQ3011 CPQ3021 CPQ3111 CPQ3112 CPQ3121 CPQ3122
  do 
    slot_info=`/sbin/resmgr | grep $BOARD_ID` 2> /dev/null 
    if [ "$slot_info" ]; then
      echo $BOARD_ID
      found_it=1
      break
    fi
  done
  if [ $found_it -eq 0 ]; then
    echo ""
  fi
}

#
#  Get high memory address for primary QVision
#
find_address() 
{
  read BOARD_ID
  if [ "$BOARD_ID" != "" ] 
  then
  	/sbin/resmgr -f /stand/resmgr | ( awk -v QVIS=$BOARD_ID '

	BEGIN { FS = "\n"; RS = ""; found = 0; }

	function print_mem() {
		mem = 0
		for(i = 2; i <= NF; i++) {
			if ($i ~ "^[ ]*[A-Z]+.*$") {
				if ($i ~ "MEMADDR") {
					mem = 1
					found = 1
				}
				else {
					mem = 0
				}
			}
			if ((mem == 1) && ($i ~ "0x")) {
				gsub(/MEMADDR/,"",$i);
				gsub(/0x/,"",$i);
				sub(/^[ ]*/,"",$i);
				printf("%s\n", $i)
			} 
		}
		return found
	}

	# Check for QVision EISA board.  If found print memory.
	($0 ~ "BRDBUSTYPE *0x2") && ($0 ~ "BRDID *"QVIS) { 
		if (print_mem() == 1) {
			exit
		}
	}

	END {
		if (found == 0) {
			print "bus-type=isa";
		}

	} ' - ) | xgi_params

  fi

}

#
#  Use flat (linear) memory map if HMA enabled, else banked
#
xgi_params()
{
  awk \
    ' BEGIN {
        hiflag = 0 
        adrs = "a0000"
      } 
      /00000/ { 
        hiflag = 1  
        adrs = $1
      } 
      END { 
        if (hiflag) {
          msn   = substr(adrs,1,(length(adrs))-5)
          news  = sprintf("%04s ",msn )
          him   = substr(news,1,2)
          lom   = substr(news,3,2)
          printf "%s 200000 ", adrs 
          printf "%s ",   lom;
          printf "%s ",   him;
          printf "FLAT TVGA ";
        }
        else {
          printf "%s ", adrs 
          printf "10000 00 00 BANK VGA "
        }
      } '
}

#
#  Create grafinfo file
#
create_xgi()
{
  read BASAD FBLEN HAMLO HAMHI FBTYP CLASS
  if [ $BASAD ]; then
    for ADAPTER in qvis1024 qvis1280
    do
       sed < $ADAPTER.tmpl > $ADAPTER.xgi -e \
         "s/@BASAD@/$BASAD/g
          s/@FBLEN@/$FBLEN/g
          s/@HAMLO@/$HAMLO/g
          s/@HAMHI@/$HAMHI/g
          s/@FBTYP@/$FBTYP/g
          s/@CLASS@/$CLASS/g"
       chown bin $ADAPTER.xgi
       chgrp bin $ADAPTER.xgi
       chmod 444 $ADAPTER.xgi
    done
  fi
}

#main()  


#
#  Exit if not EISA system
#
exec 2>/dev/null
if [ ! -f $GRAFDIR/qvis1024.tmpl ]
then
	exit 1
fi

cd $GRAFDIR

#
#  Create QVision grafinfo file with memory address stored in EISA NVRAM
#
get_brdid | find_address | create_xgi > /dev/null 2>&1
exit 0
