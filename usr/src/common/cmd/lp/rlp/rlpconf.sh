#!/usr/bin/sh
#	%c%
#
##############################################################################
#
#     @(#)rlpconf.sh	1.2
#
# USAGE:  rlpconf<ret>
#
# DESCRIPTION:
#     The rlpconf shell script allows printers to be configured that can be
#     used by the rlp commands (rlpcmd, rlpstat and indirectly lp).
#     The script prompts for the name of the local printer accepting remote
#     print requests. It then prompts for the name of the remote host and the
#     name of the printer on the remote host. It also prompts the user if the
#     printer is to be the local default printer.
#     The printers is then added locally (using lpadmin), and set to the
#     default printer if requested. Finally the printer is added to the rlp
#     remote printer file, which maps the local printer to the remote one
#
# REQUIREMENTS:
#     Before rlp printing or the rlpstat and rlpcmd commands can be used a
#     a .rhosts(SFF) file is needed in the remote machine's /usr/spool/lp
#     directory with the following line:
#          local_host_name    lp
#
# EFFECTS:
#     Adding printers using this shell scripts, does the following:
#      1. Adds a printer using the lpadmin command, with the following
#         command line:
#          lpadmin -p <localPrinter> -i /usr/lib/lp/model/network -v /dev/null
#      2. If requestedi, sets the printer as the default printer using the
#         lpadmin command with the following command line:
#          lpadmin -d <localPrinter>
#      3. Adds a line into the /usr/spool/lp/remote file to map the
#         <localPrinter> onto the <remoteHost> and its <remotePrinter>. The
#         line added is of the form:
#          <localPrinter>: /usr/bin/rlpcmd <remoteHost> lp -d<remotePrinter>
#
# REMOVE:
#     There is no command to remove the printers added by rlpconf, so rlp
#     printers have to be deleted manually by the system administrator. Do the
#     following to delete a rlp printer:
#      1. use lpadmin to delete the printer, using command line:
#          lpadmin -x <localPrinter>
#      2. remove the entry for the printer in the /usr/spool/lp/remote file
#         using a text editor (eg. vi), search for the line containing
#         <localPrinter> and delete it.
#
##############################################################################

PRINTERS=/usr/spool/lp/admins/lp/printers
NETWORK=/usr/lib/lp/model/network
NULLDEV=/dev/null
REMOTE=/usr/spool/lp/remote
RLPCMD=/usr/bin/rlpcmd
LPCMD=/usr/bin/lp
LPADMIN=/usr/lib/lpadmin
LPSTAT=/usr/bin/lpstat
LPENABLE=/usr/bin/enable
LPACCEPT=/usr/lib/accept

LPRINTER=
RPRINTER=
HOST=
DODEF=

add_rlp_printer()
{
   # Add the printer locally using lpadmin

   $LPADMIN -p $LPRINTER -i $NETWORK -v $NULLDEV
   if [ $? != 0 ]
   then
     # failed to add printer, so ensure it has been fully deleted
     #
     $LPADMIN -x $LPRINTER >$NULLDEV 2>$NULLDEV
     echo "ERROR: failed to add printer $LPRINTER"
     exit 1
   fi

   if [ $DODEF -eq 1 ]
   then
     # Set printer as the local default printer
     #
     $LPADMIN -d $LPRINTER
     if [ $? != 0 ]
     then
       echo "WARNING: failed to set $LPRINTER as default"
     fi
   fi

   # Add printer to the rlp remote file
   #

   if [ ! -f $REMOTE ]
   then
     >$REMOTE
   fi

   chmod 644 $REMOTE >$NULLDEV 2>$NULLDEV
   chown lp $REMOTE >$NULLDEV 2>$NULLDEV
   chgrp lp $REMOTE >$NULLDEV 2>$NULLDEV

   echo "$LPRINTER: $RLPCMD $HOST $LPCMD -d$RPRINTER" >> $REMOTE

   if [ $? = 0 ]
   then
     echo ""
     echo "rlp printer $LPRINTER added okay"
   fi

   # Set printer to enabled and accepting print jobs (ignoring result)

   $LPENABLE $LPRINTER >$NULLDEV 2>$NULLDEV
   $LPACCEPT $LPRINTER >$NULLDEV 2>$NULLDEV
}

##############################################################################
#
#  Main Part of shell script rlpconf
#
#  Get printer name and host name until user enters 'q'
#
##############################################################################

while true; do
  tput clear
  echo "\t\t\tRemote rlp Printing Configuration\n"

  echo "Enter information for local printers accepting remote printing requests"
  echo

  # Get the local printer name

  LPRINTER=
  while [ X$LPRINTER = X ]; do
    echo "Please enter the printer name (q to quit): \c"
    read LPRINTER
  done
  if [ $LPRINTER = q ]; then
    exit 0
  else
    if [ -d $PRINTERS/$LPRINTER ]
    then
      tput clear
      echo "ERROR: printer $LPRINTER already exists on system"
      echo "       using lpstat -p to display known printer names:"
      echo ""
      $LPSTAT -p 2>NULLDEV
      exit 1
    fi
  fi

  while true; do
    HOST=
    while [ X$HOST = X ]; do
      echo "\nPlease enter the the name of the remote host : \c"
      read HOST
    done

    # Get the remote printer name

    RPRINTER=
    while [ X$RPRINTER = X ]; do
      echo "Please enter the name of the printer on the remote host \c"
      echo "(q to quit): \c"
      read RPRINTER
    done
    if [ $RPRINTER = q ]; then
      exit 0
    fi

    echo "\n  Printer $RPRINTER is connected to host $HOST\n"
    RESP=
    while [ X$RESP = X ]; do
      echo "Is this correct? (y/n) \c"
      read RESP
    done
    if expr "$RESP" : "^[yY]">$NULLDEV 2>$NULLDEV; then
      DODEF=0
      echo "Would you like $LPRINTER to be the system default printer? \c"
      echo "(y/n) [n]: \c"
      read RESP
      if [ X$RESP = X ]; then
          RESP="n"
      fi
      if expr "$RESP" : "^[yY]">/dev/null 2>&1; then
        DODEF=1
      fi

      break 
    else
      echo "\n\tTry again? (y/n) \c"
      read RESP
      if [ X$RESP = X ]; then
        RESP=y
      fi
      if expr "$RESP" : "^[yY]">$NULLDEV 2>$NULLDEV; then
        echo
      else
        exit 1
      fi
    fi
  done

  # Add the rlp printer

  add_rlp_printer $LPRINTER $RPRINTER $HOST $DODEF

  echo "\nPlease hit <return> to continue"
  read x
done

exit 0
